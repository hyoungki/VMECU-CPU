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

/* ------------------------------------------------ */
/* 호스트 관련 상수정의                             */
/* 원격소장치관련 상수 정의                         */
/* ------------------------------------------------ */
#define MAX_HOST            5           // 최대 HOST 수 

#define MASTER_PORT         0           // HOST 내 MASTER/SLAVE 포트 구분
#define SLAVE_PORT          1 

#define STATUS_DUMP_TICK    10           // 초기화 상태덤프 수

/* HOST 운영모드 지정 */
#define HOST_NOT_USE        0
#define HOST_SINGLE         1
#define HOST_DUAL           2

/* HOST 통신프로토콜 */
#define HOST_NULL           0
#define HOST_HARRIS         1
#define HOST_LANDIS         2
#define HOST_DNP            3
#define HOST_MODBUS         4
#define HOST_IEC_101	    5
#define HOST_SICS	        6


/* HOST 연계속도 */
#define SPEED_1200  	    0
#define SPEED_2400          1
#define SPEED_4800          2
#define SPEED_9600          3
#define SPEED_19200         4
#define SPEED_38400         5
#define SPEED_57600         6

/* HOST 연계방식 */
#define COM_RS232      	    0
#define COM_MODEM      	    1
#define	COM_RS485		    2
#define COM_TCPIP      	    3
    
/* ------------------------------------------------ */
/* 모듈관련 상수정의                                */
/* 원격소장치관련 상수 정의                         */
/* ------------------------------------------------ */

#define MAX_SCAN_PORT       10       // 기존 : SCAN-PORT => 5

#define SCAN_PORT_1         0
#define SCAN_PORT_2         1
#define SCAN_PORT_3         2
#define SCAN_PORT_4         3
#define SCAN_PORT_5         4


#if 0
=========================================================
#define MAX_VMECU_POINT     4096        // 실제 VMECU-데이터베이스 상의 포인트 DB
#define MAX_DBASE_POINT     4096        // 운영상의 데이터베이스 상의 포인트 DB

#define MAX_MMI_POINT       2048        // 데이터베이스 상의 포인트 DB

#define MAX_DNP_DI_POINT    2688        // DNP HOST 상의 운영 포인트
#define MAX_DNP_AI_POINT    1024        // DNP HOST 상의 운영 포인트
#define MAX_DNP_DO_POINT    256         // DNP HOST 상의 운영 포인트
#define MAX_DNP_AO_POINT    128         // DNP HOST 상의 운영 포인트
=========================================================
#endif

//#define MAX_DBASE_POINT     4096        // 운영상의 데이터베이스 상의 포인트 DB
#define MAX_DBASE_POINT     2048        // 운영상의 데이터베이스 상의 포인트 DB

#define MAX_MMI_POINT       2048        // 데이터베이스 상의 포인트 DB

#define MAX_DNP_DI_POINT    1152        // DNP HOST 상의 운영 포인트
#define MAX_DNP_AI_POINT    512         // DNP HOST 상의 운영 포인트
#define MAX_DNP_DO_POINT    256         // DNP HOST 상의 운영 포인트
#define MAX_DNP_AO_POINT    128         // DNP HOST 상의 운영 포인트


/* ------------------------------------------------ */
/* CU 최대 포인트 - 계전기 32 장착시 */
/* ------------------------------------------------ */
//#define MAX_MODULE          64       
//#define MAX_MODULE          40        
#define MAX_MODULE          32           
#define MAX_DNP_DEVICE      MAX_MODULE
#define MAX_MODULE_MASK		31

#define MAX_DEV_DI_POINT    1024        // 장치당 최대 DI 포인트 
#define MAX_DEV_DO_POINT    1024        // 장치당 최대 DO 포인트 
#define MAX_DEV_AI_POINT    256         // 장치당 최대 AI 포인트 
#define MAX_DEV_AO_POINT    10          // 장치당 최대 AI 포인트 
#define MAX_DEV_COUNT_POINT 16          // 장치당 최대 COUNT 포인트 

//#define MAX_DEV_POINT       (MAX_MODULE + 8)            	/* 전체 Device 포인트 : 계전기 통신상태, 이중화, 절체 등 Device 종속 포인트  */
#define MAX_DEV_POINT       100           					/* 전체 Device 포인트 : 계전기 통신상태, 이중화, 절체 등 Device 종속 포인트  */

#define	MAX_DI_DATA_COUNT	(MAX_DEV_DI_POINT / 8)		// 계전기로부터의 최대 DI Byte Count


/* ------------------------------------------------ */
/* 계전기 Module Type defination                    */
/* ------------------------------------------------ */
#define	TYPE_NULL			0x00	
#define TYPE_DIM            0x01    /* Digital Input Board      */
#define TYPE_DOM            0x02    /* Digital Output Board[1]  */
#define TYPE_AIM            0x03    /* Analog Input Board       */
#define TYPE_DIOM           0x04    /* DIO Module               */
#define TYPE_HIMAP          0x05    /* HIMAP 계전기 (DNP-HIMAP)        */
#define TYPE_GIPAM2000      0x06    /* LG  계전기               */
#define TYPE_FRTU           0x07    /* 지중용 FRTU              */
#define TYPE_PROPACK        0x08    /* 효성계전기 -PROPACK      */
#define TYPE_VIPAM          0x09    /* 비츠로계전기             */

#define TYPE_MODBUS_SZ      0x0A    /* MODBUS 계전기- 지멘스          */
#define TYPE_MODBUS_LG      0x0B    /* LG 계전기 - GIPAM115-MODBUS     */

#define TYPE_DNP_SIEMENS    0x0C    /* 지멘스 TCPIP-DNP 계전기 적용 */
#define TYPE_LOCATOR1       0x0D    /* 고장점(쯔다) #1 */
#define TYPE_LOCATOR2       0x0E    /* 고장점(쯔다) #1 */
#define TYPE_MPLC           0x0F    /* 미쯔비시 계전기 */


/* ------------------------------------------------ */
/* 모듈관련 상수정의                                */
/* Point Type 및 상수 정의                          */
/* ------------------------------------------------ */
/* point Config  */
#define NULPT               0
#define COSPT               1
#define SOEPT               2
#define ALLPT               3
#define ACCPT               4
#define SBOPT               5
#define CCOPT               6
#define MA1PT               7       // 아날로그 - 1mA
#define MA20PT              8       // 아날로그 - 20mA
#define V10PT               9       // 아날로그 - 10V
#define ACTPT               10      // 계전기 Active Flag

#define LOCKPT              11      // 포인트 쇄정/해정
#define DLOCKPT             12      // 계전기 쇄정/해정


/* DEVICE Type Define */
#define NULL_DEV            0
#define DI_POINT            1
#define DO_POINT            2
#define AI_POINT            3
#define AO_POINT            4
#define DIO_POINT           5
#define RDI_POINT           6
#define RDO_POINT           7
#define RAI_POINT           8
#define RAO_POINT           9
#define ADEV_POINT          10       /* 전체 디바이스 통신상태 포인트 */
#define DEV_POINT           11       /* 디바이스별 통신상태 포인트 */

