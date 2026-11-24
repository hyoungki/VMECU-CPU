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

#ifndef	COMMON_HEADER_INCLUDED

#define	COMMON_HEADER_INCLUDED

//#define MPLC_TEST     100
//#define LOCATOR_TEST    200


#define DATE_STRING         "2026.07.07"
#define VERSION_STRING      "Ver26.07.07"
#define SIMULATOR_NAME      "VME-CU DB EDITOR V12.0.0.3, 2017-08-01"

/* ------------------------------------------------ */
/* ROM Version 또는 NFS Version 선택                */
/* ------------------------------------------------ */
#if 1
#define     ROM_VERSION         100        
#define     NETWORK_CONSOLE     200     // Network - CONSOLE   
#endif

//#define LOG_FILE_CREATE      100

#define	ACE_CONTROL_TARGET			1		// 에이스콘트롤의 경우...
//#define	SHINA_ENG_TARGET			2		// 신아엔지니어링의 경우...
#define     THE_ONE_TARGET  1

#define SDP_VMECU 1


#ifdef	ACE_CONTROL_TARGET

/* ================================================ */
/* VMECU 를 ACE-LBS-RTU 로 적용하는 경우            		*/
/*  - ACE_LBS_6U : 자립형(VMECU) 적용시             		*/
/*  - ACE_LBS_4U : 일체형 LBS-RTU(4U) 적용시        		*/
/* ================================================ */
#if 0
#define ACE_LBS_RTU         100				// LBS-6U 일체형 RTU (VMEBUS 방식)
#define ACE_LBS_4U          200           	// LBS-RTU 이면서...LBS-4U 일체형 RTU (VMEBUS 방식)
#define ACE_LBS_6U          300           	// LBS-RTU 이면서...LBS-6U 자립형 RTU (VMECU TYPE)
#endif

/* ------------------------------------------------ */
/* 원격소장치/CU 관련 상수정의                      */
/* ------------------------------------------------ */
#ifdef  ACE_LBS_6U          // ACE-LBS-6U 정의시 
#define TARGET_NAME         ">>ACE-LBS-6U-V11 : "
#else

#ifdef  ACE_LBS_4U          // ACE-LBS-4U 정의시 
#define TARGET_NAME         ">>ACE-LBS-4U-V11 : "
#else
#define TARGET_NAME         ">>ACE-VMECU-NET-V11 : "
#endif
#endif

#endif		// ACE_CONTROL_TARGET


#ifdef	SHINA_ENG_TARGET

/* ================================================ */
/* VMECU 를 ACE-LBS-RTU 로 적용하는 경우            		*/
/*  - ACE_LBS_6U : 자립형(VMECU) 적용시             		*/
/*  - ACE_LBS_4U : 일체형 LBS-RTU(4U) 적용시        		*/
/* ================================================ */
#if 0
#define ACE_LBS_RTU         100				// LBS-6U 일체형 RTU (VMEBUS 방식)
#define ACE_LBS_4U          200           	// LBS-RTU 이면서...LBS-4U 일체형 RTU (VMEBUS 방식)
#define ACE_LBS_6U          300           	// LBS-RTU 이면서...LBS-6U 자립형 RTU (VMECU TYPE)
#endif

/* ------------------------------------------------ */
/* 원격소장치/CU 관련 상수정의                      */
/* ------------------------------------------------ */
#ifdef  ACE_LBS_6U          // ACE-LBS-6U 정의시 
#define TARGET_NAME         ">>SHINA-LBS-6U-V11 : "
#else

#ifdef  ACE_LBS_4U          // ACE-LBS-4U 정의시 
#define TARGET_NAME         ">>SHINA-LBS-4U-V11 : "
#else
#define TARGET_NAME         ">>SHINA-VMECU-NET-V11 : "
#endif
#endif

#endif		// SHINA_ENG_TARGET

#undef TARGET_NAME 
#define TARGET_NAME get_target_name() 

