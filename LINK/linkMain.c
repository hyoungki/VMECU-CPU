/* 
================================================================== 

*/

#include	"localLib.h"
#include    "external.h"

extern  void    linkMaster();
extern  void    linkSlave();



TTY_DESC linkPort 	 = {FID_LINK, 0, NULL, ASYNC_860T, COM_RS232, 115200,   8,  1,  0,'N',    FID_LINK_PORT,   {0, 50000}};
//TTY_DESC linkPort 	 = {FID_LINK, 0, NULL, ASYNC_860T, COM_RS232, 57600,   8,  1,  0,'N',    FID_LINK_PORT,   {0, 50000}};

SHM_DESC	    shmDesc = {-1, SHM_KEY, sizeof(SHM_MEMORY), NULL};
SHM_MEMORY	    *shmPtr = NULL;
int             termExec;

TASK_INFO	*taskPtr= NULL;

RTC             *rtc    = NULL;
OPR_MSG         *opr    = NULL;
LINK_MSG		*linkCfg= NULL;
SCU_MSG         *scuCfg = NULL;

CONSOLE_INFO	*console= NULL;

RTU_DATABASE    *rtudb  = NULL;
SYSTEM_QUE      *sque   = NULL;         // MMI 용 이벤트
HISTORY_QUE     *hque   = NULL;         // CONSOLE 용 이벤트

HOST_DCB        *hostDCB[MAX_HOST];             /* HARRIS, LANDIS, SICS HOST Structure */
SCAN_CONFIG     *scanCfg[MAX_SCAN_PORT];         // 하위계전기 SCAN Config
DNP_DEVICE      *dnpcfg[MAX_MODULE];

LANDIS_CFG      *landis;                            /* LANDIS HOST Structure */
HOST_STSBUF     *hostStsBuf[MAX_HOST_STS_PT];       /* LANDIS-HOST defined Status Point */
HOST_ANABUF     *hostAnaBuf[MAX_HOST_ANA_PT];       /* LANDIS-HOST defined Analog Point */

POINT_BUF       *devPtBuf[MAX_DEV_POINT];           // 시스템 상태포인트 : 계전기 통신, 절체, 등 

PDPDTIME_DATE_TIME      dnpRTC;
PDPDTIME_MS_SINCE_70    dnpRTCInfo;

byte    bitcode[8] = {0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80};

int     devHero;
int     devExio;

byte    linkStatus = 0, cosSts = 0;
int     initCheck  = 0;
int     linkInitial;

/* *************************************************************************************
*	FUNCTION : sigHandler()
* **************************************************************************************/
void	SigHandler( int sig )
{
    char    buffer[128];
    
    if(opr->wdtDebug)	
    printf("link> ... signal generated (%2d)...!\n", sig);
    pause(100);
    
	switch(sig)
	{
    case SIGTERM:
        termExec = 0;
        //if(opr->wdtDebug)  
        	 printf("link> ... signal [SIGTERM] generated (%2d)...!\n", sig);
        
        /* ------------------------------------------------ */
        /* LOG File 저장                                    */
        /* ------------------------------------------------ */
        sprintf(buffer, "link> ... signal [SIGTERM] generated (%2d)...!\n", sig);
        LogFile_MPU ( opr, NOT_DEFINED, buffer, strlen(buffer));
          
        // hkkim    
        //pause(1000);     
        break;
	        
	case SIGBUS : 
	    termExec = 0;
        if(opr->wdtDebug)   printf("link> ... signal [SIGBUS] generated (%2d)...!\n", sig);
        
        /* ------------------------------------------------ */
        /* LOG File 저장                                    */
        /* ------------------------------------------------ */
        sprintf(buffer, "link> ... signal [SIGBUS] generated (%2d)...!\n", sig);
        LogFile_MPU ( opr, NOT_DEFINED, buffer, strlen(buffer));
            
        pause(1000);     	    
	    break;
		    
	case SIGSEGV: 
	    termExec = 0;
        if(opr->wdtDebug)   printf("link> ... signal [SIGSEGV] generated (%2d)...!\n", sig);
        
        /* ------------------------------------------------ */
        /* LOG File 저장                                    */
        /* ------------------------------------------------ */
        sprintf(buffer, "link> ... signal [SIGSEGV] generated (%2d)...!\n", sig);
        LogFile_MPU ( opr, NOT_DEFINED, buffer, strlen(buffer));
            
        pause(1000);     	    
	    break;
		    
	case SIGPIPE: 
        if(opr->wdtDebug)   printf("link> ... signal [SIGPIPE] generated (%2d)...!\n", sig);
        
        /* ------------------------------------------------ */
        /* LOG File 저장                                    */
        /* ------------------------------------------------ */
        sprintf(buffer, "link> ... signal [SIGPIPE] generated (%2d)...!\n", sig);
        LogFile_MPU ( opr, NOT_DEFINED, buffer, strlen(buffer));
            
        pause(1000);     	    
		break;

	default : 
	    if(opr->wdtDebug)	printf("link> ... signal[%2d] generated ...!\n", sig);
	    
	    /* ------------------------------------------------ */
        /* LOG File 저장                                    */
        /* ------------------------------------------------ */
        sprintf(buffer, "link> ... signal[%2d] generated ...!\n", sig);
        LogFile_MPU ( opr, NOT_DEFINED, buffer, strlen(buffer));
            
		break;
	}
    
}


