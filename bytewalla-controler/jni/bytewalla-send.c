#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

// char INVALID=0;

/*char STARTSERVER=1;
char STOPSERVER=2;
char DTNSEND=3;
char RCPTDTNSEND=4;*/
    
/*char SUCCESS=101;
char FAIL=102;*/

// char buff[130];

int main(int argc, char **argv)
{
    //prepare the buff to send
    char *buff;
    int buff_size=0;
    int *size=(int *)malloc(sizeof(int)*argc);
    int i=0;
    for(i=0;i<argc;i++)
    {
        size[i]=strlen(*(argv+i));
        buff_size+=size[i]+1;
    }
    // printf("argv's strlen:%d\n",buff_size);
    buff=(char *)malloc(sizeof(char)*buff_size);
    char *buff_temp=buff;
    for(i=0;i<argc;i++)
    {
        strcpy(buff_temp,*(argv+i));
        buff_temp+=size[i];
        *buff_temp=' ';
        ++buff_temp;
    }
    buff[buff_size-1]='\0';

    printf("%s\n",buff);
    // return 0;


    /*if ((b2send==DTNSEND && b2send==RCPTDTNSEND) || argc < 2)
    {
        printf("Usage: %s ip port", argv[0]);
        exit(1);
    }*/
    // printf("This is a UDP client\n");
    struct sockaddr_in addr;
    int sock;

    if ( (sock=socket(AF_INET, SOCK_DGRAM, 0)) <0)
    {
        perror("socket");
        exit(1);
    }

    const char address[]="127.0.0.1";
    int port=61001;

    addr.sin_family = AF_INET;
    // addr.sin_port = htons(atoi(argv[2]));
    // addr.sin_addr.s_addr = inet_addr(argv[1]);
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = inet_addr(address);
    if (addr.sin_addr.s_addr == INADDR_NONE)
    {
        printf("Incorrect ip address!");
        close(sock);
        exit(1);
    }

    
    int len = sizeof(addr);

    /*buff[0]=b2send;
    buff[1]=' ';
    if(eid!=NULL)
        strcpy(buff+2,eid);
    else
        buff[2]='\0';*/

    
    printf("send %s to daemon\n",buff+2);
    // return 0;

    int n = sendto(sock, buff, strlen(buff), 0, (struct sockaddr *)&addr, sizeof(addr));
    if (n < 0)
    {
        perror("sendto");
        close(sock);
    }
    // printf("waiting for dtnDaemon excuting\n");
    printf("send cmd successfully\n");

    /*n = recvfrom(sock, buff, strlen(buff), 0, (struct sockaddr *)&addr, &len);
    if (n>0)
    {
        // printf("received cmd %d\n",(int)buff[0]);
        if(buff[0]==SUCCESS)
            printf("cmd excute successfully\n");
        else if(buff[0]==FAIL)
            printf("cmd excute fail\n");
        // puts(buff);
    }
    else if (n==0)
    {
        printf("server closed\n");
        close(sock);
    }
    else if (n == -1)
    {
        perror("recvfrom");
        close(sock);
    }*/

    return 0;
}

