#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/inotify.h>
#include <unistd.h>
#include <pthread.h>

#include <sys/types.h>    
#include <sys/stat.h>    
#include <fcntl.h>

#include "queue.h"

typedef struct file_and_type
{
	// char *file;
	int type;//watched logdir or shared dir

	int dtn_type;
	char *payload_dir;
	char *destination_eid;
	char *local_eid;//local eid,for dtn2
	char *dtn_shared_dir;
	char *dtn_log_dir;
	char *program_log;
}file_type;

//the payload dir changed event node
typedef struct paylod_receive_event
{
	int event_type;
	char *payload_filename;
}payload_dir_event;

//the dtn received log queue's node
typedef struct dtn_log_change_event_node
{
	file_type *temp_s;
	char *name;//file name
}dtn_log_event;

void * watchFile(void *s);
void print_usage();
// void send_sharedfile_into_bundle(char* filepath,char *filename,char *temp,int dtn_type);
void  dtn2_send(char *destination_eid,char *local_eid,char *payload_path);
void bytewalla_send(char *destination_eid,char *local_eid,char *payload_path);

#define LOGDIR 1
#define SHAREDDIR 2
#define PAYLOADDIR 3

#define DTN2 0
#define BYTEWALLA 1

//the file int the shared dir which is received file
queue_head *received_file_queue;

//the received bundle id queue
queue_head *received_bundle_id_queue;

//payload dir event,create event=1,close_nowrite event=2
queue_head *payload_dir_event_queue;

