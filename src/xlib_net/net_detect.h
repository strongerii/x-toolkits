

#ifndef _DEF_NET_DETECT_H_
#define _DEF_NET_DETECT_H_

#define RTACTION_ADD 1   /* add action */
#define RTACTION_DEL 2   /* del action */
int inet_setroute(int action, char **args);

int NetIsLink(const char *iface);
int NetIsLink2(const char *iface);
int NetCheckPhyIsExist(const char *iface);
int NetIsOk(const char *iface, const char *address, long *pSpendMS, int *pTTL);

#endif //_DEF_NET_DETECT_H_
