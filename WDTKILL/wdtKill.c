/*
 * ==============================================================
 * System TARGET : ACE Control VME-CU Ver 2.0   
 * Target CPU    : MPC8248, VME6U
 * --------------------------------------------------------------
 * System DESIGN : SANE-SYSTEM   .... by  Lee Ho-Sang
 * Initial-DATA  : 2012,12,10
 * Last Updated  : 2013,01,08
 * ==============================================================
 */


#include	"localLib.h"
#include    "external.h"
//#include    "eventmsg.h"

int	            termExec;

SHM_DESC	    shmDesc = {-1, SHM_KEY, sizeof(SHM_MEMORY), NULL};

SHM_MEMORY	    *shmPtr = NULL;

TASK_INFO	    *taskPtr= NULL;

RTC             *rtc    = NULL;
OPR_MSG         *opr    = NULL;
LINK_MSG        *linkCfg= NULL;
SCU_MSG         *scuCfg = NULL;

CONSOLE_INFO	*console= NULL;

RTU_DATABASE    *rtudb  = NULL;
MMI_DATABASE    *mmidb  = NULL;
SYSTEM_QUE      *sque   = NULL;         // MMI 용 이벤트
HISTORY_QUE     *hque   = NULL;         // CONSOLE 용 이벤트

SYSTEM_QUE      localSQ;
volatile SYSTEM_QUE      *framSque;

HISTORY_QUE      localHQ;
volatile HISTORY_QUE      *framHque;


SCAN_CONFIG     *scanCfg[MAX_SCAN_PORT];            // 하위계전기 SCAN Config
DNP_DEVICE      *dnpcfg[MAX_MODULE];

POINT_BUF       *devPtBuf[MAX_DEV_POINT];           // 시스템 상태포인트 : 계전기 통신, 절체, 등 
	
HOST_DCB        *hostDCB[MAX_HOST];                 /* HARRIS, LANDIS, SICS HOST Structure */

MPC860IO_DESC    ledPort = {0, NULL, 0, "/dev/ledswitch", O_RDWR};
MPC860IO_DESC    rtcPort = {0, NULL, 0, "/dev/rtc0", O_RDWR};

FILE_DESC	logFile = {NULL, FIO_APPEND, "/root/romdb/log.txt" };
char    	filename[FILENAME_LEN];
//static  int logDayInfo;


int     devHero;
int     devExio;


/* ---------------------------------------- */
/*  VMEBUS 관련 변수 초기화                 */
/* ---------------------------------------- */
void    *vmebus_ptr ;
int     vme_fd; 

VME_SIODCB      *vmeSioDCB;
VME_CHAN_DCB    *vmeChanDCB[SIO_CHANNEL_MAX];


VME_SIODCB      tempVmeSioDCB;
VME_CHAN_DCB    tempVmeChanDCB[SIO_CHANNEL_MAX];

void    *sram_ptr;
void    *mram_ptr;
int     sram_fd;                // SVME-860 Device Driver 

void    *nvram_ptr;
int     nvram_fd;                // SVME-860 Device Driver 

int     wdtid;                      

//
// 모듈:	SigHandler()
//
void SigHandler(int sig)
{
    //if(opr->wdtDebug)
    Debug(console,"wdt> %4d/%2d/%2d-%02d:%02d:%02d ... signal generated (%2d)...!\n", rtc->year, rtc->month,rtc->day,rtc->hour, rtc->min, rtc->sec, sig);
    pause(100);
    
	switch(sig)
	{
    case SIGTERM:
        termExec = 0;
        //if(opr->wdtDebug)
        Debug(console,"wdt> ... signal [SIGTERM] generated (%2d)...!\n", sig);
        pause(100);     
        break;
	        
	case SIGBUS : 
	    termExec = 0;
	    //if(opr->wdtDebug)
        Debug(console,"wdt> ... signal [SIGBUS] generated (%2d)...!\n", sig);
        pause(100);     	    
	    break;
		    
	case SIGSEGV: 
	    termExec = 0;
	    //if(opr->wdtDebug)
        Debug(console,"wdt> ... signal [SIGSEGV] generated (%2d)...!\n", sig);
        pause(100);     	    
	    break;
		    
	case SIGPIPE: 
	    //if(opr->wdtDebug)
        Debug(console,"wdt> ... signal [SIGPIPE] generated (%2d)...!\n", sig);
        pause(100);     	    
		break;

	default : 
	    //if(opr->wdtDebug)
	    Debug(console,"wdt> ... signal[%2d] generated ...!\n", sig);
		break;
	}
	
}