int main(int argc, char *argv[])
{
	//init the received files's queue
	received_file_queue=queue_init();
	received_bundle_id_queue=queue_init();
	payload_dir_event_queue=queue_init();

	//init the argument read
	int dtn_type=DTN2;
	char *payload_dir=NULL;
	char *destination_eid=NULL;
	char *local_eid=NULL;
	char *dtn_shared_dir=NULL;
	char *dtn_log_dir=NULL;
	char *program_log=NULL;

	//log in file or in screen
	int log_in_file_or_screen=0;

	//read the arguments
	int c, done = 0;
	while (!done)
	{
	  c = getopt(argc, argv, "m:M:t:T:p:P:d:D:s:S:h:H:l:L:r:R:f:");
	  switch (c)
	  {
	  case 'h':
	  case 'H':
	    print_usage();
	    return -1;

	  //get dtn type
	  case 't':
	  case 'T':
	    dtn_type=atoi(optarg);
	    // printf("%d\n",dtn_type);
	    break;

	  //get payload dir
	  case 'p':
	  case 'P':
	    payload_dir=optarg;
	    // printf("%s\n",optarg);
	    break;

	  //get destination eid
	  case 'd':
	  case 'D':
	    destination_eid=optarg;
	    // printf("%s\n",optarg);
	    break;

	  //get local eid
	  case 'm':
	  case 'M':
	    local_eid=optarg;
	    // printf("%s\n",optarg);
	    break;

	  //get shared dir
	  case 's':
	  case 'S':
	    dtn_shared_dir=optarg;
	    // printf("%s\n",optarg);
	    break;

	  //get dtn received bundle log
	  case 'l':
	  case 'L':
	    dtn_log_dir=optarg;
	    // printf("%s\n",optarg);
	    break;

	  //this program's running log file path
	  case 'r':
	  case 'R':
	    program_log=optarg;
	    // printf("%s",optarg);
	    break;
	  /*case 's':
	  case 'S':
	      byte=DTNSEND;
	      strcpy(buff+2,optarg);
	      break;
	  */

	      //program log in file or in screen
      case 'f':
      	log_in_file_or_screen=atoi(optarg);
		break;
	  case -1:
	      done = 1;
	      break;    

	  default:
	      print_usage();
	      return -1;
	  }
	}

	if(!(	(dtn_type==DTN2 && payload_dir!=NULL && destination_eid!=NULL && dtn_shared_dir!=NULL && dtn_log_dir!=NULL && program_log!=NULL && local_eid!=NULL) ||
		(dtn_type==BYTEWALLA && payload_dir!=NULL && destination_eid!=NULL && dtn_shared_dir!=NULL && dtn_log_dir!=NULL && program_log!=NULL) )	)
	{
		printf("you input the wrong arguments\n");
		print_usage();
		return -1;
	}
  
  	//program log display write in file
  	if(log_in_file_or_screen==0)
  	{
	  	char program_log_filename[128];
	  	memset(program_log_filename,'\0',sizeof(program_log_filename));
	  	sprintf(program_log_filename,"%s/dtn2-bytewalla-app.log",program_log);
	  	printf("logfile:%s\n",program_log_filename);
	  	remove(program_log_filename);
	  	int out=open(program_log_filename,O_CREAT|O_RDWR,S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
	  	if(out>0)
	  	{
	  		dup2(out,fileno(stdout));
	  	}
	  	else
	  	{
	  		printf("redirect stdout to file %s failed \n",program_log_filename);
	  	}
  	}
  	else if(log_in_file_or_screen==1)
  	{
  		fprintf(stdout,"program_log display in screen\n");
  	}
  	else
  	{
  		fprintf(stdout,"-f (log_in_file_or_screen argument is wrong,it can only be 0 or 1)\n");
  		return -1;
  	}


	//watched received bundle log dir
	file_type *f_logdir_type=(file_type *)malloc(sizeof(file_type));
	f_logdir_type->type=LOGDIR;

	f_logdir_type->dtn_type=dtn_type;
	f_logdir_type->dtn_log_dir=dtn_log_dir;
	f_logdir_type->payload_dir=payload_dir;
	f_logdir_type->destination_eid=destination_eid;
	f_logdir_type->dtn_shared_dir=dtn_shared_dir;
	f_logdir_type->program_log=program_log;
	f_logdir_type->local_eid=local_eid;
	
	pthread_t watchLogDir;
	int t=pthread_create(&watchLogDir,NULL,watchFile,f_logdir_type);
	if(t!=0)
	{
		fprintf(stdout,"create thread for watching dtn log dir failed\n");
	}
	else
	{
		pthread_detach(watchLogDir);
	}

	//watched shared dir
	file_type *f_shared_type=(file_type *)malloc(sizeof(file_type));
	f_shared_type->type=SHAREDDIR;

	f_shared_type->dtn_type=dtn_type;
	f_shared_type->dtn_log_dir=dtn_log_dir;
	f_shared_type->payload_dir=payload_dir;
	f_shared_type->destination_eid=destination_eid;
	f_shared_type->dtn_shared_dir=dtn_shared_dir;
	f_shared_type->program_log=program_log;
	f_shared_type->local_eid=local_eid;

	pthread_t watchSharedDir;
	t=pthread_create(&watchSharedDir,NULL,watchFile,f_shared_type);
	if(t!=0)
	{
		fprintf(stdout,"create thread for watching shared dir failed\n");
	}
	else
	{
		// fprintf(stdout,"watching shared dir %s\n",argv[1]);
		pthread_detach(watchSharedDir);
	}


	//watch the payload dir
	file_type *f_payload_type=(file_type *)malloc(sizeof(file_type));
	f_payload_type->type=PAYLOADDIR;

	f_payload_type->dtn_type=dtn_type;
	f_payload_type->dtn_log_dir=dtn_log_dir;
	f_payload_type->payload_dir=payload_dir;
	f_payload_type->destination_eid=destination_eid;
	f_payload_type->dtn_shared_dir=dtn_shared_dir;
	f_payload_type->program_log=program_log;
	f_payload_type->local_eid=local_eid;

	pthread_t watchPayloadDir;
	t=pthread_create(&watchPayloadDir,NULL,watchFile,f_payload_type);
	if(t!=0)
	{
		fprintf(stdout,"create thread for watching shared dir failed\n");
	}
	else
	{
		// fprintf(stdout,"watching shared dir %s\n",argv[1]);
		pthread_detach(watchPayloadDir);
	}


	while(1)
	{
		//received bundle id queue isn't null
		while(queue_empty(received_bundle_id_queue)==0)
		{
			queue_node *father_event=queue_get(received_bundle_id_queue);
			dtn_log_event *event=father_event->content;
			file_type *temp_s=event->temp_s;

			//solve the event
			// remove dtn log file
			char *file=temp_s->dtn_log_dir;
			char removefilename[128];
			memset(removefilename,'\0',sizeof(removefilename));
			sprintf(removefilename,"%s/%s",file,event->name);

			if(remove(removefilename)!=0)
			{
				fprintf(stdout,"remove log file %s failed\n",removefilename);
			}
			else
			{
				fprintf(stdout,"remove log file %s successfully\n",removefilename);
			}

			// parse payload
			char payload_filename[256];//fila path and file name
			memset(payload_filename,'\0',sizeof(payload_filename));

			char _payload_filename[256];//only the payload file name
			memset(_payload_filename,'\0',sizeof(_payload_filename));

			if(temp_s->dtn_type==BYTEWALLA)
			{
				sprintf(payload_filename,"%s/payload_%s",temp_s->payload_dir,event->name);
				sprintf(_payload_filename,"payload_%s",event->name);
			}
			else if(temp_s->dtn_type==DTN2)
			{
				sprintf(payload_filename,"%s/bundle_%s.dat",temp_s->payload_dir,event->name);
				sprintf(_payload_filename,"bundle_%s.dat",event->name);
			}
			else
			{
				fprintf(stdout,"wrong dtn_type:%d",temp_s->dtn_type);
			}

			//check payload file exist --- start
			fprintf(stdout,"check the payload file:%s if it is ready\n",_payload_filename);
			int ready=0;
			while(queue_empty(payload_dir_event_queue)==0)
			{
				queue_node *_node=queue_get(payload_dir_event_queue);
				payload_dir_event *node=_node->content;

				//find the create event
				if(node->event_type!=1 || strcmp(node->payload_filename,_payload_filename)!=0)
				{
					fprintf(stdout,"this event isn't the create event: event_type=%d  filename=%s \n",node->event_type,node->payload_filename);
					free(node);
					continue;
				}
				else
				{
					ready=1;
					break;
				}
					

				//find the payload file's create event
				/*if()
				{
					fprintf(stdout,"this event isn't the create event: event_type=%d  filename=%s \n",node->event_type,node->payload_filename);
					free(node);
					continue;
				}*/

				// free(node);
			}

			if(ready!=1)
			{
				fprintf(stdout,"error: payload(%s) hasn't been created \n",_payload_filename);
				continue;
			}
			ready=0;


			fprintf(stdout,"payload file %s has created,now wait for it's ready to read\n",_payload_filename);
			//wait for payload ready for 20s
			int seconds=20;
			while(seconds>0)
			{
				while(queue_empty(payload_dir_event_queue)==0)
				{
					queue_node *_node=queue_get(payload_dir_event_queue);
					payload_dir_event *node=_node->content;

					//find the close_nowrite event
					if(node->event_type==2 && strcmp(node->payload_filename,_payload_filename)==0)
					{
						ready=1;
						free(node);
						fprintf(stdout,"payload file %s is ready\n",_payload_filename);
						break;
					}
					else
					{
						free(node);
					}
				}

				if(ready==1)
					break;

				fprintf(stdout,"wait for payload file %s --- %ds \n",_payload_filename,seconds);
				sleep(1);
				--seconds;
			}

				/*if(ready==1)
					break;*/
			
				//can't find the payload file's close no_write event
			if(ready!=1)
			{
				fprintf(stdout,"waiting for payload file %s time out\n",_payload_filename);
				continue;
			}
			//check payload file exist --- end

			FILE *payload_file=fopen(payload_filename,"r");

			if(payload_file==NULL)
			{
				fprintf(stdout,"payload %s do not exsists\n",payload_filename);
				continue;
			}

			char buff[256];
			memset(buff,'\0',sizeof(buff));
			if(fgets(buff,256,payload_file)==NULL)
			{
				// fprintf(stdout,"payload %s do not exsists\n",payload_filename);
				fclose(payload_file);
				continue;
			}
			int num=(int)buff[0]+1;//the first char is the num of filename
			buff[0]='/';

			//get the received file's name ,and put into the received queue
			char *add_queue_file_name=(char *)malloc(sizeof(char)*num);
			strncpy(add_queue_file_name,buff+1,num-1);
			queue_node *node=(queue_node*)malloc(sizeof(queue_node));
			node->content=add_queue_file_name;
			if(queue_add(received_file_queue,node)==0)
			{
				fprintf(stdout,"add file %s to received_file_queue failed \n",add_queue_file_name);
			}
			else
			{
				fprintf(stdout,"add file %s to received_file_queue \n",add_queue_file_name);
			}

			char target_filename[256];
			memset(target_filename,'\0',sizeof(target_filename));
			strcpy(target_filename,temp_s->dtn_shared_dir);
			int start=strlen(temp_s->dtn_shared_dir);
			strncpy(target_filename+start,buff,num);

			FILE *target_file=fopen(target_filename,"w");

			if(target_file==NULL)
				continue;

			fputs(buff+num,target_file);
			while(fgets(buff,256,payload_file)!=NULL)
			{
				fputs(buff,target_file);
			}

			fclose(payload_file);
			fclose(target_file);

			//free member
			free(event);
		}
	}
	return 0;
}

//notify the dir,file:dir ,type:
void * watchFile(void *s)
{
	file_type *temp_s=(file_type *)s;
	char *file;
	if(temp_s->type==LOGDIR)
	{
		file=temp_s->dtn_log_dir;
		fprintf(stdout,"watching dtn log dir %s\n",file);
	}
	else if(temp_s->type==SHAREDDIR)
	{
		file=temp_s->dtn_shared_dir;
		fprintf(stdout,"watching shared dir %s\n",file);
	}
	else if(temp_s->type==PAYLOADDIR)
	{
		file=temp_s->payload_dir;
		fprintf(stdout,"watching payload_dir %s\n",file);
	}
	else
	{
		fprintf(stdout,"can't watching %s dir \n","null");
		return NULL;
	}

	char last_file_name[128];
	memset(last_file_name,'\0',sizeof(last_file_name));
	last_file_name[0]='\0';

	int fd;
	int wd;
	int len;
	int nread;
	char buf[BUFSIZ];
	struct inotify_event *event;
	
	

	fd = inotify_init();
	if( fd < 0 )
	{
		fprintf(stderr, "inotify_init failed\n");
		return NULL;
	}

	wd = inotify_add_watch(fd, file, IN_ALL_EVENTS /*IN_CREATE|IN_CLOSE|IN_MODIFY*/);
	if(wd < 0)
	{
		fprintf(stderr, "inotify_add_watch %s failed\n", file);
		return NULL;
	}

	buf[sizeof(buf) - 1] = 0;
	while( (len = read(fd, buf, sizeof(buf) - 1)) > 0 )
	{
		nread = 0;
		while( len > 0 )
		{
			event = (struct inotify_event *)&buf[nread];

			switch (event->mask & (/*IN_CREATE|IN_CLOSE|IN_MODIFY*/ IN_ALL_EVENTS | IN_UNMOUNT | IN_Q_OVERFLOW | IN_IGNORED))
			{
				/* File was accessed */
				case IN_ACCESS:
				// fprintf(stdout,"file:%s --- event:%s\n",event->name,"IN_ACCESS");
				break;

				/* File was modified */
				case IN_MODIFY:
				// fprintf(stdout,"file:%s --- event:%s\n",event->name,"IN_MODIFY");

				break;

				/* File changed attributes */
				case IN_ATTRIB:
				// fprintf(stdout,"file:%s --- event:%s\n",event->name,"IN_ATTRIB");
				break;

				/* File open for writing was closed */
				//this is the file changed event
				case IN_CLOSE_WRITE:

				//check the duplicated event
				fprintf(stdout,"file:%s --- event:%s\n",event->name,"IN_CLOSE_WRITE");

				/*if(strcmp(last_file_name,event->name)==0)
				{
					memset(last_file_name,'\0',sizeof(last_file_name));
					break;
				}*/

			
				char *filename=event->name;

				if(temp_s->type==SHAREDDIR)
				{
					//check that if it's received file
					if(file_queue_find_and_remove(received_file_queue,event->name)==1)
					{
						fprintf(stdout,"file %s is the received file\n",event->name);
						break;
					}
					else
					{
						fprintf(stdout,"this is not the duplicated file\n");
					}

					//process the file to send
					char send_filename[256];
					memset(send_filename,'\0',sizeof(send_filename));
					char temp_filename[256];
					memset(temp_filename,'\0',sizeof(temp_filename));
					sprintf(send_filename,"%s/%s",file,filename);
					sprintf(temp_filename,"%s/%s.payload",temp_s->program_log,filename);
					FILE *send_file=fopen(send_filename,"r");
					FILE *temp_file=fopen(temp_filename,"w");

					if(send_file==NULL || temp_file==NULL)
						break;

					char buff[256];
					memset(buff,'\0',sizeof(buff));
					buff[0]=strlen(filename);
					strcpy(buff+1,filename);
					fputs(buff,temp_file);
					while(fgets(buff,256,send_file)!=NULL)
					{
						fputs(buff,temp_file);
					}

					fclose(send_file);
					fclose(temp_file);

					if(temp_s->dtn_type==DTN2)
					{
						// fprintf(stdout,"dtn send bundle\n");
						dtn2_send(temp_s->destination_eid,temp_s->local_eid,temp_filename);
					}
					else if(temp_s->dtn_type==BYTEWALLA)
					{
						// fprintf(stdout,"bytewalla send bundle\n");
						bytewalla_send(temp_s->destination_eid,temp_s->local_eid,temp_filename);
					}
					else
					{
						fprintf(stdout,"wrong argumen dtn_type:%d\n",temp_s->dtn_type);
					}

					//remove temp file
				}
				else if(temp_s->type==LOGDIR)
				{
					dtn_log_event *content=(dtn_log_event*)malloc(sizeof(dtn_log_event));
					content->temp_s=temp_s;
					content->name=event->name;
					queue_node *node=(queue_node*)malloc(sizeof(queue_node));
					node->content=content;
					queue_add(received_bundle_id_queue,node);
					fprintf(stdout,"dtn received bundle id :%s and add it into the received_bundle_id_queue\n",event->name);

					//old code

					/*// remove dtn log file
					char removefilename[128];
					memset(removefilename,'\0',sizeof(removefilename));
					sprintf(removefilename,"%s/%s",file,event->name);

					if(remove(removefilename)!=0)
					{
						fprintf(stdout,"remove log file %s failed\n",removefilename);
					}
					else
					{
						fprintf(stdout,"remove log file %s successfully\n",removefilename);
					}

					// parse payload
					char payload_filename[256];
					memset(payload_filename,'\0',sizeof(payload_filename));
					if(temp_s->dtn_type==BYTEWALLA)
					{
						sprintf(payload_filename,"%s/payload_%s",temp_s->payload_dir,event->name);
					}
					else if(temp_s->dtn_type==DTN2)
					{
						sprintf(payload_filename,"%s/bundle_%s.dat",temp_s->payload_dir,event->name);
					}
					else
					{
						fprintf(stdout,"wrong dtn_type:%d",temp_s->dtn_type);
					}

					FILE *payload_file=fopen(payload_filename,"r");

					if(payload_file==NULL)
					{
						fprintf(stdout,"payload %s do not exsists\n",payload_filename);
						break;
					}

					char buff[256];
					memset(buff,'\0',sizeof(buff));
					if(fgets(buff,256,payload_file)==NULL)
					{
						// fprintf(stdout,"payload %s do not exsists\n",payload_filename);
						fclose(payload_file);
						break;
					}
					int num=(int)buff[0]+1;//the first char is the num of filename
					buff[0]='/';

					//get the received file's name ,and put into the received queue
					char *add_queue_file_name=(char *)malloc(sizeof(char)*num);
					strncpy(add_queue_file_name,buff+1,num-1);
					queue_node *node=(queue_node*)malloc(sizeof(queue_node));
					node->content=add_queue_file_name;
					if(queue_add(received_file_queue,node)==0)
					{
						fprintf(stdout,"add file %s to received_file_queue failed \n",add_queue_file_name);
					}
					else
					{
						fprintf(stdout,"add file %s to received_file_queue \n",add_queue_file_name);
					}

					char target_filename[256];
					memset(target_filename,'\0',sizeof(target_filename));
					strcpy(target_filename,temp_s->dtn_shared_dir);
					int start=strlen(temp_s->dtn_shared_dir);
					strncpy(target_filename+start,buff,num);

					FILE *target_file=fopen(target_filename,"w");

					if(target_file==NULL)
						break;

					fputs(buff+num,target_file);
					while(fgets(buff,256,payload_file)!=NULL)
					{
						fputs(buff,target_file);
					}

					fclose(payload_file);
					fclose(target_file);*/
				}

			
				/*else
					fprintf(stdout,"file:%s last_file:%s --- already warning\n",event->name,last_file_name);*/

				strcpy(last_file_name,event->name);
				break;

				/* File open read-only was closed */
				case IN_CLOSE_NOWRITE:
				if(temp_s->type==PAYLOADDIR)
				{
					if ( event->mask & IN_ISDIR )
					{
						// fprintf(stdout,"file_or_dir :Dir --- file:%s --- event:%s\n",event->name,"IN_CLOSE_NOWRITE");
					}
					else
					{
						fprintf(stdout,"file_or_dir :File --- file:%s --- event:%s\n",event->name,"IN_CLOSE_NOWRITE");
						queue_node *node=(queue_node*)malloc(sizeof(queue_node));
						payload_dir_event *content=(payload_dir_event*)malloc(sizeof(payload_dir_event));
						content->payload_filename=event->name;
						content->event_type=2;//close_nowrite event
						node->content=content;
						queue_add(payload_dir_event_queue,node);
						fprintf(stdout,"add file:%s --- event:%s into the payload_dir_event_queue\n",event->name,"IN_CLOSE_NOWRITE");
					}
				}
				break;

				/* File was opened */
				case IN_OPEN:
				// fprintf(stdout,"file:%s --- event:%s\n",event->name,"IN_OPEN");
				break;

				/* File was moved from X */
				case IN_MOVED_FROM:
				// fprintf(stdout,"file:%s --- event:%s\n",event->name,"IN_MOVED_FROM");
				break;

				/* File was moved to X */
				case IN_MOVED_TO:
				// fprintf(stdout,"file:%s --- event:%s\n",event->name,"IN_MOVED_TO");
				break;

				/* Subdir or file was deleted */
				case IN_DELETE:
				// fprintf(stdout,"file:%s --- event:%s\n",event->name,"IN_DELETE");
				break;

				/* Subdir or file was created */
				case IN_CREATE:
				// fprintf(stdout,"file:%s --- event:%s\n",event->name,"IN_CREATE");
				if(temp_s->type==PAYLOADDIR)
				{
					if ( event->mask & IN_ISDIR )
					{
						// fprintf(stdout,"file_or_dir :Dir --- file:%s --- event:%s\n",event->name,"IN_CREATE");
					}
					else
					{
						fprintf(stdout,"file_or_dir :File --- file:%s --- event:%s\n",event->name,"IN_CREATE");
						queue_node *node=(queue_node*)malloc(sizeof(queue_node));
						payload_dir_event *content=(payload_dir_event*)malloc(sizeof(payload_dir_event));
						content->payload_filename=event->name;
						content->event_type=1;//in_create event
						node->content=content;
						queue_add(payload_dir_event_queue,node);

						fprintf(stdout,"add file:%s --- event:%s into the payload_dir_event_queue\n",content->payload_filename,"IN_CREATE");
					}
				}
				break;

				/* Watched entry was deleted */
				case IN_DELETE_SELF:
				// fprintf(stdout,"file:%s --- event:%s\n",event->name,"IN_DELETE_SELF");
				break;

				/* Watched entry was moved */
				case IN_MOVE_SELF:
				// fprintf(stdout,"file:%s --- event:%s\n",event->name,"IN_MOVE_SELF");
				break;

				/* Backing FS was unmounted */
				case IN_UNMOUNT:
				// fprintf(stdout,"file:%s --- event:%s\n",event->name,"IN_UNMOUNT");
				break;

				/* Too many FS events were received without reading them
				some event notifications were potentially lost.  */
				case IN_Q_OVERFLOW:
				// fprintf(stdout,"file:%s --- event:%s\n",event->name,"IN_Q_OVERFLOW");
				break;

				/* Watch was removed explicitly by inotify_rm_watch or automatically
				because file was deleted, or file system was unmounted.  */
				case IN_IGNORED:
				// fprintf(stdout,"file:%s --- event:%s\n",event->name,"IN_IGNORED");
				break;

				/* Some unknown message received */
				default:
				fprintf(stdout,"file:%s --- event:%s\n","null","null");
				break;
			}
			/*for(i=0; i<EVENT_NUM; i++)
			{

				if((event->mask >> i) & 1)
				{
					if(event->len > 0)
						fprintf(stdout, "%s --- %s\n", event->name, event_str[i]);
					else
						fprintf(stdout, "%s --- %s\n", " ", event_str[i]);
				}
			}*/
			nread = nread + sizeof(struct inotify_event) + event->len;
			len = len - sizeof(struct inotify_event) - event->len;
		}
	}
	return NULL;
}

//help info 
void print_usage()
{
  printf("Option:\n");
  printf("-t\t:<dtn2=0|bytewalla=1(type of DTN,default is dtn2)> \n");
  printf("-p\t:<(payload dir)> \n");
  printf("-d\t:<destination eid> \n");
  printf("-m\t:<source eid> \n");
  printf("-s\t:<shared dir> \n");
  printf("-l\t:<dtn2|bytewalla received bundle log dir> \n");
  printf("-r\t:<this program's running log file path> \n");
  printf("-f\t:<program log write inot file(0) or display in screen(1)0|1\n");
}

int exec_shell(char *shell_chars)
{
	int i=system(shell_chars);
	switch(i)
	{
		case -1:
		
		break;

		case 127:

		break;

		case 0:

		default :
		break;
	}
	fprintf(stdout,"exec shell %s ,return %d \n",shell_chars,i);
	return i;
}


//exec dtn2 send cmd
void  dtn2_send(char *destination_eid,char *local_eid,char *payload_path)
{
	//exec send cmd
	char *cmd[512];
	memset(cmd,'\0',sizeof(cmd));
	sprintf(cmd,"dtnsend -s %s -d %s -t f -p %s",local_eid,destination_eid,payload_path);
	if(exec_shell(cmd)==0)
	{
		fprintf(stdout,"exec dtn2 send cmd successfully\n");
	}
	return ;
}

//exec bytewalla send cmd
void bytewalla_send(char *destination_eid,char *local_eid,char *payload_path)
{	
	//exec send cmd
	char *cmd[512];
	memset(cmd,'\0',sizeof(cmd));
	sprintf(cmd,"bytewallasend -d %s -t f -p %s",destination_eid,payload_path);
	if(exec_shell(cmd)==0)
	{
		fprintf(stdout,"exec bytewalla send cmd successfully\n");
	}
	return ;
}

