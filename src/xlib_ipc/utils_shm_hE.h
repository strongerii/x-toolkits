#ifndef _UTILS_SHM_HE_H_
#define _UTILS_SHM_HE_H_

#include "xlib_type.h"


typedef enum {
    ERROR_NORMAL                     = -1,
    ERROR_TOTAL_BUFFER_NOT_ENOUGH    = -2,
    ERROR_READ_BUFFER_NOT_ENOUGH     = -3,
    ERROR_NO_FOUND_PACKET            = -4,
    ERROR_NO_SYNC                    = -5
} e_error_code;

typedef struct {
    u32 _packet_idx;
    u32 _packet_offset;
    u32 _packet_size;
    u32 _packet_state;

    //core
    pthread_condattr_t _cond_attr;
    pthread_mutexattr_t _mutex_attr;

    pthread_mutex_t _mutex;
    pthread_cond_t _cond;
} t_multi_packet_info;

//shm buffer
typedef struct
{
    u32 _is_cross_process;           //in

    //buffer
    u32 _buf_packet_offset;
    u32 _buf_start_offset;
    u32 _buf_total_size;            //in
    u32 _buf_packet_num;            //in

    //packet info
    u32 _cur_write_index;
    u32 _cur_write_offset;
    u32 _cur_used_buf_size;

} multi_shm_t;


#ifdef __cplusplus
extern "C"
{
#endif//__cplusplus

s32 shm_mutil_init(multi_shm_t *_pbsb);
void shm_mutil_uninit(multi_shm_t *_pbsb);
s32 shm_mutil_write_one_packet(multi_shm_t *pbsb, u8 *packet_addr, s32 packet_size);
s32 shm_mutil_read_one_packet(multi_shm_t *pbsb, u8 *buffer_addr, s32 packet_size, u32 *pcur_read_index, u32 *pcur_packet_index);
s32 shm_multi_read_sync(multi_shm_t *pbsb, u32 *pcur_read_index, u32 *pcur_packet_index);

#ifdef __cplusplus
}
#endif//__cplusplus

#endif //_UTILS_SHM_HE_H_