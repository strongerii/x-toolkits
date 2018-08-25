#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <signal.h>

#include "utils_fifo.h"

#define DEF_SHMMAP_KEY			(0x1001)

int storage = 100;

multi_shm_t *g_shm_bsb = NULL;

int g_flag_shm_read_exit = 0;
int g_flag_shm_write_exit = 0;
int g_flag_shm_exit = 0;

int g_force_reader_thread_exit = 0;

pthread_t g_handle_shm_write = 0;
pthread_t g_handle_shm_read = 0;

void* shm_write_thread(void *arg)
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

        ret = fifo_multi_send(g_shm_bsb,  (u8 *)common, size);
        if(ret < 0){
            printf("fifo_send failed\n");
            usleep(10000);
            continue;
        }
    }
    return NULL;
}

void * shm_read_thread(void *arg)
{
    int ret = -1;
    t_header_info header;
    u8 common[1024] = {0};

    while(!g_force_reader_thread_exit)
    {
        memset(common, 0, 1024);

        ret = fifo_multi_recv(g_shm_bsb, common, 1024);
        if(ret < 0){
            printf("fifo_recv failed\n");
            usleep(1000000);
            continue;
        }
        usleep(100000);

        printf("\t\t\t----packet: common   [%s]\n",common);
        //printf("----header: id       [%d]\n",header.id);
        //printf("----header: timestamp[%d]\n",header.timestamp);
    }

    return NULL;
}

int start_writer()
{
    if(0 == g_flag_shm_write_exit){
        if(pthread_create(&g_handle_shm_write, NULL, (void *)shm_write_thread, NULL) != 0)
        {
            printf("shm_write_thread error!\n");
            return -1;
        }
        printf("start_writer thread ok!\n");
    }
    return 0;
}
int start_reader()
{
    if(0 == g_flag_shm_read_exit){
        if(pthread_create(&g_handle_shm_read,NULL,(void *)shm_read_thread, NULL) != 0){
            printf("shm_read_thread error!\n");
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
        fifo_release_multi_shm(g_shm_bsb);
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

int main(int argc, char *argv[])
{
    init_signal();

    g_shm_bsb = fifo_capture_multi_shm(DEF_SHMMAP_KEY);
    if(NULL == g_shm_bsb){
        printf("fifo_capture_shm failed \n");
        return -1;
    }
#if 0
    if(start_writer() < 0){
         printf("start_writer failed!\n");
         return -1;
    }
#else
    if(start_reader() < 0){
         printf("start_reader failed!\n");
         return -1;
    }
#endif
    while(!g_flag_shm_exit){
        sleep(100);
    }

    return 0;
}
