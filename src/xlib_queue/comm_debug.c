#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "comm_debug.h"


void comm_dump_hex(char *p, int len)
{
    int i = 0;
    printf("============================================\n");
    for (i = 0; i < len; i++) {
        if (i % 16 == 0 && i != 0) {
            printf("\n");
        }
        printf("%02X ", (unsigned char)p[i]);
    }
    printf("\n");
    printf("============================================\n");
}