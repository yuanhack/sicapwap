#ifndef __YH_YHPATH_H__
#define __YH_YHPATH_H__

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <limits.h>

#ifdef __cplusplus
extern "C"
{
#endif

#pragma pack(push)
#pragma pack(1)
#pragma pack(pop)


int propath(char *buff, int len, char *errinfo, int errlen);


#ifdef __cplusplus
}
#endif
#endif /* __YH_YHPATH_H__ */
