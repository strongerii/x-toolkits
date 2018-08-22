#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "utils_net.h"
#include "utils_sink.h"
#include "comm_debug.h"
#include "comm_header.h"

int comm_find_magic_num(const char *SyncFlag, const int SyncFlagLen, char *SrcBuffer, int Len)
{
    int i = 0;
    int j = 0;
    int search_len = Len - SyncFlagLen;

    for (i = 0; i <= search_len; i++) {
        if (SyncFlag[0] == SrcBuffer[i]) {
            for (j = 1; j < SyncFlagLen; j++) {
                if (SyncFlag[j] != SrcBuffer[i + j]) {
                    i += j;
                    break;
                }
            }
            if (j == SyncFlagLen) {
                //find;
                return i;
            }
        }
    }
    return i;
}

int comm_raw_parse(COMM_MANAGER_S *pCommCtx,
                struct LIST_TCP_REQ *pCurReq,
                int CurRecvLen,
                int *NextRecvOffset,
                const char* MsgSyncFlag,
                const int MsgSyncFlagLen,
                const int MsgHeadLen,
                const int MsgDataLenOffset)
{
    char *Buff = pCurReq->BufferAddr;
    int CurRecvOffset = pCurReq->CurRecvOffset;

    int left_len = CurRecvOffset + CurRecvLen;
    int drop_len = 0;
    int drop = 0;

    while (1)
    {
        drop = comm_find_magic_num(MsgSyncFlag, MsgSyncFlagLen, &Buff[drop_len], left_len);
        left_len -= drop;
        drop_len += drop;

        if (left_len >= MsgHeadLen) {
            // pick msg len
            int msg_data_len = *(int *)&Buff[drop_len + MsgDataLenOffset];
            if (left_len >= MsgHeadLen + msg_data_len) {
                // msg parse
                if (pCommCtx->fCB_msg_parse) {
                    pCommCtx->fCB_msg_parse((void *)pCommCtx, &Buff[drop_len], MsgHeadLen + msg_data_len);
                }
                left_len -= (MsgHeadLen + msg_data_len);
                drop_len += (MsgHeadLen + msg_data_len);

            } else {
                // need recv more
                break;
            }
        } else {
            // need recv more
            break;
        }
    }
    if (left_len) {
        memmove(Buff, &Buff[drop_len], left_len);
        *NextRecvOffset = left_len;
    } else {
        *NextRecvOffset = 0;
    }

    return CurRecvLen;
}

int comm_tcp_recv(COMM_MANAGER_S *pCommCtx, struct LIST_TCP_REQ *pCurReq)
{
#if 1
    int cur_recv_len = (int) recv(pCurReq->Skt,
                                 &pCurReq->BufferAddr[pCurReq->CurRecvOffset],
                                  pCurReq->MaxBufLen - pCurReq->CurRecvOffset,
                                  0);
#else
    int cur_recv_len = receive_packet(pCurReq->Skt,
                                 (unsigned char *)&pCurReq->BufferAddr[pCurReq->CurRecvOffset],
                                  pCurReq->MaxBufLen - pCurReq->CurRecvOffset);
#endif

    comm_dump_hex(&pCurReq->BufferAddr[pCurReq->CurRecvOffset], cur_recv_len);

    if (cur_recv_len > 0)
    {
        int next_recv_offset = 0;

        int sync_word = DEF_COMM_SYNC_CODE;

        comm_raw_parse(pCommCtx,
                        pCurReq,
                        cur_recv_len,
                        &next_recv_offset,
                        (const char *)&sync_word,
                        DEF_COMM_HEADER_SYNC_CODE_LEN,
                        DEF_COMM_HEADER_LEN,
                        DEF_COMM_HEADER_MSG_LEN_OFFSET);

        pCurReq->CurRecvOffset = next_recv_offset;
    }

    return cur_recv_len;
}

