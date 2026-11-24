//==============================================================================
//
// 파일:    cliMain.c
//
//==============================================================================
#include    "localLib.h"
#include	"external.h"

#include    "cli.h"

// hkkim
#include <readline/readline.h>
#include <readline/history.h>

extern void initialize_readline (void);

extern void         InputProcessing(void);






int	        termExec;

SHM_DESC	shmDesc = {-1, SHM_KEY, sizeof(SHM_MEMORY), NULL};
SHM_MEMORY	*shmPtr = NULL;
TASK_INFO	*taskPtr = NULL;
//CNF_PRC		*shmPrc = NULL;

RTC             *rtc    = NULL;
OPR_MSG         *opr    = NULL;

LINK_MSG		*linkCfg= NULL;
SCU_MSG         *scuCfg = NULL;

RTU_DATABASE    *rtudb  = NULL;
MMI_DATABASE    *mmidb  = NULL;

SYSTEM_QUE      *sque   = NULL;         // MMI 용 이벤트
HISTORY_QUE     *hque   = NULL;         // CONSOLE 용 이벤트

SCAN_CONFIG     *scanCfg[MAX_SCAN_PORT];         // 하위계전기 SCAN Config
DNP_DEVICE      *dnpcfg[MAX_MODULE];

POINT_BUF       *devPtBuf[MAX_DEV_POINT];
HOST_DCB        *hostDCB[MAX_HOST];             /* HARRIS, LANDIS, SICS HOST Structure */

LANDIS_CFG      *landis;                            /* LANDIS HOST Structure */
HOST_STSBUF     *hostStsBuf[MAX_HOST_STS_PT];       /* LANDIS-HOST defined Status Point */
HOST_ANABUF     *hostAnaBuf[MAX_HOST_ANA_PT];       /* LANDIS-HOST defined Analog Point */

TTY_DESC consolePort = {0, 0, NULL, ASYNC_860T, COM_RS232, 57600,  8,  1, 0, 'N',  FID_CONSOLE_PORT,    {0, 50000}};    // TTYS0 

CONSOLE_INFO	*console= NULL;

int     passTimeCLI;

/* THREAD 관련 변수 정의 */
void            *debug_thread(int  *arg);
THREAD_ENTRY    debugTHREAD;

//
// 모듈:	SigHandler()
//
void SigHandler(int sig)
{
    if(opr->wdtDebug)	printf("cli> ... signal generated (%2d)...!\n", sig);
    pause(100);
    
	switch(sig)
	{
    case SIGTERM:
        termExec = 0;
        if(opr->wdtDebug)   printf("cli> ... signal [SIGTERM] generated (%2d)...!\n", sig);
        pause(1000);     
        break;
	        
	case SIGBUS : 
	    termExec = 0;
        if(opr->wdtDebug)   printf("cli> ... signal [SIGBUS] generated (%2d)...!\n", sig);
        pause(1000);     	    
	    break;
		    
	case SIGSEGV: 
	    termExec = 0;
        if(opr->wdtDebug)   printf("cli> ... signal [SIGSEGV] generated (%2d)...!\n", sig);
        pause(1000);     	    
	    break;
		    
	case SIGPIPE: 
        if(opr->wdtDebug)   printf("cli> ... signal [SIGPIPE] generated (%2d)...!\n", sig);
        pause(1000);     	    
		break;

	default : 
	    if(opr->wdtDebug)	printf("cli> ... signal[%2d] generated ...!\n", sig);
		break;
	}
}

int check_process(char *process)
{
	    char buffer[128];
	    int rc ;
    FILE *fp = popen("pidof your_process_name", "r");
    if (fp == NULL) {
        perror("popen failed");
        return 1;
    }

    if (fgets(buffer, sizeof(buffer), fp) != NULL) {
        printf("PID(s): %s", buffer);
		    rc =  1; 
    } else {
        printf("Process not running.\n");
        rc =  0;
    }

    pclose(fp);
    return rc;
	
}

//
// 모듈:	ClearEnvironment()
//
void ClearEnv(void)
{
  int rc ;
  int i ; 

	printf("CLI : Cleaning...\r\n");

	// 공유 메모리 설정 해제
	if (shmPtr != NULL)
 		ShmDetach(&shmDesc);
	
  	
#if 0	
	sleep(3);
	
	for( i=0 ; i < 10; i++)
	{
		 rc =check_process("WDT");
		 if (rc == 0  )  break;
		 	sleep(1);
	}
	
	
	// 공유 메모리 설정 해제
	if (shmPtr != NULL)
	// 공유 메모리를 제거한다
	ShmDelete(&shmDesc);

 //		ShmDetach(&shmDesc);


#endif 	

    //PortClose(&consolePort);
	printf("[*] Close Device ....4!\n");
}
					    