//
// 모듈:	ClearEnvironment()
//
void ClearEnv(void)
{
    //printf("link exit...!\n");
    PortClose(&linkPort);


    
  // hkkim  프로세서 정보정리하고 공유메모리 detach ..  
    
	// 프로세스 정보를 정리한다
	UpdateProcessInfo(taskPtr, 0, getpid(), 0);
	
	// hkkim
	// 공유 메모리 설정 해제
	if (shmPtr != NULL)
		ShmDetach(&shmDesc);
		
   
   
   
	
			
}
					    

//
// 모듈:	InitEnvironment()
//
int InitEnv( void)
{
    int     i;
    
	// 공유 메모리 상태 확인
	if (ShmCheck(&shmDesc) < 0)
	{
		printf("link> *ERR_공유 메모리 상태 이상\n");
		return(0);
	}
	
	//printf("link> SHM %p \n", shmDesc.address);
	
	// 포인터 변수 초기화
	shmPtr = (SHM_MEMORY *) shmDesc.address;
	taskPtr = (TASK_INFO *) &shmPtr->taskInfo[LINK_PROCESS];

	rtc     = (RTC *)       &shmPtr->rtc;  
	opr     = (OPR_MSG *)   &shmPtr->opr_msg;
	linkCfg = (LINK_MSG *)  &shmPtr->link_msg;
	scuCfg 	= (SCU_MSG *)   &shmPtr->scu_msg;
	
	rtudb   = (RTU_DATABASE *) &shmPtr->rtuDatabase;
	sque    = (SYSTEM_QUE *)  &shmPtr->localEvent;
    hque    = (HISTORY_QUE *)  &shmPtr->localHistoryQ;
    
    console = (CONSOLE_INFO *) &shmPtr->console; 
    
    /* ------------------------------------ */
    /* HOST_DCB : 상위 HOST 구조체          */
    /* ------------------------------------ */
    for(i=0; i< MAX_HOST; i++)   
	{
	    hostDCB[i] = (HOST_DCB *) &shmPtr->hostDCB[i];
	}
		
		
    /* ------------------------------------ */
    /* DNP_DEVICE : 전자식 배전반 구조체    */
    /* ------------------------------------ */
    for(i=0; i< MAX_SCAN_PORT; i++)   
	{
	    scanCfg[i] = (SCAN_CONFIG *) &shmPtr->scanCFG[i];
	}
	    
    for(i=0; i< MAX_MODULE; i++)   
	{
	    dnpcfg[i] = (DNP_DEVICE *) &shmPtr->dnpCFG[i];
	}

    for(i=0; i< MAX_DEV_POINT; i++)   
	{
	    devPtBuf[i] = (POINT_BUF *) &shmPtr->devPtBuf[i];
	}	
	
	landis = (LANDIS_CFG *) &shmPtr->landis;                    /* LANDIS HOST Structure */
    for(i=0; i< MAX_HOST_STS_PT; i++)   
	{
	    hostStsBuf[i] = (HOST_STSBUF *) &shmPtr->hostStsBuf[i];
	}

    for(i=0; i< MAX_HOST_ANA_PT; i++)   
	{
	    hostAnaBuf[i] = (HOST_ANABUF *) &shmPtr->hostAnaBuf[i];
	}	
			
    /* ------------------------------------------------------------ */
	/* 프로세스 정보를 초기화한다                                   */
	/* HOST Task 는 TCPIP 전송대기를 고려하여 WDT 기능을 삭제함...  */
	/* ------------------------------------------------------------ */
	taskPtr->initial    = 1;
	taskPtr->wdtEnable  = 1;            
	
	UpdateProcessInfo(taskPtr, 1, getpid(), 0);
	
	return(1);
}


/*
*/
int rcvWithLINK(TTY_DESC *tp, byte *rxbuf)
{
    int     rcvCount;
    int     rcvTimeout;
    int     reqSize, rcvSize;
    byte    lrc;

    rcvTimeout = 0;

    /* receive 1'st STX */
    do {
        if(asyncRead( opr, tp, &rxbuf[0], 1) == 0) return(0);
        if(rcvTimeout++ > 50) return (0);
    } while(rxbuf[0] != 0x7e);
    
    /* receive size */
    if(asyncRead( opr, tp, &rxbuf[1], 3) == 0)
    {
        //if(opr->linkDebug)    
        //Debug(console,"link> Size not received ...!\n");
        return(0);
    }
    
	if(rxbuf[1] != 0x7E)	return (-1);
		
    reqSize = (rxbuf[2]*256 + rxbuf[3]) - 4;   /* except stx, size */ 

    /* receive body */
    if((rcvSize = asyncRead( opr, tp, &rxbuf[4], reqSize)) == reqSize)
    {
        rcvCount = rcvSize + 4;
        lrc = genlrc(rxbuf, rcvCount);
        
        if(lrc == 0)   return (rcvCount);
        else    
        {
            if(opr->linkDebug)    
            {
                Debug(console,"link> *LRC Error ...cal=%2x, rcv=%2x!\n", lrc, rxbuf[rcvCount-1]);
                DumpBuff(console,"*LRC: ", rxbuf, rcvCount);
            }                
            return(0);
        }
        
    }
    else    
    {
        if(opr->linkDebug)    
            Debug(console,"link> *Tail not received ...req=%2d, rcv=%2d!\n", reqSize, rcvSize);
        return(0);
    }
    
}


