
#include	  "localLib.h"
#include    "external.h"




SHM_DESC	    shmDesc = {-1, SHM_KEY, sizeof(SHM_MEMORY), NULL};
SHM_MEMORY	    *shmPtr = NULL;

int             termExec;

TASK_INFO	*taskPtr= NULL;

RTC             *rtc    = NULL;
OPR_MSG         *opr    = NULL;
LINK_MSG		*linkCfg= NULL;

SYSTEM_QUE      *sque   = NULL;         // MMI 용 이벤트
HISTORY_QUE     *hque   = NULL;         // CONSOLE 용 이벤트
RTU_DATABASE    *rtudb  = NULL;

CONSOLE_INFO	*console= NULL;

HOST_DCB        *hostDCB[MAX_HOST];             /* HARRIS, LANDIS, SICS HOST Structure */
SCAN_CONFIG     *scanCfg[MAX_SCAN_PORT];         // 하위계전기 SCAN Config
DNP_DEVICE      *dnpcfg[MAX_MODULE];        // DNP DEVICE : 전자식배전반 

struct  sockaddr_in	srvAddr;
int     socketServerFd;         /* for Server */

int	    threadCount = 0;

extern	void	*simInfComm(void *data);

/* *************************************************************************************
*	FUNCTION : sigHandler()
* **************************************************************************************/
void	SigHandler( int sig )
{
    //if(opr->wdtDebug)
	Debug(console,"<sim> *****  ... signal generated (%2d)...!\n", sig);
    pause(100);
    
	switch(sig)
	{
	    case SIGTERM:
	        if(opr->wdtDebug)   Debug(console,"sim> *SIG TERM Error ... !\n", sig); 
            pause(1000);
	        termExec = 0;
	        break;
	        
		case SIGBUS : 
		    //if(opr->wdtDebug)   
		    Debug(console,"sim> *SIG BUS Error ... !\n", sig); 
            pause(1000);		        
            termExec = 0;
		    break;
		    
		case SIGSEGV: 
		    if(opr->wdtDebug)   Debug(console,"sim> *SIG SEGV Error ... !\n", sig); 
            pause(1000);
            termExec = 0;		        
		    break;
		    
		case SIGPIPE: 
		    if(opr->wdtDebug)   Debug(console,"sim> *SIG PIPE Error ... !\n", sig); 
            pause(1000);		        
			break;
			
		default : 
			if(opr->wdtDebug)   Debug(console,"<sim> (%d) ... signal generated ...!\n", sig);
			break;
	}

}


/*******************************************************************************
 *                                                                        
 * 모듈명:	_clearNetServer()
 *                                                                        
 ******************************************************************************/

void _clearNetServer(void)
{
	if (socketServerFd != ERROR) close(socketServerFd);
	Debug(console,"sim> ....NET 서버 종료 \n");
}


