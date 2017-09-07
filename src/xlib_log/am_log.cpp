/*******************************************************************************
 * am_log.cpp
 *
 * Histroy:
 *  2012-2-29 2012 - [ypchang] created file
 *
 * Copyright (C) 2008-2011, Ambarella ShangHai Co,Ltd
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella
 *
 ******************************************************************************/

#include "am_base_include.h"
#include "am_define.h"
#include "am_log.h"

#include <errno.h>
#include <stdarg.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <pthread.h>
#include <syslog.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/un.h>

#include <atomic>

#define empty_module_name(x)   (x ? x : "AmLog")
#define empty_level_str(x)     (x ? x : "error")
#define empty_target_str(x)    (x ? x : "stderr")
#define empty_timestamp_str(x) (x ? x : "false")

std::atomic_flag isInitialized = ATOMIC_FLAG_INIT;
std::atomic_flag lock = ATOMIC_FLAG_INIT;
//pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
//#define AM_LOCK()   pthread_mutex_lock(&mutex)
//#define AM_UNLOCK() pthread_mutex_unlock(&mutex)

static const int level_to_syslog[] =
{
 LOG_INFO,    /* PRINT  */
 LOG_ERR,     /* ERROR  */
 LOG_WARNING, /* WARN   */
 LOG_INFO,    /* STAT   */
 LOG_NOTICE,  /* NOTICE */
 LOG_INFO,    /* INFO   */
 LOG_DEBUG    /* DEBUG  */
};

//static const char * target_to_str[] =
//{
// "Stderr",
// "Syslog",
// "File",
// "Null",
// "Max"
//};

class LogFile
{
  public:
    LogFile()
      : logfd(-1), statfs_number(0)
    {
    }
    virtual ~LogFile()
    {
      set_log_fd(-1);
    }
    void set_log_fd(int fd)
    {
      if (fd >= 0) {
        if (logfd >= 0) {
          close(logfd);
        }
        logfd = fd;
      } else if (logfd >= 0) {
        close(logfd);
        logfd = -1;
        sync();
      }
    }
    int fd()
    {
      return logfd;
    }

  private:
    int logfd;
  public:
    int statfs_number;
};

static LogFile       logfile;
static AM_LOG_LEVEL  logLevel;
static AM_LOG_TARGET logTarget;
static bool          logTimeStamp;

static bool logLevelInitFlag = false;
static bool logTargetInitFlag = false;
static bool logTimeStampInitFlag = false;