/*
========================================================
*   CPU : Active/활성화
*    - 상위/하위 통신 Master 기능, Shutdown Disable
========================================================
*/
void setMaster()
{
    int     i;
    char    led;
    char    buffer[128];
    //byte    front, rear;
    
    DNP_DEVICE      *dnp;
    //HOST_DCB        *host; 
    //DNP_SOE_QUEUE   *dnpSOEQ;   
    //DNP_COS_QUEUE   *dnpCOSQ;   

//	printf("Fun-in setMaster\r\n");

#ifdef  ACE_LBS_4U
    /* -------------------------------------------- */
	/*	CPU Shutdown : Disable .... MASTER 기능     */
	/* -------------------------------------------- */
    led = 0x81;                     
    write( devHero, &led, 1);   
#else    
    /* -------------------------------------------- */
	/*	CPU Shutdown : Disable .... MASTER 기능     */
	/* -------------------------------------------- */
    led = 0x0f;                     
    write( devHero, &led, 1);    
#endif

    /* -------------------------------------------- */
	/*	Master 전환시 : MMI 이벤트 동기화 ...       */
	/* -------------------------------------------- */
    if(opr->runMode == LOCAL_SLAVE)
    {
        opr->mmiTimeSync = SET;
        opr->mmiRear = (opr->mmiFront - 1) & 0xff;
        if(opr->linkDebug)   Debug(console,"vmecu> MMI-Event Queue Sync...!\n");
    }
    
    
    opr->runMode = LOCAL_MASTER;
    opr->stscode |= LED_ACTIVE_BIT;     // ACT-LED ON
    
    
	/* -------------------------------------------- */
	/*	ACTIVE-CPU 관련 이벤트 생성 ...				*/
	/* -------------------------------------------- */
	if(opr->runMode != LOCAL_MASTER)
	{
		if(opr->cpuMode == CPU_MASTER)	
		{
		    logEvent1( opr, sque, EVENT_MASTER_ACTIVE, 0, 0, 0, opr->cpuMode);
		    logHistoryQue1( opr, hque, EVENT_MASTER_ACTIVE, 0, 0, 0, opr->cpuMode);
		    
		    /* LOG File 저장 */
            sprintf(buffer, "%s", ">> SET-MASTER ... MASTER ");
            LogFile_MPU (opr, EVENT_MASTER_ACTIVE, buffer, strlen(buffer));
		}
		else
		{
		    logEvent1( opr, sque, EVENT_SLAVE_ACTIVE, 0, 0, 0, opr->cpuMode);	
		    logHistoryQue1( opr, hque, EVENT_SLAVE_ACTIVE, 0, 0, 0, opr->cpuMode);	
		    
		    /* LOG File 저장 */
            sprintf(buffer, "%s", ">> SET-MASTER ... SLAVE ");
            LogFile_MPU (opr, EVENT_SLAVE_ACTIVE, buffer, strlen(buffer));
		}
	}
		
	
    
#ifdef  ACE_LBS_4U
    /* -------------------------------------------- */
    /* CPU Live 상태표출 ... (Master/Slave 여부 )   */
    /* -------------------------------------------- */
    linkCfg->cpuControl |= DATA_CPU_ACTIVE;  // 상대편에서 1로 읽는다. POR 시 0 이고.
	if(write(devHero, &linkCfg->cpuControl, 1) != 1) 
	{            
	    Debug(console,"2) %s write error : %s \n", DEVICE_HERO_IO, strerror(errno));  
	}       
	
#else    
    /* -------------------------------------------- */
    /* CPU Live 상태표출 ... (Master/Slave 여부 )   */
    /* -------------------------------------------- */
    linkCfg->cpuControl |= DATA_CPU_ACTIVE;
	if(write(devExio, &linkCfg->cpuControl, 1) != 1) 
	{            
	    Debug(console,"2) %s write error : %s \n", DEVICE_EX_IO, strerror(errno));  
	}             
#endif
	
	//if(opr->linkDebug) 
	{
	    Debug(console,"\n==========================\n");
	    Debug(console," [%02d/%02d-%02d:%02d:%02d]... *** SET MASTER ...!\n", rtc->month, rtc->day, rtc->hour, rtc->min, rtc->sec);
	    Debug(console,"==========================\n");	
    }

    /* ---------------------------------------------------- */
	/*  MASTER/SLAVE 동작모드에 따른 계전기 Update          */
    /* ---------------------------------------------------- */
    for(i = 0; i < MAX_MODULE; i++)
    {
        dnp = (DNP_DEVICE *) dnpcfg[i];
        dnp->offlineTick= 0;
        //dnp->online		= 2;            // 초기상태 ... 미지정.
        dnp->initial		= RESET;            // 장치별 초기화 
        
        dnp->timeSyncReq 	= SET;
        dnp->statusDump 	= SET;
        dnp->analogDump 	= SET;
        
    }

#if 0    
    /* LINK-SOE Queue Clear... */
    linkCfg->front = 0;
    linkCfg->rear  = 0;
#endif
    
    
    
}

