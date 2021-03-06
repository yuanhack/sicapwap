/************************************************************************************************
 * Copyright (c) 2006-2009 Laboratorio di Sistemi di Elaborazione e Bioingegneria Informatica	*
 *                          Universita' Campus BioMedico - Italy								*
 *																								*
 * This program is free software; you can redistribute it and/or modify it under the terms		*
 * of the GNU General Public License as published by the Free Software Foundation; either		*
 * version 2 of the License, or (at your option) any later version.								*
 *																								*
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY				*
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A				*
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.						*
 *																								*
 * You should have received a copy of the GNU General Public License along with this			*
 * program; if not, write to the:																*
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330, Boston,							*
 * MA  02111-1307, USA.																			*
 *																								*
 * -------------------------------------------------------------------------------------------- *
 * Project:  Capwap																				*
 *																								*
 * Authors : Ludovico Rossi (ludo@bluepixysw.com)												*  
 *           Del Moro Andrea (andrea_delmoro@libero.it)											*
 *           Giovannini Federica (giovannini.federica@gmail.com)								*
 *           Massimo Vellucci (m.vellucci@unicampus.it)											*
 *           Mauro Bisson (mauro.bis@gmail.com)													*
 *	         Antonio Davoli (antonio.davoli@gmail.com)											*
 ************************************************************************************************/

#include "sino_comm.h"
#include "getifmac.h"
#include "getifaddr.h"
#include "pthread_stack.h"

#include "CWWTP.h"

#ifdef DMALLOC
#include "../dmalloc-5.5.0/dmalloc.h"
#endif


CW_THREAD_RETURN_TYPE CWWTPReceiveFrame(void *arg);
CW_THREAD_RETURN_TYPE CWWTPReceiveStats(void *arg);
CW_THREAD_RETURN_TYPE CWWTPReceiveFreqStats(void *arg);

int 	gEnabledLog;
int 	gMaxLogFileSize;
char 	gLogFileName[] = WTP_LOG_FILE_NAME;

/* addresses of ACs for Discovery */
char	**gCWACAddresses;
int	gCWACCount = 0;

int gIPv4StatusDuplicate = 0;
int gIPv6StatusDuplicate = 0;

char *gWTPLocation = NULL;
char *gWTPName = NULL;

/* if not NULL, jump Discovery and use this address for Joining */
char 		*gWTPForceACAddress = NULL;
CWAuthSecurity 	gWTPForceSecurity;

/* UDP network socket */
CWSocket 		gWTPSocket;
/* DTLS session vars */
CWSecurityContext	gWTPSecurityContext;
CWSecuritySession 	gWTPSession;

/* list used to pass frames from wireless interface to main thread */
CWSafeList 		gFrameList;

/* list used to pass CAPWAP packets from AC to main thread */
CWSafeList 		gPacketReceiveList;

/* used to synchronize access to the lists */
CWThreadCondition    gInterfaceWait;
CWThreadMutex 		gInterfaceMutex;

/* infos about the ACs to discover */
CWACDescriptor *gCWACList = NULL;
/* infos on the better AC we discovered so far */
CWACInfoValues *gACInfoPtr = NULL;

/* WTP statistics timer */
int gWTPStatisticsTimer = CW_STATISTIC_TIMER_DEFAULT;

WTPRebootStatisticsInfo gWTPRebootStatistics;
CWWTPRadiosInfo gRadiosInfo;

/* path MTU of the current session */
int gWTPPathMTU = 0;

int gWTPRetransmissionCount;

CWPendingRequestMessage gPendingRequestMsgs[MAX_PENDING_REQUEST_MSGS];	

CWBool WTPExitOnUpdateCommit = CW_FALSE;

/* 
 * Receive a message, that can be fragmented. This is useful not only for the Join State
 */
