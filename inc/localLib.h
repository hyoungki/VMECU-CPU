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

#ifndef	LOCALLIB_H_

#define	LOCALLIB_H_

//
// Include Files
//
#include	<stdio.h>
#include	<unistd.h>
#include	<stdlib.h>
#include 	<stdio.h>
#include 	<stdlib.h>
#include 	<string.h>
#include 	<stdarg.h>
#include 	<signal.h>
#include    <time.h>
#include 	<termios.h>
#include 	<sys/types.h>
#include 	<sys/time.h>
//#include 	<sys/io.h>
#include 	<sys/ioctl.h>
#include 	<sys/ipc.h>
#include 	<sys/shm.h>
#include 	<sys/signal.h>
#include 	<sys/stat.h>
#include 	<sys/socket.h>
#include    <sys/mman.h>
#include 	<arpa/inet.h>
#include    <linux/rtc.h>  // hello world 

#include 	<errno.h>
#include	<pthread.h>
#include 	<fcntl.h>
#include 	<unistd.h>
#include 	<ctype.h>
#include 	<math.h>

#include    "common.h"

#include    "m860io.h"
#include    "vmesio.h"

#include    "op_iomodule.h"
#include    "op_harris.h"


#include    "op_dnp.h"
#include    "op_modbus.h"
#include    "op_rtusim.h"

#include    "op_iec.h"

#include    "rtudb.h"

#include    "op_landis.h"

#include    "usrtype.h"
#include    "netInc.h"
#include    "logdb.h"
//#include    "eventmsg.h"


// hkkim for parsing 
#include "configini.h"
#include "queue.h"
#include "parse_config.h"


#include    "shmMemory.h"



/* ------------------------------------------------ */
/*  영상검지기용 860 Board : NET860-500             */
/*  860 Internal ASYNC -PORT Define ...             */
/* ------------------------------------------------ */
#define DEVICE_HERO_IO      "/dev/hera-io"
#define DEVICE_EX_IO        "/dev/ex-io"

/* ---------------------------------------------------- */
/*  RTU-TYPE : ACE-LBS-4U... ASYNC  PORT Comm Parameter */
/* ---------------------------------------------------- */
#ifdef  ACE_LBS_4U

#define FID_CONSOLE_PORT  	"/dev/ttyCPM0"
#define FID_LINK_PORT  	    "/dev/ttyCPM1"	    /* CPU 링크연계 */
#define FID_SCU_PORT  	    "/dev/ttyCPM2"	    /* 절체장치 연계 */

/*  External ASYNC -PORT Define ... */
#define FID_SCAN1_PORT 	    "/dev/ttyS0"	    /* 외부용 ASYNC 포트 연계 */
#define FID_SCAN2_PORT 	    "/dev/ttyS1"	    /* 외부용 ASYNC 포트 연계 */

#define FID_SCAN3_PORT	    "/dev/ttyCPM3"		/* 외부용 ASYNC 포트 연계 */
#define FID_SCAN4_PORT	    "/dev/ttyCPM4"		/* 외부용 ASYNC 포트 연계 */

#else


#if 0
#define FID_CONSOLE_PORT  	"/dev/ttyCPM0"
#define FID_LINK_PORT  	    "/dev/ttyCPM1"	    /* CPU 링크연계 */
#define FID_SCU_PORT  	    "/dev/ttyCPM2"	    /* 절체장치 연계 */
#define FID_RFU_PORT  	    "/dev/ttyCPM3"	    /* 예비용 포트 연계 */
#define FID_MMI_PORT  	    "/dev/ttyCPM4"	    /* MMI 장치 연계포트 */
#else // hkkim
#define FID_CONSOLE_PORT  	"/dev/ttyCPM0"
#define FID_LINK_PORT  	    "/dev/ttyCPM3"	    /* THE-ONE : CPU 링크연계 */
#define FID_SCU_PORT  	    "/dev/ttyCPM1"	    /* 절체장치 연계 */
#define FID_RFU_PORT  	    "/dev/ttyCPM2"	    /* 예비용 포트 연계 */
#define FID_MMI_PORT  	    "/dev/ttyCPM4"	    /* MMI 장치 연계포트 */

#endif 


/*  External ASYNC -PORT Define ... */
#define FID_SCAN1_PORT	    "/dev/ttyS0"		/* 하위 계전기#1  */
#define FID_SCAN2_PORT	    "/dev/ttyS1"		/* 하위 계전기#2  */
#define FID_SCAN3_PORT	    "/dev/ttyS2"		/* 하위 계전기#3  */
#define FID_SCAN4_PORT	    "/dev/ttyS3" 		/* 하위 계전기#4  */
#define FID_SCAN5_PORT	    "/dev/ttyS4" 		/* 하위 계전기#4  */

#endif


/* 전체 통신포트에 대한 ID */
#define FID_CONSOLE     0
#define FID_LINK        1
#define FID_SCU         2
#define FID_RFU         3
#define FID_MMI         4

/* ------------------------------------------------ */
/*  하위 계전기용 통신포트 ...(총 10개)             */
/* ------------------------------------------------ */
#define FID_SCAN1       0       // CPU 내장 SCAN #1
#define FID_SCAN2       1       // CPU 내장 SCAN #2
#define FID_SCAN3       2       // CPU 내장 SCAN #3
#define FID_SCAN4       3       // CPU 내장 SCAN #4

#define FID_SCAN5       4
#define FID_SCAN6       5
#define FID_SCAN7       6
#define FID_SCAN8       7
#define FID_SCAN9       8
#define FID_SCAN10      9

/* ---------------------------------------------------- */
/*  RTU-TYPE : ACE-LBS-4U... ASYNC  PORT Comm Parameter */
/* ---------------------------------------------------- */
#ifdef  ACE_LBS_4U

#define MAX_CPUPORT     4       // LBS-4U : ASYNC 통신포트 수 ... 4

#else

#define MAX_CPUPORT     6

#endif


#define VME_COM1        10      // VME SIO Port #1
#define VME_COM2        11      // VME SIO Port #2
#define VME_COM3        12      // VME SIO Port #3
#define VME_COM4        13      // VME SIO Port #4
#define VME_COM5        14      // VME SIO Port #5
#define VME_COM6        15      // VME SIO Port #6
#define VME_COM7        16      // VME SIO Port #7
#define VME_COM8        17      // VME SIO Port #8


/* ----------------------------- */
/* MESSAGE 관련 Structure 외부선언 */
/* ----------------------------- */ 
#define	MSG_LENGTH		1024

typedef struct {
	long	mType;
	char	mText[MSG_LENGTH];
} MSG_BUFFER;

typedef struct {
	key_t	key;
	int		id;
	MSG_BUFFER	mBuffer;
} MSG_DESC;

/* ----------------------------- */
/* FILE 관련 Structure 외부선언 */
/* ----------------------------- */  
#define	FIO_NORMAL			0
#define	FIO_APPEND			1
#define	FIO_CREATE			2

#define	FIO_BUFLEN			(256 * 1024)
#define	FIO_DELAY_TIME		(1000)

typedef struct {
	FILE	*id;
	int		mode;
	char	name[FILENAME_LEN];
} FILE_DESC;


//
// 구조체 선언
//
#define	WDT_WAIT_COUNT			10

typedef struct
{
	char	name[PRCNAME_LEN];
	char	argument[MESSAGE_LEN];
	short	check;
	char	waitCount;
} PROCESS_DESC;

#endif

//
// End of aesLib.h
//