/* ------------------------------------------------ */
/* 원격소장치/CU 관련 데이터베이스                  */
/* ------------------------------------------------ */
#ifdef  ACE_LBS_RTU          // ACE-LBS-6U 정의시 
#define ROMDB_STRING        "/mnt/ACEDB-LBS"                
#define ROMDB_STRING2       "/mnt/ACEDB-LBS-1"           // 시험용 SLAVE보드 DB
#else						// ACE-VMECU 정의시 
#define ROMDB_STRING        "/mnt/ACEDB-NET4"                
#define ROMDB_STRING2       "/mnt/ACEDB-NET4-1"           // 시험용 SLAVE보드 DB
#endif

/*
   KMEC VMECU 용
   
*/
#undef  ROMDB_STRING
#undef  ROMDB_STRING2
#define ROMDB_STRING        "/mnt/VMECU-DB-NET4"                
#define ROMDB_STRING2       "/mnt/VMECU-DB-NET4-1"           // 시험용 SLAVE보드 DB


/* **************************************** */
/* **************************************** */

#define _PACK_ __attribute__((__packed__))      // Structure Data Size Packed...

//
// Macro
//
#define	max(a,b)	((a) > (b) ? (a) : (b))
#define	min(a,b)	((a) < (b) ? (a) : (b))

//
// General Type
//
typedef unsigned char   BYTE;
typedef unsigned char   UCHAR;
typedef unsigned short  WORD;

typedef unsigned char   byte;
typedef unsigned char   uchar;
typedef unsigned short  word;


// bsdsignal.c 
void (*bsd_signal(int, void (*)(int)))(int);
void *xmalloc(size_t size);

void mysyslog(const char *fmt, ...);
void myopenlog(const char *name, int option);
void mycloselog(void);

#define xmsleep(x)   usleep(x*1000) // x is in mS *

#define pause(x)   usleep(x*1000) // x is in mS *

#define	NO 		    0
#define	YES		    1

#define	RESET 	    0
#define	SET		    1

//#define	NULL	    0

#define	TRUE 	    1
#define	FAIL	    -1
#define FALSE       0

#define	OK 		    1
#define	NOK		    0

#define	ON 		    1
#define	OFF		    0

#define ERROR       -1
#define CONNECT     1

//
// Return Codes
//
#define	E_OK				0
#define	E_ERR				-1
#define	E_SKIP				-1
#define	E_WAIT				-2


#define BIT0            0x01
#define BIT1            0x02
#define BIT2            0x04
#define BIT3            0x08
#define BIT4            0x10
#define BIT5            0x20
#define BIT6            0x40
#define BIT7            0x80

#define AUTO_ENABLE     0x10
#define AUTO_DISABLE    0x11
#define DATA7           0x12
#define DATA8           0x13

#define FIOFLUSH        0x01
#define QUIT            0x03
#define EXIT            0x18
#define FIOFLUSH        0x01
#define RTS_ON          0x02
#define RTS_OFF         0x03



//
// Process Index
// PROCESS 별 ID 부여 (실제 운영되는 프로세서 정의 )
//
#define	MAX_PROCESS		10

#define	LINK_PROCESS    0       /* LINK Process */
#define	SIM_PROCESS	    1       /* SIMULATOR 통신용 Process */
#define	MMI_PROCESS     2       /* SIMULATOR 통신용 Process */
#define	SCAN_PROCESS	3       /* RTU SCAN Process */
#define	SCU_PROCESS	    4       /* 이중화 절체장치 통신용 Process */

#define	HOST1_PROCESS	5       /* HOST      통신용 Process */
#define	HOST2_PROCESS	6       /* HOST      통신용 Process */
#define	HOST3_PROCESS	7       /* HOST      통신용 Process */
#define	HOST4_PROCESS	8       /* HOST      통신용 Process */
#define	HOST5_PROCESS	9       /* HOST      통신용 Process */

