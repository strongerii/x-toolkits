#ifndef _IPC_NET_HEADER_
#define _IPC_NET_HEADER_

#include "xlib_type.h"

#ifdef __cplusplus
extern "C"
{
#endif//__cplusplus

//net socket related
int start_listen(int port);
int handle_connection(int socketfd_listen);
int handle_disconnection(int sockfd);
int send_packet(int sockfd, u8 *pBuffer, u32 size);
int receive_packet(int sockfd,u8 *pBuffer, u32 size);

int setnonblock(int sockfd);
int setblock(int sockfd);
int setreuseaddr(int sockfd);

int setrecvtime(int sockfd, int secs);
int setsendtime(int sockfd, int secs);

#ifdef __cplusplus
}
#endif//__cplusplus


#endif //_IPC_NET_HEADER_