#define	MCPU_POINT		    12		/* CPU 이중화 상태 포인트 (MASTER)*/
#define	SCPU_POINT		    13		/* CPU 이중화 상태 포인트 (SLAVE) */
#define	CNTR_POINT		    14		/* CPU 이중화 절체 포인트 (제어) */

#define LDI_POINT           15      /* 고장점 : 상태포인트 */
#define LAI_POINT           16      /* 고장점 : 계측포인트 */
#define	ALOCK_POINT		    17		/* 전체 계전기 쇄정/해정 제어) */

/* ------------------------------------ */
/* DO 포인트 타입 설정                  */
/* ------------------------------------ */
#define POINT_CONTROL       1       // 개별 포인트 제어
#define SYSTEM_CONTROL      2       // 시스템 이중화 절체 제어
#define ALL_LOCK_CONTROL    3       // 시스템 전체 쇄정/해정 제어


/* ---------------------------------------------------- */
/*  DNP 계전기 제어방식별 Config 정의                   */
/* ---------------------------------------------------- */
#define DNP_POINT_TC        0       // TRIP-CLOSE 방식제어
#define DNP_PULSE_CLOSE     1       // PULSE 방식제어 (CLOSE 우선)
#define DNP_PULSE_TRIP      2       // PULSE 방식제어 (TRIP 우선)
#define DNP_LATCH_CLOSE     3       // LATCH 방식제어 (CLOSE 우선)
#define DNP_LATCH_TRIP      4       // LATCH 방식제어 (TRIP 우선)
#define DNP_RESET_CLOSE     5		// 광명부하개폐기, RESET 제어 - CLOSE 전용
#define DNP_RESET_TRIP      6		// 광명부하개폐기, RESET 제어 - CLOSE 전용
#define DNP_AO_CONTROL      7       // DNP 계전기 AO 출력제어


#define TRIP                1
#define CLOSE               2

#define TRIP_CONTROL        0x80
#define CLOSE_CONTROL       0x40

/* IEC-HOST : 상위 제어명령에 대한 구분... */
#define CONTROL_CLEAR       0
#define CONTROL_SELECT      1
#define CONTROL_OPERATE     2

/* --------------------------------------- */
/* VMESIO : SCAN Task : 프로토콜 타입 정의 */
/* --------------------------------------- */
#define SCAN_DNP            0		
#define SCAN_MODBUS         1
#define SCAN_ASYNC     	    2		/* 8 bit, No Parity */ 
#define SCAN_IEC    	    3		/* 8 bit Even Parity */
#define SCAN_HARRIS    	    4		/* 7 bit Odd Parity */
#define SCAN_BUS     	    5

#define SCAN_IEC_101   	    5		/* 8 bit Even Parity */

/* --------------------------------------- */
/* SCAN Task : 채널별 프로토콜 타입 정의   */
/* --------------------------------------- */
#define	PROTOCOL_DNP	    0       // DNP 통신 계전기 
#define	PROTOCOL_MODBUS	    1       // MODBUS 통신 계전기
#define	PROTOCOL_MPLC	    2       // 미쯔비시 계전기 
#define	PROTOCOL_BUS	    3       // CU 내부 기능모듈 (VMEBUS)

#define	PROTOCOL_LOCATOR1   4       // 예비 : LOCATOR 처리
#define	PROTOCOL_LOCATOR2   5

#define MMI_STATUS_PT       1
#define MMI_ANALOG_PT       2
#define MMI_CONTROL_PT      3


/* ------------------------------------ */
/*  DEVICE 포인트 Index                 */
/* ------------------------------------ */
#define INDEX_ALL_DEVICE    0               // Device Point : 전체 계전기 통신상태
#define INDEX_AUTO_MANUAL   65              // Device Point-65 : 수동/자동 상태, 0:자동, 1:수동
#define INDEX_CPURUN_STS    66              // Device Point-66 : CPU A/B 동작모드, 0: CPU-A, 1:CPU-B
#define INDEX_CPU_A_STS     67              // Device Point-67 : CPU-A 동작상태, 0:정상, 1:이상  
#define INDEX_CPU_B_STS     68              // Device Point-68 : CPU-B 동작상태, 0:정상, 1:이상 
#define INDEX_SYS_CHANGE    69              // Device Point-69 : 시스템 절체 


#define LOC_STX     0x02
#define LOC_ETX     0x03

#define LOC_ACK     0x06
#define LOC_NACK    0x15

#define LOC_ENQ     0x05
#define LOC_BEL     0x07

#define LOC_ENQ_SEARCH      1
#define LOC_DATA_READY      2


typedef struct
    {
        byte    eventCode;              // 0x20 : SOE, 0x30 : 장치포인트
        byte    devid;                  // 0 ~ 63
        word    devPoint;               // 0 ~ 4095            
        byte    state;
        byte    reserved;

        byte    timeBuf[6];             // DNP Time
        
    } __attribute__ ((packed)) LINK_SOE;
    
/* ---------------------------------------------------- */
/* STURECTURE 정의 : POINTBUF                           */
/* 1. DI/DO/AI등 CU내 포인트 운영을 위한 스트럭쳐       */
/* ---------------------------------------------------- */   
typedef struct
        {
        byte    cpuControl;         // 이중화 CPU 상태정보 => WRITE
        byte    cpuStatus;          // 이중화 CPU 상태정보 => READ
        short   offline;            // 이중화 CPU 통신상태
        
        word    comFailTick;        // 통신상태 Check
       	
       	short   chgOpcode;          // 이중화 CPU 정보연계
        short   cmdFlag;            // 이중화 절체상태 Check
        short   linkStatus;
        short   changeCPU;          // 이중화 절체 명령
        
        byte    runStatus;          // CU Run 상태
        byte    rcvStatus;          // 수신 CU Run 상태
        byte    sndSeqno;
        byte    rcvSeqno;
        
        
		/* ------------------------------------ */
        /*  이중화 CPU : 데이터베이스 연계      */
        /* ------------------------------------ */
		short	timeSync;           
        short   chksumReq;      
        short	dbDownFlag;
        
        short	sysDbDown;
        short	deviceDbDown;
        short	harrisDbDown;
        short	pointDbDown;
        short	modbusDbDown;
        
        short	endFlag;
        short	dbDownPoint;
        short	dbDownIndex;
        
        /* ------------------------------------ */
        /*  이중화 CPU : 상위 제어정보          */
        /* ------------------------------------ */
        short   cntDev;
        short   cntPoint;
        short   cntTCF;
        short   cntHost;
        short   rcvONtime;
        short   rcvOFFtime;
        short   cntFlag;
        
        short   cntSndTick;            
                                        
        byte    eventRear;              // 이벤트 정보 전송용 Rear Pointer
        word    historyRear;            // History 정보 전송용 Rear Pointer
                                                    
        /* ------------------------------------ */
        /*  이중화 CPU : SOE Queue              */
        /* ------------------------------------ */
        byte    front;
        byte    rear;
        byte    report;
        //byte    temp;
                              
        LINK_SOE    linkSOE[256+2];
                                            
        } __attribute__ ((packed)) LINK_MSG;
        
