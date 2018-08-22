#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "comm_header.h"
#include "utils_sink.h"
#include "comm_client_mgr.h"

CLIENT_MGR_HANDLE comm_linklist_init()
{
    // init sink
    init_sink_items();
	
    t_dlinklist *plst = create_dlinklist();
    if (plst) {
        return (CLIENT_MGR_HANDLE)plst;
    } else {
        return NULL;
    }
}

void comm_linklist_uninit(CLIENT_MGR_HANDLE pHandle)
{
	destroy_dlinklist((t_dlinklist *)pHandle);
	uninit_sink_items();
}

int comm_linklist_insert(CLIENT_MGR_HANDLE pHandle, int socket_fd, struct sockaddr_in *pAddr)
{
    int TotalBufferSize = 0;
    struct LIST_TCP_REQ *pConnectNew = NULL;
    t_sink_item *pSink = NULL;
    t_dlinklist *plst = (t_dlinklist *)pHandle;

    TotalBufferSize = sizeof(struct LIST_TCP_REQ) + DEF_RECV_BUFFER_MAX;

    pConnectNew = (struct LIST_TCP_REQ *)malloc(TotalBufferSize);
    if (NULL == pConnectNew) {
        printf("malloc %d bytes failed.[%m]\n", TotalBufferSize);
        return -1;
    }

    memset(pConnectNew, 0, TotalBufferSize);

    pSink = get_valid_sink_item();
    if (NULL == pSink) {
        printf("get_valid_sink_item failed.[%d]\n", pSink->idx);
        return -1;
    }

    pConnectNew->Idx         = pSink->idx;
    pConnectNew->Skt         = socket_fd;
    pConnectNew->BufferAddr  = (char *)(pConnectNew + sizeof(struct LIST_TCP_REQ));
    pConnectNew->MaxBufLen   = DEF_RECV_BUFFER_MAX;
    memcpy(&pConnectNew->Addr, pAddr, sizeof(struct sockaddr_in));

    if (insert_dlinklist_tail(plst, (void *)pSink, (void *)pConnectNew) < 0) {
        printf("insert_dlinklist_tail failed.\n");
        return -1;
    }

    return 0;
}

int comm_linklist_remove(CLIENT_MGR_HANDLE pHandle, int idx)
{
    int nRet = 0;
    t_sink_item *pSink = NULL;
    t_dlinklist *plst = (t_dlinklist *)pHandle;

    pSink = get_sink_item_by_idx(idx);
    if (NULL == pSink) {
        printf("get_sink_item_by_idx failed.\n");
        return -1;
    }

    // Notes: internel already free pdata.
    if (delete_dlinklist_node(plst, pSink) < 0) {
        printf("delete_dlinklist_node failed.\n");
        nRet = -1;
        goto EXIT;
    }

EXIT:
    set_invalid_sink_id(pSink->idx);

    return nRet;
}

int comm_linklist_travel(CLIENT_MGR_HANDLE pHandle, __PFNTravelCB pfn, void *puserdata)
{
    if (NULL == pHandle) {
        return -1;
    }
    t_dlinklist *plst = (t_dlinklist *)pHandle;

    return travel_dlinklist(plst, pfn, puserdata);
}