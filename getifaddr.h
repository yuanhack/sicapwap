#ifndef __YH_GETIFADDR_H__
#define __YH_GETIFADDR_H__

#include <netdb.h>     /* NI_NUMERICHOST */
#include <ifaddrs.h>   /* getaddrinfo */
#include <sys/types.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>


#ifdef __cplusplus
extern "C"
{
#endif


int getifaddr(const char *itf_name, int type, 
        char *ip, int len, char *errinfo, int errlen);

#ifdef __cplusplus
}
#endif
#endif /* __YH_GETIFADDR_H__ */
