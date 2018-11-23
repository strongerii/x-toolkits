#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <signal.h>

#include "sys_utils.h"

int shell_system(const char * cmd)
{
    if(!cmd)
    {
        printf("shell_system cmd is null");
        return -1;
    }
    printf("shell_system : %s",cmd);
    int ret = 0;
    typedef void (*sighandler_t)(int);
    sighandler_t old_handler;

    /*
        如果父进程忽略了SIGCHID信号，waitpid就没有不能得到子进程的SIGCHLD信号,
        那么，处理的返回值就会有问题。system的返回值也会有问题, 通常可用此方法：
        signal函数返回之前信号处理handler
    */
    old_handler = signal(SIGCHLD, SIG_DFL);
    ret = system(cmd);
    signal(SIGCHLD, old_handler);
    if(ret == -1)
    {
        printf("shell_system fail = [%s]",strerror(errno));
        ret = -1;
    }
    else
    {
        /*
            WIFEXITED(status) 若此值为非0 表明进程正常结束。
        */
        if(WIFEXITED(ret))
        {
            /*
                 若上宏为真，此时可通过WEXITSTATUS(status)获取进程退出状态(exit时参数)
            */
            if(0 == WEXITSTATUS(ret))
            {
                printf("shell_system run shell script successfully, status = [%d]", WEXITSTATUS(ret));
                ret = 0;
            }
            else
            {
                printf("shell_system run shell script fail, status = [%d]", WEXITSTATUS(ret));
                ret = -1;
            }
        }
        /*
             WIFSIGNALED(status)为非0 表明进程异常终止。
        */
        else if(WIFSIGNALED(ret))
        {
            printf("shell_system abnormal termination, signal number = [%d]", WTERMSIG(ret));
            ret = -1;
        }
        /*
             WIFSTOPPED(status)为非0 表明进程处于暂停状态。
             WIFCONTINUED(status) 非0表示暂停后已经继续运行。
        */
        else if(WIFSTOPPED(ret))
        {
            printf("shell_system process stopped, signal number = [%d]", WSTOPSIG(ret));
            ret = -1;
        }
        else
        {
            printf("shell_system run shell unknown error, ret = [%d]", ret);
            ret = -1;
        }
    }
    return ret;
}

