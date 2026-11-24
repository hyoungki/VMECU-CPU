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

/* ---------------------------------------- */
/* HISTORY & EVENT 저장용 메모리 영역       */
/* ---------------------------------------- */
#define SRAM_START_ADDRESS      0x60000000         /* this definition is related with dd and cpld */
#define SRAM_SIZE               0x200000           /* 2MK */

#ifdef  ACE_LBS_4U
#define MRAM_START_ADDRESS     	0x60000000         /* this definition is related with dd and cpld */
#define MRAM_SIZE              	0x80000            /* 512K */
#else
#define MRAM_START_ADDRESS     	0x80000000         /* this definition is related with dd and cpld */
#define MRAM_SIZE              	0x80000            /* 512K */
#endif

/* -------------------------------------------- */
/*  이중화 절체기 상태 정보                     */ 
/* -------------------------------------------- */
#define AUTO_MODE           1  
#define MANUAL_MODE         0

#define SYSTEM_A            0                
#define SYSTEM_B            1

#define CHANGE_S_TO_M       1       // Slave  에서 Master 로 절체
#define CHANGE_M_TO_S       2       // Master 에서 Slage  로 절체

#define CHANGE_ALL_DEVICE   1       // 전체 계전기 통신상태 
#define CHANGE_EACH_DEVICE  2       // 개별 계전기 통신상태


/* -------------------------------------------- */
/*  CPU 상태 정보                               */ 
/* -------------------------------------------- */

#define DATA_CPU_LIVE       0x01                // LINK : CPU 가 동작중임을 표시
#define DATA_CPU_ACTIVE     0x80                // LINK : CPU 가 ACTIVE 임을 표시

#define SYS_MANUAL_BIT      0x40                // LINK : Manual/Auto 판정 Bit
#define SYS_CHANNEL_B       0x20                // LINK : Manual/Auto 판정 Bit

#define SYS_MASTER_BIT      0x08                // LINK : Master/Slave 판정 Bit
#define SYS_DUAL_CPU_BIT    0x04                // LINK : 이중화 CPU 장착여부

#define CPU_PFR_BIT         0x80                // CPU Status : PFR 상태
#define CPU_ACT_BIT         0x02                // CPU 장착여부
#define CPU_RUN_BIT         0x01                // CPU Local-Master 여부


#ifdef  ACE_LBS_4U

/* -------------------------------------------- */
/*  CPU-LED 상태정보                            */ 
/* -------------------------------------------- */

#define LED_RUN_BIT         0x01
#define LED_MASTER_BIT      0x02              // MASTER/SLAVE 보드 상태
#define LED_ACTIVE_BIT      0x04              // MASTER/SLAVE 모드 상태

#define LED_STS_D1          0x08
#define LED_STS_D2          0x10
#define LED_STS_D3          0x20
#define LED_STS_D4          0x40

#define LED_SCU_ONLINE      LED_STS_D1 

#else

#define LED_MASTER_BIT      0x0001              // MASTER/SLAVE 보드 상태
#define LED_ACTIVE_BIT      0x0002              // MASTER/SLAVE 모드 상태

#define LED_SCAN1_BIT       0x0004
#define LED_SCAN2_BIT       0x0008
#define LED_SCAN3_BIT       0x0010
#define LED_SCAN4_BIT       0x0020
#define LED_SCAN5_BIT       0x0040

#define LED_HOST1_BIT       0x0080
#define LED_HOST2_BIT       0x0100
#define LED_HOST3_BIT       0x0200
#define LED_HOST4_BIT       0x0400
#define LED_HOST5_BIT       0x0800

#define LED_RUN_BIT         0x1000

#define LED_MMI_ONLINE      0x2000
#define LED_SCU_ONLINE      0x4000 

#endif
 

#define CHANNEL_TIMEOUT_TICK    30          // 채널 Time out Tick

/* 2018.08.21 최병필.... 통신이상시 자동절체에 대한 처리 ... 너무길어서 줄임 */
#define CHANGE_CPU_TICK         10          // CPU 절체 Tick

typedef struct
    {
        short   sec;
        short   min;
        short   hour;
        short   week;
        short   day;
        short   month;
        short   year;
        long    rtctick;

    } __attribute__ ((packed)) RTC ;