/* ---------------------------------------------------- */
/* STURECTURE 정의 : POINTBUF                           */
/* 1. DI/DO/AI등 CU내 포인트 운영을 위한 스트럭쳐       */
/* ---------------------------------------------------- */   
typedef struct
        {
       	/* ------------------------ */
        /* POINT 운영 정보 ...      */
        /* ------------------------ */
        byte    status;             /* Point status */   
        byte    oldsts;             /* 이전 상태값 */
        
        int     pointData;          /* Analog Point Data */
        int     locData;            // Locator : 
        float	floatData;			/* FLOAT 데이터 */
        
        byte    pointComFail;		// Locator : 
        byte    pointOver;          // Locator : Point Over
        byte    locEvent;           // Locator : 3FF 이벤트
        byte    limitEvent;         // Locator : Limit 초과 이벤트
        byte    locStatus;
         
        //word    controlBase;        /* MODBUS Control Base */
        byte    costick;            /* Point previous status */   
        byte    config;             /* device 포인트 : 포인트 사용여부 ... */  
        byte    cos;
        //byte    cosTime;            // Point COS set Time [ms] 
        byte    debounce;           // Point Debounce Time [ms]     
        word    cntrTime;           // Point Control Delay Time [ms]  

#if 0
		/* ----------------------------- */
		/*	미쯔비시 계전기 인터락 처리 */
		/* ----------------------------- */
		short	interLockPoint;		// InterLock Flag
		short	lock_Dev;			
		short	lock_Point;
#endif
		                
        /* ------------------------ */
        /* POINT Config 정보 ...    */
        /* ------------------------ */
        byte    devNo;              /* device 포인트 : 계전기 번호*/   
        byte    ptConfig;           /* device 포인트 : Config */    
        word    devPt;              /* device 포인트 : 포인트 번호 */
        
        byte    devType;            /* device 포인트 : Device TYPE  */    
        byte    ptType;             /* device 포인트 : 포인트 TYPE  */     
        
        word	modBase;			/* device 포인트 : MODBUS Base Addr */
        word	modIndex;   		/* device 포인트 : MODBUS Index */
        word    dbmax;              /* device 포인트 : MAX-Value */
        byte    writeBlockIndex;    /* device 포인트 : MODBUS Write Block Index[0~3] */
		
        /* HOST 별 포인트 구성정보 */                        
        byte    dbport1;            /* device 포인트 : HARRIS-HOST #1 Port  Index */    
        byte    dbpoint1;           /* device 포인트 : HARRIS-HOST #1 Point Index */ 
        byte    dbport2;            /* device 포인트 : HARRIS-HOST #2 Port  Index */    
        byte    dbpoint2;           /* device 포인트 : HARRIS-HOST #2 Point Index */ 
        
        word    landisIndex;            /* device 포인트 : DNP-HOST #3 Index */    
        word    hostIndex[MAX_HOST];    /* device 포인트 : DNP-HOST #3 Index */       
        
        byte    nameStr[20];
        
        } __attribute__ ((packed)) POINT_BUF;


/* ---------------------------------------------------- */
/* SCAN PORT 별 DEVICE Config                           */
/* ---------------------------------------------------- */
typedef struct
    {
        byte    active;             /* SCAN-PORT Active Status */      
        byte    protocol;       	/* SCAN-PORT : 통신 프로토콜 */
        byte	comMode;			/* SCAN-PORT : 통신모드 */
        byte	comPort;			/* SCAN-PORT : 통신 포트 */
        byte    comSpeed;          	/* SCAN-PORT : Comm Speed */
        byte    comDelay;			/* SCAN-PORT : Comm Delay ,    가상계전기 통신 Addr */
        byte	offCount;           /* SCAN-PORT : Offline Count , 가상계전기 대상 Addr */
        
        byte	ipAddr[16];
        byte	nameStr[16];
        
        word	tcpPort;
        short   virtualMode;        /* 가상계전기 운영모드 */
        short   comAddr;            /* 가상계전기 : 실제 통신 Address */
        short   targetAddr;         /* 가상계전기 : 내부 계전기 번호 */
        
        short   tcpMode;            /* 계전기 이중화 구성시 ... 절체여부 */
        short   changeMode;         /* 계전기 이중화 구성시 ... 절체여부 */
        
        byte    portid;             /* SCAN-PORT : index  */
        byte    devNum;
        byte    device[MAX_MODULE];

        short   scaninit;
        
        /* SCAN 구조체별 통신 Buffer */
        byte    sndBuffer[128];
        byte    rcvBuffer[256];
                            
    } __attribute__ ((packed)) SCAN_CONFIG;
        

/* 
*	MODBUS Protocol 구성정보 
*/
typedef struct
    {
        byte    useFlag;           	/* 사용여부 */
        byte    opcode;           	/* OPCODE */
        word    baseAddr;   		/* BASE Address */
        word    baseOffset;   		/* BASE Address */

		word	dataNum;			/* point Number */
		byte	dataType;			/* Data Type */
		byte	dataFormat;			/* Data Format */
		
		byte	swepEnb;
		byte    function;

		word	commIndex;          /* READ-Block 별 통신회수 제어 */
		
    } __attribute__ ((packed)) MODBUS_READ;
    
typedef struct
    {
        byte    useFlag;           	/* 사용여부 */
        byte    opcode;           	/* OPCODE */
        word    baseAddr;   		/* BASE Address */
        word    baseOffset;   		/* BASE Address */

		word	dataNum;			/* point Number */
		byte	dataType;			/* Data Type */
		byte	dataFormat;			/* Data Format */
		word	reserved;
		
		word	tripData;
		word	closeData;

    } __attribute__ ((packed)) MODBUS_WRITE;    


typedef struct
    {
        byte    useFlag;           	/* 사용여부 */
        byte    cntrReq;            // 포인트 제어요구... 
        //byte    devid;           	// 계전기 번호, 0, 1, 2
        word    devPt;   		    // 계전기 포인트 번호 
        
        byte    activeState;        // 제어수행 상태
        byte    cntrState;          // 포인트 제어상태

    } __attribute__ ((packed)) LOCK_POINT_BUF;    


/* 미쯔비시 계전기 대상 포인트 구조 */
typedef struct
    {
        byte    config;           	/* 사용여부 */
        byte    pointType;          // 해당 포인트 TYPE, 1 : 상태, 2: 제어
        word    mplcIndex;          // MPLC-포인트 Index 1...4096
        word    devPoint;           // 해당 계전기내 포인트 번호, 0... 4096
         
        byte    pointState;         // 해당 MPLC 포인트 상태
        
        byte    controlReq;         // 제어요청 Flag
        byte    cntrTCF;            // 제어요청 TYPE

    } __attribute__ ((packed)) MPLC_POINT_BUF;    
        

