#ifndef _LOG_INFO_H_
#define _LOG_INFO_H_

#include "xlib_struct.h"

#define PRINT_CURRENT_TIME(info) do { print_current_time(info); } while(0);

void print_current_time_start(const char *info);
void print_current_time_stop(const char *info);

void debug_current_time(char *pTimeString);
int debug_write(char *pfile,char *pstr);
void local_debug(char *args,...);

#endif