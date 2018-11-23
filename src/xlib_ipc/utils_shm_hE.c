#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include "utils_shm_hE.h"
#include "comm_debug.h"


s32 shm_mutil_init(multi_shm_t *_pbsb)
{
    if (NULL == _pbsb) {
        return -1;
    }

    _pbsb->_buf_packet_offset= sizeof(multi_shm_t);
    _pbsb->_buf_start_offset = sizeof(multi_shm_t) + sizeof(t_multi_packet_info)*_pbsb->_buf_packet_num;

    _pbsb->_cur_write_index   = 0;
    _pbsb->_cur_write_offset  = 0;
    _pbsb->_cur_used_buf_size = 0;

    t_multi_packet_info *_packet_addr = (t_multi_packet_info *)((u8 *)_pbsb + _pbsb->_buf_packet_offset);
    u8 *_buf_start_addr = (u8 *)_pbsb + _pbsb->_buf_start_offset;

    for (int i = 0; i < _pbsb->_buf_packet_num; i ++) {
         //init core
        if (_pbsb->_is_cross_process) {
            pthread_mutexattr_init(&_packet_addr[i]._mutex_attr);
            pthread_mutexattr_setpshared(&_packet_addr[i]._mutex_attr, PTHREAD_PROCESS_SHARED);

            //pthread_condattr_init(&_packet_addr[i]._cond_attr);
            //pthread_condattr_setpshared(&_packet_addr[i]._cond_attr, PTHREAD_PROCESS_SHARED);
        }

        pthread_mutex_init(&_packet_addr[i]._mutex, &_packet_addr[i]._mutex_attr);
        //pthread_cond_init(&_packet_addr[i]._cond, &_packet_addr[i]._cond_attr);
    }

    memset(_buf_start_addr, 0, _pbsb->_buf_total_size);
    memset(_packet_addr, 0, sizeof(t_multi_packet_info) * _pbsb->_buf_packet_num);

    return 0;
}

void shm_mutil_uninit(multi_shm_t *_pbsb)
{
    if (NULL == _pbsb) {
        return;
    }
    t_multi_packet_info *_packet_addr = (t_multi_packet_info *)((u8 *)_pbsb + _pbsb->_buf_packet_offset);

    for (int i = 0; i < _pbsb->_buf_packet_num; i ++) {
        if (_pbsb->_is_cross_process) {
            //pthread_condattr_destroy(&_packet_addr[i]._cond_attr);
            pthread_mutexattr_destroy(&_packet_addr[i]._mutex_attr);
        }
        //pthread_cond_destroy(&_packet_addr[i]._cond);
        pthread_mutex_destroy(&_packet_addr[i]._mutex);
    }

    return;
}

