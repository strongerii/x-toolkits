#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include "sh_tbitstream.h"

static void cancel_read_wait(void *arg)
{
	pthread_mutex_t *mutex = arg;
	pthread_mutex_unlock(mutex);

	printf("cancel_read_wait \n");
}

s32 tbsb_init(shm_t *_pbsb)
{
	if(NULL == _pbsb){
		return -1;
	}
	//init buffer
	_pbsb->_buf_total_size = sizeof(_pbsb->_buf_start_addr);
	_pbsb->_buf_next_write_offset = 0;
	_pbsb->_buf_next_read_offset = 0;
	
	//init headers info
	_pbsb->_header_size = sizeof(t_header_info);
	
	//init packs info
	_pbsb->_max_packet_num = DEF_MAX_PACKETNUM;
	_pbsb->_cur_packet_num = 0;
	
	//init shm_info
	_pbsb->_shm_index_first = 0;
	_pbsb->_shm_index_last = DEF_MAX_PACKETNUM - 1;
	_pbsb->_shm_index_for_next_read = 0;
	_pbsb->_shm_index_for_next_write = 0;
	_pbsb->_shm_index_for_being_used = -1;//no packet being used
	
	//init core
	pthread_mutexattr_init(&_pbsb->_mutex_attr);
	pthread_mutexattr_setpshared(&_pbsb->_mutex_attr, PTHREAD_PROCESS_SHARED);

	pthread_condattr_init(&_pbsb->_cond_attr);
	pthread_condattr_setpshared(&_pbsb->_cond_attr, PTHREAD_PROCESS_SHARED);

	pthread_mutex_init(&_pbsb->_mutex, &_pbsb->_mutex_attr);
	pthread_cond_init(&_pbsb->_cond, &_pbsb->_cond_attr);
	
	//reset buffers
	memset(_pbsb->_buf_start_addr, 0, sizeof(_pbsb->_buf_start_addr));
	memset(_pbsb->_header_addr, 0, sizeof(_pbsb->_header_addr));
	memset(_pbsb->_packet_addr, 0, sizeof(_pbsb->_packet_addr));
	
	return 0;
	
}

void tbsb_uninit(shm_t *_pbsb)
{
	if(NULL == _pbsb){
		return;
	}
	pthread_condattr_destroy(&_pbsb->_cond_attr);
	pthread_mutexattr_destroy(&_pbsb->_mutex_attr);
	pthread_cond_destroy(&_pbsb->_cond);	
	pthread_mutex_destroy(&_pbsb->_mutex);

	return;
}

s32 tbsb_write_one_packet(shm_t *pbsb, u8 *header, u8 *packet_addr, s32 packet_size)
{
	s32 rev = -1;
	if(NULL == pbsb || NULL == header || NULL == packet_addr || 
		packet_size <= 0 || packet_size > DEF_MAX_SHARE_BUFSIZE){
		return rev;
	}
	
	u8 *cur_packet_write_addr = NULL;
	u8 *cur_header_write_addr = NULL;
	int cur_write_index = 0;

	pthread_mutex_lock(&pbsb->_mutex);

	cur_packet_write_addr = pbsb->_buf_start_addr + pbsb->_buf_next_write_offset;
	cur_write_index = pbsb->_shm_index_for_next_write;
	
	printf("++++cur_packet_write_addr[%p]\n",cur_packet_write_addr);
	if(cur_write_index == pbsb->_shm_index_for_being_used){
		++cur_write_index;
		if(cur_write_index > pbsb->_shm_index_last){
			cur_write_index = pbsb->_shm_index_first;
		}
	}
	printf("++++cur_write_index[%d]\n",cur_write_index);
	
	if((pbsb->_buf_next_write_offset + packet_size) > pbsb->_buf_total_size){
		cur_packet_write_addr = pbsb->_buf_start_addr;
	}
	
	rev = packet_size;
	printf("++++packet_size[%d]\n",packet_size);
	//copy packet to pbsb
	memcpy(cur_packet_write_addr, packet_addr, packet_size);

	cur_header_write_addr = pbsb->_header_addr + cur_write_index * pbsb->_header_size;
	//copy header to pbsb
	memcpy(cur_header_write_addr, header, pbsb->_header_size);
	
	if(pbsb->_packet_addr[cur_write_index]._packet_state == SHPKT_INVALID){
		++pbsb->_cur_packet_num;
	}

	pbsb->_packet_addr[cur_write_index]._packet_offset = cur_packet_write_addr - pbsb->_buf_start_addr;
	pbsb->_packet_addr[cur_write_index]._packet_size  = packet_size;
	pbsb->_packet_addr[cur_write_index]._packet_state = SHPKET_VALID;

	pbsb->_shm_index_for_next_write = cur_write_index + 1;
	if(pbsb->_shm_index_for_next_write > pbsb->_shm_index_last){
		pbsb->_shm_index_for_next_write = pbsb->_shm_index_first;
	}

	pbsb->_buf_next_write_offset = cur_packet_write_addr + packet_size - pbsb->_buf_start_addr;
	if(pbsb->_buf_next_write_offset >= pbsb->_buf_total_size){
		pbsb->_buf_next_write_offset -= pbsb->_buf_total_size;
	}

	if(pbsb->_cur_packet_num > 0){
		pthread_cond_signal(&pbsb->_cond);
	}
	printf("===================pthread_cond_signal[%p]\n",&pbsb->_cond);
	pthread_mutex_unlock(&pbsb->_mutex);

	return rev;
}

