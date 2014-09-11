#ifndef _SH_TBITSTREAM_H_
#define _SH_TBITSTREAM_H_

#include "common.h"

#define DEF_MAX_PACKETNUM 					(8)
#define DEF_MAX_PACKETSIZE					(DEF_MAX_PACKETNUM *sizeof(t_packet_info))
#define DEF_MAX_HEADERSIZE					(DEF_MAX_PACKETNUM *sizeof(t_header_info))
#define DEF_MAX_SHARE_BUFSIZE				(1024*1024)

typedef enum {

	SHPKT_INVALID = 0, 
	SHPKET_VALID
}e_shpacket_state;

typedef struct 
{
	u32 _packet_offset;
	u32 _packet_size;
	u32 _packet_state;
}t_packet_info;

typedef struct{
	u32 id;
	u32 timestamp;

}t_header_info;

//shm buffer
typedef struct 
{
	//buffer
	u8	_buf_start_addr[DEF_MAX_SHARE_BUFSIZE];
	u32 _buf_next_write_offset;
	u32 _buf_next_read_offset;
	u32 _buf_total_size;
	
	//header info
	u8  _header_addr[DEF_MAX_HEADERSIZE];
	u32 _header_size;

	//packet info
	t_packet_info _packet_addr[DEF_MAX_PACKETSIZE];
	u32 _max_packet_num;
	u32 _cur_packet_num;
	
	//sham info
	s32 _shm_index_first;
	s32 _shm_index_last;
	s32 _shm_index_for_next_read;
	s32 _shm_index_for_next_write;
	s32 _shm_index_for_being_used;
	
	//core
	pthread_condattr_t _cond_attr;
	pthread_mutexattr_t _mutex_attr;

	pthread_mutex_t	_mutex;
	pthread_cond_t	_cond;
	
}shm_t;


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

#endif //_SH_TBITSTREAM_H_