#ifdef USE_DLINKLIST
int __comm_recv_callback(void *pkey, void *pdata, void *puserdata)
{
    if (NULL == pkey || NULL == pdata || NULL == puserdata) {
        printf("__comm_recv_callback param is invalid.\n");
        return -1;
    }

    int nRet = 0;
    COMM_MANAGER_S *pCommCtx = (COMM_MANAGER_S *)puserdata;
    struct LIST_TCP_REQ *pCurConnect = (struct LIST_TCP_REQ *)pdata;

    if (pCurConnect->Skt >= 0) {
        comm_dump_client(pCurConnect);

        if (FD_ISSET(pCurConnect->Skt, &pCommCtx->FdSet)) {
            nRet = comm_tcp_recv(pCommCtx, pCurConnect);
            if (nRet <= 0) {

                printf("comm_tcp_recv failed. Idx: %d, Skt: %d, recv:%d\n", pCurConnect->Idx, pCurConnect->Skt, nRet);

                //close socket.
                handle_disconnection(&pCurConnect->Skt);
                //remove connect client.
                comm_linklist_remove(pCommCtx->client_mgr_handle, pCurConnect->Idx);
            }
        }
    }
    return 0;
}

int __comm_fd_set_callback(void *pkey, void *pdata, void *puserdata)
{
    if (NULL == pkey || NULL == pdata || NULL == puserdata) {
        printf("__comm_fd_set_callback param is invalid.\n");
        return -1;
    }

    COMM_MANAGER_S *pCommCtx = (COMM_MANAGER_S *)puserdata;
    struct LIST_TCP_REQ *pCurConnect = (struct LIST_TCP_REQ *)pdata;

    if (pCurConnect->Skt >= 0) {
        FD_SET(pCurConnect->Skt, &pCommCtx->FdSet);
    }

    return 0;
}

int comm_client_process(COMM_MANAGER_S *pCommCtx)
{
    return comm_linklist_travel(pCommCtx->client_mgr_handle, __comm_recv_callback, (void *)pCommCtx);
}

void comm_fd_set(COMM_MANAGER_S *pCommCtx)
{
    if (NULL == pCommCtx) {
        return;
    }
    FD_SET(pCommCtx->socket_fd, &pCommCtx->FdSet);

    comm_linklist_travel(pCommCtx->client_mgr_handle, __comm_fd_set_callback, (void *)pCommCtx);
}

int comm_accept_handle(COMM_MANAGER_S *pCommCtx, int socket_fd, struct sockaddr_in *addr)
{
    if (NULL == pCommCtx) {
        return -1;
    }
    if (NULL == pCommCtx->client_mgr_handle) {
        pCommCtx->client_mgr_handle = comm_linklist_init();
        if (NULL == pCommCtx->client_mgr_handle) {
            printf("comm_linklist_init failed.\n");
            return -1;
        }
    }
    if (comm_linklist_insert(pCommCtx->client_mgr_handle, socket_fd, addr) < 0) {
        printf("comm_linklist_insert failed.\n");
        return -1;
    }

    return 0;
}

#else

