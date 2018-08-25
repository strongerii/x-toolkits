#ifndef _UTILS_FIFO_H_
#define _UTILS_FIFO_H_

#include "utils_shm.h"
#include "utils_shm_hE.h"

#ifdef __cplusplus
extern "C"
{
#endif//__cplusplus

int fifo_send(shm_t *pHandle, unsigned char *pBuffer, int len);
int fifo_recv(shm_t *pHandle, unsigned char *pBuffer, int len);

//thread
shm_t *fifo_creat(int packet_num, int buffer_size);
int fifo_destory(shm_t *pShm);

// process
shm_t *fifo_creat_shm(unsigned int ukey, int packet_num, int buffer_size);
int fifo_destroy_shm(unsigned int ukey, shm_t *pbsb);
shm_t *fifo_capture_shm(unsigned int ukey);
int fifo_release_shm(shm_t *pbsb);

// multi client read.
int fifo_multi_send(multi_shm_t *pHandle, unsigned char *pBuffer, int len);
int fifo_multi_recv(multi_shm_t *pHandle, unsigned char *pBuffer, int len);
multi_shm_t *fifo_creat_multi_shm(unsigned int ukey, int packet_num, int packet_size);
multi_shm_t *fifo_capture_multi_shm(unsigned int ukey);
int fifo_release_multi_shm(multi_shm_t *pbsb);
int fifo_destroy_multi_shm(unsigned int ukey, multi_shm_t *pbsb);


#ifdef __cplusplus
}
#endif//__cplusplus

#endif //_UTILS_FIFO_H_
