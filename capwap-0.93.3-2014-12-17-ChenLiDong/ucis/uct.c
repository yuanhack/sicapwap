#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <unistd.h>

#include "uctLib.h"

void printWTPList(struct WTPInfo *wtpList, int nWTPs);
void printVersionHeader();
void printVersionInfo(struct version_info v_info, int wtpId, struct WTPInfo *wtpList);
void printVersionFooter();
void do_version_cmd(int acserver, char *wtpIds, char *wtpNames);
void do_update_cmd(int acserver, char *wtpIds, char *wtpNames, char *cup_path);
void do_cancel_cmd(int acserver, char *wtpIds, char *wtpNames);

void do_uci_modify_config(char cmd, int acserver, char *wtpIds, char *wtpNames, char *msg);

int get_cmd_id(char *cmd);
int get_uci_cmd_id(char *cmd);
char *WTP_id2name(int id);
void usage(char *name);

#define ACSERVER_ADDRESS "127.0.0.1"
#define ACSERVER_PORT	1235

/* Commands */
#define CMD_NUM 4
typedef struct { char id; const char *name; } cmd_t;

enum {NO_CMD, WTPS_CMD, VERSION_CMD, UPDATE_CMD, CANCEL_CMD};

cmd_t CMDs[] = {
	{WTPS_CMD, "wtps"},
	{VERSION_CMD, "version"},
	{UPDATE_CMD, "update"},
	{CANCEL_CMD, "cancel"},
	{NO_CMD, ""}
};

/* UCI Commands */
#define UCI_CMD_NUM 20
typedef struct { char id; const char *name; } uci_cmd_t;

enum {
	NO_UCI_CMD,
	UCI_CMD_WTPS,
	/* section cmds */
	UCI_CMD_GET,
	UCI_CMD_SET,
	UCI_CMD_ADD_LIST,
	UCI_CMD_DEL_LIST,
	UCI_CMD_DEL,
	UCI_CMD_RENAME,
	UCI_CMD_REVERT,
	UCI_CMD_REORDER,
	/* package cmds */
	UCI_CMD_SHOW,
	UCI_CMD_CHANGES,
	UCI_CMD_EXPORT,
	UCI_CMD_COMMIT,
	/* other cmds */
	UCI_CMD_RSP,
	UCI_CMD_ADD,
	UCI_CMD_IMPORT,
	UCI_CMD_HELP,
	/* add cmds */
	IWINFO_CMD_GET_STATION_LIST,
	IWINFO_CMD_DEL_STATION,
};

uci_cmd_t UCI_CMDs[] = {
	{UCI_CMD_WTPS, "wtps"},
	/* section cmds */
	{UCI_CMD_GET, "get"},
	{UCI_CMD_SET, "set"},
	{UCI_CMD_ADD_LIST, "add_list"},
	{UCI_CMD_DEL_LIST, "del_list"},
	{UCI_CMD_DEL, "del"},
	{UCI_CMD_RENAME, "rename"},
	{UCI_CMD_REVERT, "revert"},
	{UCI_CMD_REORDER, "reorder"},
	/* package cmds */
	{UCI_CMD_SHOW, "show"},
	{UCI_CMD_CHANGES, "changes"},
	{UCI_CMD_EXPORT, "export"},
	{UCI_CMD_COMMIT, "commit"},
	/* other cmds */
	{UCI_CMD_RSP, "rsp"},
	{UCI_CMD_ADD, "add"},
	{UCI_CMD_IMPORT, "import"},
	{UCI_CMD_HELP, "help"},
	/* add cmds */
	{IWINFO_CMD_GET_STATION_LIST, "get_station_list"},
	{IWINFO_CMD_DEL_STATION, "del_station"},
	{NO_UCI_CMD, ""}
};

/* Global WTP List */
struct WTPInfo *wtpList;
int nWTPs;
char gwtpIds[256];

/*test*/
int gTest = 20;