/*
========================================================
*   CPU : DeActive/비활성화
*    - 상위/하위 통신 Slave 기능, Shutdown Enable
========================================================
*/
void resetMaster()
{
    char    led;
    char    buffer[128];
    
    /* -------------------------------------------- */
	/*	CPU Shutdown : Enable .... SLAVE  기능      */
	/* -------------------------------------------- */
#ifdef	THE_ONE_TARGET	
	/* -------------------------------------------- */
	/*	CPU Shutdown : Disable .... MASTER 기능     */
	/* -------------------------------------------- */
    led = 0x0f;                     
    write( devHero, &led, 1);  
#else    
    led = 0x00;                     
    write( devHero, &led, 1);    
#endif
    
    opr->runMode = LOCAL_SLAVE;
	opr->rtuinit = 32;

    opr->stscode &= ~LED_ACTIVE_BIT;     // ACT-LED ON

#ifdef  ACE_LBS_4U
    /* -------------------------------------------- */
    /* CPU Live 상태표출 ... (Master/Slave 여부 )   */
    /* -------------------------------------------- */
    linkCfg->cpuControl &= ~DATA_CPU_ACTIVE;
	if(write(devHero, &linkCfg->cpuControl, 1) != 1) 
	{            
	    Debug(console,"2) %s write error : %s \n", DEVICE_EX_IO, strerror(errno));  
	}             
	
#else    
    /* -------------------------------------------- */
    /* CPU Live 상태표출 ... (Master/Slave 여부 )   */
    /* -------------------------------------------- */
    linkCfg->cpuControl &= ~DATA_CPU_ACTIVE;
	if(write(devExio, &linkCfg->cpuControl, 1) != 1) 
	{            
	    Debug(console,"2) %s write error : %s \n", DEVICE_EX_IO, strerror(errno));  
	}             
#endif
	
	//if(opr->linkDebug) 
	{
	    Debug(console,"\n==========================\n");
	    Debug(console," [%02d/%02d-%02d:%02d:%02d]... *** RESET MASTER ...!\n", rtc->month, rtc->day, rtc->hour, rtc->min, rtc->sec);
	    Debug(console,"==========================\n");	

//	    printf(" [%02d/%02d-%02d:%02d:%02d]... *** RESET MASTER ...!\n", rtc->month, rtc->day, rtc->hour, rtc->min, rtc->sec);
    }

    /* LINK-SOE Queue Clear... */
    linkCfg->front = 0;
    linkCfg->rear  = 0;
    linkCfg->eventRear = sque->front;
    
    /* LOG File 저장 */
    sprintf(buffer, "%s", ">> RESET-MASTER ");
    LogFile_MPU (opr, NOT_DEFINED, buffer, strlen(buffer));

#if 0
    /* ------------------------------------------ */
    for(i=0; i< MAX_MODULE; i++)
    {        
        dnp = (DNP_DEVICE *) dnpcfg[i];
        
        if((dnp->type == 0) || (dnp->scan == 0)) continue;
        if((dnp->tcpipThread == 0) && (dnp->SocketFd))   close( dnp->SocketFd);
    }
#endif
    
}


