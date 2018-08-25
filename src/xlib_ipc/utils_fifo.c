#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <unistd.h>

#include "utils_fifo.h"
#include "comm_debug.h"

int fifo_send(shm_t *pHandle, unsigned char *pBuffer, int len)
{
    return tbsb_write_one_packet(pHandle, NULL, pBuffer, len);
}
int fifo_recv(shm_t *pHandle, unsigned char *pBuffer, int len)
{
    return tbsb_read_one_packet(pHandle, NULL, pBuffer, len);
}

shm_t *fifo_creat(int packet_num, int packet_size)
{
    int total_buffer_size = 0;
    void *pShmBaseAddr = NULL;
    shm_t *pShmCtx = NULL;

    total_buffer_size = sizeof(shm_t) +
                        sizeof(t_header_info) * packet_num +
                        sizeof(t_packet_info) * packet_num +
                        packet_size * packet_num;

    int page_size = sysconf(_SC_PAGESIZE);
    if(total_buffer_size % page_size)
    {
        total_buffer_size = (page_size - total_buffer_size % page_size) + total_buffer_size;
    }

    printf("fifo_creat malloc %d x %d bytes.[%d %d] page_size = %d. [%ld %ld %ld %d]\n",
        total_buffer_size, packet_num, packet_size, packet_num, page_size,
        sizeof(shm_t), sizeof(t_header_info) * packet_num, sizeof(t_packet_info) * packet_num, packet_size * packet_num);

    pShmBaseAddr = malloc(total_buffer_size);
    if (NULL == pShmBaseAddr) {
        return NULL;
    }

    pShmCtx = (shm_t *) pShmBaseAddr;

    pShmCtx->_header_offset    = sizeof(shm_t);
    pShmCtx->_packet_offset    = pShmCtx->_header_offset + sizeof(t_header_info) * packet_num;
    pShmCtx->_buf_start_offset = pShmCtx->_packet_offset + sizeof(t_packet_info) * packet_num;
    pShmCtx->_buf_total_size   = packet_size * packet_num;

    pShmCtx->_buf_packet_num   = packet_num;

    printf("[%p ~ %p]\n", pShmBaseAddr, (u8 *)pShmBaseAddr + total_buffer_size -1);

    printf("shm: [0x%x ~ 0x%x]\n", 0, pShmCtx->_header_offset);
    printf("hdr: [0x%x ~ 0x%x]\n", pShmCtx->_header_offset, pShmCtx->_packet_offset);
    printf("pck: [0x%x ~ 0x%x]\n", pShmCtx->_packet_offset, pShmCtx->_buf_start_offset);
    printf("buf: [0x%x ~ 0x%x]\n", pShmCtx->_buf_start_offset, pShmCtx->_buf_start_offset + packet_size * packet_num);

    if (tbsb_init(pShmCtx) < 0) {
        printf("tbsb_init failed.\n");
        return NULL;
    }

    return pShmCtx;
}

int fifo_destory(shm_t *pShm)
{
    if (NULL == pShm) {
        printf("fifo_destory param is invalid.\n");
        return -1;
    }
    tbsb_uninit(pShm);

    return 0;
}

