
#include	"localLib.h"
#include    "external.h"




extern  int     storeAI_DNP(int devid, int point, float fdata, byte *lowdata);
extern  int     storeSOE_CU_DNP( int devid, int devPoint, int state, byte *buf);
extern  int     storeCOS_CU_DNP( int devid, int point, int state);
extern  int     deviceComUpdate();
extern  int     systemStsUpdate();

#define TARGET_NAME get_target_name()

TTY_DESC scanChan[MAX_CPUPORT];

TTY_DESC scanTunnel	 = {0, 0, NULL, ASYNC_860T, COM_RS232, 38400,   8,  1, 0, 'N',    FID_SCAN4_PORT,   {0, 50000}}; 


SHM_DESC	    shmDesc = {-1, SHM_KEY, sizeof(SHM_MEMORY), NULL};
SHM_MEMORY	    *shmPtr = NULL;
int             termExec;

TASK_INFO	    *taskPtr= NULL;

RTC             *rtc    = NULL;
OPR_MSG         *opr    = NULL;
RTU_DATABASE    *rtudb  = NULL;

SYSTEM_QUE      *sque   = NULL;         // MMI 용 이벤트
HISTORY_QUE     *hque   = NULL;         // CONSOLE 용 이벤트
//MMI_COA_QUE     *mmiCOA = NULL;             // MMI Analog-Event

CONSOLE_INFO	*console= NULL;

SCU_MSG         *scuCfg = NULL;
LINK_MSG		*linkCfg= NULL;

SCAN_CONFIG     *scanCfg[MAX_SCAN_PORT];         // 하위계전기 SCAN Config
DNP_DEVICE      *dnpcfg[MAX_MODULE];
POINT_BUF       *devPtBuf[MAX_DEV_POINT];
	
HOST_DCB        *hostDCB[MAX_HOST];                 /* HARRIS, LANDIS, SICS HOST Structure */

LANDIS_CFG      *landis;                            /* LANDIS HOST Structure */
HOST_STSBUF     *hostStsBuf[MAX_HOST_STS_PT];       /* LANDIS-HOST defined Status Point */
HOST_ANABUF     *hostAnaBuf[MAX_HOST_ANA_PT];       /* LANDIS-HOST defined Analog Point */


PDPDTIME_DATE_TIME      dnpRTC;
PDPDTIME_MS_SINCE_70    dnpRTCInfo;

/* ---------------------------------------- */
/*  THREAD 관련 변수 정의                   */
/* ---------------------------------------- */
void    *scan_dnp_thread(int *arg);                     // SCAN - DNP TASK
void    *scan_dnp_thread_TCP(int *arg);                 // SCAN - DNP TASK

void    *scan_modbus_thread(int *arg);                  // SCAN - MODBUS TASK
void    *scan_modbus_thread_TCP(int *arg);              // SCAN - MODBUS-TCPIP TASK
void    *scan_iom_thread(int  *arg);                    // SCAN - VME BUS TASK
void    *scan_locator_thread(int *arg);                 // SCAN - LOCATOR TASK


THREAD_ENTRY    scanTHREAD[MAX_SCAN_PORT];              // SCAN 구조체 TASK

THREAD_ENTRY    MOD_THREAD[MAX_MODULE];                 // MODULE 구조체 TASK

int     ThreadActive[MAX_MODULE];


/* ---------------------------------------- */
/*  VMEBUS 관련 변수 초기화                 */
/* ---------------------------------------- */
void    *siobus_ptr ;                                   // SIO 모듈 Base Addr
void    *iombus_ptr ;                                   // Function 모듈 Base Addr
int     vme_fd; 

VME_SIODCB      *vmeSioDCB;
VME_CHAN_DCB    *vmeChanDCB[SIO_CHANNEL_MAX];

VME_SIODCB      tempVmeSioDCB;
VME_CHAN_DCB    tempVmeChanDCB[SIO_CHANNEL_MAX];

/* 내부 기능모듈 */
VME_MODULE      *vmeModule[MAX_MODULE];
VME_MODULE      tempVmeModule[MAX_MODULE];

word    soeTimer;

//static  byte    rxbuf[256];

/*
*   VME Channel WRITE :
*/
int vmeScanChanReset(int devid)
{
    DNP_DEVICE  *dnp;
    VME_CHAN_DCB    *vmeChan;
    
    /* ---------------------------------------------------- */
	/*	CPU 이중화 : LOCAL-SLAVE ... 대기모드 				*/
	/* ---------------------------------------------------- */
	if(opr->runMode == LOCAL_SLAVE)	return (0);  
		
    dnp      = (DNP_DEVICE *) dnpcfg[devid];
    vmeChan = dnp->vmeChan;
    
    vmeChan->initReq = 0x1234;
    //Debug(console,"\n ===> VME Channel[%2d] Re-Initial...!\n", devid);
    return (0); 
}