s32 tbsb_read_one_packet(shm_t *pbsb, u8 *header, u8 *packet_addr, s32 packet_size)
{
	s32 rev = -1;
	if(NULL == pbsb || NULL == header || NULL == packet_addr || 
		packet_size <= 0 || packet_size > DEF_MAX_SHARE_BUFSIZE){
		return rev;
	}

	u8 *cur_header_read_addr = NULL;
	int cur_read_index = 0;

	pthread_mutex_lock(&pbsb->_mutex);

	printf("----_shm_index_for_being_used[%d]\n",pbsb->_shm_index_for_being_used);
	if(pbsb->_shm_index_for_being_used != -1){
		pbsb->_packet_addr[pbsb->_shm_index_for_being_used]._packet_state = SHPKT_INVALID;
		pbsb->_shm_index_for_being_used = -1;
		--pbsb->_cur_packet_num;
	}
	
	printf("----_cur_packet_num[%d]\n",pbsb->_cur_packet_num);
	while(pbsb->_cur_packet_num == 0){
		pthread_cleanup_push(cancel_read_wait, (void *)&pbsb->_mutex);
		pthread_cond_wait(&pbsb->_cond, &pbsb->_mutex);
		pthread_cleanup_pop(0);
	}
	printf("----cur_read_index[%d]\n",cur_read_index);
	cur_read_index = pbsb->_shm_index_for_next_read;

	while(pbsb->_packet_addr[cur_read_index]._packet_state == SHPKT_INVALID){
		++cur_read_index;
		if(cur_read_index > pbsb->_shm_index_last){
			cur_read_index = pbsb->_shm_index_first;
		}
	}
	
	if(packet_size >= pbsb->_packet_addr[cur_read_index]._packet_size){
		rev = pbsb->_packet_addr[cur_read_index]._packet_size;
		memcpy(packet_addr, 
				pbsb->_packet_addr[cur_read_index]._packet_offset + pbsb->_buf_start_addr, 
				pbsb->_packet_addr[cur_read_index]._packet_size);
	}else{
		printf("vbsb_read_one_packet packet size[%d] is larger than buf size %d\n", 
			pbsb->_packet_addr[cur_read_index]._packet_size,packet_size);
	}

	cur_header_read_addr = pbsb->_header_addr + cur_read_index *(pbsb->_header_size);
	memcpy(header, cur_header_read_addr, pbsb->_header_size);

	pbsb->_shm_index_for_next_read = cur_read_index + 1;
	if(pbsb->_shm_index_for_next_read > pbsb->_shm_index_last){
		pbsb->_shm_index_for_next_read = pbsb->_shm_index_first;
	}

	pbsb->_shm_index_for_being_used = cur_read_index;
	printf("----_shm_index_for_being_used[%d]\n",pbsb->_shm_index_for_being_used);
	pthread_mutex_unlock(&pbsb->_mutex);

	return rev;
}
