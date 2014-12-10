#ifndef __YH__STASH_PROJECT_CAPWAP_SINO_SERVICE_H__
#define __YH__STASH_PROJECT_CAPWAP_SINO_SERVICE_H__

#include "list.h"
#include "yhepoll.h"

typedef struct {
    fd_event_handle fe;
    time_t          time;
    struct list     node;
    char            buf[128]; // recv buffer: packet_stat
    int             len;
} wtpc;

int wtps = 0;

#endif /* __YH__STASH_PROJECT_CAPWAP_SINO_SERVICE_H__ */
