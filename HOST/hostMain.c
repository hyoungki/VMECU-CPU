
#include	"localLib.h"
#include    "external.h"

extern  int    hostDnp_tcpip_Thread(int hostid);       // TCPIP DNP
extern  int    hostDnpThread(int hostid);
extern  int    hostHarrisThread(int hostid);
extern  int    hostLandisThread(int hostid);




THREAD_ENTRY	hostTHREAD[MAX_HOST];
TASK_INFO	    *taskPtr= NULL;
int             termExec;
int             wdtHostFlag[MAX_HOST];

SHM_DESC	    shmDesc = {-1, SHM_KEY, sizeof(SHM_MEMORY), NULL};
SHM_MEMORY	    *shmPtr = NULL;

RTC             *rtc    = NULL;
OPR_MSG         *opr    = NULL;
LINK_MSG		*linkCfg= NULL;

SYSTEM_QUE      *sque   = NULL;         // MMI 용 이벤트
HISTORY_QUE     *hque   = NULL;         // CONSOLE 용 이벤트
RTU_DATABASE    *rtudb  = NULL;
SCU_MSG         *scuCfg = NULL;

CONSOLE_INFO	*console= NULL;

HOST_DCB        *hostDCB[MAX_HOST];             /* DNP HOST Structure */

LANDIS_CFG      *landis;                        /* LANDIS HOST Structure */
HOST_STSBUF     *hostStsBuf[MAX_HOST_STS_PT];   /* LANDIS-HOST defined Status Point */
HOST_ANABUF     *hostAnaBuf[MAX_HOST_ANA_PT];   /* LANDIS-HOST defined Analog Point */

SCAN_CONFIG     *scanCfg[MAX_SCAN_PORT];        // 하위계전기 SCAN Config
DNP_DEVICE      *dnpcfg[MAX_MODULE];

byte            vmeRxbuf[MAX_HOST][1024];

PDPDTIME_DATE_TIME      dnpRTC;
PDPDTIME_MS_SINCE_70    dnpRTCInfo;

/* HOST 통신용 ASYNC PORT */
TTY_DESC tty_HostChan[MAX_CPUPORT];


/* ---------------------------------------- */
/*  VMEBUS 관련 변수 초기화                 */
/* ---------------------------------------- */
void    *vmebus_ptr ;
int     vme_fd; 

VME_SIODCB      *vmeSioDCB;
VME_CHAN_DCB    *vmeChanDCB[SIO_CHANNEL_MAX];


VME_SIODCB      tempVmeSioDCB;
VME_CHAN_DCB    tempVmeChanDCB[SIO_CHANNEL_MAX];

byte    bitcode[8] = {0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80};

word    hostLEDsts[MAX_HOST] = {0x0080, 0x0100, 0x0200, 0x0400, 0x0800};


/*
*   VME Channel WRITE :
*/
void sioThreadReset( )
{
    //HOST_DCB        *hst;
    //VME_CHAN_DCB    *vmeChan;
    
    vmeSioDCB->initReq = 0x1234;
    vmeSioDCB->initEnd = 0x4321;
    
    Debug(console,"\n===================================\n");
    Debug(console,"===> SIO Thread Re-Start...!\n");
    Debug(console,"===================================\n");
}

/*
*   VME Channel WRITE :
*/
void vmeHostChanReset(int hostid, int chid)
{
    HOST_DCB        *hst;
    VME_CHAN_DCB    *vmeChan;
    
    hst = (HOST_DCB *) hostDCB[hostid];
    vmeChan = hst->vmeChan[chid];
    
    vmeChan->initReq = 0x1234;
    
    //Debug(console,"\n ===> VME Channel[%2d] Re-Initial...!\n", chid);
}

/*
*/
void  modemDelay(int  msec)
{
    //fd_set      reads, temps;
    //int         result;
    struct timeval timeDelay;    

    timeDelay.tv_sec = 0;
    timeDelay.tv_usec = msec * 1000;
    
    select( 1, NULL, NULL, NULL, &timeDelay);
}