int main(int argc, char *argv[])
{
	int acserver, wtpId, cmd_id, cmd;
	void *cup;
	struct version_info update_v; 
    char *command = NULL, *cup_path = NULL;
    char *wtpIds = NULL, *wtpNames = NULL;

    char *command_uci = NULL;
    char *package_uci = NULL, *section_uci = NULL;
    char *option_uci = NULL, *value_uci = NULL;
    char *list_uci = NULL;

    char *acserver_address = ACSERVER_ADDRESS;
	int acserver_port = ACSERVER_PORT;;
	int index;
    int c;
    unsigned char mac_addr[7] = {0};
    
    opterr = 0;
    
	/* Parse options */
    while ((c = getopt (argc, argv, "ha:p:w:c:f:x:t:s:o:l:v:n:")) != -1)
        switch (c)
        {
		case 'a':
			acserver_address = optarg;
			break;
		case 'p':
			acserver_port = atoi(optarg);
			break;
        case 'w':
            wtpIds = optarg;
	    strcpy(gwtpIds, wtpIds);
            break;
        case 'n':
        	wtpNames = optarg;
        	break;
        case 'c':
            command = optarg;
            break;
	case 'x':
	    command_uci = optarg;
	    break;
	case 't':
	    package_uci = optarg;
	    break;
	case 's':
	    section_uci = optarg;
	    break;
	case 'o':
	    option_uci = optarg;
	    break;
	case 'l':
	    list_uci = optarg;
	    break;
	case 'v':
	    value_uci = optarg;
	    break;
        case 'f':
        	cup_path = optarg;
        	break;
        case 'h':
        	usage(argv[0]);
        	break;	
        case '?':
            if (optopt == 'w' || optopt == 'c' || optopt == 'f' || optopt == 'n' || \
			optopt == 'x' || optopt == 't' || optopt == 'l' || \
			optopt == 's' || optopt == 'o' || optopt == 'v')
           		fprintf (stderr, "Option -%c requires an argument.\n", optopt);
            else if (isprint (optopt))
            	fprintf (stderr, "Unknown option `-%c'.\n", optopt);
            else
           		fprintf (stderr,
                    "Unknown option character `\\x%x'.\n",
                    optopt);
            exit(EXIT_FAILURE);
        default:
    		usage(argv[0]);
            abort();

		}
     
    /* Check arguments */ 
	/*if (command == NULL) {
		fprintf(stderr, "No command specified!\n");
		exit(EXIT_FAILURE);
	}*/
    /* Check arguments */ 
	if (command_uci == NULL) {
		fprintf(stderr, "No command specified!\n");
		exit(EXIT_FAILURE);
	}
	
	/*if ((cmd_id = get_cmd_id(command)) == NO_CMD) {
		fprintf(stderr, "Wrong command specified!");
	}*/
	fprintf(stderr, "*_* Debug get command = %s\n", command_uci);

	if ((cmd_id = get_uci_cmd_id(command_uci)) == NO_UCI_CMD) {
		fprintf(stderr, "Wrong command specified!\n");
	}

	fprintf(stderr, "*_* Debug get cmd = %d\n", cmd_id);

	/* Connect to server and get WTPs list */
	acserver = ACServerConnect(acserver_address, acserver_port);
	wtpList = ACServerWTPList(acserver, &nWTPs);
	fprintf(stderr, "ac = %d option = %s.\n", acserver, option_uci);

	/* Execute command */
	/*switch(cmd_id) {
		case WTPS_CMD:
			printWTPList(wtpList, nWTPs);
			break;
		case VERSION_CMD:
			do_version_cmd(acserver, gwtpIds, wtpNames);
			break;
		case UPDATE_CMD:
			do_update_cmd(acserver, gwtpIds, wtpNames, cup_path);
			break;
		case CANCEL_CMD:
			do_cancel_cmd(acserver, gwtpIds, wtpNames);
			break;
	}*/

	/* Execute typical */
	switch(cmd_id) {
		case UCI_CMD_WTPS:
			printWTPList(wtpList, nWTPs);
			break;
		case UCI_CMD_GET:
		case UCI_CMD_SET:
		case UCI_CMD_DEL:
		case UCI_CMD_COMMIT:
		case UCI_CMD_RSP:
		case UCI_CMD_SHOW:
		case IWINFO_CMD_GET_STATION_LIST:
		case IWINFO_CMD_DEL_STATION:
			fprintf(stderr, "UCI_CMD_SET_LIST!\n");
			do_uci_modify_config(cmd_id, acserver, gwtpIds, wtpNames, option_uci);
			break;
		//case IWINFO_CMD_DEL_STATION:
			/*mac_to_unchar(option_uci, mac_addr);
			fprintf(stderr, "mac:[%02x%02x%02x%02x%02x%02x]", \
						mac_addr[0],
						mac_addr[1],
						mac_addr[2],
						mac_addr[3],
						mac_addr[4],
						mac_addr[5]
						);
			do_uci_modify_config(cmd_id, acserver, gwtpIds, wtpNames, mac_addr);
			//do_uci_get_config();
			fprintf(stderr, "This UCI_CMD unfinished!\n");
			break;*/
	}
	
	freeWTPList(wtpList, nWTPs);
	ACServerDisconnect(acserver);

	exit(EXIT_SUCCESS);
}

int sanitize_wtp_list(int *work_list, int n)
{
	int i, j, z, new_size = n;

	/* Delete unknown wtp ids */
	for(i = 0; i < new_size; i++) {
		if (WTP_id2name(work_list[i]) == NULL) {
			for (j = i; j < new_size - 1; j++) {
				work_list[j] = work_list[j+1];
			}
			i--;
			new_size--;
		}
	}
	
	/* Delete duplicates */
	for(i = 0; i < new_size; i++) {
		for (j = i + 1; j < new_size; j++) {
			if (work_list[i] == work_list[j]) {
				for (z = j; z < new_size - 1; z++) {
		        	work_list[z] = work_list[z+1];
		        }
				j--;
				new_size--;
			}
		} 
	}

	return new_size;
}

