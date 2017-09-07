#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <fcntl.h>
#include <unistd.h>
#include <getopt.h>
#include <errno.h>//strerror(errno)
#include <time.h>
#include <stdarg.h> 
#include <sys/time.h>
#include "log_info.h"

#define DEF_MAX_LOGLINE_LEN						(256)
#define DEF_LOGFILE_LOCAL_NAME					"./local_debug.log"
/////////////////////////////////////////////////////////////////////////
struct timeval start_current1;

void print_current_time_start(const char *info)
{
    gettimeofday(&start_current1, NULL);
}

void print_current_time_end(const char *info)
{
    struct timeval current;
    gettimeofday(&current, NULL);
    long long int time1, time2;

    time1 = start_current1.tv_sec * 1000 + start_current1.tv_usec/1000;
    time2 = current.tv_sec * 1000 + current.tv_usec/1000;
    
    fprintf(stderr, "\033[1;31m[1]##### %s:\t%lld ms\033[0m\n", 
      info, time2 - time1);
}
/////////////////////////////////////////////////////////////////////////
void print_current_time(const char *info)
{
    struct timeval current;
    gettimeofday(&current, NULL);
    fprintf(stderr, "\033[1;31m##### %s:\t%lu.%lu\033[0m\n", info, current.tv_sec, current.tv_usec);
}

void debug_current_time(char *pTimeString)
{
	time_t current = time(NULL);
	struct tm* tmstruct = gmtime(&current);
  
	sprintf(pTimeString, "[%04d-%02d-%02d %02d:%02d:%02d] ",
              tmstruct->tm_year + 1900,
              tmstruct->tm_mon + 1,
              tmstruct->tm_mday,
              tmstruct->tm_hour,
              tmstruct->tm_min,
              tmstruct->tm_sec);
}

int debug_write(char *pfile,char *pstr)
{	

	FILE *pFile = NULL;
	char szLog[DEF_MAX_LOGLINE_LEN] = {0};
	char sTimeString[64] = {0};

	debug_current_time(sTimeString);
	
	sprintf(szLog, "%s:%s", sTimeString, pstr);
	
	pFile = fopen(pfile, "at");
	if(pFile){
		fwrite(szLog, strlen(szLog), 1, pFile);
		fclose(pFile);
	}
	
	return 0;
}

void local_debug(char *args,...)
{
	char szline[DEF_MAX_LOGLINE_LEN] = {0};
	va_list pArgList ;
	va_start(pArgList, args);
	vsprintf(szline, args, pArgList) ;
	va_end(pArgList);

	debug_write(DEF_LOGFILE_LOCAL_NAME, szline);
}