/*
*   VME Channel WRITE :
*/
int hostChanWrite(int hostid, int chid, int socketID,  byte *sndbuf, int sndCount)
{
    //int         timeout, mode;
    //char        ctsSts;
    int         rxVal;
    HOST_DCB    *host;
    //TTY_DESC    *tp;
    
    /* ---------------------------------------------------- */
	/*	CPU 이중화 : LOCAL-SLAVE ... 대기모드 				*/
	/* ---------------------------------------------------- */
	//if(opr->runMode == LOCAL_SLAVE)	return (0);  
		
    host = (HOST_DCB *) hostDCB[hostid];
    
    if(host->hostComType == COM_TCPIP)
    {
        if((rxVal = tkWriteTCP( socketID ,(byte *) sndbuf, sndCount, 100)) < 0 )
        {
            if(opr->hostDebug == hostid)  Debug(console,"host%02d> *** Channel=%d  net Send Error [%d/%d]...socket = %d !\n", hostid+1, chid+1, sndCount,  rxVal, socketID);		
        }
#if 0
        if( rxVal != sndCount)
        {
            if(opr->hostDebug == hostid)  Debug(console,"host%02d> **** Channel=%d   net Send Error [%d %d]...!\n", hostid+1, chid+1, sndCount,  rxVal);
            host->threadActive[chid] = RESET;
        }           
#endif        
    }
    else
    {        
        /* ---------------------------------------------------- */
        /*  RTU-TYPE : ACE-LBS-6U... ASYNC  PORT Comm Parameter */
        /*  RTU-TYPE : ACE-VMECU ... VMESIO PORT Comm Parameter */
        /* ---------------------------------------------------- */
#ifdef  ACE_LBS_RTU        
        PortWrite(host->ttyPort[chid], sndbuf, sndCount);
#else
        vmeChanWrite(hostid, vmeSioDCB, host->vmeChan[chid], sndbuf, sndCount);
#endif
        
    }
    
    return (sndCount);
    
}

/*
*   VME Channel READ :
*/
int hostChanRead(int hostid, int chid, byte *rxbuf, int reqCount)
{
    int rcvCount;
    HOST_DCB  *host;
    //TTY_DESC    *tp;

	/* ---------------------------------------------------- */
	/*	CPU 이중화 : LOCAL-SLAVE ... 대기모드 				*/
	/* ---------------------------------------------------- */
	if(opr->runMode == LOCAL_SLAVE)	return (0);  
		
    host = (HOST_DCB *) hostDCB[hostid];

    /* ---------------------------------------------------- */
    /*  RTU-TYPE : ACE-LBS-6U... ASYNC  PORT Comm Parameter */
    /*  RTU-TYPE : ACE-VMECU ... VMESIO PORT Comm Parameter */
    /* ---------------------------------------------------- */
#ifdef  ACE_LBS_RTU        
    rcvCount = asyncRead( opr, host->ttyPort[chid], rxbuf, reqCount);
#else
    rcvCount = vmeChanRead(hostid, vmeSioDCB, host->vmeChan[chid], rxbuf, reqCount);
#endif    

    return (rcvCount);
    
}

/*
*   VME Channel READ :
*/
int hostVMERead(int hostid, int chid, byte *rxbuf)
{
    int     i;
    int     rcvCount;
	//int	    size,rcount;
	int	    timeout = 0;
	//int	    retValue, rxCount;

    HOST_DCB        *hst;
    VME_CHAN_DCB    *vmeChan;
    
    /* ---------------------------------------------------- */
	/*	CPU 이중화 : LOCAL-SLAVE ... 대기모드 				*/
	/* ---------------------------------------------------- */
	if(opr->runMode == LOCAL_SLAVE)	return (0);  
		
    hst = (HOST_DCB *) hostDCB[hostid];
    
    vmeChan = (VME_CHAN_DCB *) hst->vmeChan[chid];
    
    while(vmeSioDCB->mpuAccess + vmeSioDCB->sioAccess)  
    {
        usleep(1000);
        if(++timeout > 20)  
        {
            //Debug(console,"vme%02d> read ready timeout...\n", chid);
            return (0);
        }            
    }

    if(vmeChan->rxFlag)
    {
        vmeSioDCB->mpuAccess = 1;
        rcvCount = vmeChan->rxCount;

        for(i=0; i< rcvCount; i++)      rxbuf[i] = vmeChan->rcvbuf[i];
        vmeChan->rxFlag     = 0;
        vmeChan->rxCount    = 0;
        
        vmeSioDCB->mpuAccess = 0;
        
        return(rcvCount);
    }
        
    return(0);    
}


