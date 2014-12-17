#include <stdio.h> 
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define PORT	7767
#define BUFSIZE 1024
#define CONFIG_FILE "ac.address"

typedef struct {
	int flag;//7767
	int payload_len;
	//char *payload;
}dhcp_resp_t;

void daemond(int argc, char **argv);
int get_ac_address(char *pri_str, char *sec_str);

int main(int argc, char **argv) 
{
	int sock;
	int num;
	struct sockaddr_in server;
	struct sockaddr_in client;
	char recv_buf[BUFSIZE];
	char send_buf[BUFSIZE] = {0};
	int pri_str_len;
	int sec_str_len;
	int send_len;
	char pri_str[128] = {0};
	char sec_str[128] = {0};

	if (get_ac_address(pri_str, sec_str) < 0) {
		fprintf(stderr, "Can't get from %s get ac address.\n", CONFIG_FILE);
		exit(1);
	}

	pri_str_len = strlen(pri_str);
	sec_str_len = strlen(sec_str);

	fprintf(stderr, "primary ac address: %s len:%d\n", pri_str, pri_str_len);
	fprintf(stderr, "secondary ac address: %s len:%d\n", sec_str, sec_str_len);

	daemond(argc, argv);

	if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
		perror("Creating socket failed!");
		exit(1);
	}

	memset(&server, 0, sizeof(server));
	server.sin_family = AF_INET;
	server.sin_port = htons(PORT);
	server.sin_addr.s_addr = htonl(INADDR_ANY);

	if (bind(sock, (struct sockaddr *)&server, sizeof(struct sockaddr)) == -1) {
		perror("Bind error");
		exit(1);
	}
	
	int sin_size = sizeof(struct sockaddr_in);
	while(1) {
		num = recvfrom(sock, recv_buf, BUFSIZE, 0, (struct sockaddr *)&client, &sin_size);
		if (num <= 0) {
			usleep(2000);
		}

		recv_buf[num] = '\0';
		fprintf(stderr, "recv_buf = %s.", recv_buf);

		if (strcmp(recv_buf, "who has the AC address?") == 0) {
			fprintf(stderr, "strcmp return 0\n");
			//strcpy(str, "172.10.10.98,172.10.10.99");
			dhcp_resp_t msg;
			msg.flag = htonl(7767);
			msg.payload_len = htonl(pri_str_len + sec_str_len + 1);
			send_len = 8 + pri_str_len + sec_str_len + 1;
			memcpy(send_buf, &msg, 8);
			memcpy(send_buf + 8, pri_str, pri_str_len);
			memcpy(send_buf + 8 + pri_str_len, ",", 1);
			memcpy(send_buf + 8 + pri_str_len + 1, sec_str, sec_str_len);
			fprintf(stderr, "sendbuf = %s\n", send_buf + 8);
			if (sendto(sock, send_buf, send_len, 0, (struct sockaddr *)&client, sin_size) != send_len) {
				continue;				
			}
			fprintf(stderr, "sendto over.\n");
		}
	}

	close(sock);
}

void daemond(int argc, char **argv)
{
	pid_t pid;

	if (argc <= 1)
		printf("Usage: WTP working_path\n");

	if ((pid = fork()) < 0)
		exit(1);
	else if (pid != 0)
		exit(0);
	else {
		setsid();
		fclose(stdout);
		if (chdir(argv[1]) != 0)
			exit(1);
	}
}

int get_ac_address(char *pri_str, char *sec_str)
{
	FILE *fp = NULL;
	char *ptr = NULL;
	int len = 0;
	char buf[256] = {0};
	
	if ((fp = fopen(CONFIG_FILE, "rb")) == NULL) {
		fprintf(stderr, "fopen error!");
		return -1;
	}

	while(fgets(buf, 256, fp) != NULL) {
		len = strlen(buf);
		buf[len - 1] = '\0';
		fprintf(stderr, "%s", buf);
		if ((ptr = strstr(buf, "primary")) != NULL) {
			fprintf(stderr, "ptr = %s\n", ptr);
			strcpy(pri_str, ptr + 8);//strlen("primary")
		}
		if ((ptr = strstr(buf, "secondary")) != NULL) {
			fprintf(stderr, "ptr = %s\n", ptr);
			strcpy(sec_str, ptr + 10);
		}
	}

	fclose(fp);
	return 0;
}
