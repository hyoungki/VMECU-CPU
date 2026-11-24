
#include    "includes.h"
#include	"localLib.h"
#include    "external.h"

extern  int     chkRcvFrameHOST( int hostid, int chid, int socketID, DNP_DATA_LINK *dnpData);
extern  int     checkDataLinkHOST( int hostid, int chid, int socketID, DNP_DATA_LINK *dnpData);
extern  int     checkAPPFrame_HOST(int hostid, int chid, int socketID);
extern  int     readDataLinkHOST(int hostid, int chid, int socketID, DNP_DATA_LINK *dnpData);

extern  TASK_INFO	*taskPtr;

//extern  RTC             *rtc;
//extern  OPR_MSG         *opr;
//extern  SYSTEM_QUE      *sque;
//extern  HISTORY_QUE     *hque;         // CONSOLE 용 이벤트

//extern  CONSOLE_INFO	*console;
//extern  HOST_DCB        *hostDCB[MAX_HOST];             /* HARRIS, LANDIS, SICS HOST Structure */

//extern	LINK_MSG		*linkCfg;

extern  int             termExec;
extern  int             wdtHostFlag[MAX_HOST];

extern  void	SigHandler( int sig );


/*******************************************************************************
 *                                                                        
 * 모듈명:	_initNetServerHOST_DNP()
 *                                                                        
 ******************************************************************************/