//
// 모듈:	ClearEnvironment()
//
void ClearEnv(void)
{
    /* -------------------------------- */
    /*  SERVER Socket Close ...         */
    /* -------------------------------- */
     _clearNetServer();
    
    /* -------------------------------- */
	/*  공유 메모리 설정 해제           */
	/* -------------------------------- */
	if (shmPtr != NULL)     ShmDetach(&shmDesc);

    /* -------------------------------- */
	/*  프로세스 정보를 정리한다        */
	/* -------------------------------- */
	UpdateProcessInfo(taskPtr, 0, getpid(), 0);
			
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
		printf("sim> *ERR_공유 메모리 상태 이상\n");
		return(0);
	}
	
	//printf("sim> SHM %p \n", shmDesc.address);
	
	// 포인터 변수 초기화
	shmPtr = (SHM_MEMORY *) shmDesc.address;
	taskPtr = (TASK_INFO *) &shmPtr->taskInfo[SIM_PROCESS];

	rtc     = (RTC *)       &shmPtr->rtc;  
	opr     = (OPR_MSG *)   &shmPtr->opr_msg;
	linkCfg = (LINK_MSG *)  &shmPtr->link_msg;
	
	rtudb   = (RTU_DATABASE *) &shmPtr->rtuDatabase;
	sque    = (SYSTEM_QUE *)  &shmPtr->localEvent;
	hque    = (HISTORY_QUE *)  &shmPtr->localHistoryQ; 
	
	console = (CONSOLE_INFO *) &shmPtr->console; 

    /* ------------------------------------ */
    /* HOST_DCB : 상위 HOST 구조체          */
    /* ------------------------------------ */
    for(i=0; i< MAX_HOST; i++)   hostDCB[i] = (HOST_DCB *) &shmPtr->hostDCB[i];
		
    /* ------------------------------------ */
    /* DNP_DEVICE : 전자식 배전반 구조체    */
    /* ------------------------------------ */
    for(i=0; i< MAX_SCAN_PORT; i++)     scanCfg[i] = (SCAN_CONFIG *) &shmPtr->scanCFG[i];
    for(i=0; i< MAX_MODULE; i++)    dnpcfg[i]  = (DNP_DEVICE *) &shmPtr->dnpCFG[i];

    /* ------------------------------------------------------------ */
	/* 프로세스 정보를 초기화한다                                   */
	/* SIM Task 는 TCPIP 전송대기를 고려하여 WDT 기능을 삭제함...  */
	/* ------------------------------------------------------------ */
	taskPtr->initial    = 1;
	taskPtr->wdtEnable  = 0;            
	
	UpdateProcessInfo(taskPtr, 1, getpid(), 0);
	
	return(1);
}




/*******************************************************************************
 *                                                                        
 * 모듈명:	_initNetpServer()
 *                                                                        
 ******************************************************************************/
