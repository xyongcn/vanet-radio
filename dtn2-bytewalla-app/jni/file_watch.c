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

void * watchFile(void *s);
void print_usage();
// void send_sharedfile_into_bundle(char* filepath,char *filename,char *temp,int dtn_type);
void  dtn2_send(char *destination_eid,char *local_eid,char *payload_path);
void bytewalla_send(char *destination_eid,char *local_eid,char *payload_path);

#define LOGDIR 1
#define SHAREDDIR 2
#define DTN2 0
#define BYTEWALLA 1

#define LOGOUT(...) \
fprintf(stdout,__VA_ARGS__); \
if(program_log_file!=NULL) \
{ \
	fprintf(program_log_file,__VA_ARGS__); \
	fflush(program_log_file); \
} 

//program log file's FILE
FILE *program_log_file=NULL;

//queue's head
queue_head *received_file_queue;

int main(int argc, char *argv[])
{
	//init the received files's queue
	received_file_queue=queue_init();

	//init the argument read
	int dtn_type=DTN2;
	char *payload_dir=NULL;
	char *destination_eid=NULL;
	char *local_eid=NULL;
	char *dtn_shared_dir=NULL;
	char *dtn_log_dir=NULL;
	char *program_log=NULL;

	//read the arguments
	int c, done = 0;
	while (!done)
	{
	  c = getopt(argc, argv, "m:M:t:T:p:P:d:D:s:S:h:H:l:L:r:R");
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
  
  	//stdout redirect to File
  	char program_log_filename[128];
  	memset(program_log_filename,'\0',sizeof(program_log_filename));
  	sprintf(program_log_filename,"%s/dtn2-bytewalla-app.log",program_log);
  	program_log_file=fopen(program_log_filename,"w");
  	if(program_log_file!=NULL)
  		LOGOUT("create program log file %s successfully",program_log_filename);

  	/*printf("logfile:%s\n",program_log_filename);
  	remove(program_log_filename);
  	int out=open(program_log_filename,O_CREAT|O_RDWR,S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
  	if(out>0)
  	{
  		dup2(out,fileno(stdout));
  	}
  	else
  	{
  		printf("redirect stdout to file %s failed \n");
  	}*/

//watched log dir
	file_type *f_logdir_type=(file_type *)malloc(sizeof(file_type));
	f_logdir_type->type=LOGDIR;

	f_logdir_type->dtn_type=dtn_type;
	f_logdir_type->dtn_log_dir=dtn_log_dir;
	f_logdir_type->payload_dir=payload_dir;
	f_logdir_type->destination_eid=destination_eid;
	f_logdir_type->dtn_shared_dir=dtn_shared_dir;
	f_logdir_type->program_log=program_log;
	f_logdir_type->local_eid=local_eid;
	/*int dtn_type;
	char *payload_dir=NULL;
	char *destination_eid=NULL;
	char *dtn_shared_dir=NULL;
	char *dtn_log_dir=NULL;
	char *program_log=NULL;*/
	pthread_t watchLogDir;
	int t=pthread_create(&watchLogDir,NULL,watchFile,f_logdir_type);
	if(t!=0)
	{
		LOGOUT("create thread for watching dtn log dir failed\n");
	}
	else
	{
		pthread_detach(watchLogDir);
	}

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
		LOGOUT("create thread for watching shared dir failed\n");
	}
	else
	{
		// LOGOUT("watching shared dir %s\n",argv[1]);
		pthread_detach(watchSharedDir);
	}



	while(1)
	{

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
		LOGOUT("watching dtn log dir %s\n",file);
	}
	else if(temp_s->type==SHAREDDIR)
	{
		file=temp_s->dtn_shared_dir;
		LOGOUT("watching shared dir %s\n",file);
	}
	else
	{
		LOGOUT("can't watching %s dir \n","null");
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
				// LOGOUT("file:%s --- event:%s\n",event->name,"IN_ACCESS");
				break;

				/* File was modified */
				case IN_MODIFY:
				// LOGOUT("file:%s --- event:%s\n",event->name,"IN_MODIFY");

				break;

				/* File changed attributes */
				case IN_ATTRIB:
				// LOGOUT("file:%s --- event:%s\n",event->name,"IN_ATTRIB");
				break;

				/* File open for writing was closed */
				//this is the file changed event
				case IN_CLOSE_WRITE:
				if(strcmp(last_file_name,event->name)==0)
				{
					memset(last_file_name,'\0',sizeof(last_file_name));
					break;
				}

			
				char *filename=event->name;
				LOGOUT("file:%s --- event:%s\n",event->name,"IN_CLOSE_WRITE");
				//related option
				//remove log file

				if(temp_s->type==SHAREDDIR)
				{
					//check that if it's received file
					if(file_queue_find_and_remove(received_file_queue,event->name)==1)
					{
						LOGOUT("file %s is the received file\n",event->name);
						break;
					}
					else
					{
						LOGOUT("this is not the duplicated file\n");
					}

					//process the file to send
					char send_filename[256];
					memset(send_filename,'\0',sizeof(send_filename));
					char temp_filename[256];
					memset(temp_filename,'\0',sizeof(temp_filename));
					sprintf(send_filename,"%s/%s",file,filename);
					sprintf(temp_filename,"%s/%s.payload",temp_s->program_log,filename);
					// FILE *send_file=fopen(send_filename,"r");
					// FILE *temp_file=fopen(temp_filename,"w");
					int send_file_fd=open(send_filename,O_RDONLY);
					int temp_file_fd=open(temp_filename,O_WRONLY|O_CREAT|O_TRUNC,0766);

					// if(send_file==NULL || temp_file==NULL)
					// 	break;
					if(send_file_fd==-1 || temp_file_fd==-1)
					{
						LOGOUT("open shared file %s failed or create temp payload %s failed\n",send_filename,temp_filename);
						break;
					}

					char buff[256];
					memset(buff,'\0',sizeof(buff));
					//write the filename length and filename
					/*buff[0]=strlen(filename);
					strcpy(buff+1,filename);
					fputs(buff,temp_file);*/
					strcpy(buff,filename);
					write(temp_file_fd,buff,sizeof(buff));
					memset(buff,'\0',sizeof(buff));

					//read payload file and write to the temp payload
					/*while(!feof(send_file))
					{
						fgets(buff,sizeof(buff),send_file);
						fputs(buff,temp_file);
					}

					fclose(send_file);
					fclose(temp_file);*/
					int readnum;
					while((readnum=read(send_file_fd,buff,sizeof(buff)))>0)
					{
						if(write(temp_file_fd,buff,readnum)<readnum)
						{
							// close(temp_file_fd);
							// close(send_file_fd);
							// LOGOUT("write into temp payload %s failed\n",temp_filename);
							readnum=-1;
							break;
						}
					}

					close(temp_file_fd);
					close(send_file_fd);

					if(readnum==-1)
					{
						LOGOUT("write into temp payload %s failed\n",temp_filename);
						break;
					}
					else
					{
						LOGOUT("create temp payload file %s \n",temp_filename);
					}


					if(temp_s->dtn_type==DTN2)
					{
						// LOGOUT("dtn send bundle\n");
						dtn2_send(temp_s->destination_eid,temp_s->local_eid,temp_filename);
					}
					else if(temp_s->dtn_type==BYTEWALLA)
					{
						// LOGOUT("bytewalla send bundle\n");
						bytewalla_send(temp_s->destination_eid,temp_s->local_eid,temp_filename);
					}
					else
					{
						LOGOUT("wrong argumen dtn_type:%d\n",temp_s->dtn_type);
					}

					//remove temp file
				}
				else if(temp_s->type==LOGDIR)
				{
					// remove dtn log file
					char removefilename[128];
					memset(removefilename,'\0',sizeof(removefilename));
					sprintf(removefilename,"%s/%s",file,event->name);

					if(remove(removefilename)!=0)
					{
						LOGOUT("remove log file %s failed\n",removefilename);
					}
					else
					{
						LOGOUT("remove log file %s successfully\n",removefilename);
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
						LOGOUT("wrong dtn_type:%d",temp_s->dtn_type);
						break;
					}

					/*FILE *payload_file=fopen(payload_filename,"r");

					if(payload_file==NULL)
					{
						LOGOUT("payload %s do not exsists\n",payload_filename);
						break;
					}*/
					int payload_file_fd=open(payload_filename,O_RDONLY);
					if(payload_file_fd==-1)
					{
						LOGOUT("payload %s do not exsists\n",payload_filename);
						break;
					}
					else
					{
						LOGOUT("open the payload file %s\n",payload_filename);
					}

					char buff[256];
					memset(buff,'\0',sizeof(buff));
					/*if(fgets(buff,256,payload_file)==NULL)
					{
						fclose(payload_file);
						break;
					}
					int num=(int)buff[0]+1;//the first char is the num of filename
					buff[0]='/';*/
					read(payload_file_fd,buff,sizeof(buff));
					LOGOUT("the shared file name:%s\n",buff);

					//get the received file's name ,and put into the received queue
					/*char *add_queue_file_name=(char *)malloc(sizeof(char)*num);
					strncpy(add_queue_file_name,buff+1,num-1);*/
					char *add_queue_file_name=(char *)malloc(sizeof(buff));
					strcpy(add_queue_file_name,buff);
					queue_node *node=(queue_node*)malloc(sizeof(queue_node));
					node->content=add_queue_file_name;
					if(queue_add(received_file_queue,node)==0)
					{
						LOGOUT("add file %s to received_file_queue failed \n",add_queue_file_name);
					}
					else
					{
						LOGOUT("add file %s to received_file_queue \n",add_queue_file_name);
					}

					char target_filename[256];
					memset(target_filename,'\0',sizeof(target_filename));
					/*strcpy(target_filename,temp_s->dtn_shared_dir);
					int start=strlen(temp_s->dtn_shared_dir);
					strncpy(target_filename+start,buff,num);

					FILE *target_file=fopen(target_filename,"w");

					if(target_file==NULL)
						break;*/

					sprintf(target_filename,"%s/%s",temp_s->dtn_shared_dir,buff);
					int target_file_fd=open(target_filename,O_WRONLY|O_CREAT|O_TRUNC,0766);
					if(target_file_fd==-1)
					{
						LOGOUT("ceated received shared file %s failed\n",target_filename);
						break;
					}

					/*fputs(buff+num,target_file);
					while(fgets(buff,256,payload_file)!=NULL)
					{
						fputs(buff,target_file);
					}

					fclose(payload_file);
					fclose(target_file);*/

					int read_num;
					while((read_num=read(payload_file_fd,buff,sizeof(buff)))>0)
					{
						if((write(target_file_fd,buff,read_num))<read_num)
						{
							read_num=-1;
							break;
						}
					}

					close(payload_file_fd);
					close(target_file_fd);

					if(read_num==-1)
					{
						LOGOUT("generate shared file %s form payload %s failed\n",target_filename,payload_filename);
						break;
					}
					else
					{
						LOGOUT("generate shared file %s successfully\n",target_filename);
					}
				}

			
				/*else
					LOGOUT("file:%s last_file:%s --- already warning\n",event->name,last_file_name);*/

				strcpy(last_file_name,event->name);
				break;

				/* File open read-only was closed */
				case IN_CLOSE_NOWRITE:
				// LOGOUT("file:%s --- event:%s\n",event->name,"IN_CLOSE_NOWRITE");
				break;

				/* File was opened */
				case IN_OPEN:
				// LOGOUT("file:%s --- event:%s\n",event->name,"IN_OPEN");
				break;

				/* File was moved from X */
				case IN_MOVED_FROM:
				// LOGOUT("file:%s --- event:%s\n",event->name,"IN_MOVED_FROM");
				break;

				/* File was moved to X */
				case IN_MOVED_TO:
				// LOGOUT("file:%s --- event:%s\n",event->name,"IN_MOVED_TO");
				break;

				/* Subdir or file was deleted */
				case IN_DELETE:
				// LOGOUT("file:%s --- event:%s\n",event->name,"IN_DELETE");
				break;

				/* Subdir or file was created */
				case IN_CREATE:
				// LOGOUT("file:%s --- event:%s\n",event->name,"IN_CREATE");
				break;

				/* Watched entry was deleted */
				case IN_DELETE_SELF:
				// LOGOUT("file:%s --- event:%s\n",event->name,"IN_DELETE_SELF");
				break;

				/* Watched entry was moved */
				case IN_MOVE_SELF:
				// LOGOUT("file:%s --- event:%s\n",event->name,"IN_MOVE_SELF");
				break;

				/* Backing FS was unmounted */
				case IN_UNMOUNT:
				// LOGOUT("file:%s --- event:%s\n",event->name,"IN_UNMOUNT");
				break;

				/* Too many FS events were received without reading them
				some event notifications were potentially lost.  */
				case IN_Q_OVERFLOW:
				// LOGOUT("file:%s --- event:%s\n",event->name,"IN_Q_OVERFLOW");
				break;

				/* Watch was removed explicitly by inotify_rm_watch or automatically
				because file was deleted, or file system was unmounted.  */
				case IN_IGNORED:
				// LOGOUT("file:%s --- event:%s\n",event->name,"IN_IGNORED");
				break;

				/* Some unknown message received */
				default:
				LOGOUT("file:%s --- event:%s\n","null","null");
				break;
			}
			/*for(i=0; i<EVENT_NUM; i++)
			{

				if((event->mask >> i) & 1)
				{
					if(event->len > 0)
						LOGOUT( "%s --- %s\n", event->name, event_str[i]);
					else
						LOGOUT( "%s --- %s\n", " ", event_str[i]);
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
  printf("-r\t:[this program's running log file path] \n");
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
	LOGOUT("exec shell %s ,return %d \n",shell_chars,i);
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
		LOGOUT("exec dtn2 send cmd successfully\n");
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
		LOGOUT("exec bytewalla send cmd successfully\n");
	}
	return ;
}