int comm_client_process(COMM_MANAGER_S *pCommCtx)
{
    int nRet = 0;
    struct LIST_TCP_REQ *p_cur = pCommCtx->TcpReqHead;
    struct LIST_TCP_REQ *p_next = NULL;
    struct LIST_TCP_REQ *p_prev = NULL;

    while (p_cur) {
        p_next = p_cur->Next;
        if (-1 != p_cur->Skt) {
            if (FD_ISSET(p_cur->Skt, &pCommCtx->FdSet)) {
                nRet = comm_tcp_recv(pCommCtx, p_cur);
                if (nRet <= 0) {
                    printf("comm_tcp_recv failed. Idx: %d, Skt: %d, recv:%d\n",p_cur->Idx, p_cur->Skt, nRet);
                    handle_disconnection(&p_cur->Skt);

                    pCommCtx->ListTcpReq[p_cur->Idx] = NULL;
                    set_invalid_sink_id(p_cur->Idx);

                    free(p_cur);
                    p_cur = NULL;

                    if(NULL == p_prev) {
                        pCommCtx->TcpReqHead = p_next;
                    } else {
                        p_prev->Next = p_next;
                    }
                    pCommCtx->TcpReqCnt--;
                }
            }
        }
        p_prev = p_cur;
        p_cur = p_next;
    }

    comm_dump_list_req(pCommCtx);

    return 0;
}
void comm_fd_set(COMM_MANAGER_S *pCommCtx)
{
    struct LIST_TCP_REQ *p_cur = NULL;
    struct LIST_TCP_REQ *p_new = NULL;
    struct LIST_TCP_REQ *p_next = NULL;

    if (NULL == pCommCtx) {
        return;
    }
    FD_SET(pCommCtx->socket_fd, &pCommCtx->FdSet);

    if(pCommCtx->TcpReqNew) {
        p_new = pCommCtx->TcpReqNew;
        p_next = NULL;
        while (p_new) {
            p_next = p_new->Next;
            p_new->Next = pCommCtx->TcpReqHead;
            pCommCtx->TcpReqHead = p_new;
            p_new = p_next;
        }
        pCommCtx->TcpReqNew = NULL;
    }

    p_cur = pCommCtx->TcpReqHead;
    while (p_cur) {
        if (-1 != p_cur->Skt) {
            FD_SET(p_cur->Skt, &pCommCtx->FdSet);
        }
        p_cur = p_cur->Next;
    }
}
int comm_accept_handle(COMM_MANAGER_S *pCommCtx, int socket_fd, struct sockaddr_in *addr)
{
    if (NULL == pCommCtx) {
        return -1;
    }
    int index = 0;
    int max_buf_len = sizeof(struct LIST_TCP_REQ) + DEF_RECV_BUFFER_MAX;
    struct LIST_TCP_REQ *pTcpReqNew = (struct LIST_TCP_REQ *)malloc(max_buf_len);

    if (pTcpReqNew) {
        memset(pTcpReqNew, 0, max_buf_len);

        index = get_valid_sink_id();
        if (index < 0) {
            printf("get_valid_sink_id failed.[%d]\n", index);
            return -1;
        }
        pTcpReqNew->Idx = index;
        pTcpReqNew->Skt = socket_fd;
        pTcpReqNew->Next = pCommCtx->TcpReqNew;
        pTcpReqNew->BufferAddr = (char *)(pTcpReqNew + sizeof(struct LIST_TCP_REQ));
        pTcpReqNew->MaxBufLen = DEF_RECV_BUFFER_MAX;
        memcpy(&pTcpReqNew->Addr, addr, sizeof(struct sockaddr_in));

        pCommCtx->TcpReqNew = pTcpReqNew;
        pCommCtx->ListTcpReq[pTcpReqNew->Idx] = pTcpReqNew;
        pCommCtx->TcpReqCnt++;
    }

    return 0;
}

#endif

void comm_fd_isset(COMM_MANAGER_S *pCommCtx)
{
    int new_skt = -1;
    struct sockaddr_in from_addr;
    socklen_t addr_len = (socklen_t)sizeof(struct sockaddr_in);

    // process last req.
    comm_client_process(pCommCtx);

    // check new connect.
    if (FD_ISSET(pCommCtx->socket_fd, &pCommCtx->FdSet)) {
        new_skt = accept(pCommCtx->socket_fd, (struct sockaddr *)&from_addr, &addr_len);

        printf("New: %d [%s]\n", new_skt, inet_ntoa(from_addr.sin_addr));

        if (new_skt >= 0) {
            if (comm_accept_handle(pCommCtx, new_skt, &from_addr) < 0) {
                printf("comm_accept_handle is failed.\n");
                handle_disconnection(&new_skt);
            }
        } else {
            printf("accept failed.[%m]\n");
        }
    }
}
int comm_parse_creat(COMM_MANAGER_S *pCommCtx)
{
	if (NULL == pCommCtx) {
		return -1;
	}

	pCommCtx->client_mgr_handle = comm_linklist_init();
    if (NULL == pCommCtx->client_mgr_handle) {
        printf("comm_linklist_init failed.\n");
        return -1;
    }

	return 0;
}

void comm_parse_destory(COMM_MANAGER_S *pCommCtx)
{
	if (NULL == pCommCtx) {
		return;
	}
	comm_linklist_uninit(pCommCtx->client_mgr_handle);
}
