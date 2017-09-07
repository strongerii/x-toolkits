#ifndef _XLIB_STRUCT_DEFINE_H_
#define _XLIB_STRUCT_DEFINE_H_

#include "xlib_type.h"

typedef struct {
	u16 dhcpEnable;
	u16 phyaddrEnable;
	u8 szEthName[XLIB_MAX_NETWORK_STRLEN];
	u8 szIP[XLIB_MAX_NETWORK_STRLEN];
	u8 szSubMsk[XLIB_MAX_NETWORK_STRLEN];
	u8 szGateWay[XLIB_MAX_NETWORK_STRLEN];
	u8 szPhyAddr[XLIB_MAX_NETWORK_STRLEN];
	u8 szFirstDns[XLIB_MAX_NETWORK_STRLEN];
	u8 szSecDns[XLIB_MAX_NETWORK_STRLEN];
}XLIB_NETWORKINFO;


#endif //_XLIB_STRUCT_DEFINE_H_