s32 shm_mutil_write_one_packet(multi_shm_t *pbsb, u8 *packet_addr, s32 packet_size)
{
    if (NULL == pbsb || NULL == packet_addr || packet_size <= 0) {
        return ERROR_NORMAL;
    }
    int r = 0;
    t_multi_packet_info *_packet_addr = (t_multi_packet_info *)((u8 *)pbsb + pbsb->_buf_packet_offset);
    u8 *_buf_start_addr = (u8 *)pbsb + pbsb->_buf_start_offset;
    u32 _cur_write_index = pbsb->_cur_write_index % pbsb->_buf_packet_num;

    printf("lock1[%d->%p]\n", _cur_write_index, &_packet_addr[_cur_write_index]._mutex);
    r = pthread_mutex_lock(&_packet_addr[_cur_write_index]._mutex);
    printf("lock2[%d->%p] r= %d\n", _cur_write_index, &_packet_addr[_cur_write_index]._mutex, r);
    if (r != 0) {
        exit(1);
    }
    //printf("_cur_write_index = %d\n", _cur_write_index);
    if (_packet_addr[_cur_write_index]._packet_size > 0) {
        pbsb->_cur_used_buf_size -= _packet_addr[_cur_write_index]._packet_size;
        _packet_addr[_cur_write_index]._packet_size = 0;
    }

    if (pbsb->_cur_used_buf_size + packet_size > pbsb->_buf_total_size) {
        printf("[%d]: Buffer is not enough.[%d / %d, need %d bytes]\n",
            _cur_write_index, pbsb->_cur_used_buf_size, pbsb->_buf_total_size, packet_size);

        pthread_mutex_unlock(&_packet_addr[_cur_write_index]._mutex);
        return ERROR_TOTAL_BUFFER_NOT_ENOUGH;
    }

    if (pbsb->_cur_write_offset + packet_size > pbsb->_buf_total_size) {
        u32 _cur_copy_size = pbsb->_buf_total_size - pbsb->_cur_write_offset;

        memcpy(&_buf_start_addr[pbsb->_cur_write_offset], packet_addr, _cur_copy_size);
        pbsb->_cur_used_buf_size += _cur_copy_size;

        _packet_addr[_cur_write_index]._packet_idx    = pbsb->_cur_write_index;
        _packet_addr[_cur_write_index]._packet_offset = pbsb->_cur_write_offset;
        _packet_addr[_cur_write_index]._packet_size   = packet_size;

        printf("---> [%d/%08d]: write tail not enough. 1.write to 0x%04x %04d bytes ",
            _cur_write_index,
            _packet_addr[_cur_write_index]._packet_idx,
            pbsb->_cur_write_offset,
            _cur_copy_size);

        pbsb->_cur_write_offset = 0;
        u32 _left_write_size = packet_size - _cur_copy_size;

        memcpy(&_buf_start_addr[pbsb->_cur_write_offset], packet_addr + _cur_copy_size, _left_write_size);
        pbsb->_cur_used_buf_size += _left_write_size;
        pbsb->_cur_write_offset = _left_write_size;

        printf(" write head. 2.write to 0x%04x %04d bytes. used %04d / %05d btyes. offset 0x%04x.\n",
            pbsb->_cur_write_offset, _left_write_size, pbsb->_cur_used_buf_size, pbsb->_buf_total_size, pbsb->_cur_write_offset);

    } else {
        memcpy(&_buf_start_addr[pbsb->_cur_write_offset], packet_addr, packet_size);

        _packet_addr[_cur_write_index]._packet_idx    = pbsb->_cur_write_index;
        _packet_addr[_cur_write_index]._packet_offset = pbsb->_cur_write_offset;
        _packet_addr[_cur_write_index]._packet_size   = packet_size;

        printf("---> [%d/%08d]: write to 0x%04x %04d bytes.",
            _cur_write_index,
            _packet_addr[_cur_write_index]._packet_idx,
            pbsb->_cur_write_offset,
            packet_size);

        pbsb->_cur_write_offset  += packet_size;
        pbsb->_cur_used_buf_size += packet_size;

        if (pbsb->_cur_write_offset >= pbsb->_buf_total_size) {
            pbsb->_cur_write_offset = 0;
        }

        printf(" used %04d / %05d bytes. offset 0x%04x\n",
            pbsb->_cur_used_buf_size, pbsb->_buf_total_size, pbsb->_cur_write_offset);
    }

    //printf("[%d/%d]: [0x%05x --> %04d bytes]\n", _cur_write_index, _packet_addr[_cur_write_index]._packet_idx,
    //        _packet_addr[_cur_write_index]._packet_offset, _packet_addr[_cur_write_index]._packet_size);

    r = pthread_mutex_unlock(&_packet_addr[_cur_write_index]._mutex);
    printf("-- unlock[%d->%p] r= %d\n", _cur_write_index, &_packet_addr[_cur_write_index]._mutex, r);
    if (r != 0) {
        exit(1);
    }
    //comm_dump_hex((char *)_packet_addr, sizeof(t_multi_packet_info)*pbsb->_buf_packet_num);
    pbsb->_cur_write_index++;

    return 0;
}
s32 shm_multi_read_sync(multi_shm_t *pbsb, u32 *pcur_read_index, u32 *pcur_packet_index)
{
    s32 result = 0;
    if (NULL == pbsb || NULL == pcur_read_index || NULL == pcur_packet_index) {
        return ERROR_NORMAL;
    }
    u32 read_index = 0;
    u32 packet_index = 0;
    t_multi_packet_info *_packet_addr = (t_multi_packet_info *)((u8 *)pbsb + pbsb->_buf_packet_offset);

    // get packet index base.
    pthread_mutex_lock(&_packet_addr[0]._mutex);
    if (_packet_addr[0]._packet_size) {
        packet_index = _packet_addr[0]._packet_idx;
    } else {
        pthread_mutex_unlock(&_packet_addr[0]._mutex);
        return ERROR_NO_SYNC;
    }
    pthread_mutex_unlock(&_packet_addr[0]._mutex);

    for (int i = 1; i < pbsb->_buf_packet_num; i++) {
        pthread_mutex_lock(&_packet_addr[i]._mutex);
        if (_packet_addr[i]._packet_size) {
            if (_packet_addr[i]._packet_idx != packet_index + 1) {
                pthread_mutex_unlock(&_packet_addr[i]._mutex);
                break;
            } else {
                read_index++;
                packet_index++;
            }
        } else {
            pthread_mutex_unlock(&_packet_addr[i]._mutex);
            break;
        }
        pthread_mutex_unlock(&_packet_addr[i]._mutex);
    }
    *pcur_read_index = read_index;
    *pcur_packet_index = packet_index;

    return result;
}
s32 shm_multi_get_current_idx(multi_shm_t *pbsb, u32 *pcur_read_index, u32 *pcur_packet_index)
{
    if (NULL == pbsb || NULL == pcur_read_index || NULL == pcur_packet_index) {
        return ERROR_NORMAL;
    }
    t_multi_packet_info *_packet_addr = (t_multi_packet_info *)((u8 *)pbsb + pbsb->_buf_packet_offset);

    *pcur_read_index = pbsb->_cur_write_index;
    *pcur_packet_index = _packet_addr[pbsb->_cur_write_index]._packet_idx;

    return 0;
}
s32 shm_mutil_read_one_packet(multi_shm_t *pbsb, u8 *buffer_addr, s32 packet_size, u32 *pcur_read_index, u32 *pcur_packet_index)
{
    int r = 0;
    s32 result = 0;
    if (NULL == pbsb || NULL == buffer_addr || packet_size <= 0 ) {
        return ERROR_NORMAL;
    }
    t_multi_packet_info *_packet_addr = (t_multi_packet_info *)((u8 *)pbsb + pbsb->_buf_packet_offset);
    u8 *_buf_start_addr   = (u8 *)pbsb + pbsb->_buf_start_offset;
    u32 _cur_read_index   = *pcur_read_index % pbsb->_buf_packet_num;
    u32 _cur_packet_index = *pcur_packet_index;

    printf("_cur_read_index = %d _cur_packet_index = %d _packet_addr[_cur_read_index]._packet_idx = %d\n",
        _cur_read_index, _cur_packet_index, _packet_addr[_cur_read_index]._packet_idx);

    printf("lock1[%d->%p]\n", _cur_read_index, &_packet_addr[_cur_read_index]._mutex);
    r = pthread_mutex_trylock(&_packet_addr[_cur_read_index]._mutex);
    printf("lock2[%d->%p] r= %d\n", _cur_read_index, &_packet_addr[_cur_read_index]._mutex, r);
    if (r != 0) {
        return -1;
    }
    if (_packet_addr[_cur_read_index]._packet_size > 0) {
        if (packet_size >= _packet_addr[_cur_read_index]._packet_size) {
            if (_cur_packet_index == _packet_addr[_cur_read_index]._packet_idx) {
                // Notes This !!
                *pcur_read_index = _cur_read_index + 1;
                *pcur_packet_index = _cur_packet_index + 1;

                if (_packet_addr[_cur_read_index]._packet_offset +
                    _packet_addr[_cur_read_index]._packet_size > pbsb->_buf_total_size) {

                    printf("---> read tail not enough and read head.[0x%x - ", _packet_addr[_cur_read_index]._packet_offset);

                    u32 _cur_copy_size = pbsb->_buf_total_size - _packet_addr[_cur_read_index]._packet_offset;
                    memcpy(buffer_addr, &_buf_start_addr[_packet_addr[_cur_read_index]._packet_offset], _cur_copy_size);

                    u32 _left_copy_size = _packet_addr[_cur_read_index]._packet_size - _cur_copy_size;
                    memcpy(buffer_addr + _cur_copy_size, &_buf_start_addr[0], _left_copy_size);

                    printf("0x%x].\n", _left_copy_size);

                } else {
                    memcpy(buffer_addr, &_buf_start_addr[_packet_addr[_cur_read_index]._packet_offset],
                        _packet_addr[_cur_read_index]._packet_size);
                }
                printf("[%d/%d]: [0x%05x --> %04d bytes]\n", _cur_read_index, _packet_addr[_cur_read_index]._packet_idx,
                    _packet_addr[_cur_read_index]._packet_offset, _packet_addr[_cur_read_index]._packet_size);

            } else {
                printf("pcur_read_index = %d pcur_packet_index = %d not found.\n\n", *pcur_read_index, *pcur_packet_index);
                result = ERROR_NO_FOUND_PACKET;
            }
        } else {
            printf("tbsb_read_one_packet packet size[%d] is larger than buf size %d\n",
                _packet_addr[_cur_read_index]._packet_size, packet_size);
            result = ERROR_READ_BUFFER_NOT_ENOUGH;
        }
    }

    r = pthread_mutex_unlock(&_packet_addr[_cur_read_index]._mutex);
    printf("-- unlock[%d->%p] r= %d\n", _cur_read_index, &_packet_addr[_cur_read_index]._mutex, r);


    return result;
}
