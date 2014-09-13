#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <fcntl.h>
#include <unistd.h>
#include <getopt.h>
#include <sched.h>
#include <time.h>
#include <sys/signal.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <errno.h>
#include "ipc_net.h"
#include "ipc_proc.h"


//net socket related
int start_listen(int port)
{
	int flag = 1;
	int socketfd_listen = -1;
	struct sockaddr_in server_addr;

	if ((socketfd_listen = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		printf("socket failed %d !", socketfd_listen);
		return -1;
	}
	if (setsockopt(socketfd_listen, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag)) < 0) {
		printf("setsockopt failed %d !", socketfd_listen);
		return -1;
	}

	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(port);
	server_addr.sin_addr.s_addr = htons(INADDR_ANY);

	if (bind(socketfd_listen, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
		printf("bind failed %d !", socketfd_listen);
		return -1;
	}
	
	if (listen(socketfd_listen, 10) < 0) {
		printf("listen failed %d !", socketfd_listen);
		return -1;
	}

	printf("start listen ok ,Listen to port[%d]\n", port);
	return socketfd_listen;
}


int handle_connection(int socketfd_listen)
{
	int sockfd = -1;
	struct sockaddr_in client_addr;
	socklen_t length = sizeof(client_addr);

	if ((sockfd = accept(socketfd_listen, (struct sockaddr *)&client_addr, &length)) < 0) {
		printf("accept failed %d\n", sockfd);
		return -1;
	}
#if 0
	struct timeval timeout;
	timeout.tv_sec=1;    
	timeout.tv_usec=500;
	int result = 0;
	result = setsockopt(sockfd,SOL_SOCKET,SO_RCVTIMEO,(char *)&timeout.tv_sec,sizeof(struct timeval));
	if (result < 0)
	{
		printf("setsockopt SO_RCVTIMEO failed %d\n", sockfd);
		return -1;
	}
	result = setsockopt(sockfd,SOL_SOCKET,SO_SNDTIMEO,(char *)&timeout.tv_sec,sizeof(struct timeval));
	if (result < 0)
	{
		printf("setsockopt SO_SNDTIMEO failed %d\n", sockfd);
		return -1;
	}
#endif
	//printf("One connection[client ip=%s] is accept\n", inet_ntoa(client_addr.sin_addr));
	return sockfd;
}

int handle_disconnection(int sockfd)
{
	if(sockfd < 0)
		return -1;
	
	close(sockfd);
	sockfd = -1;
	return 0;
}
#if 0 
int send_packet(int sockfd, u8 *pBuffer, u32 size)
{
	int retv = send(sockfd, pBuffer, size, MSG_NOSIGNAL);
	if ((u32)retv != size) {
		//printf("send_packet returns %d.", retv);
		return -1;
	}
	return 0;
}

int receive_packet(int sockfd,u8 *pBuffer, u32 size)
{
	int retv = recv(sockfd, pBuffer, size, MSG_WAITALL);
	if (retv <= 0) {
		if (retv == 0) {
			//printf("receive_packet failed\n");
			return -2;
		}
		//printf("receive_packet returns %d.", retv);
		return -1;
	}
	return retv;
}
#endif

int setnonblock(int sockfd)
{
	return fcntl(sockfd, F_SETFL, fcntl(sockfd, F_GETFL) | O_NONBLOCK);
}

int setblock(int sockfd)
{
	return fcntl(sockfd, F_SETFL, fcntl(sockfd, F_GETFL) & (~O_NONBLOCK));
}

int setrecvtime(int sockfd, int secs)
{
	struct timeval timeout = {0,0};
	timeout.tv_sec = secs;

	return setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout,sizeof(struct timeval));
}

int setsendtime(int sockfd, int secs)
{
	struct timeval timeout = {0,0};
	timeout.tv_sec = secs;

	return setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, (char*)&timeout,sizeof(struct timeval));

}

int setreuseaddr(int sockfd)
{
	int flag = 1;
	return setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag));
}


static int __send(int sockfd,u8 *pBuffer, u32 size)
{
	int retv = 0;
_send_loop:
	retv  = send(sockfd, pBuffer, size, MSG_NOSIGNAL);
	if (retv <= 0) {
		if (retv == 0) {
			//printf("__send remote socket closed!\n");
			return -2;
		}

		if(errno == EAGAIN || errno == EWOULDBLOCK){
			goto _send_loop;
		}
		//printf("__send returns %d.", retv);
		return -1;
	}
	return retv;	
}
static int __recv(int sockfd,u8 *pBuffer, u32 size)
{
	int retv = 0;
_recv_loop:
	retv = recv(sockfd, pBuffer, size, 0);
	if (retv <= 0) {
		if (retv == 0) {
			//printf("__recv remote socket closed!\n");
			return -2;
		}

		if(errno == EAGAIN || errno == EWOULDBLOCK){
			goto _recv_loop;
		}
		//printf("__recv returns %d.", retv);
		return -1;
	}
	return retv;	
}



int send_packet(int sockfd,u8 *pBuffer, u32 size)
{
	int nSend = 0;
	int nTmpSend = 0;

	nTmpSend = __send(sockfd, pBuffer, size);
	if(nTmpSend < 0){
		return nTmpSend;
	}

	nSend += nTmpSend;
	while(nSend < size){
		nTmpSend = __send(sockfd, pBuffer + nSend, size - nSend);
		if(nTmpSend < 0){
			break;
		}else{
			nSend += nTmpSend;
		}
	}
	//printf("send_packet %d\n", nSend);
	return nSend;
}

int receive_packet(int sockfd,u8 *pBuffer, u32 size)
{
	int nRecv = 0;
	int nTmpRecv = 0;
    nTmpRecv = __recv( sockfd,pBuffer, size);
	if(nTmpRecv < 0){
		return nTmpRecv;
	}

	nRecv += nTmpRecv;
	while(nRecv < size){
	        nTmpRecv = __recv( sockfd,pBuffer + nRecv, size - nRecv);
		if(nTmpRecv < 0){
			break;
		}else{
			nRecv += nTmpRecv;
		}
	}
	return nRecv;
}

