#ifndef _UTILS_SHM_H_
#define _UTILS_SHM_H_

#include "xlib_type.h"

typedef enum {
    SHPKT_INVALID = 0,
    SHPKET_VALID
} e_packet_state;

typedef struct {
    u32 _packet_offset;
    u32 _packet_size;
    u32 _packet_state;
} t_packet_info;

typedef struct {
    u32 id;
    u32 timestamp;
} t_header_info;

//shm buffer
typedef struct
{
    u32 _is_cross_process;
    //buffer
    u32 _buf_start_offset;          //in
    u32 _buf_next_write_offset;
    u32 _buf_next_read_offset;
    u32 _buf_total_size;            //in
    u32 _buf_packet_num;            //in
    //u32 _buf_used_size;

    //header info
    u32 _header_offset;             //in
    //u32 _header_size;

    //packet info
    u32 _packet_offset;             //in
    u32 _max_packet_num;
    u32 _cur_packet_num;

    //shm info
    s32 _shm_index_first;
    s32 _shm_index_last;
    s32 _shm_index_for_next_read;
    s32 _shm_index_for_next_write;
    s32 _shm_index_for_being_used;

    //core
    pthread_condattr_t _cond_attr;
    pthread_mutexattr_t _mutex_attr;

    pthread_mutex_t _mutex;
    pthread_cond_t _cond;

} shm_t;


#ifdef __cplusplus
extern "C"
{
#endif//__cplusplus

s32 tbsb_init(shm_t *pbsb);
void tbsb_uninit(shm_t *_pbsb);
s32 tbsb_write_one_packet(shm_t *pbsb, u8 *pheader, u8 *packet_addr, s32 packet_size);
s32 tbsb_read_one_packet(shm_t *pbsb, u8 *pheader, u8 *packet_addr, s32 packet_size);

#ifdef __cplusplus
}
#endif//__cplusplus

#endif //_UTILS_SHM_H_