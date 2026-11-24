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
 
#if 0
 /* HISTORY 저장용 메모리 영역 */
#define VME_BASE_ADDRESS        0xF0000000       /* this definition is related with dd and cpld */
#define VME_SIZE                0x1000000        /* 32K */

#endif

#define VMESIO_START_ADDRESS    0x20000000      /* VME BASE ADDRESS */
#define VMESIO_SIZE             0x20000         /* 128K, 세인SIO 모듈당 Address Area  */

#define VMEIOM_START_ADDRESS    0x20040000                      /* VME BASE ADDRESS */
#define MODULE_OFFSET           0x1000                          /* 모듈간 공유메모리 영역 : 4Kbyte */
#define VMEIOM_SIZE             (MODULE_OFFSET * MAX_MODULE)    /* 4K X 32 , 모듈당 Address Area  */



#define	SIO_BOARD_MAX		    1	/* Number of SIO Board */
#define	SIO_CHANNEL_MAX		    8	/* Number of SIO Channel */


#define VME_SIO_CH1     0
#define VME_SIO_CH2     1
#define VME_SIO_CH3     2
#define VME_SIO_CH4     3
#define VME_SIO_CH5     4
#define VME_SIO_CH6     5
#define VME_SIO_CH7     6
#define VME_SIO_CH8     7

#define VME_SCAN_TUNNEL     VME_SIO_CH5
#define VME_SCAN_MASTER     VME_SIO_CH7
#define VME_SCAN_SLAVE      VME_SIO_CH8

#define TYPE_RS232      0
#define TYPE_MODEM      1
#define TYPE_RS485      2

#define SPEED_1200BPS   0
#define SPEED_2400BPS   1
#define SPEED_4800BPS   2
#define SPEED_9600BPS   3
#define SPEED_19200BPS  4
#define SPEED_38400BPS  5
#define SPEED_57600BPS  6
#define SPEED_115200BPS 7

#define STOP_1_BIT      0
#define STOP_2_BIT      1

#define PARITY_NONE     0
#define PARITY_ODD      1
#define PARITY_EVEN     2

#define DATA_8_BIT      0
#define DATA_7_BIT      1

#define FLOW_ON         0
#define FLOW_OFF        1

#define VME_CHAN_MASK   511
#define VME_CHAN_SIZE   512


typedef struct
    {
        word    chid;               // SIO-CH Channel ID
        word    localMaster;        // SIO-CH Valid Mask Pattern : 0x5580
        word    activeCount;        // SIO-CH active Count 
        word    resetFlag;          // SIO-CH Reset Flag 
        word    initEnd;            // SIO-CH initial End Flag  : 0x1234
        word    initReq;            // SIO-CH initial Request : 0x5580 
        
        word    sioValid;
        word    sioCount;
        
        word    mpuAccess;
        word    sioAccess;        
        
        short   rtcUpdateFlag;             // RTC Update  정보처리 요구 
        short   year;                      // RTC 설정용 
        short   month;                     // RTC 설정용 
        short   day;                       // RTC 설정용 
        short   week;                      // RTC 설정용     
        short   hour;                      // RTC 설정용 
        short   min;                       // RTC 설정용 
        short   sec;                       // RTC 설정용 
        
    } __attribute__ ((packed)) VME_SIODCB;


typedef struct
    {
        word    chid;               // SIO-CH Channel ID
        word    sioTestMode;        // SIO-CH Valid Mask Pattern : 0x5580
        word    activeCount;        // SIO-CH active Count 
        word    protocolType;       // SIO-CH 운영 프로토콜 TYPE , 0 : DNP, 1: MODBUS 
        word    initEnd;            // SIO-CH initial End Flag  : 0x1234
        word    initReq;            // SIO-CH initial Request : 0x5580 
        
        word    cfgType;            // Channel TYPE 
        word    cfgSpeed;           // Channel SPEED 
        word    cfgStopbit;         // Channel SPEED 
        word    cfgParity;          // Channel PARITY 
        word    cfgData;            // Channel DATA 
        word    cfgFlowCntr;        // Channel FLOW
        
        word    mpuAccess;
        word    sioAccess;
        
        word    rxFront;
        word    rxRear;
        word    rxCount;
        word    rxFlag;
        word    txCount;
        word    txFlag;
         
        word    rxbuf[VME_CHAN_SIZE];
        word    rcvbuf[VME_CHAN_SIZE];
        word    txbuf[VME_CHAN_SIZE];
        
    } __attribute__ ((packed)) VME_CHAN_DCB;
