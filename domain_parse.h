#ifndef __YH__STASH_SRC_DNS_DOMAIN_PARSE_H__
#define __YH__STASH_SRC_DNS_DOMAIN_PARSE_H__

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>


#ifdef __cplusplus
extern "C"
{
#endif


int domain_parse(const char *domain, char *buff, int len, 
        char *errinfo, int errlen);


#ifdef __cplusplus
}
#endif
#endif /* __YH__STASH_SRC_DNS_DOMAIN_PARSE_H__ */