/*
*   VMECU - 계전기별 운영 Buffer
*/
typedef struct
    {
        byte    *bfptr;             /* BUS 방식 : Base Address */
        
        short   locMode;            // Locator : 데이터 수집모드 
        short   locControl;         // LOCK Control 사용유무
        short   locPtNum;           // 계전기별 쇄정 포인트 수
        short   lock_allControl;    // 쇄정포인트 사용유무
        short   initialLoc;
        
        short   locEvent;           // LOCATOR Event ....
                                    
        short   ioid;               /* 계전기 번호 */
        short   hostid;             /* CU 번호 */
        
        short   type;				/* 계전기 모듈 타입 */			
        short   scan;				/* 계전기 사용유무 */
        short   scanPort;          	/* 계전기 통신포트 */
        
        short   vmeInitial;         /* 해당모듈이 VME 모듈인 경우... */
        short   vmeChkFlag;         /* 기능모듈 : VME Access Check Flag */
        short   vmeReset;           /* 기능모듈 : 재기동 Flag */
        short   vmeResetTick;       /* 기능모듈 : 재기동 Count */
        short   oldsec;
        
        short   modbusRcvOK;        /* MODBUS Block별 Index */       
        short   reqAckDelay;
        
        byte	nameStr[20];		/* 계전기 Name String */
        
        //byte    soeFront;           
        byte    soeRear;            /* VMEBUS : SOE 정보 Rear 포인터 */
        byte    stsByteCnt;         /* CPU 이중화연계 : 상태정보 수 */
        
        byte    cursts[MAX_DI_DATA_COUNT];      /* DEVICE 별 포인트 정보 : 상태포인트 Current */
        byte    df[MAX_DI_DATA_COUNT];          /* DEVICE 별 포인트 정보 : 상태포인트 Old */
        byte    linkSts[MAX_DI_DATA_COUNT];     /* Slave 전송용 상태 데이터용 버퍼 */
        
        byte    devType[MAX_DEV_DI_POINT];      /* 계전기별 - 포인트 TYPE */
        
        word    devDiPoint;         /* DEVICE : 장치별 DI 포인트 수 */
        word    devDoPoint;         /* DEVICE : 장치별 DO 포인트 수 */
        word    devAiPoint;         /* DEVICE : 장치별 AI 포인트 수 */
        word    devAoPoint;         /* DEVICE : 장치별 AO 포인트 수 */
        word    devBCPoint;         /* DEVICE : 장치별 Binary Counter 포인트 수 */
        
        short   initial;            /* DEVICE 초기화 상태 */
        short	initDevStatus;		/* 계전기 상태정보(HOST) 0: 미완료, 1: 완료 */ 
        short   online;             /* DEVICE Online 상태 */
        short	offlineTick;        /* DEVICE Offline Count Tick */
        short	offCount;			/* 통신 Offline 기준 */
        
        /* ------------------------ */
        /* 지멘스(TCPIP) 이중화 관련*/
        /*  - MODBUS 프로토콜 관련  */
        /* ------------------------ */
        short   activeMode;         /* 계전기 통신 Active Flag... */
        short   activeFlag;         /* 계전기 통신 Active Flag... */
        short   activeTick;
        short	activeChan;
        
        LOCK_POINT_BUF  locPoint[16];   // 계전기별 쇄정/리스트    
        
        /* ------------------------ */
        /*	DNP 프로토콜 관련 		*/
        /* ------------------------ */
        byte    rcvIIN[2];          /* DEVICE : DNP-IIN 상태정보 */
        
        short   rcvAC;       
		short	rcvTH;
        short   rcvACfunc; 
        short   rcvACseq; 
        short   rcvObj;
        short   rcvVar;
        short   rcvIndex;
        short   rcvQcode;

        word    rcvStart;
        word    rcvStop;
        short   rcvQnum;
        short   rcvDataPos;
        
        short   faultResetReq;      /* MODBUS : Remote-Reset Request */

#if 0
        /* ------------------------------------ */
        /*  농어촌공사 RTU - 제어정보           */
        /* ------------------------------------ */
        short   pts4500_CntrReq;    /* 농어촌 공사용 제어시...        */
        word    ptsCntrData[8];     /* 제어용 데이터 */
        word    feedCntrData[8];
#endif
        
        /* 계전기 제어관련 */
        short   rcvONtime;          // 릴레이 동작시간...
        short   rcvOFFtime;             // 계전기 제어시간

        short   clearReq;
        short   clearTick;
                        
        short   selectReq;
        short   operateReq;
        short   cntPoint;           // 제어 포인트
        short   cntTcf;             // 제어타입 : TRIP/CLOSE
        short   cntrType;           // 계전기 제어방식 : Pulse/Latch ... 
        
        short   devRestart;        
        short   timeSyncReq;
        short   eventDumpReq;
        short   classReq;
        short   class1Req;
        short   class2Req;
        short   class3Req;
        short   statusDump;		// Status Input Dump Flag
        short   analogDump;		// Analog Input Dump Flag
        short   anaOutDump;		// Binary Counter Dump Flag
		short	countDump;		// Binary Counter Dump Flag
		
        short   aoDumpReq;
        short   aoVpoint;
        short   aoVpointData;        
        short   anaVOutSelect;      /* V0 Reset 관련 */
        short   anaVOutOpr;

        /* [Common]  Reference variable */
        short   sndLinkSts;
        short   rcvLinkSts;
        short   sndFcbBit;
        short   sndFcvBit;
        short   rcvFcbBit;
        short   rcvFcvBit;
                
        short   ACseqFail;                
        short   acSndSeqno;
        short   thSndSeqno;

        short   modbusRcvFail;          // MODBUS Rcv Frame Error 
        short   reqDataCount;           // MODBUS 수신요청 데이터 수 
        short   reqDataOpcode;          // MODBUS 수신요청 OPCODE 
        
		/* ------------------------ */
        /*	프로토콜 구성정보 관련 	*/
        /* ------------------------ */
        short	dnpFileNo;				// DNP 프로파일 번호
        short   dualIED;                /* 계전기 - TCPIP 기반 이중화 통신 구성 여부 */
        short	modbusFileNo;			// MODBUS 프로파일 번호
        short	readIndex;				// MODBUS READ-BLOCK Index
        short	writeIndex;
        short	defaultRead;			// MODBUS Default READ
        
        MODBUS_READ		readBlock[8];
        MODBUS_WRITE	writeBlock[4];
        MODBUS_READ		timeBlock;
        
        
        short   sndFlag;
        short   timeOutTick;
        short   comFailTick;
        
        short   appFailTick;        // APP 데이터 송신실패...
        ulong   comSndCount;        // DEVICE send Count
        ulong   comRcvCount;        // DEVICE receive Count
        word    oldRcvCount;

	    POINT_BUF       diPtBuf[MAX_DEV_DI_POINT];          // 전자식배전반 DI 상태포인트
	    POINT_BUF       doPtBuf[MAX_DEV_DO_POINT];          // 전자식배전반 DO 상태포인트
	    POINT_BUF       aiPtBuf[MAX_DEV_AI_POINT];          // 전자식배전반 AI 상태포인트
	    POINT_BUF       countBuf[MAX_DEV_COUNT_POINT];      // 전자식배전반 카운터 포인트

        /* -------------------------------- */
        /*  계전기별 Network 구성정보       */
        /* -------------------------------- */
        char	ipString[16];		            /* Master : SCAN IP Address */
        char	ipString2[16];		            /* Slave  : SCAN IP Address */
        word    netPort;			            /* SCAN TCPIP Port번호 */
        
        /* ------------------------------------ */
        /*  TCP/IP 통신 : DEVICE 별 구조체      */
        /* ------------------------------------ */
        struct sockaddr_in	serv_addr;

        short   tcpipThread;
        
        short   SocketID;           /* for Client */
        //
        int    SocketFd;           /* for Client */
        short   newSocketFd;        /* for Client */
        short   connectStatus;
        short   connectFail;
        
        /* MODBUS 통신 Buffer Define */
        byte    sndBuffer[128];
        byte    rcvBuffer[256];
        
        byte    txCount;
        byte    rxCount;
        
        /* DNP 통신 Buffer Define */
        DNP_APP_FRAME   rcvAppFrame;
        DNP_APP_FRAME   sndAppFrame;
        DNP_DATA_LINK   dataLinkFrame;

        /* -------------------------------- */
        /*  계전기별 통신포트 구성          */
        /* -------------------------------- */
        VME_CHAN_DCB    *vmeChan;
        TTY_DESC        *ttyPort;
        
    } __attribute__ ((packed)) DNP_DEVICE;


