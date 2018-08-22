#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>
#include <signal.h>


#include "utils_net.h"
#include "comm_header.h"
#include "comm_debug.h"

static int g_LoopExit = 0;

void print_help()
{
    printf("command:\n");
    printf("\t-i: server ip addr .\n");
    printf("\t-p: server port.\n");
    printf("\t-h: Help.\n");
}

static int __comm_send_msg(int socket_fd, COMM_MSG_HEADER_S *pHeader, char *pPayload, unsigned int len)
{
    int ret = -1;

    if (NULL == pHeader) {
        printf("pHeader is NULL");
        return -1;
    }

    printf("[0x%x][From: 0x%08x ----> To: 0x%08x]: Type = %2d MsgLen = %3d RlyLen: %d\n",
        pHeader->sync_code, pHeader->msg_from, pHeader->msg_to, pHeader->msg_type, pHeader->msg_len,
        pHeader->relay_len);

    //Send Header + Rly
    ret = send(socket_fd, (char *)pHeader, sizeof(COMM_MSG_HEADER_S) + pHeader->relay_len, 0);
    if(ret < 0)
    {
        printf("send header fail skt[%d][%m]\n", socket_fd);
    }
    comm_dump_hex((char *)pHeader, sizeof(COMM_MSG_HEADER_S) + pHeader->relay_len);
    printf("Send Header: %d bytes.\n", ret);
    // Send Payload
    if (pPayload && len > 0)
    {
        ret = send(socket_fd, (char *)pPayload, len, 0);
        if(ret < 0)
        {
            printf("send payload fail skt[%d][%m]\n", socket_fd);
        }
        comm_dump_hex((char *)pPayload, len);
        printf("Send Payload: %d bytes.\n", ret);
    }

    return ret;
}

int comm_pack_msg(int socket_fd, char* pPayload, unsigned int len)
{
    COMM_MSG_HEADER_S tHeader;

    memset(&tHeader, 0, sizeof(COMM_MSG_HEADER_S));

    tHeader.sync_code  = DEF_COMM_SYNC_CODE;
    tHeader.msg_from   = 1;
    tHeader.msg_to     = 2;
    tHeader.msg_type   = 1234;
    tHeader.msg_len    = len;

    return __comm_send_msg(socket_fd, &tHeader, pPayload, len);
}

static void __sigstop(int signo)
{
    printf("Catch signal %d\n", signo);
    g_LoopExit = 1;
}

int main(int argc, char *argv[])
{
    int c;
    int isExit = 0;
    int socketfd = -1;
    char cmd[1024] = {0};
    char *pServerIP = "127.0.0.1";
    unsigned short port = DEF_COMM_SERVER_PORT;

    signal(SIGPIPE, SIG_IGN);
    signal(SIGINT, __sigstop);
    signal(SIGQUIT, __sigstop);
    signal(SIGTERM, __sigstop);
    signal(SIGSEGV, __sigstop);
    signal(SIGBUS, __sigstop);

    while ((c = getopt(argc, argv, "i:p:h")) != -1)
    {
        switch (c)
        {
        case 'i':
            pServerIP = optarg;
            break;
        case 'p':
            port = atoi(optarg);
        case 'h':
        default:
            print_help();
            isExit = 1;
            break;
        }
    }

    if (isExit) {
        exit(0);
    }

    socketfd = start_connection(pServerIP, port);


    while(0 == g_LoopExit) {
        printf("#: ");

        fflush(stdin);

        if (fgets(cmd, 1024, stdin) == NULL) {
            break;
        }

        if (g_LoopExit) {
            break;
        }
        //send_packet(socketfd, cmd, strlen(cmd));
        comm_pack_msg(socketfd, cmd, strlen(cmd));
    }

    handle_disconnection(&socketfd);

    return 0;
}