/* *************************************************************************************
*	FUNCTION : sigHandler()
* **************************************************************************************/
void	SigHandler( int sig )
{
    char    buffer[128];
    
    Debug(console,"host> ... signal generated (%2d)...!\n", sig);
    pause(100);
    
	switch(sig)
	{
    case SIGTERM:
        termExec = 0;
        if(opr->wdtDebug)   Debug(console,"host> ... signal [SIGTERM] generated (%2d)...!\n", sig);
            
        /* ------------------------------------------------ */
        /* LOG File 저장                                    */
        /* ------------------------------------------------ */
        sprintf(buffer, "host> ... signal [SIGTERM] generated (%2d)...!\n", sig);
        LogFile_MPU ( opr, NOT_DEFINED, buffer, strlen(buffer));
            
        pause(1000);     
        break;
	        
	case SIGBUS : 
	    termExec = 0;
        if(opr->wdtDebug)   Debug(console,"host> ... signal [SIGBUS] generated (%2d)...!\n", sig);
        
        /* ------------------------------------------------ */
        /* LOG File 저장                                    */
        /* ------------------------------------------------ */
        sprintf(buffer, "host> ... signal [SIGBUS] generated (%2d)...!\n", sig);
        LogFile_MPU ( opr, NOT_DEFINED, buffer, strlen(buffer));
            
        pause(1000);     	    
	    break;
		    
	case SIGSEGV: 
	    termExec = 0;
        if(opr->wdtDebug)   Debug(console,"host> ... signal [SIGSEGV] generated (%2d)...!\n", sig);
        
        /* ------------------------------------------------ */
        /* LOG File 저장                                    */
        /* ------------------------------------------------ */
        sprintf(buffer, "host> ... signal [SIGSEGV] generated (%2d)...!\n", sig);
        LogFile_MPU ( opr, NOT_DEFINED, buffer, strlen(buffer));
            
        pause(1000);     	    
	    break;
		    
	case SIGPIPE: 
        if(opr->wdtDebug)   Debug(console,"host> ... signal [SIGPIPE] generated (%2d)...!\n", sig);
        
        /* ------------------------------------------------ */
        /* LOG File 저장                                    */
        /* ------------------------------------------------ */
        sprintf(buffer, "host> ... signal [SIGPIPE] generated (%2d)...!\n", sig);
        LogFile_MPU ( opr, NOT_DEFINED, buffer, strlen(buffer));
            
        pause(1000);     	    
		break;

	default : 
	    if(opr->wdtDebug)	Debug(console,"host> ... signal[%2d] generated ...!\n", sig);
	    
	    /* ------------------------------------------------ */
        /* LOG File 저장                                    */
        /* ------------------------------------------------ */
        sprintf(buffer, "host> ... signal[%2d] generated ...!\n", sig);
        LogFile_MPU ( opr, NOT_DEFINED, buffer, strlen(buffer));
            
		break;
	}
	
}


//
// 모듈:	ClearEnvironment()
//
void ClearEnv(int hostid)
{
	
    munmap( vmebus_ptr, VMESIO_SIZE);	
	// 공유 메모리 설정 해제
	if (shmPtr != NULL)
		ShmDetach(&shmDesc);

	// 프로세스 정보를 정리한다
	UpdateProcessInfo(taskPtr, 0, getpid(), hostid+1);
			
}
					    

//
// 모듈:	InitEnvironment()
//
int InitEnv( int hostid)
{
    int     i;
    
	// 공유 메모리 상태 확인
	if (ShmCheck(&shmDesc) < 0)
	{
		printf("host> *ERR_공유 메모리 상태 이상\n");
		return(0);
	}
	
	//printf("host> SHM %p \n", shmDesc.address);
	
	// 포인터 변수 초기화
	shmPtr = (SHM_MEMORY *) shmDesc.address;
	taskPtr = (TASK_INFO *) &shmPtr->taskInfo[HOST1_PROCESS + hostid];

	rtc     = (RTC *)       &shmPtr->rtc;  
	opr     = (OPR_MSG *)   &shmPtr->opr_msg;
	linkCfg = (LINK_MSG *)  &shmPtr->link_msg;
	
	rtudb   = (RTU_DATABASE *) &shmPtr->rtuDatabase;
    sque    = (SYSTEM_QUE *)  &shmPtr->localEvent; 
    hque    = (HISTORY_QUE *) &shmPtr->localHistoryQ; 
    scuCfg 	= (SCU_MSG *)   &shmPtr->scu_msg;
    
    console = (CONSOLE_INFO *) &shmPtr->console; 
     
    /* ------------------------------------ */
    /* HOST_DCB : 상위 HOST 구조체          */
    /* ------------------------------------ */
    for(i=0; i< MAX_HOST; i++)  
    {
    	hostDCB[i] 	= (HOST_DCB *) &shmPtr->hostDCB[i];
    }
    
    for(i=0; i< MAX_HOST_STS_PT; i++)   hostStsBuf[i] = (HOST_STSBUF *) &shmPtr->hostStsBuf[i];
    for(i=0; i< MAX_HOST_ANA_PT; i++)   hostAnaBuf[i] = (HOST_ANABUF *) &shmPtr->hostAnaBuf[i];
	
	landis = (LANDIS_CFG *) &shmPtr->landis;                    /* LANDIS HOST Structure */
	
    /* ------------------------------------ */
    /* DNP_DEVICE : 전자식 배전반 구조체    */
    /* ------------------------------------ */
    for(i=0; i< MAX_SCAN_PORT; i++) scanCfg[i] = (SCAN_CONFIG *) &shmPtr->scanCFG[i];
    for(i=0; i< MAX_MODULE; i++)    dnpcfg[i] = (DNP_DEVICE *) &shmPtr->dnpCFG[i];
		
    /* ------------------------------------------------------------ */
	/* 프로세스 정보를 초기화한다                                   */
	/* HOST Task 는 TCPIP 전송대기를 고려하여 WDT 기능을 삭제함...  */
	/* ------------------------------------------------------------ */
	taskPtr->initial    = 1;
	taskPtr->wdtEnable  = 1;            
	
	UpdateProcessInfo(taskPtr, 1, getpid(), hostid+1);
	
	return(1);
}