typedef struct
    {
        int     flag;
        
        int     pointData;              // Varaition #1 : 32Bit Analog Input with Flag
        float   floatData;
        //word    pointDataV2;          // Varaition #2 : 16Bit Analog Input with Flag   
        //float   pointDataV5;          // Varaition #5 : 32Bit Float Analog Input
        byte    lowdata[4];

    } __attribute__ ((packed)) DNP_ANA_INPUT;


/* ------------------------------------ */
/*  IEC-HOST CLASS1 : SOE Queue         */
/* ------------------------------------ */    
typedef struct
    {
    	word	baseAddr;
        byte	status;
        byte    soeTime[3];         
    } __attribute__ ((packed)) IEC_SOEQ_ENTRY;  

typedef struct
    {
        byte    front;
        byte    rear;
        IEC_SOEQ_ENTRY  soeQueue[256+2];
    } __attribute__ ((packed)) IEC_SOE_QUEUE;  


/* ------------------------------------ */
/*  CLASS1 : SOE Queue                  */
/* ------------------------------------ */    
typedef struct
    {
        /* QCode 7 : Single field Quantity, 포인트가 256 이하인 경우    */
        byte    soeBuf[12];         
    } __attribute__ ((packed)) DNP_SOEQ_ENTRY;  

typedef struct
    {
        byte    front;      // HOST 보고용 Flag
        byte    rear;
        
        byte    front1;     // CPU 링크용 Flag
        byte    rear1;
        
        DNP_SOEQ_ENTRY  soeQueue[256+2];
    } __attribute__ ((packed)) DNP_SOE_QUEUE;  


/* ------------------------------------ */
/*  CLASS2 : COS Queue                  */
/* ------------------------------------ */
typedef struct
    {
        byte    cosBuf[4];
    } __attribute__ ((packed)) DNP_COSQ_ENTRY;  

typedef struct
    {
        byte    front;
        byte    rear;
        DNP_COSQ_ENTRY  cosQueue[256+2];

    } __attribute__ ((packed)) DNP_COS_QUEUE;                  

/* ------------------------------------ */
/*  CLASS3 : COA Queue                  */
/* ------------------------------------ */
typedef struct
    {
        byte    pointIndex;             // HOST Point Number                
        byte    pointFlag;              //      Point Status
        byte    pointData[4];           //      Point Data
        byte    dnpTime[6];             //      DNP Time
    } __attribute__ ((packed)) DNP_COAQ_ENTRY;  

typedef struct
    {
        byte    front;
        byte    rear;
        DNP_COAQ_ENTRY  coaQueue[256+2];

    } __attribute__ ((packed)) DNP_COA_QUEUE;  
    
typedef struct
        {
        /* ------------------------ */
        /* POINT Config 정보 ...    */
        /* ------------------------ */
        byte    devNo;
        word    devPt;
        byte    port;           // HARRIS Port
        byte    point;          // HARRIS Point
        byte    type;           // 포인트 타입 : 계전기 또는 시스템   
        byte    config;         // 제어포인트 설정여부
        
        byte    cntrConfig;     // 제어방식 : Pulse/Latch...
        
        word    dbmax;          // 제어 인덱스 
        
        word	iecIndex;			// IEC HOST의 경우...                        
        } __attribute__ ((packed)) CONTROL_INFO;

typedef struct
        {
        /* ------------------------ */
        /* POINT Config 정보 ...    */
        /* ------------------------ */
        byte    devNo;
        word    devPt;
        byte    type;
        byte    config;
        
        byte    multiPoint;
        //word    dbmax;
        word	iecIndex;			// IEC HOST의 경우...
                        
        } __attribute__ ((packed)) DIPOINT_INFO;

typedef struct
        {
        /* ------------------------ */
        /* POINT Config 정보 ...    */
        /* ------------------------ */
        byte    devNo;
        word    devPt;
        byte    type;
        byte    config;
        //word    dbmax;

		word	iecIndex;			// IEC HOST의 경우...                        
        } __attribute__ ((packed)) AIPOINT_INFO;


