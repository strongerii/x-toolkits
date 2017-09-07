#ifndef _NET_INFO_H_
#define _NET_INFO_H_

#include "xlib_struct.h"

//set dhcp effect
int set_localdhcp();

//get local ip address
int get_localip(const char *ifname,char *szNetIP, int len);
//set local ip
int set_localip(const char *ifname,const char *szNetIP);

//get local mac
int get_localmac(const char *ifname,char *szNetMac, int len);
//set local mac
int set_localmac(const char *ifname,const char *szNetMac);

//get sub net mask
int get_netmask(const char *ifname,char *szNetMask, int len);
//set sub net mask
int set_netmask(const char *ifname,const char *szNetMask);

//get default gateway
int get_gateway(char *szGateWay, int len);
//set default gateway
int set_gateway(const char *szGateWay);

//get dns address
int get_dnsaddress(char *pPrimary, const int nPrimLen, char *pSecond, const int nSecLen);
//set dns address
int set_dnsaddress(const char *pPrimary, const char *pSecond);

int save_dns_info(const char *pPrimary, const char *pSecond);
int save_net_info(XLIB_NETWORKINFO *pNetInfo);

int get_networkinfo(XLIB_NETWORKINFO *pNetInfo);
int set_networkinfo(XLIB_NETWORKINFO *pNetInfo);

int set_network_manual(XLIB_NETWORKINFO *pNetInfo,XLIB_NETWORKINFO *pCurNetInfo);
int set_network_dhcp(XLIB_NETWORKINFO *pNetInfo,XLIB_NETWORKINFO *pCurNetInfo);

int resume_staticip(XLIB_NETWORKINFO *pNetInfo);

#endif