CWBool CWReceiveMessage(CWProtocolMessage *msgPtr) {
	CWList fragments = NULL;
	int readBytes;
	char buf[CW_BUFFER_SIZE];
	
	CW_REPEAT_FOREVER {
		CW_ZERO_MEMORY(buf, CW_BUFFER_SIZE);
#ifdef CW_NO_DTLS
		char *pkt_buffer = NULL;

		CWLockSafeList(gPacketReceiveList);

		while (CWGetCountElementFromSafeList(gPacketReceiveList) == 0)
			CWWaitElementFromSafeList(gPacketReceiveList);

		pkt_buffer = (char*)CWRemoveHeadElementFromSafeList(gPacketReceiveList, &readBytes);

		CWUnlockSafeList(gPacketReceiveList);

		CW_COPY_MEMORY(buf, pkt_buffer, readBytes);
		CW_FREE_OBJECT(pkt_buffer);
#else
		if(!CWSecurityReceive(gWTPSession, buf, CW_BUFFER_SIZE, &readBytes)) {return CW_FALSE;}
#endif
		CWBool dataFlag = CW_FALSE;
		if(!CWProtocolParseFragment(buf, readBytes, &fragments, msgPtr, &dataFlag)) {
			if(CWErrorGetLastErrorCode() == CW_ERROR_NEED_RESOURCE) { // we need at least one more fragment
				continue;
			} else { // error
				CWErrorCode error;
				error=CWErrorGetLastErrorCode();
				switch(error)
				{
					case CW_ERROR_SUCCESS: {CWDebugLog("ERROR: Success"); break;}
					case CW_ERROR_OUT_OF_MEMORY: {CWDebugLog("ERROR: Out of Memory"); break;}
					case CW_ERROR_WRONG_ARG: {CWDebugLog("ERROR: Wrong Argument"); break;}
					case CW_ERROR_INTERRUPTED: {CWDebugLog("ERROR: Interrupted"); break;}
					case CW_ERROR_NEED_RESOURCE: {CWDebugLog("ERROR: Need Resource"); break;}
					case CW_ERROR_COMUNICATING: {CWDebugLog("ERROR: Comunicating"); break;}
					case CW_ERROR_CREATING: {CWDebugLog("ERROR: Creating"); break;}
					case CW_ERROR_GENERAL: {CWDebugLog("ERROR: General"); break;}
					case CW_ERROR_OPERATION_ABORTED: {CWDebugLog("ERROR: Operation Aborted"); break;}
					case CW_ERROR_SENDING: {CWDebugLog("ERROR: Sending"); break;}
					case CW_ERROR_RECEIVING: {CWDebugLog("ERROR: Receiving"); break;}
					case CW_ERROR_INVALID_FORMAT: {CWDebugLog("ERROR: Invalid Format"); break;}
					case CW_ERROR_TIME_EXPIRED: {CWDebugLog("ERROR: Time Expired"); break;}
					case CW_ERROR_NONE: {CWDebugLog("ERROR: None"); break;}
				}
				CWDebugLog("~~~~~~");
				return CW_FALSE;
			}
		} else break; // the message is fully reassembled
	}
	
	return CW_TRUE;
}

