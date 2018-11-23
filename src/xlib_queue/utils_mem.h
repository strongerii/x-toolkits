#ifndef _DEF_UTILS_MEM_H_
#define _DEF_UTILS_MEM_H_

typedef enum {
    SHPKT_INVALID = 0,
    SHPKET_VALID
} e_packet_state;

typedef struct {
    unsigned int _packet_offset;
    unsigned int _packet_size;
    unsigned int _packet_state;
} t_packet_info;

typedef struct {
    unsigned int id;
    unsigned int timestamp;
} t_header_info;

//shm buffer
typedef struct
{
    unsigned int _is_cross_process;
    //buffer
    unsigned int _buf_start_offset;          //in
    unsigned int _buf_next_write_offset;
    unsigned int _buf_next_read_offset;
    unsigned int _buf_total_size;            //in
    unsigned int _buf_packet_num;            //in
    //unsigned int _buf_used_size;

    //header info
    unsigned int _header_offset;             //in
    unsigned int _header_size;               //in ??

    //packet info
    unsigned int _packet_offset;             //in
    unsigned int _max_packet_num;
    unsigned int _cur_packet_num;

    //shm info
    int _shm_index_first;
    int _shm_index_last;
    int _shm_index_for_next_read;
    int _shm_index_for_next_write;
    int _shm_index_for_being_used;

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

int tbsb_init(shm_t *pbsb);
void tbsb_uninit(shm_t *_pbsb);
int tbsb_write_one_packet(shm_t *pbsb, unsigned char *pheader, unsigned char *packet_addr, int packet_size);
int tbsb_read_one_packet(shm_t *pbsb, unsigned char *pheader, unsigned char *packet_addr, int packet_size);

#ifdef __cplusplus
}
#endif//__cplusplus

#endif //_DEF_UTILS_MEM_H_