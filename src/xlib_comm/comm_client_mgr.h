#ifndef _COMM_CLIENT_MGR_H_
#define _COMM_CLIENT_MGR_H_


#include "utils_dlinklist.h"

typedef void* CLIENT_MGR_HANDLE;

CLIENT_MGR_HANDLE comm_linklist_init();
void comm_linklist_uninit(CLIENT_MGR_HANDLE pHandle);
int comm_linklist_insert(CLIENT_MGR_HANDLE pHandle, int socket_fd, struct sockaddr_in *pAddr);
int comm_linklist_remove(CLIENT_MGR_HANDLE pHandle, int idx);
int comm_linklist_travel(CLIENT_MGR_HANDLE pHandle, __PFNTravelCB pfn, void *puserdata);

#endif //_COMM_CLIENT_MGR_H_