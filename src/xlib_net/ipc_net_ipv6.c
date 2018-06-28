#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ipc_net_ipv6.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <netdb.h>
#include <errno.h>

int start_listen_ipv6(int port, context *pcontext)
{
    int i;
    int err;
    int on;
    struct addrinfo hints;
    char name[NI_MAXHOST];
    struct addrinfo *aip, *aip2;

    bzero(&hints, sizeof(hints));
    hints.ai_family = PF_UNSPEC;    
    hints.ai_flags = AI_PASSIVE;
    hints.ai_socktype = SOCK_STREAM;

    snprintf(name, sizeof(name), "%d", port);       //port
    if((err = getaddrinfo(NULL, name, &hints, &aip)) != 0) {
        perror(gai_strerror(err));
        exit(EXIT_FAILURE);
    }

    i = 0;
    for (aip2 = aip; aip2 != NULL; aip2 = aip2->ai_next) 
    {
#if 0
		printf("[%d]: [%s]", i, name);
		printf("\n\thost %s", aip2->ai_canonname ? aip2->ai_canonname:"-");
        if (aip2->ai_family == AF_INET) {
            sinp = (struct sockaddr_in *)aip2->ai_addr;
            addr = inet_ntop(AF_INET, &sinp->sin_addr, abuf, 256);
            printf(" address %s", addr?addr:"unknown");
            printf(" port %d", ntohs(sinp->sin_port));
        }
        printf("\n");
#endif
        if((pcontext->sd[i] = socket(aip2->ai_family, aip2->ai_socktype, 0)) < 0) {
			perror("socket failed");
            continue;
        }

        /* ignore "socket already in use" errors */
        on = 1;
        if(setsockopt(pcontext->sd[i], SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) < 0) {
            perror("setsockopt(SO_REUSEADDR) failed");
        }

        /* IPv6 socket should listen to IPv6 only, otherwise we will get "socket already in use" */
        on = 1;
        if(aip2->ai_family == AF_INET6 && setsockopt(pcontext->sd[i], IPPROTO_IPV6, 
                IPV6_V6ONLY, (const void *)&on , sizeof(on)) < 0) {
            perror("setsockopt(IPV6_V6ONLY) failed");
        }

        /* perhaps we will use this keep-alive feature oneday */
        /* setsockopt(sd, SOL_SOCKET, SO_KEEPALIVE, &on, sizeof(on)); */
        if(bind(pcontext->sd[i], aip2->ai_addr, aip2->ai_addrlen) < 0) 
        {
            perror("bind");
            pcontext->sd[i] = -1;
            continue;
        }

        if(listen(pcontext->sd[i], 10) < 0) 
        {
            perror("listen");
            pcontext->sd[i] = -1;
        } 
        else 
        {
            i++;
            if(i >= MAX_SD_LEN) 
            {
                printf("%s(): maximum number of server sockets exceeded", __FUNCTION__);
                i--;
                break;
            }
        }
    }
    pcontext->sd_len = i;

    if(pcontext->sd_len < 1) {
        printf("%s(): bind(%d) failed", __FUNCTION__, port);
        return -1;
    }
	printf("start listen ok ,Listen to port[%d]\n", port);
	return 0;
}


int handle_connection_ipv6(context *pcontext)
{
    int i;
    int err;
    int max_fds = 0;
    int fd = 0;
    struct sockaddr_storage client_addr;
    socklen_t addr_len = sizeof(struct sockaddr_storage);
    fd_set selectfds;

    do {
            FD_ZERO(&selectfds);
            for(i = 0; i < MAX_SD_LEN; i++) {
                if(pcontext->sd[i] != -1) {
                    FD_SET(pcontext->sd[i], &selectfds);
                    if(pcontext->sd[i] > max_fds) {
                        max_fds = pcontext->sd[i];
                    }
                }
            }

            err = select(max_fds + 1, &selectfds, NULL, NULL, NULL);
            if(err < 0 && errno != EINTR) {
                perror("select");
                exit(EXIT_FAILURE);
            }
    } while(err <= 0);

    for(i = 0; i < max_fds + 1; i++) {

        if(pcontext->sd[i] != -1 && FD_ISSET(pcontext->sd[i], &selectfds)) {
            fd = accept(pcontext->sd[i], (struct sockaddr *)&client_addr, &addr_len);

            /* start new thread that will handle this TCP connected client */
            printf("create thread to handle client that just established a connection[fd = %d]\n", fd);
#if 0
            /* commented out as it fills up syslog with many redundant entries */
            if(getnameinfo((struct sockaddr *)&client_addr, addr_len, name, sizeof(name), NULL, 0, NI_NUMERICHOST) == 0) {
                syslog(LOG_INFO, "serving client: %s\n", name);
            }
#endif
            //TODO:
        }
    }
    printf("leaving server thread, calling cleanup function now\n");

    return 0;
}