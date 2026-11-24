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
 
/* -------------------------------------------- */
/*  SYSTEM-EVENT 코드 정의                      */
/* -------------------------------------------- */

#define NOT_DEFINED         0x00

#define EVENT_CU_RESTART    0x01        // CU 재기동시
#define EVENT_ROMDB_FAILED  0x02        // 데이터베이스 이상시
#define EVENT_RESET_BY_USER 0x03        // 사용자 재기동
#define EVENT_RESET_BY_WDT  0x04        // WDT에 의한 재기동

#define EVENT_MASTER_ACTIVE 0x05        // MASTER-CPU 동작
#define EVENT_SLAVE_ACTIVE  0x06        // SLAVE-CPU 동작
#define EVENT_CHANGE_SYS_A  0x07        // 이중화 절체-A
#define EVENT_CHANGE_SYS_B  0x08        // 이중화 절체-B
#define EVENT_CHANGE_AUTO   0x09        // 자동모드
#define EVENT_CHANGE_MANUAL 0x0a        // Manual모드
#define EVENT_LOCAL_TIMESET 0x0b        // 시스템 시각동기

#define EVENT_SIM_TIMESET   0x0c        // 추가 : 시뮬레이터 시각동기
#define EVENT_CHANGE_CPU    0x0d        // 추가 : 이중화 절체-CPU
#define EVENT_HOST_RESET    0x0e        // 추가 : HOST 통신이상 RESET
#define EVENT_ALL_LOCK      0x0f        // 추가 : 전체 쇄정/해정 

#define EVENT_HOST_ONLINE   0x10        // HOST 통신 Online
#define EVENT_HOST_OFFLINE  0x11        // HOST 통신 Offline
#define EVENT_HOST_TIMESET  0x12        // HOST 시각동기
#define EVENT_MOD_ONLINE    0x13        // 하위계전기 통신 Online
#define EVENT_MOD_OFFLINE   0x14        // 하위계전기 통신 Offline

#define EVENT_MMI_ONLINE    0x15        // 하위계전기 통신 Online
#define EVENT_MMI_OFFLINE   0x16        // 하위계전기 통신 Offline
#define EVENT_SCU_ONLINE    0x17        // 하위계전기 통신 Online
#define EVENT_SCU_OFFLINE   0x18        // 하위계전기 통신 Offline
#define EVENT_LINK_ONLINE   0x19        // 하위계전기 통신 Online
#define EVENT_LINK_OFFLINE  0x1a        // 하위계전기 통신 Offline

#define EVENT_SOE           0x20        // 계전기 SOE 이벤트
#define EVENT_HOST_CNTR     0x21        // 관제에 의한 제어
#define EVENT_LOCAL_CNTR    0x22        // 자체 제어

/* History Only... */
#define EVENT_MOD_SELECT    0x23        // 계전기측 제어
#define EVENT_MOD_OPERATE   0x24        // 계전기측 제어

#define EVENT_SDB_UPDATE    0x25        // SCAN-DB 변경시 Update
#define EVENT_MDB_UPDATE    0x26        // Module-DB 변경시 Update
#define EVENT_HDB_UPDATE    0x27        // Host-DB 변경시 Update


#define NUMBER_OF_EVENT     (0x30 + 2)


#define MAX_COA_NUM     128
#define MAX_COA_MASK    127

/* ------------------------------------ */
/*  CLASS3 : COA Queue                  */
/* ------------------------------------ */
typedef struct
    {
        byte    devid;                  // 계전기 번호 HOST Point Number                
        byte    point;                  // 포인트 번호

        float   floatData;
    } __attribute__ ((packed)) ANALOG_COA_ENTRY;  

typedef struct
    {
        word    front;
        word    rear;
        
        byte    overlab;
        byte    reportCnt;
        
        ANALOG_COA_ENTRY  coaQueue[MAX_COA_NUM+2];

    } __attribute__ ((packed)) MMI_COA_QUE;  
    
//#define NUMBER_OF_EVENT     (0x64 + 2)

typedef struct
        {
        byte    year;           // 2000 base
        byte    month;
        byte    day;
        byte    hour;
        byte    min;
        byte    sec;
        word    milisec;
        
        byte    logid;          // SOE/Control...
        byte    ioid;           // 계전기 번호, 0,1,2
        word    point;          // 포인트 번호, 0,1,2
        byte    state;      
        byte    hostid;
        
        } __attribute__ ((packed)) SYSLOG_FORM;

        
/* ---------------------------------------------------- */
/*  MMI 전송용 System-Queue                             */
/* ---------------------------------------------------- */
#define SYSTEM_QUE_MAX      256
#define SYSTEM_QUE_MASK     255

typedef struct
        {
        byte    front;
        byte    rear;
        byte    overlab;
        byte    clear;

        SYSLOG_FORM   queue[SYSTEM_QUE_MAX+2];        
        word    chksum;             /*   check word           */
        
        } __attribute__ ((packed)) SYSTEM_QUE;


/* ---------------------------------------------------- */
/*  CONSOLE용 History-Queue                             */
/* ---------------------------------------------------- */
#define HISTORY_QUE_MAX      1024
#define HISTORY_QUE_MASK     1023

typedef struct
        {
        word    front;
        word    overlab;
        word    clear;
        word    temp;

        SYSLOG_FORM   queue[HISTORY_QUE_MAX+2];        
        word    chksum;             /*   check word           */
        
        } __attribute__ ((packed)) HISTORY_QUE;



#define	LOG_MAX_NUM		256
#define	LOG_MASK_NUM	255

typedef struct
{

	short	active;
	word	front;
	word	rear;
	
	byte	logData[LOG_MAX_NUM+2][256];

	short	writeFlag;
	short	temp;
		
} CONSOLE_INFO;