/*
*   CPU 이중화 상태 Check...
*/
int	checkDualCPU(int mode)
{
	char    boardSts;

    /* ---------------------------------------------------------------- */
	/*  이중화 CPU 상태 Check...                                        */
	/*        7        6       5       4        3       2      1     0  */
	/*    : ACTIVE : MA/AU : CH-A/B : xxx  : MST/SLV : DUAL : xxx : xxx */
	/* ---------------------------------------------------------------- */
	
#ifdef  ACE_LBS_4U
	if(read (devHero, &boardSts, 1) != 1)
    {
        Debug(console,"link> *** %s read error : %s \n", DEVICE_EX_IO, strerror(errno));  
        return (0);
    }
    linkCfg->cpuStatus = boardSts & 0x8f;
#else    
	if(read (devExio, &boardSts, 1) != 1)
    {
        Debug(console,"link> *** %s read error : %s \n", DEVICE_EX_IO, strerror(errno));  
        return (0);
    }
    linkCfg->cpuStatus = boardSts & 0x8f; // 1000_1111
#endif
    // cosSts와 linkStatus static ...
    cosSts     = linkStatus ^ linkCfg->cpuStatus;
    linkStatus = linkCfg->cpuStatus;
    
    if(opr->linkDebug)  
    Debug(console,"link> CPU status = %02x ...\n",linkCfg->cpuStatus);

//	if ( mode)
//    printf("link> CPU status = %02x ...\n",linkCfg->cpuStatus);
//
// Master는 0x09 (1001) 0x0d (1101) 
// slave는  0x05 (0101) MS* 상태변0x04  
    /* -------------------------------------------- */
    /*  CPU 상태 및 절체스위치 상태에 따른 처리     */
    /* -------------------------------------------- */
    if(mode)                        cosSts = 1;  // 무조건 조건 Master/SLave 선택 루틴 진입
    else if(scuCfg->changeFlag)     cosSts = 1;  // 무조건 조건 Master/SLave 선택 루틴 진입, scu에서 switch가 변경된 경우
           
    if(cosSts == 0)     return 0;
 
 //  SYS_MASTER_BIT )   이건 hw 적인 것이라 변경되지 않고...
 //  SYS_DUAL_CPU_BIT) 이건 상대방이 빠지면...변할 수 있지 그런데 latch라 죽으면 물고 있고.
 
    linkCfg->cmdFlag = RESET; // scu에서 해주면 mode =1로 호출된다.
                
    /* -------------------------------------------- */
	/* 상대방 CPU 장착유무 확인                     */
	/* -------------------------------------------- */
	if(linkCfg->cpuStatus & SYS_DUAL_CPU_BIT)      // 상대방 CPU의 Live 상태 Check...link main에서 write 했다.
    {
		opr->dualCpuSts = SET;                              // CPU 이중화 여부 결정...
		
        /* ---------------------------------------- */
    	/*  MASTER-CPU 의 경우...                   */
        /* ---------------------------------------- */
    	if(linkCfg->cpuStatus & SYS_MASTER_BIT)    // MASTER_BIT이 있는 쪽이 SYSTEM_A  
        {
            if(linkInitial) // 이 변수는 아래 Message출력제어용인가 보네...
            {    
                printf("\n=======================================\n");
//                printf("   **** DUAL CPU MODE : MASTER ***\n");
                printf("   **** DUAL CPU MODE : SYSTEM-A ***\n");
                printf("=======================================\n");
            }
                            
	        opr->cpuMode = CPU_MASTER;// SYSTEM_A라는 뜻이지... 여기서 정해는데 suc  Addr에서 쓰네
    	    
    	    /* -------------------------------------------- */
    	    /*  SCU 통신상태에 따른 이중화 절체...          */
    	    /* -------------------------------------------- */
    	    if(scuCfg->remoteMode == MANUAL_MODE)       /* 수동전환 모드의 경우...      */
    	    {							// MANUAL_MODE ==0 
    	        if(scuCfg->chanMode == SYSTEM_A)   // SYSTEM_A == 0
    	        {
    	        	//if(opr->linkDebug)
  //  	            printf("master) SCU-Manual : System-A MODE.... SET-Master...!\n");
    	            Debug(console,"master) SCU-Manual : System-A MODE.... SET-Master...!\n");
    	            setMaster();
    	        }
    	        else if(scuCfg->chanMode == SYSTEM_B)   
    	        {
    	        	//if(opr->linkDebug)
 //   	            printf("master) SCU-Manual : System-B MODE.... RESET-Master...!\n");
    	            Debug(console,"master) SCU-Manual : System-B MODE.... RESET-Master...!\n");
    	            resetMaster();
    	        }
    	    }
    	    else if(scuCfg->remoteMode == AUTO_MODE)    /* 자동전환 모드의 경우...      */
    	    {
    	        /* -------------------------------------------- */
	            /*  이중화 CPU 모두 Master 모드인 경우...       */
	            /*    처음에 scu와 통신전, 부팅후 모두 master 인가.
    	        /*    => MASTER-CPU : Active 모드 유지...       */
	            /*    => SLAVE-CPU  : Non-Active 모드 설정      */
	            /* -------------------------------------------- */
        	    if(opr->runMode == LOCAL_MASTER) // SYSTEM-A이고 LOCAL_MASTER인데...
                {
                    if(linkCfg->cpuStatus & DATA_CPU_ACTIVE)   //0x80. 이 값은 어디서 쓰지 ? setMaster에서쓴다..
                    										   // 같은 bit offset 이다. 쓰기 전에는 0 
                    										// 상대편이 setMaster가 호출되서 master가 되어 버렸네..
                    {

//                       printf("master) MASTER-CPU/MASTER-MODE.... RESET-Master...!\n");
                    	if(opr->linkDebug)
                        Debug(console,"master) MASTER-CPU/MASTER-MODE.... RESET-Master...!\n");
                        resetMaster();
                    }
                    // else 조건이면 아무것도 안하네..
                }
                else // SLAVE
                {
                    if((linkCfg->cpuStatus & DATA_CPU_ACTIVE) == 0)  // 상대가 없으면..                                      
                    {
 //                       printf("master) Slave-CPU : *reset status.... SET-Master...!\n");
    	                if(opr->linkDebug)
    	                Debug(console,"master) Slave-CPU : *reset status.... SET-Master...!\n");
    	                setMaster();    // 여기다... 상대도 resetMaster하고 들어오니까.
    	            }
    	        }    
    	    }    
    	        
    	        
    	    /* ------------------------------------ */
    	    /*  MASTER 측면 : 절체상태 표기...      */
    	    /* ------------------------------------ */
    	    if(opr->runMode == LOCAL_MASTER)
    	    {	
    	    	opr->masterSts = (CPU_ACT_BIT + CPU_RUN_BIT); //장착 + 동작 
				opr->slaveSts  = CPU_ACT_BIT; // 장착만
			}
			else
    	    {	
    	    	opr->masterSts = CPU_ACT_BIT;
				opr->slaveSts  = (CPU_ACT_BIT + CPU_RUN_BIT);
			}
        }
        else // DUAL 의 SYSTEM_B
        {
            if(linkInitial)
            { 
                printf("\n=======================================\n");
//                printf("   **** DUAL CPU MODE : SLAVE ***\n");
                printf("   **** DUAL CPU MODE : SYSTEM-B ***\n");
                printf("=======================================\n");
            }
                
   	        opr->cpuMode = CPU_SLAVE;
	        
	        
	        if(scuCfg->remoteMode == MANUAL_MODE)
    	    {
    	        if(scuCfg->chanMode == SYSTEM_A)   
    	        {
    	        	//if(opr->linkDebug)
    	            Debug(console,"slave) SCU-Manual : System-A MODE.... RESET-Master...!\n");
    	            resetMaster();
    	        }
    	        else if(scuCfg->chanMode == SYSTEM_B)   
    	        {
    	        	//if(opr->linkDebug)
    	            Debug(console,"slave) SCU-Manual : System-B MODE.... SET-Master...!\n");
    	            setMaster();
    	        }
    	    }
    	    else if(scuCfg->remoteMode == AUTO_MODE)
    	    {
    	        if(opr->runMode == LOCAL_MASTER)
                {
                    if(linkCfg->cpuStatus & DATA_CPU_ACTIVE)   // 상대있나 없나 ??
                    {
                    	//if(opr->linkDebug)
                        printf("slave) SLAVE-CPU/MASTER-MODE.... *RESET-Master...!\n");
                        Debug(console,"slave) SLAVE-CPU/MASTER-MODE.... *RESET-Master...!\n");
                        resetMaster();
                    }
                }
                else
                {
                    if((linkCfg->cpuStatus & DATA_CPU_ACTIVE) == 0)                                        
                    {
        	            //if(opr->linkDebug)
//        	            printf("slave) Master-CPU : *reset status.... SET-Master...!\n");
        	            Debug(console,"slave) Master-CPU : *reset status.... SET-Master...!\n");        	            
    	                setMaster();    
    	            }
                }     
    	    }
    	    
	        /* ------------------------------------ */
    	    /*  SLAVE  측면 : 절체상태 표기...      */
    	    /* ------------------------------------ */
	        if(opr->runMode == LOCAL_MASTER)
    	    {	
    	    	opr->masterSts = CPU_ACT_BIT;
				opr->slaveSts  = (CPU_ACT_BIT + CPU_RUN_BIT);
			}
			else
    	    {	
    	    	opr->masterSts = (CPU_ACT_BIT + CPU_RUN_BIT);
				opr->slaveSts  = CPU_ACT_BIT;
			}
        }    
        
    }	
    else   // single동작 --> 정확하게는 상대 CPU가 아직 동작전...                                  
    {
        /* -------------------------------------------- */
        /*  상대방 CPU가 동작하지 않는 경우...          */
        /* -------------------------------------------- */
		opr->dualCpuSts = RESET;
	    	
        /* ---------------------------------------------------- */
	    /*  MASTER/SLAVE 동작모드에 따른 OUTPUT- Enable         */
    	/* ---------------------------------------------------- */
	    if(linkCfg->cpuStatus & SYS_MASTER_BIT)      
    	{
	        opr->cpuMode = CPU_MASTER;
	        
	        /* SIM과 MMI 용 ...*/
   	    	opr->masterSts = (CPU_ACT_BIT + CPU_RUN_BIT);
			opr->slaveSts  = 0; // slave는 장착되지 않았다.
			
			if(linkInitial)
            { 
    			printf("\n=======================================\n");
// hkkim               printf("   **** SINGLE CPU MODE : MASTER ***\n");
                printf("   **** SINGLE CPU MODE : SYSTEM-A ***\n");
                printf("=======================================\n");
            }
	    }
    	else                
	    {
        	opr->cpuMode = CPU_SLAVE;
        	
        	opr->masterSts = 0; // master는 장착되지 않았다
			opr->slaveSts  = (CPU_ACT_BIT + CPU_RUN_BIT);
			
			if(linkInitial)
            { 
    			printf("\n=======================================\n");
//  hkkim              printf("   **** SINGLE CPU MODE : SLAVE ***\n");
                printf("   **** SINGLE CPU MODE : SYSTEM-B ***\n");
                printf("=======================================\n");
            }
            
    	}

		if(opr->runMode == LOCAL_SLAVE)
  	    {   
  	        Debug(console,">>> Single CPU : .... SET-Master...!\n");
    		setMaster(); // 처음에 부팅시 여기로 와서  master가 되는구나..
    		pause(100);
	    }    	
    }

    return (1);        
}

