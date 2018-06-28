#ifndef _IPC_NET_HEADER_
#define _IPC_NET_HEADER_

#include "xlib_type.h"

#define MAX_SD_LEN (50)

typedef struct {
    int sd[MAX_SD_LEN];
    int sd_len;
} context;


#ifdef __cplusplus
extern "C"
{
#endif//__cplusplus

//net socket related
int start_listen_ipv6(int port, context *pcontext);
int handle_connection_ipv6(context *pcontext);

#ifdef __cplusplus
}
#endif//__cplusplus


#endif //_IPC_NET_HEADER_