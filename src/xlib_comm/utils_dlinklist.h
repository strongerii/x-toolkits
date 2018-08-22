#ifndef _UTILS_DLINKLIST_H_
#define _UTILS_DLINKLIST_H_

typedef int (*__PFNTravelCB)(void *pkey, void *pdata, void *puserdata);

typedef struct _dlinklist_node {
    void *_pkey;
    void *_pdata;
    struct _dlinklist_node *_pre;
    struct _dlinklist_node *_next;
} t_dlinklist_node;

typedef struct tag_dlinklist {
    t_dlinklist_node *_phead;
    pthread_mutex_t _lock;
} t_dlinklist;


//////////////////////////////////////////////////////////////////////////////

#ifdef __cplusplus
extern "C"
{
#endif//__cplusplus


t_dlinklist_node *create_dlinklist_node(void *pkey, void *pdata);
void destroy_dlinklist_node(t_dlinklist_node *pnode);
int insert_dlinklist_node(t_dlinklist_node *phead, void *pkey,void *pdata);

t_dlinklist *create_dlinklist();
void destroy_dlinklist(t_dlinklist *plst);
int insert_dlinklist_tail(t_dlinklist *plst, void *pkey,void *pdata);
int insert_dlinklist_head(t_dlinklist *plst, void *pkey,void *pdata);
int delete_dlinklist_node(t_dlinklist *plst, void *pkey);

int travel_dlinklist(t_dlinklist *plst, __PFNTravelCB pfn, void *puserdata);
t_dlinklist_node *find_dlinklist_node(t_dlinklist *plst, void *pkey);

#ifdef __cplusplus
}
#endif//__cplusplus


//////////////////////////////////////////////////////////////////////////////
#endif //_UTILS_DLINKLIST_H_
