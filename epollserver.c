
#include <stdio.h>
#include <string.h>

#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include <fcntl.h>

#include <arpa/inet.h>
#include <netinet/tcp.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/epoll.h>

#define EPOLL_MAX_SIZE 1024
#define CPU_CORE 3
#define BUFFER_LENTH 1024
#define CLIENT_MAX 1024

#define PKG_LOGIN "login"
#define PKG_LOGOUT "logout"
#define PKG_ROUNTINE "rount"
#define ZERO_BUF(buffer, size)   \
	do                           \
	{                            \
		memset(buffer, 0, size); \
	} while (0)

typedef void *(*RountineMain)(void *arg);

static int setFdNoblock(int fd)
{
	int flags = 0;
	flags = fcntl(fd, F_GETFL, 0);
	flags |= O_NONBLOCK;
	int res = fcntl(fd, F_SETFL, flags);
	if (res < 0)
	{
		return -1;
	}
	return 0;
}





struct client_talbes
{
	int socketfds[CLIENT_MAX];
	int size ;
}clientTables;

static int client_sock_push(int clientconn)
{
	if (clientTables.size<CLIENT_MAX) 
	{
		clientTables.socketfds[++clientTables.size]=clientconn;
		return 0;
	}

    return -1;

}


static int findClientSockfdId(int clientfd)
{
	int i = 0;
	for (; i < clientTables.size; ++i)
	{
		if (clientTables.socketfds[i] == clientfd)
		{
			return i;
		}
	}
	return -1;
}

typedef struct
{
	pthread_t t;
	RountineMain rountine;
	int serverfd;
} ThreadMain;

void parse_buffer(int clientfd,char *buffer, int ret)
{
	//switch
	if (strncmp(buffer,PKG_LOGIN,strlen(PKG_LOGIN)==0))
	{
		//解析后面协议
		printf("login in \n");

	}else if (strncmp(buffer,PKG_LOGOUT,strlen(PKG_LOGOUT)==0))
	{
		//解析后面协议
	    printf("log out  \n");

	}
	else if (strncmp(buffer,PKG_ROUNTINE,strlen(PKG_ROUNTINE)==0))
   {
		//解析后面协议
          printf("msg rountine  \n");
   }
}

void *serverRountine(void *arg)
{
    int serverfd = *(int *)arg;

	int epoll_fd_header = epoll_create(EPOLL_MAX_SIZE);
	struct epoll_event ev_setting, events[EPOLL_MAX_SIZE] = {0};
	
	ev_setting.events = EPOLLIN;
	ev_setting.data.fd = serverfd;
	epoll_ctl(epoll_fd_header, EPOLL_CTL_ADD, serverfd, &ev_setting);
	while (1)
	{
		printf("tick ...\n");
		//扫描
		int out_fds_count = epoll_wait(epoll_fd_header, events, EPOLL_MAX_SIZE, -1);
		if (out_fds_count == -1) {
			printf("gg\n");
			break;
		}
		
	  
		int i;
		for (i = 0; i < out_fds_count; ++i)
		{
			int connfd = events[i].data.fd;
			//主机sock产生的事件只有 accept
			if (connfd == serverfd)
			{
				struct sockaddr_in cli_addr;
				ZERO_BUF(&cli_addr, sizeof(struct sockaddr_in));
				socklen_t len = sizeof(cli_addr);
				int clientfd = accept(serverfd, (struct sockaddr *)&cli_addr, &len);
				if (clientfd < 0)
				{
					if (errno == ECONNABORTED)
					{
						continue; //next round
					}
				}

				setFdNoblock(clientfd);
				//save to epoll mananger
				ev_setting.data.fd = clientfd;
				ev_setting.events = EPOLLIN | EPOLLET;
				epoll_ctl(epoll_fd_header, EPOLL_CTL_ADD, clientfd, &ev_setting);
                
				client_sock_push(clientfd);

			}
			else
			{
				//不是主机socket事件有连接过来，那就是客户端sockfd有文件可以用读写
				char buffer[BUFFER_LENTH] = {0};
				int ret = 0;
				ret = recv(connfd, buffer, BUFFER_LENTH, 0);
				if (ret < 0)
				{
					if (errno == EINTR)
					{
						printf("data readed all\n");
					}
					ev_setting.data.fd = connfd;
					ev_setting.events = EPOLLIN | EPOLLET;
					epoll_ctl(epoll_fd_header, EPOLL_CTL_DEL, connfd, &ev_setting);
				}
				else if (ret == 0)
				{
					printf("client disconnected...\n");
					ev_setting.data.fd = connfd;
					ev_setting.events = EPOLLIN | EPOLLET;
					epoll_ctl(epoll_fd_header, EPOLL_CTL_DEL, connfd, &ev_setting);
				}
				else
				{
					//解析客户端连接数据
					printf("recv: %s , %d Bytes \n", buffer, ret);
					parse_buffer(connfd, buffer, ret);
				}
			}
		}
	}
	return (void *)(0);
}


static int ntySetNonblock(int fd) {
	int flags;

	flags = fcntl(fd, F_GETFL, 0);
	if (flags < 0) return flags;
	flags |= O_NONBLOCK;
	if (fcntl(fd, F_SETFL, flags) < 0) return -1;
	return 0;
}
#define THREAD_MAX 3
int main(int argc, char *argv[]) {

	if (argc < 2) {
		printf("Paramter Error\n");
		return -1;
	}

	int port = atoi(argv[1]);

	int sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0) {
		perror("socket");
		return 1;
	}
	ntySetNonblock(sockfd);

	struct sockaddr_in addr;
	memset(&addr, 0, sizeof(struct sockaddr_in));

	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = INADDR_ANY;

	if(bind(sockfd, (struct sockaddr*)&addr, sizeof(struct sockaddr_in)) < 0) {
		perror("bind");
		return 2;
	}
	if (listen(sockfd, 5) < 0) {
		return 3;
	}

	int i = 0;
	pthread_t thread_id[THREAD_MAX] = {0};
	for (i = 0;i < THREAD_MAX;i ++) {
		pthread_create(&thread_id[i], NULL, serverRountine, &sockfd);
		usleep(1);
	}

	for (i = 0;i < THREAD_MAX;i ++) {

		pthread_join(thread_id[i], NULL);
	}

	
	return 0;
}