CWBool CWWTPSendAcknowledgedPacket(int seqNum, 
				   CWList msgElemlist,
				   CWBool (assembleFunc)(CWProtocolMessage **, int *, int, int, CWList),
				   CWBool (parseFunc)(char*, int, int, void*), 
				   CWBool (saveFunc)(void*),
				   void *valuesPtr) {

	CWProtocolMessage *messages = NULL;
	CWProtocolMessage msg;
	int fragmentsNum = 0, i;

	struct timespec timewait;
	
	int gTimeToSleep = gCWRetransmitTimer;
	int gMaxTimeToSleep = CW_ECHO_INTERVAL_DEFAULT/2;

	msg.msg = NULL;
	
	if(!(assembleFunc(&messages, 
			  &fragmentsNum, 
			  gWTPPathMTU, 
			  seqNum, 
			  msgElemlist))) {

		goto cw_failure;
	}
	
	gWTPRetransmissionCount= 0;
	
	while(gWTPRetransmissionCount < gCWMaxRetransmit) 
	{
		CWDebugLog("Transmission Num:%d", gWTPRetransmissionCount);
		for(i = 0; i < fragmentsNum; i++) 
		{
#ifdef CW_NO_DTLS
			if(!CWNetworkSendUnsafeConnected(gWTPSocket, 
							 messages[i].msg,
							 messages[i].offset))
#else
			if(!CWSecuritySend(gWTPSession,
					   messages[i].msg, 
					   messages[i].offset))
#endif
			{
				CWDebugLog("Failure sending Request");
				goto cw_failure;
			}
		}
		
		timewait.tv_sec = time(0) + gTimeToSleep;
		timewait.tv_nsec = 0;

		CW_REPEAT_FOREVER 
		{
			CWThreadMutexLock(&gInterfaceMutex);

			if (CWGetCountElementFromSafeList(gPacketReceiveList) > 0)
				CWErrorRaise(CW_ERROR_SUCCESS, NULL);
			else {
				if (CWErr(CWWaitThreadConditionTimeout(&gInterfaceWait, &gInterfaceMutex, &timewait)))
					CWErrorRaise(CW_ERROR_SUCCESS, NULL);
			}

			CWThreadMutexUnlock(&gInterfaceMutex);

			switch(CWErrorGetLastErrorCode()) {

				case CW_ERROR_TIME_EXPIRED:
				{
					gWTPRetransmissionCount++;
					goto cw_continue_external_loop;
					break;
				}

				case CW_ERROR_SUCCESS:
				{
					/* there's something to read */
					if(!(CWReceiveMessage(&msg))) 
					{
						CW_FREE_PROTOCOL_MESSAGE(msg);
						CWDebugLog("Failure Receiving Response");
						goto cw_failure;
					}
					
					if(!(parseFunc(msg.msg, msg.offset, seqNum, valuesPtr))) 
					{
						if(CWErrorGetLastErrorCode() != CW_ERROR_INVALID_FORMAT) {

							CW_FREE_PROTOCOL_MESSAGE(msg);
							CWDebugLog("Failure Parsing Response");
							goto cw_failure;
						}
						else {
							CWErrorHandleLast();
							{ 
								gWTPRetransmissionCount++;
								goto cw_continue_external_loop;
							}
							break;
						}
					}
					
					if((saveFunc(valuesPtr))) {

						goto cw_success;
					} 
					else {
						if(CWErrorGetLastErrorCode() != CW_ERROR_INVALID_FORMAT) {
							CW_FREE_PROTOCOL_MESSAGE(msg);
							CWDebugLog("Failure Saving Response");
							goto cw_failure;
						} 
					}
					break;
				}

				case CW_ERROR_INTERRUPTED: 
				{
					gWTPRetransmissionCount++;
					goto cw_continue_external_loop;
					break;
				}	
				default:
				{
					CWErrorHandleLast();
					CWDebugLog("Failure");
					goto cw_failure;
					break;
				}
			}
		}
		
		cw_continue_external_loop:
			CWDebugLog("Retransmission time is over");
			
			gTimeToSleep<<=1;
			if ( gTimeToSleep > gMaxTimeToSleep ) gTimeToSleep = gMaxTimeToSleep;
	}

	/* too many retransmissions */
	return CWErrorRaise(CW_ERROR_NEED_RESOURCE, "Peer Dead");
	
cw_success:	
	for(i = 0; i < fragmentsNum; i++) {
		CW_FREE_PROTOCOL_MESSAGE(messages[i]);
	}
	
	CW_FREE_OBJECT(messages);
	CW_FREE_PROTOCOL_MESSAGE(msg);
	
	return CW_TRUE;
	
cw_failure:
	if(messages != NULL) {
		for(i = 0; i < fragmentsNum; i++) {
			CW_FREE_PROTOCOL_MESSAGE(messages[i]);
		}
		CW_FREE_OBJECT(messages);
	}
	CWDebugLog("Failure");
	return CW_FALSE;
}

// Yuan Hong
//local version format: x.x.x 
int  tmpv1, tmpv2, tmpv3; 
int  etcv1, etcv2, etcv3; 
int  maxv1, maxv2, maxv3;
char report_if[65]; 
char report_mac[18]; 
char report_ip[INET6_ADDRSTRLEN*2];

int read_report_info(const char *path) {
    char line[1024], errinfo[256];
    FILE *fp = fopen(path, "r");
    if (fp == 0) {
        CWLog("read_report_if_name() fopen(%s) error: %s", path, strerror(errno));
        return -1;
    }
    if (fgets(line, sizeof(line), fp)) {
        sscanf(line, "%s", report_if);
        CWLog("Read upgrade report interface name:[%s]", report_if);
    } else {
        CWLog("Read upgrade report interface name error: %s", strerror(errno));
        fclose(fp);
        return -1;
    }
    fclose(fp);

    if (getifmac(report_if, report_mac, sizeof(report_mac), errinfo, sizeof(errinfo)) < 0){
        CWLog("%s", errinfo);
        return -1;
    }
    CWLog("Read upgrade report interface  MAC:[%s]", report_mac);

    if (getifaddr(report_if, AF_INET, report_ip, sizeof(report_ip), errinfo, sizeof(errinfo)) < 0) {
        CWLog("%s", errinfo);
        return -1;
    }
    char *p = strrchr(report_ip, '%'); if (p) *p = 0; // cut off '%' if AF_INET6's report_ip

    CWLog("Read upgrade report interface addr:[%s]", report_ip);

    return 0;
}

