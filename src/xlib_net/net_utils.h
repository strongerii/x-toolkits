#ifndef _NET_UTILS_H_
#define _NET_UTILS_H_

#include "xlib_type.h"
//===================== smtp info===========================
typedef struct{
	u8 szServer[XLIB_MAX_STRLEN];
	u8 szTitle[XLIB_MAX_STRLEN];
	u8 szSender[XLIB_MAX_VARNAME_LEN];
	u8 szRecver[XLIB_MAX_VARNAME_LEN];
	u8 szUserNm[XLIB_MAX_VARNAME_LEN];
	u8 szUserPass[XLIB_MAX_VARNAME_LEN];
	u32 uPort;
	u32 uSSLEnable;
}NSV_SMTPINFO;


//int send_to_ftp(const char *szlPath);

int send_to_email(const char *szlPath);

//int set_ddns();


int mii_diag(char *ethname);

int arpcheck(char *ethname);

#endif //_NET_UTILS_H_