shm_t *fifo_creat_shm(unsigned int ukey, int packet_num, int packet_size)
{
    int shmid = -1;
    shm_t *pShmCtx = NULL;
    void *shmaddr = NULL;
    int total_buffer_size = 0;

    total_buffer_size = sizeof(shm_t) +
                        sizeof(t_header_info) * packet_num +
                        sizeof(t_packet_info) * packet_num +
                        packet_size * packet_num;

    int page_size = sysconf(_SC_PAGESIZE);
    if(total_buffer_size % page_size) {
        total_buffer_size = (page_size - total_buffer_size % page_size) + total_buffer_size;
    }

    if((shmid = shmget((key_t)ukey, total_buffer_size, 0666 | IPC_CREAT)) < 0){
        printf("creat share map [%d] failed!\n",ukey);
        return NULL;
    }
    if((shmaddr = shmat(shmid, NULL, 0)) == (void *)-1){
        printf("shmat share map [%d] failed \n",ukey);
        return NULL;
    }
    pShmCtx = (shm_t *)shmaddr;

    pShmCtx->_is_cross_process = 1;

    pShmCtx->_header_offset    = sizeof(shm_t);
    pShmCtx->_packet_offset    = pShmCtx->_header_offset + sizeof(t_header_info) * packet_num;
    pShmCtx->_buf_start_offset = pShmCtx->_packet_offset + sizeof(t_packet_info) * packet_num;
    pShmCtx->_buf_total_size   = packet_size * packet_num;

    pShmCtx->_buf_packet_num   = packet_num;

    printf("[%p ~ %p]\n", shmaddr, (u8 *)shmaddr + total_buffer_size -1);

    printf("shm: [0x%x ~ 0x%x]\n", 0, pShmCtx->_header_offset);
    printf("hdr: [0x%x ~ 0x%x]\n", pShmCtx->_header_offset, pShmCtx->_packet_offset);
    printf("pck: [0x%x ~ 0x%x]\n", pShmCtx->_packet_offset, pShmCtx->_buf_start_offset);
    printf("buf: [0x%x ~ 0x%x]\n", pShmCtx->_buf_start_offset, pShmCtx->_buf_start_offset + packet_size * packet_num);

    printf("[key: %x]:creat: shmaddr = %p\n", ukey, shmaddr);

    if(tbsb_init(pShmCtx) < 0){
        printf("tbsb_init failed !\n");
        return NULL;
    }

    comm_dump_hex((char *)shmaddr, 176);
    comm_dump_hex((char *)shmaddr + pShmCtx->_packet_offset, 120);

    return pShmCtx;
}

shm_t *fifo_capture_shm(unsigned int ukey)
{
    int shmid = -1;
    shm_t *pshm_t = NULL;
    void *shmaddr = NULL;

    if((shmid = shmget((key_t)ukey,0,0)) < 0){
        perror("capture pshm_t Fail to shmget\n");
        return NULL;
    }

    //map the shared memory to current process
    if((shmaddr = shmat(shmid,NULL,0)) == (void *)-1){
        perror("capture pshm_t Fail to shmat\n");
        return NULL;
    }

    printf("[key: %x]:capture: shmaddr = %p\n", ukey, shmaddr);

    pshm_t = (shm_t *)shmaddr;

    comm_dump_hex((char *)shmaddr, 176);
    comm_dump_hex((char *)shmaddr + pshm_t->_packet_offset, 120);

    return pshm_t;
}

int fifo_release_shm(shm_t *pbsb)
{
     void *shmaddr = NULL;
     if(NULL == pbsb){
         perror("release_shm_vbsb NULL==pbsb\n");
        return -1;
     }
    shmaddr = (void *)pbsb;
    if(shmdt(shmaddr) < 0)
    {
        perror("release_shm_vbsb Fail to shmdt\n");
        return -1;
    }
    return 0;

}

int fifo_destroy_shm(unsigned int ukey, shm_t *pbsb)
{
     int shmid = -1;
     void *shmaddr = NULL;
     if(NULL == pbsb){
         perror("destroy_shm_vbsb NULL==pbsb\n");
        return -1;
     }

     if(pbsb){
        tbsb_uninit(pbsb);
     }

    //unmount
    shmaddr = (void *)pbsb;
    if(shmdt(shmaddr) < 0)
    {
        perror("destroy_shm_vbsb Fail to shmdt\n");
        return -1;
    }

     //get shared memory that already be created
    if((shmid = shmget((key_t)ukey,0,0)) < 0)
    {
        perror("destroy_shm_vbsb Fail to shmget\n");
        return -1;
    }
    //delete it
    if(shmctl(shmid,IPC_RMID,NULL) < 0)
    {
        perror("destroy_shm_vbsb Fail to shmctl\n");
        return -1;
    }

    return 0;
}

int fifo_multi_send(multi_shm_t *pHandle, unsigned char *pBuffer, int len)
{
    return shm_mutil_write_one_packet(pHandle, pBuffer, len);
}
int fifo_multi_recv(multi_shm_t *pHandle, unsigned char *pBuffer, int len)
{
    static unsigned int read_index = 0;
    static unsigned int packet_index = 0;
    int ret = 0;
    if (0 == packet_index) {
        shm_multi_read_sync(pHandle, &read_index, &packet_index);
        printf("---------------- Read Sync: read_index = %d packet_index = %d\n", read_index, packet_index);
    }
    ret = shm_mutil_read_one_packet(pHandle, pBuffer, len, &read_index, &packet_index);
    if (ret < 0) {
       //read_index++;
        //packet_index++;
    }
    return ret;
}