/* ------------------------------------ */
/*  HOST 통신상태 정보                  */
/* ------------------------------------ */
#define HOST_NOT_READY      0   
#define HOST_READY_STS      1               // 예비 채널 통신 (IIN-Check)
#define HOST_ACTIVE_STS     2               // 주채널 통신 (DUMP)
  
                                                
typedef struct
        {                 
        short   id;                     // HOST ID, 0,1,2..
        short   hostStatus;             // Monitoring 보고 : HOST 상태정보
        
        short   hostActive;             // HOST 주/예비 채널 상태
        short   iinRcvTick;
        short   activePort;
                            
        /* -------------------------------- */
        /*  IEC-HOST 운영관련 데이터베이스 변수 */
        /* -------------------------------- */
		short	rcvCOT;
		        	
        /* -------------------------------- */
        /*  HOST 운영관련 데이터베이스 변수 */
        /* -------------------------------- */
        short   hostDualMode;           // HOST 듀얼운영 모드
        short   hostProtocol;           // HOST 통신 프로토콜
        short   hostComSpeed;           // HOST 통신속도 
        short   hostComType;            // HOST 통신타입
        word    hostid;                 // HOST 별 상위 통신 ID
        word    rtuAddr;                // HOST 별 RTU 통신 ID
        short   masterChan;             // HOST 별 MASTER 통신포트
        short   slaveChan;              // HOST 별 SLAVE  통신포트

        /* HOST : Network 구성 */		
		word	tcpPort;
		short	tcpMode;
		short   function;
		
		short	mst_Net1;           /* HOST Network 포트 #1: eth0/eth1/eth2 */
        char	mst_IPAddr1[16];	/* HOST IP Address #1 */
        short	mst_Net2;           /* HOST Network 포트 #2: eth0/eth1/eth2 */
        char	mst_IPAddr2[16];	/* HOST IP Address #2 */

        short	slv_Net1;           /* HOST Network 포트 #1: eth0/eth1/eth2 */
        char	slv_IPAddr1[16];	/* HOST IP Address #1 */
        short	slv_Net2;           /* HOST Network 포트 #2: eth0/eth1/eth2 */
        char	slv_IPAddr2[16];	/* HOST IP Address #2 */
        
        short   cosClass;                     
        short   soeClass;
        short   coaClass;
        short   unsolEvent;
        short   unsolMode;
        short	timeOutTick;			// Time Out Tick
        
        /* ------------------------------------ */
        /*  DNP HOST 관련                       */
        /* ------------------------------------ */
        short   cosReport; 
        short   coaReport;      
        short   soeReport;
        short   class1Report;  
        short   class2Report; 
        short   class3Report;   
        
        word    soeRear;
        word    coaRear;
        word    cosRear;
        
        short   type;
        short   scan;
        short   initial;
        short   status;             // DEVICE Status 
        short   online[2];
        short   offlineTick;
        byte    rtuIIN[2];
        
        short   cuRestart;
        
        short   sndACseq;
        
        short   rcvAC;  
        short   rcvTHseq;  
        short   rcvACseq;  
        short   rcvACfunc;      
        short   rcvObj;
        short   rcvVar;
        short   rcvIndex;
        short   rcvQcode;

        word    rcvStart;
        word    rcvStop;
        word    rcvQnum;
        short   rcvDataPos;
        
        /* [Common]  Reference variable */
        short   sndLinkSts;
        short   rcvLinkSts;
        short   sndFcbBit;
        short   sndFcvBit;
        short   rcvNextFCV;         // 다음명령 FCV 예상
        short   rcvNextFCB;
                
        short   sndACseqno;
        short   sndTHseqno;
        short   rcvEndOk;   
        short   thSndSeqno;    

        short   sndFlag;
        
        word    comFailTick[2];             // HOST 통신 Fail Count
        word    resetCount;                 // DNP : HOST Reset-LINK Count

		short   rtuIndex;                   // HARRIS : RTU Index
        short   rtuid;                      // HARRIS : RTU Address    
        short   rcvPACKET;                  // HARRIS : receive Packet state
        short   noCommand;
        byte    txcnt;                      // HARRIS : 송신 Count
        byte    rxcnt;                      // HARRIS : 수신 Count

        /* -------------------------------------------- */
        /* DNP-HOST 통신용 Buffer  (Master/Slave)       */
        /* -------------------------------------------- */
        DNP_APP_FRAME   rcvAppFrame[2];
        DNP_APP_FRAME   sndAppFrame[2];
        DNP_DATA_LINK   dataLinkFrame[2];
        
        DNP_APP_FRAME   sndUnsolFrame[2];
                
        /* ---------------------------- */
        /*  HARRIS/LANDIS 통신용 버퍼   */
        /* ---------------------------- */
        byte    sndBuffer[256];
        byte    rcvBuffer[256];
        

        /* ---------------------------- */
        /*  상위 HOST 통신용 버퍼       */
        /* ---------------------------- */
        short   diPtNum;
        short   doPtNum;
        short   aiPtNum;
        short   aoPtNum;
        
        short   analogDump;                 // for IEC-HOST : Analog Dump 주기
        
        short   diIndexWord;                // DI Point 최대 수 : 256보다 큰경우 WORD 처리 
        short   aiIndexWord;                // AI Point 최대 수 : 256보다 큰경우 WORD 처리 
        
        /* ---------------------------- */
        /* HARRIS-HOST 데이터 구조      */
        /* ---------------------------- */
       	RTU             rtubuf[MAX_RTU];              /* HARRIS RTU Structure */
		PORT_DB         portdb[MAX_PORT];             /* HARRIS #1 PORT Structure */
		CONTROL_INFO    harrisCntr[MAX_DNP_DO_POINT];

#if 0		
        /* ---------------------------- */
        /*  IEC 통신용 버퍼             */
        /* ---------------------------- */
        IEC_DATA_LINK   dataLinkIEC;
        
        /* ---------------------------- */
        /* IEC-HOST 데이터 구조         */
        /* ---------------------------- */
        byte            iec_singleData[MAX_DNP_DI_POINT];       // IES-HOST 별 상태포인트 영역 
        IEC_SOE_QUEUE   iecSOEQ;                                // HOST 별 SOE Queue...
#endif
        
        /* ---------------------------- */
        /* DNP-HOST 데이터 구조         */
        /* ---------------------------- */
        CONTROL_INFO    controlInfo[MAX_DNP_DO_POINT];
        DIPOINT_INFO    stateInfo[MAX_DNP_DI_POINT];
        AIPOINT_INFO    analogInfo[MAX_DNP_AI_POINT];

       	DNP_SOE_QUEUE   dnpSOEQ;                                // HOST 별 SOE Queue...
        DNP_COS_QUEUE   dnpCOSQ;                                // HOST 별 COS Queue...
        
        /* DNP, LANDIS-HOST 데이터 구조 */
        byte            sts_pointData[MAX_DNP_DI_POINT];        // HOST 별 상태포인트 영역 
        byte            multi_point[MAX_DNP_DI_POINT];          // HOST 별 MULTI-POINT 영역 
        
        DNP_ANA_INPUT   ana_pointData[MAX_DNP_AI_POINT];        // HOST 별 계측포인트 영역 

        /* ------------------------------------ */
		/* HOST 통신포트 정의(Master/Slave)     */
		/* ------------------------------------ */
        VME_CHAN_DCB    *vmeChan[2];                // VME Channel 포인터 지정
        TTY_DESC        *ttyPort[2];
        
        /* ------------------------------------ */
		/* HOST TCPIP-SERVER 정의(Master/Slave) */
		/* ------------------------------------ */
        short   acceptFailCnt;                      // HOST 접속이상 Count...
        
        int     socketServerFd;                     // TCPIP- for Server Socket
        struct  sockaddr_in	srvAddr;                // TCPIP- for Server Socket
        
        int     threadActive[2];                    // TCPIP-Thread 동작상태
                
        /* 접속대상 PORT 관리 */
        int     portResetCount;
        int     acceptPORT[2];
        
        } __attribute__ ((packed)) HOST_DCB;

    
