#ifndef _IO_WATCHER_H_
#define _IO_WATCHER_H_


#include "ipc_net.h"


#define DEF_IOE_WRITE					(1<<0)
#define DEF_IOE_READ					(1<<1)
#define DEF_IOE_READWRITE				(DEF_IOE_WRITE|DEF_IOE_READ)

typedef void (*_PFNIOWATCHERCB)(int ev,void *puserdata);

typedef struct{
	pthread_t 			_thd;
	int 					_exit;
}t_io_thread_info;

typedef struct{
	_PFNIOWATCHERCB 	_pfntrancb;
	_PFNIOWATCHERCB 	_pfnexitcb;
	void*				_puserdata;
	t_io_thread_info		_threadinfo;
	int 					_event;
}t_io_ops;

typedef struct{
	int 			_fd;
	int 			_io_event;
	t_io_ops 		_io_read;
	t_io_ops 		_io_write;
}t_iowatcher_info;

typedef struct{
	t_iowatcher_info info;
	int (*_start_iowatch)(t_iowatcher_info *p);
	int (*_stop_iowatch)(t_iowatcher_info *p);
}t_iowatcher;

#ifdef __cplusplus
extern "C"
{
#endif//__cplusplus

int start_iowatcher(t_iowatcher_info *p);
int stop_iowatcher(t_iowatcher_info *p);

#ifdef __cplusplus
}
#endif//__cplusplus


#endif //_IO_WATCHER_H_