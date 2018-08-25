#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <fcntl.h>

#include <pthread.h>
#include "utils_sink.h"

static int g_sink_item_num = 0;
static t_sink_item g_sink_item_s[DEF_MAX_SINK_NUM];
static pthread_mutex_t g_sink_io_mutex;

int init_sink_items()
{
    int i = 0;
    pthread_mutex_init(&g_sink_io_mutex, NULL);
    pthread_mutex_lock(&g_sink_io_mutex);
    for(i = 0; i < DEF_MAX_SINK_NUM; ++i){
        g_sink_item_s[i].is_used = 0;
        g_sink_item_s[i].idx = i;
    }
    pthread_mutex_unlock(&g_sink_io_mutex);

    return 0;
}

int uninit_sink_items()
{
    pthread_mutex_destroy(&g_sink_io_mutex);
    return 0;
}

int get_cur_sink_num()
{
    int nret = 0;
    pthread_mutex_lock(&g_sink_io_mutex);
    nret = g_sink_item_num;
    pthread_mutex_unlock(&g_sink_io_mutex);
    return nret;
}

int get_valid_sink_id()
{
    int i = 0;
    int idx = -1;
    pthread_mutex_lock(&g_sink_io_mutex);

    for(i = 0; i < DEF_MAX_SINK_NUM; ++i){
        if(g_sink_item_s[i].is_used == 0){
            ++g_sink_item_num;
            g_sink_item_s[i].is_used = 1;
            idx = g_sink_item_s[i].idx;
            break;
        }
    }

    pthread_mutex_unlock(&g_sink_io_mutex);
    printf("++++ g_sink_item_num = %d\n", g_sink_item_num);
    return idx;
}

t_sink_item *get_valid_sink_item()
{
    int i = 0;
    t_sink_item *pItem = NULL;

    pthread_mutex_lock(&g_sink_io_mutex);

    for(i = 0; i < DEF_MAX_SINK_NUM; ++i){
        if(g_sink_item_s[i].is_used == 0){
            ++g_sink_item_num;
            g_sink_item_s[i].is_used = 1;
            pItem = &g_sink_item_s[i];
            break;
        }
    }

    pthread_mutex_unlock(&g_sink_io_mutex);
    printf("++++ g_sink_item_num = %d\n", g_sink_item_num);
    return pItem;
}

t_sink_item *get_sink_item_by_idx(int idx)
{
    int i = 0;
    t_sink_item *pItem = NULL;

    pthread_mutex_lock(&g_sink_io_mutex);

    for(i = 0; i < DEF_MAX_SINK_NUM; ++i){
        if(g_sink_item_s[i].is_used == 1){
            if (idx == g_sink_item_s[i].idx) {
                pItem = &g_sink_item_s[i];
                break;
            }
        }
    }
    pthread_mutex_unlock(&g_sink_io_mutex);
    return pItem;
}
void set_invalid_sink_id(int idx)
{
    if(idx < 0){
        return;
    }

    int i = 0;
    pthread_mutex_lock(&g_sink_io_mutex);
    for( i = 0; i < DEF_MAX_SINK_NUM; ++i){
        if(idx == g_sink_item_s[i].idx){
            --g_sink_item_num;
            g_sink_item_s[i].is_used = 0;
            break;
        }
    }
    pthread_mutex_unlock(&g_sink_io_mutex);
    printf("---- g_sink_item_num = %d\n", g_sink_item_num);
    return ;

}

