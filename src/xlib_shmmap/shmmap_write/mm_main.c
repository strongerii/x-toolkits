#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <signal.h>

#include "shmmap.h"
#include "sh_tbitstream.h"


#define DEF_SHMMAP_KEY			(0x1001)

int storage = 100;

shm_t *g_shm_bsb = NULL;

int g_flag_shm_read_exit = 0;
int g_flag_shm_write_exit = 0;
int g_flag_shm_exit = 0;

int g_force_reader_thread_exit = 0;

pthread_t g_handle_shm_write = 0;
pthread_t g_handle_shm_read = 0;

int shm_write_thread()
{
	int ret = -1;
	t_header_info header;
	t_packet_info packet;
	u8 common[1024] = {0};
	time_t curr;
	int size = 0;
	
	while(!g_force_reader_thread_exit)
	{
		memset(&packet, 0, sizeof(t_packet_info));
		memset(&header, 0, sizeof(t_header_info));
		
		printf("++++Please Enter string for writing share map:\n");
		int len = scanf("%s",common);
		printf("scanf size is %d\n",len);

		header.id = storage++;
		time(&curr);
		header.timestamp = (u32)curr;
		size = strlen((char *)common) + 1;
		ret = tbsb_write_one_packet(g_shm_bsb,
									(u8 *)&header,
									(u8 *)common, 
									size);
		if(ret < 0){
			printf("tbsb_write_one_packet failed\n");
			usleep(10000);
			continue;
		}

	}
	return 0;
}
int shm_read_thread()
{
	int ret = -1;
	t_header_info header;
//	t_packet_info packet;
	u8 common[1024] = {0};

	while(!g_force_reader_thread_exit)
	{
		ret = tbsb_read_one_packet(g_shm_bsb,
									(u8 *)&header,
									(u8 *)common, 
									DEF_MAX_SHARE_BUFSIZE);
		if(ret < 0){
			printf("tbsb_read_one_packet failed\n");
			usleep(10000);
			continue;
		}

		printf("----packet: common   [%s]\n",common);
		printf("----header: id       [%d]\n",header.id);
		printf("----header: timestamp[%d]\n",header.timestamp);
	}

	return 0;
}

int start_writer()
{
	if(0 == g_flag_shm_write_exit){
		if(pthread_create(&g_handle_shm_write,NULL,(void *)shm_write_thread,NULL) != 0){
			perror("shm_write_thread error!\n");
			return -1;
		}
		printf("start_writer thread ok!\n");
	}
	return 0;
}
int start_reader()
{
	if(0 == g_flag_shm_read_exit){
		if(pthread_create(&g_handle_shm_read,NULL,(void *)shm_read_thread,NULL) != 0){
			perror("shm_read_thread error!\n");
			return -1;
		}
		printf("start_reader thread ok!\n");
	}
	return 0;
}
void sig_handle (int sig_num, siginfo_t *info, void *context)
{
	printf("=============signal[%d]==============\n",sig_num);
	if(g_shm_bsb){
		release_shm_t(g_shm_bsb);
		g_shm_bsb = NULL;
	}

	g_flag_shm_exit = 1;
	return;
}
void init_signal()
{
	 struct sigaction sa;
	 
	 sa.sa_flags = SA_SIGINFO;
	 sa.sa_sigaction = sig_handle;
	 sigaction(SIGINT, &sa, NULL);

	 sa.sa_flags = SA_SIGINFO;
	 sa.sa_sigaction = sig_handle;
	 sigaction(SIGQUIT, &sa, NULL);

	 sa.sa_flags = SA_SIGINFO;
	 sa.sa_sigaction = sig_handle;
	 sigaction(SIGTERM, &sa, NULL);

	 sa.sa_flags = 0;
	 sa.sa_handler = SIG_IGN;
	 sigaction(SIGPIPE, &sa, NULL);
	 
	 return;
}
int printf_info()
{
	printf("g_shm_bsb._buf_total_size[%d]\n",g_shm_bsb->_buf_total_size);
	printf("g_shm_bsb._header_size[%d]\n",g_shm_bsb->_header_size);
	printf("g_shm_bsb._max_packet_num[%d]\n",g_shm_bsb->_max_packet_num);

	printf("g_shm_bsb._cur_packet_num[%d]\n",g_shm_bsb->_cur_packet_num);
	return 0;
}
int main(int argc, char *argv[])
{
	init_signal();
	
	g_shm_bsb = capture_shm_t(DEF_SHMMAP_KEY);
	if(NULL == g_shm_bsb){
		printf("creat_shm_t failed \n");
		return -1;
	}
	printf_info();
	if(start_writer() < 0){
		 printf("start_writer failed!\n");
		 return -1;
	}
/*	if(start_reader() < 0){
		 printf("start_reader failed!\n");
		 return -1;
	}*/
	
	while(!g_flag_shm_exit){
		sleep(100);
	}
	
	return 0;
}
