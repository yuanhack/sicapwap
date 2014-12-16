#include "getifaddr.h"

// man getifaddrs 下摘录封装
int getifaddr(const char *itf_name, int type, char *ip, int len,
        char *errinfo, int errlen)
{
    struct ifaddrs *ifaddr, *ifa;
    int family, s;

    memset(ip, 0, len);
    memset(errinfo, 0, errlen);

    if (type != AF_INET && type != AF_INET6) {
        snprintf(errinfo, errlen, "getifaddr() failed: Unknown type %d, "
                "Valid AF_INET or AF_INET6", type);
        return -1;
    }

    if (getifaddrs(&ifaddr) == -1) {
        snprintf(errinfo, errlen, "getifaddrs() failed: %s", strerror(errno));
        return -1;
    }

    /* Walk through linked list, maintaining head pointer so we
       can free list later */

    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr == NULL) 
            continue;

        family = ifa->ifa_addr->sa_family;

        /* Display interface name and family (including symbolic
           form of the latter for the common families) */

        /*printf("%s  address family: %d%s\n", ifa->ifa_name, family,
                (family == AF_PACKET) ? " (AF_PACKET)" :
                (family == AF_INET) ?   " (AF_INET)" :
                (family == AF_INET6) ?  " (AF_INET6)" : ""); */

        /* For an AF_INET* interface address, display the address */

        if (family != type || strcmp(ifa->ifa_name, itf_name)) 
            continue;

        //if (family == AF_INET || family == AF_INET6) {
        if (family == type) {
            s = getnameinfo(ifa->ifa_addr,
                    (family == AF_INET) ? sizeof(struct sockaddr_in) :
                    sizeof(struct sockaddr_in6),
                    ip, len, NULL, 0, NI_NUMERICHOST);
            if (s != 0) {
                snprintf(errinfo, errlen, "getnameinfo() gai_strerror: %s", gai_strerror(s));
                goto err_free_out;
            }
            //printf("\taddress: <%s>\n", ip);
            break;  // already find
        }
    }
    if (ifa == NULL) {
        snprintf(errinfo ,errlen, "getifaddr() failed: %s: No such interface or no address", itf_name);
        goto err_free_out;
    }

    freeifaddrs(ifaddr);
    return 0;

err_free_out:
    freeifaddrs(ifaddr);
    return -1;
}