#define	CLI_PROCESS		MAX_PROCESS
#define	WDT_PROCESS	    (MAX_PROCESS + 1)       /* WDT     통신용 Process */

/* ------------------------------------------------ */
/* 	이중화 관련 									*/
/* 	CPU_MASTER   : 기본적인 CPU MASTER 설정(Switch)	*/
/* 	CPU_SLAVE    :          CPU SLAVE  설정(Switch)	*/
/* 	LOCAL_MASTER : Active CPU 설정상태				*/
/* 	LOCAL_SLAVE  : NON-Active CPU 설정상태			*/
/* ------------------------------------------------ */
#define CPU_MASTER    	0
#define CPU_SLAVE     	1

#define LOCAL_MASTER	0
#define LOCAL_SLAVE     1

#define ETH0            0
#define ETH1            1
#define ETH2            2


//#define NUMBER_OF_EVENT     (0x30 + 2)

//
// IPC
//
#define	SHM_KEY			(key_t)100

#define	MAX_QUEUE				4
#define	LINK_QUEUE		(key_t)100          // 센서노드 수집 Queue
#define	SND_QUEUE		(key_t)101          // 정보제공 Queue
#define	HST_QUEUE		(key_t)102          // HOST Queue
#define	SIM_QUEUE		(key_t)103          // Simulator Queue


#define	PLATENUM_LEN		12
#define	CARINFO_LEN			18
#define	RAWIMAGE_HEADER		16
#define	PRCNAME_LEN			32
#define	STAMP_LEN			32
#define	FILENAME_LEN		64
#define	MESSAGE_LEN			128
#define	SHORTMSG_LEN		16
#define	CAMMSG_LEN			16
#define	RES_X				1024
#define	RES_Y				1024
#define	RES_YH				(RES_Y/2)
#define	TOTAL_IMAGE			(RES_X * RES_Y)
#define	RAWIMAGE_LEN		(TOTAL_IMAGE + RAWIMAGE_HEADER)
#define	PIXEL_BLACK			0
#define	PIXEL_GRAY_NOISE	115
#define	PIXEL_GRAY			125
#define	PIXEL_WHITE			255
#define	MAX_GRAY_LEVEL		256
#define	MAX_SEGMENT			10
#define	NEW_SEGCOUNT		9
#define	OLD_SEGCOUNT		8
#define	MULTI_THRESHOLD		15
#define	RAW_EXT_NAME		"raw"
#define	MAX_LINES			6


//
// Type Definitions
//
/* ----------------------------- */
/* SHARERAM 관련 Structure 외부선언 */
/* ----------------------------- */ 
typedef struct {
	int		id;
	key_t	key;
	int		size;
	char	*address;
} SHM_DESC;

/* ----------------------------- */
/* THREAD 관련 Structure 외부선언 */
/* ----------------------------- */
#define	TASK_NAME_LEN	16

typedef struct {
	char	name[TASK_NAME_LEN];
	pthread_t		tid;
	int		priority;
	int		options;
	void	*entryPt;
	void	*arg;

} THREAD_ENTRY;
                
                
/* ----------------------------- */
/* TTYDESC 관련 Structure 외부선언 */
/* ----------------------------- */ 
#define	MAX_TTY_DEVICE		64
#define	MAX_TTY_DEVICE		64
#define	FILENAME_LEN		64
#define	MESSAGE_LEN			128



#define ASYNC_8530          1
#define ASYNC_860T          2

typedef struct {
    int     chid;
	int		id;
	FILE	*fId;
	int		config;             /* device 속성 정의 (외부8530, 내부 ASYNC) */
	int     commMode;
	int		baudRate;
	int		dataBits;
	int		stopBits;
	int     lineDebug;
	char	parity;
	char	name[FILENAME_LEN];
	struct timeval  timeOut;
} TTY_DESC;



#endif		// COMMON_HEADER_INCLUDED

//
// End of common.h
//