static int log_unix_socket_conn()
{
  int fd	= -1;
  int isok = true;

  do {
  	socklen_t len = 0;
  	sockaddr_un un;

  	if ((fd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) {
  	  fprintf(stderr, "unix domain socket");
  	  isok = false;
  	  break;
  	}

    const char* server_name = "/run/oryx/log-server.socket";
  	memset(&un, 0, sizeof(un));
  	un.sun_family = AF_UNIX;
  	strcpy(un.sun_path, server_name);
  	len = offsetof(struct sockaddr_un, sun_path) + strlen(server_name);
  	if (AM_UNLIKELY(connect(fd, (struct sockaddr *)&un, len) < 0)) {
  	  if (AM_LIKELY(errno == ENOENT)) {
    		fprintf(stderr, "log-server is not working, wait and try again...\n");
  	  } else {
    		perror("connect");
  	  }
  	  isok = false;
  	  break;
  	}
  } while (0);
  fprintf(stderr, "log-server fd is %d\n",fd);

  if (AM_UNLIKELY(!isok)) {
    if (AM_LIKELY(fd >= 0)) {
      close(fd);
      fd = -1;
    }
  }
	
  return fd;
}

static inline void get_log_level()
{
  logLevel = AM_LOG_LEVEL_WARN;
  const char *levelstr = empty_level_str(getenv(AM_LEVEL_ENV_VAR));
  int         level    = atoi(levelstr);

  switch(level) {
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
    case 6: logLevel = AM_LOG_LEVEL(level); break;
    case 0: {
      if (is_str_equal(levelstr, "print") || is_str_equal(levelstr, "0")) {
        logLevel = AM_LOG_LEVEL_PRINT;
      } else if (is_str_equal(levelstr, "error")) {
        logLevel = AM_LOG_LEVEL_ERROR;
      } else if (is_str_equal(levelstr, "warn")) {
        logLevel = AM_LOG_LEVEL_WARN;
      } else if (is_str_equal(levelstr, "stat")) {
        logLevel = AM_LOG_LEVEL_STAT;
      } else if (is_str_equal(levelstr, "notice")) {
        logLevel = AM_LOG_LEVEL_NOTICE;
      } else if (is_str_equal(levelstr, "info")) {
        logLevel = AM_LOG_LEVEL_INFO;
      } else if (is_str_equal(levelstr, "debug")) {
        logLevel = AM_LOG_LEVEL_DEBUG;
      }
    }break;
    default: break;
  }
}

static inline void get_log_target()
{
  logTarget = AM_LOG_TARGET_NULL;
  const char *target = empty_target_str(getenv(AM_TARGET_ENV_VAR));

  if (is_str_equal(target, "stderr")) {
    logTarget = AM_LOG_TARGET_STDERR;
  } else if (is_str_equal(target, "syslog")) {
    logTarget = AM_LOG_TARGET_SYSLOG;
  } else if (is_str_start_with(target, "file:")) {
    int fd = -1;
    char logname[512] = {0};
    #if 0
    time_t current = time(NULL);
    struct tm* tmstruct = gmtime(&current);
    const char *filename = strstr(target, ":");
    const char* ext = strrchr(&filename[1], '.');
    if (ext) {
      snprintf(logname, (ext - &filename[1]) + 1, "%s", &filename[1]);
      sprintf(logname + strlen(logname), "-%04d%02d%02d%02d%02d%02d",
              tmstruct->tm_year + 1900, tmstruct->tm_mon + 1,
              tmstruct->tm_mday, tmstruct->tm_hour,
              tmstruct->tm_min,  tmstruct->tm_sec);
      sprintf(logname + strlen(logname), "%s", ext);
    } else {
      sprintf(logname, "%s-%04d%02d%02d%02d%02d%02d.log",
              &filename[1],
              tmstruct->tm_year + 1900,
              tmstruct->tm_mon + 1,
              tmstruct->tm_mday,
              tmstruct->tm_hour,
              tmstruct->tm_min,
              tmstruct->tm_sec);
    }
    #else
    const char *filename = strstr(target, ":");
    strncpy(logname, &filename[1], sizeof(logname));
    #endif
    if ((fd = open(logname,
                   O_WRONLY|O_APPEND|O_CREAT,
                   S_IRUSR|S_IWUSR)) >= 0) {
      logfile.set_log_fd(fd);
      logTarget = AM_LOG_TARGET_FILE;
    } else {
      fprintf(stderr,
              "Failed to open file %s: %s(Reset log target to stderr)",
              &filename[1], strerror(errno));
      logTarget = AM_LOG_TARGET_STDERR;
    }
  } else if (is_str_equal(target, "null")) {
    logTarget = AM_LOG_TARGET_NULL;
  } else if (is_str_equal(target, "socket")) {
    int fd = log_unix_socket_conn();
    if (fd >= 0) {
      logfile.set_log_fd(fd);
      logTarget = AM_LOG_TARGET_SOCKET;
    } else {
      fprintf(stderr,
              "Failed to open socket: %s(Reset log target to stderr)",
              strerror(errno));
      logTarget = AM_LOG_TARGET_STDERR;
    }
  }
}

static inline void get_timestamp_setting()
{
  const char *target = empty_timestamp_str(getenv(AM_TIMESTAMP_ENV_VAR));

  if (is_str_equal(target, "Yes")  ||
      is_str_equal(target, "True") ||
      is_str_equal(target, "1")    ||
      is_str_equal(target, "On")) {
    logTimeStamp = true;
  } else {
    logTimeStamp = false;
  }
}

static inline void init()
{
  if (AM_UNLIKELY(!isInitialized.test_and_set())) {
      if (!logLevelInitFlag) {
        get_log_level();
        logLevelInitFlag = true;
      }
      if (!logTargetInitFlag) {
        get_log_target();
        logTargetInitFlag = true;
      }
      if (!logTimeStampInitFlag) {
        get_timestamp_setting();
        logTimeStampInitFlag = true;
      }
  }
}

static inline const char *get_timestamp()
{
  time_t current = time(NULL);
  static char timestring[128] = {0};
  int ret = snprintf(timestring, sizeof(timestring), "%s", ctime(&current));
  if (ret < (int)sizeof(timestring)) {
    char *end = strstr(timestring, "\n");
    timestring[ret] = '\0';
    if (end) {
      *end = '\0';
    }
  } else {
    timestring[sizeof(timestring) - 1] = '\0';
  }

  return timestring;
}

static inline void am_level_logv(const char  *module,
                                 AM_LOG_LEVEL level,
                                 const char  *format,
                                 va_list      vlist)
{
  char text[2*1024] = {0};
  char str[4*1024] = {0};
  int len = vsnprintf(text, sizeof(text), format, vlist);
  if (AM_LIKELY((uint32_t)len < sizeof(text))) {
    text[len] = '\0';
  } else {
    text[sizeof(text) - 1] = '\0';
  }
  logTimeStamp ?
      snprintf(str, sizeof(str) - 1, "[%s]%s\n", get_timestamp(), text) :
      snprintf(str, sizeof(str) - 1, "%s\n", text);

  while(lock.test_and_set(std::memory_order_acquire)); /* Spin lock */
  switch(logTarget) {
    case AM_LOG_TARGET_STDERR: {
      fprintf(stderr, "%s", str);
    }break;
    case AM_LOG_TARGET_SYSLOG: {
      openlog(empty_module_name(module), LOG_PID, LOG_USER);
      syslog(level_to_syslog[level], "%s\n", text);
    }break;
    case AM_LOG_TARGET_FILE: {
      if ((write(logfile.fd(), str, strlen(str)) < 0)) {
        fprintf(stderr, "%s: %s: %d\n",
                B_RED("Failed writing logs to file. Redirect log to console"),
                strerror(errno), logfile.fd());
        logfile.set_log_fd(-1);
        logTarget = AM_LOG_TARGET_STDERR;
        fprintf(stderr, "%s", str);
      } else {
        logfile.statfs_number++;
        if (logfile.statfs_number >= 10) {
          logfile.statfs_number = 0;
          struct stat buf;
          if (fstat(logfile.fd(), &buf) < 0) {
            fprintf(stderr, "%s: %s: %d\n",
                B_RED("Failed to fstat file. Redirect log to console"),
                strerror(errno), logfile.fd());
            logfile.set_log_fd(-1);
            logTarget = AM_LOG_TARGET_STDERR;
            fprintf(stderr, "%s", str);
          } else {
            if (buf.st_size >= (1*1024*1024)) {
              bool fres = false;
              do {
                if (ftruncate(logfile.fd(), 0) < 0) {
                  break;
                }
                if (lseek(logfile.fd(), 0, SEEK_SET) < 0) {
                  break;
                }
                fres = true;
              } while (0);
              if (!fres) {
                fprintf(stderr, "%s: %s: %d\n",
                    B_RED("Failed to ftruncate or lseek file. Redirect log to console"),
                    strerror(errno), logfile.fd());
                logfile.set_log_fd(-1);
                logTarget = AM_LOG_TARGET_STDERR;
                fprintf(stderr, "%s", str);
              }
            }
          }
        }
      }
    }break;
    case AM_LOG_TARGET_SOCKET: {
      if (send(logfile.fd(), str, strlen(str), 0) < 0) {
        fprintf(stderr, "%s: %s: %d\n",
                B_RED("Failed writing logs to socket. Redirect log to console"),
                strerror(errno), logfile.fd());
        logfile.set_log_fd(-1);
        logTarget = AM_LOG_TARGET_STDERR;
        fprintf(stderr, "%s", str);
      }
    } break;
    case AM_LOG_TARGET_NULL:
    default: break;
  }
  lock.clear(std::memory_order_release);
}

static inline void class_name(char *name, int len, const char *pretty_func)
{
  char temp[256] = {0};
  int ret = sprintf(temp, "%s", pretty_func);
  char *end = NULL;
  if (ret < (int)sizeof(temp)) {
    temp[ret] = '\0';
  } else {
    temp[sizeof(temp) - 1] = '\0';
  }
  end = strstr(temp, "::");
  if (end) { /* Found */
    char *classname = NULL;
    *end = '\0';
    classname = strrchr(temp, (int)' ');
    if (classname) {
      sprintf(name, "%s", &classname[1]);
    } else {
      sprintf(name, "%s", temp);
    }
  } else { /* Not Found */
    sprintf(name, "Function");
  }
}

static inline void function_name(char *name, int len, const char *pretty_func)
{
  char temp[256] = {0};
  int ret = sprintf(temp, "%s", pretty_func);
  if (ret < (int)sizeof(temp)) {
    temp[ret] = '\0';
  } else {
    temp[sizeof(temp) - 1] = '\0';
  }
  char *start = strstr(temp, "::");
  if (start) {
    sprintf(name, "%s", &start[2]);
  } else {
    sprintf(name, "%s", temp);
  }
}

static inline void file_name(char *name, int len, const char *file)
{
  char temp[256] = {0};
  int ret = sprintf(temp, "%s", file);
  if (ret < (int)sizeof(temp)) {
    temp[ret] = '\0';
  } else {
    temp[sizeof(temp) - 1] = '\0';
  }
  char *start = strrchr(temp, (int)'/');
  if (start) {
    sprintf(name, "%s", &start[1]);
  } else {
    sprintf(name, "%s", temp);
  }
}

bool set_log_level(const char *level, bool change)
{
  bool result = (0 == setenv(AM_LEVEL_ENV_VAR, level, change ? 1 : 0));
  logLevelInitFlag = false;
  isInitialized.clear();
  return result;
}

bool set_log_target(const char *target, bool change)
{
  bool result = (0 == setenv(AM_TARGET_ENV_VAR, target, change ? 1 : 0));
  logTargetInitFlag = false;
  isInitialized.clear();
  return result;
}

bool set_timestamp_enabled(bool enable)
{
  bool result = (0 == setenv(AM_TIMESTAMP_ENV_VAR, enable ? "Yes" : "No", 1));
  logTimeStampInitFlag = false;
  isInitialized.clear();
  return result;
}

void close_log_file()
{
  logfile.set_log_fd(-1);
}

void am_debug(const char *pretty_func, const char *_file,
              int                line, const char *_format, ...)
{
  init();
  if (AM_UNLIKELY(AM_LOG_LEVEL_DEBUG <= logLevel)) {
    char text[4*1024]   = {0};
    char module[256]    = {0};
    char func[256]      = {0};
    char file[256]      = {0};
    char format[2*1024] = {0};
    va_list vlist;

    function_name(func, sizeof(func), pretty_func);
    class_name(module, sizeof(module), pretty_func);
    file_name(file, sizeof(file), _file);
    snprintf(format, sizeof(format), "%s", _format);
    if (format[strlen(format) - 1] == '\n') {
      //Eat trailing \n, \n will be added automatically!
      format[strlen(format) - 1] = '\0';
    }
    if (logTarget == AM_LOG_TARGET_SYSLOG) {
      sprintf(text, "[%6s][%s:%d: %s]: %s",
              "DEBUG", file, line, func, format);
    } else if ((logTarget == AM_LOG_TARGET_FILE) ||
               (logTarget == AM_LOG_TARGET_SOCKET)) {
      sprintf(text, "[%28s][%6s][%s:%d: %s]: %s",
              module, "DEBUG", file, line, func, format);
    } else {
      sprintf(text, "[" B_GREEN("%28s") "][" B_BLUE("%6s") "]" \
              CYAN("[%s:%d: %s]: ")BLUE("%s"),
              module, "DEBUG", file, line, func, format);
    }
    va_start(vlist, _format);
    am_level_logv(module, AM_LOG_LEVEL_DEBUG, text, vlist);
    va_end(vlist);
  }
}

void am_info(const char *pretty_func, const char *_file,
             int                line, const char *_format, ...)
{
  init();
  if (AM_UNLIKELY(AM_LOG_LEVEL_INFO <= logLevel)) {
    char text[4*1024]   = {0};
    char module[256]    = {0};
    char func[256]      = {0};
    char file[256]      = {0};
    char format[2*1024] = {0};
    va_list vlist;

    function_name(func, sizeof(func), pretty_func);
    class_name(module, sizeof(module), pretty_func);
    file_name(file, sizeof(file), _file);
    snprintf(format, sizeof(format), "%s", _format);
    if (format[strlen(format) - 1] == '\n') {
      //Eat trailing \n, \n will be added automatically!
      format[strlen(format) - 1] = '\0';
    }
    if (logTarget == AM_LOG_TARGET_SYSLOG) {
      sprintf(text, "[%6s][%s:%d: %s]: %s",
              "INFO", file, line, func, format);
    } else if ((logTarget == AM_LOG_TARGET_FILE) ||
               (logTarget == AM_LOG_TARGET_SOCKET)) {
      sprintf(text, "[%28s][%6s][%s:%d: %s]: %s",
              module, "INFO", file, line, func, format);
    } else {
      sprintf(text, "[" B_GREEN("%28s") "][" B_GREEN("%6s") "]" \
              CYAN("[%s:%d: %s]: ")GREEN("%s"),
              module, "INFO", file, line, func, format);
    }
    va_start(vlist, _format);
    am_level_logv(module, AM_LOG_LEVEL_INFO, text, vlist);
    va_end(vlist);
  }
}

void am_notice(const char *pretty_func, const char *_file,
               int                line, const char *_format, ...)
{
  init();
  if (AM_UNLIKELY(AM_LOG_LEVEL_NOTICE <= logLevel)) {
    char text[2*1024]   = {0};
    char module[256]    = {0};
    char func[256]      = {0};
    char file[256]      = {0};
    char format[2*1024] = {0};
    va_list vlist;

    function_name(func, sizeof(func), pretty_func);
    class_name(module, sizeof(module), pretty_func);
    file_name(file, sizeof(file), _file);
    snprintf(format, sizeof(format), "%s", _format);
    if (format[strlen(format) - 1] == '\n') {
      //Eat trailing \n, \n will be added automatically!
      format[strlen(format) - 1] = '\0';
    }
    if (logTarget == AM_LOG_TARGET_SYSLOG) {
      sprintf(text, "[%6s][%s:%d: %s]: %s",
              "NOTICE", file, line, func, format);
    } else if ((logTarget == AM_LOG_TARGET_FILE) ||
               (logTarget == AM_LOG_TARGET_SOCKET)) {
      sprintf(text, "[%28s][%6s][%s:%d: %s]: %s",
              module, "NOTICE", file, line, func, format);
    } else {
      sprintf(text, "[" B_GREEN("%28s") "][" B_MAGENTA("%6s") "]" \
              CYAN("[%s:%d: %s]: ")MAGENTA("%s"),
              module, "NOTICE", file, line, func, format);
    }
    va_start(vlist, _format);
    am_level_logv(module, AM_LOG_LEVEL_NOTICE, text, vlist);
    va_end(vlist);
  }
}

void am_stat(const char *pretty_func, const char *_file,
             int                line, const char *_format, ...)
{
  init();
  if (AM_UNLIKELY(AM_LOG_LEVEL_STAT <= logLevel)) {
    char text[2*1024]   = {0};
    char module[256]    = {0};
    char func[256]      = {0};
    char file[256]      = {0};
    char format[2*1024] = {0};
    va_list vlist;

    function_name(func, sizeof(func), pretty_func);
    class_name(module, sizeof(module), pretty_func);
    file_name(file, sizeof(file), _file);
    snprintf(format, sizeof(format), "%s", _format);
    if (format[strlen(format) - 1] == '\n') {
      //Eat trailing \n, \n will be added automatically!
      format[strlen(format) - 1] = '\0';
    }
    if (logTarget == AM_LOG_TARGET_SYSLOG) {
      sprintf(text, "[%6s][%s:%d: %s]: %s",
              "STAT", file, line, func, format);
    } else if ((logTarget == AM_LOG_TARGET_FILE) ||
               (logTarget == AM_LOG_TARGET_SOCKET)) {
      sprintf(text, "[%28s][%6s][%s:%d: %s]: %s",
              module, "STAT", file, line, func, format);
    } else {
#if 0
      sprintf(text, "[" B_GREEN("%28s") "][" B_CYAN("%6s") "]" \
              CYAN("[%s:%d: %s]: ")B_WHITE("%s"),
              module, "STAT", file, line, func, format);
#else
      sprintf(text, "[" B_GREEN("%28s") "][" B_CYAN("%6s") "]" \
              CYAN("[%s:%d: %s]: ") "%s",
              module, "STAT", file, line, func, format);
#endif
    }
    va_start(vlist, _format);
    am_level_logv(module, AM_LOG_LEVEL_STAT, text, vlist);
    va_end(vlist);
  }
}

void am_warn(const char *pretty_func, const char *_file,
             int                line, const char *_format, ...)
{
  init();
  if (AM_UNLIKELY(AM_LOG_LEVEL_WARN <= logLevel)) {
    char text[2*1024]   = {0};
    char module[256]    = {0};
    char func[256]      = {0};
    char file[256]      = {0};
    char format[2*1024] = {0};
    va_list vlist;

    function_name(func, sizeof(func), pretty_func);
    class_name(module, sizeof(module), pretty_func);
    file_name(file, sizeof(file), _file);
    snprintf(format, sizeof(format), "%s", _format);
    if (format[strlen(format) - 1] == '\n') {
      //Eat trailing \n, \n will be added automatically!
      format[strlen(format) - 1] = '\0';
    }
    if (logTarget == AM_LOG_TARGET_SYSLOG) {
      sprintf(text, "[%6s][%s:%d: %s]: %s",
              "WARN", file, line, func, format);
    } else if ((logTarget == AM_LOG_TARGET_FILE) ||
               (logTarget == AM_LOG_TARGET_SOCKET)) {
      sprintf(text, "[%28s][%6s][%s:%d: %s]: %s",
              module, "WARN", file, line, func, format);
    } else {
      sprintf(text, "[" B_GREEN("%28s") "][" B_YELLOW("%6s") "]" \
              CYAN("[%s:%d: %s]: ")YELLOW("%s"),
              module, "WARN", file, line, func, format);
    }
    va_start(vlist, _format);
    am_level_logv(module, AM_LOG_LEVEL_WARN, text, vlist);
    va_end(vlist);
  }
}

void am_error(const char *pretty_func, const char *_file,
              int                line, const char *_format, ...)
{
  init();
  if (AM_UNLIKELY(AM_LOG_LEVEL_ERROR <= logLevel)) {
    char text[2*1024]   = {0};
    char module[256]    = {0};
    char func[256]      = {0};
    char file[256]      = {0};
    char format[2*1024] = {0};
    va_list vlist;

    function_name(func, sizeof(func), pretty_func);
    class_name(module, sizeof(module), pretty_func);
    file_name(file, sizeof(file), _file);
    snprintf(format, sizeof(format), "%s", _format);
    if (format[strlen(format) - 1] == '\n') {
      //Eat trailing \n, \n will be added automatically!
      format[strlen(format) - 1] = '\0';
    }
    if (logTarget == AM_LOG_TARGET_SYSLOG) {
      sprintf(text, "[%6s][%s:%d: %s]: %s",
              "ERROR", file, line, func, format);
    } else if ((logTarget == AM_LOG_TARGET_FILE) ||
               (logTarget == AM_LOG_TARGET_SOCKET)) {
      sprintf(text, "[%28s][%6s][%s:%d: %s]: %s",
              module, "ERROR", file, line, func, format);
    } else {
      sprintf(text, "[" B_GREEN("%28s") "][" B_RED("%6s") "]" \
              CYAN("[%s:%d: %s]: ")RED("%s"),
              module, "ERROR", file, line, func, format);
    }
    va_start(vlist, _format);
    am_level_logv(module, AM_LOG_LEVEL_ERROR, text, vlist);
    va_end(vlist);
  }
}

void am_print(const char *_format, ...)
{
  init();
  if (AM_UNLIKELY(AM_LOG_LEVEL_PRINT <= logLevel)) {
    char text[2*1024]   = {0};
    char format[2*1024] = {0};
    va_list vlist;

    snprintf(format, sizeof(format), "%s", _format);
    if (format[strlen(format) - 1] == '\n') {
      //Eat trailing \n, \n will be added automatically!
      format[strlen(format) - 1] = '\0';
    }
    if (logTarget == AM_LOG_TARGET_SYSLOG) {
      sprintf(text, "%s", format);
    } else if ((logTarget == AM_LOG_TARGET_FILE) ||
               (logTarget == AM_LOG_TARGET_SOCKET)) {
      sprintf(text, "%s", format);
    } else {
      //sprintf(text, B_WHITE("%s"), format);
      sprintf(text, ("%s"), format);
    }
    va_start(vlist, _format);
    am_level_logv(NULL, AM_LOG_LEVEL_PRINT, text, vlist);
    va_end(vlist);
  }
}
