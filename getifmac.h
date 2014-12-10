#ifndef __YH_GETIFMAC_H__
#define __YH_GETIFMAC_H__

#include <sys/socket.h>
#include <sys/ioctl.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>

#include <net/if.h>



#ifdef __cplusplus
extern "C"
{
#endif

#pragma pack(push)
#pragma pack(1)

#pragma pack(pop)


int get_mac(const char *itf_name, 
        char *macbuf, int len, 
        char *errinfo, int errlen);

#ifdef __cplusplus
}
#endif
#endif /* __YH_GETIFMAC_H__ */