multi_shm_t *fifo_creat_multi_shm(unsigned int ukey, int packet_num, int packet_size)
{
    int shmid = -1;
    multi_shm_t *pShmCtx = NULL;
    void *shmaddr = NULL;
    int total_buffer_size = 0;

    total_buffer_size = sizeof(multi_shm_t) +
                        sizeof(t_packet_info) * packet_num +
                        packet_size * packet_num;

    int page_size = sysconf(_SC_PAGESIZE);
    if(total_buffer_size % page_size) {
        total_buffer_size = (page_size - total_buffer_size % page_size) + total_buffer_size;
    }

    if((shmid = shmget((key_t)ukey, total_buffer_size, 0666 | IPC_CREAT)) < 0){
        printf("creat share map [%d] failed!\n",ukey);
        return NULL;
    }
    if((shmaddr = shmat(shmid, NULL, 0)) == (void *)-1){
        printf("shmat share map [%d] failed \n",ukey);
        return NULL;
    }
    pShmCtx = (multi_shm_t *)shmaddr;

    pShmCtx->_is_cross_process = 1;

    pShmCtx->_buf_total_size   = packet_size * packet_num;
    pShmCtx->_buf_packet_num   = packet_num;

    printf("[%p ~ %p]\n", shmaddr, (u8 *)shmaddr + total_buffer_size -1);

    printf("malloc %d x %d bytes.[%d %d] page_size = %d. [%ld %ld %d]\n",
        packet_size, packet_num, packet_size, packet_num, page_size,
        sizeof(multi_shm_t), sizeof(t_multi_packet_info) * packet_num, packet_size * packet_num);

    printf("[key: %x]:creat: shmaddr = %p\n", ukey, shmaddr);

    if(shm_mutil_init(pShmCtx) < 0){
        printf("tbsb_init failed !\n");
        return NULL;
    }

    comm_dump_hex((char *)shmaddr, 176);
    //comm_dump_hex((char *)shmaddr + pShmCtx->_packet_offset, 120);

    return pShmCtx;
}

multi_shm_t *fifo_capture_multi_shm(unsigned int ukey)
{
    int shmid = -1;
    multi_shm_t *pshm_t = NULL;
    void *shmaddr = NULL;

    if((shmid = shmget((key_t)ukey,0,0)) < 0){
        perror("capture pshm_t Fail to shmget\n");
        return NULL;
    }

    //map the shared memory to current process
    if((shmaddr = shmat(shmid,NULL,0)) == (void *)-1){
        perror("capture pshm_t Fail to shmat\n");
        return NULL;
    }

    printf("[key: %x]:capture: shmaddr = %p\n", ukey, shmaddr);

    pshm_t = (multi_shm_t *)shmaddr;

    comm_dump_hex((char *)shmaddr, 176);
    //comm_dump_hex((char *)shmaddr + pshm_t->_packet_offset, 120);

    return pshm_t;
}

int fifo_release_multi_shm(multi_shm_t *pbsb)
{
     void *shmaddr = NULL;
     if(NULL == pbsb){
         perror("release_shm_vbsb NULL==pbsb\n");
        return -1;
     }
    shmaddr = (void *)pbsb;
    if(shmdt(shmaddr) < 0)
    {
        perror("release_shm_vbsb Fail to shmdt\n");
        return -1;
    }
    return 0;

}

int fifo_destroy_multi_shm(unsigned int ukey, multi_shm_t *pbsb)
{
     int shmid = -1;
     void *shmaddr = NULL;
     if(NULL == pbsb){
         perror("destroy_shm_vbsb NULL==pbsb\n");
        return -1;
     }

     if(pbsb){
        shm_mutil_uninit(pbsb);
     }

    //unmount
    shmaddr = (void *)pbsb;
    if(shmdt(shmaddr) < 0)
    {
        perror("destroy_shm_vbsb Fail to shmdt\n");
        return -1;
    }

     //get shared memory that already be created
    if((shmid = shmget((key_t)ukey,0,0)) < 0)
    {
        perror("destroy_shm_vbsb Fail to shmget\n");
        return -1;
    }
    //delete it
    if(shmctl(shmid,IPC_RMID,NULL) < 0)
    {
        perror("destroy_shm_vbsb Fail to shmctl\n");
        return -1;
    }

    return 0;
}