int read_version_tmp(const char *path) {
    char line[1024];
    tmpv1 = tmpv2 = tmpv3 = 0;
    FILE *fp = fopen(path, "r");
    if (fp == 0) {
        CWLog("fopen(%s):%s", path, strerror(errno));
        return -1;
    }
    if (fgets(line, sizeof(line), fp)) {
        sscanf(line, "%d.%d.%d", &tmpv1, &tmpv2, &tmpv3);
        CWLog("Read tmp version info: %d.%d.%d %s", tmpv1, tmpv2, tmpv3, path);
    } else {
        CWLog("Read tmp version info error: %s: %s", path, strerror(errno));
        fclose(fp);
        return -1;
    }

    fclose(fp);
    return 0;
}
int read_version_etc(const char *path) {
    char line[1024];
    etcv1 = etcv2 = etcv3 = 0;
    FILE *fp = fopen(path, "r");
    if (fp == 0) {
        CWLog("fopen(%s):%s", path, strerror(errno));
        return -1;
    }
    if (fgets(line, sizeof(line), fp)) {
        sscanf(line, "%d.%d.%d", &etcv1, &etcv2, &etcv3);
        CWLog("Read sys version info: %d.%d.%d %s", etcv1, etcv2, etcv3, path);
    } else {
        CWLog("Read sys version info error: %s: %s", path, strerror(errno));
        fclose(fp);
        return -1;
    }

    fclose(fp);
    return 0;
}

// 按位权比较，如果前3者大于后三者返回1，相同返回0
int maxver(int a1, int a2, int a3, int b1, int b2, int b3)
{
    if (a1 > b1) return 1; 
    else {
        if (a1 < b1) return 0; 
        else { 
            if (a2 > b2) return 1;
            else { 
                if (a2 < b2) return 0;
                else { 
                    if (a3 > b3) return 1;
                    else { 
                        if (a3 < b3) return 0;
                        else { 
                            return 0; // Same
                        }
                    }
                }
            }

        }
    }
}

void load_sino_conf()
{
    CWLog("Load Sinoix configure");

    read_version_tmp(FIRMWARE_IMAGE_INFO);
    read_version_etc(FIRMWARE_VERSION);

    if (maxver(tmpv1, tmpv2, tmpv3, etcv1, etcv2, etcv3)) {
        maxv1 = tmpv1; 
        maxv2 = tmpv2; 
        maxv3 = tmpv3;
    } else {
        maxv1 = etcv1; 
        maxv2 = etcv2; 
        maxv3 = etcv3;
     }

    if (read_report_info(REPORT_IF_NAME) < 0) {
        CWLog("Load Sinoix configure failed");
        exit(1);
    }

    int err;
    if ((err = __pthread_attr_init__(&g_thread_stack_attr)) != 0) {
        CWLog("__pthread_attr_init__() error: %s\n", strerror(err));
        exit(1);
    }
    if ((err = __pthread_attr_setstacksize__(&g_thread_stack_attr,THREAD_STACK_SIZE)) != 0) {
        CWLog("__pthread_attr_setstacksize__() error: %s\n", strerror(err));
        exit(1);
    }

    CWLog("Load Sinoix configure done.");
}

int write_firmware_image_info(const char *path, const char *ver, const char *md5, const char *flen) {
    char buff[1024];
    FILE *fp = fopen(path, "w");
    if (fp == 0) {
        CWLog("write_firmware_image_info() fopen(%s) error: %s", path, strerror(errno));
        return -1;
    }
    snprintf(buff, sizeof(buff), "%s\n%s\n%s\n", ver, md5, flen);
    if (fputs(buff, fp) == EOF){
        CWLog("write_firmware_image_info() fputs() error: %s", strerror(errno));
        fclose(fp);
        return -1;
    } 
    CWLog("Save firmware image info: %s", ver);
    fclose(fp);
    return 0;
}