/*
*/
int dnpChanWrite(int fid, int devid, byte *sndbuf, int sndCount)
{
    int     comPort;
    DNP_DEVICE  *dnp;
    SCAN_CONFIG	*scan;
    
    

    scan = (SCAN_CONFIG *) scanCfg[fid];
    comPort = scan->comPort;
		
    dnp      = (DNP_DEVICE *) dnpcfg[devid];
    
    /* ------------------------------------------------------- */
    /* CNA-TECH : fid 0,1 - CPU Channel, fid 2,3 - VME Channel */
    /* ------------------------------------------------------- */
    if(comPort < MAX_CPUPORT)		PortWrite(dnp->ttyPort, sndbuf, sndCount);
    else if((comPort >= VME_COM1) && (comPort <= VME_COM8))
    {
        /* ---------------------------------------------------- */
    	/*	CPU 이중화 : LOCAL-SLAVE ... 대기모드 				*/
	    /* ---------------------------------------------------- */
	    if(opr->runMode == LOCAL_SLAVE)	return (0);  
	    
    	vmeChanWrite(fid, vmeSioDCB, dnp->vmeChan, sndbuf, sndCount);
    }

    return (sndCount);        
}

/*
*/
int dnpChanRead(int fid, int devid, byte *rxbuf, int reqCount)
{
    int rcvCount=0;
	int	comPort;
    DNP_DEVICE  *dnp;
    SCAN_CONFIG	*scan;
    
 
    scan = (SCAN_CONFIG *) scanCfg[fid];
    comPort = scan->comPort;

    dnp      = (DNP_DEVICE *) dnpcfg[devid];

    /* ------------------------------------------------------- */
    /* CNA-TECH : fid 0,1 - CPU Channel, fid 2,3 - VME Channel */
    /* ------------------------------------------------------- */
    if(comPort < MAX_CPUPORT)       rcvCount = asyncRead( opr, dnp->ttyPort, rxbuf, reqCount);
    else if((comPort >= VME_COM1) && (comPort <= VME_COM8))
    {
        /* ---------------------------------------------------- */
	    /*	CPU 이중화 : LOCAL-SLAVE ... 대기모드 				*/
	    /* ---------------------------------------------------- */
	    if(opr->runMode == LOCAL_SLAVE)	return (0);  
	        
    	rcvCount = vmeChanRead(fid, vmeSioDCB, dnp->vmeChan, rxbuf, reqCount);    
    }
    
    return (rcvCount);
    
}