/*
*   VMEBUS : SIO 관련 메모리 초기화...
*/
int  init_vmebus(void)
{
    /* ------------------------------------ */
    /*  VMEBUS MEMORY DEVICE 초기화         */
    /* ------------------------------------ */
    if ((vme_fd = open("/dev/mem", O_RDWR) ) < 0)
    {
        printf("host> *open VME_AREA : /dev/mem ... Error ! \n");
        return (NOK);
    }        

    //printf("host> open VME_AREA : %p ... OK ! \n",VME_BASE_ADDRESS );

    /* -------------------------------------------- */
    /*  VME Backup Memory 초기화                    */
    /*  - SIO Module Size : 0x20000                 */
    /* -------------------------------------------- */
    if((vmebus_ptr = ( char  *) mmap(0, VMESIO_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, vme_fd, (VMESIO_START_ADDRESS)) ) < 0)
    {            
        printf("host> *VME Memory Mapping ... Error ! \n");
        return (NOK);            
    }
    
    //printf("host> MAP VME_AREA : %p ... OK ! \n",vmebus_ptr );
    

  
    
    return (OK);
}    

/*
*/
int	init_variable()
{
    int i;
    //word    chksum;
    int     size1, size2;
    //word    *wptr1, *wptr2;     

    /* -------------------------------- */
    /* 1. 외부 메모리 포인트 초기화 ... */
    /* -------------------------------- */
    if(init_vmebus() == OK)
    {
        /* SIO보드 초기화...내부 변수 초기화에 따른 변수 */             
	    vmeSioDCB  = (VME_SIODCB *)(  vmebus_ptr);   
	    size1 = sizeof(VME_SIODCB);
	    //printf("host> VME SIO  Address : %p, size=%d\n", vmeSioDCB, size1);  

        vmeSioDCB->sioAccess = 0;
        vmeSioDCB->mpuAccess = 0;
        vmeSioDCB->initReq   = 0;   
        vmeSioDCB->initEnd   = 0;    
        vmeSioDCB->resetFlag = 0;          	    
        
        for(i=0; i< SIO_CHANNEL_MAX; i++)
        {
            size2 = sizeof(VME_CHAN_DCB);
            vmeChanDCB[i] = (VME_CHAN_DCB *) (vmebus_ptr + size1 + (i*size2));   	    
	        //printf("host> VME CHAN(%d) Address : %p, size=%d\n", i, vmeChanDCB[i], size2);  
        }	               
        
    }
    else
    {
	    printf("host> **** VMEBUS Mapping Error...Error \n"); 

        /* SIO보드 초기화...내부 변수 초기화에 따른 변수 */             
	    vmeSioDCB  = (VME_SIODCB *) &tempVmeSioDCB;   
	    size1 = sizeof(VME_SIODCB);
	    //printf("host> VME SIO  Address : %p, size=%d\n", vmeSioDCB, size1);  

        vmeSioDCB->sioAccess = 0;
        vmeSioDCB->mpuAccess = 0;
        vmeSioDCB->initReq   = 0;   
        vmeSioDCB->initEnd   = 0;    
        vmeSioDCB->resetFlag = 0;          	    
        
        for(i=0; i< SIO_CHANNEL_MAX; i++)
        {
            size2 = sizeof(VME_CHAN_DCB);
            vmeChanDCB[i] = (VME_CHAN_DCB *) &tempVmeChanDCB[i];   	    
        }	                
        
    }

    /* -------------------------------------------- */
    /*  HOST 통신포트 초기화 (Default)      */
    /* -------------------------------------------- */
    for(i=0; i< MAX_CPUPORT; i++)
    {
        tty_HostChan[i].chid     = i;
        tty_HostChan[i].id       = 0;
        tty_HostChan[i].fId      = NULL;
        tty_HostChan[i].config   = ASYNC_860T;
        tty_HostChan[i].commMode = COM_RS232;
        tty_HostChan[i].baudRate = 38400;
        tty_HostChan[i].dataBits = 8;
        tty_HostChan[i].stopBits = 1;
        tty_HostChan[i].lineDebug= 0;
        tty_HostChan[i].parity   = 'N';
        
        tty_HostChan[i].timeOut.tv_sec = 0;
        tty_HostChan[i].timeOut.tv_usec = 50000;
    }

/* ---------------------------------------------------- */
/*  RTU-TYPE : ACE-LBS-4U... ASYNC  PORT Comm Parameter */
/* ---------------------------------------------------- */
#ifdef  ACE_LBS_4U
    sprintf(tty_HostChan[0].name, "%s", FID_SCAN1_PORT);
    sprintf(tty_HostChan[1].name, "%s", FID_SCAN2_PORT);
    sprintf(tty_HostChan[2].name, "%s", FID_SCAN3_PORT);
    sprintf(tty_HostChan[3].name, "%s", FID_SCAN4_PORT);
#else
    /* ------------------------------------------------ */
    /*  LBS-6U HOST 통신용 Serial 포트 매칭...          */
    /* ------------------------------------------------ */
    sprintf(tty_HostChan[0].name, "%s", FID_RFU_PORT);          // for COM1
    //sprintf(tty_HostChan[1].name, "%s", FID_SCAN4_PORT);
    sprintf(tty_HostChan[1].name, "%s", FID_MMI_PORT);          // for COM2

    sprintf(tty_HostChan[2].name, "%s", FID_SCAN1_PORT);
    sprintf(tty_HostChan[3].name, "%s", FID_SCAN2_PORT);
    sprintf(tty_HostChan[4].name, "%s", FID_SCAN3_PORT);
    sprintf(tty_HostChan[5].name, "%s", FID_SCAN4_PORT);
    
#endif
        
    return (0);        
}


