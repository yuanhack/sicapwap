/*******************************************************************************************
 * Copyright (c) 2006-7 Laboratorio di Sistemi di Elaborazione e Bioingegneria Informatica *
 *                      Universita' Campus BioMedico - Italy                               *
 *                                                                                         *
 * This program is free software; you can redistribute it and/or modify it under the terms *
 * of the GNU General Public License as published by the Free Software Foundation; either  *
 * version 2 of the License, or (at your option) any later version.                        *
 *                                                                                         *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY         *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A 	   *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.                *
 *                                                                                         *
 * You should have received a copy of the GNU General Public License along with this       *
 * program; if not, write to the:                                                          *
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330, Boston,                    *
 * MA  02111-1307, USA.                                                                    *
 *                                                                                         *
 * --------------------------------------------------------------------------------------- *
 * Project:  Capwap                                                                        *
 *                                                                                         *
 * Author :  Ludovico Rossi (ludo@bluepixysw.com)                                          *  
 *           Del Moro Andrea (andrea_delmoro@libero.it)                                    *
 *           Giovannini Federica (giovannini.federica@gmail.com)                           *
 *           Massimo Vellucci (m.vellucci@unicampus.it)                                    *
 *           Mauro Bisson (mauro.bis@gmail.com)                                            *
 *******************************************************************************************/

 
#include "CWAC.h"
#include "CWCommon.h"
#include "sino_comm.h"
#include "read_conf.h"
#include "yhepoll.h"
#include "pthread_stack.h"
#include "sino_db.h"

#include <sys/stat.h>
#include <sys/resource.h>

#ifdef DMALLOC
#include "../dmalloc-5.5.0/dmalloc.h"
#endif

/*_________________________________________________________*/
/*  *******************___VARIABLES___*******************  */
CWThreadMutex gCreateIDMutex;

/* array that stores per WTPs infos */
CWWTPManager gWTPs[CW_MAX_WTP];
CWThreadMutex gWTPsMutex;

int gEnabledLog;
int gMaxLogFileSize;
char gLogFileName[]=AC_LOG_FILE_NAME;

/* number of active WTPs */
int gActiveWTPs = 0;
CWThreadMutex gActiveWTPsMutex;

/* max WTPs */
int gMaxWTPs;
/* The Radio MAC Field of the discovery response */
int gRMACField = 0;
/* The Wireless Field of the discovery response */
int gWirelessField = 0;
/* DTLS Policy for data channel */
int gDTLSPolicy=DTLS_ENABLED_DATA;
/* special socket to handle multiple network interfaces */
CWMultiHomedSocket gACSocket;
/* AC's network interfaces */
CWProtocolNetworkInterface *gInterfaces = NULL;
int gInterfacesCount = 0;
/* DTLS Context */
CWSecurityContext gACSecurityContext;
int gActiveStations = 0;
/* max stations */
int gLimit;
char **gMulticastGroups;
int gMulticastGroupsCount;
CWAuthSecurity gACDescriptorSecurity;
int gACHWVersion;
int gACSWVersion;
char *gACName = NULL;

int gDiscoveryTimer=20;
int gEchoRequestTimer=CW_ECHO_INTERVAL_DEFAULT;
/* PROVVISORIO: Il valore e' scelto a caso */
int gIdleTimeout=10;

/*_________________________________________________________*/
/*  *******************___FUNCTIONS___*******************  */
int main (int argc, const char * argv[]) {

	/* Daemon mode */
	
	if (argc <= 1)
		printf("Usage: AC working_path\n");

#if 0
	if (daemon(1, 0) < 0)
		exit(1);
#endif

	if (chdir(argv[1]) != 0)
		exit(1);
	

	CWACInit();
	CWACEnterMainLoop();
	CWACDestroy();  
	
	return 0;
}

int CWACSemPostForOpenSSLHack(void *s) {

	CWThreadTimedSem *semPtr = (CWThreadTimedSem*) s;
	
	if(!CWThreadTimedSemIsZero(semPtr)) {
		CWLog("This Semaphore's Value should really be 0");
		/* note: we can consider setting the value to 0 and going on,
		 * that is what we do here
		 */
		if(!CWErr(CWThreadTimedSemSetValue(semPtr, 0))) return 0;
	}
	
	if(!CWErr(CWThreadTimedSemPost(semPtr))) {
		return 0;
	}
	 
	return 1;
}