/*
*
*/
int check_IPString(char *str, byte *ipNum)
{
    int i;
    int     inx1, inx2;
    char    temp[8][8];
    
   	inx1 =0;
   	inx2 =0;
   	
   	ipNum[0] = 0;
    ipNum[1] = 0;
    ipNum[2] = 0;
    ipNum[3] = 0;
   	
   	for(i=0; i < 16; i++)
    {
        temp[inx1][inx2] = str[i];
        if(str[i] == '.')
        {
            temp[inx1][inx2] = '\0';
            inx1++;
            inx2 = 0;
        }    
        else
        {
            inx2++;
        }    
    }
        
    temp[inx1][inx2++] = '\0';
       
    ipNum[0] = atoi(temp[0]);
    ipNum[1] = atoi(temp[1]);
    ipNum[2] = atoi(temp[2]);
    ipNum[3] = atoi(temp[3]);
    
   
   return (0); 
    
}


/*
*   CPU 부팅후... HOST Route-Table 추가 
*/
int readRouteTable()
{
	//char	*buffer;                   
	//int     dbsize;
	//int     result=1;
	//int		size;
	char    buf[200];
	//byte	filename[64];

    FILE_DESC	romdbFile = {NULL, FIO_NORMAL, "/mnt/routetable.txt"};
    
    if(opr->cpuMode == CPU_MASTER)  sprintf( romdbFile.name, "%s", "/mnt/routetable-A.txt");
    else                            sprintf( romdbFile.name, "%s", "/mnt/routetable-B.txt");        

    printf("\n====================================================================\n");
    printf("  ROUTE-Table Read Start .... %s \n", romdbFile.name);
    
    /* Database File #1 Read ... */	
	if (FileOpen(&romdbFile) < 0)
	{
		printf("[route] *** %s Database File Open Error...!\n", romdbFile.name);
		return (-1);
	}
	
    while( fgets(buf, 200, romdbFile.id) != NULL)
    {
        if(buf[0] =='#')    
        {
            printf(">> %s ", buf);   
            continue;
        }
        
        printf(">> %s ", buf);     
        
        /* ---------------------------- */
        /* Linux-System 명령 출력...    */
        /* ---------------------------- */
        system(buf);
        pause(100);
    }
    
	FileClose(&romdbFile); 
	
	printf("  ROUTE-Table Read End .... %s \n", romdbFile.name);
	printf("====================================================================\n");
	return (1);
	
}