/* ---------------------------------------------------- */
/* STURECTURE 정의 : DATABASE                           */
/* 1. LCU내 데이터베이스용 스트럭쳐                     */
/* 2. System DB, Module DB, Point DB 등등               */
/* 3. start addr : 0x300000                             */
/* 4. end   addr : 0x302000                             */
/* ---------------------------------------------------- */   

/* 
*	HOST 구성정보 : 통신관련 파라메터
*/
typedef struct
    {
        byte    runMode;            /* HOST 운영모드 : [0]사용안함, [1]개별, [2]이중화  */
        byte    protocol;           /* HOST 통신 프로토콜 : HARRIS/LANDIS/DNP/MODBUS/IEC... */
        byte    comSpeed;           /* HOST 통신속도 */    
        byte    comMode;            /* HOST 통신모드 : RS232/MODEM/RS485/TCPIP */     
        byte    masterPort;         /* HOST MASTER 통신포트 */
        byte    slavePort;          /* HOST SLAVE  통신포트 */
        
        /* 2014.06.27 : HOST 통신 파라메터 변경 */
        byte	tcpipMode;			/* HOST TCPIP 운영모드 */
        byte    function;           /* 예약 : Function code */
        byte	netPort[2];			/* HOST TCPIP Port번호 */
        
        byte	mst_Net1;           /* HOST Network 포트 #1: eth0/eth1/eth2 */
        byte	mst_IPAddr1[16];	/* HOST IP Address #1 */
        byte	mst_Net2;           /* HOST Network 포트 #2: eth0/eth1/eth2 */
        byte	mst_IPAddr2[16];	/* HOST IP Address #2 */

        byte	slv_Net1;           /* HOST Network 포트 #1: eth0/eth1/eth2 */
        byte	slv_IPAddr1[16];	/* HOST IP Address #1 */
        byte	slv_Net2;           /* HOST Network 포트 #2: eth0/eth1/eth2 */
        byte	slv_IPAddr2[16];	/* HOST IP Address #2 */

        byte	hostAddr[2];		/* HOST DNP - 센터 Address */
        byte	rtuAddr[2];			/* HOST DNP - RTU Address */		
        
        byte	nameStr[20];		/* HOST Name String */

    } __attribute__ ((packed)) DB_HOST_CONFIG;

/* 
*	HOST PARAMETER 
*/
typedef struct
    {
        byte    soeClass;           /* HOST DNP : SOE Class 지정 */
        byte    cosClass;           /* HOST DNP : COS Class 지정 */
        byte    coaClass;           /* HOST DNP : COA Class 지정 */
        byte    unsolite;           /* HOST DNP : Unsolite 지정  */
        
        byte	timeout;			/* HOST Time-out */
        byte	reserved1;
        byte	reserved2;
        byte	reserved3;
        
    } __attribute__ ((packed)) DB_HOST_PARAMETER;

/* 
*	CU 구성정보 
*/
typedef struct
    {

        byte    dualCpu;            /* CU - CPU 이중화 운영모드  */
        byte    scuUseFlag;        	/* CU - SCU 모듈 사용유무*/
        byte    statusDump;        	/* CU - STATUS Dump 주기 */
       	byte    analogDump;        	/* CU - ANALOG Dump 주기 */                

		byte	dualRtu;            /* CU - Module 이중화 운영모드  */
        byte	mmiUseFlag;         /* CU - MMI 모듈 사용유무*/
        
        byte    soeDebounce;        /* FUNC1 : MODBUS SOE Debounce */
        byte    sysDBCheck;         /* FUNC2 : DB Check Flag */
        byte    hostDualWDT;        /* FUNC3 : HOST 통신 WDT */
        byte    reserved4;
        
        /* Master CPU Network 구성정보 */
        byte    net11UseFlag;        /* CU - Network#1 사용유무 */
        char	net11_IP[16];		/* CU - Network#1 IP  Address */
        char	net11_GW[16];		/* CU - Network#1 GW  Address */
        char	net11_SUB[16];		/* CU - Network#1 SUB Address */
        
        byte    net12UseFlag;        /* CU - Network#2 사용유무 */
        char	net12_IP[16];		/* CU - Network#2 IP  Address */
        char	net12_GW[16];		/* CU - Network#2 GW  Address */
        char	net12_SUB[16];		/* CU - Network#2 SUB Address */
        
        byte    net13UseFlag;        /* CU - Network#3 사용유무 */
        char	net13_IP[16];		/* CU - Network#3 IP  Address */
        char	net13_GW[16];		/* CU - Network#3 GW  Address */
        char	net13_SUB[16];		/* CU - Network#3 SUB Address */
        
        /* Slave CPU Network 구성정보 */
        byte    net21UseFlag;        /* CU - Network#1 사용유무 */
        char	net21_IP[16];		/* CU - Network#1 IP  Address */
        char	net21_GW[16];		/* CU - Network#1 GW  Address */
        char	net21_SUB[16];		/* CU - Network#1 SUB Address */
        
        byte    net22UseFlag;        /* CU - Network#2 사용유무 */
        char	net22_IP[16];		/* CU - Network#2 IP  Address */
        char	net22_GW[16];		/* CU - Network#2 GW  Address */
        char	net22_SUB[16];		/* CU - Network#2 SUB Address */
        
        byte    net23UseFlag;        /* CU - Network#3 사용유무 */
        char	net23_IP[16];		/* CU - Network#3 IP  Address */
        char	net23_GW[16];		/* CU - Network#3 GW  Address */
        char	net23_SUB[16];		/* CU - Network#3 SUB Address */

    } __attribute__ ((packed)) DB_CU_CONFIG;



/* 
*	계전기 통신포트 구성  
*/
typedef struct
    {
        byte    runMode;            /* SCAN 운영모드  */
        byte    protocol;           /* SCAN 통신 프로토콜 */
        byte    comMode;            /* SCAN 통신모드 */     
        byte	comPort;			/* SCAN 통신포트 */
        byte    comSpeed;           /* SCAN 통신속도 */    
        
        byte	comDelay;			/* SCAN 통신간격 */
        byte	offCount;			/* SCAN Offline Count */
        
        byte	ipString[16];		/* SCAN IP Address */
        byte	netPort[2];			/* SCAN TCPIP Port번호 */
        byte	tcpipMode;			/* SCAN TCPIP 운영모드 */
        
        byte	nameStr[20];		/* HOST Name String */

    } __attribute__ ((packed)) DB_SCAN_CONFIG;