/*
*   FUNCTION : hostDCBInitial()
*   - 계전기 장치 구조체 ... 초기화
*/
void hostDCBInitial(int hostid)
{
    HOST_DCB        *host;

    /* --------------------------- */
    /*    HOST 별 운영정보 init    */
    /* --------------------------- */        
    host = (HOST_DCB *) hostDCB[hostid];                 /* 주장치 #1 속성정의 */
    
    host->id = hostid;
    host->initial	= SET;

    /* DNP initialize ... */
    host->sndFlag    = RESET;
    host->sndLinkSts = RESET;
    host->rcvLinkSts = RESET;           // LINK RESET 상태 
    host->rcvEndOk   = SET;

    host->diIndexWord = RESET;          // DI Point 최대 수 : 256보다 큰경우 WORD 처리 
    host->aiIndexWord = RESET;          // AI Point 최대 수 : 256보다 큰경우 WORD 처리 

    /* ---------------------------------------------------- */
    /*  상위 HOST별 DI/AI 포인트 수에 따른 데이터 처리      */
    /* ---------------------------------------------------- */
    if(host->diPtNum > 255) host->diIndexWord    = SET;
    if(host->aiPtNum > 255) host->aiIndexWord    = SET; 
        
    /* HOST 미지정시... */
    if(host->hostDualMode == HOST_NOT_USE)  return;
    if(host->hostComType == COM_TCPIP)      return;

    /* ---------------------------------------------------- */
    /*  RTU-TYPE : ACE-LBS-6U... ASYNC  PORT Comm Parameter */
    /*  RTU-TYPE : ACE-VMECU ... VMESIO PORT Comm Parameter */
    /* ---------------------------------------------------- */
#ifdef  ACE_LBS_RTU

#else        
    /* ------------------------------------ */
    /* Default VME Channel 지정             */
    /* ------------------------------------ */
    host->vmeChan[0] = (VME_CHAN_DCB *) vmeChanDCB[host->masterChan];   /* MASTER 포트 VME 초기화 */  
    host->vmeChan[1] = (VME_CHAN_DCB *) vmeChanDCB[host->slaveChan];    /* SLAVE  포트 VME 초기화 */   
    
    if(host->hostProtocol == HOST_IEC_101)
    {
        host->vmeChan[0]->protocolType = SCAN_IEC;        // HOST 별 프로토콜 타입 지정 
        host->vmeChan[1]->protocolType = SCAN_IEC;
        host->vmeChan[0]->cfgParity    = PARITY_EVEN;
        host->vmeChan[1]->cfgParity    = PARITY_EVEN;
        
        host->vmeChan[0]->cfgData      = DATA_8_BIT;
        host->vmeChan[1]->cfgData      = DATA_8_BIT;
        host->vmeChan[0]->cfgStopbit   = STOP_1_BIT;
        host->vmeChan[1]->cfgStopbit   = STOP_1_BIT;
    }
    else if(host->hostProtocol == HOST_DNP)
    {
        host->vmeChan[0]->protocolType = SCAN_DNP;        // HOST 별 프로토콜 타입 지정 
        host->vmeChan[1]->protocolType = SCAN_DNP;
        host->vmeChan[0]->cfgParity    = PARITY_NONE;
        host->vmeChan[1]->cfgParity    = PARITY_NONE;
        
        host->vmeChan[0]->cfgData      = DATA_8_BIT;
        host->vmeChan[1]->cfgData      = DATA_8_BIT;
        host->vmeChan[0]->cfgStopbit   = STOP_1_BIT;
        host->vmeChan[1]->cfgStopbit   = STOP_1_BIT;
    }
    else if(host->hostProtocol == HOST_MODBUS)
    {
        host->vmeChan[0]->protocolType = SCAN_MODBUS;        // HOST 별 프로토콜 타입 지정 
        host->vmeChan[1]->protocolType = SCAN_MODBUS;
        host->vmeChan[0]->cfgParity    = PARITY_NONE;
        host->vmeChan[1]->cfgParity    = PARITY_NONE;
        
        host->vmeChan[0]->cfgData      = DATA_8_BIT;
        host->vmeChan[1]->cfgData      = DATA_8_BIT;
        host->vmeChan[0]->cfgStopbit   = STOP_1_BIT;
        host->vmeChan[1]->cfgStopbit   = STOP_1_BIT;
    }
    else if(host->hostProtocol == HOST_HARRIS)
    {
        host->vmeChan[0]->protocolType = SCAN_HARRIS;        // HOST 별 프로토콜 타입 지정 
        host->vmeChan[1]->protocolType = SCAN_HARRIS;
        host->vmeChan[0]->cfgParity    = PARITY_ODD;
        host->vmeChan[1]->cfgParity    = PARITY_ODD;
        
        host->vmeChan[0]->cfgData      = DATA_7_BIT;
        host->vmeChan[1]->cfgData      = DATA_7_BIT;
        host->vmeChan[0]->cfgStopbit   = STOP_1_BIT;
        host->vmeChan[1]->cfgStopbit   = STOP_1_BIT;
    }
    else
    {
        host->vmeChan[0]->protocolType = SCAN_ASYNC;        // HOST 별 프로토콜 타입 지정 
        host->vmeChan[1]->protocolType = SCAN_ASYNC;
        host->vmeChan[0]->cfgParity    = PARITY_NONE;
        host->vmeChan[1]->cfgParity    = PARITY_NONE;
        
        host->vmeChan[0]->cfgData      = DATA_8_BIT;
        host->vmeChan[1]->cfgData      = DATA_8_BIT;
        host->vmeChan[0]->cfgStopbit   = STOP_1_BIT;
        host->vmeChan[1]->cfgStopbit   = STOP_1_BIT;
    }
        
    host->vmeChan[0]->cfgType      = host->hostComType;
    host->vmeChan[1]->cfgType      = host->hostComType;

    host->vmeChan[0]->cfgSpeed     = host->hostComSpeed;
    host->vmeChan[1]->cfgSpeed     = host->hostComSpeed;

#endif

    //Debug(console,"host> => HOST(%d) DNP initial...!\n", hostid+1);
}