/* ------------------------------------------------------- */
/*  LINK MAIN 프로그램 Start Routine ....             */
/* ------------------------------------------------------- */
int	main(int argc, char **argv)
{
    //int     i,j;
    //int     fid;
	//byte	runtick;
	//int     retval;
	//int		state, oldsec;
    //int     link_fd, count;
    //int     size;
    //int     retVal;
    //int     dev;
    //char    boardSts;
    char    buffer[128];

    //int     inx1, inx2, ip1, ip2;
    //char    temp[8][8];
    //byte    ipNum[4];  
    //byte    maskNum[4];
    //byte    netNum[4];          
    
    //int     hostid;
    //int     size1, size2;
    
    //HOST_DCB    *host;
    
		read_config(0);
	// 작업 환경을 초기화한다
	termExec = InitEnv();
	
	taskPtr->wdtCount = 0;
		
	/* Signal Handler initial */
	signal( SIGBUS,  SigHandler);
	signal( SIGSEGV, SigHandler);
	signal( SIGTERM, SigHandler);	
	//signal( SIGPIPE, SigHandler);



	/* hkkim KEC-VMECU */
	  strncpy(linkPort.name, default_config.link_port,CONFIG_NAME_MAX);
    PortInitial((TTY_DESC *) &linkPort);



#ifdef  ACE_LBS_4U

    /* ---------------------------------------------------- */
    /*  System IO : /dev/hero-io : Shutdown  상태           */
    /* ---------------------------------------------------- */
    if((devHero = open(DEVICE_HERO_IO, O_RDWR | O_NDELAY)) < 0)
    {
        printf("link> *Driver  %s open fail : %s \n", DEVICE_HERO_IO, strerror(errno));  
        termExec = RESET;
        //exit(0);
    }
    
    /* CPU 상태정보 Read... */
    if(read (devHero, &linkCfg->cpuStatus, 1) != 1)
    {
        printf("link> *Driver %s read error : %s \n", DEVICE_HERO_IO, strerror(errno));  
        termExec = RESET;
        //exit(0);    	
    }
    
#else
    /* ---------------------------------------------------- */
    /*  System IO : /dev/hero-io : Shutdown  상태           */
    /* ---------------------------------------------------- */
    if((devHero = open(DEVICE_HERO_IO, O_RDWR | O_NDELAY)) < 0)
    {
        printf("link> *Driver  %s open fail : %s \n", DEVICE_HERO_IO, strerror(errno));  
        termExec = RESET;
        //exit(0);
    }
    
    /* ---------------------------------------------------- */
    /*  System IO : /dev/ex-io : Dual CPU, SIO, PWR 상태    */
    /* ---------------------------------------------------- */
    if((devExio = open(DEVICE_EX_IO, O_RDWR | O_NDELAY)) < 0)
    {
        printf("link> *Driver %s open fail : %s \n", DEVICE_EX_IO, strerror(errno));  
        termExec = RESET;
        //exit(0);
    }
    
    /* CPU 상태정보 Read... */
    if(read (devExio, &linkCfg->cpuStatus, 1) != 1)
    {
        printf("link> *Driver %s read error : %s \n", DEVICE_EX_IO, strerror(errno));  
        termExec = RESET;
        //exit(0);   	
    }
#endif

    /* 초기상태... CLEAR */    
    opr->masterSts  = 0; // mmi 와  sim에 system 동작상태표시용
    opr->slaveSts   = 0;
    
    /* -------------------------------------------- */
    /* SCU - 디폴트 절체 상체....                   */
    /* -------------------------------------------- */
    scuCfg->remoteMode  = MANUAL_MODE;
    scuCfg->chanMode    = SYSTEM_A;
    
    resetMaster(); // 여기서 DATA_CPU_LIVE clear 한다.
    
    pause(1000);
    pause(1000);

#ifdef  ACE_LBS_4U
    /* -------------------------------------------- */
    /* CPU Live 상태표출 ... (Dual/Single 여부 )    */
    /* -------------------------------------------- */
    linkCfg->cpuControl = DATA_CPU_LIVE;
	if(write(devHero, &linkCfg->cpuControl, 1) != 1) 
	{            
	    printf("link> *Driver %s write error : %s \n", DEVICE_EX_IO, strerror(errno));  
	}                    
	
#else    
    /* -------------------------------------------- */
    /* CPU Live 상태표출 ... (Dual/Single 여부 )    */
    /* -------------------------------------------- */
    linkCfg->cpuControl = DATA_CPU_LIVE;  // 상대편에게 PEER_IN 즉 SYS_DUAL_CPU_BIT 로 보인다
	if(write(devExio, &linkCfg->cpuControl, 1) != 1) 
	{            
	    printf("link> *Driver %s write error : %s \n", DEVICE_EX_IO, strerror(errno));  
	}                          
#endif

    /* -------------------------------------------- */
    /*  CPU 이중화 상태 Check...                    */
    /* -------------------------------------------- */
    linkInitial = SET;
    
//hkkim
#if 0
    if ( scuCfg->remoteMode &  MANUAL_MODE ) printf("LINK SCU MANUAL MODE\r\n");
   	else	printf("LINK SCU AUTO MODE\r\n");
    if ( scuCfg->chanMode & SYSTEM_A ) printf("LINK SCU SYSTEM-A\r\n");
   	else	printf("LINK SCU SYSTEM-B\r\n");  	
        

    if(opr->runMode == LOCAL_SLAVE)    
    		printf("LINK runmode LOCAL_SLAVE\r\n");
    else 
     		printf("LINK runmode LOCAL_MASTER\r\n");   	
#endif     
    
    if(checkDualCPU(1) == 0)  // mode=1 무조건 조건 검색
    {
        printf("link> *** check Dual CPU status... Error !\n");
        termExec = RESET;
        //exit(0);
    }

    linkInitial = RESET;

#if 0 // hkkim
    if(opr->runMode == LOCAL_SLAVE)    
    		printf("LINK runmode LOCAL_SLAVE\r\n");
    else 
     		printf("LINK runmode LOCAL_MASTER\r\n");  
#endif 

    /* ---------------------------------------------------- */
	/*  MASTER/SLAVE LOCAL 환경정보 초기화 ...              */
	/*  - MASTER : NET#1/NET#2/NET#3 초기화                 */
	/*  - SLAVE  : NET#1/NET#2/NET#3 초기화                 */
    /* ---------------------------------------------------- */
    if(opr->cpuMode == CPU_MASTER) // cpuMode는 HW 적 bit 값
    {
        printf("\n------------------------------------------\n");
        printf("link> ***** MASTER CPU ...OK \n");
        printf("------------------------------------------\n");
 
        
#ifdef  ROM_VERSION

        /* MASTER-CPU : eth0 Network...   */            
        sprintf(buffer, "ifconfig eth0 %s netmask %s up\n", opr->net_mst_IP[0], opr->net_mst_SUB[0]);
      	printf("Master-Net#1 : %s", buffer);
       	system(buffer);  
       	pause(100);  

#if 0        	
       	/* ---------------------------------------- */
       	/*  MASTER-CPU : Default Route ...          */
       	/*  - RouteTable 에서 수용                  */
       	/* ---------------------------------------- */
       	sprintf(buffer, "route add default gw %s eth0\n", opr->net_mst_GW[0]);
        printf("Master-Net#1 : %s", buffer);
        system(buffer);  
   	    pause(100);  
#endif
   	    
#endif
        
        /* MASTER-CPU : eth1 Network...   */ 
        if(opr->net_mst_Flag[1] == SET)
        {            
            sprintf(buffer, "ifconfig eth1 %s netmask %s up\n", opr->net_mst_IP[1], opr->net_mst_SUB[1]);
        	printf("Master-Net#2 : %s", buffer);
        	system(buffer);  
    	    pause(100);  
        }
        
        /* MASTER-CPU : eth2 Network...   */ 
        if(opr->net_mst_Flag[2] == SET)
        {  
            sprintf(buffer, "ifconfig eth2 %s netmask %s up\n", opr->net_mst_IP[2], opr->net_mst_SUB[2]);
        	printf("Master-Net#3 : %s", buffer);
        	system(buffer);  
    	    pause(100);  
        }    
    }
    else // // cpuMode는 HW 적 bit 값
    {
        printf("\n------------------------------------------\n");
        printf("link> ***** SLAVE CPU ...OK \n");
        printf("------------------------------------------\n");
        
#ifdef  ROM_VERSION

        /* SLAVE-CPU : eth0 Network...   */ 
        sprintf(buffer, "ifconfig eth0 %s netmask %s up\n", opr->net_slv_IP[0], opr->net_slv_SUB[0]);
       	printf("Master-Net#1 : %s", buffer);
       	system(buffer);  
       	pause(100);  

#if 0
        /* ---------------------------------------- */
       	/*  SLAVE-CPU : Default Route ...           */
       	/*  - RouteTable 에서 수용                  */
       	/* ---------------------------------------- */ 	
       	sprintf(buffer, "route add default gw %s eth0\n", opr->net_slv_GW[0]);
        printf("Master-Net#1 : %s", buffer);
        system(buffer);  
   	    pause(100);  
#endif
   	    
#endif

        /* SLAVE-CPU : eth1 Network...   */ 
        if(opr->net_slv_Flag[1] == SET)
        {            
            sprintf(buffer, "ifconfig eth1 %s netmask %s up\n", opr->net_slv_IP[1], opr->net_slv_SUB[1]);
        	printf("Slave-Net#2 : %s", buffer);
        	system(buffer);  
    	    pause(100);  
        }
        
        /* SLAVE-CPU : eth2 Network...   */ 
        if(opr->net_slv_Flag[2] == SET)
        {  
            sprintf(buffer, "ifconfig eth2 %s netmask %s up\n", opr->net_slv_IP[2], opr->net_slv_SUB[2]);
        	printf("Slave-Net#3 : %s", buffer);
        	system(buffer);  
    	    pause(100);  
        }    
        
    }

    printf("------------------------------------------\n");

    /* ---------------------------------------------------- */
    /*  HOST Network 초기화 : Route Tabel...                */
    /*  - file : /mnt/routetable.txt                        */
    /* ---------------------------------------------------- */
    readRouteTable();
    
    /* ---------------------------------------------------- */
	/*  MASTER/SLAVE 운영모드별 TASK 호출                   */
    /* ---------------------------------------------------- */
    if(opr->cpuMode == CPU_MASTER)      
    {   	    
   	    linkMaster();        
    }
    else                
    {
        linkSlave();
    }
        
#if 0
    /* 단독 CPU 운영시 ... */
    if(opr->dualCpuMode == RESET)   
    {
        printf("\n------------------------------------------\n");
        printf("link> DUAL-CPU Mode : SINGLE-CPU  \n");
        printf("------------------------------------------\n");
        
        opr->checkLinkOK   = SET;		// CPU 링크상태 확인....
        setMaster();
        
        while(termExec)
	    {
	        taskPtr->wdtCount = 0;
		    pause(200);
		}
    }
    else
    {          
       	if(opr->cpuMode == CPU_MASTER)      
   	    {   	    
       	    linkMaster();        
        }
        else                
        {
   	        linkSlave();
        }
    }
#endif
    
    printf("are you see me\r\n");
    resetMaster();    		
		            
	// 작업 환경을 정리한다
	ClearEnv();
	
	return (0);
}
