
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <signal.h>

#include "net_detect.h"

int g_main_loop_is_exit = 0;

void print_help()
{
    printf("command:\n");
    printf("\t-d: dest network address.\n");
    printf("\t-i: network phy name.\n");
    printf("\t-h: Help.\n");
}

static void __sigstop(int signo)
{
    g_main_loop_is_exit = 1;
}

int main(int argc, char *argv[])
{
    int c;
    int isExit = 0;
    char *PhyName = "eth1";
    char *DstAdress = "114.114.114.114";
    int status = -1;
    long spend_ms = 0;
    int ip_ttl = 0;

    signal(SIGPIPE, SIG_IGN);
    signal(SIGINT, __sigstop);
    signal(SIGQUIT, __sigstop);
    signal(SIGTERM, __sigstop);

    while ((c = getopt(argc, argv, "i:d:h")) != -1)
    {
        switch (c)
        {
        case 'i':
            PhyName = optarg;
            break;
        case 'd':
            DstAdress = optarg;
            break;
        case 'h':
        default:
            print_help();
            isExit = 1;
            break;
        }
    }

    if (isExit) {
        exit(0);
    }

    while(0 == g_main_loop_is_exit)
    {
        spend_ms = -1;
        ip_ttl = 0;
        status = NetIsOk(PhyName, DstAdress, &spend_ms, &ip_ttl);

        printf("[-> %s][%s]: InternetAccess=%d ttl=%d time=%ld ms\n",
            DstAdress, PhyName, status, ip_ttl, spend_ms);

        sleep(1);
    }

    return 0;
}
