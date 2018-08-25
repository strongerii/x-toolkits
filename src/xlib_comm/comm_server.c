#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>

#include "utils_net.h"
#include "utils_sink.h"
#include "comm_debug.h"
#include "comm_header.h"
#include "comm_parse.h"

int __comm_msg_parse(void *pHandle, void *pdata, int len)
{
    //COMM_MANAGER_S *pCommCtx = (COMM_MANAGER_S *)pHandle;
    COMM_MSG_HEADER_S *pHeader = (COMM_MSG_HEADER_S *) pdata;

    char *pPayload = pdata + sizeof(COMM_MSG_HEADER_S) + pHeader->relay_len;

    pPayload[pHeader->msg_len] = '\0';

    printf("[0x%x][From: 0x%08x ----> To: 0x%08x]: msg_type = %2d msg_len = %3d relay_len: %d ==> %s\n",
        pHeader->sync_code,
        pHeader->msg_from,
        pHeader->msg_to,
        pHeader->msg_type,
        pHeader->msg_len,
        pHeader->relay_len,
        (char *)pPayload);

    return 0;
}

COMM_MANAGER_S *__comm_params_init()
{
    COMM_MANAGER_S *pContx = (COMM_MANAGER_S *)malloc(sizeof(COMM_MANAGER_S));
    memset(pContx, 0, sizeof(COMM_MANAGER_S));

    pContx->main_loop_exit = 0;
    pContx->fCB_msg_parse = __comm_msg_parse;

	if (comm_parse_creat(pContx) < 0) {
		printf("comm_parse_creat failed.\n");
	}

    return pContx;
}

void __comm_params_uninit(COMM_MANAGER_S *pContx)
{
	comm_parse_destory(pContx);
}

static void __mainloop(COMM_MANAGER_S *pCommCtx)
{
    int s32Ret = 0;
    struct timeval tm;

    while (0 == pCommCtx->main_loop_exit) {
        tm.tv_sec = 0;
        tm.tv_usec = 50000;

        FD_ZERO(&pCommCtx->FdSet);
        comm_fd_set(pCommCtx);

        s32Ret = select(FD_SETSIZE, &pCommCtx->FdSet, /*(fd_set *)*/ NULL, /*(fd_set *)*/ NULL, &tm);
        if (s32Ret < 0) {
            printf("select failed[%d]\n", s32Ret);
            break;
        } else if (0 == s32Ret) {
            //time out
            continue;
        } else {
            comm_fd_isset(pCommCtx);
        }
    }
}

static void __sigstop(int signo)
{
    printf("catch signal %d\n", signo);

    printf("====exit comm server======\n");

    exit(1);
}

int main(int argc, char **argv)
{
    int socket_listen = -1;
    COMM_MANAGER_S *pCommCtx;

    signal(SIGPIPE, SIG_IGN);
    signal(SIGINT, __sigstop);
    signal(SIGQUIT, __sigstop);
    signal(SIGTERM, __sigstop);
    signal(SIGSEGV, __sigstop);
    signal(SIGBUS, __sigstop);

    printf("====start comm server======\n");

    pCommCtx = __comm_params_init();
    if (NULL == pCommCtx) {
        printf("__comm_params_init failed!\n");
        __sigstop(SIGTERM);
        return -1;
    }

    socket_listen = start_listen(DEF_COMM_SERVER_PORT);
    if (socket_listen < 0) {
        printf("start listen port:%d failed!\n", DEF_COMM_SERVER_PORT);
        __sigstop(SIGTERM);
        return -1;
    }

    pCommCtx->socket_fd = socket_listen;

    __mainloop(pCommCtx);

    handle_disconnection(&pCommCtx->socket_fd);

    __comm_params_uninit(pCommCtx);

    return 0;
}