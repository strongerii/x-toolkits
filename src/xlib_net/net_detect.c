#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <netdb.h>
#include <errno.h>
#include <arpa/inet.h>
#include <linux/sockios.h>
#include <sys/ioctl.h>
#include <linux/if.h>

#include <net/route.h>
#include <ctype.h>
#include <netdb.h>

#include <linux/sockios.h>
#include <linux/ethtool.h>
#include <sys/time.h>

#include "net_detect.h"

#define MAX_WAIT_TIME 3
#define MAX_NO_PACKETS 1
#define ICMP_HEADSIZE 8
#define PACKET_SIZE 4096

struct timeval tvrecv;
struct sockaddr_in dest_addr, recv_addr;
int sockfd;
int datalen = 56;
pid_t pid;
long g_spend_ms = 0;
int g_ip_ttl = 0;
char sendpacket[PACKET_SIZE];
char recvpacket[PACKET_SIZE];

//////////////////////////////////////////////////////////////////////////////////////////////
//      check net if link.
//////////////////////////////////////////////////////////////////////////////////////////////
int NetIsLink2(const char *iface)
{
    int fd;
    struct ifreq ifr;
    struct ethtool_value edata = {0};

    if ((fd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        printf("[NetStatus] socket failed.[%m]\n");
        return -1;
    }

    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, iface, sizeof(ifr.ifr_name) - 1);

    edata.cmd = ETHTOOL_GLINK;
    ifr.ifr_data = (caddr_t)&edata;

    if (ioctl(fd, SIOCETHTOOL, &ifr) == -1)
    {
        printf("[NetStatus] :[%s] ETHTOOL_GLINK failed. cmd:%u data:%u: [%m]\n", iface, edata.cmd, edata.data);
        close(fd);
        return -1;
    }

    close(fd);

    return edata.data ? 0 : -1;
}
//////////////////////////////////////////////////////////////////////////////////////////////
//      check net if link.
//////////////////////////////////////////////////////////////////////////////////////////////
int NetIsLink(const char *iface)
{
    FILE *pFile = NULL;
    char temp_buf[64] = {0};
    char carrier_path[64] = {0};
    int link_status = -1;

    snprintf(carrier_path, sizeof(carrier_path), "/sys/class/net/%s/carrier", iface);

    pFile = fopen(carrier_path, "r");
    if (NULL == pFile)
    {
        printf("[NetStatus] fopen[%s] is failed.[%m]\n", carrier_path);
        return -1;
    }

    while (fgets(temp_buf, sizeof(temp_buf), pFile) != NULL)
    {
        if (temp_buf[0] == '0')
        {
            link_status = -1;
        }
        else
        {
            link_status = 0;
        }
    }
    fclose(pFile);

    return link_status;
}