/*
*   FUNCTION : vmeHostInitial()
*   - 계전기 장치 구조체 ... 초기화
*/
void vmeHostInitial(int hostid, int chid)
{
    //int i, j;
    
    VME_CHAN_DCB    *vmeChan;
    HOST_DCB        *host;
    //DNP_ANA_INPUT   *anaPoint, *anaPoint1;

    host    = (HOST_DCB *) hostDCB[hostid];                 /* 주장치 #1 속성정의 */
    vmeChan = (VME_CHAN_DCB *) vmeChanDCB[chid];
    
    vmeChan->rxFront = 0;
    vmeChan->rxRear  = 0;
    vmeChan->rxCount = 0;
    vmeChan->rxFlag  = 0;        
        
    vmeChan->txCount = 0;
    vmeChan->txFlag  = 0;
    
    if(host->hostProtocol == HOST_IEC_101)
    {
        vmeChan->protocolType = SCAN_IEC;           // HOST 별 프로토콜 타입 지정 
        vmeChan->cfgParity    = PARITY_EVEN;
        
        vmeChan->cfgData      = DATA_8_BIT;
        vmeChan->cfgStopbit   = STOP_1_BIT;
    }
    else if(host->hostProtocol == HOST_DNP)
    {
        vmeChan->protocolType = SCAN_DNP;           // HOST 별 프로토콜 타입 지정 
        vmeChan->cfgParity    = PARITY_NONE;
        
        vmeChan->cfgData      = DATA_8_BIT;
        vmeChan->cfgStopbit   = STOP_1_BIT;
    }
    else if(host->hostProtocol == HOST_MODBUS)
    {
        vmeChan->protocolType = SCAN_MODBUS;        // HOST 별 프로토콜 타입 지정 
        vmeChan->cfgParity    = PARITY_NONE;
        vmeChan->cfgData      = DATA_8_BIT;
        vmeChan->cfgStopbit   = STOP_1_BIT;
    }
    else if(host->hostProtocol == HOST_HARRIS)
    {
        vmeChan->protocolType = SCAN_HARRIS;        // HOST 별 프로토콜 타입 지정 
        vmeChan->cfgParity    = PARITY_ODD;
        vmeChan->cfgData      = DATA_7_BIT;
        vmeChan->cfgStopbit   = STOP_1_BIT;
    }
    else
    {
        vmeChan->protocolType = SCAN_ASYNC;        // HOST 별 프로토콜 타입 지정 
        vmeChan->cfgParity    = PARITY_NONE;
        vmeChan->cfgData      = DATA_8_BIT;
        vmeChan->cfgStopbit   = STOP_1_BIT;
    }
            
    vmeChan->cfgType      = host->hostComType;
    vmeChan->cfgSpeed     = host->hostComSpeed;
                        
    //printf("host> => HOST(%d) Channel(%d) initial...!\n", hostid+1, chid);
}