int _initNetServerSIM(void)
{
    //int     on = 1;
	int		flag = 1, length;
    
    /* ------------------------------------------------------------ */
    /*  SERVER-소켓의 Open : IPv4 기반, 연결지향형 소켓, TCP기반    */
    /* ------------------------------------------------------------ */
	if ((socketServerFd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == ERROR)
	{
	    if(opr->simDebug)   Debug(console,"sim> ***[NET] socket 실패 \n");
		return(ERROR);
	}
	
	if(opr->simDebug)	Debug(console,"sim> socket Open OK ...%d \n", socketServerFd);

#if 0
    if(setsockopt(socketServerFd, SOL_SOCKET , SO_REUSEADDR, &on,sizeof(on))!= 0)
	{
    	if(opr->simDebug)   Debug(console,"sim> ***socket REUSEADDR error ...%d \n", on);
    }
    
    if(opr->simDebug)   Debug(console,"sim> socket RE-USEADDR OK ...%d \n", on);
#endif

    /* ------------------------------------------------ */
    /*  SOCKET Option...                                */
    /* ------------------------------------------------ */
    setsockopt(socketServerFd, SOL_SOCKET, SO_REUSEADDR, (char *)&flag, sizeof(int));
    setsockopt(socketServerFd, SOL_SOCKET, SO_KEEPALIVE, (char *)&flag, sizeof(int));
    //setsockopt(socketServerFd, SOL_SOCKET, SO_DONTROUTE, (char *)&flag, sizeof(int));
    //ioctl(socketServerFd, FIONBIO, (int)&flag);

    /* ------------------------------------------------ */    
    /*  소켓 구조체 초기화 ...                          */
    /* ------------------------------------------------ */    
    length = sizeof(struct sockaddr_in);
	bzero8248((byte *)&srvAddr, length);
	srvAddr.sin_family  = AF_INET;                      /* IPv4 기반의 인테넷 프로토콜 Family*/
	srvAddr.sin_port    = htons(SIMULATOR_NET_PORT);    /* Port   정보의 Network Byte Order 로 변경함. */
	srvAddr.sin_addr.s_addr = htonl(INADDR_ANY);        /* IP주소 정보의 Network Byte Order 로 변경함. */

    /* ------------------------------------------------ */    
    /*  소켓에 주소를 할당함...                         */
    /* ------------------------------------------------ */    
    length = sizeof(struct sockaddr_in);
	if (bind(socketServerFd, ( struct sockaddr *)&srvAddr, length) == ERROR)
	{
	    if(opr->simDebug) Debug(console,"sim> ***[NET] bind 실패 \n");
		return(ERROR);
	}
	if(opr->simDebug) Debug(console,"sim> [NET] bind OK \n");

    /* ------------------------------------------------ */    
    /*  연결요청 대기상태로 진입...                     */
    /* ------------------------------------------------ */    
	if (listen(socketServerFd, 5) == ERROR)
	{
	    if(opr->simDebug)	Debug(console,"sim> ***[NET] listen 실패\r\n");
		return(ERROR);
	}

    if(opr->simDebug)	Debug(console,"sim> [NET] listen OK \n");

	return(socketServerFd);
	
	
}

/* ------------------------------------------------------- */
/*  HOST MAIN 프로그램 Start Routine ....             */
/* ------------------------------------------------------- */
int	main(int argc, char **argv)
{
    //int     i,j;    
	int		client, length = sizeof(struct sockaddr_in);
	int		port;
	//int		initial;
	//byte	runtick;
	char	address[64];
	char	data[64];
	char    buffer[128];
	
	pthread_t	pThread;
	
	struct sockaddr_in	clientPtr;


	read_config(0);
	
	// 작업 환경을 초기화한다
	termExec = InitEnv();
	taskPtr->wdtCount = 0;
		
	/* Signal Handler initial */
	signal( SIGBUS,  SigHandler);
	signal( SIGSEGV, SigHandler);
	signal( SIGTERM, SigHandler);	
	signal( SIGPIPE, SigHandler);

    
    opr->simDebug   = 0;
	opr->simOnline  = 0;
	threadCount     = 0;
			
	pause(1000);
	
	/* ----------------------------- */    
    /* CPU 링크상태 확인 후 MMI 기동 */
    /* ----------------------------- */  
    while(opr->checkLinkOK == RESET)
    {
    	taskPtr->wdtCount = 0;
		pause(100);
    }
    	
    printf(" %s SIMULATOR-SERVER PROCESS Activated [socket=%d, port=%d]... !\n", TARGET_NAME, socketServerFd, SIMULATOR_NET_PORT);

    sprintf(buffer, "%s SIMULATOR-SERVER PROCESS Activated [socket=%d, port=%d]... !", TARGET_NAME, socketServerFd, SIMULATOR_NET_PORT);
    LogFile_MPU ( opr, NOT_DEFINED, buffer, strlen(buffer));
    
    /* ------------------------------------ */
    /* Network 초기화... 					*/
    /* ------------------------------------ */
    if (_initNetServerSIM() == ERROR) 
	{
	    printf("sim> *** SERVER Initial... Fail...\n");	
	    close(socketServerFd);
        termExec = NO;
	}
	    	
	while(termExec)
	{
	    taskPtr->wdtCount = 0;
		pause(100);

	    /* ---------------------------------------- */
	    /* 초기 연결 대기...                        */
	    /* ---------------------------------------- */
	    length = sizeof(struct sockaddr_in);
	    if ((client = accept(socketServerFd, ( struct sockaddr *)&clientPtr, (socklen_t *) &length)) != ERROR)
    	{
    		port = ntohs(clientPtr.sin_port);
    		sprintf(address, "%s", inet_ntoa(clientPtr.sin_addr));
    		
    		//if(opr->simDebug)   
    		Debug(console,"sim> accept O.K...%s... port = %d, socket = %d\n", address, port, client);

			memcpy(data, &client, sizeof(int));
			memcpy(&data[sizeof(int)], &clientPtr, length);
			sprintf(address, "%s", inet_ntoa(clientPtr.sin_addr));
			
			/* ------------------------------------ */
			/*	통신 쓰레드 생성 (PCS Interface...)	*/	
			/* ------------------------------------ */
			if (pthread_create(&pThread, NULL, &simInfComm, data) < 0)
			{
				Debug(console,"sim> *** Thread Create Fail");
				pause(1000);
				break;
			}
			
			pause(1000);
			//if(opr->simDebug)   
			Debug(console,"sim> accept Ready... \n");
    	}
    	else
    	{	
        	Debug(console,"sim> *** accept Fail...\n");		
		}
		
                            		
	}/* while */

	// 작업 환경을 정리한다
	ClearEnv();

    return (0);	
}