//////////////////////////////////////////////////////////////////////////////////////////////
//      check net if exist.
//////////////////////////////////////////////////////////////////////////////////////////////
int NetCheckPhyIsExist(const char *iface)
{
    char filename[64] = {0};

    snprintf(filename, sizeof(filename), "/sys/class/net/%s/address", iface);

    if (access(filename, F_OK) != 0)
    {
        //printf("wifi: [%s] is not exist!\n", iface);
        return -1;
    }

    return 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////
//      check net if access internet.
//////////////////////////////////////////////////////////////////////////////////////////////

// ip数据报头部20个字节 + ICMP报文64个字节（8字节的头部和56字节的数据）

// IP头部: 20 Bytes
// struct ip {
//     unsigned char  h_lenver;        //4 位IP版本号+4位首部长度
//     unsigned char  tos;             //8位服务类型TOS
//     unsigned short total_len;       //16位IP包总长度（字节）
//     unsigned short ident;           //16位标识, 用于辅助IP包的拆装
//     unsigned short frag_and_flags;  //3位标志位+13位偏移位, 也是用于IP包的拆装
//     unsigned char  ttl;             //8位IP包生存时间 TTL
//     unsigned char  proto;           //8位协议 (TCP, UDP 或其他)
//     unsigned short checksum;        //16位IP首部校验和,最初置零,等所有包头都填写正确后,计算并替换.
//     unsigned int   sourceIP;        //32位源IP地址
//     unsigned int   destIP;          //32位目的IP地址
// };

// ICMP头部: 8 Bytes.
// struct icmp
// {
//     unsigned char icmp_type;        // 类型
//     unsigned char icmp_code;        // 代码
//     unsigned short icmp_cksum;      // 校验和
//     unsigned short icmp_id;         // 标示符
//     unsigned short icmp_seq;        // 序列号
// };


unsigned short cal_chksum(unsigned short *addr, int len)
{
    int nleft = len;
    int sum = 0;
    unsigned short *w = addr;
    unsigned short answer = 0;
    while (nleft > 1) //把ICMP报头二进制数据以2字节为单位累加起来
    {
        sum += *w++;
        nleft -= 2;
    }
    if (nleft == 1) //若ICMP报头为奇数个字节,会剩下最后一字节.把最后一个字节视为一个2字节数据的高字节,这个2字节数据的低字节为0,继续累加
    {
        *(unsigned char *)(&answer) = *(unsigned char *)w;
        sum += answer;
    }
    sum = (sum >> 16) + (sum & 0xffff);
    sum += (sum >> 16);
    answer = ~sum;
    return answer;
}

int pack(int pkt_no, char *sendpacket)
{
    int packsize;
    struct icmp *icmp;
    struct timeval *tval;
    icmp = (struct icmp *)sendpacket;
    icmp->icmp_type = ICMP_ECHO;    //设置类型为ICMP请求报文
    icmp->icmp_code = 0;
    icmp->icmp_cksum = 0;
    icmp->icmp_seq = pkt_no;
    icmp->icmp_id = pid;            //设置当前进程ID为ICMP标示符
    packsize = ICMP_HEADSIZE + datalen;
    tval = (struct timeval *)icmp->icmp_data;
    gettimeofday(tval, NULL);
    icmp->icmp_cksum = cal_chksum((unsigned short *)icmp, packsize);
    return packsize;
}

int unpack(int cur_seq, char *buf, int len)
{
    int iphdrlen;
    struct ip *ip;
    struct icmp *icmp;
    struct timeval *tvsend;

    ip = (struct ip *)buf;
    iphdrlen = ip->ip_hl << 2;              //求ip报头长度,即ip报头的长度标志乘4
    icmp = (struct icmp *)(buf + iphdrlen); //越过ip报头,指向ICMP报头
    len -= iphdrlen;                        //ICMP报头及ICMP数据报的总长度

    if (len < 8)
    {
        return -1;
    }

    if ((icmp->icmp_type == ICMP_ECHOREPLY) && (icmp->icmp_id == pid) && (icmp->icmp_seq == cur_seq))
    {
        tvsend = (struct timeval *)icmp->icmp_data;
        g_spend_ms = (tvrecv.tv_sec * 1000 + tvrecv.tv_usec / 1000) - (tvsend->tv_sec * 1000 + tvsend->tv_usec / 1000);
        g_ip_ttl = ip->ip_ttl;

        return 0;
    }
    else
    {
        return -1;
    }
}

void _CloseSocket()
{
    close(sockfd);
    sockfd = 0;
}

static int send_packet(int pkt_no, char *sendpacket)
{
    int packetsize;
    packetsize = pack(pkt_no, sendpacket);

    if (sendto(sockfd, sendpacket, packetsize, 0, (struct sockaddr *)&dest_addr, sizeof(dest_addr)) < 0)
    {
        printf("[NetStatus] error : sendto error");
        return -1;
    }
    return 1;
}

static int recv_packet(int pkt_no, char *recvpacket)
{
    int n;
    int s32Ret = 0;
    unsigned int fromlen;
    fd_set rfds;
    struct timeval tm;
    fromlen = sizeof(recv_addr);

    while (1)
    {
        tm.tv_sec = MAX_WAIT_TIME;
        tm.tv_usec = 0;

        FD_ZERO(&rfds);
        FD_SET(sockfd, &rfds);

        s32Ret = select(sockfd + 1, &rfds, NULL, NULL, &tm);
        if (s32Ret < 0)
        {
            printf("[NetStatus] select failed[%d]\n", s32Ret);
            break;
        }
        else if (0 == s32Ret)
        {
            printf("select timeout[%d]\n", s32Ret);
            break;
        }
        else
        {
            if (FD_ISSET(sockfd, &rfds))
            {
                if ((n = recvfrom(sockfd, recvpacket, PACKET_SIZE, 0, (struct sockaddr *)&recv_addr, &fromlen)) < 0)
                {
                    printf("[NetStatus] recvfrom error: %m\n");
                    if (errno == EINTR)
                        return -1;

                    return -2;
                }
                gettimeofday(&tvrecv, NULL);
                if (unpack(pkt_no, recvpacket, n) == -1)
                {
                    printf("[NetStatus] unpack error\n");
                    continue;
                }
                return 1;
            }
        }
    }
    return s32Ret;
}

int NetIsOk(const char *iface, const char *address, long *pSpendMS, int *pTTL)
{
    struct ifreq if_ppp;
    unsigned long inaddr = 0l;
    int i;

    bzero(&dest_addr, sizeof(dest_addr));
    dest_addr.sin_family = AF_INET;

    inaddr = inet_addr(address);
    if (inaddr == INADDR_NONE)
    {
        struct hostent *host;
        char hostname[32];
        sprintf(hostname, "%s", address);

        if ((host = gethostbyname(hostname)) == NULL)
        {
            printf("[NetStatus]  gethostbyname failed! [%m]\n");
            return -1;
        }
        bcopy((char *)host->h_addr, (char *)&dest_addr.sin_addr, host->h_length);
    }
    else
    {
        dest_addr.sin_addr.s_addr = inet_addr(address);
    }

    if ((sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP)) < 0)
    {
        printf("[NetStatus]  error : socket[%m]\n");
        return -1;
    }

    strncpy(if_ppp.ifr_name, iface, IFNAMSIZ);
    if (setsockopt(sockfd, SOL_SOCKET, SO_BINDTODEVICE, (char *)&if_ppp, sizeof(if_ppp)) < 0)
    {
        printf("[NetStatus]  error : setsockopt[%m]\n");
        _CloseSocket();
        return -1;
    }

    int iFlag;
    if ((iFlag = fcntl(sockfd, F_GETFL, 0)) < 0)
    {
        printf("[NetStatus] error : fcntl(sockfd,F_GETFL,0)[%m]\n");
        _CloseSocket();
        return -1;
    }

    iFlag |= O_NONBLOCK;
    if ((iFlag = fcntl(sockfd, F_SETFL, iFlag)) < 0)
    {
        printf("[NetStatus] error : fcntl(sockfd,F_SETFL,iFlag )[%m]\n");
        _CloseSocket();
        return -1;
    }

    pid = getpid();
    for (i = 0; i < MAX_NO_PACKETS; i++)
    {
        if (send_packet(i, sendpacket) < 0)
        {
            printf("[NetStatus] error : send_packet\n");
            _CloseSocket();
            return -1;
        }

        if (recv_packet(i, recvpacket) > 0)
        {
            if (pSpendMS) {
                *pSpendMS = g_spend_ms;
            }
            if (pTTL) {
                *pTTL = g_ip_ttl;
            }
            _CloseSocket();
            return 0;
        }
    }
    _CloseSocket();
    return -1;
}

