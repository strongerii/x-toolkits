#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>//strerror(errno)
#include <getopt.h>
#include <sched.h>
#include <time.h>
#include <pthread.h>
#include <netdb.h>
#include <sys/signal.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <net/if_arp.h>
#include <linux/sockios.h>
#include <netinet/if_ether.h>
#include "net_utils.h"
#include "base64.h"

#define DEF_MAILDIR_PATH	"./mail"

#define DEF_MAX_CMD_LEN		128
#define ETHTOOL_GLINK		0x0000000a

struct ethtool_value {
	unsigned int	cmd;
	unsigned int	data;
};

int cur_time(char *pTime)
{
	char *wday[]={"Sun","Mon","Tue","Wed","Thu","Fri","Sat"};
	char *mday[]={"Jan","Feb","Mar","Apr","May","Jun","Jul","Agu","Sep","Oct","Nov","Dec"};
	time_t timep;
	struct tm *p;
	time(&timep);
	p = localtime(&timep);

	sprintf(pTime,"%s, %d %s %d %02d:%02d:%02d +0800",\
		wday[p->tm_wday],p->tm_mday,mday[1+p->tm_mon],(1900+p->tm_year),\
		p->tm_hour,p->tm_min,p->tm_sec);
	return 0;
}

int send_to_email(const char *szlPath)
{
	FILE *pFile = NULL;
	char *pt;
	char buf_str[1024];
	char buf_out[1024];
	int len_out = 0;
	int nRet = 0;
	char szCmd[DEF_MAX_CMD_LEN] = {0};
	char buf_time[128];
	
	NSV_SMTPINFO smtp;
	//memset(&smtp, 0, sizeof(smtp));
	cur_time(buf_time);
/*	
	nRet = get_smtp_cfg(&smtp);
	if(nRet < 0){
		printf("send_email get_smtp_cfg failed\n");
		return -1;
	}
*/
	//config file
	pFile = fopen("./mail/ssmtp.conf", "w+");
	if(NULL == pFile){
		printf("send_email ssmtp.conf failed\n");
		return -1;
	}
	if ( (pt = strrchr(szlPath,'/')) == 0 )
	{
		printf("strrchr '/' fail!\n");	
		return -1;
	}
	++pt;
	fprintf(pFile, "root=%s\n",smtp.szSender);
	fprintf(pFile, "mailhub=%s:%d\n",smtp.szServer,smtp.uPort);
	fprintf(pFile, "hostname=%s\n",smtp.szSender);
	fprintf(pFile, "AuthUser=%s\n",smtp.szUserNm);
	fprintf(pFile, "AuthPass=%s\n",smtp.szUserPass);	
	fprintf(pFile, "FromLineOverride=%s\n","YES");
	fprintf(pFile, "UseTLS=%s\n",smtp.uSSLEnable == 0 ? "NO" : "YES");
	fprintf(pFile, "UseSTARTTLS=%s\n","YES");

	fclose(pFile);

	//msg file
	pFile = fopen("./mail/msg.txt", "w+");
	if(NULL == pFile){
		printf("send_email ssmtp.conf failed\n");
		return -1;
	}
	
	fprintf(pFile, "Date:%s\n",buf_time);
	fprintf(pFile, "From:%s\n",smtp.szSender);
	fprintf(pFile, "To:%s\n",smtp.szRecver);
	fprintf(pFile, "Subject:%s\n",smtp.szTitle);
	fprintf(pFile, "MIME-Version: 1.0\n");
	fprintf(pFile, "Content-Type:multipart/mixed;\n");
	fprintf(pFile, " boundary=\"=====001_NextPart167448323667_=====\"\n");	//邮件正文开始

	fprintf(pFile, "\r\n");
	fprintf(pFile, "--=====001_NextPart167448323667_=====\n");
	fprintf(pFile, "Content-Type:multipart/alternative;\n");
	fprintf(pFile, " boundary=\"=====002_NextPart167448323667_=====\"\r\n");
	
	fprintf(pFile, "--=====002_NextPart167448323667_=====\n");
	fprintf(pFile, "Content-Type: text/plain; charset=\"UTF-8\"\n");
	fprintf(pFile, "Content-Transfer-Encoding: base64\r\n");
	sprintf(buf_str,"Hello!\n	This e-mail is come from montion detection of IPNC.\n");
	base64Encode(buf_str,buf_out,strlen(buf_str),&len_out);
	fprintf(pFile, "%s\r\n",buf_out);
	fprintf(pFile, "--=====002_NextPart167448323667_=====--\r\n");

	
	fprintf(pFile, "--=====001_NextPart167448323667_=====\n");
//	fprintf(pFile, "Content-Type: application/octet-stream;\n");
	fprintf(pFile, "Content-Transfer-Encoding: base64\n");
	fprintf(pFile, "Content-Type:image/jpeg; name=\"%s\"\n",pt);
	fprintf(pFile, "Content-Disposition: attachment; filename=%s\r\n\r\n",pt);
	int in = 0;
	int ufile_size = 0;
	if( (in = open(szlPath,O_RDONLY)) == -1 ){
		printf("File open error!\n");
		return -1;
	}
	if( (ufile_size = lseek(in, 0, SEEK_END)) == -1 ) {
		printf("fseek SEEK_END error");
		return -1;
	}
	ufile_size += 20;
	if( lseek(in, 0, SEEK_SET) == -1 ) {
		printf("fseek SEEK_SET error");
		return -1;
	}
	base64Encode_file(in, pFile, ufile_size);

	
	fprintf(pFile, "\n--=====001_NextPart167448323667_=====--\r\n\r\n");	// 邮件结束

	close(in);
	fclose(pFile);
	
	//to_address=$1
	//msg_file=$2
	sprintf(szCmd, "%s/ssmtp -C %s %s %s",DEF_MAILDIR_PATH,"./mail/ssmtp.conf", smtp.szRecver,"<./mail/msg.txt");
	nRet = system(szCmd);
	if(nRet == 0){
		printf("send_email cmd=%s sucess\n", szCmd);
		return nRet;
	}
	printf("send_email cmd=%s failed\n", szCmd);
	return nRet;
}


