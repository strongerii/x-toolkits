#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>

#include "utils_dlinklist.h"

#define _DISABLE_LOCK

static int __initlock(t_dlinklist *plst)
{
#ifndef _DISABLE_LOCK
    if (NULL == plst) {
        return -1;
    }
    if (pthread_mutex_init(&(plst->_lock), NULL)) {
        return -1;
    }
#endif
    return 0;
}

static int __uninitlock(t_dlinklist *plst)
{
#ifndef _DISABLE_LOCK
    if (NULL == plst) {
        return -1;
    }
    if (pthread_mutex_destroy(&(plst->_lock))) {
        return -1;
    }
#endif
    return 0;
}

static int __lock(t_dlinklist *plst)
{
#ifndef _DISABLE_LOCK
    if (NULL == plst) {
        return -1;
    }
    if (pthread_mutex_lock(&(plst->_lock))) {
        printf("pthread_mutex_lock failed\n");
        return -1;
    }
#endif
    return 0;
}

static int __unlock(t_dlinklist *plst)
{
#ifndef _DISABLE_LOCK
    if (NULL == plst) {
        return -1;
    }
    if (pthread_mutex_unlock(&(plst->_lock))) {
        printf("pthread_mutex_unlock failed\n");
        return -1;
    }
#endif
    return 0;
}

///////////////////////////////////////////////////////////
//double link list node
t_dlinklist_node *create_dlinklist_node(void *pkey, void *pdata)
{
    t_dlinklist_node *p = NULL;
    p = (t_dlinklist_node *)malloc(sizeof(t_dlinklist_node));
    if (p) {
        p->_pkey = pkey;
        p->_pdata = pdata;
        p->_pre = p;
        p->_next = p;
    } else {
        printf("create_dlinklist_node malloc failed\n");
    }

    return p;
}

void destroy_dlinklist_node(t_dlinklist_node *pnode)
{
    if (pnode) {
        // Notes: already free user malloc memmory !!!
        if (pnode->_pdata) {
            free(pnode->_pdata);
            pnode->_pdata = NULL;
        }
        free(pnode);
        pnode = NULL;
    }
    return ;
}

int insert_dlinklist_node(t_dlinklist_node *phead, void *pkey, void *pdata)
{
    t_dlinklist_node *pnode = create_dlinklist_node(pkey, pdata);
    if (pnode) {

        pnode->_pre = phead->_pre;
        pnode->_next = phead;
        phead->_pre->_next = pnode;
        phead->_pre = pnode;
        return 0;
    } else {
        printf("create_dlinklist_node failed\n");
    }
    return -1;
}

///////////////////////////////////////////////////////////
//double link list
t_dlinklist *create_dlinklist()
{
    t_dlinklist *p = NULL;
    p = (t_dlinklist *)malloc(sizeof(t_dlinklist));
    if (p) {
        p->_phead = NULL;
        __initlock(p);
    }

    return p;
}

void destroy_dlinklist(t_dlinklist *plst)
{
    t_dlinklist_node *ptmp = NULL;
    t_dlinklist_node *p = NULL;

    __lock(plst);
    p = plst->_phead;
    if (p) {
        while (p->_next != plst->_phead) {
            ptmp = p;
            p = p->_next;

            destroy_dlinklist_node(ptmp);
        }
        destroy_dlinklist_node(p);
    }
    __unlock(plst);

    if (plst) {
        __uninitlock(plst);
        free(plst);
        plst = NULL;
    }
    return ;
}

int insert_dlinklist_tail(t_dlinklist *plst, void *pkey, void *pdata)
{
    if (__lock(plst) < 0) {
        return -1;
    }
    if (NULL == plst->_phead) {
        plst->_phead = create_dlinklist_node(pkey, pdata);
    } else {
        insert_dlinklist_node(plst->_phead, pkey, pdata);
    }
    if (__unlock(plst) < 0) {
        return -1;
    }
    return 0;
}

int insert_dlinklist_head(t_dlinklist *plst, void *pkey, void *pdata)
{
    if (__lock(plst) < 0) {
        return -1;
    }
    if (NULL == plst->_phead) {
        plst->_phead = create_dlinklist_node(pkey,pdata);
    } else {
        insert_dlinklist_node(plst->_phead, pkey, pdata);
    }
    plst->_phead = plst->_phead->_pre;

    if (__unlock(plst) < 0) {
        return -1;
    }
    return 0;
}

t_dlinklist_node *find_dlinklist_node(t_dlinklist *plst, void *pkey)
{
    t_dlinklist_node *p = NULL;
    if (__lock(plst) < 0) {
        return NULL;
    }
    p = plst->_phead;
    if (p) {
        while (p->_pkey != pkey) {
            if (p->_next == plst->_phead) {
                p = NULL;
                break;
            }
            p = p->_next;
        }
    }
    if (__unlock(plst) < 0) {
        return NULL;
    }
    return p;
}

int delete_dlinklist_node(t_dlinklist *plst,  void *pkey)
{
    t_dlinklist_node *p = NULL;
    if(__lock(plst) < 0){
        return -1;
    }
    p = plst->_phead;
    if (p) {
        while (p->_pkey != pkey) {
            if (p->_next == plst->_phead) {
                p = NULL;
                break;
            }
            p = p->_next;
        }

        if (p) {
            if (p == plst->_phead) {
                if(p->_pre == p->_next && p->_next == p) {
                    //only one node in the list
                    plst->_phead = NULL;
                } else {
                    p->_pre->_next = plst->_phead->_next;
                    p->_next->_pre = plst->_phead->_pre;

                    plst->_phead = p->_next;
                }
            } else {
                p->_pre->_next = p->_next;
                p->_next->_pre = p->_pre;
            }
            destroy_dlinklist_node(p);
        }
    }
    if (__unlock(plst) < 0) {
        return -1;
    }
    return 0;
}

int travel_dlinklist(t_dlinklist *plst, __PFNTravelCB pfn, void *puserdata)
{
    int ncount = 0;
    t_dlinklist_node *p = NULL;
    if (__lock(plst) < 0) {
        return -1;
    }
    p = plst->_phead;
    if (p) {
        while (p->_next != plst->_phead) {
            ++ncount;
            pfn (p->_pkey, p->_pdata, puserdata);
            p = p->_next;
        }
        ++ncount;
        pfn (p->_pkey, p->_pdata, puserdata);
    }
    if (__unlock(plst) < 0) {
        return -1;
    }
    return ncount;
}
