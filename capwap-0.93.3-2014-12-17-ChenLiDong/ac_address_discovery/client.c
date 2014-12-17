#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <arpa/inet.h>
int main() 
{
	int sock;

	int nb = 0;
	const int opt = 1;
	struct sockaddr_in addr_to;
	char send_buf[128] = {0};
	char recv_buf[128] = {0};

	if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
		fprintf(stderr, "create socket error!\n");
		return -1;
	}

	if ((nb = setsockopt(sock, SOL_SOCKET, SO_BROADCAST, (char *)&opt, sizeof(opt))) == -1) {
		fprintf(stderr, "setsockopt error!\n");
	}

	bzero(&addr_to, sizeof(addr_to));
	addr_to.sin_family = AF_INET;
	addr_to.sin_addr.s_addr = htonl(INADDR_BROADCAST);
	addr_to.sin_port = htons(7767);

	strcpy(send_buf, "who has the AC address?");
	sendto(sock, send_buf, strlen(send_buf), 0, (struct sockaddr *)&addr_to, sizeof(addr_to));

	recv(sock, recv_buf, 128, 0);

	fprintf(stderr, "recv_buf:%s\n", recv_buf);

	return 0;
}