/*
* VME 버스상의 SIO 모듈 Time-Sync
*/
int vmeTimeSync()
{
	int	year;
	
	/* ---------------------------------------- */
	/*	SLAVE 모드에서 VME Access 를 하지 않음  */
	/* ---------------------------------------- */
	if(opr->runMode == LOCAL_SLAVE)     return (0);

    /* VME BUS 상의 SIO보드 - Time Sync...  */        
	year = (rtc->year % 100) + 2000;
    vmeSioDCB->year  = year;
    vmeSioDCB->month = rtc->month;
    vmeSioDCB->day   = rtc->day;
    vmeSioDCB->week  = rtc->week;
    vmeSioDCB->hour  = rtc->hour;
    vmeSioDCB->min   = rtc->min;
    vmeSioDCB->sec   = rtc->sec;
    
    vmeSioDCB->rtcUpdateFlag = 0x1234;
    
    if(opr->wdtDebug)
	Debug(console,"[vme Time-Sync] %04d:%02d:%02d-%02d-%02d:%02d:%02d ...\n", 	year, rtc->month, rtc->day, rtc->week, rtc->hour, rtc->min, rtc->sec);
	
	return (0);
}
    

/*
*   원격관제 와의 통신상태 확인...
*   - 1분동안 통신이 끊어진 경우... CPU 절체
*   - 5분동안 통신이 끊어진 경우... 재기동
*/
int check_HOST_CommState()
{
    int     i;
    int     onlineSum;
    int     runSum;
    HOST_DCB *host;
    
    onlineSum = 0;
    runSum    = 0;
    
    /* -------------------------------------------  */
    /* 5개 HOST 구성정보와 통신상태 확인...         */
    /* -------------------------------------------  */
    for(i=0; i< MAX_HOST; i++)
    {
        host = (HOST_DCB *) hostDCB[i];  
        
        /* HOST 미지정시... */
        if(host->hostDualMode == HOST_NOT_USE)  continue;
            
        onlineSum += host->online[MASTER_PORT];
        onlineSum += host->online[SLAVE_PORT];
        runSum += host->hostDualMode;               // 0 : HOST 실행않함, 1 : 개별, 2 : 이중화
    }
    
    /* -------------------------------------------  */
    /* 5개 HOST 중 하나라도 정의되어 있는 경우...   */
    /* -------------------------------------------  */
    if((onlineSum == 0) && (runSum != 0))
    {
        if(opr->wdtDebug)
        Debug(console,"wdt> HOST Comm State... %d... count=%d\n", onlineSum, opr->hostComError);
        
        if(++opr->hostComError > 300)
        {
            return (-1);
        }
    }
    else
    {
        opr->hostComError = 0;
    }
    
    return (0);
    
}


