#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include "utils_mem.h"
#include "comm_debug.h"

static void cancel_read_wait(void *arg)
{
    pthread_mutex_t *mutex = arg;
    pthread_mutex_unlock(mutex);

    printf("cancel_read_wait \n");
}

int tbsb_init(shm_t *_pbsb)
{
    if (NULL == _pbsb) {
        return -1;
    }

    //init buffer
    _pbsb->_buf_next_write_offset = 0;
    _pbsb->_buf_next_read_offset = 0;

    //init headers info
    //_pbsb->_header_size = sizeof(t_header_info);

    //init packs info
    _pbsb->_max_packet_num = _pbsb->_buf_packet_num;
    _pbsb->_cur_packet_num = 0;

    //init shm_info
    _pbsb->_shm_index_first = 0;
    _pbsb->_shm_index_last = _pbsb->_buf_packet_num - 1;
    _pbsb->_shm_index_for_next_read = 0;
    _pbsb->_shm_index_for_next_write = 0;
    _pbsb->_shm_index_for_being_used = -1;//no packet being used

    //init core
    if (_pbsb->_is_cross_process) {
        pthread_mutexattr_init(&_pbsb->_mutex_attr);
        pthread_mutexattr_setpshared(&_pbsb->_mutex_attr, PTHREAD_PROCESS_SHARED);

        pthread_condattr_init(&_pbsb->_cond_attr);
        pthread_condattr_setpshared(&_pbsb->_cond_attr, PTHREAD_PROCESS_SHARED);
    }

    pthread_mutex_init(&_pbsb->_mutex, &_pbsb->_mutex_attr);
    pthread_cond_init(&_pbsb->_cond, &_pbsb->_cond_attr);

    //reset buffers
    unsigned char *_buf_start_addr = (unsigned char *)_pbsb + _pbsb->_buf_start_offset;
    unsigned char *_header_addr = (unsigned char *)_pbsb + _pbsb->_header_offset;
    t_packet_info *_packet_addr = (t_packet_info *)((unsigned char *)_pbsb + _pbsb->_packet_offset);

    memset(_buf_start_addr, 0, _pbsb->_buf_total_size);
    memset(_header_addr, 0, _pbsb->_header_size * _pbsb->_buf_packet_num);
    memset(_packet_addr, 0, sizeof(t_packet_info) * _pbsb->_buf_packet_num);

    return 0;
}

void tbsb_uninit(shm_t *_pbsb)
{
    if (NULL == _pbsb) {
        return;
    }
    if (_pbsb->_is_cross_process) {
        pthread_condattr_destroy(&_pbsb->_cond_attr);
        pthread_mutexattr_destroy(&_pbsb->_mutex_attr);
    }
    pthread_cond_destroy(&_pbsb->_cond);
    pthread_mutex_destroy(&_pbsb->_mutex);

    return;
}

int tbsb_write_one_packet(shm_t *pbsb, unsigned char *header, unsigned char *packet_addr, int packet_size)
{
    int rev = -1;
    if (NULL == pbsb || NULL == packet_addr || packet_size <= 0) {
        return rev;
    }

    unsigned char *cur_packet_write_addr = NULL;
    unsigned char *cur_header_write_addr = NULL;

    unsigned char *_buf_start_addr = (unsigned char *)pbsb + pbsb->_buf_start_offset;
    unsigned char *_header_addr = (unsigned char *)pbsb + pbsb->_header_offset;
    t_packet_info *_packet_addr = (t_packet_info *)((unsigned char *)pbsb + pbsb->_packet_offset);

    int cur_write_index = 0;

    pthread_mutex_lock(&pbsb->_mutex);

    cur_packet_write_addr = _buf_start_addr + pbsb->_buf_next_write_offset;
    cur_write_index = pbsb->_shm_index_for_next_write;

    if (cur_write_index == pbsb->_shm_index_for_being_used) {
        ++cur_write_index;
        if(cur_write_index > pbsb->_shm_index_last){
            cur_write_index = pbsb->_shm_index_first;
        }
    }
    printf("++++cur_write_index[%d / %d]: write %d bytes\n", cur_write_index, pbsb->_cur_packet_num, packet_size);

    if ((pbsb->_buf_next_write_offset + packet_size) > pbsb->_buf_total_size) {
        cur_packet_write_addr = _buf_start_addr;
    }

    rev = packet_size;

    //copy packet to pbsb
    memcpy(cur_packet_write_addr, packet_addr, packet_size);

    // if need copy header
    if (header) {
        cur_header_write_addr = _header_addr + cur_write_index * pbsb->_header_size;
        //copy header to pbsb
        memcpy(cur_header_write_addr, header, pbsb->_header_size);
    }

    if (_packet_addr[cur_write_index]._packet_state == SHPKT_INVALID) {
        ++pbsb->_cur_packet_num;
    }

    _packet_addr[cur_write_index]._packet_offset = cur_packet_write_addr - _buf_start_addr;
    _packet_addr[cur_write_index]._packet_size  = packet_size;
    _packet_addr[cur_write_index]._packet_state = SHPKET_VALID;

    printf("++++[%d]_packet_offset[0x%x]\n", cur_write_index, _packet_addr[cur_write_index]._packet_offset);

    pbsb->_shm_index_for_next_write = cur_write_index + 1;
    if (pbsb->_shm_index_for_next_write > pbsb->_shm_index_last) {
        pbsb->_shm_index_for_next_write = pbsb->_shm_index_first;
    }

    pbsb->_buf_next_write_offset = cur_packet_write_addr + packet_size - _buf_start_addr;
    if (pbsb->_buf_next_write_offset >= pbsb->_buf_total_size) {
        pbsb->_buf_next_write_offset -= pbsb->_buf_total_size;
    }
    printf("_buf_next_write_offset = 0x%x\n", pbsb->_buf_next_write_offset);
    if (pbsb->_cur_packet_num > 0) {
        pthread_cond_signal(&pbsb->_cond);
    }

    pthread_mutex_unlock(&pbsb->_mutex);

    comm_dump_hex((char *)_packet_addr, sizeof(t_packet_info)*pbsb->_buf_packet_num);

    return rev;
}

