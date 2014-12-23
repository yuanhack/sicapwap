/*******************************************************************************************
 * Copyright (c) 2006-7 Laboratorio di Sistemi di Elaborazione e Bioingegneria Informatica *
 *                      Universita' Campus BioMedico - Italy                               *
 *                                                                                         *
 * This program is free software; you can redistribute it and/or modify it under the terms *
 * of the GNU General Public License as published by the Free Software Foundation; either  *
 * version 2 of the License, or (at your option) any later version.                        *
 *                                                                                         *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY         *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A 	       *
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

#include "CWWTP.h"
#ifdef DMALLOC
#include "../dmalloc-5.5.0/dmalloc.h"
#endif
/*____________________________________________________________________________*/
/*  *****************************___ASSEMBLE___*****************************  */
CWBool CWAssembleMsgElemACName(CWProtocolMessage *msgPtr) 
{
	char *name;
	if(msgPtr == NULL) return CWErrorRaise(CW_ERROR_WRONG_ARG, NULL);
	
	name = CWWTPGetACName();
//	CWDebugLog("AC Name: %s", name);
	
	// create message
	CW_CREATE_PROTOCOL_MESSAGE(*msgPtr, strlen(name), return CWErrorRaise(CW_ERROR_OUT_OF_MEMORY, NULL););
	
	CWProtocolStoreStr(msgPtr, name);
				
	return CWAssembleMsgElem(msgPtr, CW_MSG_ELEMENT_AC_NAME_CW_TYPE);
}
CWBool CWAssembleMsgElemACNameWithIndex(CWProtocolMessage *msgPtr) 
{
	const int ac_Index_length=1;
	CWACNamesWithIndex ACsinfo;
	CWProtocolMessage *msgs;
	int len = 0;
	int i;
	int j;
	
	if(msgPtr == NULL) return CWErrorRaise(CW_ERROR_WRONG_ARG, NULL);
	
	if(!CWWTPGetACNameWithIndex(&ACsinfo)){
		return CW_FALSE;
	}
	
	CW_CREATE_PROTOCOL_MSG_ARRAY_ERR(msgs, ACsinfo.count, return  CWErrorRaise(CW_ERROR_OUT_OF_MEMORY, NULL););
	
	// create message
	for(i = 0; i < ACsinfo.count; i++) {
		// create message
		CW_CREATE_PROTOCOL_MESSAGE(msgs[i], ac_Index_length+strlen(ACsinfo.ACNameIndex[i].ACName), return CWErrorRaise(CW_ERROR_OUT_OF_MEMORY, NULL););
		CWProtocolStore8(&(msgs[i]), ACsinfo.ACNameIndex[i].index); // ID of the AC
		CWProtocolStoreStr(&(msgs[i]), ACsinfo.ACNameIndex[i].ACName); // name of the AC
		if(!(CWAssembleMsgElem(&(msgs[i]), CW_MSG_ELEMENT_AC_NAME_INDEX_CW_TYPE))) {
			for(j = i; j >= 0; j--) { CW_FREE_PROTOCOL_MESSAGE(msgs[j]);}
			CW_FREE_OBJECT(ACsinfo.ACNameIndex);
			CW_FREE_OBJECT(msgs);
			return CW_FALSE;
		}
//		CWDebugLog("AC Name with index: %d - %s", ACsinfo.ACNameIndex[i].index, ACsinfo.ACNameIndex[i].ACName);
		len += msgs[i].offset;
	}
	CW_CREATE_PROTOCOL_MESSAGE(*msgPtr, len, return CWErrorRaise(CW_ERROR_OUT_OF_MEMORY, NULL););
	
	for(i = 0; i < ACsinfo.count; i++) {
		CWProtocolStoreMessage(msgPtr, &(msgs[i]));
		CW_FREE_PROTOCOL_MESSAGE(msgs[i]);
	}
	
	CW_FREE_OBJECT(msgs);
 	/*
         * They free ACNameIndex, which is an array of CWACNameWithIndexValues,
         * but nobody cares to free the actual strings that were allocated as fields
         * of the CWACNameWithIndexValues structures in the CWWTPGetACNameWithIndex() 
         * function.. Here we take care of this.
         *
         * BUG ML06
         * 16/10/2009 - Donato Capitella 
         */
        CW_FREE_OBJECT(ACsinfo.ACNameIndex[0].ACName);
        CW_FREE_OBJECT(ACsinfo.ACNameIndex[1].ACName);
	CW_FREE_OBJECT(ACsinfo.ACNameIndex);
	
	return CW_TRUE;
}
CWBool CWAssembleMsgElemDataTransferData(CWProtocolMessage *msgPtr, int data_type) {
	if(msgPtr == NULL) return CWErrorRaise(CW_ERROR_WRONG_ARG, NULL);
	
	char* debug_data= " #### DATA DEBUG INFO #### ";   //to be changed...
	// create message
	CW_CREATE_PROTOCOL_MESSAGE(*msgPtr, 2 + strlen(debug_data) , return CWErrorRaise(CW_ERROR_OUT_OF_MEMORY, NULL););
	
	CWProtocolStore8(msgPtr, data_type);
	CWProtocolStore8(msgPtr,strlen(debug_data));
	CWProtocolStoreStr(msgPtr, debug_data);
	
	return CWAssembleMsgElem(msgPtr, CW_MSG_ELEMENT_DATA_TRANSFER_DATA_CW_TYPE);
}
CWBool CWAssembleMsgElemDiscoveryType(CWProtocolMessage *msgPtr) {
	if(msgPtr == NULL) return CWErrorRaise(CW_ERROR_WRONG_ARG, NULL);
	
	// create message
	CW_CREATE_PROTOCOL_MESSAGE(*msgPtr, 1, return CWErrorRaise(CW_ERROR_OUT_OF_MEMORY, NULL););
	
//	CWDebugLog("Discovery Type: %d", CWWTPGetDiscoveryType());
	CWProtocolStore8(msgPtr, CWWTPGetDiscoveryType());
	return CWAssembleMsgElem(msgPtr, CW_MSG_ELEMENT_DISCOVERY_TYPE_CW_TYPE);
}
CWBool CWAssembleMsgElemLocationData(CWProtocolMessage *msgPtr) {
	char *location;
	
	if(msgPtr == NULL) return CWErrorRaise(CW_ERROR_WRONG_ARG, NULL);
	
	location = CWWTPGetLocation();
	
	// create message
	CW_CREATE_PROTOCOL_MESSAGE(*msgPtr, strlen(location), return CWErrorRaise(CW_ERROR_OUT_OF_MEMORY, NULL););
	
//	CWDebugLog("Location Data: %s", location);
	CWProtocolStoreStr(msgPtr, location);
					
	return CWAssembleMsgElem(msgPtr, CW_MSG_ELEMENT_LOCATION_DATA_CW_TYPE);	
}
CWBool CWAssembleMsgElemStatisticsTimer(CWProtocolMessage *msgPtr)
{	
	const int statistics_timer_length= 2;
	
	CW_CREATE_PROTOCOL_MESSAGE(*msgPtr, statistics_timer_length, return CWErrorRaise(CW_ERROR_OUT_OF_MEMORY, NULL););
	
	CWProtocolStore16(msgPtr, CWWTPGetStatisticsTimer());
	
//	CWDebugLog("Statistics Timer: %d", CWWTPGetStatisticsTimer());
	
	return CWAssembleMsgElem(msgPtr, CW_MSG_ELEMENT_STATISTICS_TIMER_CW_TYPE);	
}
CWBool CWAssembleMsgElemWTPBoardData(CWProtocolMessage *msgPtr) 
{
	const int VENDOR_ID_LENGTH = 4; 	//Vendor Identifier is 4 bytes long
	const int TLV_HEADER_LENGTH = 4; 	//Type and Length of a TLV field is 4 byte long 
	CWWTPVendorInfos infos;
	int i, size = 0;
	
	if(msgPtr == NULL) return CWErrorRaise(CW_ERROR_WRONG_ARG, NULL);
	 // get infos
	if(!CWWTPGetBoardData(&infos)) {
		return CW_FALSE;
	}
	
	//Calculate msg elem size
	size = VENDOR_ID_LENGTH;
	for(i = 0; i < infos.vendorInfosCount; i++) 
		{size += (TLV_HEADER_LENGTH + ((infos.vendorInfos)[i]).length);}
	
	// create message
	CW_CREATE_PROTOCOL_MESSAGE(*msgPtr, size, return CWErrorRaise(CW_ERROR_OUT_OF_MEMORY, NULL););
	
	CWProtocolStore32(msgPtr, ((infos.vendorInfos)[0].vendorIdentifier));
	for(i = 0; i < infos.vendorInfosCount; i++) 
	{
		CWProtocolStore16(msgPtr, ((infos.vendorInfos)[i].type));
		CWProtocolStore16(msgPtr, ((infos.vendorInfos)[i].length));
		
		if((infos.vendorInfos)[i].length == 4) {
			*((infos.vendorInfos)[i].valuePtr) = htonl(*((infos.vendorInfos)[i].valuePtr));
		}
	
		CWProtocolStoreRawBytes(msgPtr, (char*) ((infos.vendorInfos)[i].valuePtr), (infos.vendorInfos)[i].length);
		
//		CWDebugLog("Board Data: %d - %d - %d - %d", (infos.vendorInfos)[i].vendorIdentifier, (infos.vendorInfos)[i].type, (infos.vendorInfos)[i].length, *((infos.vendorInfos)[i].valuePtr));
	}
	CWWTPDestroyVendorInfos(&infos);
	return CWAssembleMsgElem(msgPtr, CW_MSG_ELEMENT_WTP_BOARD_DATA_CW_TYPE);
}
CWBool CWAssembleMsgElemWTPDescriptor(CWProtocolMessage *msgPtr) {
	const int GENERIC_RADIO_INFO_LENGTH = 4;//First 4 bytes for Max Radios, Radios In Use and Encryption Capability 
	const int VENDOR_ID_LENGTH = 4; 	//Vendor Identifier is 4 bytes long
	const int TLV_HEADER_LENGTH = 4; 	//Type and Length of a TLV field is 4 byte long 
	CWWTPVendorInfos infos;
	int i, size = 0;
	
	if(msgPtr == NULL) return CWErrorRaise(CW_ERROR_WRONG_ARG, NULL);
	// get infos
	if(!CWWTPGetVendorInfos(&infos)) { 
		return CW_FALSE;
	}
	
	//Calculate msg elem size
	size = GENERIC_RADIO_INFO_LENGTH;
	for(i = 0; i < infos.vendorInfosCount; i++) 
		{size += (VENDOR_ID_LENGTH + TLV_HEADER_LENGTH + ((infos.vendorInfos)[i]).length);}
		
	// create message
	CW_CREATE_PROTOCOL_MESSAGE(*msgPtr, size, return CWErrorRaise(CW_ERROR_OUT_OF_MEMORY, NULL););
	
	CWProtocolStore8(msgPtr, CWWTPGetMaxRadios()); // number of radios supported by the WTP
	CWProtocolStore8(msgPtr, CWWTPGetRadiosInUse()); // number of radios present in the WTP
	CWProtocolStore16(msgPtr, CWWTPGetEncCapabilities()); // encryption capabilities
 
	for(i = 0; i < infos.vendorInfosCount; i++) {
		CWProtocolStore32(msgPtr, ((infos.vendorInfos)[i].vendorIdentifier));
		CWProtocolStore16(msgPtr, ((infos.vendorInfos)[i].type));
		CWProtocolStore16(msgPtr, ((infos.vendorInfos)[i].length));
		
		if((infos.vendorInfos)[i].length == 4) {
			*((infos.vendorInfos)[i].valuePtr) = htonl(*((infos.vendorInfos)[i].valuePtr));
		}
	
		CWProtocolStoreRawBytes(msgPtr, (char*) ((infos.vendorInfos)[i].valuePtr), (infos.vendorInfos)[i].length);
//		CWDebugLog("WTP Descriptor Vendor ID: %d", (infos.vendorInfos)[i].vendorIdentifier);
//		CWDebugLog("WTP Descriptor Type: %d", (infos.vendorInfos)[i].type);
//		CWDebugLog("WTP Descriptor Length: %d", (infos.vendorInfos)[i].length);
//		CWDebugLog("WTP Descriptor Value: %d", *((infos.vendorInfos)[i].valuePtr));
		//CWDebugLog("Vendor Info \"%d\" = %d - %d - %d", i, (infos.vendorInfos)[i].vendorIdentifier, (infos.vendorInfos)[i].type, (infos.vendorInfos)[i].length);
	}
	
	CWWTPDestroyVendorInfos(&infos);
	
	return CWAssembleMsgElem(msgPtr, CW_MSG_ELEMENT_WTP_DESCRIPTOR_CW_TYPE);
}
CWBool CWAssembleMsgElemWTPFrameTunnelMode(CWProtocolMessage *msgPtr) {
	if(msgPtr == NULL) return CWErrorRaise(CW_ERROR_WRONG_ARG, NULL);
	
	// create message
	CW_CREATE_PROTOCOL_MESSAGE(*msgPtr, 1, return CWErrorRaise(CW_ERROR_OUT_OF_MEMORY, NULL););
	
//	CWDebugLog("Frame Tunnel Mode: %d", CWWTPGetFrameTunnelMode());
	CWProtocolStore8(msgPtr, CWWTPGetFrameTunnelMode()); // frame encryption
	return CWAssembleMsgElem(msgPtr, CW_MSG_ELEMENT_WTP_FRAME_TUNNEL_MODE_CW_TYPE);
}
CWBool CWAssembleMsgElemWTPIPv4Address(CWProtocolMessage *msgPtr) {
	if(msgPtr == NULL) return CWErrorRaise(CW_ERROR_WRONG_ARG, NULL);
	
	// create message
	CW_CREATE_PROTOCOL_MESSAGE(*msgPtr, 4, return CWErrorRaise(CW_ERROR_OUT_OF_MEMORY, NULL););
//	CWDebugLog("WTP IPv4 Address: %d", CWWTPGetIPv4Address());
	CWProtocolStore32(msgPtr, CWWTPGetIPv4Address());
	return CWAssembleMsgElem(msgPtr, CW_MSG_ELEMENT_WTP_IPV4_ADDRESS_CW_TYPE);
}
CWBool CWAssembleMsgElemWTPMACType(CWProtocolMessage *msgPtr) {
	if(msgPtr == NULL) return CWErrorRaise(CW_ERROR_WRONG_ARG, NULL);
	
	// create message
	CW_CREATE_PROTOCOL_MESSAGE(*msgPtr, 1, return CWErrorRaise(CW_ERROR_OUT_OF_MEMORY, NULL););
	
//	CWDebugLog("WTP MAC Type: %d", CWWTPGetMACType());
	CWProtocolStore8(msgPtr, CWWTPGetMACType()); // mode of operation of the WTP (local, split, ...)
	
	return CWAssembleMsgElem(msgPtr, CW_MSG_ELEMENT_WTP_MAC_TYPE_CW_TYPE);
}
CWBool CWAssembleMsgElemWTPName(CWProtocolMessage *msgPtr) {
	char *name;
	
	if(msgPtr == NULL) return CWErrorRaise(CW_ERROR_WRONG_ARG, NULL);
	
	name = CWWTPGetName();
	
	// create message
	CW_CREATE_PROTOCOL_MESSAGE(*msgPtr, strlen(name), return CWErrorRaise(CW_ERROR_OUT_OF_MEMORY, NULL););
	
//	CWDebugLog("WTPName: %s", name);
	CWProtocolStoreStr(msgPtr, name);
	
	return CWAssembleMsgElem(msgPtr, CW_MSG_ELEMENT_WTP_NAME_CW_TYPE);	
}
CWBool CWAssembleMsgElemWTPOperationalStatistics(CWProtocolMessage *msgPtr, int radio)
{
	const int operational_statistics_length= 4;
	if(msgPtr == NULL) return CWErrorRaise(CW_ERROR_WRONG_ARG, NULL);
	if(radio<0 || radio>=gRadiosInfo.radioCount) return CWErrorRaise(CW_ERROR_WRONG_ARG, NULL);
	
	CW_CREATE_PROTOCOL_MESSAGE(*msgPtr, operational_statistics_length, return CWErrorRaise(CW_ERROR_OUT_OF_MEMORY, NULL););
	
	CWProtocolStore8(msgPtr, radio);
	CWProtocolStore8(msgPtr, gRadiosInfo.radiosInfo[radio].TxQueueLevel);
	CWProtocolStore16(msgPtr, gRadiosInfo.radiosInfo[radio].wirelessLinkFramesPerSec);
//	CWDebugLog("");	
//	CWDebugLog("WTPOperationalStatistics of radio \"%d\": %d - %d", radio,gRadiosInfo.radiosInfo[radio].TxQueueLevel,  gRadiosInfo.radiosInfo[radio].wirelessLinkFramesPerSec);
	CWAssembleMsgElem(msgPtr, CW_MSG_ELEMENT_WTP_OPERAT_STATISTICS_CW_TYPE);
	
	return CW_TRUE;	
}
CWBool CWAssembleMsgElemWTPRadioStatistics(CWProtocolMessage *msgPtr, int radio)
{
	const int radio_statistics_length= 20;
	if(msgPtr == NULL) return CWErrorRaise(CW_ERROR_WRONG_ARG, NULL);
	if(radio<0 || radio>gRadiosInfo.radioCount) return CWErrorRaise(CW_ERROR_WRONG_ARG, NULL);
	
	CW_CREATE_PROTOCOL_MESSAGE(*msgPtr, radio_statistics_length, return CWErrorRaise(CW_ERROR_OUT_OF_MEMORY, NULL););
	CWProtocolStore8(msgPtr, radio);
	CWProtocolStore8(msgPtr, gRadiosInfo.radiosInfo[radio].statistics.lastFailureType);
	CWProtocolStore16(msgPtr, gRadiosInfo.radiosInfo[radio].statistics.resetCount);
	CWProtocolStore16(msgPtr, gRadiosInfo.radiosInfo[radio].statistics.SWFailureCount);
	CWProtocolStore16(msgPtr, gRadiosInfo.radiosInfo[radio].statistics.HWFailuireCount);
	CWProtocolStore16(msgPtr, gRadiosInfo.radiosInfo[radio].statistics.otherFailureCount);
	CWProtocolStore16(msgPtr, gRadiosInfo.radiosInfo[radio].statistics.unknownFailureCount);
	CWProtocolStore16(msgPtr, gRadiosInfo.radiosInfo[radio].statistics.configUpdateCount);
	CWProtocolStore16(msgPtr, gRadiosInfo.radiosInfo[radio].statistics.channelChangeCount);
	CWProtocolStore16(msgPtr, gRadiosInfo.radiosInfo[radio].statistics.bandChangeCount);
	CWProtocolStore16(msgPtr, gRadiosInfo.radiosInfo[radio].statistics.currentNoiseFloor);
//	CWDebugLog("");
//	CWDebugLog("WTPRadioStatistics of radio: \"%d\"", radio);
//	CWDebugLog("WTPRadioStatistics(1): %d - %d - %d", gRadiosInfo.radiosInfo[radio].statistics.lastFailureType, gRadiosInfo.radiosInfo[radio].statistics.resetCount, gRadiosInfo.radiosInfo[radio].statistics.SWFailureCount);
//	CWDebugLog("WTPRadioStatistics(2): %d - %d - %d", gRadiosInfo.radiosInfo[radio].statistics.HWFailuireCount, gRadiosInfo.radiosInfo[radio].statistics.otherFailureCount, gRadiosInfo.radiosInfo[radio].statistics.unknownFailureCount);
//	CWDebugLog("WTPRadioStatistics(3): %d - %d - %d - %d", gRadiosInfo.radiosInfo[radio].statistics.configUpdateCount, gRadiosInfo.radiosInfo[radio].statistics.channelChangeCount,gRadiosInfo.radiosInfo[radio].statistics.bandChangeCount,gRadiosInfo.radiosInfo[radio].statistics.currentNoiseFloor);
	//return CWAssembleMsgElem(msgPtr, CW_MSG_ELEMENT_WTP_RADIO_STATISTICS_CW_TYPE);
	CWAssembleMsgElem(msgPtr, CW_MSG_ELEMENT_WTP_RADIO_STATISTICS_CW_TYPE);
	
	return CW_TRUE;
}
CWBool CWAssembleMsgElemWTPRebootStatistics(CWProtocolMessage *msgPtr)
{
	const int reboot_statistics_length= 15;
	
	if(msgPtr == NULL) return CWErrorRaise(CW_ERROR_WRONG_ARG, NULL);
	
	CW_CREATE_PROTOCOL_MESSAGE(*msgPtr, reboot_statistics_length, return CWErrorRaise(CW_ERROR_OUT_OF_MEMORY, NULL););
	
	CWProtocolStore16(msgPtr, gWTPRebootStatistics.rebootCount);
	CWProtocolStore16(msgPtr, gWTPRebootStatistics.ACInitiatedCount);
	CWProtocolStore16(msgPtr, gWTPRebootStatistics.linkFailurerCount);
	CWProtocolStore16(msgPtr, gWTPRebootStatistics.SWFailureCount);
	CWProtocolStore16(msgPtr, gWTPRebootStatistics.HWFailuireCount);
	CWProtocolStore16(msgPtr, gWTPRebootStatistics.otherFailureCount);
	CWProtocolStore16(msgPtr, gWTPRebootStatistics.unknownFailureCount);
	CWProtocolStore8(msgPtr, gWTPRebootStatistics.lastFailureType);
//	CWDebugLog("");	
//	CWDebugLog("WTPRebootStat(1): %d - %d - %d", gWTPRebootStatistics.rebootCount, gWTPRebootStatistics.ACInitiatedCount, gWTPRebootStatistics.linkFailurerCount);
//	CWDebugLog("WTPRebootStat(2): %d - %d - %d", gWTPRebootStatistics.SWFailureCount, gWTPRebootStatistics.HWFailuireCount, gWTPRebootStatistics.otherFailureCount);
//	CWDebugLog("WTPRebootStat(3): %d - %d", gWTPRebootStatistics.unknownFailureCount, gWTPRebootStatistics.lastFailureType);
	return CWAssembleMsgElem(msgPtr, CW_MSG_ELEMENT_WTP_REBOOT_STATISTICS_CW_TYPE);
}
//test version
CWBool CWAssembleMsgElemDuplicateIPv4Address(CWProtocolMessage *msgPtr) {
	const int duplicate_ipv4_length= 11;
	char *macAddress;
	if(msgPtr == NULL) return CWErrorRaise(CW_ERROR_WRONG_ARG, NULL);
	
	// create message
	CW_CREATE_PROTOCOL_MESSAGE(*msgPtr, duplicate_ipv4_length, return CWErrorRaise(CW_ERROR_OUT_OF_MEMORY, NULL););
//	CWDebugLog("");	
//	CWDebugLog("Duplicate IPv4 Address: %d", CWWTPGetIPv4Address());
	
	CWProtocolStore32(msgPtr, CWWTPGetIPv4Address());
	CWProtocolStore8(msgPtr, CWWTPGetIPv4StatusDuplicate());
	CWProtocolStore8(msgPtr, 6);
	CW_CREATE_ARRAY_ERR(macAddress, 6, char, return CWErrorRaise(CW_ERROR_OUT_OF_MEMORY, NULL););
	macAddress[0] = 103;
	macAddress[1] = 204;
	macAddress[2] = 204;
	macAddress[3] = 190;
	macAddress[4] = 180;
	macAddress[5] = 0;
	
	CWProtocolStoreRawBytes(msgPtr, macAddress, 6);
	CW_FREE_OBJECT(macAddress);
	//CWProtocolStore8(msgPtr, CWWTPGetIPv4StatusDuplicate());
	return CWAssembleMsgElem(msgPtr, CW_MSG_ELEMENT_DUPLICATE_IPV4_ADDRESS_CW_TYPE);
}
//test version
CWBool CWAssembleMsgElemDuplicateIPv6Address(CWProtocolMessage *msgPtr) {
	const int duplicate_ipv6_length= 23;
	char *macAddress;
	if(msgPtr == NULL) return CWErrorRaise(CW_ERROR_WRONG_ARG, NULL);
	
	// create message
	CW_CREATE_PROTOCOL_MESSAGE(*msgPtr, duplicate_ipv6_length, return CWErrorRaise(CW_ERROR_OUT_OF_MEMORY, NULL););
//	CWDebugLog("");	
//	CWDebugLog("Duplicate IPv6 Address");
	
	struct sockaddr_in6 myAddr;
	CWWTPGetIPv6Address(&myAddr);
	CWProtocolStoreRawBytes(msgPtr, (char*)myAddr.sin6_addr.s6_addr, 16);
	CWProtocolStore8(msgPtr, CWWTPGetIPv6StatusDuplicate());
	
	CWProtocolStore8(msgPtr, 6);
	CW_CREATE_ARRAY_ERR(macAddress, 6, char, return CWErrorRaise(CW_ERROR_OUT_OF_MEMORY, NULL););
	macAddress[0] = 103;
	macAddress[1] = 204;
	macAddress[2] = 204;
	macAddress[3] = 190;
	macAddress[4] = 180;
	macAddress[5] = 0;
	
	CWProtocolStoreRawBytes(msgPtr, macAddress, 6);
	CW_FREE_OBJECT(macAddress);
	//CWProtocolStore8(msgPtr, CWWTPGetIPv6StatusDuplicate());
	return CWAssembleMsgElem(msgPtr, CW_MSG_ELEMENT_DUPLICATE_IPV6_ADDRESS_CW_TYPE);
}
CWBool CWAssembleMsgElemRadioAdminState(CWProtocolMessage *msgPtr) 
{
	const int radio_Admin_State_Length=2;
	CWRadiosAdminInfo infos;
	CWProtocolMessage *msgs;
	int len = 0;
	int i;
	int j;
	
	if(msgPtr == NULL) return CWErrorRaise(CW_ERROR_WRONG_ARG, NULL);
	
	if(!CWGetWTPRadiosAdminState(&infos)) {
		return CW_FALSE;
	}
	
	CW_CREATE_PROTOCOL_MSG_ARRAY_ERR(msgs, (infos.radiosCount), return  CWErrorRaise(CW_ERROR_OUT_OF_MEMORY, NULL););
	
	for(i = 0; i < infos.radiosCount; i++) {
		// create message
		CW_CREATE_PROTOCOL_MESSAGE(msgs[i], radio_Admin_State_Length, return CWErrorRaise(CW_ERROR_OUT_OF_MEMORY, NULL););
		CWProtocolStore8(&(msgs[i]), infos.radios[i].ID); // ID of the radio
		CWProtocolStore8(&(msgs[i]), infos.radios[i].state); // state of the radio
		//CWProtocolStore8(&(msgs[i]), infos.radios[i].cause);
		
		if(!(CWAssembleMsgElem(&(msgs[i]), CW_MSG_ELEMENT_RADIO_ADMIN_STATE_CW_TYPE))) {
			for(j = i; j >= 0; j--) { CW_FREE_PROTOCOL_MESSAGE(msgs[j]);}
			CW_FREE_OBJECT(infos.radios);
			CW_FREE_OBJECT(msgs);
			return CW_FALSE;
		}
		
		len += msgs[i].offset;
//		CWDebugLog("Radio Admin State: %d - %d - %d", infos.radios[i].ID, infos.radios[i].state, infos.radios[i].cause);
	}
	
	CW_CREATE_PROTOCOL_MESSAGE(*msgPtr, len, return CWErrorRaise(CW_ERROR_OUT_OF_MEMORY, NULL););
	
	for(i = 0; i < infos.radiosCount; i++) {
		CWProtocolStoreMessage(msgPtr, &(msgs[i]));
		CW_FREE_PROTOCOL_MESSAGE(msgs[i]);
	}
	
	CW_FREE_OBJECT(msgs);
	CW_FREE_OBJECT(infos.radios);
	return CW_TRUE;
}
//if radioID is negative return Radio Operational State for all radios
CWBool CWAssembleMsgElemRadioOperationalState(int radioID, CWProtocolMessage *msgPtr) 
{
	const int radio_Operational_State_Length=3;
	CWRadiosOperationalInfo infos;
	CWProtocolMessage *msgs;
	int len = 0;
	int i;
	if(msgPtr == NULL) return CWErrorRaise(CW_ERROR_WRONG_ARG, NULL);
	if(!(CWGetWTPRadiosOperationalState(radioID,&infos))) {
		return CW_FALSE;
	}
	CW_CREATE_PROTOCOL_MSG_ARRAY_ERR(msgs, (infos.radiosCount), return  CWErrorRaise(CW_ERROR_OUT_OF_MEMORY, NULL););
	
	for(i = 0; i < infos.radiosCount; i++) {
		// create message
		CW_CREATE_PROTOCOL_MESSAGE(msgs[i], radio_Operational_State_Length, return CWErrorRaise(CW_ERROR_OUT_OF_MEMORY, NULL););
		CWProtocolStore8(&(msgs[i]), infos.radios[i].ID); // ID of the radio
		CWProtocolStore8(&(msgs[i]), infos.radios[i].state); // state of the radio
		CWProtocolStore8(&(msgs[i]), infos.radios[i].cause);
		
		if(!(CWAssembleMsgElem(&(msgs[i]), CW_MSG_ELEMENT_RADIO_OPERAT_STATE_CW_TYPE))) {
			int j;
			for(j = i; j >= 0; j--) { CW_FREE_PROTOCOL_MESSAGE(msgs[j]);}
			CW_FREE_OBJECT(infos.radios);
			CW_FREE_OBJECT(msgs);
			return CW_FALSE;
		}
		
		len += msgs[i].offset;
//		CWDebugLog("Radio Operational State: %d - %d - %d", infos.radios[i].ID, infos.radios[i].state, infos.radios[i].cause);
	}
	
	CW_CREATE_PROTOCOL_MESSAGE(*msgPtr, len, return CWErrorRaise(CW_ERROR_OUT_OF_MEMORY, NULL););
	
	for(i = 0; i < infos.radiosCount; i++) {
		CWProtocolStoreMessage(msgPtr, &(msgs[i]));
		CW_FREE_PROTOCOL_MESSAGE(msgs[i]);
	}
	
	CW_FREE_OBJECT(msgs);
	CW_FREE_OBJECT(infos.radios);
	return CW_TRUE;
}
CWBool CWAssembleMsgElemDecryptErrorReport(CWProtocolMessage *msgPtr, int radioID) 
{
	int decrypy_Error_Report_Length=0;
	CWDecryptErrorReportInfo infos;
	CWProtocolMessage *msgs;
	int len = 0;
	int i;
	if(msgPtr == NULL) return CWErrorRaise(CW_ERROR_WRONG_ARG, NULL);
	if(!(CWGetDecryptErrorReport(radioID,&infos))) {
		return CW_FALSE;
	}
	CW_CREATE_PROTOCOL_MSG_ARRAY_ERR(msgs, (infos.radiosCount), return  CWErrorRaise(CW_ERROR_OUT_OF_MEMORY, NULL););
	
	for(i = 0; i < infos.radiosCount; i++) {
		// create message
		decrypy_Error_Report_Length = 2 + sizeof(CWMACAddress)*(infos.radios[i].numEntries); 
		
		CW_CREATE_PROTOCOL_MESSAGE(msgs[i], decrypy_Error_Report_Length, return CWErrorRaise(CW_ERROR_OUT_OF_MEMORY, NULL););
		CWProtocolStore8(&(msgs[i]), infos.radios[i].ID); // ID of the radio
		CWProtocolStore8(&(msgs[i]), infos.radios[i].numEntries); // state of the radio
		CWProtocolStore8(&(msgs[i]), (unsigned char)sizeof(CWMACAddress)*(infos.radios[i].numEntries));
		CWProtocolStoreRawBytes(&(msgs[i]), (char*)*(infos.radios[i].decryptErrorMACAddressList), sizeof(CWMACAddress)*(infos.radios[i].numEntries));
		
		/*
		CWDebugLog("###numEntries = %d", infos.radios[i].numEntries);
		CWDebugLog("j = %d", sizeof(CWMACAddress)*(infos.radios[i].numEntries));
		
		int j;
		for (j=(sizeof(CWMACAddress)*(infos.radios[i].numEntries)); j>0; j--)
			CWDebugLog("##(%d/6) = %d", j, msgs[i].msg[(msgs[i].offset)-j]);
		*/
		
		if(!(CWAssembleMsgElem(&(msgs[i]), CW_MSG_ELEMENT_CW_DECRYPT_ER_REPORT_CW_TYPE))) {
			int j;
			for(j = i; j >= 0; j--) { CW_FREE_PROTOCOL_MESSAGE(msgs[j]);}
			for(j=0; j<infos.radiosCount; j++) {CW_FREE_OBJECT(infos.radios[j].decryptErrorMACAddressList);}
			CW_FREE_OBJECT(infos.radios);
			CW_FREE_OBJECT(msgs);
			return CW_FALSE;
		}
		
		len += msgs[i].offset;
//		CWDebugLog("Radio Decrypt Error Report of radio \"%d\" = %d", infos.radios[i].ID, infos.radios[i].numEntries);
	}
	
	CW_CREATE_PROTOCOL_MESSAGE(*msgPtr, len, return CWErrorRaise(CW_ERROR_OUT_OF_MEMORY, NULL););
	
	for(i = 0; i < infos.radiosCount; i++) {
		CWProtocolStoreMessage(msgPtr, &(msgs[i]));
		CW_FREE_PROTOCOL_MESSAGE(msgs[i]);
	}
	
	for(i = 0; i < infos.radiosCount; i++) {
		CW_FREE_OBJECT(infos.radios[i].decryptErrorMACAddressList);
	}
	
	CW_FREE_OBJECT(msgs);
	CW_FREE_OBJECT(infos.radios);
	return CW_TRUE;
	
}
/*
CWBool CWAssembleMsgElemWTPRadioInformation(CWProtocolMessage *msgPtr) {
	CWProtocolMessage *msgs;
	CWRadiosInformation infos;
	
	int len = 0;
	int i;
	
	if(msgPtr == NULL) return CWErrorRaise(CW_ERROR_WRONG_ARG, NULL);
	
	CWDebugLog("Assemble WTP Radio Info");
	
	if(!CWWTPGetRadiosInformation(&infos)) {
		return CW_FALSE;
	}
	
	// create one message element for each radio
	
	CW_CREATE_ARRAY_ERR(msgs, (infos.radiosCount), CWProtocolMessage, return CWErrorRaise(CW_ERROR_OUT_OF_MEMORY, NULL););
	
	for(i = 0; i < infos.radiosCount; i++) {
		// create message
		CW_CREATE_PROTOCOL_MESSAGE(msgs[i], 5, return CWErrorRaise(CW_ERROR_OUT_OF_MEMORY, NULL););
		CWProtocolStore8(&(msgs[i]), infos.radios[i].ID); // ID of the radio
		CWProtocolStore32(&(msgs[i]), infos.radios[i].type); // type of the radio
		
		CWDebugLog("WTPRadioInformation: %d - %d", infos.radios[i].ID, infos.radios[i].type);
		
		if(!(CWAssembleMsgElem(&(msgs[i]), CW_MSG_ELEMENT_WTP_RADIO_INFO_CW_TYPE))) {
			int j;
			for(j = i; j >= 0; j--) { CW_FREE_PROTOCOL_MESSAGE(msgs[j]);}
			CW_FREE_OBJECT(infos.radios);
			CW_FREE_OBJECT(msgs);
			return CW_FALSE;
		}
		
		len += msgs[i].offset;
	}
	
	// return all the messages as one big message
	CW_CREATE_PROTOCOL_MESSAGE(*msgPtr, len, return CWErrorRaise(CW_ERROR_OUT_OF_MEMORY, NULL););
	
	for(i = 0; i < infos.radiosCount; i++) {
		CWProtocolStoreMessage(msgPtr, &(msgs[i]));
		CW_FREE_PROTOCOL_MESSAGE(msgs[i]);
	}
	
	CW_FREE_OBJECT(msgs);
	CW_FREE_OBJECT(infos.radios);
	
	return CW_TRUE;
}
*/
/*_________________________________________________________________________*/
/*  *****************************___PARSE___*****************************  */
CWBool CWParseACDescriptor(CWProtocolMessage *msgPtr, int len, CWACInfoValues *valPtr) {
	int i=0, theOffset=0;
		
	CWParseMessageElementStart();
	
	valPtr->stations= CWProtocolRetrieve16(msgPtr);
//	CWDebugLog("AC Descriptor Stations: %d", valPtr->stations);
	
	valPtr->limit	= CWProtocolRetrieve16(msgPtr);
//	CWDebugLog("AC Descriptor Limit: %d", valPtr->limit);
	
	valPtr->activeWTPs= CWProtocolRetrieve16(msgPtr);
//	CWDebugLog("AC Descriptor Active WTPs: %d", valPtr->activeWTPs);
	
	valPtr->maxWTPs	= CWProtocolRetrieve16(msgPtr);
//	CWDebugLog("AC Descriptor Max WTPs: %d",	valPtr->maxWTPs);
	
	valPtr->security= CWProtocolRetrieve8(msgPtr);
//	CWDebugLog("AC Descriptor Security: %d",	valPtr->security);
	
	valPtr->RMACField= CWProtocolRetrieve8(msgPtr);
//	CWDebugLog("AC Descriptor Radio MAC Field: %d",	valPtr->security);
//	valPtr->WirelessField= CWProtocolRetrieve8(msgPtr);
//	CWDebugLog("AC Descriptor Wireless Field: %d",	valPtr->security);
	CWProtocolRetrieve8(msgPtr);			//Reserved
	valPtr->DTLSPolicy= CWProtocolRetrieve8(msgPtr); // DTLS Policy
//	CWDebugLog("DTLS Policy: %d",	valPtr->DTLSPolicy);
	valPtr->vendorInfos.vendorInfosCount = 0;
	
	theOffset = msgPtr->offset;
	
	// see how many vendor ID we have in the message
	while((msgPtr->offset-oldOffset) < len) {	// oldOffset stores msgPtr->offset's value at the beginning of this function.
							// See the definition of the CWParseMessageElementStart() macro.
		int tmp, id=0, type=0;		
		//CWDebugLog("differenza:%d, offset:%d, oldOffset:%d", (msgPtr->offset-oldOffset), (msgPtr->offset), oldOffset);
		id=CWProtocolRetrieve32(msgPtr);
//		CWDebugLog("ID: %d", id); // ID
		
		type=CWProtocolRetrieve16(msgPtr);
//		CWDebugLog("TYPE: %d",type); // type
		
		tmp = CWProtocolRetrieve16(msgPtr);
		msgPtr->offset += tmp; // len
//		CWDebugLog("offset %d", msgPtr->offset);
		valPtr->vendorInfos.vendorInfosCount++;
	}
	
	msgPtr->offset = theOffset;
	
	// actually read each vendor ID
	CW_CREATE_ARRAY_ERR(valPtr->vendorInfos.vendorInfos, valPtr->vendorInfos.vendorInfosCount, CWACVendorInfoValues,
		return CWErrorRaise(CW_ERROR_OUT_OF_MEMORY, NULL););
//	CWDebugLog("len %d", len);
//	CWDebugLog("vendorInfosCount %d", valPtr->vendorInfos.vendorInfosCount);
	for(i = 0; i < valPtr->vendorInfos.vendorInfosCount; i++) {
//		CWDebugLog("vendorInfosCount %d vs %d", i, valPtr->vendorInfos.vendorInfosCount);
		(valPtr->vendorInfos.vendorInfos)[i].vendorIdentifier = CWProtocolRetrieve32(msgPtr);
		(valPtr->vendorInfos.vendorInfos)[i].type = CWProtocolRetrieve16(msgPtr);																
		(valPtr->vendorInfos.vendorInfos)[i].length = CWProtocolRetrieve16(msgPtr);
		(valPtr->vendorInfos.vendorInfos)[i].valuePtr = (int*) (CWProtocolRetrieveRawBytes(msgPtr, (valPtr->vendorInfos.vendorInfos)[i].length));
		
		if((valPtr->vendorInfos.vendorInfos)[i].valuePtr == NULL) return CWErrorRaise(CW_ERROR_OUT_OF_MEMORY, NULL);
		
		if((valPtr->vendorInfos.vendorInfos)[i].length == 4) {
			*((valPtr->vendorInfos.vendorInfos)[i].valuePtr) = ntohl(*((valPtr->vendorInfos.vendorInfos)[i].valuePtr));
		}
		
//		CWDebugLog("AC Descriptor Vendor ID: %d", (valPtr->vendorInfos.vendorInfos)[i].vendorIdentifier);
//		CWDebugLog("AC Descriptor Type: %d", (valPtr->vendorInfos.vendorInfos)[i].type);
//		CWDebugLog("AC Descriptor Value: %d", *((valPtr->vendorInfos.vendorInfos)[i].valuePtr));
	}		
//	CWDebugLog("AC Descriptor Out");
	CWParseMessageElementEnd();
}
CWBool CWParseACIPv4List(CWProtocolMessage *msgPtr, int len, ACIPv4ListValues *valPtr) {
	int i;
	CWParseMessageElementStart();
	
	if(len == 0 || ((len % 4) != 0)) return CWErrorRaise(CW_ERROR_INVALID_FORMAT, "Malformed AC IPv4 List Messame Element");
	
	valPtr->ACIPv4ListCount = (len/4);
	
	CW_CREATE_ARRAY_ERR(valPtr->ACIPv4List, valPtr->ACIPv4ListCount, int, return CWErrorRaise(CW_ERROR_OUT_OF_MEMORY, NULL););
	
	for(i = 0; i < valPtr->ACIPv4ListCount; i++) {
		struct sockaddr_in addr;
		(valPtr->ACIPv4List)[i] = CWProtocolRetrieve32(msgPtr);
//		CWDebugLog("AC IPv4 List (%d): %d", i+1, (valPtr->ACIPv4List)[i]);
		addr.sin_addr.s_addr = (valPtr->ACIPv4List)[i];
		addr.sin_family = AF_INET;
		addr.sin_port = 1024;
		CWUseSockNtop(&addr, CWDebugLog(str););
	}
	
	CWParseMessageElementEnd();
}
CWBool CWParseACIPv6List(CWProtocolMessage *msgPtr, int len, ACIPv6ListValues *valPtr) 
{
	int i;
	CWParseMessageElementStart();
	
	if(len == 0 || ((len % 16) != 0)) return CWErrorRaise(CW_ERROR_INVALID_FORMAT, "Malformed AC IPv6 List Messame Element");
	
	valPtr->ACIPv6ListCount = (len/16);
	
	CW_CREATE_ARRAY_ERR(valPtr->ACIPv6List, valPtr->ACIPv6ListCount, struct in6_addr, return CWErrorRaise(CW_ERROR_OUT_OF_MEMORY, NULL););
	
	for(i = 0; i < valPtr->ACIPv6ListCount; i++) {
		struct sockaddr_in6 addr;
   		
		/*
                 * BUG ML09
                 * 19/10/2009 - Donato Capitella
                 */                
		void *ptr;
                ptr =  CWProtocolRetrieveRawBytes(msgPtr, 16);
                CW_COPY_MEMORY(&((valPtr->ACIPv6List)[i]), ptr, 16);
                CW_FREE_OBJECT(ptr);
		CW_COPY_MEMORY(&(addr.sin6_addr), &((valPtr->ACIPv6List)[i]), 16);
		addr.sin6_family = AF_INET6;
		addr.sin6_port = htons(CW_CONTROL_PORT);
		
//		CWUseSockNtop(&addr, CWDebugLog("AC IPv6 List: %s",str););
	}
	
	CWParseMessageElementEnd();
}
CWBool CWParseAddStation(CWProtocolMessage *msgPtr, int len) 
{
	int radioID=0,Length=0;
	unsigned char* StationMacAddress;
	
	//CWParseMessageElementStart();	 sostituire al posto delle righe successive quando passerò valPtr alla funzione CWarseAddStation
	/*--------------------------------------------------------------------------------------*/
	int oldOffset;												
			if(msgPtr == NULL) return CWErrorRaise(CW_ERROR_WRONG_ARG, NULL);	
						oldOffset = msgPtr->offset;
	/*----------------------------------------------------------------------------------*/
	
	
	radioID = CWProtocolRetrieve8(msgPtr);
	//CWDebugLog("radio ID %d",radioID);
	Length = CWProtocolRetrieve8(msgPtr);
	//CWDebugLog("Length of mac address field %d",Length);
	StationMacAddress = (unsigned char*)CWProtocolRetrieveRawBytes(msgPtr, Length);
	
	    
      
	CWDebugLog("STATION'S MAC ADDRESS TO FORWARD TRAFFIC: %02X:%02X:%02X:%02X:%02X:%02X",  
								StationMacAddress[0] & 0xFF,
      								StationMacAddress[1] & 0xFF,
      								StationMacAddress[2] & 0xFF,
      								StationMacAddress[3] & 0xFF,
      								StationMacAddress[4] & 0xFF,
      								StationMacAddress[5] & 0xFF);
	
	CWParseMessageElementEnd();  
}
CWBool CWParseCWControlIPv4Addresses(CWProtocolMessage *msgPtr, int len, CWProtocolIPv4NetworkInterface *valPtr) {
	CWParseMessageElementStart();
	valPtr->addr.sin_addr.s_addr = htonl(CWProtocolRetrieve32(msgPtr));
	valPtr->addr.sin_family = AF_INET;
	valPtr->addr.sin_port = htons(CW_CONTROL_PORT);
	
	CWUseSockNtop((&(valPtr->addr)), CWDebugLog("Interface Address: %s", str););
	
	valPtr->WTPCount = CWProtocolRetrieve16(msgPtr);
//	CWDebugLog("WTP Count: %d",	valPtr->WTPCount);
	
	CWParseMessageElementEnd();
}
CWBool CWParseCWControlIPv6Addresses(CWProtocolMessage *msgPtr, int len, CWProtocolIPv6NetworkInterface *valPtr) {
	CWParseMessageElementStart();
	
	CW_COPY_MEMORY(&(valPtr->addr.sin6_addr), CWProtocolRetrieveRawBytes(msgPtr, 16), 16);
	valPtr->addr.sin6_family = AF_INET6;
	valPtr->addr.sin6_port = htons(CW_CONTROL_PORT);
	
	CWUseSockNtop((&(valPtr->addr)), CWDebugLog("Interface Address: %s", str););
	
	valPtr->WTPCount = CWProtocolRetrieve16(msgPtr);
//	CWDebugLog("WTP Count: %d",	valPtr->WTPCount);
	
	CWParseMessageElementEnd();
}
CWBool CWParseCWTimers (CWProtocolMessage *msgPtr, int len, CWProtocolConfigureResponseValues *valPtr)
{
	CWParseMessageElementStart();
	
	valPtr->discoveryTimer	= CWProtocolRetrieve8(msgPtr);	
//	CWDebugLog("Discovery Timer: %d", valPtr->discoveryTimer);
	valPtr->echoRequestTimer = CWProtocolRetrieve8(msgPtr);
//	CWDebugLog("Echo Timer: %d", valPtr->echoRequestTimer);
	
	CWParseMessageElementEnd();
}
CWBool CWParseDecryptErrorReportPeriod (CWProtocolMessage *msgPtr, int len, WTPDecryptErrorReportValues *valPtr)
{
	CWParseMessageElementStart();
	
	valPtr->radioID = CWProtocolRetrieve8(msgPtr);
	valPtr->reportInterval = CWProtocolRetrieve16(msgPtr);
//	CWDebugLog("Decrypt Error Report Period: %d - %d", valPtr->radioID, valPtr->reportInterval);
	
	CWParseMessageElementEnd();
}
CWBool CWParseIdleTimeout (CWProtocolMessage *msgPtr, int len, CWProtocolConfigureResponseValues *valPtr)
{
	CWParseMessageElementStart();
	
	valPtr->idleTimeout = CWProtocolRetrieve32(msgPtr);	
//	CWDebugLog("Idle Timeout: %d", valPtr->idleTimeout);
		
	CWParseMessageElementEnd();
}
CWBool CWParseWTPFallback (CWProtocolMessage *msgPtr, int len, CWProtocolConfigureResponseValues *valPtr)
{
	CWParseMessageElementStart();
	
	valPtr->fallback = CWProtocolRetrieve8(msgPtr);	
//	CWDebugLog("WTP Fallback: %d", valPtr->fallback);
		
	CWParseMessageElementEnd();
}
void CWWTPResetRebootStatistics(WTPRebootStatisticsInfo *rebootStatistics)
{
	rebootStatistics->rebootCount=0;
	rebootStatistics->ACInitiatedCount=0;
	rebootStatistics->linkFailurerCount=0;
	rebootStatistics->SWFailureCount=0;
	rebootStatistics->HWFailuireCount=0;
	rebootStatistics->otherFailureCount=0;
	rebootStatistics->unknownFailureCount=0;
	rebootStatistics->lastFailureType=NOT_SUPPORTED;
}	
char addr[1000];
char md5[65];
char report_if_name[65];
off_t flen;
int  tftp_port = 69;
char rept_addr[1000];
int  rept_port=12345;
// 20141114 Yuan Hong
#ifdef SINOIX_PAYLOAD

