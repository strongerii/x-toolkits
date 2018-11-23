#ifndef _DEF_UTILS_QUEUE_H_
#define _DEF_UTILS_QUEUE_H_

#include "utils_mem.h"

typedef void* QUEUE_HANDLE;


#ifdef __cplusplus
extern "C"
{
#endif//__cplusplus

int queue_push(QUEUE_HANDLE pHandle, unsigned char *pHeader, unsigned char *pPayload, int len);
int queue_pop(QUEUE_HANDLE pHandle, unsigned char *pHeader, unsigned char *pPayload, int len);
QUEUE_HANDLE queue_creat(int packet_num, int packet_size, int header_size);
int queue_destory(QUEUE_HANDLE pHandle);


#ifdef __cplusplus
}
#endif//__cplusplus

#endif //_DEF_UTILS_QUEUE_H_
