#ifndef _SHMMAP_H_
#define _SHMMAP_H_

#include "common.h"
#include "sh_tbitstream.h"

#ifdef __cplusplus
extern "C"
{
#endif//__cplusplus

shm_t *creat_shm_t(u32 ukey);
shm_t *capture_shm_t(u32 ukey);
s32 release_shm_t(shm_t *pbsb);
s32 destroy_shm_t(u32 ukey,shm_t *pbsb);

#ifdef __cplusplus
}
#endif//__cplusplus

#endif //_SHMMAP_H_