//////////////////////////////////////////////////////////////////////////////////////////////
// *  IPv4 add/del route item in route table
// *  inet_setroute RTACTION_ADD/RTACTION_DEL -net 192.56.76.0 netmask 255.255.255.0 dev eth0
// *  inet_setroute RTACTION_ADD/RTACTION_DEL gw 192.168.1.1 dev eth0
//////////////////////////////////////////////////////////////////////////////////////////////
int inet_setroute(int action, char **args)
{
    struct rtentry route; /* route item struct */
    char target[128] = {0};
    char gateway[128] = {0};
    char netmask[128] = {0};

    struct sockaddr_in *addr;

    int skfd;

    /* clear route struct by 0 */
    memset((char *)&route, 0x00, sizeof(route));

    /* default target is net (host)*/
    route.rt_flags = RTF_UP;

    args++;
    while (args)
    {
        if (*args == NULL)
        {
            break;
        }
        if (!strcmp(*args, "-net"))
        { /* default is a network target */
            args++;
            strcpy(target, *args);
            addr = (struct sockaddr_in *)&route.rt_dst;
            addr->sin_family = AF_INET;
            addr->sin_addr.s_addr = inet_addr(target);
            args++;
            continue;
        }
        else if (!strcmp(*args, "-host"))
        { /* target is a host */
            args++;
            strcpy(target, *args);
            addr = (struct sockaddr_in *)&route.rt_dst;
            addr->sin_family = AF_INET;
            addr->sin_addr.s_addr = inet_addr(target);
            route.rt_flags |= RTF_HOST;
            args++;
            continue;
        }
        else
        {
            // usage();
            return -1;
        }
        if (!strcmp(*args, "netmask"))
        { /* netmask setting */
            args++;
            strcpy(netmask, *args);
            addr = (struct sockaddr_in *)&route.rt_genmask;
            addr->sin_family = AF_INET;
            addr->sin_addr.s_addr = inet_addr(netmask);
            args++;
            continue;
        }
        if (!strcmp(*args, "gw") || !strcmp(*args, "gateway"))
        { /* gateway setting */
            args++;
            strcpy(gateway, *args);
            addr = (struct sockaddr_in *)&route.rt_gateway;
            addr->sin_family = AF_INET;
            addr->sin_addr.s_addr = inet_addr(gateway);
            route.rt_flags |= RTF_GATEWAY;
            args++;
            continue;
        }
        if (!strcmp(*args, "device") || !strcmp(*args, "dev"))
        { /* device setting */
            args++;
            route.rt_dev = *args;
            args++;
            continue;
        }
        if (!strcmp(*args, "mtu"))
        { /* mtu setting */
            args++;
            route.rt_flags |= RTF_MTU;
            route.rt_mtu = atoi(*args);
            args++;
            continue;
        }
        /* if you have other options, please put them in this place,
          like the options above. */
    }

    /* create a socket */
    skfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (skfd < 0)
    {
        printf("socket: %m\n");
        return -1;
    }

    /* tell the kernel to accept this route */
    if (action == RTACTION_DEL)
    { /* del a route item */
        if (ioctl(skfd, SIOCDELRT, &route) < 0)
        {
            printf("SIOCDELRT: %m\n");
            close(skfd);
            return -1;
        }
    }
    else
    { /* add a route item */
        if (ioctl(skfd, SIOCADDRT, &route) < 0)
        {
            printf("SIOCADDRT: %m\n");
            close(skfd);
            return -1;
        }
    }
    (void)close(skfd);

    return 0;
}