#include "sino_comm.h"
#include "sino_service.h"
#include "getifmac.h"
#include "getifaddr.h"

#include <sys/vfs.h>
#include <pthread.h>
#include <string.h>

#define SINO_CHECK(s)   if (dlen > sizeof(version) - 1) break; check = 1;
#define SINO_STRNCPY(s) do {memset(s,0,sizeof(s)); strncpy((char*)s, (const char*)data->info, dlen);}while(0)
#define SINO_CHECK_STRNCPY(s)   SINO_CHECK(s); SINO_STRNCPY(s);
#define CHILD_EXIT() do { my_exit(0); } while(0)

void my_exit(int n) 
{
    fflush(0);
    _exit(n);
}

// WTP.c
int write_firmware_image_info(const char *path, const char *ver, const char *md5, const char *flen);
int get_report_if_name(char *if_name, int len);
void stat_report(char *addr, int port, int stat, char *ver, char *mac, char *ip, char *ifname);

void CWParseVendorPayloadSinoix (CWProtocolMessage *msgPtr, int len)
{
    CWLog("*** Sinoix Payload Data packet len %d ***", len);
    if (!msgPtr || len > SINOIX_MAX_SIZE)  return;

    char buff[SINOIX_MAX_SIZE];

    CW_COPY_MEMORY(buff, &((msgPtr->msg)[(msgPtr->offset)]), len);
    (msgPtr->offset) += len;

    sino_head *sino = (sino_head*)buff;
    sino_elem *data = sizeof(sino_head) + (void*)sino;
    int i, type, elems = ntohs(sino->elems);
    char bport[6], blen[11];
    int dlen, check = 0; // 1: pass
    char version[65];
    int v1,v2,v3;       //version x.x.x

    if (SI_MAGIC != ntohl(sino->magic)) {
        CWLog("*** Sinoix Payload Data Head check failure %x", ntohl(sino->magic));
        return;
    }

    CWLog("*** Sinoix Payload Data Head check passed");
    CWLog("head elems %d, elems len %d", elems, ntohs(sino->len));

    for (i = 0; i < elems; i++ ) {
        check = 0; type = ntohs(data->type); dlen = ntohs(data->len);
        switch (type) {
            case SI_FI_VERSION: SINO_CHECK_STRNCPY(version); 
                                sscanf(version, "%d.%d.%d", &v1, &v2, &v3);
                                CWLog("  Version %2d [%d.%d.%d]", dlen, v1, v2, v3);
                                break;
            case SI_FI_MD5:     SINO_CHECK_STRNCPY(md5); 
                                CWLog(" Md5 code %2d [%s]", dlen, md5);
                                break;
            case SI_FI_LEN:     SINO_CHECK_STRNCPY(blen); 
                                flen = atoll(blen);
                                CWLog("Firm size %2d [%ld]", dlen, flen);
                                break;
            case SI_TFTP_ADDR:  SINO_CHECK_STRNCPY(addr); 
                                CWLog("TFTP addr %2d [%s]", dlen, addr);
                                break;
            case SI_TFTP_PORT:  SINO_CHECK_STRNCPY(bport); 
                                tftp_port = atoi(bport);
                                CWLog("TFTP port %2d [%d]", dlen, tftp_port);
                                break;
            case SI_REPT_ADDR:  SINO_CHECK_STRNCPY(rept_addr); 
                                CWLog("Rept addr %2d [%s]", dlen, rept_addr);
                                break;
            case SI_REPT_PORT:  SINO_CHECK_STRNCPY(bport); 
                                rept_port = atoi(bport);
                                CWLog("Rept port %2d [%d]", dlen, rept_port);
                                break;
            default: CWLog("Unknown: type %d", type);
                     break;
        }
        if (!check) {
            CWLog("! Vendor Payload check exception: type %d, len %d, info %s",
                    type, dlen, data->info);
            break;
        }
        data = (void*)data + sizeof(sino_elem) + dlen;
    }
    if (check) {
        // 对比从AC传来的版本号和本地最大的版本号, 若AC的大则下载
        if (maxver(v1, v2, v3, maxv1, maxv2, maxv3)) {
            CWLog("Are preparing to update the version %d.%d.%d", v1, v2, v3);
            do {
                // Environment check and create
                struct statfs fs;
                pid_t pid;
                char *tftp = 0, *checker = 0;
                char dir[128], name[64], *p;

                // Extract the directory and the firmware_name
                strcpy(dir, FIRMWARE_IMAGE_PATH);
                p = strrchr(dir, '/');  
                *(p++) = 0; strcpy(name, p);

                if      ((access("/usr/bin/tftp", X_OK)) == 0)      tftp = "/usr/bin/tftp";
                else if ((access("/usr/bin/tftp-hpa", X_OK)) == 0)  tftp = "/usr/bin/tftp-hpa";
                else { 
                    CWLog("! Missing tftp client (/usr/bin: tftp or tftp-hpa)"); 
                    break; 
                }
                if      (access("/bin/md5sum", X_OK) == 0)      checker = "/bin/md5sum";
                else if (access("/usr/bin/md5sum", X_OK) == 0)  checker = "/usr/bin/md5sum";
                else {
                    CWLog("! Missing tool md5sum. (/bin/md5sum or /usr/bin/md5sum");
                    break;
                }
                if (statfs(dir, &fs) < 0) {
                    CWLog("statfs error: %s.", strerror(errno));
                    CWLog("Connot download: Get filesystem informatio failure.");
                    break;
                }
                if (fs.f_bfree * fs.f_bsize < flen + 1024) {
                    CWLog("Connot download: /tmp The space is insufficient (only %ld, need %ld + 1024).", 
                            fs.f_bsize * fs.f_bfree, flen);
                    stat_report(rept_addr, rept_port, UPSTAT_ERR_LACKBUF, version, report_mac, report_ip, report_if);
                    break;
                }
                // Environment pass
                // fork process tftp download new version WTP
                if ((pid = fork()) < 0) { CWLog("fork error: %s", strerror(errno)); break; } 
                else if (pid > 0) { break; }
                else { 
                    // child process, Yuan Hong
                    FILE *f; 
                    char cmd[1024],  new_md5[65]; 
                    int n;

                    setsid(); 
                    chdir(dir);

                    CWLog("upgrade process %d\n", getpid());

                    /* 
                     * In order to solve the problem of "No child processes" 
                     *  Here, to restore the signal processing
                     *  After the process is complete recovery  
                     */
                    typedef void (*sighandler_t)(int); 
                    sighandler_t old_handler;  
                    old_handler = signal(SIGCHLD, SIG_DFL);

                    stat_report(rept_addr, rept_port, UPSTAT_ING, version, report_mac, report_ip, report_if);

                    //*
                    CWLog("Starting Update version %d.%d.%d Firmware", v1, v2, v3);
                    { // popen tftp download: FIRMWARE_IMAGE_PATH
                        // must be install: iptables -I INPUT -p udp --dport 60000:61000 -j ACCEPT
                        snprintf(cmd, sizeof(cmd), "%s -R 60000:61000 %s %d -c get %s", 
                                tftp, addr, tftp_port, name);
                        CWLog("popen(tftp) begin");
                        if ((f = popen(cmd, "r")) == 0) {
                            CWLog("popen(tftp) error: %s", strerror(errno));
                            CWLog("! Download failed");
                            stat_report(rept_addr, rept_port, UPSTAT_ERR_TFTP, version, report_mac, report_ip, report_if);
                            CHILD_EXIT();
                        }
                        while(fgets(buff, sizeof(buff), f) != 0) {
                            n = strlen(buff);
                            if ('\n' == buff[n-1]) buff[n-1] = 0;
                            CWLog("%s", buff);
                        }
                        if (pclose(f) < 0) {
                            CWLog("pclose(tftp) error: %s", strerror(errno));
                            CWLog("! Download failed");
                            stat_report(rept_addr, rept_port, UPSTAT_ERR_TFTP, version, report_mac, report_ip, report_if);
                            CHILD_EXIT();
                        }
                        CWLog("popen(tftp) done");
                    }
                    { // check hash: FIRMWARE_IMAGE_PATH
                        snprintf(cmd, sizeof(cmd), "%s %s", checker, FIRMWARE_IMAGE_PATH);
                        CWLog("popen(md5sum) begin", cmd);
                        if ((f = popen(cmd, "r")) == 0) {
                            CWLog("popen(md5sum) error: %s", cmd, strerror(errno));
                            stat_report(rept_addr, rept_port, UPSTAT_ERR_MD5, version, report_mac, report_ip, report_if);
                            CHILD_EXIT();
                        }
                        fgets(buff, sizeof(buff), f);
                        sscanf(buff, "%[^ ] ", new_md5);
                        CWLog("Receive md5 [%s]", md5);
                        CWLog("Checker md5 [%s]", new_md5);
                        if (strncmp(md5, new_md5, strlen(md5))) {
                            CWLog("MD5 Check failure.");
                            stat_report(rept_addr, rept_port, UPSTAT_ERR_MD5, version, report_mac, report_ip, report_if);
                            CHILD_EXIT();
                        } 
                        CWLog("MD5 Check Success.");
                        if (pclose(f) < 0) {
                            CWLog("pclose(md5sum) error: %s", strerror(errno));
                            CHILD_EXIT();
                        }
                        CWLog("popen(md5sum) done");
                    } // */
                    if (write_firmware_image_info(FIRMWARE_IMAGE_INFO, version, md5, blen) < 0) {
                        CWLog("write_firmware_image_info error: %s", strerror(errno));
                        stat_report(rept_addr, rept_port, UPSTAT_ERR_SAVEINFO, version, report_mac, report_ip, report_if);
                        CHILD_EXIT();
                    }
                    stat_report(rept_addr, rept_port, UPSTAT_SUC, version, report_mac, report_ip, report_if);
                    CWLog("Download Successful");          // All Finish
                    signal(SIGCHLD, old_handler);          // Solve: No child processes
                    CHILD_EXIT();
                } // if ((pid = fork()) < 0) 
            } while (0);
        } // if (maxver(v1, v2, v3, maxv1, maxv2, maxv3))
        else  { 
            // 本地版本大于或等于AC上的版本，不需下载
            CWLog("Firmware do not need to download");
            do { 
                // 判断本地缓存版本和现有版本，决定是否更新固件
                if (maxver(etcv1, etcv2, etcv3, tmpv1, tmpv2, tmpv3)) {
                    CWLog("Firmware do not need to update");
                    break;
                }
                CWLog("Firmware need to update");
                // 更新tmp缓存的固件
                CWLog("Startup update firmware...");

                // --------------
            } while (0);
        }
    } // if (check)
    CWLog("************************************************");
}

