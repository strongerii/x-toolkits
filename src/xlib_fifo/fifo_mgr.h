#ifndef _IPC_FIFO_MGR_HEADER_
#define _IPC_FIFO_MGR_HEADER_

#include "xlib_type.h"

#ifdef __cplusplus
extern "C"
{
#endif//__cplusplus

#define DEF_FIXED_UNIT_SIZE             (720*400*3/2)
#define DEF_MAX_INDEX_NUM               (50)
#define MAGIC_NUM                       (0xFFEF00F0)
#define DEF_RING_BUF_SIZE               (DEF_FIXED_UNIT_SIZE*DEF_MAX_INDEX_NUM)


typedef struct {
    
	u8	*pkg_addr; /* the actual address in stream buffer */
	u32	pkg_size;	 /* the actual output frame size , including CAVLC */
    
}pkg_info_t;

//int dispatch_and_copy_one_frame(u8 *pBuffer, u32 size);
int dispatch_and_copy_one_frame(u8 *pBuffer, u32 buffer_size, u8* pHeader);

//int bsreader_get_one_pkg(pkg_info_t * info);
int bsreader_get_one_pkg(pkg_info_t * info, u8 *header_info);

void fifo_mgr_flush(void);
int fifo_mgr_init(int buffer_size, int header_size, int buffer_num);
void fifo_mgr_uninit();


#ifdef __cplusplus
}
#endif//__cplusplus


#endif //_IPC_FIFO_MGR_HEADER_