//
// 모듈:	InitEnvironment()
//
int InitEnv( void)
{
    int     i;
    
	// SIGNAL 처리 루틴 설정
	signal( SIGBUS,  SigHandler);
	signal( SIGSEGV, SigHandler);
	signal( SIGTERM, SigHandler);

	// 공유 메모리 상태 확인
	if (ShmCheck((SHM_DESC *) &shmDesc) < 0)
	{
		printf("cli> *ERR_공유 메모리 상태 이상\n");
		return(0);
	}
	
	// 포인터 변수 초기화
	shmPtr = (SHM_MEMORY *) shmDesc.address;
	
	taskPtr = &shmPtr->taskInfo[CLI_PROCESS];

	rtc     = (RTC *)       &shmPtr->rtc;
	opr     = (OPR_MSG *)   &shmPtr->opr_msg;
	
	linkCfg = (LINK_MSG *)  &shmPtr->link_msg;
	scuCfg 	= (SCU_MSG *)   &shmPtr->scu_msg;
	
	rtudb    = (RTU_DATABASE *) &shmPtr->rtuDatabase;
	mmidb   = (MMI_DATABASE *) &shmPtr->mmiDatabase;
	
	sque    = (SYSTEM_QUE *)  &shmPtr->localEvent;
	hque    = (HISTORY_QUE *) &shmPtr->localHistoryQ;

    /* 운영자 CONSOLE 초기화 */
    console = (CONSOLE_INFO *) &shmPtr->console; 
    
    /* ------------------------------------ */
    /* DNP_DEVICE : 전자식 배전반 구조체    */
    /* ------------------------------------ */
    for(i=0; i< MAX_SCAN_PORT; i++)     scanCfg[i]  = (SCAN_CONFIG *) &shmPtr->scanCFG[i];
    for(i=0; i< MAX_MODULE; i++)        dnpcfg[i]   = (DNP_DEVICE *) &shmPtr->dnpCFG[i];
	for(i=0; i< MAX_DEV_POINT; i++)     devPtBuf[i] = (POINT_BUF *) &shmPtr->devPtBuf[i];

    /* ------------------------------------ */
    /* HOST_DCB : 상위 HOST 구조체          */
    /* ------------------------------------ */
    for(i=0; i< MAX_HOST; i++)          hostDCB[i]  = (HOST_DCB *) &shmPtr->hostDCB[i];
    
    landis = (LANDIS_CFG *) &shmPtr->landis;                    /* LANDIS HOST Structure */
    for(i=0; i< MAX_HOST_STS_PT; i++)   hostStsBuf[i] = (HOST_STSBUF *) &shmPtr->hostStsBuf[i];
    for(i=0; i< MAX_HOST_ANA_PT; i++)   hostAnaBuf[i] = (HOST_ANABUF *) &shmPtr->hostAnaBuf[i];
	
	return(1);
}


#ifndef BUILD_TIME
#define BUILD_TIME "unknown"
#endif



//
// 모듈:	DisplayLogo()
//
void DisplayLogo(void)
{
	//ExecCommand("clear");
	system("clear; cat /mnt/cli.txt") ;
	printf("\r\n");
	printf("\t\t//        [*] Update Date ...%s                   //\n", BUILD_TIME);
		

/*

	printf("\t\t////////////////////////////////////////////////////////////\n");
	printf("\t\t//                                                        //\n");
	printf("\t\t//        %s : CLI - USER Debugger     //\n", TARGET_NAME);
	printf("\t\t//        [*] Designed by ACE-CONTROL System               //\n");
	printf("\t\t//        [*] Version   %s                       //\n", VERSION_STRING);
	printf("\t\t//        [*] %s         //\n", SIMULATOR_NAME);  
	printf("\t\t//        [*] Update Date ...%s                   //\n", DATE_STRING);
	printf("\t\t//                                                        //\n");
	printf("\t\t////////////////////////////////////////////////////////////\n\n");
*/	
 }

/*
*   NETWORK Debug MAIN() 
*/
void debug_thread_call()
{
    int     i;     
    //int		plcid; 
    int	    state;
    int     index;

    i = 0;
    
    debugTHREAD.tid        = i;
    sprintf(debugTHREAD.name, "%s%d","DEBUG", i);
    debugTHREAD.priority   = 0;
    debugTHREAD.options    = 0;
    
    debugTHREAD.entryPt    = debug_thread;  // MODBUS-THREAD
    debugTHREAD.arg        = 0;
    
  	index = i;      // Thread Index #1
    state = pthread_create( &debugTHREAD.tid, NULL, debugTHREAD.entryPt, &i);
   	if(state != 0)
    {
	    printf("CLI> *Thread# [%s] Create Error ...!\n", debugTHREAD.name);
	    termExec = RESET;
        //exit(0);
    }
	
	pause(100);
}

 
	
/* ------------------------------------------------------- */
/*  신호제어기 MAIN 프로그램 Start Routine ....             */
/* ------------------------------------------------------- */
int	main(int argc, char **argv)
{
    //int     wdtid, wdtEnable;
	byte	runtick;
	int		oldmin;
	//word    chksum;
	
    //PortInitial((TTY_DESC *) &consolePort);


	
	read_config(1);



	// 작업 환경을 초기화한다
	if ((termExec = InitEnv()) == 1)
	{
		DisplayLogo();
		display_logo();		
	}
		
	runtick = 0;

#ifdef    SDP_VMECU
        printf("SYSTEM : SDP_VMECU SET\r\n");
#endif 
    printf("...CLI start...!\n");



#ifdef  NETWORK_CONSOLE

	debug_thread_call();

#endif
	
	// 어 debug_thread 에서도 하는데..
	console->front = 0;
	console->rear  = 0;
	console->active = SET;
	
    passTimeCLI = 0;
    oldmin = rtc->min;
   
// hkkim
	initialize_readline() ;

 
	while(termExec)
	{
		InputProcessing();
		usleep(100000);		/* 100 msec delay */
		
	}

    console->front = 0;
	console->rear  = 0;
	console->active = RESET; // console->buffer에 기록 안함..처기값은 뭐지..
	
	// 작업 환경을 정리한다
	ClearEnv();
	
	return (0);
}
