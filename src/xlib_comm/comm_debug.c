#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "comm_debug.h"

void comm_dump_client(struct LIST_TCP_REQ *Req)
{
    //printf("Connected cnt %d\n", pCommCtx->TcpReqCnt);

    printf("[%d]: Skt = %d ip = %s BufferAddr = %p CurRecvOffset = %d MaxBufLen = %d\n",
        Req->Idx,
        Req->Skt,
        inet_ntoa(Req->Addr.sin_addr),
        Req->BufferAddr,
        Req->CurRecvOffset,
        Req->MaxBufLen);
}

void comm_dump_list_req(COMM_MANAGER_S *pCommCtx)
{
    printf("Connected cnt %d\n", pCommCtx->TcpReqCnt);
    struct LIST_TCP_REQ *Req;

    for (Req = pCommCtx->TcpReqHead; Req != NULL; Req = Req->Next) {
        printf("[%d]: Skt = %d ip = %s BufferAddr = %p CurRecvOffset = %d MaxBufLen = %d\n",
            Req->Idx,
            Req->Skt,
            inet_ntoa(Req->Addr.sin_addr),
            Req->BufferAddr,
            Req->CurRecvOffset,
            Req->MaxBufLen);
    }
}

void comm_dump_hex(char *p, int len)
{
    int i = 0;
    printf("============================================\n");
    for (i = 0; i < len; i++) {
        if (i % 16 == 0 && i != 0) {
            printf("\n");
        }
        printf("%02X ", (unsigned char)p[i]);
    }
    printf("\n");
    printf("Recv %d bytes\n", len);
    printf("============================================\n");
}