int *all_WTPs()
{
	int *ret, i;
	
	ret = malloc(nWTPs*sizeof(int));
	
	for (i = 0; i < nWTPs; i++) {
		ret[i] = wtpList[i].wtpId;
	}
	
	return ret;
}

int count_tokens(char *str1, char *str2)
{
	int n = 1;
	char *ptr;

	if (str1 != NULL)
		ptr = str1;
	else if (str2 != NULL) 
		ptr = str2;
	else
		return 0;
	
	while (*ptr != '\0') {
		if (*ptr == ',' && *(ptr + 1) != ',' && *(ptr + 1) != '\0') 
			n++;
		ptr++;
	}
	return n;
}

int *get_id_list(char *wtpIds, char *wtpNames, int *n)
{
	char *token = NULL, *ptr = NULL;
	int *ret = NULL;
	int i;
	
	*n = count_tokens(wtpIds, wtpNames); 
	
	if (*n <= 0) return NULL;
	
	/* allocate memory */
	ret = malloc(*n*sizeof(int));
	if (ret == NULL) {
		perror("malloc error!");
		return NULL;
	}
	
	if (wtpIds != NULL) {
		/* read ids */
		token = (char*)strtok(wtpIds, ",");
		if(token)  {
			ret[0] = atoi(token);

			if (ret[0] == -1) 
				return all_WTPs();

			for (i = 1; i < *n; i++)
				ret[i] = atoi( (const char*)strtok(NULL, ",") );
		}
		
	} else {	
		/* read names and convert into ids */
		for (i = 0; i < *n; i++) {
			int id;

			if (i == 0) {
				token = (char*)strtok(wtpNames, ",");
				if (strcmp(token, "all") == 0)
					return all_WTPs();
				
			} else {
				token = (char*)strtok(NULL, ",");
			}
			
			if ((id = WTP_name2id(token)) == -1) {
				fprintf(stderr, "%s: specified WTP does not exits\n", token);
			}

			ret[i] = id;
		}
	};
	
	/* remove duplicated and unknown WTP ids */
	*n = sanitize_wtp_list(ret, *n);

	return ret;
}

void do_version_cmd(int acserver, char *wtpIds, char *wtpNames)
{
	int *wtps, n, i;
	struct version_info v_info;

	/* WTP work list */
	wtps = get_id_list(wtpIds, wtpNames, &n);
	
	if (wtps == NULL) {
		fprintf(stderr, "Either a list of wtp ids or wtp names must be specified!\n");
		return;
	}

	printVersionHeader();	
	for (i = 0; i < n; i++) {
		WUMGetWTPVersion(acserver, wtps[i], &v_info);
		printVersionInfo(v_info, wtps[i], wtpList);	
	}
	printVersionFooter();
}

void do_update_cmd(int acserver, char *wtpIds, char *wtpNames, char *cup_path)
{
	int *wtps, n, i, ret;
	struct version_info update_v;
	void *cup;

	if (cup_path == NULL) {
		fprintf(stderr, "In order to execute an update, an update package must be specified! (-f pathname)\n");
		return;
	}

	/* WTP work list */
	wtps = get_id_list(wtpIds, wtpNames, &n);
	if (wtps == NULL) {
		fprintf(stderr, "Either a list of wtp ids or wtp names must be specified!\n");
		return;
	}

	int fd = open(cup_path, O_RDONLY);
	if (fd < 0) {
		perror("open error");
		return;
	}
	
	if (WUMReadCupVersion(cup_path, &update_v)) {
		return;
	}
	
	cup = mmap(NULL, update_v.size, PROT_READ, MAP_SHARED , fd, 0);
	if (cup == NULL) {
		perror("mmap error");
		return;
	}

	printf("*--------*--------------------------------*------------*\n");
	printf("| %-6s | %-30s | %-10s |\n", "WTP Id", "WTp Name", "Result");
	printf("*--------*--------------------------------*------------*\n");
	for (i = 0; i < n; i++) {
		ret = WUMUpdate(acserver, i, cup, update_v);
		printf("| %-6d | %-30s | %-10s |\n", wtpList[i].wtpId, wtpList[i].name, (ret == 0) ? "SUCCESS" : "FAILURE");
	}
	printf("*--------*--------------------------------*------------*\n");

	munmap(cup, update_v.size);
	close(fd);
}

