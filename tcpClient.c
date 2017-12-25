#include <sys/socket.h>
#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <errno.h>

#include <unistd.h>
#include <pthread.h>

#define WORKER_NUM 2
#define BUFFZERO(buffer,size)  do{memset(buffer,0,size);}while(0)

typedef struct Worker
{
   pthread_t thread;
   int sockfd;
   void*(*run)(void* arg);
}Worker_t;


void * reader_rountine(void* arg)
{
    Worker_t *reader_ctx = (Worker_t*)arg;
    int  sockfd = reader_ctx->sockfd;
    char buffer[1024]={0};

    while(1)
    {
        printf("reader tick ...\n");
        BUFFZERO(buffer,sizeof(buffer));
        int ret =0;
        if ((ret=read(sockfd,buffer,sizeof(buffer)))<0)
        {
            continue;
        }
        if (ret==0)
        {
          printf ("server is disconnnect...\n");
          
         // break ;
        }
        printf("respones:%s\n",buffer);

        sleep(1);
    }

   return (void*)0;
}

const char* pkgs[]={"login","rount","rount"};

void* write_rountine(void *arg)
{
     Worker_t *writer_ctx = (Worker_t*)arg;
    int  sockfd = writer_ctx->sockfd;
    char buffer[1024]={0};

    unsigned int count =0;
    const char* msg_t = 0;
    while(1)
    {
        sleep(3);
        msg_t = pkgs[count%3];
        write(sockfd,msg_t,strlen(msg_t)+1);
        count++;
    }
    return (void*)0;
}

Worker_t *workrs[WORKER_NUM]={0};

int main(int argc,const char * argvs[])
{
   //buy telphone 
   int tel = socket(AF_INET,SOCK_STREAM,0);
   const char *ip = argvs[1];
   int port =atoi(argvs[2]);
   //buy wire container
   struct sockaddr_in addrc;
   BUFFZERO(&addrc,sizeof(addrc));
   addrc.sin_family = AF_INET;
   addrc.sin_port = htons(port);
   addrc.sin_addr.s_addr = inet_addr(ip);


   printf ("ip :%s port=%d ...\n",ip,port);
   int rec = connect(tel,(struct sockaddr*)&addrc,sizeof(struct sockaddr));
   if (rec<0)
   {
       perror("connnect failed...\n");
       return -1;
   }

    Worker_t reader_worker={0};
    reader_worker.run = reader_rountine;
    reader_worker.sockfd = tel;
   
    Worker_t writer_worker = {0};
    writer_worker.run =write_rountine;

    workrs[0]= & reader_worker;
    workrs[1]= & writer_worker;
    writer_worker.sockfd = tel;
    int i;
    Worker_t *tempworker;
    for(i=0;i<WORKER_NUM;++i)
    {
        tempworker = workrs[i];
        if (pthread_create(&tempworker->thread,0,tempworker->run,tempworker)<0)
        {
            printf("create failed for thread indx=%d...\n",i);
            return -1;
        }
    }
    
    for(i=0;i<WORKER_NUM;++i)
    {
        tempworker = workrs[i];
        pthread_join(tempworker->thread,0);
    }
            
  return 0;
}
