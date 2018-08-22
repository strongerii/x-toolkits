#ifndef _UTILS_NET_H_
#define _UTILS_NET_H_

#ifdef __cplusplus
extern "C"
{
#endif//__cplusplus

int start_listen(int port);
int start_connection(const char *pServerIP, unsigned short port);
int handle_connection(int socketfd_listen);
int handle_disconnection(int *sockfd);

int send_packet(int sockfd, unsigned char *pBuffer, unsigned int size);
int receive_packet(int sockfd, unsigned char *pBuffer, unsigned int size);

int setnonblock(int sockfd);
int setblock(int sockfd);
int setreuseaddr(int sockfd);
int setrecvtime(int sockfd, int secs);
int setsendtime(int sockfd, int secs);

#ifdef __cplusplus
}
#endif//__cplusplus


#endif //_UTILS_NET_H_