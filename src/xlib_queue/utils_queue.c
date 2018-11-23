#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <unistd.h>

#include "utils_queue.h"
#include "comm_debug.h"

int queue_push(QUEUE_HANDLE pHandle, unsigned char *pHeader, unsigned char *pPayload, int len)
{
    if (NULL == pHandle) {
        return -1;
    }
    shm_t *pShmCtx = (shm_t *)pHandle;

    return tbsb_write_one_packet(pShmCtx, pHeader, pPayload, len);
}
int queue_pop(QUEUE_HANDLE pHandle, unsigned char *pHeader, unsigned char *pPayload, int len)
{
    if (NULL == pHandle) {
        return -1;
    }
    shm_t *pShmCtx = (shm_t *)pHandle;

    return tbsb_read_one_packet(pShmCtx, pHeader, pPayload, len);
}

QUEUE_HANDLE queue_creat(int packet_num, int packet_size, int header_size)
{
    int total_buffer_size = 0;
    void *pShmBaseAddr = NULL;
    shm_t *pShmCtx = NULL;

    total_buffer_size = sizeof(shm_t) +
                        header_size * packet_num +
                        sizeof(t_packet_info) * packet_num +
                        packet_size * packet_num;

    printf("queue_creat malloc %d bytes.[%d %d] [%ld + %d + %ld + %d]\n",
        total_buffer_size,
        packet_size,
        packet_num,
        sizeof(shm_t),
        header_size * packet_num,
        sizeof(t_packet_info) * packet_num,
        packet_size * packet_num);

    pShmBaseAddr = malloc(total_buffer_size);
    if (NULL == pShmBaseAddr) {
        return NULL;
    }

    pShmCtx = (shm_t *) pShmBaseAddr;

    pShmCtx->_header_offset    = sizeof(shm_t);
    pShmCtx->_packet_offset    = pShmCtx->_header_offset + header_size * packet_num;
    pShmCtx->_buf_start_offset = pShmCtx->_packet_offset + sizeof(t_packet_info) * packet_num;
    pShmCtx->_buf_total_size   = packet_size * packet_num;

    pShmCtx->_header_size      = header_size;
    pShmCtx->_buf_packet_num   = packet_num;

    printf("[%p ~ %p]\n", pShmBaseAddr, (unsigned char *)pShmBaseAddr + total_buffer_size -1);

    printf("shm: [0x%x ~ 0x%x]\n", 0, pShmCtx->_header_offset);
    printf("hdr: [0x%x ~ 0x%x]\n", pShmCtx->_header_offset, pShmCtx->_packet_offset);
    printf("pck: [0x%x ~ 0x%x]\n", pShmCtx->_packet_offset, pShmCtx->_buf_start_offset);
    printf("buf: [0x%x ~ 0x%x]\n", pShmCtx->_buf_start_offset, pShmCtx->_buf_start_offset + packet_size * packet_num);

    if (tbsb_init(pShmCtx) < 0) {
        printf("tbsb_init failed.\n");
        return NULL;
    }

    return (QUEUE_HANDLE) pShmCtx;
}

int queue_destory(QUEUE_HANDLE pHandle)
{
    if (NULL == pHandle) {
        printf("queue_destory param is invalid.\n");
        return -1;
    }
    shm_t *pShmCtx = (shm_t *)pHandle;

    tbsb_uninit(pShmCtx);

    return 0;
}