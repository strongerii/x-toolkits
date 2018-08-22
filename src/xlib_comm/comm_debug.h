
#ifndef _DEF_COMM_DEBUG_H_
#define _DEF_COMM_DEBUG_H_

#include "comm_header.h"

void comm_dump_client(struct LIST_TCP_REQ *Req);
void comm_dump_list_req(COMM_MANAGER_S *pCommCtx);
void comm_dump_hex(char *p, int len);

#endif //_DEF_COMM_DEBUG_H_