int create_socket_v4_connect(int family, int type, const char* p_addr, int port)
{
    struct sockaddr_in saddr;
    int err, is;
    saddr.sin_port = htons(port);
    saddr.sin_family = family;
    err = inet_pton(family, p_addr, &saddr.sin_addr.s_addr);

    if (err == 0 || saddr.sin_port<=0)
        return -2;
    else if (err < 0)
        return -1;

    if ((is = socket(family, type, 0)) < 0)
        return -1;

    // connect timeout times
    struct timeval con_timeo = {30, 0};
    setsockopt(is, SOL_SOCKET, SO_SNDTIMEO, &con_timeo, sizeof(con_timeo));

    if (connect(is, (struct sockaddr *) &saddr, sizeof(saddr)) < 0)
    {
        close(is);
        return -1;
    }

    return is;
}

#include "domain_parse.h"

// thread-unsafe
int send_pack(packet_stat *pack, const char *dst, int port)
{
    char errinfo[128];

    // 在固件下载状态上报回话中，不允许进行重复解析
    static char ip[128];  
    static int flag = 0;

    if (flag == 0) {
        if (domain_parse(dst, ip, sizeof(ip), errinfo, sizeof(errinfo)) < 0) {
            CWLog("%s %s\n", dst, errinfo);
            return -1;
        }
        CWLog("domain parse: %s - %s\n", dst, ip);
        flag = 1;
    } else {
        CWLog("domain parsed %s - %s\n", dst, ip);
    }

    int fd = create_socket_v4_connect(PF_INET, SOCK_STREAM, ip, port);
    if (fd < 0) {
        CWLog("create_socket_v4_connect error: %s", strerror(errno));
        return -1;
    }
    //printf("create_socket_v4_connect %s\n", ip);

    if (send(fd, pack, sizeof(packet_stat), 0) != sizeof(packet_stat)) {
        CWLog("send stat %d packet error: %s\n", ntohs(pack->stat), strerror(errno));
        //printf("send stat %d packet error: %s\n", ntohs(pack->stat), strerror(errno));
    }
    else {
        CWLog("send stat %d packet to %s:%d\n", ntohs(pack->stat), dst, port);
        //printf("send stat %d packet to %s:%d\n", ntohs(pack->stat), dst, port);
    }

    close(fd);
    return 0;
}

void stat_report(char *addr, int port, int stat, char *ver, char *mac, char *ip, char *ifname)
{
    packet_stat pack;
    pack_net_stat(&pack, stat, ver, mac, ip, ifname);
    send_pack(&pack, addr, port);
}

#endif