int _initNetServerHOST_DNP(int hostid, int chid)
{
    //int     on = 1;
	int		flag = 1, length;
	//int		port;
	//char	address[64];
	
    HOST_DCB        *host;
    
    host = (HOST_DCB *) hostDCB[hostid];                 /* 주장치 #1 속성정의 */
    
    if(host->socketServerFd != 0)   close(host->socketServerFd);
    
    /* ------------------------------------------------------------ */
    /*  SERVER-소켓의 Open : IPv4 기반, 연결지향형 소켓, TCP기반    */
    /* ------------------------------------------------------------ */
	if ((host->socketServerFd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == ERROR)
	{
	    if(opr->hostDebug == hostid)   
	    Debug(console,"host%2d> *** Channel=%d [NET] socket 실패 \n",hostid+1, chid+1);
		return(ERROR);
	}
	
	if(opr->hostDebug == hostid)	
    Debug(console,"host%2d> Channel=%d...[NET] socket Open OK ...%d \n", hostid+1, chid+1, host->socketServerFd);

    /* ------------------------------------------------ */
    /*  SOCKET Option...                                */
    /* ------------------------------------------------ */
    setsockopt(host->socketServerFd, SOL_SOCKET, SO_REUSEADDR, (char *)&flag, sizeof(int));
    setsockopt(host->socketServerFd, SOL_SOCKET, SO_KEEPALIVE, (char *)&flag, sizeof(int));
    //setsockopt(host->socketServerFd, SOL_SOCKET, SO_DONTROUTE, (char *)&flag, sizeof(int));
    //ioctl(host->socketServerFd, FIONBIO, (int)&flag);

    /* NON-BLOCK 설정 */
    flag = fcntl( host->socketServerFd, F_GETFL,0);
    fcntl( host->socketServerFd, F_SETFL, (flag | O_NONBLOCK));
    
    /* ------------------------------------------------ */    
    /*  소켓 구조체 초기화 ...                          */
    /* ------------------------------------------------ */    
    length = sizeof(struct sockaddr_in);
	bzero8248((byte *)&host->srvAddr, length);
	host->srvAddr.sin_family  = AF_INET;                          /* IPv4 기반의 인테넷 프로토콜 Family*/
	host->srvAddr.sin_port    = htons(host->tcpPort);             /* Port   정보의 Network Byte Order 로 변경함. */

    //host->srvAddr[chid].sin_addr.s_addr = htonl(INADDR_ANY);        /* IP주소 정보의 Network Byte Order 로 변경함. */

#if 1
    /* ------------------------------------------------ */
    /* CPU 타입과 HOST별 지정된 포트별 IP (CPU)지정     */
    /*  - mst_Net1 : Master-CPU의 Master 포트           */
    /*  - mst_Net2 : Master-CPU의 Slave 포트            */
    /*  - slv_Net1 : Slave-CPU의 Master 포트            */
    /*  - slv_Net2 : Slave-CPU의 Slave 포트             */
    /* ------------------------------------------------ */
	if(opr->cpuMode == CPU_MASTER)
	{
	    if(chid == 0)   host->srvAddr.sin_addr.s_addr = inet_addr(opr->net_mst_IP[host->mst_Net1]);   /* Master-TASK : 지정된 NET 포트 의 IP 설정 */
	    else            host->srvAddr.sin_addr.s_addr = inet_addr(opr->net_mst_IP[host->mst_Net2]);   /* Slave-TASK  : 지정된 NET 포트 의 IP 설정 */ 
	}
	else
	{   
	    if(chid == 0)   host->srvAddr.sin_addr.s_addr = inet_addr(opr->net_slv_IP[host->slv_Net1]);   /* Master-TASK : 지정된 NET 포트 의 IP 설정 */
	    else            host->srvAddr.sin_addr.s_addr = inet_addr(opr->net_slv_IP[host->slv_Net2]);   /* Slave-TASK  : 지정된 NET 포트 의 IP 설정 */ 
	}
#endif
	
    /* ------------------------------------------------ */    
    /*  소켓에 주소를 할당함...                         */
    /* ------------------------------------------------ */    
    length = sizeof(struct sockaddr_in);
	if (bind(host->socketServerFd, ( struct sockaddr *)&host->srvAddr, length) == ERROR)
	{
	    if(opr->hostDebug == hostid) 
	    { 
	        if(opr->cpuMode == CPU_MASTER)
	        {   
	            if(chid == 0)   Debug(console,"host%2d> *** Channel=%d /IP=%s [NET] bind 실패 ... socket = %d\n", hostid+1, chid+1, opr->net_mst_IP[host->mst_Net1], host->socketServerFd);
	            else            Debug(console,"host%2d> *** Channel=%d /IP=%s [NET] bind 실패 ... socket = %d\n", hostid+1, chid+1, opr->net_mst_IP[host->mst_Net2], host->socketServerFd);    
	        }
	        else
	        {
	            if(chid == 0)   Debug(console,"host%2d> *** Channel=%d /IP=%s [NET] bind 실패 ... socket = %d\n", hostid+1, chid+1, opr->net_slv_IP[host->slv_Net1], host->socketServerFd);
	            else            Debug(console,"host%2d> *** Channel=%d /IP=%s [NET] bind 실패 ... socket = %d\n", hostid+1, chid+1, opr->net_slv_IP[host->slv_Net2], host->socketServerFd); 
	        }     
	    }
		return(ERROR);
	}
	
	if(opr->hostDebug == hostid) 
    {
        if(opr->cpuMode == CPU_MASTER)
        {      	    
	        if(chid == 0)   Debug(console,"host%2d> Channel=%d...[NET] bind OK (ip=%s)... socket = %d \n", hostid+1, chid+1, opr->net_mst_IP[host->mst_Net1],host->socketServerFd);
            else            Debug(console,"host%2d> Channel=%d...[NET] bind OK (ip=%s)... socket = %d \n", hostid+1, chid+1, opr->net_mst_IP[host->mst_Net2],host->socketServerFd);	            
	    }
	    else
	    {
	        if(chid == 0)   Debug(console,"host%2d> Channel=%d...[NET] bind OK (ip=%s)... socket = %d \n", hostid+1, chid+1, opr->net_slv_IP[host->slv_Net1],host->socketServerFd);
            else            Debug(console,"host%2d> Channel=%d...[NET] bind OK (ip=%s)... socket = %d \n", hostid+1, chid+1, opr->net_slv_IP[host->slv_Net2],host->socketServerFd);
	    }      
    }
    
    /* ------------------------------------------------ */    
    /*  연결요청 대기상태로 진입...                     */
    /* ------------------------------------------------ */    
	if (listen(host->socketServerFd, 5) == ERROR)
	{
	    if(opr->hostDebug == hostid)	Debug(console,"host%2d> *** Channel=%d...[NET] listen 실패 \n", hostid+1, chid+1);
		return(ERROR);
	}

    //if(opr->hostDebug == hostid)	
    //Debug(console,"host%2d> Channel=%d ... [NET] listen OK ... socket = %d \n", hostid+1, chid+1, host->socketServerFd);

    if(opr->hostDebug == hostid) 
    {
        if(opr->cpuMode == CPU_MASTER)
        {      	    
	        if(chid == 0)   Debug(console,"host%2d> Channel=%d...[NET] listen OK (ip=%s)... socket = %d \n", hostid+1, chid+1, opr->net_mst_IP[host->mst_Net1],host->socketServerFd);
            else            Debug(console,"host%2d> Channel=%d...[NET] listen OK (ip=%s)... socket = %d \n", hostid+1, chid+1, opr->net_mst_IP[host->mst_Net2],host->socketServerFd);	            
	    }
	    else
	    {
	        if(chid == 0)   Debug(console,"host%2d> Channel=%d...[NET] listen OK (ip=%s)... socket = %d \n", hostid+1, chid+1, opr->net_slv_IP[host->slv_Net1],host->socketServerFd);
            else            Debug(console,"host%2d> Channel=%d...[NET] listen OK (ip=%s)... socket = %d \n", hostid+1, chid+1, opr->net_slv_IP[host->slv_Net2],host->socketServerFd);
	    }      
    }
    
    
	return(host->socketServerFd);
	
	
}



char *CHANNEL_NAME[]=
{
	"MASTER",
	"SLAVE"
} ;

char *get_channel_name( int chid)
{
	if ( chid ==0)  return "MASTER";
    else            return "SLAVE" ;
	
}

/* ============================================================ */
/*	HOST 인터페이스용 THREAD...									*/
/* ============================================================ */
void    *hostTcpipComm(void *data)
{
	int     hostid, chid;
	int		length = sizeof(struct sockaddr_in);
	int     index;
	int		socketID;
	struct 	sockaddr_in	clientAddr;
	int		port;
	char	address[64];
	int		oldsec=0;
	int     retVal;
	int     hostTerminate=0;
	int     comFailCnt;
	int		rxcnt;
	char    buffer[128];
	
	HOST_DCB        *host;

    DNP_DATA_LINK   *dnpData;
    DNP_APP_FRAME   *rcvAppDnp;
    DNP_APP_FRAME   *sndAppDnp;

    /* -------------------------------------------- */
    /*	클라이언트 정보를 전역 자료에 입력한다. 	*/
    /* -------------------------------------------- */
	memcpy(&socketID, data, sizeof(int)); 
	memcpy(&clientAddr, (char *)data+sizeof(int), length); 
	
	/* hostid 정보 추출... */
	index = sizeof(int) + length;
	memcpy(&hostid, (char *)data+index, sizeof(int)); 
	
	/* chid 정보 추출... */
	index = sizeof(int) + length + sizeof(int);
	memcpy(&chid, (char *)data+index, sizeof(int)); 
	
	/* -------------------------------------------- */
    /*	HOST 구조체 정보. 	                        */
    /* -------------------------------------------- */
	host = (HOST_DCB *) hostDCB[hostid];                 /* 주장치 #1 속성정의 */
	//host->socketID[chid] = socketID;
	
	port = ntohs(clientAddr.sin_port);
	sprintf(address, "%s", inet_ntoa(clientAddr.sin_addr));
   	
   	printf(" %s HOST(%d)/CH(%d) DNP-TCPIP_THREAD Activated ...%s...port=%d,  socket = %d !\n", TARGET_NAME, hostid+1, chid, address, port, socketID);

    sprintf(buffer, "%s HOST(%d)/CH(%d) DNP-TCPIP_THREAD Activated ...%s...port=%d,  socket = %d !", TARGET_NAME, hostid+1, chid, address, port, socketID);
    LogFile_MPU ( opr, NOT_DEFINED, buffer, strlen(buffer));
        
    /* ------------------------------------ */
    /* Network 연결후 - TASK 실행		    */
    /* ------------------------------------ */
	host->threadActive[chid] = SET;
    host->acceptFailCnt = 0;
    host->online[chid] = 0;
    
    /* ------------------------------------ */
    /* THREAD 관리변수 초기화		        */
    /* ------------------------------------ */
    hostTerminate       = SET;
    comFailCnt          = 0;
    	        
    /* ------------------------------------ */
    /* DNP 송수신 구조체 초기화             */ 
    /* ------------------------------------ */
    dnpData   = (DNP_DATA_LINK *) &host->dataLinkFrame[chid];
    sndAppDnp = (DNP_APP_FRAME *) &host->sndAppFrame[chid];
    rcvAppDnp = (DNP_APP_FRAME *) &host->rcvAppFrame[chid];

    while(hostTerminate)
    {
        wdtHostFlag[hostid] = 0;
        pause(10);
        
        /* ---------------------------------------------------- */
		/*	CPU 이중화 : LOCAL-SLAVE ... 대기모드 				*/
		/*  - LINK 이상시 ... SLAVE CPU에서 통신하지 않음...    */
		/* ---------------------------------------------------- */
		if(opr->runMode == LOCAL_SLAVE)
	    {
	    	// dualCpySts는 sw적 확인 ,db 아니고
	    	// 둘다 즉 상대편 CPU가 살아 있는데,  link가 끝어진 경우
	        if((opr->dualCpuSts == SET) && (linkCfg->offline == SET)) 
	        {
	            Debug(console, "[host=%d/%d] *Link Fail... Exit...!\n", hostid+1, chid);
	            pause(1000);
	            
	            continue;
	        }
        }
              
        /* ---------------------------------------------------- */
        /*  MASTER 통신포트 : 상위 데이터 수신 처리 ...                  */
        /* ---------------------------------------------------- */
        /* 2025-10-10 오후 4:12:35  close 이외는 erro 처리 없군...*/
        /* 시스템 Error인 경우도 close 해아는데... */
        if((rxcnt = readDataLinkHOST( hostid, chid, socketID, dnpData)) > 0)
        {
            opr->hostComSts[hostid] = 1;    // MMI 상태표출
            
            /* 접속오류 초기화 */
            host->acceptFailCnt = 0;
            
            host->portResetCount = 0;
            
            comFailCnt = 0;
            
            if(opr->hostDebug == hostid)
            {
                Debug(console,"\n[host=%d/%d]-----------------------------------------[%02d/%02d-%02d:%02d:%02d]\n", hostid+1,chid,
      	            rtc->month, rtc->day, rtc->hour, rtc->min, rtc->sec);
      	        
      	        if(chid == MASTER_PORT)         DumpDNP_rcv(console, "RXM:", &dnpData->rcvFrame, rxcnt);
      	        else if(chid == SLAVE_PORT)     DumpDNP_rcv(console, "RXS:", &dnpData->rcvFrame, rxcnt);    
            }
                
            dnpData->rcvCount = rxcnt;
                
            /* ---------------------------------------- */
            /*  HOST Online 이벤트 생성...              */
            /* ---------------------------------------- */
            if(host->online[chid] != 1)  
            {
                sysEvent1(opr, sque, EVENT_HOST_ONLINE, hostid, chid, 0, opr->cpuMode);  
                logHistoryQue1(opr, hque, EVENT_HOST_ONLINE, hostid, chid, 0, opr->cpuMode);  
                
                /* -------------------------------------------- */
                /* LOG File 저장                                */
                /* -------------------------------------------- */
                sprintf(buffer, "[TCPIP] ID=%2d, Chid=%d Online ", hostid+1, chid+1);
                LogFile_MPU ( opr, EVENT_HOST_ONLINE, buffer, strlen(buffer));
            }
                
            host->online[chid] = 1;     // Online 
            
            /* ---------------------------------------- */
            /* 수신 DNP-Packet 처리...                  */
            /* ---------------------------------------- */
            retVal = chkRcvFrameHOST(hostid, chid, socketID, dnpData);
            if((retVal == FUNC_USER_DATA) || (retVal == FUNC_UNCONFIRM))
            {
                retVal = checkDataLinkHOST(hostid, chid, socketID, dnpData);
                if(retVal)
                {
                    checkAPPFrame_HOST( hostid, chid, socketID);
                }                    
            }

        } // MASTER PORT            
        else if(rxcnt  == 0)
        {
            
            //if(opr->hostDebug == hostid)
           
            {	
            Debug(console,"host%02d> *TCP/IP (chid=%d)read ...0 Connection...Closed...!\n", hostid+1, chid);
            /* -------------------------------------------- */
            /* LOG File 저장                                */
            /* -------------------------------------------- */
            sprintf(buffer, "host%02d> *TCP/IP (chid=%d)read ...0 Connection...Closed...!", hostid+1, chid);
            LogFile_MPU ( opr, NOT_DEFINED, buffer, strlen(buffer));
			}            

            pause(1000);

            /* ---------------------------------------- */
            /*  HOST Offline이벤트 생성...              */
            /* ---------------------------------------- */
            /* 다른 것 처리는 없구나..밑에서 thread 종료하기만하네..*/
            if(host->online[chid] == 1)  
            {   
                sysEvent1(opr, sque, EVENT_HOST_OFFLINE, hostid, chid, 0, opr->cpuMode);
                logHistoryQue1(opr, hque, EVENT_HOST_OFFLINE, hostid, chid, 0, opr->cpuMode);
                
                /* -------------------------------------------- */
                /* LOG File 저장                                */
                /* -------------------------------------------- */
                sprintf(buffer, "[TCPIP] ID=%2d, Chid=%d Offline ", hostid+1, chid+1);
                LogFile_MPU ( opr, EVENT_HOST_OFFLINE, buffer, strlen(buffer));
            }
                                       
            host->online[chid] = 0;
            opr->hostComSts[hostid] = 0;    // MMI 상태표출
               
  	        /* DNP-Thread 종료... Accept() 대기 */
  	        hostTerminate = RESET;
            host->threadActive[chid] = RESET;
        }
        
        // readDataLinkHOST은 내가 master가 아니므로  timeout 은 나올 수 있는데....
        // CRC Error나 Magic Error는 network 에서 발생하기 힘들 듯 한데...
        else if ( rxcnt == TK_DNP_PROTOCOL_ERR )
        {
            Debug(console,"host%02d> *TCP/IP (chid=%d) DNP PROTOCOL ERR!\n", hostid+1, chid);
            /* -------------------------------------------- */
            /* LOG File 저장                                */
            /* -------------------------------------------- */
            sprintf(buffer, "host%02d> *TCP/IP (chid=%d) DNP PROTOCOL ERR!", hostid+1, chid);
            LogFile_MPU ( opr, NOT_DEFINED, buffer, strlen(buffer));

             pause(1000);

            /* ---------------------------------------- */
            /*  HOST Offline이벤트 생성...              */
            /* ---------------------------------------- */
            /* 다른 것 처리는 없구나..밑에서 thread 종료하기만하네..*/
            if(host->online[chid] == 1)  
            {   
                sysEvent1(opr, sque, EVENT_HOST_OFFLINE, hostid, chid, 0, opr->cpuMode);
                logHistoryQue1(opr, hque, EVENT_HOST_OFFLINE, hostid, chid, 0, opr->cpuMode);
                
                /* -------------------------------------------- */
                /* LOG File 저장                                */
                /* -------------------------------------------- */
                sprintf(buffer, "[TCPIP] ID=%2d, Chid=%d Offline ", hostid+1, chid+1);
                LogFile_MPU ( opr, EVENT_HOST_OFFLINE, buffer, strlen(buffer));
            }
                                       
            host->online[chid] = 0;
            opr->hostComSts[hostid] = 0;    // MMI 상태표출
               
  	        /* DNP-Thread 종료... Accept() 대기 */
  	        hostTerminate = RESET;
            host->threadActive[chid] = RESET;       	
        } 	
        
            
        if(oldsec == rtc->sec)  continue;
        oldsec = rtc->sec;
        
        host->acceptFailCnt      = 0;

        /* ------------------------------------------------ */
        /* HOST 통신이상시 처리...주/예비별 차별            */
        /* 주 포트 : Timeout 10초, 예비 포트 : Timeout 60초 */     
        /* ------------------------------------------------ */
        if(host->hostActive == HOST_ACTIVE_STS)
        {
            if(++comFailCnt > 10)  
            {
                /* ---------------------------------------- */
                /*  HOST Offline이벤트 생성...              */
                /* ---------------------------------------- */
                if(host->online[chid] != 0)  
                {
                    sysEvent1(opr, sque, EVENT_HOST_OFFLINE, hostid, chid, 0, opr->cpuMode);
                    logHistoryQue1(opr, hque, EVENT_HOST_OFFLINE, hostid, chid, 0, opr->cpuMode);
                    
                    /* -------------------------------------------- */
                    /* LOG File 저장                                */
                    /* -------------------------------------------- */
                    sprintf(buffer, "[TCPIP] ID=%2d, Chid=%d Offline ", hostid+1, chid+1);
                    LogFile_MPU ( opr, EVENT_HOST_OFFLINE, buffer, strlen(buffer));
                }
                
                host->online[chid] = 0;
                opr->hostComSts[hostid] = 0;    // MMI 상태표출
                host->hostActive = HOST_NOT_READY;      // OFFLINE
            
                //if(opr->hostDebug == hostid)    
                Debug(console,">> host(%2d) Active-Sts : *TCPIP (chid=%d) Comm Fail...Re-initial...!\n", hostid+1, chid);
            
                /* DNP-Thread 종료... Accept() 대기 */
                hostTerminate = RESET;
                host->threadActive[chid] = RESET;
            
            }      
        }
        else
        {              
            if(++comFailCnt > 60)   
            {
                /* ---------------------------------------- */
                /*  HOST Offline이벤트 생성...              */
                /* ---------------------------------------- */
                if(host->online[chid] != 0)  
                {
                    sysEvent1(opr, sque, EVENT_HOST_OFFLINE, hostid, chid, 0, opr->cpuMode);
                    logHistoryQue1(opr, hque, EVENT_HOST_OFFLINE, hostid, chid, 0, opr->cpuMode);
                    
                    /* -------------------------------------------- */
                    /* LOG File 저장                                */
                    /* -------------------------------------------- */
                    sprintf(buffer, "[TCPIP] ID=%2d, Chid=%d Offline ", hostid+1, chid+1);
                    LogFile_MPU ( opr, EVENT_HOST_OFFLINE, buffer, strlen(buffer));
                
                }
            
                host->online[chid] = 0;
                opr->hostComSts[hostid] = 0;    // MMI 상태표출
                host->hostActive = HOST_NOT_READY;      // OFFLINE
                
                //if(opr->hostDebug == hostid)    
                Debug(console,">> host(%2d) Ready-Sts: *TCPIP (chid=%d) Comm Fail...Re-initial...!\n", hostid+1, chid);
            
                /* DNP-Thread 종료... Accept() 대기 */
                hostTerminate = RESET;
                host->threadActive[chid] = RESET;
            }
        }

    }
    
    /* ------------------------------------ */    
    /*  HOST-Socket Close...                */
    /* ------------------------------------ */
    host->online[chid] = 0;
    opr->hostComSts[hostid] = 0;        // MMI 상태표출
    
    host->threadActive[chid] = RESET;   // THREAD 상태 종료 
    
    close(socketID);                    // THREAD Socket 종료
    
    Debug(console,">> HOST(%2d) : *TCPIP-Thread(ch=%d) Exit ...Close Port=%d, Socket = %d !\n", hostid+1, chid, port, socketID);
    
    /* ------------------------------------ */
    /* 접속종료되는 PORT 관리 (Max 4)       */
    /* ------------------------------------ */
    host->acceptPORT[chid] = 0;
    
    Debug(console,"\n------------------------------------------------------------------\n");
    Debug(console,"host%02d> PORT Close... MASTER = %4d, SLAVE = %4d\n", hostid+1, host->acceptPORT[0],host->acceptPORT[1]);
    Debug(console,"------------------------------------------------------------------\n");
                
    return (0);    
}


/* ============================================================ */
/*	HOST 인터페이스용 THREAD...									*/
/* ============================================================ */
int    hostDnp_tcpip_Thread(int hostid)
{
	int     chid, rcvChid;
	int		client, length = sizeof(struct sockaddr_in);
	int		port;
	int     retval, mstAddr, slvAddr;
	int     index;
	int     portSum;
	int     taskCount;
	char	address[64];
	char	data[64];

    char    buffer[128];
        	
	pthread_t	pThread;
	
	struct sockaddr_in	clientPtr;
	
	HOST_DCB        *host;

	/* -------------------------------- */
	/*  HOST별 Index ...                */
	/* -------------------------------- */
	chid      = 0;                    // MASTER-NET
	taskCount = 0;
	
    host = (HOST_DCB *) hostDCB[hostid];                 /* 주장치 #1 속성정의 */
    
    host->hostActive = HOST_NOT_READY;          /* HOST 통신모드 : 미지정 */   
    host->threadActive[0] = RESET;
    host->threadActive[1] = RESET;                
    
    host->online[MASTER_PORT]      = 0;    // default OFFLINE
    host->online[SLAVE_PORT]       = 0;    // default OFFLINE
        
    host->iinRcvTick = 0;
    host->acceptFailCnt = 0;
    
    /* ------------------------------------ */
    /* 접속/종료되는 PORT 관리 (Max 4)      */
    /* ------------------------------------ */
    host->acceptPORT[0] = 0;
    host->acceptPORT[1] = 0;
     
    printf(" %s HOST[%d] DNP-TCPIP-THREAD Activated ... !\n", TARGET_NAME, hostid+1);

    /* ------------------------------------ */
    /*  HOST 서버 환경초기화                */
    /* ------------------------------------ */    
    if(_initNetServerHOST_DNP(hostid, chid) == ERROR)
    {
   	    printf("host%02d> *** NET SERVER Initial... Fail...\n", hostid+1);	
	    close(host->socketServerFd);
        termExec = NO;

        /* -------------------------------------------- */
        /* LOG File 저장                                */
        /* -------------------------------------------- */
        sprintf(buffer, "host%02d> *** NET SERVER Initial... Fail...", hostid+1);	
        LogFile_MPU ( opr, NOT_DEFINED, buffer, strlen(buffer));        
        
    }
    
    opr->hostRestart[hostid] = RESET;
    host->portResetCount = 0;
    
	while(termExec)
	{
	    taskPtr->wdtCount = 0;
	    
	    wdtHostFlag[hostid] = 0;
		pause(1000);
		pause(1000);
		pause(1000);

        /* ------------------------------------ */
        /* HOST-DB 변경에 따른 재기동 ...       */
        /* ------------------------------------ */
        if(opr->hostRestart[hostid] == SET)
        {
            Debug(console,"host%02d> *** HOST TASK Restart... DB Chg ...\n", hostid+1);
            pause(1000);
            termExec = RESET;  // continue 해야되지 않나.
        }
         
        /* ------------------------------------ */        
        /* HOST  접속요청에따른  접속 허용      */
        /* ------------------------------------ */
    	length = sizeof(struct sockaddr_in);
    	

    	// 어 여기서 무한대기 ??? ..위에서 non-blocking 으로 열린다.
	    if ((client = accept(host->socketServerFd, ( struct sockaddr *)&clientPtr, (socklen_t *) &length)) != ERROR)
        {        	
            /* 접속 Port, Address 정보 Assign... */
        	port = ntohs(clientPtr.sin_port);
    		sprintf(address, "%s", inet_ntoa(clientPtr.sin_addr));
    	
        	/* socket 정보 Assign... */
	    	memcpy(data, &client, sizeof(int));
			memcpy(&data[sizeof(int)], &clientPtr, length);
		    //sprintf(address, "%s", inet_ntoa(clientPtr.sin_addr));
		
    		/* hostid 정보 Assign... */
	    	index = sizeof(int)+ length;
			memcpy(&data[index], &hostid, sizeof(int));
		
    		/* ------------------------------------ */
	    	/* Access HOST 채널ID 확인              */
			/* ------------------------------------ */
    		retval  = inet_addr(address);                    // Access HOST-IP
	    	mstAddr = inet_addr(host->mst_IPAddr1);         // Master-Port IP
            slvAddr = inet_addr(host->mst_IPAddr2);         // Slave-Port IP
        
            if(retval == mstAddr)           rcvChid = MASTER_PORT;
            else if (retval == slvAddr)     rcvChid = SLAVE_PORT;   
            else         // 이건 지정된 addr외 에서 오는 경우...                   
            {  // ? 여긴 뭐지..
                if(taskCount == 0)  rcvChid = MASTER_PORT;
                else                rcvChid = SLAVE_PORT;    
            
                taskCount = (taskCount + 1) & 0x01;
            }
        
            //if(opr->hostDebug == hostid)   
            Debug(console,"host%02d> accept O.K...%s (chid=%d)... port = %d, socket = %d\n", hostid+1, address, rcvChid, port, client);
        
            /* -------------------------------------------- */
            /* LOG File 저장                                */
            /* -------------------------------------------- */
            sprintf(buffer, "[TCPIP] ID=%2d, Chid=%d : Network ACCEPT() ... %s, PORT=%d  ", hostid+1, rcvChid+1, address, port);
            LogFile_MPU ( opr, NOT_DEFINED, buffer, strlen(buffer));
                
            /* ------------------------------------ */
	    	/* HOST 해당 chid-Task 체크...          */
			/*  - 해당 TASK 가 없는경우...  실행    */
		    /* ------------------------------------ */
            if(host->threadActive[rcvChid] == RESET) //host->threadActive[rcvChid] 변수는 해당 thread가 사용중인지 표시.
            {
                /* chid 정보 Assign... */
                index = sizeof(int)+ length + sizeof(int);
                memcpy(&data[index], &rcvChid, sizeof(int));
		    
		        /* ------------------------------------ */
   	            /* 접속되는 PORT 관리                   */
		        /* ------------------------------------ */
                host->acceptPORT[rcvChid] = port;
                host->portResetCount = 0;
                
                Debug(console,"\n------------------------------------------------------------------\n");
                Debug(console,"host%02d> Accept PORT .. MASTER = %4d, SLAVE = %4d\n", hostid+1, host->acceptPORT[0],host->acceptPORT[1]);
                Debug(console,"------------------------------------------------------------------\n");
            
            	/* ------------------------------------ */
        	    /*	통신 쓰레드 생성 (PCS Interface...)	*/	
	        	/* ------------------------------------ */
        	    if (pthread_create(&pThread, NULL, &hostTcpipComm, data) < 0)
	        	{
		    	    Debug(console,"host%02d> *** HOST TCPIP Thread Create Fail", hostid+1);
    	        	pause(1000);
    	    	
    	    	    /* -------------------------------------------- */
                    /* LOG File 저장                                */
                    /* -------------------------------------------- */
                    sprintf(buffer, "host%02d> *** HOST TCPIP Thread Create Fail", hostid+1);
                    LogFile_MPU ( opr, NOT_DEFINED, buffer, strlen(buffer));
                
                
        	    	/* 프로그램 재기동... */
                    termExec = RESET;
    	        }
            }
            else
            {
                Debug(console,"host%02d> ==> TASK-Already ACTIVE... %s (chid=%d)... port = %d, socket = %d\n", hostid+1, address, rcvChid, port, client);
                
                /* -------------------------------------------- */
                /* LOG File 저장                                */
                /* -------------------------------------------- */
                sprintf(buffer, "host%02d> ==> TASK-Already ACTIVE... %s (chid=%d)... port = %d, socket = %d", hostid+1, address, rcvChid, port, client);
                LogFile_MPU ( opr, NOT_DEFINED, buffer, strlen(buffer));
                    
        		pause(1000);
		
    		    /* ---------------------------------------- */
	    	    /* 2015년 4월 24일                          */
			    /* 유효 소켓이 남아있는 경우, CLOSE...      */
		        /* ---------------------------------------- */
		        close(client);
    
            }
                                 
        }
        else // accept 실패시..??????
        {	

            /* -------------------------------------------- */
            /* 접속되는 PORT 관리                           */
            /*  - 포트가 4개이상 활성화된 경우...Restart    */
            /* -------------------------------------------- */
            portSum = host->acceptPORT[0] + host->acceptPORT[1];
            
            if(opr->hostDebug == hostid)   
            Debug(console,"host%02d> port sum = %d, resetCount= %d .. MASTER = %4d, SLAVE = %4d\n", hostid+1, portSum, host->portResetCount, 
                host->acceptPORT[0],host->acceptPORT[1]);
            
            if(portSum != 0) // 사용중인 thread가 있다.
            {
                /* HOST 통신시 초기화 됨 */
                if(++host->portResetCount > 10) /* thread가 비정상적으로 종료되면 ..여기서..*/
                {      
                    Debug(console,"host%02d> *** Residunt PORT ... HOST-PROCESS Timeout ... RESTART...\n", hostid+1);	
            	    close(host->socketServerFd);
        	    
            	    /* 프로그램 재기동... */
                    termExec = RESET;
                    
                    /* -------------------------------------------- */
                    /* LOG File 저장                                */
                    /* -------------------------------------------- */
                    sprintf(buffer, "host%02d> *** Residunt PORT ... HOST-PROCESS Timeout ... RESTART...", hostid+1);	
                    LogFile_MPU ( opr, NOT_DEFINED, buffer, strlen(buffer));
                
                }
            }
                   
		}
		
        
	}/* while */
	
	Debug(console,"host%2d> *TCPIP-THREAD ......END...!\n", hostid+1);
    return (0);
}