/*
*   VME Channel READ :
*/
int scanVMERead(int fid, int devid, byte *rxbuf)
{
    int     i;
    int     rcvCount;
	//int	    size,rcount;
	int	    timeout = 0;
	//int	    retValue, rxCount;

    DNP_DEVICE      *dnp;
    VME_CHAN_DCB    *vmeChan;
    
    dnp      = (DNP_DEVICE *) dnpcfg[devid];
    vmeChan = (VME_CHAN_DCB *) dnp->vmeChan;
    
    while(vmeSioDCB->mpuAccess + vmeSioDCB->sioAccess)  
    {
        usleep(1000);
        if(++timeout > 20)  
        {
            //Debug(console,"vme%02d> read ready timeout...\n", fid);
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
    
    Debug(console,"scan> ... signal generated (%2d)...!\n", sig);
    pause(100);
    
	switch(sig)
	{
    case SIGTERM:
        termExec = 0;
        //if(opr->wdtDebug)   
        Debug(console,"scan> ... signal [SIGTERM] generated (%2d)...!\n", sig);
        
        /* ------------------------------------------------ */
        /* LOG File 저장                                    */
        /* ------------------------------------------------ */
        sprintf(buffer, "scan> ... signal [SIGTERM] generated (%2d)...!\n", sig);
        LogFile_MPU ( opr, NOT_DEFINED, buffer, strlen(buffer));
        
        pause(1000);     
        break;
	        
	case SIGBUS : 
	    termExec = 0;
        //if(opr->wdtDebug)   
        Debug(console,"scan> ... signal [SIGBUS] generated (%2d)...!\n", sig);
        
        /* ------------------------------------------------ */
        /* LOG File 저장                                    */
        /* ------------------------------------------------ */
        sprintf(buffer, "scan> ... signal [SIGBUS] generated (%2d)...!\n", sig);
        LogFile_MPU ( opr, NOT_DEFINED, buffer, strlen(buffer));
        
        pause(1000);     	    
	    break;
		    
	case SIGSEGV: 
	    termExec = 0;
        //if(opr->wdtDebug)   
        Debug(console,"scan> ... signal [SIGSEGV] generated (%2d)...!\n", sig);
        
        /* ------------------------------------------------ */
        /* LOG File 저장                                    */
        /* ------------------------------------------------ */
        sprintf(buffer, "scan> ... signal [SIGSEGV] generated (%2d)...!\n", sig);
        LogFile_MPU ( opr, NOT_DEFINED, buffer, strlen(buffer));
        
        pause(1000);     	    
	    break;
		    
	case SIGPIPE: 
        //if(opr->wdtDebug)   
        Debug(console,"scan> ... signal [SIGPIPE] generated (%2d)...!\n", sig);
        
        /* ------------------------------------------------ */
        /* LOG File 저장                                    */
        /* ------------------------------------------------ */
        sprintf(buffer, "scan> ... signal [SIGPIPE] generated (%2d)...!\n", sig);
        LogFile_MPU ( opr, NOT_DEFINED, buffer, strlen(buffer));
        
        pause(1000);     	    
		break;

	default : 
	    //if(opr->wdtDebug)	
	    Debug(console,"scan> ... signal[%2d] generated ...!\n", sig);
	    /* ------------------------------------------------ */
        /* LOG File 저장                                    */
        /* ------------------------------------------------ */
        sprintf(buffer, "scan> ... signal[%2d] generated ...!\n", sig);
        LogFile_MPU ( opr, NOT_DEFINED, buffer, strlen(buffer));
        
		break;
	}
	
}



//
// 모듈:	ClearEnvironment()
//
void ClearEnv(void)
{
    int i;
    
    for(i=0; i< MAX_CPUPORT; i++)     PortClose(&scanChan[i]);
    
    printf("SCAN unmmapping...\r\n");
    munmap( siobus_ptr, VMESIO_SIZE);
    munmap( iombus_ptr, VMEIOM_SIZE);    
    
    close(vme_fd);

	// 프로세스 정보를 정리한다
	UpdateProcessInfo(taskPtr, 0, getpid(), 0);
	
    
	// 공유 메모리 설정 해제
	if (shmPtr != NULL)
		ShmDetach(&shmDesc);


			
}
					    

/*
*   VMEBUS : SIO 관련 메모리 초기화...
*/
int  init_vmebus(void)
{
    
    /* for TEST.... */
    //return (NOK);
    
    /* ------------------------------------ */
    /*  VMEBUS MEMORY DEVICE 초기화         */
    /* ------------------------------------ */
    if ((vme_fd = open("/dev/mem", O_RDWR) ) < 0)
    {
        printf("scan> *open VME_AREA : /dev/mem ... Error ! \n");
        return (NOK);
    }        

    //printf("host> open VME_AREA : %p ... OK ! \n",VME_BASE_ADDRESS );

    /* -------------------------------------------- */
    /*  VME Backup Memory 초기화                    */
    /*  - SIO Module Size : 0x20000                 */
    /* -------------------------------------------- */
    if((siobus_ptr = ( char  *) mmap(0, VMESIO_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, vme_fd, (VMESIO_START_ADDRESS)) ) < 0)
    {            
        printf("scan> *VME Memory Mapping (SIO) ... Error ! \n");
        return (NOK);            
    }

    /* ---------------------------------------------------- */
    /*  VME Backup Memory 초기화                            */
    /*  - IOMODULE Size : 0x1000                            */
    /* VMEIOM_START_ADDRESS    0x20040000                   */    
    /* MODULE_OFFSET           0x1000                       */      
    /* VMEIOM_SIZE             (MODULE_OFFSET * MAX_MODULE) */   
    /* ---------------------------------------------------- */
    if((iombus_ptr = ( char  *) mmap(0, VMEIOM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, vme_fd, (VMEIOM_START_ADDRESS)) ) < 0)
    {            
        printf("scan> *VME Memory Mapping (IO-MODULE) ... Error ! \n");
        return (NOK);            
    }
    
    //printf("host> MAP VME_AREA : %p ... OK ! \n",siobus_ptr );
    
    return (OK);
}    

/*
*/
int	init_variable()
{
    int i;
    //word    chksum;
    int     size1, size2;

        /* -------------------------------- */
    /* 1. 외부 메모리 포인트 초기화 ... */
    /* -------------------------------- */
    if(init_vmebus() == OK)
    {
	    /* ---------------------------------------- */
        /* SIO보드 초기화...VME 포인터 초기화       */
        /* ---------------------------------------- */             
	    vmeSioDCB  = (VME_SIODCB *)(  siobus_ptr);   
	    size1 = sizeof(VME_SIODCB);
	    //printf("host> VME SIO  Address : %p, size=%d\n", vmeSioDCB, size1);  

        vmeSioDCB->sioAccess = 0;
        vmeSioDCB->mpuAccess = 0;
        vmeSioDCB->initReq   = 0;   
        vmeSioDCB->initEnd   = 0;    
        vmeSioDCB->resetFlag = 0;          	    
        
        /* VMESIO 통신보드 - 채널구성 정보 */
        for(i=0; i< SIO_CHANNEL_MAX; i++)
        {
            size2 = sizeof(VME_CHAN_DCB);
            vmeChanDCB[i] = (VME_CHAN_DCB *) (siobus_ptr + size1 + (i*size2));   	    
	        //printf("host> VME CHAN(%d) Address : %p, size=%d\n", i, vmeChanDCB[i], size2);  
        }	       
        
        /* ---------------------------------------- */
        /* VME IOM 기능모듈부 정보                  */
        /* ---------------------------------------- */
        for(i=0; i< MAX_MODULE; i++)
        {
            vmeModule[i] = (VME_MODULE *) (iombus_ptr + (i*MODULE_OFFSET));   	    
	        //printf("host> VME Module(%d) Address : %p, size=%d\n", i, vmeModule[i], MODULE_OFFSET);  
        }	 
        
        printf("\n-----------------------------------------------------\n");
	    printf("SCAN> VMEBUS(SIO/IOM) Mapping ... OK   sio=%p, iom=%p \n", siobus_ptr, iombus_ptr); 
	    printf("-----------------------------------------------------\n");     
    }
    else
    {
        printf("\n-----------------------------------------------------\n");
	    printf("scan> **** VMEBUS(SIO/IOM) Mapping Error...Error \n"); 
	    printf("-----------------------------------------------------\n");

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
        
        /* VME IOM 기능모듈부 정보 */
        for(i=0; i< MAX_MODULE; i++)
        {
            vmeModule[i] = (VME_MODULE *) &tempVmeModule[i];
        }	   
    }

    /* -------------------------------------------- */
    /*  하위계전기용 통신포트 초기화 (Default)      */
    /* -------------------------------------------- */
    for(i=0; i< MAX_CPUPORT; i++)
    {
        scanChan[i].chid     = i;
        scanChan[i].id       = 0;
        scanChan[i].fId      = NULL;
        scanChan[i].config   = ASYNC_860T;
        scanChan[i].commMode = COM_RS485;
        scanChan[i].baudRate = 38400;
        scanChan[i].dataBits = 8;
        scanChan[i].stopBits = 1;
        scanChan[i].lineDebug= 0;
        scanChan[i].parity   = 'N';
        
        scanChan[i].timeOut.tv_sec = 0;
        scanChan[i].timeOut.tv_usec = 50000;
    }

/* ---------------------------------------------------- */
/*  RTU-TYPE : ACE-LBS-4U... ASYNC  PORT Comm Parameter */
/* ---------------------------------------------------- */
#ifdef  ACE_LBS_4U
    sprintf(scanChan[0].name, "%s", FID_SCAN1_PORT);
    sprintf(scanChan[1].name, "%s", FID_SCAN2_PORT);
    sprintf(scanChan[2].name, "%s", FID_SCAN3_PORT);
    sprintf(scanChan[3].name, "%s", FID_SCAN4_PORT);
#else

#ifdef  ACE_LBS_6U
    /* ------------------------------------------------ */
    /*  LBS-6U HOST 통신용 Serial 포트 매칭...          */
    /* ------------------------------------------------ */
    sprintf(scanChan[0].name, "%s", FID_RFU_PORT);
    sprintf(scanChan[1].name, "%s", FID_MMI_PORT);
    sprintf(scanChan[2].name, "%s", FID_SCAN1_PORT);
    sprintf(scanChan[3].name, "%s", FID_SCAN2_PORT);
    sprintf(scanChan[4].name, "%s", FID_SCAN3_PORT);
    sprintf(scanChan[5].name, "%s", FID_SCAN4_PORT);
    
#else
    sprintf(scanChan[0].name, "%s", FID_SCAN1_PORT);
    sprintf(scanChan[1].name, "%s", FID_SCAN2_PORT);
    sprintf(scanChan[2].name, "%s", FID_SCAN3_PORT);
    sprintf(scanChan[3].name, "%s", FID_SCAN4_PORT);
    sprintf(scanChan[4].name, "%s", FID_RFU_PORT);
    
    sprintf(scanChan[5].name, "%s", FID_MMI_PORT);
#endif
    
#endif
        
            
    return (0); 
}

/*
*   FUNCTION : vmeScanInitial()
*   - 계전기 장치 구조체 ... 초기화
*/
void vmeScanInitial(int scanid, int chid)
{
    //int i, j;
    
    VME_CHAN_DCB    *vmeChan;
    SCAN_CONFIG     *scan;

    /* SCAN-TASK 별 모듈구성 정보 */ 
    scan = (SCAN_CONFIG *) scanCfg[scanid];
    vmeChan = (VME_CHAN_DCB *) vmeChanDCB[chid];
    
    vmeChan->rxFront = 0;
    vmeChan->rxRear  = 0;
    vmeChan->rxCount = 0;
    vmeChan->rxFlag  = 0;        
        
    vmeChan->txCount = 0;
    vmeChan->txFlag  = 0;
    
    if(scan->protocol == PROTOCOL_DNP)
    {
        vmeChan->protocolType = SCAN_DNP;        // HOST 별 프로토콜 타입 지정 
        vmeChan->cfgParity    = PARITY_NONE;
        vmeChan->cfgData      = DATA_8_BIT;
        vmeChan->cfgStopbit   = STOP_1_BIT;
    }
    else if(scan->protocol == PROTOCOL_MODBUS)	
    {
        vmeChan->protocolType = SCAN_MODBUS;        // HOST 별 프로토콜 타입 지정 
        vmeChan->cfgParity    = PARITY_NONE;
        vmeChan->cfgData      = DATA_8_BIT;
        vmeChan->cfgStopbit   = STOP_1_BIT;
    }
    else
    {
        vmeChan->protocolType = SCAN_ASYNC;        // HOST 별 프로토콜 타입 지정 
        vmeChan->cfgParity    = PARITY_NONE;
        vmeChan->cfgData      = DATA_8_BIT;
        vmeChan->cfgStopbit   = STOP_1_BIT;
    }
    		    
    vmeChan->cfgType      = scan->comMode;
    vmeChan->cfgType      = COM_RS232;
    
    vmeChan->cfgSpeed     = scan->comSpeed;
                        
}



//
// 모듈:	InitEnvironment()
//
int InitEnv( void)
{
    int  i;
    
	// 공유 메모리 상태 확인
	if (ShmCheck(&shmDesc) < 0)
	{
		printf("scan> *ERR_공유 메모리 상태 이상\n");
		return(0);
	}
	
	//printf("scan> SHM %p \n", shmDesc.address);
	
	// 포인터 변수 초기화
	shmPtr = (SHM_MEMORY *) shmDesc.address;
	taskPtr = (TASK_INFO *) &shmPtr->taskInfo[SCAN_PROCESS];

	rtc     = (RTC *)       &shmPtr->rtc;
	opr     = (OPR_MSG *)   &shmPtr->opr_msg;
	
	rtudb   = (RTU_DATABASE *) &shmPtr->rtuDatabase;
	sque    = (SYSTEM_QUE *)  &shmPtr->localEvent;          // 시스템 이벤트
	hque    = (HISTORY_QUE *)  &shmPtr->localHistoryQ;           // 상태 이벤트
	//mmiCOA  = (MMI_COA_QUE *) &shmPtr->mmiCOA;
	
    scuCfg 	= (SCU_MSG *)   &shmPtr->scu_msg;
    linkCfg = (LINK_MSG *)  &shmPtr->link_msg;
    
    console = (CONSOLE_INFO *) &shmPtr->console; 
        
    /* ------------------------------------ */
    /* DNP_DEVICE : 전자식 배전반 구조체    */
    /* ------------------------------------ */
    for(i=0; i< MAX_SCAN_PORT; i++)     scanCfg[i]  = (SCAN_CONFIG *) &shmPtr->scanCFG[i];
    for(i=0; i< MAX_MODULE; i++)    dnpcfg[i]   = (DNP_DEVICE *) &shmPtr->dnpCFG[i];
	for(i=0; i< MAX_DEV_POINT; i++)     devPtBuf[i] = (POINT_BUF *) &shmPtr->devPtBuf[i];

    /* ------------------------------------ */
    /* HOST_DCB : 상위 HOST 구조체          */
    /* ------------------------------------ */
    for(i=0; i< MAX_HOST; i++)          hostDCB[i]  = (HOST_DCB *) &shmPtr->hostDCB[i];
    
    landis = (LANDIS_CFG *) &shmPtr->landis;                    /* LANDIS HOST Structure */
    for(i=0; i< MAX_HOST_STS_PT; i++)   hostStsBuf[i] = (HOST_STSBUF *) &shmPtr->hostStsBuf[i];
    for(i=0; i< MAX_HOST_ANA_PT; i++)   hostAnaBuf[i] = (HOST_ANABUF *) &shmPtr->hostAnaBuf[i];
	
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
* ==================================================
*   SCAN-TASK 
* ==================================================
*/
int scanTask_Call()
{
    int     i;      
    int	    state;
    int     index;
    SCAN_CONFIG *scan;
    
    /* ------------------------------------ */
    /*  SCAN THREAD 초기화 및 실행          */ 
    /* ------------------------------------ */       
    for(i=0; i< MAX_SCAN_PORT; i++)
    {
        scan = (SCAN_CONFIG *) scanCfg[i];
        
        // offline 처리하는 timing의 최소. 최대 지정 
        if(scan->offCount <= 2)		scan->offCount = 2;
    	if(scan->offCount > 100)	scan->offCount = 100;	
    		
    	// scan delay 초소 초대 지정 
    	if(scan->comDelay < 1)		scan->comDelay = 1;
    	if(scan->comDelay > 250)	scan->comDelay = 250;		
    	    
        scanTHREAD[i].tid        = 0;
        sprintf(scanTHREAD[i].name, "%s%d","SCAN", i+1);
        
        scanTHREAD[i].priority   = 0;
        scanTHREAD[i].options    = 0;       // 비실행 Flag
    
        /* ------------------------------------ */
		/* 	ACTIVE 상태에 따라서 Thread 실행 	*/
		/* ------------------------------------ */
		if(scan->active == RESET)	continue;  // DB 상에서  scan 사용하지 않는경우

        if(scan->comMode == COM_TCPIP)
        {
            if(scan->protocol == PROTOCOL_DNP)          scanTHREAD[i].entryPt = scan_dnp_thread_TCP;     		// TCPIP 방식 DNP-THREAD
            else if(scan->protocol == PROTOCOL_MODBUS)	scanTHREAD[i].entryPt = scan_modbus_thread_TCP;  		// TCPIP 방식 MODBUS-THREAD			
            else                                        scanTHREAD[i].entryPt = scan_modbus_thread_TCP;         // 계전기 DNP-THREAD			
        }
        else // serial
        {
          	if(scan->protocol == PROTOCOL_DNP)              scanTHREAD[i].entryPt = scan_dnp_thread;     		// 계전기 DNP-THREAD
    		else if(scan->protocol == PROTOCOL_MODBUS)	    scanTHREAD[i].entryPt = scan_modbus_thread;  		// 터널제어기 MODBUS-THREAD			    
	        else if(scan->protocol == PROTOCOL_BUS)		    scanTHREAD[i].entryPt = scan_iom_thread;  			// 계전기 IOMODULE-THREAD	    
	        else if(scan->protocol == PROTOCOL_LOCATOR1)	scanTHREAD[i].entryPt = scan_locator_thread;		// 계전기 IOMODULE-THREAD	        
   		    else                                       	    scanTHREAD[i].entryPt = scan_dnp_thread;     		// 계전기 DNP-THREAD			
        }    
    			    
        scanTHREAD[i].options    = SET;	
	    scanTHREAD[i].arg        = 0;
    
      	index = i;      // Thread Index #1
        state = pthread_create( &scanTHREAD[i].tid, NULL, scanTHREAD[i].entryPt, &index);
       	if(state != 0)
        {
	        printf("scan> *Thread# [%s] Create Error ...!\n", scanTHREAD[i].name);
	        // termExec가 global 변수로  이 함수 다음에 실행되는 while에서  scanMain이 종료된다.
    	    termExec = RESET;
            //exit(0);
        }
	    pause(100);
    }

    return (0); 
}

#if 0
/*
*   사용자 데이터 입력 시험 : ANALOG
*/
int testAnalogDATA( )
{
    int     i;
    //int     ioid;
    //byte    data[8];
    int     adata;
    //int     count;
    //float   *fptr1, *fptr2;
    float   fdata;
    //long    *lptr1;
    //int     sign, exp, mant;
    int     point;
                    
    DNP_DEVICE      *dnp;
    POINT_BUF       *aiPoint;

    Debug(console,"==> Test Analog Data Update....[%2d] !\n", rtc->sec);
    
    adata = 0;
    for(i=0; i< MAX_MODULE; i++)
    {        
        dnp = (DNP_DEVICE *) dnpcfg[i];
        
        for(point = 0; point < dnp->devAiPoint; point++)
        {
            aiPoint = (POINT_BUF *) &dnp->aiPtBuf[point];
            if(aiPoint->config == RESET)  continue;

            fdata = (float ) adata;
            
            /* ------------------------------------ */
            /* Analog Data Assign into Buffer ...   */
            /* ------------------------------------ */
            storeAI_DNP( i, point, fdata, (byte *)&fdata);
            adata += 10;
        }
    }
    return (0); 
}
#endif

/*
*   사용자 데이터 입력 시험 : ANALOG
*/
int testAnalogDATA( )
{
	int     devid, point, state;
    float   fdata;
                    
    DNP_DEVICE      *dnp;
    POINT_BUF       *aiPoint;

	devid = opr->testDevid;
    point = opr->testPoint;
    state = opr->testState;
    
    opr->testAnalogFLAG = RESET;
    
    Debug(console,"==> Test Analog Data Update....dev=%2d, point=%2d, data = %d !\n", devid+1, point+1, state);
    
    dnp = (DNP_DEVICE *) dnpcfg[devid];
    aiPoint = (POINT_BUF *) &dnp->aiPtBuf[point];
    if(aiPoint->config == RESET)  
    {
    	Debug(console,"*** Not Define Point ... return\n");
    	return (0);
    }
    
    fdata = (float ) state;

    /* ------------------------------------ */
    /* Analog Data Assign into Buffer ...   */
    /* ------------------------------------ */
    storeAI_DNP( devid, point, fdata, (byte *)&fdata);
            
    return (0); 
}

/*
*   사용자 데이터 입력 시험 : ANALOG
*/
int testAnalogCOA( int testData)
{
    int     i;
    //int     ioid;
    //byte    data[8];
    //int     adata;
    //int     count;
    //float   *fptr1;
    float   fdata;
    //long    *lptr1;
    //int     sign, exp, mant;
    int     point;
                    
    DNP_DEVICE      *dnp;
    POINT_BUF       *aiPoint;

    //Debug(console,"==> Test Analog Data Update....[%2d] !\n", testData);

    for(i=0; i< MAX_MODULE; i++)
    {        
        dnp = (DNP_DEVICE *) dnpcfg[i];
        
        for(point = 0; point < dnp->devAiPoint; point++)
        {
            aiPoint = (POINT_BUF *) &dnp->aiPtBuf[point];
            if(aiPoint->config == RESET)  continue;

            fdata = (float ) testData;
            
            /* ------------------------------------ */
            /* Analog Data Assign into Buffer ...   */
            /* ------------------------------------ */
            storeAI_DNP( i, point, fdata, (byte *) &fdata);
        }
    }
    return (0); 
}


/*
*   사용자 데이터 입력 시험 : STATUS
*/
int testStatusDATA()
{
    int     i;
    int     devid, point, state;
    //byte    cosState;
    byte    soeBuffer[64];
    int     millisecond;
    
    struct timeval rv; 
    time_t  tm;
    struct  tm  localtm;
    DNP_DEVICE  *dnp;
    
    devid = opr->testDevid;
    point = opr->testPoint;
    state = opr->testState;
    
    opr->testStatusFLAG = RESET;
    
    dnp = (DNP_DEVICE *) dnpcfg[devid];
    dnp->initial = STATUS_DUMP_TICK;

    //if(state == 1)  state = 0x80;
    //else            state = 0x40;    
    
    for(i=0; i < 1; i++)
    { 
        /* ---------------------------------- */
        /* 현재시간 정보를 DNP Form 으로 변환 */
        /* ---------------------------------- */
        time(&tm);
        //localtm = localtime(&tm);
        localtime_r (&tm, &localtm);
        
        gettimeofday(&rv, NULL);
        millisecond = (rv.tv_usec / 1000) % 1000;

        dnpRTC.year       = localtm.tm_year + 1900;
        dnpRTC.month      = localtm.tm_mon + 1;
        dnpRTC.day        = localtm.tm_mday;
        dnpRTC.hour       = localtm.tm_hour;
        dnpRTC.minute     = localtm.tm_min;
        dnpRTC.second     = localtm.tm_sec;
        dnpRTC.millisecond= millisecond;

        makeDnpTimeByte( &dnpRTCInfo, &dnpRTC);
    
        //if((i & 0x01) == 0)  state = 1;
        //else                 state = 0;
                 
        soeBuffer[0] = point;
        soeBuffer[1] = state;
        
        soeBuffer[2] = (dnpRTCInfo.leastSignificant) & 0xff;
        soeBuffer[3] = (dnpRTCInfo.leastSignificant >> 8) & 0xff;
        soeBuffer[4] = (dnpRTCInfo.mostSignificant) & 0xff;
        soeBuffer[5] = (dnpRTCInfo.mostSignificant >> 8)  & 0xff;
        soeBuffer[6] = (dnpRTCInfo.mostSignificant >> 16) & 0xff;
        soeBuffer[7] = (dnpRTCInfo.mostSignificant >> 24) & 0xff;

        pause(100);

        /* -------------------------------- */
        /*  SOE 정보 저장 (for TEST)        */
        /* -------------------------------- */
        storeSOE_CU_DNP(devid, point, state, &soeBuffer[2]); 
    }
                   
    /* -------------------------------- */
    /*  COS 정보 저장 (for TEST)        */
    /* -------------------------------- */
    //cosState = (state >> 7) & 0x01;
    //storeCOS_CU_DNP( devid, point, state);
    
    return (0); 
}                                    
#define __THIS_FILE__   "scanMain"    
#define CHECK  printf("%s:%d\r\n",__THIS_FILE__,__LINE__);        

/* ------------------------------------------------------- */
/*  HOST MAIN 프로그램 Start Routine ....             */
/* ------------------------------------------------------- */
int	main(int argc, char **argv)
{
    int     i;
    byte    comUpdateTick;
    int     oldsec=0;
    int     stsDumpPeriod;
	int     anaDumpPeriod;
	
	int     testAnalog, testState;
	byte    stsRunTick, anaRunTick;
    char    buffer[128];
      	
    DNP_DEVICE  *dnp;
    HOST_DCB *host;

	read_config(0);
	/* ---------------------------------------- */
	/* 작업 환경을 초기화한다                   */
	/* ---------------------------------------- */
	termExec = InitEnv();

 

	init_variable();
	
	taskPtr->wdtCount = 0;
		
	/* Signal Handler initial */
	/* 이 signal들 때문에 block 되어 있던 것들이 다 풀린다. EINTR */
	signal( SIGBUS,  SigHandler);
	signal( SIGSEGV, SigHandler);
	signal( SIGTERM, SigHandler);	
	//signal( SIGPIPE, SigHandler);


    pause(100);
    
    /* ----------------------------- */    
    /* CPU 링크상태 확인 후 MMI 기동  ???*/
    /* LinkMaster와 LinkSlave에서  SET 됨..                        */
    /* 위 두 함수가 실행되려면  local_master 와 local_slave가 결정되 뒤이다..*/
    /* 즉 Master와 Slave가 결정될 때 까지 기다린다                       */
    /* -----------------------------*/  
    while(opr->checkLinkOK == RESET)
    {
    	taskPtr->wdtCount = 0;
		pause(100);
    }
    
	/* ---------------------------------------- */
	/* 계전기별 통신 파라메터 지정                      */
	/* scan DB가 변경되면 scanResart = SET 된다      */
	/* ---------------------------------------- */
    opr->scanRestart = RESET;
 
     /* ---------------------------------------------------- */
	/*  MASTER/SLAVE 동작모드에 따른 계전기 Update          */
    /* ---------------------------------------------------- */
    for(i = 0; i < MAX_MODULE; i++)
    {
        dnp = (DNP_DEVICE *) dnpcfg[i];
        dnp->offlineTick= 0;
        //dnp->online		= 2;            // 초기상태 ... 미지정.  기본값이뭘까..
        dnp->initial		= RESET;    // 장치별 초기화 
        dnp->initDevStatus 	= RESET;	// 계전기 초기화 상태, host 응답지연과 관련있다.
        
        dnp->timeSyncReq = SET;
        dnp->statusDump = SET;
        dnp->analogDump = SET;
        
    }
       
    for(i=0; i< MAX_SCAN_PORT; i++)     opr->scanComSts[i] = 2;         // 미정의
    
    printf(" %s SCAN-MAIN PROCESS Activated ... !\n", TARGET_NAME);
  

    
    sprintf(buffer, "%s SCAN-MAIN PROCESS Activated ... !", TARGET_NAME);
    LogFile_MPU ( opr, NOT_DEFINED, buffer, strlen(buffer));
    
    /* ---------------------------------------- */
	/* 계전기별 통신 Thread 실행                */
	/* ---------------------------------------- */
    scanTask_Call();
          
    comUpdateTick = 0;

	stsDumpPeriod = opr->stsDumpPeriod;
    anaDumpPeriod = opr->anaDumpPeriod;
    stsRunTick = 0;
    anaRunTick = 0;
    testAnalog = 0;
    testState  = 0;

    opr->cpuChangeTick = 0;
    

	while(termExec)
	{

	    taskPtr->wdtCount = 0;
        pause(100);

        if(oldsec == rtc->sec)  continue;
        oldsec = rtc->sec;


        /* -------------------------------------------- */
	    /* 계전기 전체 통신상태 Check  .....            */
	    /* -------------------------------------------- */
        if(++comUpdateTick & 0x01) // 매 2초 마다.
        {
            /* LOCAL-MASTER 인경우... 디바이스 포인트 생성 */     
            if(opr->runMode == LOCAL_MASTER)
            {       
                deviceComUpdate();
                systemStsUpdate();
            }
        }

		/* ------------------------------------------------ */
        /*  DEVICE 별 상태 및 계측 Dump : 해당 Flag 설정           */
        /* ------------------------------------------------ */
        
        /* 매 초다마다 ++ 한다..결국 elapsed sec 값이 된다. */
        stsRunTick++;
        anaRunTick++;
        
        if(stsRunTick >= stsDumpPeriod)
        {
            stsRunTick = 0;
            for(i=0; i< MAX_MODULE; i++)
            {
                dnp = (DNP_DEVICE *) dnpcfg[i];
                
                if(dnp->devDiPoint)		dnp->statusDump = SET;
                else                    dnp->statusDump = RESET;
            }
        }

        if(anaRunTick >= anaDumpPeriod)
        {
            anaRunTick = 0;
            for(i=0; i< MAX_MODULE; i++)
            {
                dnp = (DNP_DEVICE *) dnpcfg[i];
                
                /* 아날로그 포인트가 없는경우... analogDump Clear...    */
                if(dnp->devAiPoint)		dnp->analogDump = SET;
                else                    dnp->analogDump = RESET;
            }
            
            /* ---------------------------------------- */
            /*  IEC-HOST 의 경우.,...                   */
            /* ---------------------------------------- */
            for(i=0; i< MAX_HOST; i++)
            {
                host = (HOST_DCB *) hostDCB[i];                 /* 주장치 #1 속성정의 */
                host->analogDump = SET;
            } 
            //Debug(console,"ana dump set...%d\n", oldsec);
        }
                                
        /* -------------------------------------------- */
	    /* SCAN DB Update 경우 ... 재기동  .....        */
	    /* -------------------------------------------- */
        if(opr->scanRestart == SET)
        {
            // 중요한 것들은 print를 추가하여 계속 출력해야 되지 않나..
            Debug(console,"wdt> *** System dB Updated...[SCAN] Restart !\n");

            termExec = 0;  // 여기서 다음에 continue 가 나와야 되는데..그냥 계속하네..
            pause(100);
        }
        
        /* -------------------------------------------- */
	    /* THREAD 감시 : Internal WDT Flag......        */
	    /* -------------------------------------------- */
	    for(i=0; i< MAX_SCAN_PORT; i++)
	    {
	        if(scanTHREAD[i].options == RESET)	continue;
            if(opr->scanWDT[i]++ > 50)
            {
                Debug(console,"****> SCAN-Thread #%d ... WDT Failed...!!!\n", i+1);
                pause(1000);
                
                termExec = 0;
            }   
        }

        /* ---------------------------------------- */
		/*	SLAVE 모드에서 VME Access 를 하지 않음  */
		/* ---------------------------------------- */
        if(opr->runMode == LOCAL_SLAVE)		
		{
			continue;
        }
        
        /* -------------------------------------------- */
	    /* 사용자 상태지정.. CONSOLE ......             */
	    /* -------------------------------------------- */        
        if(opr->testAnalogFLAG == SET)  testAnalogDATA();
        if(opr->testStatusFLAG == SET)  testStatusDATA();
            

	}/* while */

    Debug(console,"scan> *** SCAN-Thread Exit...!!!\n", i+1);
    
	// 작업 환경을 정리한다
	ClearEnv();
	
	return (0); 
}