void CWACInit() {
	int i;
	CWNetworkLev4Address *addresses = NULL;
	struct sockaddr_in *IPv4Addresses = NULL;
	
	CWLogInitFile(AC_LOG_FILE_NAME);
	
	#ifndef CW_SINGLE_THREAD
		CWDebugLog("Use Threads");
	#else
		CWDebugLog("Don't Use Threads");
	#endif
	
	CWErrorHandlingInitLib();
	
	if(!CWParseSettingsFile())
	{
		CWLog("Can't start AC");
		exit(1);
	}

	CWLog("Starting AC");

	CWThreadSetSignals(SIG_BLOCK, 1, SIGALRM);
	if (timer_init() == 0) {
		CWLog("Can't init timer module");
		exit(1);
	}

	if(!CWErr(CWParseConfigFile()) ||
#ifndef CW_NO_DTLS
	   !CWErr(CWSecurityInitLib()) ||
#endif
	   !CWErr(CWNetworkInitSocketServerMultiHomed(&gACSocket, CW_CONTROL_PORT, gMulticastGroups, gMulticastGroupsCount)) ||
	   !CWErr(CWNetworkGetInterfaceAddresses(&gACSocket, &addresses, &IPv4Addresses)) ||
	   !CWErr(CWCreateThreadMutex(&gWTPsMutex)) ||
	   !CWErr(CWCreateThreadMutex(&gActiveWTPsMutex))) {

		/* error starting */
		CWLog("Can't start AC");
		exit(1);
	}

#ifndef CW_NO_DTLS
	if(gACDescriptorSecurity == CW_X509_CERTIFICATE) {

		if(!CWErr(CWSecurityInitContext(&gACSecurityContext,
						"root.pem",
						"server.pem",
						"prova",
						CW_FALSE,
						CWACSemPostForOpenSSLHack))) {

			CWLog("Can't start AC");
			exit(1);
		}
	} else { /* preshared */
		if(!CWErr(CWSecurityInitContext(&gACSecurityContext,
						NULL,
						NULL,
						NULL,
						CW_FALSE,
						CWACSemPostForOpenSSLHack))) {
			CWLog("Can't start AC");
			exit(1);
		}
	}
#endif
	CW_FREE_OBJECTS_ARRAY(gMulticastGroups, gMulticastGroupsCount);

	for(i = 0; i < CW_MAX_WTP; i++) {
		gWTPs[i].isNotFree = CW_FALSE;
	}

	/* store network interface's addresses */
	gInterfacesCount = CWNetworkCountInterfaceAddresses(&gACSocket);
	CWLog("Found %d Network Interface(s)", gInterfacesCount);
	
	if (gInterfacesCount<=0){
		CWLog("Can't start AC");
		exit(1);
	}

	CW_CREATE_ARRAY_ERR(gInterfaces, 
			    gInterfacesCount,
			    CWProtocolNetworkInterface,
			    CWLog("Out of Memory"); return;);

	for(i = 0; i < gInterfacesCount; i++) {
		gInterfaces[i].WTPCount = 0;
		CW_COPY_NET_ADDR_PTR(&(gInterfaces[i].addr), ((CWNetworkLev4Address*)&((addresses)[i])) );
		if(IPv4Addresses != NULL) {
			CW_COPY_NET_ADDR_PTR(&(gInterfaces[i].addrIPv4), &((IPv4Addresses)[i]));
		}
	}
	CW_FREE_OBJECT(addresses);
	CW_FREE_OBJECT(IPv4Addresses);

	if(!CWErr(CWCreateThreadMutex(&gCreateIDMutex))) {
		exit(1);
	}

    // add by Yuan Hong 
    int err;
    if ((err = __pthread_attr_init__(&g_thread_stack_attr)) != 0) {
        CWLog("__pthread_attr_init__() error: %s\n", strerror(err));
        exit(1);
    }
    if ((err = __pthread_attr_setstacksize__(&g_thread_stack_attr,THREAD_STACK_SIZE)) != 0) {
        CWLog("__pthread_attr_setstacksize__() error: %s\n", strerror(err));
        exit(1);
    }

    struct rlimit rl;
    if (getrlimit(RLIMIT_NOFILE, &rl) == 0) {
        CWLog("Process files %ld max=%ld", (long)rl.rlim_cur, (long)rl.rlim_max);
    } else {
        CWLog("getrlimit error: %s", strerror(errno));
        exit(1);
    }

    rl.rlim_cur = 30000;
    rl.rlim_max = 30000;
    if (setrlimit(RLIMIT_NOFILE, &rl) == 0) {
        CWLog("Set process files %ld max=%ld success", (long)rl.rlim_cur, (long)rl.rlim_max);
    } else {
        CWLog("setrlimit error: %s", strerror(errno));
        exit(1);
    }

    // WTP's updata module, load configuration payload
    // 2014 Sinoix - Yuan Hong
#ifdef SINOIX_PAYLOAD

    void sino_init();
    sino_init();

#endif
    CWLog("AC Started");
}