int tbsb_read_one_packet(shm_t *pbsb, unsigned char *header, unsigned char *packet_addr, int packet_size)
{
    int rev = -1;
    if (NULL == pbsb || NULL == packet_addr || packet_size <= 0 ) {
        return rev;
    }

    unsigned char *cur_header_read_addr = NULL;
    int cur_read_index = 0;

    unsigned char *_buf_start_addr = (unsigned char *)pbsb + pbsb->_buf_start_offset;
    unsigned char *_header_addr = (unsigned char *)pbsb + pbsb->_header_offset;
    t_packet_info *_packet_addr = (t_packet_info *)((unsigned char *)pbsb + pbsb->_packet_offset);

    comm_dump_hex((char *)_packet_addr, sizeof(t_packet_info)*pbsb->_buf_packet_num);

    pthread_mutex_lock(&pbsb->_mutex);

    if (pbsb->_shm_index_for_being_used != -1) {
        _packet_addr[pbsb->_shm_index_for_being_used]._packet_state = SHPKT_INVALID;
        pbsb->_shm_index_for_being_used = -1;
        --pbsb->_cur_packet_num;
    }

    while (pbsb->_cur_packet_num == 0) {
        pthread_cleanup_push(cancel_read_wait, (void *)&pbsb->_mutex);
        pthread_cond_wait(&pbsb->_cond, &pbsb->_mutex);
        pthread_cleanup_pop(0);
    }
    printf("----cur_read_index[%d / %d]\n", cur_read_index, pbsb->_cur_packet_num);
    cur_read_index = pbsb->_shm_index_for_next_read;

    while (_packet_addr[cur_read_index]._packet_state == SHPKT_INVALID) {
        ++cur_read_index;
        if(cur_read_index > pbsb->_shm_index_last){
            cur_read_index = pbsb->_shm_index_first;
        }
    }

    if (packet_size >= _packet_addr[cur_read_index]._packet_size) {
        printf("----[%d]_packet_offset[0x%x]\n", cur_read_index, _packet_addr[cur_read_index]._packet_offset);
        rev = _packet_addr[cur_read_index]._packet_size;
        memcpy(packet_addr,
                _packet_addr[cur_read_index]._packet_offset + _buf_start_addr,
                _packet_addr[cur_read_index]._packet_size);
    } else {
        printf("vbsb_read_one_packet packet size[%d] is larger than buf size %d\n",
            _packet_addr[cur_read_index]._packet_size,packet_size);
    }

    // if need copy header
    if (header) {
        cur_header_read_addr = _header_addr + cur_read_index * pbsb->_header_size;
        memcpy(header, cur_header_read_addr, pbsb->_header_size);
    }

    pbsb->_shm_index_for_next_read = cur_read_index + 1;
    if (pbsb->_shm_index_for_next_read > pbsb->_shm_index_last) {
        pbsb->_shm_index_for_next_read = pbsb->_shm_index_first;
    }

    pbsb->_shm_index_for_being_used = cur_read_index;
    printf("----_shm_index_for_being_used[%d]\n",pbsb->_shm_index_for_being_used);
    pthread_mutex_unlock(&pbsb->_mutex);

    return rev;
}
