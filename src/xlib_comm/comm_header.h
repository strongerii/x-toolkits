#ifndef _DEF_COMM_HEADER_H_
#define _DEF_COMM_HEADER_H_

#include <sys/socket.h>
#include <netinet/in.h>

#include "utils_sink.h"
#include "comm_client_mgr.h"

#define USE_DLINKLIST

//////////////////////////////////////////////////////////////
typedef unsigned char                   u8;
typedef unsigned short                  u16;
typedef unsigned int                    u32;
typedef unsigned long long              u64;
typedef signed char                     s8;
typedef short                           s16;
typedef int                             s32;
typedef long long                       s64;
//////////////////////////////////////////////////////////////

#define DEF_COMM_SYNC_CODE              (0xAA55AA55)
#define DEF_COMM_SERVER_PORT            (9000)
#define DEF_RECV_BUFFER_MAX             (1000)

#define DEF_COMM_HEADER_SYNC_CODE_LEN   (4)
#define DEF_COMM_HEADER_LEN             (40)
#define DEF_COMM_HEADER_MSG_LEN_OFFSET  (16)

//////////////////////////////////////////////////////////////

typedef struct {
    unsigned int sync_code;
    unsigned int msg_type;
    unsigned int msg_from;
    unsigned int msg_to;

    unsigned int msg_len;
    unsigned int relay_len;
    unsigned int result;

    unsigned short ver;
    unsigned short seq;
    unsigned short enc;
    unsigned short rsv[3];
} COMM_MSG_HEADER_S;

//////////////////////////////////////////////////////////////

typedef int (*callback_msg_parse)(void *pHandle, void *pdata, int len);

struct LIST_TCP_REQ {
    int Idx;
    int Skt;
    struct sockaddr_in Addr;
    char *BufferAddr;
    int CurRecvOffset;
    int MaxBufLen;

    struct LIST_TCP_REQ *Next;
};

typedef struct COMM_MANAGER{
    int socket_fd;
    fd_set FdSet;
    int main_loop_exit;
    CLIENT_MGR_HANDLE client_mgr_handle;

    int TcpReqCnt;
    struct LIST_TCP_REQ *TcpReqNew;
    struct LIST_TCP_REQ *TcpReqHead;
    struct LIST_TCP_REQ *ListTcpReq[DEF_MAX_SINK_NUM];

    callback_msg_parse fCB_msg_parse;

} COMM_MANAGER_S;

#endif //_DEF_COMM_HEADER_H_
