#ifndef _SINK_MANGER_H_
#define _SINK_MANGER_H_

#define DEF_MAX_SINK_NUM        (1024)

typedef struct _SINK_ITEM {
    int is_used;
    int idx;
} t_sink_item;

//////////////////////////////////////////////////////////////////////////////
#ifdef __cplusplus
extern "C"
{
#endif//__cplusplus

int init_sink_items();
int uninit_sink_items();
int get_cur_sink_num();
int get_valid_sink_id();
t_sink_item *get_valid_sink_item();
t_sink_item *get_sink_item_by_idx(int idx);
void set_invalid_sink_id(int idx);

#ifdef __cplusplus
}
#endif//__cplusplus
//////////////////////////////////////////////////////////////////////////////

#endif //_SINK_MANGER_H_