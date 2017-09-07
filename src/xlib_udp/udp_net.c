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


//net socket related
int start_listen(int port)
{
	int flag = 1;
	int socketfd_listen = -1;
	struct sockaddr_in server_addr;

	if ((socketfd_listen = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		printf("socket failed %d !", socketfd_listen);
		return -1;
	}

	if (setsockopt(socketfd_listen, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag)) < 0) {
		printf("setsockopt failed %d !", socketfd_listen);
		return -1;
	}
    if (setsockopt(socketfd_listen, SOL_SOCKET, SO_BROADCAST, &flag, sizeof(flag)) < 0) {
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

	printf("start bind ok ,bind to port[%d]\n", port);
	return socketfd_listen;
}


int send_buffer(int sock, char *buf, int len)
{
    int sendRet;
    int sndPktLen;
    int remotePort = DS_SOCK_UDP_USR_SIDE_PORT;

    struct sockaddr_in toAddr;

    memset(&toAddr, 0, sizeof(toAddr));
    toAddr.sin_family = AF_INET;
    toAddr.sin_addr.s_addr = inet_addr("255.255.255.255");
    toAddr.sin_port = htons(remotePort);
    
    sndPktLen = len > DS_SOCK_UDP_PKT_MIN_LEN ? len:DS_SOCK_UDP_PKT_MIN_LEN;

    if( (sendRet = sendto(sock, buf, sndPktLen, 0, (struct sockaddr*)&toAddr, sizeof(toAddr))) != sndPktLen)
    {
        printf("sendto failed, return value: %d\t error no.: %d\r\n", sendRet, errno);
    }
    return sendRet;
}

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
