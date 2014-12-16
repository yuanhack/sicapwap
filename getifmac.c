#include "getifmac.h"

int getifmac(const char *itf_name, char *macbuf, int len, char *errinfo, int errlen)
{
    struct ifreq ifreq;
    int sock;
    
    memset(macbuf, 0, len);
    memset(errinfo,0, errlen);

    if ((sock = socket(AF_INET,SOCK_STREAM,0))<0) {
        snprintf(errinfo, errlen, "socket error: %s", strerror(errno));
        return -1;
    }
    strncpy((char*)ifreq.ifr_name, itf_name, IFNAMSIZ);
    if (ioctl(sock,SIOCGIFHWADDR,&ifreq)<0) {
        snprintf(errinfo, errlen, "ioctl error: %s", strerror(errno));
        close(sock);
        return -1;
    }
    snprintf(macbuf, len, "%02x:%02x:%02x:%02x:%02x:%02x",
            (unsigned char)ifreq.ifr_hwaddr.sa_data[0],
            (unsigned char)ifreq.ifr_hwaddr.sa_data[1],
            (unsigned char)ifreq.ifr_hwaddr.sa_data[2],
            (unsigned char)ifreq.ifr_hwaddr.sa_data[3],
            (unsigned char)ifreq.ifr_hwaddr.sa_data[4],
            (unsigned char)ifreq.ifr_hwaddr.sa_data[5]);
    close(sock);
    return 0;
}