/* 
*	계전기 구성정보 
*/
typedef struct
    {
        byte    scanPort;           /* 계전기 통신 포트 */
        byte    type;           	/* 계전기 TYPE */
        byte    scan;        		/* 계전기 SCAN 여부 */
		byte	dualIED;			/* 계전기 - 계전기 이중화 여부 */
		//byte	dnpFileNo;			/* 계전기 - DNP 프로파일 */
		byte	modbusFileNo;		/* 계전기 - MODBUS 프로파일 */
		byte    reserved;
		
		byte	di_ptnum[2];		/* 계전기 - DI 포인트 수 */		
		byte	do_ptnum[2];		/* 계전기 - DO 포인트 수 */		
		byte	ai_ptnum[2];		/* 계전기 - AI 포인트 수 */		
		byte	ao_ptnum[2];		/* 계전기 - AO 포인트 수 */				

		byte	nameStr[20];		/* 계전기 Name String */

		/* -------------------- */
		/*  계전기 Network 정보 */
		/* -------------------- */
		byte	ipString[16];		/* HOST IP Address */
        //byte	gwString[16];		/* HOST GW Address */
        //byte	subString[16];		/* HOST SUB MASK Address */
        
        byte	netPort[2];			/* HOST TCPIP Port번호 */
   		
    } __attribute__ ((packed)) DB_DEVICE_CONFIG;


/* ---------------------------------------------------- */
/* DEVICE POINT 관련 데이터베이스                       */
/* ---------------------------------------------------- */
typedef struct
    {
        byte    devNo;              /* device 번호 [1..32] */
        byte    devPt[2];           /* device 포인트 번호 [1..1024] */
        byte    devType;            /* devic TYPE  */    
        byte    ptType;             /* device 포인트 TYPE  */     
     
     	byte	modBase[2];			/* MODBUS base Address */
     	byte	modIndex[2];		/* MODBUS index */
        byte    ptConfig;           /* device 포인트 Config */
        byte    maxValue[2];        /* Point MAX Scale 값 ,Harris & Landis*/             
        
        byte    port1;              /* 주장치 PORT 번호  [1..16] */  
        byte    point1;             /* 주장치 POINT 번호 [1..64] */  
        
        byte    port2;              /* 주장치 PORT 번호  [1..16] */  
        byte    point2;             /* 주장치 POINT 번호 [1..64] */  
        
        byte    cntrFlag;           /* Control Flag */
        byte    writeBlock;         /* MODBUS Write Block */
        
        byte    hostIndex1[2];      /* 상위 호스트#1 Index 번호 [1..1024] */ 
        byte    hostIndex2[2];      /* 상위 호스트#2 Index 번호 [1..1024] */  
        byte    hostIndex3[2];      /* 상위 호스트#3 Index 번호 [1..1024] */  
        byte    hostIndex4[2];      /* 상위 호스트#4 Index 번호 [1..1024] */  
        byte    hostIndex5[2];      /* 상위 호스트#5 Index 번호 [1..1024] */     

		byte    onStr[10];      	/* ON  상태 이름 */
		byte    offStr[10];      	/* OFF 상태 이름 */
        byte    pointName[20];      /* 포인트 이름 */

    } __attribute__ ((packed)) DEVICE_PTCFG;

/* 
*	MODBUS Protocol 구성정보 
*/
typedef struct
    {
        byte    useFlag;           	/* 사용여부 */
        byte    opcode;           	/* OPCODE */
        byte    baseAddr[2];   		/* BASE Address */
        byte    baseOffset[2];   	/* BASE Address */

		word	dataNum;			/* point Number */
		byte	dataType;			/* Data Type */
		byte	dataFormat;			/* Data Format */
		byte	swepEnb;
		byte    function;

    } __attribute__ ((packed)) DB_MODBUS_READ;
    
typedef struct
    {
        byte    useFlag;           	/* 사용여부 */
        byte    opcode;           	/* OPCODE */
        byte    baseAddr[2];   		/* BASE Address */
        byte    baseOffset[2];   	/* BASE Address */

		word	dataNum;			/* point Number */
		byte	dataType;			/* Data Type */
		byte	dataFormat;			/* Data Format */
		word	reserved;
		
		byte	tripData[2];
		byte	closeData[2];

    } __attribute__ ((packed)) DB_MODBUS_WRITE;    

/* 
*	MODBUS Protocol 구성정보 
*/
typedef struct
    {
    	DB_MODBUS_READ	readBlock[8];
    	DB_MODBUS_WRITE	writeBlock[4];
    	DB_MODBUS_READ	timeBlock;
    	
    } __attribute__ ((packed)) DB_MODBUS_BLOCK;


#define MAX_DB_HOST            	5           // 최대 HOST 수 
#define MAX_DB_SCAN_PORT       	10       // 기존 : SCAN-PORT => 5
#define MAX_DB_MODULE          	64          
#define MAX_DB_POINT     		4096        // 운영상의 데이터베이스 상의 포인트 DB

/* ======================================== */
/*  VMECU- DATABASE 구성정보                */
/* ======================================== */
typedef struct
        {
		DB_HOST_CONFIG 		hostCfg[MAX_DB_HOST];			/* HOST 구성정보 : MAX 5 */
		DB_HOST_PARAMETER	hostParameter[MAX_DB_HOST];	/* HOST 운영파라메터 */
		DB_CU_CONFIG		cuConfig;					/* CU 운영정보 */
		
		DB_SCAN_CONFIG		scanConfig[MAX_DB_SCAN_PORT];	/* 계전기 통신포트 구성 */
		DB_DEVICE_CONFIG	relayConfig[MAX_DB_MODULE];	/* 계전기별 운영 구성 */
			
        byte    portdb1[64];                			/* HARRIS#1 PORT 구성정보 (32) */
        byte    portdb2[64];                			/* HARRIS#2 PORT 구성정보 (32) */
                    
        /* LANDIS Database config ... */
        byte    chassisNum;                 			/* Chassis Number */ 
        byte    chassis[16][16];            		    /* LANDIS RTU 구성정보 */

        DEVICE_PTCFG    	devPtConfig[MAX_DB_POINT]; 	/* 포인트 DB 데이터베이스 - 4096 */
        
        DB_MODBUS_BLOCK		modbusBlock[8];
        
        byte    sysParameter[32];

        byte    chksum[2];


	    } __attribute__ ((packed)) RTU_DATABASE;


/* ---------------------------------------- */
/*  MMI-DB 구성정보                         */
/* ---------------------------------------- */
typedef struct
    {
    	byte    devNo;
    	word    pointNo;
    	byte    ptType;
    	
    	byte    onStr[10];
    	byte    offStr[10];
    	byte    nameStr[20];
    	
    } __attribute__ ((packed)) MMI_POINT_CFG;

typedef struct
    {
    	byte    scan;
    	byte    portNo;
    	byte    nameStr[20];
    	
    } __attribute__ ((packed)) MMI_HOST_CFG;

typedef struct
    {
    	byte    scan;
    	byte    portNo;
    	byte    nameStr[20];
    	
    } __attribute__ ((packed)) MMI_MODULE_CFG;

/*
*/        
typedef struct
    {
    	MMI_HOST_CFG    mmiHost[MAX_HOST];
    	byte            cuName[20];
    	word            maxPoint;

        MMI_MODULE_CFG  mmiModule[MAX_MODULE];
    	MMI_POINT_CFG   mmiPoint[MAX_MMI_POINT];
        
        word    chksum;    	
        
    } __attribute__ ((packed)) MMI_DATABASE;

	    	    
/* .......... end of "RTUDB.H" ............... */    