void CWACDestroy() {
	
	CWNetworkCloseMultiHomedSocket(&gACSocket);
	
	/*
	for(i = 0; i < CW_MAX_WTP; i++) {
		//CW_FREE_OBJECT(gWTPs[i].addr);
	}
	*/

	CWSslCleanUp();

	CWDestroyThreadMutex(&gWTPsMutex);
	CWDestroyThreadMutex(&gCreateIDMutex);
	CWDestroyThreadMutex(&gActiveWTPsMutex);
	
	CW_FREE_OBJECT(gACName);
	CW_FREE_OBJECT(gInterfaces);
	
	CWLog("AC Destroyed");
}


__inline__ unsigned int CWGetSeqNum() {

	static unsigned int seqNum = 0;
	unsigned int r;
	
	if(!CWThreadMutexLock(&gCreateIDMutex)) {
		
		CWDebugLog("Error Locking a mutex");
	}
	
	r = seqNum;
	
	if (seqNum==CW_MAX_SEQ_NUM) 
		seqNum=0;
	else 
		seqNum++;

	CWThreadMutexUnlock(&gCreateIDMutex);
	return r;
}


__inline__ int CWGetFragmentID() {

	static int fragID = 0;
	int r;

	if(!CWThreadMutexLock(&gCreateIDMutex)) {
		
		CWDebugLog("Error Locking a mutex");
	}
	
	r = fragID;
	
	if (fragID==CW_MAX_FRAGMENT_ID) 
		fragID=0;
	else 
		fragID++;

	CWThreadMutexUnlock(&gCreateIDMutex);
	return r;
}

void sino_config()
{
    char imagepath[128]; 
    const char * p, *pinfo;
    struct stat st;
    p = strrchr(FIRMWARE_IMAGE_PATH, '/');
    ++p;
    pinfo = strrchr(FIRMWARE_IMAGE_INFO, '/');
    ++pinfo;

    snprintf(imagepath, sizeof(imagepath), "%s/%s", TFTPBOOT, p);
    CWLog("\n");
    CWLog("######### %s #########", imagepath);

    sino_lock();
    sino_data_init(&sino);
    sino_data_head(&sino);

    if (stat(imagepath, &st) < 0) {
        CWLog("stat() error: %s: %s", imagepath, strerror(errno));
    } else {
        if (!S_ISREG(st.st_mode)) { // File no exists
            CWLog("%s: Is not a normal regular file", imagepath);
            return;
        } else {
            do {
                if (read_conf(pinfo, "#") < 0) break;
                if (firmware_version[0]) {
                    sino_data_push(&sino, SI_FI_VERSION, firmware_version, strlen(firmware_version));
                    CWLog("Firmware Version %s", firmware_version);
                }
                if (firmware_len[0]) {
                    sino_data_push(&sino, SI_FI_LEN,  firmware_len, strlen(firmware_len));
                    CWLog("Firmware Length %s", firmware_len);
                }
                if (firmware_md5[0]) {
                    int n = strlen(firmware_md5);
                    if (n == 32 || n == 64) {
                        sino_data_push(&sino, SI_FI_MD5, firmware_md5, n);
                        CWLog("Firmware MD5 %s", firmware_md5);
                    } else {
                        CWLog("MD5 format or length error[%s]", firmware_md5);
                    }
                }

                if (tftp_locat[0]) {
                    sscanf(tftp_locat, "%[^:]:%d", tftp_addr, &tftp_port);
                    CWLog("TFTP Server %s", tftp_locat);
                }
                if (tftp_addr[0])   // tftp addr
                    sino_data_push(&sino, SI_TFTP_ADDR, tftp_addr, strlen(tftp_addr));
                if (tftp_port > 0) { // port
                    char port[6];
                    snprintf(port, sizeof(port), "%d", tftp_port);
                    sino_data_push(&sino, SI_TFTP_PORT, port, strlen(port));
                }

                if (rept_locat[0]) {
                    sscanf(rept_locat, "%[^:]:%d", rept_addr, &rept_port);
                    CWLog("Rept Server %s", rept_locat);
                }
                if (rept_addr[0])   // rept addr
                    sino_data_push(&sino, SI_REPT_ADDR, rept_addr, strlen(rept_addr));
                if (rept_port > 0) { // rept port
                    char port[6];
                    snprintf(port, sizeof(port), "%d", rept_port);
                    sino_data_push(&sino, SI_REPT_PORT, port, strlen(port));
                }
            } while (0);
        } 
    } // if (stat(imagepath, &st) < 0) else
    sino_unlock();
    CWLog("######### %s #########", imagepath);
}

void sino_init()
{
    sino_config();
    if (firmware_version[0] == 0)
        CWLog("Without the firmware file");

    sino_service_start();
    if (mysql_initialize() < 0) {
        CWLog("mysql_initialize failed");
        exit(1);
    }
    if (mysql_conn() < 0) {
        CWLog("mysql_conn failed");
        exit(1);
    }

    ap_online("xxxxxxxxxx");
    sleep(3);
    ap_offline("xxxxxxxxxx");

        
}