//
// 모듈:	InitEnvironment()
//
int InitEnv( void)
{
	// SIGNAL 처리 루틴 설정
	signal( SIGBUS,  SigHandler);
	signal( SIGSEGV, SigHandler);
	signal( SIGTERM, SigHandler);

	/* -------------------------------------------- */
	/* 디바이스 관련 초기화 LED - IO Device Open ... */
	/* -------------------------------------------- */
    if((ledPort.id = open( ledPort.name, ledPort.attr)) <0)  
	{
        printf("wdt> *Device Open Error ... LED \n");
        exit(1);
    }
    
   	if((rtcPort.id = open( rtcPort.name, rtcPort.attr)) < 0)
    {
        printf("wdt> *Device Open Error ... RTC \n");
        exit(1);
    }

    /* ------------------------------------ */
	/*  공유 메모리를 생성한다              */
	/* ------------------------------------ */
	if (ShmCreate(&shmDesc) < 0)
	{
		printf("wdt> *공유 메모리 Open 이상 \n");
		return(0);
	}

    /* ------------------------------------ */
	/*  공유 메모리 상태 확인               */
	/* ------------------------------------ */
	if (ShmCheck(&shmDesc) < 0)
	{
		printf("wdt> *ERR_공유 메모리 상태 이상\n");
		return(0);
	}
	
	//printf("wdt> SHM %p \n", shmDesc.address);
	
	/* ------------------------------------ */
	/* 공유 메모리상의 포인터 변수 초기화   */      
	/* ------------------------------------ */
	shmPtr = (SHM_MEMORY *) shmDesc.address;
	taskPtr = (TASK_INFO *) &shmPtr->taskInfo[WDT_PROCESS];
	
	rtc     = (RTC *)       &shmPtr->rtc;
	opr     = (OPR_MSG *)   &shmPtr->opr_msg;

    linkCfg = (LINK_MSG *)  &shmPtr->link_msg;
    scuCfg 	= (SCU_MSG *)   &shmPtr->scu_msg;
	
	rtudb   = (RTU_DATABASE *) &shmPtr->rtuDatabase;
	mmidb   = (MMI_DATABASE *) &shmPtr->mmiDatabase;
	
	sque    = (SYSTEM_QUE *)  &shmPtr->localEvent; 
	hque    = (HISTORY_QUE *)  &shmPtr->localHistoryQ; 

    console = (CONSOLE_INFO *) &shmPtr->console; 
    
	return(1);
}


    
/* ================================================================ */
/*  ACE-VMECU-NET-V7 - HOST PROCESS 방식 MAIN 프로그램              */
/*      - VMESIO Active Check...                                    */
/*      - HOST 통신상태 Update & WDT Check...                       */
/* ================================================================ */
int	main(int argc, char **argv)
{
    /* ------------------------------------------------ */
	/*  작업 환경을 초기화한다                          */
	/* ------------------------------------------------ */
	if ((termExec = InitEnv()) == 1)
	{
	}

    /* ------------------------------------------------ */
    /*  하드웨어 WDT 기능을 설정함...                   */
    /* ------------------------------------------------ */
    opr->wdtEnable = 1;

	if ((wdtid=open("/dev/wdt",O_RDWR)) < 0)  
	{
		printf("wdt> ***/dev/wdt can't open file\n");
		opr->wdtEnable = 0;
	}
    else
    {
	    if(ioctl(wdtid,CRUTCHES_WDT_CTR_ON ,  NULL) < 0 )
		printf("wdt> ioctl fail\n");
    }
    
    printf("\n============================================================\n");
    printf(" ==> WDT KILL ... \n");
    printf("============================================================\n");
        
    ioctl(wdtid, CRUTCHES_WDT_CTR_CLR, NULL);   /* WDT Clear.... */
        
    
	while(termExec)
	{
	    pause(100);
	    
	}

    return (0);	
}