/////////detect ip line drop/////////////////////
int mii_diag(char *ethname)
{
	int skfd;
	char *ifname;
	int result=0;
	struct ethtool_value edata;
	struct ifreq ifr;


	if (!strcmp(ethname,"eth0") ||!strcmp(ethname,"eth1") ||!strcmp(ethname,"bond0") )
	{
		if ((skfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
		{
			printf("mii_diag Socket Failed\n");
			fprintf(stderr, "skfd: %d, errno: %d\n", skfd, errno);
			return -1;
			//exit(-1);
		}

		if (NULL != ethname)
		{
			ifname = ethname;
		}
		else
		{
			ifname = "eth0";
		}

		memset(&ifr, 0, sizeof(ifr));
		strcpy(ifr.ifr_name, ifname);

		edata.cmd = ETHTOOL_GLINK;
		ifr.ifr_data = (char *)&edata;

		if (!(ioctl(skfd, SIOCETHTOOL, &ifr)<0))
		{
			if(edata.data)
			{
				result = 0;
			}
			else
			{
				result = -1;
			}
		}
		else
		{
			close(skfd);
			printf("mii_diag Ioctl SIOCETHTOOL\n");
			return -2;
		}

		close(skfd);
	}

	return result;

}


/////////detect ip addr conflict/////////////////////
static struct in_addr myself, mymask;
static u_char *ptr;
static struct ifreq ifr; /*   ifr   structure   */
#define	MAC_BCAST_ADDR		"\xff\xff\xff\xff\xff\xff"

struct arpMsg1 {
	/* Ethernet header */
	u_char h_dest[6]; /* destination ether addr */
	u_char h_source[6]; /* source ether addr */
	u_short h_proto; /* packet type ID field */

	/* ARP packet */
	uint16_t htype; /* hardware type (must be ARPHRD_ETHER) */
	uint16_t ptype; /* protocol type (must be ETH_P_IP) */
	uint8_t hlen; /* hardware address length (must be 6) */
	uint8_t plen; /* protocol address length (must be 4) */
	uint16_t operation; /* ARP opcode */
	uint8_t sHaddr[6]; /* sender's hardware address */
	uint8_t sInaddr[4]; /* sender's IP address */
	uint8_t tHaddr[6]; /* target's hardware address */
	uint8_t tInaddr[4]; /* target's IP address */
	uint8_t pad[18]; /* pad for min. Ethernet payload (60 bytes) */
} ATTRIBUTE_PACKED;

/* Include our own copy of struct sysinfo to avoid binary compatability
 * problems with Linux 2.4, which changed things.  Grumble, grumble. */
struct sysinfo {
	long uptime;			/* Seconds since boot */
	unsigned long loads[3];		/* 1, 5, and 15 minute load averages */
	unsigned long totalram;		/* Total usable main memory size */
	unsigned long freeram;		/* Available memory size */
	unsigned long sharedram;	/* Amount of shared memory */
	unsigned long bufferram;	/* Memory used by buffers */
	unsigned long totalswap;	/* Total swap space size */
	unsigned long freeswap;		/* swap space still available */
	unsigned short procs;		/* Number of current processes */
	unsigned short pad;			/* Padding needed for m68k */
	unsigned long totalhigh;	/* Total high memory size */
	unsigned long freehigh;		/* Available high memory size */
	unsigned int mem_unit;		/* Memory unit size in bytes */
	char _f[20-2*sizeof(long)-sizeof(int)];	/* Padding: libc5 uses this.. */
};
extern int sysinfo (struct sysinfo* info);

static long uptime(void) 
{
	struct sysinfo info;
	sysinfo(&info);
	return info.uptime;
}

int getIPMAC(const char *pEthName)
{
	int ret = 0;
	int fd_arp; /*   socket   fd   for   receive   packets   */

	char device[32]; /*   ethernet   device   name   */
	struct sockaddr_in *sin_ptr;

	memset(device, 0, 32);
	strcpy(device, pEthName);

	if ((fd_arp = socket(AF_INET, SOCK_PACKET, htons(0x0806))) < 0)
	{
    	return -1;
	}

	strcpy(ifr.ifr_name, device);
	/*   ifr.ifr_addr.sa_family   =   AF_INET;   */

	/*   get   ip   address   of   my   interface   */
	if (ioctl(fd_arp, SIOCGIFADDR, &ifr) < 0)
	{
    	ret = -1;
    	goto err_label;
	}
	sin_ptr = (struct sockaddr_in *) &ifr.ifr_addr;
	myself = sin_ptr->sin_addr;

	/*   get   network   mask   of   my   interface   */
	if (ioctl(fd_arp, SIOCGIFNETMASK, &ifr) < 0) {
    	ret = -1;
    	goto err_label;
	}
	sin_ptr = (struct sockaddr_in *) &ifr.ifr_addr;
	mymask = sin_ptr->sin_addr;

	/*   get   mac   address   of   the   interface   */
	if (ioctl(fd_arp, SIOCGIFHWADDR, &ifr) < 0)
	{
    	ret = -1;
    	goto err_label;
	}
	ptr = (u_char *) &ifr.ifr_ifru.ifru_hwaddr.sa_data[0];

err_label:
	close(fd_arp);

    return ret;
}

static int arpping(uint32_t yiaddr, uint32_t ip, uint8_t *pMac, char *pInterface)
{
	int timeout = 2;
	int optval = 1;
	int s; /* socket */
	int rv = 1; /* return value */
	struct sockaddr addr; /* for interface name */
	struct arpMsg1 arp;
	fd_set fdset;
	struct timeval tm;
	time_t prevTime;

	if ((s = socket(PF_PACKET, SOCK_PACKET, htons(ETH_P_ARP))) == -1) 
	{
		return -1;
	}

	if (setsockopt(s, SOL_SOCKET, SO_BROADCAST, &optval, sizeof(optval)) == -1) 
	{
		close(s);
		return -1;
	}

	//#define MAC_BCAST_ADDR		(uint8_t *) "\xff\xff\xff\xff\xff\xff"
	/* send arp request */
	memset(&arp, 0, sizeof(arp));
	memcpy(arp.h_dest, MAC_BCAST_ADDR, 6); /* MAC DA */
	memcpy(arp.h_source, pMac, 6); /* MAC SA */
	arp.h_proto = htons(ETH_P_ARP); /* protocol type (Ethernet) */
	arp.htype = htons(ARPHRD_ETHER); /* hardware type */
	arp.ptype = htons(ETH_P_IP); /* protocol type (ARP message) */
	arp.hlen = 6; /* hardware address length */
	arp.plen = 4; /* protocol address length */
	arp.operation = htons(ARPOP_REQUEST); /* ARP op code */
	memcpy(arp.sInaddr, &ip, sizeof(ip)); /* source IP address */
	memcpy(arp.sHaddr, pMac, 6); /* source hardware address */
	memcpy(arp.tInaddr, &yiaddr, sizeof(yiaddr)); /* target IP address */
	
	memset(&addr, 0, sizeof(addr));
	strcpy(addr.sa_data, pInterface);
	if (sendto(s, &arp, sizeof(arp), 0, &addr, sizeof(addr)) < 0)
		rv = 0;

	/* wait arp reply, and check it */
	tm.tv_usec = 0;
	prevTime = uptime();
	while (timeout > 0) 
	{
		FD_ZERO(&fdset);
		FD_SET(s, &fdset);
		tm.tv_sec = timeout;
		if (select(s + 1, &fdset, (fd_set *) NULL, (fd_set *) NULL, &tm) < 0) 
		{
			printf("Error on ARPING request: %s", strerror(errno));
			if (errno != EINTR){
				rv = 0;
			}	
		} 
		else if (FD_ISSET(s, &fdset)) 
		{
			if (recv(s, &arp, sizeof(arp), 0) < 0){
				rv = 0;
			}	
#if 1
			if(*((uint32_t *) arp.sInaddr) == yiaddr)
			{
				printf("\r\n op:%d,ip:%x,mac:%x:%x:%x:%x:%x:%x\r\n",ntohs(arp.operation),*(uint32_t *)arp.sInaddr,
					arp.sHaddr[0],arp.sHaddr[1],arp.sHaddr[2],arp.sHaddr[3],arp.sHaddr[4],arp.sHaddr[5]);
				printf("mac:%x:%x:%x:%x:%x:%x\n",pMac[0],pMac[1],pMac[2],pMac[3],pMac[4],pMac[5]);
			}
#endif
			if (arp.operation == htons(ARPOP_REPLY) && bcmp(arp.tHaddr, pMac, 6) == 0 && *((uint32_t *) arp.sInaddr) == yiaddr) 
			{
				printf("\r\n find the same ip.\r\n");
				rv = 0;
				break;
			}
			else if (arp.operation == htons(ARPOP_REQUEST) &&  *((uint32_t *) arp.sInaddr) == yiaddr &&(bcmp(arp.sHaddr, pMac, 6)))
			{
				rv = 0;
				break;
			}
		}
		timeout -= uptime() - prevTime;
		prevTime = uptime();
	}
	close(s);

	return rv;
}


int arpcheck(char *ethname)
{
	if (-1 == getIPMAC(ethname))
		return 0;
	
#if 0
	char myMAC[6];
	printf("myMAC:%s,myip:%s\r\n", myMAC, inet_ntoa(myself));
	printf("request   mac	%02x:%02x:%02x:%02x:%02x:%02x,%02x \r\n", *ptr, *(ptr
				+ 1), *(ptr + 2), *(ptr + 3), *(ptr + 4), *(ptr + 5), *ptr);
	printf("request   netmask	  %s   \r\n", inet_ntoa(mymask));
	printf("request   IP	 %s\r\n", inet_ntoa(myself));
	
#endif
		
	return !arpping(myself.s_addr,myself.s_addr,ptr,ethname);

}