int main (int argc, const char * argv[]) {
	
	/* Daemon Mode */
 
	pid_t pid;
	
	if (argc <= 1)
		printf("Usage: WTP working_path\n");
#if 0
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
#endif
	
	
	CWStateTransition nextState = CW_ENTER_DISCOVERY;

	CWLogInitFile(WTP_LOG_FILE_NAME);

#ifdef CW_NO_DTLS
	if( !CWErr(CWWTPLoadConfiguration()) ) {
#else
	if( !CWErr(CWSecurityInitLib())	|| !CWErr(CWWTPLoadConfiguration()) ) {
#endif
		CWLog("Can't start WTP");
		exit(1);
	}
	
    load_sino_conf();
    signal(SIGCHLD, SIG_IGN);

#ifndef CW_SINGLE_THREAD
	CWDebugLog("Use Threads");
#else
	CWDebugLog("Don't Use Threads");
#endif
	CWErrorHandlingInitLib();
	
	if(!CWParseSettingsFile()){
		CWLog("Can't start WTP");
		exit(1);
	}


	/* Capwap receive packets list */
	if (!CWErr(CWCreateSafeList(&gPacketReceiveList)))
	{
		CWLog("Can't start WTP");
		exit(1);
	}

	/* Capwap receive frame list */
	if (!CWErr(CWCreateSafeList(&gFrameList)))
	{
		CWLog("Can't start WTP");
		exit(1);
	}

	CWCreateThreadMutex(&gInterfaceMutex);
	CWSetMutexSafeList(gPacketReceiveList, &gInterfaceMutex);
	CWSetMutexSafeList(gFrameList, &gInterfaceMutex);
	CWCreateThreadCondition(&gInterfaceWait);
	CWSetConditionSafeList(gPacketReceiveList, &gInterfaceWait);
	CWSetConditionSafeList(gFrameList, &gInterfaceWait);

	CWLog("Starting WTP...");
	
	CWRandomInitLib();

	CWThreadSetSignals(SIG_BLOCK, 1, SIGALRM);
	
	if (timer_init() == 0) {
		CWLog("Can't init timer module");
		exit(1);
	}

    /*
#ifdef CW_NO_DTLS
	if( !CWErr(CWWTPLoadConfiguration()) ) {
#else
	if( !CWErr(CWSecurityInitLib())	|| !CWErr(CWWTPLoadConfiguration()) ) {
#endif
		CWLog("Can't start WTP");
		exit(1);
	} */
	
	CWDebugLog("Init WTP Radio Info");
	if(!CWWTPInitConfiguration())
	{
		CWLog("Error Init Configuration");
		exit(1);
	}


	CWThread thread_receiveFrame;
	if(!CWErr(CWCreateThread(&thread_receiveFrame, CWWTPReceiveFrame, NULL))) {
		CWLog("Error starting Thread that receive binding frame");
		exit(1);
	}

	CWThread thread_receiveStats;
	if(!CWErr(CWCreateThread(&thread_receiveStats, CWWTPReceiveStats, NULL))) {
		CWLog("Error starting Thread that receive stats on monitoring interface");
		exit(1);
	}

	/****************************************
	 * 2009 Update:							*
	 *				Spawn Frequency Stats	*
	 *				Receiver Thread			*
	 ****************************************/
		
	CWThread thread_receiveFreqStats;
	if(!CWErr(CWCreateThread(&thread_receiveFreqStats, CWWTPReceiveFreqStats, NULL))) {
		CWLog("Error starting Thread that receive frequency stats on monitoring interface");
		exit(1);
	}

	/* if AC address is given jump Discovery and use this address for Joining */
	if(gWTPForceACAddress != NULL)	nextState = CW_ENTER_JOIN;

	/* start CAPWAP state machine */	
	CW_REPEAT_FOREVER {
		switch(nextState) {
			case CW_ENTER_DISCOVERY:
				nextState = CWWTPEnterDiscovery();
				break;
			case CW_ENTER_SULKING:
				nextState = CWWTPEnterSulking();
				break;
			case CW_ENTER_JOIN:
				nextState = CWWTPEnterJoin();
				break;
			case CW_ENTER_CONFIGURE:
				nextState = CWWTPEnterConfigure();
				break;	
			case CW_ENTER_DATA_CHECK:
				nextState = CWWTPEnterDataCheck();
				break;	
			case CW_ENTER_RUN:
				nextState = CWWTPEnterRun();
				break;
			case CW_ENTER_RESET:
				/*
				 * CWStopHeartbeatTimer();
				 * CWStopNeighborDeadTimer();
				 * CWNetworkCloseSocket(gWTPSocket);
				 * CWSecurityDestroySession(gWTPSession);
				 * CWSecurityDestroyContext(gWTPSecurityContext);
				 * gWTPSecurityContext = NULL;
				 * gWTPSession = NULL;
				 */
				nextState = CW_ENTER_DISCOVERY;
				break;
			case CW_QUIT:
				CWWTPDestroy();
				return 0;
		}
	}
}

__inline__ unsigned int CWGetSeqNum() {
	static unsigned int seqNum = 0;
	
	if (seqNum==CW_MAX_SEQ_NUM) seqNum=0;
	else seqNum++;
	return seqNum;
}

__inline__ int CWGetFragmentID() {
	static int fragID = 0;
	return fragID++;
}


/* 
 * Parses config file and inits WTP configuration.
 */
CWBool CWWTPLoadConfiguration() {
	int i;
	
	CWLog("WTP Loads Configuration");
	
	/* get saved preferences */
	if(!CWErr(CWParseConfigFile())) {
		CWLog("Can't Read Config File");
		exit(1);
	}
	
	if(gCWACCount == 0) 
		return CWErrorRaise(CW_ERROR_NEED_RESOURCE, "No AC Configured");
	
	CW_CREATE_ARRAY_ERR(gCWACList, 
			    gCWACCount,
			    CWACDescriptor,
			    return CWErrorRaise(CW_ERROR_OUT_OF_MEMORY, NULL););


	for(i = 0; i < gCWACCount; i++) {

		CWDebugLog("Init Configuration for AC at %s", gCWACAddresses[i]);
		CW_CREATE_STRING_FROM_STRING_ERR(gCWACList[i].address, gCWACAddresses[i],
						 return CWErrorRaise(CW_ERROR_OUT_OF_MEMORY, NULL););

	}
	
	CW_FREE_OBJECTS_ARRAY(gCWACAddresses, gCWACCount);
	return CW_TRUE;
}

void CWWTPDestroy() {
	int i;
	
	CWLog("Destroy WTP");
	
	for(i = 0; i < gCWACCount; i++) {
		CW_FREE_OBJECT(gCWACList[i].address);
	}
	
	timer_destroy();

	CW_FREE_OBJECT(gCWACList);
	CW_FREE_OBJECT(gRadiosInfo.radiosInfo);
}

CWBool CWWTPInitConfiguration() {
	int i;

	CWWTPResetRebootStatistics(&gWTPRebootStatistics);

	gRadiosInfo.radioCount = CWWTPGetMaxRadios();
	CW_CREATE_ARRAY_ERR(gRadiosInfo.radiosInfo, gRadiosInfo.radioCount, CWWTPRadioInfoValues, return CWErrorRaise(CW_ERROR_OUT_OF_MEMORY, NULL););
	gRadiosInfo.radiosInfo[0].radioID= 0;
	/* gRadiosInfo.radiosInfo[0].numEntries = 0; */
	gRadiosInfo.radiosInfo[0].decryptErrorMACAddressList = NULL;
	gRadiosInfo.radiosInfo[0].reportInterval= CW_REPORT_INTERVAL_DEFAULT;
	gRadiosInfo.radiosInfo[0].adminState= ENABLED;
	gRadiosInfo.radiosInfo[0].adminCause= AD_NORMAL;
	gRadiosInfo.radiosInfo[0].operationalState= ENABLED;
	gRadiosInfo.radiosInfo[0].operationalCause= OP_NORMAL;
	gRadiosInfo.radiosInfo[0].TxQueueLevel= 0;
	gRadiosInfo.radiosInfo[0].wirelessLinkFramesPerSec= 0;
	CWWTPResetRadioStatistics(&(gRadiosInfo.radiosInfo[0].statistics));
	if(!CWWTPInitBinding(0)) {return CW_FALSE;}
	
	for (i=1; i<gRadiosInfo.radioCount; i++)
	{
		gRadiosInfo.radiosInfo[i].radioID= i;
		/* gRadiosInfo.radiosInfo[i].numEntries = 0; */
		gRadiosInfo.radiosInfo[i].decryptErrorMACAddressList = NULL;
		gRadiosInfo.radiosInfo[i].reportInterval= CW_REPORT_INTERVAL_DEFAULT;
		/* Default value for CAPWA� */
		gRadiosInfo.radiosInfo[i].adminState= ENABLED; 
		gRadiosInfo.radiosInfo[i].adminCause= AD_NORMAL;
		gRadiosInfo.radiosInfo[i].operationalState= DISABLED;
		gRadiosInfo.radiosInfo[i].operationalCause= OP_NORMAL;
		gRadiosInfo.radiosInfo[i].TxQueueLevel= 0;
		gRadiosInfo.radiosInfo[i].wirelessLinkFramesPerSec= 0;
		CWWTPResetRadioStatistics(&(gRadiosInfo.radiosInfo[i].statistics));
		if(!CWWTPInitBinding(i)) {return CW_FALSE;}
	}
	return CW_TRUE;
}