/* ------------------------------------------------------- */
/*  HOST MAIN 프로그램 Start Routine ....             */
/* ------------------------------------------------------- */
int	main(int argc, char **argv)
{
    //int     i;
	int     hostid;
	int		oldsec=0;
	
	HOST_DCB *host;
	
    /* Signal Handler initial */
	signal( SIGBUS,  SigHandler);
	signal( SIGSEGV, SigHandler);
	signal( SIGTERM, SigHandler);	
	signal( SIGPIPE, SigHandler);

    hostid = atoi(argv[1]);
    
    
    
    
    printf("HOST> HOST-MAIN Call ... argc=%d, argv[]= %s %s, hostid=%d \n", argc, argv[0], argv[1], hostid);
    
    read_config(0);
    		
	// 작업 환경을 초기화한다
	termExec = InitEnv(hostid);
	
	/* 내부변수 초기화 */
    init_variable();

    pause(1000);


	/* ------------------------------------ */    
    /* CPU 링크상태 확인 후 HOST-TASK  기동 */
    /* ------------------------------------ */  
    while(opr->checkLinkOK == RESET)
    {
    	taskPtr->wdtCount = 0;
		pause(100);
    }
    
    pause(1000);
    taskPtr->wdtCount = 0;
    
    host = (HOST_DCB *) hostDCB[hostid];                 /* 주장치 #1 속성정의 */
    
    /* ------------------------------------ */
    /* 상위 호스트관련 변수 초기화          */
    /* ------------------------------------ */
	hostDCBInitial(hostid);
	
	/* ------------------------------------ */
    /* HOST채널 미정의... MMI 상 OFF 처리   */
    /* ------------------------------------ */
    opr->mmiHostSts[hostid] = 3;     // 사용않함.   
    opr->hostComSts[hostid] = 2;     // 미정의
    
    opr->hostRestart[hostid] = 0;

    /* ------------------------------------ */
    /*  HOST-THREAD 호출 ...                */
    /* ------------------------------------ */
    //retVal = hostTask_Call(hostid);

    /* ------------------------------------------------ */
    /* HOST Protocol 타입에 따른 분기 ...               */
    /*   - HARRIS 인경우    : HARRIS-Task 호출...       */
    /*   - DNP(TCPIP)인경우 : DNP-TCPIP-Task 호출...    */
    /*   - DNP(ASYNC)인경우 : DNP-Task 호출...          */
    /*   - 기타(MODBUS, IEC): 미정의 Task               */
    /* ------------------------------------------------ */
    if(host->hostDualMode != HOST_NOT_USE)
    {     
        if(host->hostProtocol == HOST_HARRIS)           hostHarrisThread(hostid);  // MODBUS-THREAD
        else if(host->hostProtocol == HOST_LANDIS)      hostLandisThread(hostid);  // MODBUS-THREAD    
        else if(host->hostProtocol == HOST_DNP)     
        {
            /* ---------------------------------------- */
            /* TCPIP 인 경우 : 각각의 SERVER 프로그램   */
            /* ---------------------------------------- */
            if(host->hostComType == COM_TCPIP)          hostDnp_tcpip_Thread(hostid);      // RS232 : DNP-HOST 실행
            else                                        hostDnpThread(hostid);     // RS232 : DNP-HOST 실행
        }
        else
        {
            while(termExec)
        	{
        	    taskPtr->wdtCount = 0;
	        	pause(100);

                if(oldsec == rtc->sec)  continue;
	        	oldsec = rtc->sec;
		
		        /* ------------------------------------ */
                /* HOST-DB 변경에 따른 재기동 ...       */
                /* ------------------------------------ */
                if(opr->hostRestart[hostid] == SET)
                {
                    Debug(console,"host%02d> *** HOST TASK Restart... DB Chg ...\n", hostid+1);
                    pause(1000);
                    termExec = RESET;
                }
        
                if(opr->hostDebug == hostid)
                Debug(console,"host%02d> *Invalid PROTOCOL-Type HOST ...! \n", hostid+1 );
            }
        }
    }    
    else  
    {
        while(termExec)
    	{
    	    taskPtr->wdtCount = 0;
	    	pause(100);

            if(oldsec == rtc->sec)  continue;
	    	oldsec = rtc->sec;
		
		    /* ------------------------------------ */
            /* HOST-DB 변경에 따른 재기동 ...       */
            /* ------------------------------------ */
            if(opr->hostRestart[hostid] == SET)
            {
                Debug(console,"host%02d> *** HOST TASK Restart... DB Chg ...\n", hostid+1);
                pause(1000);
                termExec = RESET;
            }
                
            if(opr->hostDebug == hostid)
            Debug(console,"host%02d> *HOST NO-Define ...! \n", hostid+1 );
        }
    }
    
	// 작업 환경을 정리한다
	ClearEnv(hostid);
	
	return (0);
}
