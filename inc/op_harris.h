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

#define MAX_RTU  		2
#define MAX_PORT        32
#define MAX_POINT       64

/* ------------------------ */
/*  HARRIS BIT Constant     */
/* ------------------------ */
#define OFFLINE     1
#define OL          0x01        // Offline
#define PF          0x02        // Power Fail Reset
#define MF          0x04        // Message Fail
#define SOE_S       0x08        // SOE set
#define SOEOF       0x10        // SOE Overflow
#define COS_S       0x20        // COS set
#define COSOF       0x40        // COS Overflow
#define CNTFALL     0x80        // Control Fail


/* ------------------------------------------------ */
/* HARRIS - PORT Type defination                    */
/* ------------------------------------------------ */
//#define NULL        0       /* Null Port        */
#define CAI         1       /* C&I Port         */
#define ANA         2       /* Analog Port      */
#define ACC         3       /* Accumulator Port */

/* Control Constant Defination */
#define ARM             0x6
#define OPR             0x7
#define ARM_RUN         0x8
#define OPR_RUN         0x9


/* ------------------------ */
/*  HARRIS OPCODE           */
/* ------------------------ */

/* ---------------------------------------------------- */
/* STURECTURE 정의 : COS_Q                              */
/* 1. HARRIS 용 COS 이벤트 저장용 Buffer 관련 스트럭쳐  */
/* ---------------------------------------------------- */   
typedef struct
        {
        byte    front;
        byte    rear;
        byte    report;
        byte    dummy;
        
        byte    que[256+2][2];
        } __attribute__ ((packed)) COS_Q ;


/* ---------------------------------------------------- */
/* STURECTURE 정의 : SOE_Q                              */
/* 1. HARRIS 용 SOE 이벤트 저장용 Buffer 관련 스트럭쳐  */
/* ---------------------------------------------------- */    
typedef struct
        {
        byte    front;
        byte    rear;
        byte    report;
        byte    dummy;
        
        byte    que[256+2][8];
        byte    lastSOE[8];
        } __attribute__ ((packed)) SOE_Q ;

/* ---------------------------------------------------- */
/* STURECTURE 정의 : PORT_DB                            */
/* 1. HARRIS 용 포트 내부 데이터 관련 스트럭쳐          */
/* ---------------------------------------------------- */   
typedef struct
        {
        short   type;                       /* C&I, IND, ANA, ACC, CTL */
        short   portStatus;
        short   rtuid;
        short   harrisPort;
        short   pointData[MAX_POINT];       /* current scaned data */

        } __attribute__ ((packed)) PORT_DB;

/* ---------------------------------------------------- */
/* STURECTURE 정의 : RTU                                */
/* 1. HARRIS 용 원격소장치별 내부 데이터 관련 스트럭쳐  */
/* ---------------------------------------------------- */   
typedef struct
        {
        short   id;
        short   type;
        short   status;
        short   max_port;
        short   dataPortCnt;
        short   ciPortCnt;
        short   rts_delay;
        short   tx_delay;
        short   preOpcode;
        short   reportcnt;

        byte    rxbuf[32];
        byte    reportbuf[1024];
        byte    cmdSize[34];

        COS_Q   cosq;
        SOE_Q   soeq;
        
        } __attribute__ ((packed)) RTU;

/*----------------------------------------- */
