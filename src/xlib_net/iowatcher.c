#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <fcntl.h>
#include <unistd.h>
#include <getopt.h>
#include <sched.h>
#include <time.h>
#include <sys/signal.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <pthread.h>
#include "ipc_net.h"
//#include "ipc_proc.h"
#include "iowatcher.h"

int set_realtime_schedule(pthread_t thread_id)
{
	struct sched_param param;
	int policy = SCHED_RR;
	int priority = 90;
	if (!thread_id)
		return -1;
	memset(&param, 0, sizeof(param));
	param.sched_priority = priority;
	if (pthread_setschedparam(thread_id, policy, &param) < 0)
		perror("pthread_setschedparam");
	pthread_getschedparam(thread_id, &policy, &param);
	if (param.sched_priority != priority)
		return -1;
	return 0;
}

static void * io_thread_func(void * arg)
{
	t_io_ops *p = (t_io_ops *)arg;
	t_io_thread_info *pt = NULL;
	if(NULL == p){
		printf("io_thread_func NULL==p\n");
		return NULL;
	}
	if(NULL == p->_pfntrancb){
		printf("io_thread_func NULL==p->_pfncb\n");
		return NULL;
	}
	pt = &p->_threadinfo;
	printf("io_thread_func start event=%d!\n", p->_event);
	while (!pt->_exit) {
		p->_pfntrancb(p->_event, p->_puserdata);
	}
	if(p->_pfnexitcb){
		p->_pfnexitcb(p->_event, p->_puserdata);
	}
	printf("io_thread_func exit!\n");
	return NULL;
}


/* create a main thread reading data and four threads for four buffer */
static s32 create_io_thread(t_io_ops *p)
{
	int ret = 0;
	t_io_thread_info *pt = NULL;
	pthread_attr_t attr;
	pthread_attr_init (&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	
	if(NULL == p){
		printf("NULL == p\n");
		return -1;
	}

	if( NULL == p->_pfntrancb){
		printf("NULL == p->_pfncb\n");
		return 0;
	}

	pt = &p->_threadinfo;
	pt->_exit = 0;
	pt->_thd = -1;
	ret = pthread_create(&pt->_thd, &attr, io_thread_func, p);
	if (ret != 0) {
		perror("create_absreader_thread failed");
		return -1;
	}
	if (set_realtime_schedule(pt->_thd) < 0) {
		printf("set realtime schedule error \n");
		return -1;
	}
	printf("create_io_thread thread id=0x%x, type=%d\n", (unsigned int)pt->_thd, p->_event);
	return 0;
}

static s32 cancel_io_thread (t_io_ops *p)
{
	t_io_thread_info *pt = NULL;
	if(NULL == p){
		printf("NULL == p\n");
		return -1;
	}
	pt = &p->_threadinfo;
	pt->_exit = 1;
	printf("cancel_io_thread thread id=0x%x, type=%d\n", (unsigned int)pt->_thd, p->_event);
	return 0;
}

int start_iowatcher(t_iowatcher_info *p)
{
	if(NULL == p){
		return -1;
	}
	if(p->_io_event&DEF_IOE_READ){
		if(create_io_thread(&p->_io_read) < 0){
			printf("create_io_thread read failed\n");
			return -1;
		}
		printf("create_io_thread read ok\n");
	}

	if(p->_io_event& DEF_IOE_WRITE){
		if(create_io_thread(&p->_io_write) < 0){
			printf("create_io_thread write failed\n");
			return -1;
		}
		printf("create_io_thread write ok\n");
	}

	return 0;
}

int stop_iowatcher(t_iowatcher_info *p)
{
	if(NULL == p){
		return -1;
	}
	if(p->_io_event& DEF_IOE_READ){
		cancel_io_thread(&p->_io_read);
		printf("cancel_io_thread read ok\n");
	}
	if(p->_io_event& DEF_IOE_WRITE){
		cancel_io_thread(&p->_io_write);
		printf("cancel_io_thread write ok\n");
	}

	
	close(p->_fd);
	p->_fd = -1;
	return 0;
}
////////////////////////////////////////////////////////////////////

