#include "domain_parse.h"

int domain_parse(const char *domain, char *buff, int len, char *errinfo, int errlen)
{
    struct addrinfo *answer, hint, *curr;
    bzero(&hint, sizeof(hint));
    hint.ai_family = AF_INET;
    hint.ai_socktype = SOCK_STREAM;

    memset(errinfo, 0, errlen);

    int ret = getaddrinfo(domain, NULL, &hint, &answer);
    if (ret != 0) {
        snprintf(errinfo, errlen, "getaddrinfo: %s", gai_strerror(ret));
        return -1;
    }

    for (curr = answer; curr != NULL; curr = curr->ai_next) {
        if (!inet_ntop(AF_INET, &(((struct sockaddr_in *)(curr->ai_addr))->sin_addr), buff, len)) {
            snprintf(errinfo, errlen, "inet_ntop error: %s", strerror(errno));
            return -1;
        }
        printf("%s\n", buff);
        break;
    }
    freeaddrinfo(answer);
    return 0;
}

/*
int main(int argc, char **argv)
{
  if (argc != 2) { printf("usage: %s domain\n", argv[0]); return 1; }
  char ip[256];
  char errinfo[256];
  if (domain_parse(argv[1], ip, sizeof(ip), errinfo, sizeof(errinfo)) < 0) {
      printf("%s\n", errinfo);
      return 1;
  }
  return 0;
} 
 */ 
