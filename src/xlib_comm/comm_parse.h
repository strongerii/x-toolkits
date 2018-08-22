#ifndef _DEF_COMM_PARSE_H_
#define _DEF_COMM_PARSE_H_

void comm_fd_set(COMM_MANAGER_S *pCommCtx);
void comm_fd_isset(COMM_MANAGER_S *pCommCtx);

int  comm_parse_creat(COMM_MANAGER_S *pCommCtx);
void comm_parse_destory(COMM_MANAGER_S *pCommCtx);

#endif //_DEF_COMM_PARSE_H_