typedef struct
        {
        short   hostHarris;                 // HARRIS HOST 사용시...
        short   hostIEC;                    // IEC    HOST 사용시...
        short   hostLandis;
        short	hostComReady;
        
        short   use_MPLCflag;               // 미쯔비시 PLC 사용유무
                
        short   useSIOBoard;
        short   userEXIT;
        
        short   iecControl[MAX_HOST];       // IEC 제어 Terminate
        short   IEC_cpuChange;              // IEC 제어 : CPU Change
        short   IEC_ChangeTick;             // IEC 제어 : CPU Change
        
        short   squeWrite;                  // Event/History 생성시...
        short   mqueWrite;                  // Event/History 생성시...
        //short   nramAccess;                 // WDT: Event/History Update시...
        
        short   sramAccess;
        short   mramAccess;
        
        short   coaEnable;          
        
        short   dbUpdate;                   // DB Update 상태 */
        
        /* -------------------------------- */
        /*	CPU 이중화 설정 파라메터		*/
        /* -------------------------------- */
        short	dualCpuMode;	            // 이중화 CPU 설정상태...
        short   dualRtuMode;                // 이중화 RTU 구성정보 ...
        short   scuUseFlag;
        short   mmiUseFlag;
        short   soeDebounce;
        short   sysDBCheck;
        short   hostDualWDT;
        short	dnpAIreportVAR;

        char    net_mst_Flag[3];        /* MASTER-CPU Network#1 사용유무 : eth0/eth1/eth2 */
        char	net_mst_IP[3][16];		/* MASTER-CPU IP  Address */
        char	net_mst_GW[3][16];		/* MASTER-CPU GW  Address */
        char	net_mst_SUB[3][16];		/* MASTER-CPU SUB Address */
        
        char    net_slv_Flag[3];        /* SLAVE-CPU Network#1 사용유무 : eth0/eth1/eth2 */
        char	net_slv_IP[3][16];		/* SLAVE-CPU IP  Address */
        char	net_slv_GW[3][16];		/* SLAVE-CPU GW  Address */
        char	net_slv_SUB[3][16];		/* SLAVE-CPU SUB Address */
        
        
        word    stscode;                    // Front 상태 LED
                                            
        short   runMode;					// 이중화 CPU : CPU 동작모드.... Master/Slave
        short   cpuMode;					// 이중화 CPU : 하드웨어 장착상태
        short   cpuChange;                  // 이중화 CPU : CPU 상태 변경...
        
        short   allDevOnline[MAX_HOST];     // 전체계전기 통신상태
        
		short	checkLinkOK;                // 이중화 CPU 상태확인 
		
        short   cpuStatus;                  // Monitoring 보고 : CU 상태정보
        short   dualCpuSts;                 // 이중화 CPU 상태 정보
        short   maxCuPoint;
        
        /* -------------------------------- */
        /*  MMI 관련 변수                   */
        /* -------------------------------- */
        short   masterSts;                  // MMI : MASTER-CPU 동작상태
        short   slaveSts;                   // MMI : SLAVE-CPU 동작상태
        short   mmiHostSts[MAX_HOST];       // MMI : HOST 동작상태
        short   rcvMmiHost[MAX_HOST];
        short   mmiHostTemp[MAX_HOST];
        
        short   hostComSts[MAX_HOST];       // HOST 별 통신포트 상태
        short   scanComSts[MAX_SCAN_PORT];  // SCAN 별 통신포트 상태

		short	mmiTimeSync;
		short	mmiDBcheck;
		                
        short   mmiPFR;
        short   mmiStatus;
        
        byte    mmiFront;                   // MMI : 전송용 이벤트 Queue
        byte    mmiRear;                    // MMI : 전송용 이벤트 Queue
        
        short   mmiOverlab;
        short	mmiReport;
        
        short   wdtEnable;                  // 제어기내부 WDT 기능 사용여부
        short   romDBFail;                  // CU 내부DB 이상

        /* TEST 관련 변수       */
        short   testAnalogFLAG;
        short   testStatusFLAG;
        
        short   testDevid;
        short   testPoint;
        short   testState;
        short   testFlag;

        /* -------------------------------- */
        /* CPU 이중화 관련 변수             */
        /* -------------------------------- */
		short	dbCheckFlag;                // LINK : DB Chksum
		
        short   pointDBDown;			    /* POINT 구성정보 Download Flag */
        short   hostDBDown;				    /* HOST 구성정보 Down Flag */
        short	hostRestart[MAX_HOST];      /* HOST-TASK 재기동 Flag */
        
        short   scanDBDown;				    /* SCAN 구성정보 Down Flag */
        short   scanRestart;                // SCAN-Process 재기동 Flag

        short	modbusDBDown;			    /* MODBUS 구성정보 Down Flag */
        
        short   rcvONtime;                  // 상위 수신한 계전기 제어시간
        short   rcvOFFtime;                 // 상위 수신한 계전기 제어시간

        short   cpuChangeTick;              // 지속적인 이중화 절체 Check Count
                                
        short   stsDumpPeriod;              // 계전기 : Status Dump 주기
        short   anaDumpPeriod;              // 계전기 : Analog Dump 주기
        
        short   simOnline;      
        short   mmiOnline;       
                                            
	    short   devScanCount[MAX_SCAN_PORT];    // DEVICE 별 통신 카운트...
	    short   rtuinit;                            // SCAN 포트 : 초기화 카운트... 
	 
	    /* -------------------------------- */
	    /* DO 모듈 제어변수                 */
	    /* -------------------------------- */
	    short   cntrState;
        short   cntrData;
        struct  timespec setclock;               // DO제어시...Relay 동작시간 Check
        struct  timespec curclock;               // DO제어시...Relay 동작시간 Check
        
	    short   cntrDevId[MAX_SCAN_PORT];
	    short   cntrFlag[MAX_SCAN_PORT];
	    short   cntrComTick[MAX_SCAN_PORT];     // 계전기 : 제어명령 송수신 카운트
	    
        short   hostComError;                   // HOST : 전체 통신이상 Check...
        short   wdtComError;                    // WDT  : 전체 통신이상 Check...
        short   handOffTimer;                   /* decrement timer */
        
        
        /* -------------------------------- */
        /* LANDIS-HOST 관련 변수....        */
        /* -------------------------------- */
        short   chassisNum;                 /* Landis Chassis Number*/
		short	landisINDChg;	            /* IND Chage Flag */
		
        short   landisANAPoint;             /* LANDIS Analog Point Number */
        short   landisSTSPoint;             /* LANDIS Status Point Number */
        short   landisCNTPoint;             /* LANDIS Control Point Number */    
                  
        short   landisANACard;              /* LANDIS Analog Card Number */
        short   landisSTSCard;              /* LANDIS Status Card Number */       
        short   landisACCCard;              /* LANDIS Status Card Number */     
        
        /* -------------------------------- */
        /* HARRIS-HOST 관련 변수....        */
        /* -------------------------------- */
        short   armPoint;
        short   armTCF;
        short   tcf;            /* trip/close flag */
        short   point;
        short   port;           /* port  number */
        short   pointState;     /* point number */
        short   cntStatus;
        
        word    cntTime;
        word    timeBias;
        
        struct  timeval dnpTimeVal;         // DNP-HOST 수신 timeval 
        short   hostDnpTimeSET;             // DNP-HOST Time SET
        
        short   vmecuTimeSet;
        
        short   rtcUpdateFlag;             // RTC Update  정보처리 요구 
        short   year;                      // RTC 설정용 
        short   month;                     // RTC 설정용 
        short   day;                       // RTC 설정용 
        short   week;                      // RTC 설정용     
        short   hour;                      // RTC 설정용 
        short   min;                       // RTC 설정용 
        short   sec;                       // RTC 설정용 
        short   milisec;

        /* 계전기 동작시점 일치용 Time 정보 */
        short   soeYear;
        short   soeMonth;
        short   soeDay;
        short   soeHour;
        short   soeMin;
        short   soeSec;
        short   soeMilisec;
        
        short   devTimeDelay;
        
        /* -------------------------------- */
        /* 내부 운영 DEBUG 관련 변수        */ 
        /* -------------------------------- */       
        short   slaveDebug; 
        short   simDebug;       
        short   dnpDebug;
        short   hostDebug;
        short   msgDebug;
        short   modDebug;
        short   mmiDebug;
        short   soeDebug;
        short   coaDebug;
        short   scuDebug;
        short   linkDebug;
        short   lineDebug;
        short   scanDebug;
        //short   lineCount;
        short   hexDebug;
        short   wdtDebug;
        
        //word    hostWDT[8];
        word    scanWDT[MAX_SCAN_PORT];
                               
        } __attribute__ ((packed)) OPR_MSG;

/* -------------------------------------------- */
/*  이중화 절체장치 관련 정의                   */
/* -------------------------------------------- */
#define SCU_GPOLL           0x00
#define SCU_STATUS_DUMP     0x10
#define SCU_CONTROL         0x20
#define SCU_PFR_CLEAR       0x40

typedef struct
    {
        short   status;
        short   oldsts;
        short   changeFlag;
        
        short   address;        // Address
        
        short   sendFlag;
        short   comFailTick;
        short   online;
        
        short   sndSeqno;
        short   rcvSeqno;

        short   changeMode;        
        short   cntrFlag;
        
        short   cpuRestart;     // CU 재기동 상태
        short   pfrReset;
        short   active;
        
        short   remoteMode;     // 수동/자동 모드
        short   chanMode;       // 시스템 A/B 모드
        
        short   chanSts[8];     // channel 절체상태
        short   chanCmd[8];     // channel 절체명령
        
        byte    txbuf[256];
        byte    rxbuf[256];

    } __attribute__ ((packed)) SCU_MSG;
    