void do_uci_modify_config(char cmd, int acserver, char *wtpIds, char *wtpNames, char *msg)
{
	int *wtps, n, i, ret;
	int size = strlen(msg);
	//struct cmd_info;
	
	/*if (msg == NULL) {
		fprintf(stderr, "In order to set configure, package section option value must be specified! ( -x command_uci -t package -s section -o option/-l list -v value)\n");
		return;
	}*/

	/* WTP work list */
	fprintf(stderr, ".......%s.....\n", msg);
	wtps = get_id_list(wtpIds, wtpNames, &n);
	if (wtps == NULL) {
		fprintf(stderr, "Either a list of wtp ids or wtp names must be specified!\n");
		return;
	}

	for(i = 0; i < n; i++) {
		UCIModifyCmd(cmd, acserver, wtps[i], (void *)msg, size);
	}
}

void do_cancel_cmd(int acserver, char *wtpIds, char *wtpNames)
{
	int *wtps, n, i;
	struct version_info v_info;

	/* WTP work list */
	wtps = get_id_list(wtpIds, wtpNames, &n);
	
	if (wtps == NULL) {
		fprintf(stderr, "Either a list of wtp ids or wtp names must be specified!\n");
		return;
	}

	for (i = 0; i < n; i++) {
		if (WUMSendCancelRequest(acserver, wtps[i])) {
			fprintf(stderr, "Error while handling cancel request to WTP %d.\n", wtps[i]);
		} else {
			printf("Cancel request sent for WTP %d\n", wtps[i]);
		}
	}
}

int WTP_name2id(char *name)
{
	int i;

	for (i = 0; i < nWTPs; i++) {
		if (strcmp(name, wtpList[i].name) == 0) {
			/* found WTP! */
			return wtpList[i].wtpId;
		}
	}

	return -1;
}

char *WTP_id2name(int id)
{
	int i;

	for(i = 0; i < nWTPs; i++) {
		if (wtpList[i].wtpId == id) return wtpList[i].name;
	}

	return NULL;
}

int get_cmd_id(char *cmd)
{
	int i;
	for (i = 0; i < CMD_NUM; i++) {
		if (strcmp(CMDs[i].name, cmd) == 0)
			break;
	}
	
	return CMDs[i].id;
}

int get_uci_cmd_id(char *cmd)
{
	int i;
	for(i = 0; i < UCI_CMD_NUM; i++) {
		if (strcmp(UCI_CMDs[i].name, cmd) == 0)
			break;
	}

	return UCI_CMDs[i].id;
}

void printWTPList(struct WTPInfo *wtpList, int nWTPs)
{
	int i;
	
	printf("*-------*--------------------------------*\n");
	printf("| %5s | %-30s |\n", "WTPId", "WTPName");
	printf("*-------*--------------------------------*\n");
	printf("| %5s | %-30s |\n", "-1", "all");

	if (wtpList != NULL) {

		for (i = 0; i < nWTPs; i++) {
			printf("| %5d | %-30s |\n", wtpList[i].wtpId, wtpList[i].name);
		}
	}else{
		printf("wtpList == NULL\n");
	}

	printf("*-------*--------------------------------*\n");
}

void printVersionHeader()
{
	printf("*-------*--------------------------------*-----------------*\n");
	printf("| %5s | %-30s | %-15s |\n", "WTPId", "WTPName", "Version");
	printf("*-------*--------------------------------*-----------------*\n");
}

void printVersionInfo(struct version_info v_info, int wtpId, struct WTPInfo *wtpList)
{
	char buf[15];
	snprintf(buf, 15, "%d.%d.%d", v_info.major, v_info.minor, v_info.revision);
	printf("| %5d | %-30s | %-15s |\n", wtpList[wtpId].wtpId, wtpList[wtpId].name, buf);
}

void printVersionFooter()
{
	printf("*-------*--------------------------------*-----------------*\n");
}

int mac_to_unchar(char *str, char *mac)
{
	int i;
	char *ptr, *e;
	fprintf(stderr, "mac_to_unchar str--->%s\n", mac);

	if ((mac == NULL) || (str == NULL)) {
		return -1;
	}

	ptr = (char *) str;
	for(i = 0; i < 6; i++) {
		mac[i] = ptr ? strtoul(ptr, &e, 16) : 0;
		if (ptr)
			ptr = (*e) ? e + 1 : e;
	}

	return 0;
}

void usage(char *name)
{
	printf("%s -c command [-w id1,...] [-n name1,...] [-f cup_file] [-a address] [-p port]\n", name);
	printf("\nAvailable commands:\n");
	printf("   wtps: list of active wtps.\n");	
	printf("version: version of the specified list of wtps (use -w or -n).\n");	
	printf(" update: sends a cup (specified with -f) to the specified list of wtps (use -w or -n).\n");		
	printf(" cancel: cancel a pending update on the desired wtps (use -w or -n).\n");			
}
