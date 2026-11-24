#include   "includes.h"
//?
#include	"localLib.h"
#include    "external.h"


extern int scan_tk_connect(int scanid, int devid, int actChanID) ;

extern  int     setOffline( int devid);
extern  int     setOnline( int devid);
extern  int     makeCmdMODBUS( int mode, int scanid, int devid, byte *sndbuf);
extern  int     checkEXPcode(int scanid, int devid, int expCode);
extern  int     dnpFlagClear( int devid);
extern  int     rcvHandlerMODBUS(int scanid, int devid, byte *rxbuf);

extern  TTY_DESC scanChan[MAX_CPUPORT];

extern  int             termExec;
extern  TASK_INFO	    *taskPtr;

extern  PDPDTIME_DATE_TIME      dnpRTC;
extern  PDPDTIME_MS_SINCE_70    dnpRTCInfo;


extern  THREAD_ENTRY    MOD_THREAD[MAX_MODULE];

extern  int     ThreadActive[MAX_MODULE];


/*******************************************************************************
 *                                                                        
 * 모듈명:	_initNetpServer()
 *                                                                        
 ******************************************************************************/

int _initNetClientSCAN(int scanid, int devid, int actChanID)
{
    //struct sockaddr_in	srvAddr;
    //int     so_reuseaddr = 1;	
    int     flag = 1;
    long    lflag ; 
    time_t now  ;
       
    char curTime[80] ;   
    
    DNP_DEVICE      *dnp;

    /* --------------------------------------- */
    /* CLIENT 프로세서에 해당하는 구조체 ... */
    /* --------------------------------------- */
    dnp = (DNP_DEVICE *) dnpcfg[devid];


    // 2025-08-24 오후 4:20:08 
    if (dnp->SocketFd != -1)    
    {   
    	 		
		Debug(console,"Scan-init> Current dev[%02d]> socket[%d] already opened \n", devid+1, dnp->SocketFd);	    	
    	return -ERROR ; 
    }	
    
    // 여기서 할 수도 있지...일단 장점은 한 곳에서 close / open 한다는 것 
#if 0    
    /* 소켓 CLOSE ... */
    if(dnp->SocketFd)   
    {
    	if((opr->dnpDebug == devid) || (opr->scanDebug == scanid)) 
        Debug(console,"Scan-init> Current dev[%02d]> socket = %d... Close & Re-initial \n", devid+1, dnp->SocketFd);	
        close(dnp->SocketFd);
    }
#endif     

#if 0         
    /* SLAVE 모드인 경우...RETURN  */
    if(dnp->type == TYPE_DNP_SIEMENS)
    {
        if(opr->runMode == LOCAL_SLAVE)
        {
            if((opr->dnpDebug == devid) || (opr->scanDebug == scanid)) 
            Debug(console,"Scan-init> dev[%02d]> *** CPU-SLAVE Mode ...return \n", devid+1);	
            pause(1000);
            
		    return(0);
        }      	
    }
#endif
        
    /* 소켓의 Open : IPv4 기반, 연결지향형 소켓, TCP기반  */
	if ((dnp->SocketFd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0 )
	{
	   // if((opr->dnpDebug == devid) || (opr->scanDebug == scanid))
        Debug(console,"Scan-init>  DEV[%02d]> *** [NET] Socket Open Failed...\n", devid+1);	
		return(ERROR);
	}
	
    if((opr->dnpDebug == devid) || (opr->scanDebug == scanid))
    {
        Debug(console,"------------------------------------------ \n");
        tk_print_curTime(curTime, 80) ;
        Debug(console,"Scan-init> Dev[%02d]> socket[%d] Open at %s\n", devid+1, dnp->SocketFd,curTime );
    }
    
#if 0
	if (( setsockopt(dnp->SocketFd, SOL_SOCKET, SO_REUSEADDR, (char *)&so_reuseaddr, sizeof( so_reuseaddr))) == ERROR)
	{
	    if((opr->dnpDebug == devid) || (opr->scanDebug == scanid))
	    Debug(console,"TCPIP[%02d]> ***[NET] set option : REUSEADDR 실패 \n", devid+1);	
		return ERROR;
	}
#endif
	
    /* ------------------------------------------------ */
    /*  SOCKET Option...                                */
    /* ------------------------------------------------ */
    setsockopt(dnp->SocketFd, SOL_SOCKET, SO_REUSEADDR, (char *)&flag, sizeof(int));
    setsockopt(dnp->SocketFd, SOL_SOCKET, SO_KEEPALIVE, (char *)&flag, sizeof(int));
    //setsockopt(dnp->SocketFd, SOL_SOCKET, SO_DONTROUTE, (char *)&flag, sizeof(int));
   // 이동 
 //   ioctl(dnp->SocketFd, FIONBIO, (int)&flag);
    	
    /* ---------------------------------------- */
    /* 소켓 구조체 초기화 ...                   */
    /* ---------------------------------------- */
	dnp->serv_addr.sin_family   = AF_INET;                          /* IPv4 기반의 인테넷 프로토콜 Family */
	dnp->serv_addr.sin_port     = htons(dnp->netPort);              /* Port   정보의 Network Byte Order 로 변경함. */
	
	/* ---------------------------------------- */
    /* 계전기 이중화 구성에 따른 통신초기화     */
    /* ---------------------------------------- */
    if(actChanID == 0)          // Master IP
    {
        if((opr->dnpDebug == devid) || (opr->scanDebug == scanid))
        Debug(console,"Scan-init> dev=%2d, Master IP - Network Init : %s\n", devid+1, dnp->ipString); 

        dnp->serv_addr.sin_addr.s_addr = inet_addr(dnp->ipString);	    /* IP conversion & assign */;
    }
    else if(actChanID == 1)     // Slave IP
    {
        if((opr->dnpDebug == devid) || (opr->scanDebug == scanid))
        Debug(console,"Scan-init> dev=%2d, SLAVE  IP - Network Init : %s\n", devid+1, dnp->ipString2); 

        dnp->serv_addr.sin_addr.s_addr = inet_addr(dnp->ipString2);	    /* IP conversion & assign */;
    }
          
    /* ------------------------------------------------ */
    /* 해당 계전기(Server) 로 연결을 시도...            */   
    /* ------------------------------------------------ */       
    if(connect(dnp->SocketFd, (struct sockaddr *) &dnp->serv_addr, sizeof(dnp->serv_addr)) == -1)
    {
        if((opr->dnpDebug == devid) || (opr->scanDebug == scanid))
        {   
            if(actChanID == 0)      Debug(console,"Scan-init> DEV[%02d]> ***IP=%s, PORT=%d  connect Error...!\n", devid+1, dnp->ipString, dnp->netPort);	
	        else if(actChanID == 1) Debug(console,"Scan-init> DEV[%02d]> ***IP=%s, PORT=%d  connect Error...!\n", devid+1, dnp->ipString2, dnp->netPort);	
	    }
          
        // 2025-08-24 오후 4:26:04
      	close( dnp->SocketFd);
        dnp->SocketFd = -1 ;       	
		dnp->connectStatus = RESET;	  
        return -1 ; 
    }
    else
    {
		lflag = fcntl(dnp->SocketFd, F_GETFL, 0);    	
 		lflag |= O_NONBLOCK;
    
    	if (fcntl(dnp->SocketFd, F_SETFL, lflag) == -1 )
    	{
        	Debug(console,"Scan-init> Dev[%02d] set SockFd NONBLOCK failed %s\r\n", devid+1, strerror(errno));
        	return -1;
    	}        
        
        if((opr->dnpDebug == devid) || (opr->scanDebug == scanid))
        {
           tk_print_curTime(curTime, 80) ;           
            if(actChanID == 0)     
            	 Debug(console,"Scan> Dev[%02d]: Socekt[%d]:Ch[%d] connect to IP=%s:%d  OK  %s\n", devid+1,dnp->SocketFd, actChanID, dnp->ipString, dnp->netPort, curTime);	
//	        else if(actChanID == 1) 
//	        	Debug(console,"Scan-init> Dev[%02d]: Ch[%d] connect to IP=%s:%d  OK...socket=%d %s\n", devid+1,actChanID, dnp->ipString2, dnp->netPort,dnp->SocketFd,curTime);	
        }
 
        /* -------------------------------- */
        /*  DNP 통신의 경우.... Flag Reset  */
        /* -------------------------------- */       
      //   dnp->sndLinkSts = SET;
        dnp->sndFcvBit  = FCV_BIT;
        dnp->sndFcbBit  = FCB_BIT;
                
        dnp->thSndSeqno = 0;        
        dnp->acSndSeqno = 0;
        dnp->ACseqFail  = 0;
        
        /* -------------------------------- */
        /*  계전기 Socket 연결상태.. SET    */
        /* -------------------------------- */        
        dnp->connectStatus = SET;
		return(dnp->SocketFd);
    }
            

}





/*
*/
int _reConnectClientPLC(int scanid, int devid, int actChanID)
{
    //struct sockaddr_in	srvAddr;
    //int  so_reuseaddr = 1;
    //int  flag = 1;	
    
    DNP_DEVICE      *dnp;

    /* --------------------------------------- */
    /* CLIENT 프로세서에 해당하는 구조체 ... */
    /* --------------------------------------- */
    dnp = (DNP_DEVICE *) dnpcfg[devid];
    
    /* SLAVE 모드인 경우...RETURN  */
    if(dnp->type == TYPE_DNP_SIEMENS)
    {
        if(opr->runMode == LOCAL_SLAVE)
        {
            if((opr->dnpDebug == devid) || (opr->scanDebug == scanid))
            Debug(console,"net-connect> dev[%02d]> *** CPU-SLAVE Mode ...return \n", devid+1);	
            pause(1000);
            
		    return(0);
        }      	
    }
    
    /* Image - Server Connection... */
    if(connect(dnp->SocketFd, (struct sockaddr *) &dnp->serv_addr, sizeof(dnp->serv_addr)) == -1)
    {
        if((opr->dnpDebug == devid) || (opr->scanDebug == scanid))
        {
            if(actChanID == 0)      Debug(console,"TCPIP[%02d] ***IP=%s, PORT=%d ... connect Error...!\n", devid+1, dnp->ipString, dnp->netPort);
            else if(actChanID == 1) Debug(console,"TCPIP[%02d] ***IP=%s, PORT=%d ... connect Error...!\n", devid+1, dnp->ipString2, dnp->netPort);    
        }
        
        dnp->connectStatus = 0;
        //close( hostSocketFd);
    }
    else
    {
        if((opr->dnpDebug == devid) || (opr->scanDebug == scanid))
        {
            if(actChanID == 0)      Debug(console,"TCPIP[%02d] IP=%s, PORT=%d ... connect OK... socket=%d !\n", devid+1, dnp->ipString, dnp->netPort, dnp->SocketFd);
            else if(actChanID == 1) Debug(console,"TCPIP[%02d] IP=%s, PORT=%d ... connect OK... socket=%d !\n", devid+1, dnp->ipString2, dnp->netPort, dnp->SocketFd);
        }      
        
        /* -------------------------------- */
        /*  DNP 통신의 경우.... Flag Reset  */
        /* -------------------------------- */       
        dnp->sndLinkSts = SET;
        dnp->sndFcvBit  = FCV_BIT;
        dnp->sndFcbBit  = FCB_BIT;
                
        dnp->thSndSeqno = 0;        
        dnp->acSndSeqno = 0;
        dnp->ACseqFail  = 0;
        
        /* -------------------------------- */
        /*  계전기 Socket 연결상태.. SET    */
        /* -------------------------------- */        
        dnp->connectStatus = SET;
    }
            
	return(dnp->SocketFd);
}


/*****************************************
* handle response 
* 
* return :  1   get vaild response  
*  			0   TCP Connection closed
*           -1  Timeout
*           -2  select system error 
******************************************/
int  rcvWithModbus_TCPIP( int scanid, int devid, byte *rxbuf)
{
    int     result;
    int     size;
    int     count;
    //word    crc;
    //word    rx_crc;
   	int     idx;
   	//int     loop;

    DNP_DEVICE      *dnp;

    /* --------------------------------------- */
    /* CLIENT 프로세서에 해당하는 구조체 ... */
    /* --------------------------------------- */
    dnp = (DNP_DEVICE *) dnpcfg[devid];
    
    if(dnp->connectStatus == RESET)
    {
        return (-2);
    }
        
    count = 0;

    /* ---------------------------------------- */
    /*  READ MODBUS - Header                    */
    /* ---------------------------------------- */ 
	if((result =tk_net_readn(dnp->SocketFd, (char *) rxbuf, 6, 2000)) < 0)
//	if((result =TKreadn(dnp->SocketFd, (char *) rxbuf, 6, 2000)) < 0)
    {
        /*  such as timeout */
		if(result == TK_TIMEOUT ) /* timeout */
	    {
	        if((opr->dnpDebug == devid) || (opr->scanDebug == scanid))
            Debug(console,"dev%02d> *MOD-TCP: Header Read Timeout [%d]... \n", devid+1, result);			     			        
	    }
        else
        {
            if((opr->dnpDebug == devid) || (opr->scanDebug == scanid)) 
		    Debug(console,"dev%02d> *MOD-TCP: Header Read fail [%d][%s] \n", devid+1, result, strerror(errno));
        }
        return result;   // TK_SYS_ERR                                                                                      
    }

	if(result == 0)	return 0;

    if(result != 6)
    {
        if((opr->dnpDebug == devid) || (opr->scanDebug == scanid)) 
	    Debug(console,"dev%02d> *MOD-TCP: Header Read fail expect[%d] but got[%d] \n", devid+1, 6,result);     
        return TK_SYS_ERR ;   
    }

	idx = 0; 

CHECK_MAGIC :
                                                                                                              
    if(rxbuf[idx] != 0x7E )
	{
		if(idx < result-1)  idx++;
		else
		{ 
		    if((opr->dnpDebug == devid) || (opr->scanDebug == scanid))
			Debug(console,"dev%02d> *MOD-TCP: Can't find magic number \n", devid+1);
			return TK_SYS_ERR;
		}
		goto CHECK_MAGIC ;
	}

	/* find magic-number and get the rest */
	if(idx > 0)
	{
		rxbuf[0]=rxbuf[idx];

		if((result =TKreadn(dnp->SocketFd,(char *) &rxbuf[1], 5, 100)) < 0)
        {
            if((opr->dnpDebug == devid) || (opr->scanDebug == scanid)) 
		    Debug(console,"dev%02d> *MOD-TCP: Header Read fail\n", devid+1);
            return TK_SYS_ERR;                                                                                   
      	}
	}

	//if(opr->plcDebug == scanid)  DumpBuff(console,"RCV: ", rxbuf, 6);
		
    size = ((rxbuf[4] << 8)| rxbuf[5]) & 0xff;
    
	if(size > 256)
    {
        if((opr->dnpDebug == devid) || (opr->scanDebug == scanid))
    	Debug(console,"dev%02d> *MOD-TCP: rcv BODY-Size error...%d \n", devid+1);
		return TK_SYS_ERR;
    }	    
                                                                                                              

    if((result =tk_net_readn(dnp->SocketFd, (char *)&rxbuf[6], size, 2000))<=0)
//    if((result =TKreadn(dnp->SocketFd, (char *)&rxbuf[6], size, 2000))<0)
    {
        if ( result == 0)
        {
            if((opr->dnpDebug == devid) || (opr->scanDebug == scanid)) 
            Debug(console,"dev%02d> *MOD-TCP: while reading  BODY  Peer Close socket\n", devid+1);            
            return 0; 
        }

        if((opr->dnpDebug == devid) || (opr->scanDebug == scanid)) 
        Debug(console,"dev%02d> *MOD-TCP: READ BODY FAIL... \n", devid+1);
		return TK_SYS_ERR;
    }
    
    
    
    
    count = result + 6;
    if(count > 256)
    {
        if((opr->dnpDebug == devid) || (opr->scanDebug == scanid))
    	Debug(console,"dev%02d> *MOD-TCP: rcv Size error...%d \n", devid+1, count);
		return TK_SYS_ERR;
    }	
    
    return count;
                                                                                                              
}


/* -------------------------------------------------------- */
/*  SCAN THREAD Start Routine ....                          */
/* -------------------------------------------------------- */
void    *scan_tcpip_modbus(int  *arg)
{
    int     scanid;
    //int		comPort;
    //int     activeChan;
    int     devid;
    int     chanComFail;
    int     chanReset;
    int     txcnt;
    int     rxcnt, rxVal;
    int     reConnectCount;
    int     timeout, reqTimeTick;
    int     oldsec=0;
    int     taskDelay;
    int     offlineTick;
    int     errCount;
    byte    rcvAddr, opcode, expCode;
    byte    *rxbuf, *txbuf;
    char    buffer[128];
    
    DNP_DEVICE      *dnp;
    SCAN_CONFIG     *scan;
	
	/* ------------------------------------ */
	/* THREAD Argment Assign ...            */
	/* ------------------------------------ */
	devid = (int ) *arg;
	
	/* ------------------------------------ */
    /* SCAN-TASK 별 모듈구성 정보                */ 
    /* ------------------------------------ */
    dnp = (DNP_DEVICE *) dnpcfg[devid];
    dnp->activeTick = 0;
    
    scanid = dnp->scanPort;
    scan = (SCAN_CONFIG *) scanCfg[scanid];
    
    taskDelay = scan->comDelay * 10;
    if(taskDelay < 10)		taskDelay = 10;
    if(taskDelay > 2000)	taskDelay = 2000;	
        
    printf("Dev[%02d] TCPIP-MODBUS (scan=%d) Thread Activated (Delay = %d ms)...\n", devid+1,  scanid+1 , taskDelay);
    
    txbuf = (byte *) &dnp->sndBuffer[0];
    rxbuf = (byte *) &dnp->rcvBuffer[0];
    
    dnp->activeChan     = 0; 
    
    dnp->modbusRcvFail  = 0;	
    dnp->reqDataCount   = 0;
    dnp->reqDataOpcode  = 0;
    offlineTick = 0;
    
    /* ------------------------------------ */
    /*  장치별 TCPIP-CLIENT initial...      */
    /* ------------------------------------ */
#if  0    // hkkim
   	if (_initNetClientSCAN(scanid, devid, dnp->activeChan) == ERROR)
    { 
        if((opr->dnpDebug == devid) || (opr->scanDebug == scanid))
        printf("TCPIP[%02d]> *** Network Initial... Fail...\n", devid+1);	
        ThreadActive[devid] = NO; // thread와 이것을 호출한 것으 종료하는 방법
    }
#endif 
    
    chanComFail = 0;
    chanReset   = 0;
    errCount    = 0;
    reConnectCount = 0;
    
    // hkkim
    dnp->SocketFd = -1 ;    
 	dnp->connectStatus = RESET;   
    
    while(ThreadActive[devid])
	{
	    pause(taskDelay);

//        printf("modbus tcp running.\r\n");

        /* ---------------------------------------------------- */
		/*	CPU 이중화 : LOCAL-SLAVE ... 대기모드 				*/
		/* ---------------------------------------------------- */
		if(opr->runMode == LOCAL_SLAVE)		// 절체할때는 offline 처리는 안 하는거군...
		{
		    /* ---------------------------------------- */
			/*	2017.06.27 SLAVE 모드 전환시 소켓 Close */
			/* ---------------------------------------- */
            if(dnp->connectStatus == SET)
            {
                Debug(console,"dev%02d> *MODBUS-TCP: CPU Mode Change => SLAVE ... Socket Close(%d) ...!\n", devid+1, dnp->SocketFd);                
                
                close( dnp->SocketFd);                  
                // hkkim
                dnp->SocketFd = - 1 ;    
 	            dnp->connectStatus = RESET;   
                
                
            }
                  
			/* ---------------------------------------- */
			/*	SLAVE 모드에서 수신된 데이터를 처리함   */
			/* ---------------------------------------- */
			dnp->connectStatus = RESET;
			dnp->connectFail = 0;
			reConnectCount   = 3;   // 재접속시간을 최소화... 의미 없는 변수...삭제해애 한다.
			pause(500);
			continue;
		} 

        /* ------------------------------------- */
	    /*  접속실패에 따른 재접속...            */
	    /* ------------------------------------- */
	    if(dnp->connectStatus == RESET)
	    {
	    	//if((opr->dnpDebug == devid) || (opr->scanDebug == scanid))  Debug(console,"dev%02d> * MOD-TCP: retry Connect....!\n", devid+1);                
            //_reConnectClientPLC(scanid, devid, dnp->activeChan);  
            pause(500);
            
            if(1) // hkkim
//            if(++reConnectCount > 5)
            {
                reConnectCount = 0;
                //close( dnp->SocketFd);  

				/* ---------------------------------------- */
                /*  장치별 TCPIP 초기화...                  */
                /* ---------------------------------------- */
            //    if(_initNetClientSCAN(scanid, devid, dnp->activeChan) < 0 )
		   	    if (scan_tk_connect(scanid, devid, dnp->activeChan) < 0 )                    
                { 
                    if((opr->dnpDebug == devid) || (opr->scanDebug == scanid))
                    Debug(console,"dev%02d> *** _initNetClientSCAN  Fail...\n" , devid+1);	
                            
//		                if((opr->dnpDebug == devid) || (opr->scanDebug == scanid))   
//		                	Debug(console,"dev%02d> * MOD-TCP: Initial...CLIENT....!\n", devid+1);

	                /* ---------------------------------------- */
	                /* 계전기 이중화에 따른 IP 구성정보 변경    */   
	                /* ---------------------------------------- */                 
	                if(dnp->dualIED == SET)		dnp->activeChan = (dnp->activeChan + 1) & 0x01;
	                else						dnp->activeChan = 0;
                    
	                /* ---------------------------------------------------- */
	                /*  2025-01-13 장치별 Offline 판정...: Net-Initial 5회이후		*/
	                /* ---------------------------------------------------- */       
	                if(++offlineTick > 3)
	                {
	                    offlineTick = 0;
	          	        if(dnp->online != RESET)  
	                    {
	                        if((opr->dnpDebug == devid) || (opr->scanDebug == scanid))
	                        Debug(console,"mod-tcpip> *dev(%02d)...offline...!\n", devid + 1);

	                        setOffline(devid);
	                        dnp->online = RESET;
	                    }
	                }
#if 0 // hkkim                
				/* ---------------------------------------- */
                /*  장치별 TCPIP 초기화...                  */
                /* ---------------------------------------- */
                if(_initNetClientSCAN(scanid, devid, dnp->activeChan) == ERROR)
                { 
                    if((opr->dnpDebug == devid) || (opr->scanDebug == scanid))
                    Debug(console,"dev%02d> *** Target No Response Re-Initial... Fail...\n" , devid+1);	
                }
#endif 
	                /* -------------------------------------------- */
	                /* 계전기별 통신이상에 따른 Offline 처리        */
	                /* -------------------------------------------- */
	                if(++dnp->comFailTick > scan->offCount)
	                {
	                    dnp->comFailTick = 0;

	                    /* ---------------------------------------- */ 
	                    /* 개별 장치별 Offline 상태 : CPU 절체      */
	                    /* ---------------------------------------- */ 
	                    if((opr->runMode == LOCAL_MASTER) && (scan->changeMode == CHANGE_EACH_DEVICE))
	                    {
	                        if((opr->dualCpuSts == SET) && (scuCfg->remoteMode == AUTO_MODE) && (opr->cpuChangeTick < CHANGE_CPU_TICK))   
	                        {
	                            opr->cpuChange = SET;
	                            logEvent1(opr, sque, EVENT_CHANGE_CPU, CHANGE_EACH_DEVICE, 0, 0, opr->cpuMode); 
	                            logHistoryQue1(opr, hque, EVENT_CHANGE_CPU, CHANGE_EACH_DEVICE, 0, 0, opr->cpuMode);   
	                        
	                            /* -------------------------------- */
	                            /* LOG File 저장                    */
	                            /* -------------------------------- */
	                            sprintf(buffer, "[MODBUS-TCPIP] 전체장치 통신절체...Device=%d, changeTick=%d ", devid+1, opr->cpuChangeTick);
	                            LogFile_MPU (opr, EVENT_CHANGE_CPU, buffer, strlen(buffer));  
	                        
	                            pause(1000);
	                        
	                            opr->cpuChangeTick++;
	                        
	                            if(opr->wdtDebug)
	                            {   
	                                Debug(console,"\n--------------------------------------------\n");
	                                Debug(console,"scan %2d> %02d:%02d:%02d *** TCPIP-MODBUS Device scan Fail... CPU Change (%d)...!\n", scanid+1, rtc->hour, rtc->min, rtc->sec, opr->cpuChangeTick);
	                                Debug(console,"--------------------------------------------\n");
	                            }
	                        
	                            /* OFFLINE 이벤트 발생... */
	                            dnp->online = 2;
	                        }
	                        
	                    }
	                        
	                    dnp->connectStatus = RESET;
	                }
	                
	                
            		pause(500);	                  
	            } 	            	            
	            else 
	            {   
	                // _initNetClientSCAN  성공    
                    if((opr->dnpDebug == devid) || (opr->scanDebug == scanid))
                    Debug(console,"dev%02d>  connection ok\n" , devid+1);	
					
					dnp->connectStatus = SET ; 	            	
	            }
            }
		    continue;
		}

        opr->devScanCount[scanid]++;

        /* ---------------------------------------- */
        /* 2015년 11월 5일                          */
        /* MODBUS : 수신 Frame Error 가 발생한 경우 */
        /* ---------------------------------------- */
        if(dnp->modbusRcvFail > 2)
        {
            if((opr->dnpDebug == devid) || (opr->scanDebug == scanid))
          		Debug(console, "dev%02d> *MODBUS Rcv Frame Error...Pause % Continue !\n", devid + 1, dnp->modbusRcvFail);
          	
          	dnp->modbusRcvFail = 0;	
          	
          	TKreadn(dnp->SocketFd, (char *)rxbuf, 64, 1000);
          	
            pause(500);   
            continue;
        }

//        printf("call makeCmdMODBUS\r\n");

        /* ---------------------------------------- */
        /* MODBUS : 보호계전기 송수신 루틴...       */
        /* ---------------------------------------- */
        if((txcnt = makeCmdMODBUS( COM_TCPIP, scanid, devid, txbuf)) > 0)
        {
            if((opr->dnpDebug == devid) || (opr->scanDebug == scanid))   DumpBuff(console,"TXD: ", txbuf, txcnt);
            
            dnp->comSndCount++;
            
            /* -------------------------------- */
		    /* Network Response...              */
    		/* -------------------------------- */    
    		if(dnp->connectStatus == SET)  // 당연하게 여기지..
		    {
		        // hkkim
		        if((rxVal = tkWriteTCP( dnp->SocketFd , txbuf, txcnt, 1000)) != txcnt )
    			{
            		if((opr->dnpDebug == devid) || (opr->scanDebug == scanid))
            		Debug(console, "dev%02d> [MODBUS-TCP] **** net Send Error [%d / %d]...socket = %d !\n", devid+1, txcnt,  rxVal, dnp->SocketFd);
            		/* -------------------------------------------- */
                    /* LOG File 저장                                */
                    /* -------------------------------------------- */
                    sprintf(buffer, "dev%02d> [MODBUS-TCP] **** net Send Error [%d / %d]...socket = %d !", devid+1, txcnt,  rxVal, dnp->SocketFd);
                    LogFile_MPU ( opr, NOT_DEFINED, buffer, strlen(buffer));
                    pause(1000);
                    
                    // hkkim
                    dnp->SocketFd = - 1 ;    
 	                dnp->connectStatus = RESET;                       
			    	continue ; 
			    }
            
                /* -------------------------------- */
                /* 접속이상에 따른 송신실패시...    */
                /* -------------------------------- */
                if( rxVal != txcnt)
                {
                    // 위에서 처리해서...
                    if((opr->dnpDebug == devid) || (opr->scanDebug == scanid))
                    Debug(console,"dev%02d> [MODBUS-TCP] * net Send Size Error [%d %d]...!\n", devid+1, txcnt,  rxVal);
                    
                    /* -------------------------------------------- */
                    /* LOG File 저장                                */
                    /* -------------------------------------------- */
                    sprintf(buffer, "dev%02d> [MODBUS-TCP] *net Send Size Error [%d %d]...!", devid+1, txcnt,  rxVal);
                    LogFile_MPU ( opr, NOT_DEFINED, buffer, strlen(buffer));
                    pause(1000);
                                
                    // hkkim
                    dnp->SocketFd = - 1 ;    
 	                dnp->connectStatus = RESET;                       
			    	continue ; 
                }
            }         

            /* ------------------------------------------------ */
            /*   계전기 OFFLINE : 주기적으로 ONLINE Check ..    */
            /* ------------------------------------------------ */
            reqTimeTick = 100;
            
            if(scan->changeMode == 0)
            {    
                if(dnp->online == 0)
                {
                    dnp->timeOutTick = (dnp->timeOutTick + 1) & 0x3f;

                    if(dnp->timeOutTick == 0)   reqTimeTick = 100;
                    else                        reqTimeTick = 10;
                }
            }
            
            /* ------------------------ */
            /* 모듈별 Time out 처리     */
            /* ------------------------ */
            // for(timeout = 0; timeout < reqTimeTick; timeout++)
            for(timeout = 0; timeout < 1; timeout++)
            {
                usleep(1000);            
                if((rxcnt = rcvWithModbus_TCPIP( scanid, devid, rxbuf) ) > 0)
                {
                    rcvAddr = rxbuf[6];
                    opcode  = rxbuf[7];
                    
                    if((opr->dnpDebug == devid) || (opr->scanDebug == scanid))	
                        DumpBuff(console,"RXD: ", rxbuf, rxcnt);
                    	
                    if(opcode & 0x80)
                    {
                    	expCode = rxbuf[8];
        				checkEXPcode(scanid, devid, expCode);
                        Debug(console,"dev%2d>  rcv expCode [%d]!\n", devid+1, expCode);        				
        				break  ; // for를 살리자. 더 이상 해석 말자..
        				// break; // for 가 없어 while을 빠진다. 2026-07-14 오후 8:54:48 지운다.
                    }
                    	
                    /* ------------------------------------ */
                    /* 수신패킷의 Address  Check...         */
                    /* ------------------------------------ */	
                    if(scan->virtualMode == 1)  // scan 의 포트 번호
                    {
                        if(rcvAddr != scan->comAddr)
                    	{
                    		if((opr->dnpDebug == devid) || (opr->scanDebug == scanid))	Debug(console,"dev%2d> *** rcv Invalid  V-Addr...rcv = %d !\n", devid+1, rcvAddr);
                    		break;
                    	} 
                    }
                    else
                    {                                  
                    	if(rcvAddr != dnp->ioid)
                    	{
                    		if((opr->dnpDebug == devid) || (opr->scanDebug == scanid))	Debug(console,"dev%2d> *** rcv Invalid Addr...rcv = %d!\n", devid+1, rcvAddr);
                    		break;
                    	}
                    }
                	                        
                    opr->scanComSts[scanid] = 1;
                    
                    dnp->comRcvCount++;
                    dnp->comFailTick = 0;
                    
                    offlineTick = 0;
                    chanReset   = 0;
                    chanComFail = 0;
                    errCount  = 0;
                    
                    /* ------------------------------------------------ */
                    /*  장치별 Online 판정...                           */
                    /* ------------------------------------------------ */
                    if(dnp->online != SET)  
                    {
                        if((opr->dnpDebug == devid) || (opr->scanDebug == scanid))  Debug(console,"tcpip> dev(%02d)  online...!\n", devid+1);
                        setOnline(devid);
                        opr->cpuChangeTick = 0;
                    }
                    
                    dnp->online = SET;
                
                    /* Header를 뺀 나머지 정보 처리 */
                    rcvHandlerMODBUS(scanid, devid, &rxbuf[6]);
            
                    /* ------------------------------------------------ */
                    /*  지멘스계전기 이중화 : 계전기 통신 Active Flag에 의한 절체...  	*/
                    /* ------------------------------------------------ */
                    if((dnp->dualIED == SET) && (dnp->type == TYPE_MODBUS_SZ))
                    {
                        if((dnp->activeMode == SET) && (dnp->activeFlag == RESET))
                        {
                            if(++dnp->activeTick > 4)
                            {    
                                if((opr->dnpDebug == devid) || (opr->scanDebug == scanid))
	                            Debug(console,"dev%02d> [MODBUS-TCP] *** [지멘스] Device Status (Active Flag) ... RESET...!\n", devid+1);
            	                
            	                /* -------------------------------------------- */
                                /* LOG File 저장                                */
                                /* -------------------------------------------- */
                                sprintf(buffer, "dev%02d> [MODBUS-TCP] *** [지멘스] Device Status (Active Flag)... RESET...!", devid+1);
                                LogFile_MPU ( opr, NOT_DEFINED, buffer, strlen(buffer));
                                pause(1000);
                        
            	                dnp->connectStatus = RESET;    
                                // hkkim
                                dnp->connectStatus = RESET;
                                close (dnp->SocketFd) ;
           	                    dnp->SocketFd =  -1 ;  

            	                
            	                dnp->activeTick = 0;
            	            }
                        } 
                        else
                        	dnp->activeTick = 0;   
                    }
                                 
                    break;
                }
                else
                {
            	    if(rxcnt == 0) // End-of-Connection
        	        {
        	            if((opr->dnpDebug == devid) || (opr->scanDebug == scanid))
	                    Debug(console,"dev%02d> [MODBUS-TCP] *** TCP/IP read ...0 Connection...Failed...!\n", devid+1);
	                    
	                    /* -------------------------------------------- */
                        /* LOG File 저장                                */
                        /* -------------------------------------------- */
                        sprintf(buffer, "dev%02d> [MODBUS-TCP] *** TCP/IP read ...0 Connection...Failed...!", devid+1);
                        LogFile_MPU ( opr, NOT_DEFINED, buffer, strlen(buffer));
                        pause(1000);
                        
                        // hkkim
                        dnp->connectStatus = RESET;
                        close (dnp->SocketFd) ;
           	             dnp->SocketFd =  -1 ;  
                        
                        
            	        dnp->connectStatus = RESET;
            	        reConnectCount 	   = 10;		 // Socket 생성 & 접속
	                }
	                else if(rxcnt == -1) // 이게 timeoout 이네..
        	        {
        	            if((opr->dnpDebug == devid) || (opr->scanDebug == scanid))
	                    Debug(console,"dev%02d> [MODBUS-TCP] *** TCP/IP read ...Packet Error..%d!\n", devid+1, errCount);
	                    
	                    pause(100);  
	                    TKreadn(dnp->SocketFd, (char *)rxbuf, 64, 1000);
						pause(100);  
						
	                    if(++errCount > 5)
	                    {   
	                        /* -------------------------------------------- */
                            /* LOG File 저장                                */
                            /* -------------------------------------------- */
                            sprintf(buffer, "dev%02d> [MODBUS-TCP] *** TCP/IP read ...Packet Error..%d!", devid+1, errCount);
                            LogFile_MPU ( opr, NOT_DEFINED, buffer, strlen(buffer));
                            pause(1000);
                         
                                // hkkim
                                dnp->connectStatus = RESET;
                                close (dnp->SocketFd) ;
           	                    dnp->SocketFd =  -1 ;                           
                         
                         
	                        dnp->connectStatus = RESET;
	                        reConnectCount 	   = 10;		 // Socket 생성 & 접속
	                        errCount = 0;
                        }
	                }
	                else if(rxcnt == -2) // error
       	            {
        	            if((opr->dnpDebug == devid) || (opr->scanDebug == scanid))
	                    Debug(console,"dev%02d> [MODBUS-TCP] *** TCP/IP un Connected...Failed...!\n", devid+1);
	                    
	                    /* -------------------------------------------- */
                        /* LOG File 저장                                */
                        /* -------------------------------------------- */
                        sprintf(buffer, "dev%02d> [MODBUS-TCP] *** TCP/IP un Connected...Failed...!", devid+1);
                        LogFile_MPU ( opr, NOT_DEFINED, buffer, strlen(buffer));
                        pause(1000);

                        // hkkim
                        dnp->connectStatus = RESET;
                        close (dnp->SocketFd) ;
           	             dnp->SocketFd =  -1 ;  

                            
            	        dnp->connectStatus = RESET;
	                }
	                else //hkkim
	                {
	                    Debug(console,"dev%02d> [MODBUS-TCP] *** TCP/IP un unknown error !\n", devid+1);
                        // hkkim
                        dnp->connectStatus = RESET;
                        close (dnp->SocketFd) ;
           	             dnp->SocketFd =  -1 ;  	                
	                }  
	                
	                break;
	            }   
           
            }	            
        }        
        else // hkkim  TX 실패시
        {
              Debug(console, "dev%02d> *MODBUS makeCmdMODBUS failed !\n", devid + 1);         	  
         	  dnp->connectStatus = RESET;
              close (dnp->SocketFd) ;
           	  dnp->SocketFd =  -1 ;    
 			  pause(500);                 
        }

	    if(oldsec == rtc->sec)  continue;
        oldsec = rtc->sec;


 //       printf("update tick\r\n");

        /* -------------------------------------------- */
        /* 장치별 통신상태 Check : Device Com rate ...  */
        /* -------------------------------------------- */
        if(dnp->comSndCount >= 1000)
        {
            dnp->oldRcvCount = dnp->comRcvCount;
            dnp->comSndCount = 0;
            dnp->comRcvCount = 0;
        }

	}
	
	ThreadActive[devid] = 0;
	
	printf("exit modbus task\r\n");
	
	return (0);
	
}

/*
*/
int  make_tcpip_modbus_Call(int scanid, SCAN_CONFIG *scan)
{
    int         i;
    int         devid;
    int         index, state;
    DNP_DEVICE      *dnp;
    
    /* 등록된 계전기별 Thread 호출...       */
    for(i=0; i< scan->devNum; i++)
    {
        devid = scan->device[i] & 0x3f;
        if(devid >= MAX_MODULE) continue;
            
        dnp = (DNP_DEVICE *) dnpcfg[devid];
        
        dnp->modbusRcvFail = 0;	
        
        if(dnp->scan == 0)  continue;
        
        printf("scan%02d> index=%d, MODBUS-TCPIP(%02d) Task Call... !\n", scanid+1, i, devid+1);
                            
        MOD_THREAD[devid].tid        = 0;
        sprintf(MOD_THREAD[devid].name, "%s%d","IED", devid+1);
        
        MOD_THREAD[devid].priority   = 0;
        MOD_THREAD[devid].options    = 0;       // 비실행 Flag
    
        /* ------------------------------------------------ */
		/* 	ACTIVE 상태에 따라서 Thread 실행 	            */
		/* ------------------------------------------------ */
		if(scan->active == RESET)	continue;
        
        MOD_THREAD[devid].entryPt = scan_tcpip_modbus;  		// TCPIP 방식 MODBUS-THREAD			    
    			    
        MOD_THREAD[devid].options    = SET;	

	    MOD_THREAD[devid].arg        = 0;
    
        ThreadActive[devid] = SET;
    
      	index = devid;      // Thread Index #1
        state = pthread_create( &MOD_THREAD[devid].tid, NULL, MOD_THREAD[devid].entryPt, &index);
       	if(state != 0)
        {
	        printf("scan> *IED(%2d) Thread# [%s] Create Error ...!\n", devid+1, MOD_THREAD[devid].name);
    	    termExec = RESET;
            //exit(0);
        }
	    pause(100);
    }
    
    return (0);
    
} 

/* -------------------------------------------------------- */
/*  SCAN THREAD Start Routine ....                          */
/* -------------------------------------------------------- */
void    *scan_modbus_thread_TCP(int  *arg)
{
    int     i,scanid;
    int		comPort;
    int     devid=0, devIndex;
    int     chanComFail;
    int     chanReset;
    //int     txcnt;
    //int     rxcnt, retVal;
    int     oldsec=0;
    //int     reqTimeTick;
    int		taskDelay;
    //int		rcvAddr;
    //int     startDev, stopDev;
    //int     modTimeout, timeout;
    int     chanFailTick;
    //byte    *rxbuf, *txbuf;
    char    buffer[128];
    
    DNP_DEVICE      *dnp;
	SCAN_CONFIG     *scan;
	//TTY_DESC        *scanPort;
	
	/* -------------------------------------------- */
	/* THREAD Argment Assign ...                    */
	/* -------------------------------------------- */
	scanid = (int ) *arg;
	
    /* SCAN-TASK 별 모듈구성 정보 */ 
    scan = (SCAN_CONFIG *) scanCfg[scanid];
    comPort = scan->comPort;
    
    //txbuf = (byte *) &scan->sndBuffer[0];
    //rxbuf = (byte *) &scan->rcvBuffer[0];

    taskDelay = scan->comDelay * 10;
    if(taskDelay < 10)		taskDelay = 10;
    if(taskDelay > 2000)	taskDelay = 2000;	

    printf(" %s SCAN[%d] MODBUS-TCPIP PROCESS Activated ...(COM-delay = %d ms, Offline Count=%d) !\n", TARGET_NAME, scanid, taskDelay, scan->offCount);

    sprintf(buffer, "%s SCAN[%d] MODBUS-TCPIP PROCESS Activated ...(COM-delay = %d ms, Offline Count=%d) !", TARGET_NAME, scanid, taskDelay, scan->offCount);
    LogFile_MPU ( opr, NOT_DEFINED, buffer, strlen(buffer));
    
#if 1            
    printf("MOD-TCP-SCAN[%d] device Num = %02d ... ", scanid, scan->devNum );
    for(i=0; i< scan->devNum; i++)  printf(" %2d", scan->device[i]);
    printf("\n");
#endif

    /* -------------------------------------------------------- */
    /* 등록된 Device 가 없는 경우                               */
    /* -------------------------------------------------------- */
    while((scan->devNum == 0) || (scan->active == 0))   
	{
	    if(termExec == 0)   break;

        if(opr->wdtDebug)	Debug(console,"SCAN%2d> *** No Assign DEVICE ...!\n", scanid);
        opr->scanWDT[scanid] = 0;
        opr->scanComSts[scanid] = 2;         // 미정의

        pause(1000);
	}

	/* -------------------------------------------------------- */
    /* 계전기별 통신 Task 호출...                               */
    /* -------------------------------------------------------- */           
    make_tcpip_modbus_Call(scanid, scan); 
    
    devIndex    = 0;
    chanComFail = 0;
    chanReset   = 0;

	while(termExec)
	{
	    /* ---------------------------------------- */
	    /*  계전기별 통신 지연                      */
	    /* ---------------------------------------- */
	    opr->scanWDT[scanid] = 0;
	    pause(100);

		/* ---------------------------------------------------- */
		/*	CPU 이중화 : LOCAL-SLAVE ... 대기모드 				*/
		/* ---------------------------------------------------- */
		if(opr->runMode == LOCAL_SLAVE)		
		{
			continue;
		}

        if(oldsec == rtc->sec)  continue;
        oldsec = rtc->sec;

        /* ---------------------------------------- */
        /* 해당 채널상 : 전체 통신라인 이상시....   */
        /* ---------------------------------------- */
        chanFailTick = SET;
        for(i=0; i< scan->devNum; i++)    
        {
            devid = scan->device[i] & MAX_MODULE_MASK;
            dnp = (DNP_DEVICE *) dnpcfg[devid];
            
            //Debug(console," %2d: check device(%d) online... %d\n", scanid+1, devid+1, dnp->online);
            if(dnp->online == SET)  
            {
                chanReset    = 0;
                chanFailTick = RESET;
            }
        }
        
        /* ---------------------------------------- */
        /* RS485 통신라인에 대한 재 초기화 ...      */ 
        /* ---------------------------------------- */
        if((chanFailTick == SET) && (++chanReset > CHANNEL_TIMEOUT_TICK))
        {
            chanReset = 0;
            opr->scanComSts[scanid] = 0;
            
            //Debug(console," %2d: check SCAN online... \n", scanid);
            
            /* ------------------------------------ */
            /* 해당 채널 : 전체 통신라인 이상시...  */
            /* ------------------------------------ */
            if((opr->dualCpuSts == SET) && (scuCfg->remoteMode == AUTO_MODE))
            {    
                if((scan->changeMode == CHANGE_ALL_DEVICE) && (++chanComFail > 5) && (opr->cpuChangeTick < CHANGE_CPU_TICK))
                {
                    chanComFail    = 0;
                    opr->cpuChange = SET;
                    logEvent1(opr, sque, EVENT_CHANGE_CPU, CHANGE_ALL_DEVICE, 0, 0, opr->cpuMode);   
                    logHistoryQue1(opr, hque, EVENT_CHANGE_CPU, CHANGE_ALL_DEVICE, 0, 0, opr->cpuMode);   
                    
                    /* -------------------------------- */
                    /* LOG File 저장                    */
                    /* -------------------------------- */
                    sprintf(buffer, "[MODBUS-TCPIP] 개별장치 통신절체...Device=%d, changeTick=%d ", devid+1, opr->cpuChangeTick);
                    LogFile_MPU (opr, EVENT_CHANGE_CPU, buffer, strlen(buffer));  
                        
                    pause(1000);
                    
                    opr->cpuChangeTick++;

                    if(opr->wdtDebug)
                    {    
                        Debug(console,"\n--------------------------------------------\n");
                        Debug(console,"scan %2d> %02d:%02d:%02d *** TCPIP-MODBUS Channel Com-Fail... CPU Change (%d)...!\n", scanid, rtc->hour, rtc->min, rtc->sec, opr->cpuChangeTick);
                        Debug(console,"--------------------------------------------\n");
                    }
                            
	                for(i=0; i< scan->devNum; i++)    
    	            {
        	            devid = scan->device[i] & MAX_MODULE_MASK;
            	        dnp   = (DNP_DEVICE *) dnpcfg[devid];
                	    dnpFlagClear(devid);
                        
                    	/* OFFLINE 이벤트 발생... */
                    	dnp->online = 2;
                	}
              	}      
            }
            
        }  

        
	}/* while */
    
    return (0);
}
