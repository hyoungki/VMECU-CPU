/*
 * ==============================================================
 * System TARGET : ACE Control VME-CU Ver 2.0   
 * Target CPU    : MPC8248, VME6U
 * Main Factors  : 호남고속철용 VMECU
 *          - CPU 이중화 구성
 *          - VMEBUS Function : DI/DO/AI (프로세서 타입)    
 *          - 통합 시뮬레이터 : 2013/07/08 V8.2 적용 
 * --------------------------------------------------------------
 * System DESIGN : SANE-SYSTEM   .... by  Lee Ho-Sang
 * Initial-DATA  : 2012,12,10
 * Last Updated  : 2013,07,10
 * ==============================================================
 */

#ifndef	SHMMEMORY_HEADER_INCLUDED

#define	SHMMEMORY_HEADER_INCLUDED

//
// Include Files
//
#include "common.h"


typedef struct
{
	pid_t	pid;
	int		define;
	int		check;
	int		active;
	int     hostid;
	
	int     debug;
	int     restart;
	int     initial;
	int		wdtEnable;
	int     wdtCount;
	int		lastWdt;
	int		prevEtime;
	int		maxEtime;
	char	name[16];
} TASK_INFO;


#define	PORTNAME_LEN		32

typedef struct
{
	char	name[PORTNAME_LEN];
	int		baudRate;
	BYTE	byteSize;
	BYTE	stopBit;
	BYTE	parity;
	BYTE	resetCom;
} PORT_INFO;


typedef struct
{
	TASK_INFO	    taskInfo[MAX_PROCESS + 2];
	
	RTC             rtc; 
	OPR_MSG         opr_msg;
    LINK_MSG        link_msg;                       // CPU 이중화
    SCU_MSG         scu_msg;                        // 이중화 절체장치 
    
    RTU_DATABASE    rtuDatabase;                    // CU 데이터베이스
	MMI_DATABASE    mmiDatabase;                    // MMI 데이터베이스
	
    /* -------------------------------- */
    /* DNP-HOST 관련 공유메모리         */
    /* -------------------------------- */
    HOST_DCB        hostDCB[MAX_HOST];              /* HARRIS, LANDIS, SICS HOST Structure */
	
	/* -------------------------------- */
    /* LANDIS-HOST 관련 공유메모리      */
    /* -------------------------------- */
    LANDIS_CFG      landis;                         /* LANDIS HOST Structure */
    HOST_STSBUF     hostStsBuf[MAX_HOST_STS_PT];    /* LANDIS-HOST defined Status Point */
    HOST_ANABUF     hostAnaBuf[MAX_HOST_ANA_PT];    /* LANDIS-HOST defined Analog Point */
    
	/* -------------------------------- */
	/* DEVICE(계전기) 관련 공유메모리   */
	/* -------------------------------- */
	POINT_BUF       devPtBuf[MAX_DEV_POINT];
	SCAN_CONFIG     scanCFG[MAX_SCAN_PORT];         // 하위계전기 SCAN Config
	DNP_DEVICE      dnpCFG[MAX_MODULE];             // 전자식배전반 (GiPAM, HiMAP...)
	
	/* -------------------------------- */
	/* 시스템 운영 이벤트 Queue         */
	/* -------------------------------- */
	SYSTEM_QUE      localEvent;                     // MMI 용 LOCAL Event (SOE...)
	HISTORY_QUE     localHistoryQ;                  // CONSOLE 용 LOCAL Event (SOE...)
	//MMI_COA_QUE     mmiCOA;                       // MMI Analog-Event

	CONSOLE_INFO	console;
	
} SHM_MEMORY;

#endif		// SHMMEMORY_HEADER_INCLUDED

//
// End of shmMemory.h
//
