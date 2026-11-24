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
/*  RTU TYPE 에 따른 모듈 Access Address            */
/* ------------------------------------------------ */
/* for PDS-RTU */
#define DI_OFFSET       0
#define DO_OFFSET       4
//#define DI_OFFSET       0
//#define DO_OFFSET       0


//#define POINT_OFFSET    1
//#define OPR_OFFSET      2
//#define CLEAR_OFFSET    3

#define POINT_OFFSET    1
#define OPR_OFFSET      2
#define CLEAR_OFFSET    3

// DO 모듈관련 Control Index
#define TRIP_OPR		0x01
#define CLOSE_OPR		0x02
#define SOURCE_OPR		0x80


/*
*   포인트 구성정보
*/    
typedef struct
    {
        word    updateFlag;         // Config DB update Flag
        word    modPoint;           // 모듈 포인트 수
        word    pointInfo[32][4];   // 포인트 구성정보 : 4 word
        
        word    chksum;             // chksum 영역
    } __attribute__ ((packed)) VME_CONFIG;    

typedef struct
    {
        word    updateTick;         // 신규 Update Tick
        word    modPoint;           // 모듈 포인트 수
        word    pointData[32];      // 포인트 데이터 : 4 word

    } __attribute__ ((packed)) VME_DATA;        
    
typedef struct
    {
        word    controlFlag;        // 제어정보 유효표시
        
        word    cntrPoint;          // 제어 포인트 번호 [1.. 16]
        word    cntrState;          // 제어상태
        word    relayTime;          // 릴레이 동작시간
        word    reserved;
        
        word    chksum;             // chksum 영역

    } __attribute__ ((packed)) VME_CONTROL;        

/*
*/
typedef struct
    {
        word    pointNo;
        word    pointState;
        word    soeHour;
        word    soeMin;
        word    soeSec;
        word    soeMilisec;
    } __attribute__ ((packed)) SOE_INFO;        

typedef struct
    {
        word    soeFront;           // soe Front
        
        SOE_INFO    soeInfo[64];
        
        word    chksum;
    } __attribute__ ((packed)) VME_SOEQ;        

/*
*   VME BUS 상의 모듈부 데이터 영역
*/
typedef struct
    {
        /* 모듈부 Operation 영역 */
        word    vmeStx1;            /* STX #1 */
        word    vmeStx2;            /* STX #2 */
        
        word    modId;              // VME 모듈 ID
        word    modType;            // VME 모듈 타입
        word    cpuAccess;          // CPU Access Flag
        word    modAccess;          // Module Access Flag
        word    pfrFlag;            // Module Power Fail Flag
        word    dbDownFlag;         // Module DB Down Flag
        word    cfgError;           // 모듈 : DB Config Error
        word    soeFlag;            // 모듈 : SOE Queue update Flag
        word    resetFlag;          // 모듈 : 재기동 Flag
        word    initState;          // 모듈 : 초기화 Flag
        word    cpuActCount;        // CPU 활성화 카운트
        word    modActCount;        // 모듈부 활성화 카운트
        
        word    timeSyncFlag;       // VME 시각동기 Flag
        word    year;           
        word    month;
        word    day;
        word    hour;
        word    min;
        word    sec;

        VME_CONFIG      modConfig;          // 모듈부 구성정보 영역 
        VME_DATA        modData;            // 모듈부 데이터 영역
        VME_CONTROL     modControl;         // 모듈부 제어 영역
        VME_SOEQ        modSoeQ;            // 모듈부 SOEQ 영역      
        
    } __attribute__ ((packed)) VME_MODULE;       
    
