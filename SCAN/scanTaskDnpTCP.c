
#include 	"includes.h"
#include	"localLib.h"
#include    "external.h"

extern  int     storeAI_DNP(int devid, int point, float fdata, byte *lowdata);
extern  int     storeAI_FLOAT_DNP(int devid, int point, float fdata, byte *lowdata);

extern  int     curStore_BIT_DNP( int devid, byte *stsbuf, int dataSize);
extern  int     storeSOE_CU_DNP( int devid, int devPoint, int state, byte *buf);
extern  int     storeCOS_CU_DNP( int devid, int point, int state);

extern  int     setOffline( int devid);
extern  int     setOnline( int devid);

extern  int     resetDataLink( int mode, int scanid, int devid);
extern  int     dnpIINWrite(int scanid, int devid);
extern  int     dnpTimeSync(int scanid, int devid);
extern  int     dnpSelectWrite(int scanid, int devid);
extern  int     dnpOperateWrite(int scanid, int devid);
extern  int     dnpAoV2SelectWrite(int scanid, int devid);
extern  int     dnpAoV2OperateWrite(int scanid, int devid);
extern  int     dnpClassRead(int scanid, int devid, int classid);
extern  int     dnpBinaryInput(int scanid, int devid);
extern  int     dnpAnalogInput(int scanid, int devid);
extern  int     dnpCountInput(int scanid, int devid);
extern  int     dnpBinaryChange(int scanid, int devid);
extern  int     sendAppFrame(int mode, int scanid, int devid, DNP_APP_FRAME *sndAppDnp);
extern  int     _initNetClientSCAN(int scanid, int devid, int actChanID);
extern  int     _reConnectClientPLC(int scanid, int devid, int actChanID);
extern  int     chkRcvFrame ( int mode, int scanid, int devid, DNP_DATA_LINK *dnpData);
extern  int     checkDataLink(int mode, int scanid, int devid, DNP_DATA_LINK *dnpData);
extern  int     dnpFlagClear( int devid);


extern  TTY_DESC scanChan[MAX_CPUPORT];


extern  int             termExec;
extern  TASK_INFO	    *taskPtr;

extern  PDPDTIME_DATE_TIME      dnpRTC;
extern  PDPDTIME_MS_SINCE_70    dnpRTCInfo;

extern  THREAD_ENTRY    MOD_THREAD[MAX_MODULE];
extern  int     ThreadActive[MAX_MODULE];


// hkkim
int scan_tk_connect(int scanid, int devid, int actChanID)
{
    //struct sockaddr_in	srvAddr;
    //int     so_reuseaddr = 1;	
	int retVal ;
    int     flag = 1;
    long    lflag ; 
//    time_t now  ;
       
    char curTime[80] ;   
    
    DNP_DEVICE      *dnp;

    /* --------------------------------------- */
    /* CLIENT 프로세서에 해당하는 구조체 ... */
    /* --------------------------------------- */
    dnp = (DNP_DEVICE *) dnpcfg[devid];


    // 2025-08-24 오후 4:20:08 
    if (dnp->SocketFd != -1)    
    {       	 		
		Debug(console,"Err-Dev[%02d]> socket[%d] already opened \n", devid+1, dnp->SocketFd);	    	
    	return -ERROR ; 
    }	


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
//        if((opr->dnpDebug == devid) || (opr->scanDebug == scanid))
//        Debug(console,"Scan-init> dev=%2d, Master IP - Network Init : %s\n", devid+1, dnp->ipString); 
//
          dnp->serv_addr.sin_addr.s_addr = inet_addr(dnp->ipString);	    /* IP conversion & assign */;
    }
    else if(actChanID == 1)     // Slave IP
    {
//        if((opr->dnpDebug == devid) || (opr->scanDebug == scanid))
//        Debug(console,"Scan-init> dev=%2d, SLAVE  IP - Network Init : %s\n", devid+1, dnp->ipString2); 
         dnp->serv_addr.sin_addr.s_addr = inet_addr(dnp->ipString2);	    /* IP conversion & assign */;
    }                                                                          
    /* 소켓의 Open : IPv4 기반, 연결지향형 소켓, TCP기반  */
    /* 내부적으로 Delay 및 timeout 수행 */
	if (( retVal = tk_connect (AF_INET, SOCK_STREAM, IPPROTO_TCP ,
					(const struct sockaddr *) &dnp->serv_addr, sizeof(dnp->serv_addr),
					2000, 2, 500, &dnp->SocketFd )) < 0 )
	{
	      if((opr->dnpDebug == devid) || (opr->scanDebug == scanid))
         //  	Debug(console,"Err-Dev[%02d]> Socket Open Failed...\n", devid+1);	
          {
           tk_print_curTime(curTime, 80) ;            
           Debug(console,"Err> Dev[%02d] : Socket[%d], %s connection to %s:%d Failed\r\n",
            	 	 devid+1, dnp->SocketFd, 
            	 	 (actChanID == 0)? "MASTER":"SLAVE",            	 	             	 	 
            	 	 inet_ntoa( dnp->serv_addr.sin_addr ),
            	 	 dnp->netPort);	
           Debug(console,"%s at %s\r\n",strerror(errno), curTime);          
         }       
		   return(ERROR);
	}

#if 0	
    if((opr->dnpDebug == devid) || (opr->scanDebug == scanid))
    {
        Debug(console,"------------------------------------------ \n");
        tk_print_curTime(curTime, 80) ;
        Debug(console,"Dev[%02d]> socket[%d] Open at %s\n", devid+1, dnp->SocketFd,curTime );
    }
#endif     
        if((opr->dnpDebug == devid) || (opr->scanDebug == scanid))
        {
           tk_print_curTime(curTime, 80) ;            
           Debug(console,"Dev[%02d] : Socekt[%d] Ch[%s] connected to %s:%d OK. %s\n",
            	 	 devid+1,dnp->SocketFd, 
            	 	 (actChanID == 0)? "MASTER ":"SLAVE", 
            	 	 inet_ntoa( dnp->serv_addr.sin_addr ),
            	 	 dnp->netPort, curTime);	
//	        else if(actChanID == 1) 
//	        	Debug(console,"Dev[%02d]: Ch[%d] connect to IP=%s:%d  OK...socket=%d %s\n",
//				devid+1,actChanID, dnp->ipString2, dnp->netPort,dnp->SocketFd,curTime);	
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
    	
#if 0
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
#endif     	
    {
		lflag = fcntl(dnp->SocketFd, F_GETFL, 0);    	
 		lflag |= O_NONBLOCK;
    
    	if (fcntl(dnp->SocketFd, F_SETFL, lflag) == -1 )
    	{
        	Debug(console,"Scan-init> Dev[%02d] set SockFd NONBLOCK failed %s\r\n", devid+1, strerror(errno));
        	return -1;
    	}        
        

 
        /* -------------------------------- */
        /*  DNP 통신의 경우.... Flag Reset  */
        /* -------------------------------- */       
      //   dnp->sndLinkSts = SET;
      // TODO
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


/***************************************************
*   FUNCTION : readDataLink()
*   receive DATA-LINK with CRC , datalink는 최대 255 byte 이다.
*
*  return : 정상시 수신된 data  byte 수
			EOF
			SYS_ERROR
			TIMEOUT
			DNP_PROTOCOL_ERROR
			
*
***************************************************/
int readDataLink_TCP( int scanid, int devid, DNP_DATA_LINK *dnpData)
{
    int     i, size;
    int     rxcnt;
    int     result;
    int     length;
    int     frameCnt,left;
    int     dataLen;
    int     desAddr, srcAddr;
    byte    *rxbuf;
    word    crc;
    word    timeout;
    DL_FRAME    *rcvData;
    DNP_DEVICE  *dnp;

	int rxReqSize ;

    /* --------------------------------------- */
    /* CLIENT 프로세서에 해당하는 구조체 ... */
    /* --------------------------------------- */
    dnp = (DNP_DEVICE *) dnpcfg[devid];
    
    if(dnp->connectStatus == RESET)
    {
        Debug(console,"dev%02d>  readDataLink_TCP Logic Error  Socekt[%d] \n", devid+1, dnp->SocketFd);	
        return TK_AI_ERR;              
    }
    
    timeout = 0;
    rcvData = (DL_FRAME *) &dnpData->rcvFrame;

    rxcnt = 0;
    rxbuf = (byte *) &rcvData->start1;
    
    bzero( &rcvData->start1, TK_DNP_LINK_HEADER_SIZE  ) ;    

#if 0

    /* ---------------------------- */
    /*  receive STX(0x0564)         */
    /* ---------------------------- */
RCV_START_TCP:
    do{
		if((result =TKreadn(dnp->SocketFd, (char *) &rxbuf[0], 1, 1000)) < 0)
    	{
	        if(result == -1) /* timeout */
	        {
	            if((opr->dnpDebug == devid) || (opr->scanDebug == scanid))
                Debug(console,"dev%02d> *Header Read Timeout [%d]... \n", devid+1, result);			     			        
	        }
            else
            {
                if((opr->dnpDebug == devid) || (opr->scanDebug == scanid))
		        Debug(console,"dev%02d> *Header Read fail [%d][%s] \n", devid+1, result, strerror(errno));
            }
            return result;    
        }
        
        if(++timeout > 50)	return (-1);
       	//usleep(100);
        
    }while(rxbuf[0] != 0x05);
#else 

    // 사실 의미가 없다.  tk_net_readn이 한번에 10개를 읽어야지 아니면 timeout 이나 error
    // STEP1  read link header(10byte)    
    rxReqSize = TK_DNP_LINK_HEADER_SIZE ;
    // 일단 1.5초//
    // do_while 문 필요 없네... 그냥 시간을 많이 주면 되네...
    do{
    	// 4가지 응답  1. UserData  2. EOF  3. Timeout  4. SysErr
		if((result = tk_net_readn(dnp->SocketFd, (char *) &rxbuf[0], rxReqSize, 2000)) <=0)
    	{
	        // step 1  timeout
	        if(result == TK_TIMEOUT ) /* timeout -1 */
	        {
	            if((opr->dnpDebug == devid) || (opr->scanDebug == scanid))
                Debug(console,"dev%02d> DNP MAGIC Number Read Timeout [%d]... \n", devid+1, result);		
                return TK_TIMEOUT ;  	     			        
	        }

	        // step2  EOF, 0
	        if(result == TK_SOCKET_EOF ) 
	        {
	            // if((opr->dnpDebug == devid) || (opr->scanDebug == scanid))
                Debug(console,"dev%02d> readDataLink_TCP : Peer close retVal[%d]... \n", devid+1, result);			     			        
	        	return TK_SOCKET_EOF ; 
	        }
            else // system error 
            {
                //if((opr->dnpDebug == devid) || (opr->scanDebug == scanid))
		        Debug(console,"dev%02d> Socket Err [%d][%s] \n", devid+1, result, strerror(errno));
		        return TK_SYS_ERR ; 
            }           
        }
        
        rxReqSize = rxReqSize - result ; 

//        if(++timeout > 50)	return (-1);        
        if(++timeout > 5)	return TK_TIMEOUT ;
       	//usleep(100); tk_net_readn에서 sleep 한다.

    }while( rxReqSize !=0 );
    

    // STEP2  Check 0x0564  and address 
    if ( rxbuf[0] != 0x05 || rxbuf[1] != 0x64)
    {	
         Debug(console,"dev%2d> DNP MAGIC Number Err [%02X:%02X]\n", devid+1, rxbuf[0], rxbuf[1]);	    
    	return TK_DNP_PROTOCOL_ERR ; 
    }

#endif 



#if 0  
{
      if(result == 0)	return 0;

    if((result =TKreadn(dnp->SocketFd, (char *) &rxbuf[1], 1, 100)) < 0)	return (-1);
    if(rxbuf[1] != 0x64)    goto    RCV_START_TCP;
    rxcnt = 2;

    /* read Header */
    if((result =TKreadn(dnp->SocketFd, (char *) &rxbuf[rxcnt], 8, 100)) != 8)	
    {
        if((opr->dnpDebug == devid) || (opr->scanDebug == scanid))
        Debug(console,"dev%02d> *Header-Tail Read fail [%d] \n", devid+1, result);    
        return (-1);
    }
 }   
#endif     
    /* ------------------------ */
    /*  check LINK HEADER CRC   */
    /* ------------------------ */    
    if(B013_ckdnpcrc(&rxbuf[0], 10) != 0)
    {
        crc = B013_mkdnpcrc( &rxbuf[0], 8);
        if((opr->dnpDebug == devid) || (opr->scanDebug == scanid))
        {
            Debug(console,"dev%2d> DNP LINK header - crc error cal=%04x, rcv=%02x-%02x!\n",
                devid+1, crc, rxbuf[8],rxbuf[9]);
            //prn_rcv("rcv-CRC: ", &rxbuf[0], 10);
            DumpDNP_rcv(console, "rcv-CRC:", &rxbuf[0], 10);
        }
        return (TK_DNP_PROTOCOL_ERR);
    }

    /* ------------------------ */
    /*  check DNP-ADDR ...      */
    /* ------------------------ */
    desAddr = rcvData->desAddr[0] + (rcvData->desAddr[1]*256);
    srcAddr = rcvData->srcAddr[0] + (rcvData->srcAddr[1]*256);
        
    if(desAddr == 0xffff)
    {
        if((opr->dnpDebug == devid) || (opr->scanDebug == scanid)) 
        Debug(console,"dev%2d> Broadcast Addr .. !\n", devid+1);
        dnpData->broadCast = RESET; // ????
        return ( TK_DNP_BROADCAST); // 2025-10-10 오후 2:15:12 
    }
    else
    {
        dnpData->broadCast = RESET; // ????

        if(desAddr != dnp->hostid)
        {
            if((opr->dnpDebug == devid) || (opr->scanDebug == scanid)) 
            {
                Debug(console,"rcv%2d> *1 des Addr failed : rcv %4x, host %4x.. !\n", devid+1, desAddr, dnp->hostid);
                //prn_rcv("*rcv: ", &rxbuf[0], 10);
                DumpDNP_rcv(console, "*rcv: ", &rxbuf[0], 10);
            }
            return (TK_DNP_PROTOCOL_ERR);
        }
        else if(srcAddr != dnp->ioid)
        {
            if((opr->dnpDebug == devid) || (opr->scanDebug == scanid))
            {
                Debug(console,"rcv%2d> *1 src Addr failed : rcv %4x, fru %4x.. !\n", devid+1, srcAddr, dnp->ioid);
                //prn_rcv("*rcv: ", &rxbuf[0], 10);
                DumpDNP_rcv(console, "*rcv: ", &rxbuf[0], 10);
            }
            return (TK_DNP_PROTOCOL_ERR);
        }
    }

//NEXT_PROCESS_TCP:    
    rxcnt  = 10;                /* user data */
    length = rxbuf[2] - 5;      /* except control, src, des addr (5byte) */


    if(length == 0)     
    {
	    /* ------------------------------------ */
	    /*  1) if CONFIRM frame received ...    */
	    /*  2) if reset LINK frame received ... */
	    /* ------------------------------------ */

        if((opr->dnpDebug == devid) || (opr->scanDebug == scanid)) 
        Debug(console,"dev%2d> No DNP_APP_DATA, Link  Length = %d\n", devid+1, length);
        return (rxcnt);
    }
    else if(length > 255)     
    {
        //if(opr->hostDebug == hostid)  
        Debug(console,"dev%2d> DNP LINK HEADER - Invalid Length = %d \n", devid+1, length);
        return (-1);
    }
    
    
    
    frameCnt = (length / FRAME_LENGTH);
    left     = (length % FRAME_LENGTH);
    if(left)    frameCnt = frameCnt + 1;

    /* Multi-user data block : execpt CRC ... */
    for(i = 0; i < frameCnt; i++) // read max 16 bytes
    {
        if(length <= 0 )    break;

        if(length >= FRAME_LENGTH)  dataLen = FRAME_LENGTH;
        else                        dataLen = length;

#if 0
        /* read USER data  */
        if((size =TKreadn(dnp->SocketFd, (char *) &rxbuf[rxcnt], dataLen+2, 100)) != (dataLen+2))	
        {
            if((opr->dnpDebug == devid) || (opr->scanDebug == scanid))
            Debug(console,"rcv%2d> 1. rcv Time out  ... req=%d, rcv=%d \n", devid+1, dataLen+2,size);
            return (TK_DNP_PROTOCOL_ERR);
        }
#else
        /* read USER data  */
        // time 100->1000
        if((size =tk_net_readn(dnp->SocketFd, (char *) &rxbuf[rxcnt], dataLen+2, 2000)) != (dataLen+2))	
        {

	        // step 1  timeout
	        if(size == TK_TIMEOUT ) /* timeout */
	        {
	            if((opr->dnpDebug == devid) || (opr->scanDebug == scanid))
                Debug(console,"dev%02d> *link body Read Timeout [%d]... \n", devid+1, size);		
                return TK_TIMEOUT ;  	     			        
	        }
	        			
	        // step2  EOF 
	        if(size == TK_SOCKET_EOF ) 
	        {
	            // if((opr->dnpDebug == devid) || (opr->scanDebug == scanid))
                Debug(console,"dev%02d> *Peer close socket  [%d]... \n", devid+1, size);			     			        
	        	return TK_SOCKET_EOF ; 
	        }
	        
            else // system error 
            {
                //if((opr->dnpDebug == devid) || (opr->scanDebug == scanid))
		        Debug(console,"dev%02d> Socket Err [%d][%s] \n", devid+1, result, strerror(errno));
		        return TK_SYS_ERR ; 
            }

        }

#endif 


        if(B013_ckdnpcrc(&rxbuf[rxcnt], size) != 0)
        {
            crc = B013_mkdnpcrc( &rxbuf[rxcnt], dataLen);
            if((opr->dnpDebug == devid) || (opr->scanDebug == scanid)) 
            Debug(console,"rcv%2d> *user data [%d] - crc error cal=%04x, rcv=%02x-%02x!\n",
                devid+1, i+1, crc, rxbuf[rxcnt + dataLen], rxbuf[rxcnt + dataLen + 1]);
            rxcnt = rxcnt + dataLen;
            return (TK_DNP_PROTOCOL_ERR);
        }

        rxcnt  = rxcnt + dataLen;   /* include crc-data */
        length = length - dataLen;
    }

    return (rxcnt);

}

/*
*   TCPIP-DNP 송신메세지 구성
*
*/
int makeCmdRELAY_TCP(int mode, int scanid, int devid)
{
    int     retVal;

    DNP_DEVICE      *dnp;
    DNP_APP_FRAME   *sndAppDnp;
    SCAN_CONFIG     *scan;

    dnp = (DNP_DEVICE *) dnpcfg[devid];
    
    /* SCAN-TASK 별 모듈구성 정보 */ 
    scan = (SCAN_CONFIG *) scanCfg[scanid];
    //sndAppDnp = (DNP_APP_FRAME *) &sndAppFrame[fid];
    sndAppDnp = (DNP_APP_FRAME *) &dnp->sndAppFrame;



	// hkkim
	retVal = -1 ;  

    /* ---------------------------------- */
    /* Offline 시 : check LINK-Status ... */
    /* ---------------------------------- */
    
    if(dnp->sndLinkSts == RESET)
    {    
    	
    	//TODO offlineTick  조사하자..
        if(++dnp->offlineTick > dnp->offCount)     
        {
            dnp->offlineTick = 0;
            if(dnp->online != 0)    
            {
                setOffline(devid);
                //dnp->connectStatus = RESET;
            }
        }
        // printf("SEND LINK RESET\r\n");
        retVal=resetDataLink( mode, scanid, devid);
        
        return ( retVal);
    }

    dnp->sndFlag = RESET;

    if((opr->dnpDebug == devid) || (opr->scanDebug == scanid))
    {
        Debug(console,"\n-----------------------------------------[%02d/%02d-%02d:%02d:%02d]\n", 
        	rtc->month, rtc->day, rtc->hour, rtc->min, rtc->sec);
    }
    
    /* ---------------------------------------- */
    /* DNP APP-Command Handler                  */
    /* ---------------------------------------- */
    // ?? 엥 위에서 RESETT 했잖아..
    if(dnp->sndFlag == RESET)
    {
        if(dnp->devRestart == SET)          dnpIINWrite(scanid, devid);
        else if(dnp->timeSyncReq == SET)    dnpTimeSync(scanid, devid);
        else if(dnp->selectReq == SET)      dnpSelectWrite(scanid, devid);
        else if(dnp->operateReq == SET)     dnpOperateWrite(scanid, devid);
        else if(dnp->anaVOutSelect == SET)  dnpAoV2SelectWrite(scanid, devid);        /* V0    Reset Select */
        else if(dnp->anaVOutOpr == SET)     dnpAoV2OperateWrite(scanid, devid);       /* V0    Reset Operate */ 
        else if(dnp->class1Req == SET)      dnpClassRead(scanid, devid, DNP_CLASS1);
        else if(dnp->class2Req == SET)      dnpClassRead(scanid, devid, DNP_CLASS2);
        else if(dnp->class3Req == SET)      dnpClassRead(scanid, devid, DNP_CLASS3);
        else if(dnp->statusDump == SET)     dnpBinaryInput(scanid, devid);
        else if(dnp->analogDump == SET)     dnpAnalogInput(scanid, devid);
        //else if(dnp->countDump == SET)      dnpCountInput(scanid, devid);        
        else
        {
            /* -------------------------------------------------------- */
    		/*	2024.12.10 DNP 통신시 Change Check 명령 처리					*/
		    /*	=> Change Check 명령을 하지 않음 ... modProfile 을 4로 지정함...	*/
		    /* 	modbusFileNo => PROFILE 4 지정							*/
    		/* -------------------------------------------------------- */
    		if(dnp->modbusFileNo == 3)	dnpBinaryInput(scanid, devid);
    		else						dnpBinaryChange(scanid, devid);	
        }            
        
        /* ------------------------------------ */
        /*  APPLICATION Command Request...      */
        /* ------------------------------------ */
        if(sndAppDnp->sndCount)
        {
            /* 장치별 통신상태 Check : Device Com rate ... */
            if(dnp->comSndCount >= 1000)
            {
                dnp->oldRcvCount = dnp->comRcvCount;
                dnp->comSndCount = 0;
                dnp->comRcvCount = 0;
            }

            /* --------------------------------------- */
            /* 통신모드(TCPIP/RS232) 에따라서 전송 ... */
            /* --------------------------------------- */
            retVal = sendAppFrame( mode, scanid, devid, sndAppDnp);
            
//            printf("sendAppFrame retVa[%d]\r\n",retVal) ;
            // 2025-08-28 오후 7:21:01  hkkim
            // 송신 실패... protocol 상이아니라  socket 상에서
            if ( retVal < 0) 
            		return retVal ;  
            else 
            {  
            	dnp->comSndCount++;
            	dnp->appFailTick = 0;
	       		return retVal ; 
			}	
		}
        else return 0 ; 
    }

#if 0
    /* ---------------------------------------------- */
    /* APP 송신 / 응답 없는 경우 실패... Retry 5회    */
    /* ---------------------------------------------- */
    if(++dnp->appFailTick > 10)
    {
        //dnp->sndLinkSts = RESET;
        dnp->appFailTick = 0;

        dnp->sndFlag    = RESET;    // 송신상태 Flag Clear...
        //Debug(console,"*[%2d] APP send Error...!\n", devid);
    }
#endif

    return (retVal);
}


/* ------------------------------------------------------- */
/*  SCAN THREAD Start Routine ....             */
/* ------------------------------------------------------- */
void    *scan_tcpip_dnp(int  *arg)
{
    int     scanid;
    //int		comPort;
    int     devid;
    int     chanReset;
    int     rxcnt, retVal;
    int     oldsec=0;
    int     reqTimeTick;
    int		taskDelay;
//    int     reConnectCount; // 너의 동작을 유심히 보니...안 하는데.
//    int     errCount;  //  // 2025-09-02 오전 10:08:12 사용안하는 것 같아 제거

    int     offlineTick;

    int     timeout;
//    int     netMasterSET; // 2025-09-02 오전 10:08:12 사용안하는 것 같아 제거

    char    buffer[128];
    
    DNP_DATA_LINK   *dnpData;
    DNP_APP_FRAME   *rcvAppDnp;
    DNP_APP_FRAME   *sndAppDnp;
    DNP_DEVICE      *dnp, *dnp_slave;

	SCAN_CONFIG     *scan;

	
	/* ------------------------------------ */
	/* THREAD Argment Assign ...            */
	/* ------------------------------------ */
	devid = (int ) *arg;                                /* 해당 계전기 번호 ... */
	
	/* ------------------------------------ */
    /* SCAN-TASK 별 모듈구성 정보           */ 
    /* ------------------------------------ */
    dnp = (DNP_DEVICE *) dnpcfg[devid];
    
    scanid = dnp->scanPort;                             /* 계전기가 할당된 SCAN-Port */
    scan = (SCAN_CONFIG *) scanCfg[scanid];
    
    taskDelay = scan->comDelay * 10;
    if(taskDelay < 10)		taskDelay = 10;
    if(taskDelay > 2000)	taskDelay = 2000;	
//    if(taskDelay > 5000)	taskDelay = 5000;	
    	
    /* ------------------------------------------------ */
    /*	2022.01.30 DNP-TCPIP 이중화 처리						*/
    /*	=> 서로다른 Network 구성시... modProfile 을 7로 지정함...	*/
    /* ------------------------------------------------ */
    
    // 2025-10-27 오후 2:00:50 
    //  bzero(dnp->ipString2,16) ;
   
    
    if(dnp->modbusFileNo == 6)
   	{
		// 이 경우 계전기 db 입력시 devid +1에는 id만..넣어야 ㅎ되나..   		
   		dnp_slave = (DNP_DEVICE *) dnpcfg[devid + 1];
   		memcpy(dnp->ipString2,  dnp_slave->ipString, 16);
   	}
   	else 
   	{ 
   	        strncpy( dnp->ipString2,"NO DEV ",16);
   	       // memcpy(dnp->ipString2,"NO DEV", strlen("NO DEV")  );
   	}	 	
    printf("Dev[%02d] TCP Client Thread : SCAN[%d] (IP1:%s IP2:%s, Port=%d) Activated ...(Delay = %d ms, OffCount=%d) !\n",
    	devid+1, scanid+1,  
         dnp->ipString, dnp->ipString2, dnp->netPort, taskDelay, scan->offCount);

	/* -------------------------------------------- */
	/* THREAD Argment Assign ...                    */
	/* -------------------------------------------- */
    dnpData   = (DNP_DATA_LINK *) &dnp->dataLinkFrame;
    sndAppDnp = (DNP_APP_FRAME *) &dnp->sndAppFrame;
    rcvAppDnp = (DNP_APP_FRAME *) &dnp->rcvAppFrame;	
    
    /* 계전기 통신 Channel ... Default */
    dnp->activeChan = 0; 
    dnp->offCount   = scan->offCount;
    offlineTick = 0;


#if 0 // 2025-08-29 오후 12:51:35 hkkim
	/* ------------------------------------ */
    /*  장치별 TCPIP-CLIENT initial...      */
    /* ------------------------------------ */
   	if (_initNetClientSCAN(scanid, devid, dnp->activeChan) == ERROR)
    { 
        if((opr->dnpDebug == devid) || (opr->scanDebug == scanid))
        printf("TCPIP[%02d]> *** Network Initial... Fail...\n", devid+1);	
        ThreadActive[devid] = NO; // 죽으면 안 됨... thread를 살릴 때 YES로...
    }
#endif                 
    chanReset   = 0;
//    errCount    = 0;
    
//    reConnectCount = 0;
//    netMasterSET = SET;

	dnp->connectStatus = RESET	;
	dnp->SocketFd = -1 ;        
    
	while(ThreadActive[devid])
	{
//	    pause(taskDelay);
		msleep(taskDelay);
        /* ---------------------------------------------------- */
		/*	CPU 이중화 : LOCAL-SLAVE ... 대기모드 				*/
		/* ---------------------------------------------------- */
		if(opr->runMode == LOCAL_SLAVE)		
		{
		    /* ---------------------------------------- */
			/*	2017.06.27 SLAVE 모드 전환시 소켓 Close */
			/* ---------------------------------------- */
            if(dnp->connectStatus == SET)
            {
                Debug(console,"Dev[%02d] DNP-TCP Client change to SLAVE. Socket Close(%d)!\n", devid+1, dnp->SocketFd);
			    dnp->connectStatus = RESET;
                close( dnp->SocketFd);  
				dnp->SocketFd = -1 ;    
            }
                        
			/* ---------------------------------------- */
			/*	SLAVE 모드에서 수신된 데이터를 처리함   */
			/* ---------------------------------------- */
			dnp->connectStatus = RESET; //?
			dnp->connectFail = 0;
//			reConnectCount   = 3;  
//			netMasterSET     = 0;

//          dnp->online = RESET; 이건 걸들지 않네, RESET이 되면 다음에 master가 될 때 online 이벤트올라간다

			pause(500);
			continue;
		} 
#if 0		
		else
        {
            if(netMasterSET == 0)
            {
                Debug(console,"dev%02d> *CPU Mode Change => MASTER ... !\n", devid+1);
                opr->scanRestart = SET;
                pause(1000);
            }
                    
            netMasterSET = SET;
        }		    
#endif

        /* -------------------------------------         */
	    /*  접속실패에 따른 재접속...아니고 연결이 없으면 연결시도       */
	    /* -------------------------------------          */
	    if(dnp->connectStatus == RESET)
	    {
#if 0	 	    	
            if((opr->dnpDebug == devid) || (opr->scanDebug == scanid))  Debug(console,"dev%02d> * DNP-TCP: retry Connect....!\n", devid+1);                
            _reConnectClientPLC(scanid, devid, dnp->activeChan);  
                              
            //reConnectCount = 3;     // 재접속시간을 최소화...
            pause(500);
                
            if(++reConnectCount > 5)
            {
                reConnectCount = 0;
                //close( dnp->SocketFd);  

                if((opr->dnpDebug == devid) || (opr->scanDebug == scanid))   Debug(console,"dev%02d> * RE Initial...CLIENT....!\n", devid+1);

                /* ---------------------------------------- */
                /* 계전기 이중화에 따른 IP 구성정보 변경    */   
                /* ---------------------------------------- */                 
                if(dnp->dualIED == SET)		dnp->activeChan = (dnp->activeChan + 1) & 0x01;
                else						dnp->activeChan = 0;
                    
                /* ---------------------------------------------------- */
                /*  2025-01-13 장치별 Offline 판정...: Net-Initial 5회이후		*/
                /* ---------------------------------------------------- */       
                if(++offlineTick > 3)  // dnp->offlineTick 과 중복인가 ?
                {
                    offlineTick = 0;
          	        if(dnp->online != RESET)  
                    {
                        if((opr->dnpDebug == devid) || (opr->scanDebug == scanid))
                        Debug(console,"dnp-tcpip> *dev(%02d)...offline...!\n", devid + 1);

                        setOffline(devid);
                        dnpFlagClear(devid);
                        dnp->online = RESET;
                    }
                }
                    
                /* ---------------------------------------- */
                /*  장치별 TCPIP 초기화...                  */
                /* ---------------------------------------- */
                if(_initNetClientSCAN(scanid, devid, dnp->activeChan) == ERROR)
                { 
                    if((opr->dnpDebug == devid) || (opr->scanDebug == scanid))  Debug(console,"dev%02d> *** Target No Response Re-Initial... Fail...\n" , devid+1);	
                }
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
                            sprintf(buffer, "[TCPIP-DNP] 개별장치 통신절체...Device=%d, changeTick=%d ", devid+1, opr->cpuChangeTick);
                            LogFile_MPU (opr, EVENT_CHANGE_CPU, buffer, strlen(buffer)); 
                     
                            pause(1000);
                        
                            opr->cpuChangeTick++;
                        
                            if(opr->wdtDebug)
                            {   
                                Debug(console,"\n--------------------------------------------\n");
                                Debug(console,"scan %2d> %02d:%02d:%02d *** TCPIP-DNP Device scan Fail... CPU Change (%d)...!\n", scanid+1, rtc->hour, rtc->min, rtc->sec, opr->cpuChangeTick);
                                Debug(console,"--------------------------------------------\n");
                            }
                        
                            /* OFFLINE 이벤트 발생... */
                            dnp->online = 2;
                        }
                       
                    }
                        
                    dnp->connectStatus = RESET;
                }  
           }
           
#endif 
		   	if (scan_tk_connect(scanid, devid, dnp->activeChan) < 0 )
		    { 
		        /* Connection 이 실패했다는 건   상대방이 없다...그러니 좀 쉬었다. */
				/* 또는 바로 RST ... */

  
                // dualIED시 offlineTick 주기조정
                if(++offlineTick > 3) /* 처음 online 으로 살기 때문에 3회 connect 실패후..offline 처리 */
                {
                    offlineTick = 0;
          	        
          	        if(dnp->online != RESET)  
                    {
                        if((opr->dnpDebug == devid) || (opr->scanDebug == scanid))
                        Debug(console,"Dev[%02d]> offline...!\n", devid + 1);

                        setOffline(devid);
                        dnpFlagClear(devid);
                        dnp->online = RESET;
                    }
                	// 2025-08-26 오후 3:34:11  죽으면 안 됨 commnet out 
                	// ThreadActive[devid] = NO;
                }
				// 일단 offlineTick 보다는 커야지 말이 도지ㅣ..
                /* -------------------------------------------- */
                /* 계전기별 통신이상에 따른 Offline 처리        */
                /* -------------------------------------------- */
                if(++dnp->comFailTick > scan->offCount)
                 {
                    dnp->comFailTick = 0;

                    /* ---------------------------------------- */ 
                    /* 개별 장치별 Offline 상태 : CPU 절체      */
                    /* ---------------------------------------- */ 
                    //
                    // 2025-09-02 오전 10:39:43 일단 master일때만 여기오니까.. runMode 확인제거
                    if( (scan->changeMode == CHANGE_EACH_DEVICE))
                    {
                    	//일단  cpuChangeTick 보는 것 막음..다시 원복
                        if((opr->dualCpuSts == SET) && (scuCfg->remoteMode == AUTO_MODE) && (opr->cpuChangeTick < CHANGE_CPU_TICK))   
                        {
                            opr->cpuChange = SET;

                        	if((opr->dnpDebug == devid) || (opr->scanDebug == scanid))		        			
		        			printf("DNP-TCP Client-Dev[%02d]> ask cpuChange\n", devid+1);	
		        	                            
                            logEvent1(opr, sque, EVENT_CHANGE_CPU, CHANGE_EACH_DEVICE, 0, 0, opr->cpuMode); 
                            logHistoryQue1(opr, hque, EVENT_CHANGE_CPU, CHANGE_EACH_DEVICE, 0, 0, opr->cpuMode);  
                            
                            /* -------------------------------- */
                            /* LOG File 저장                    */
                            /* -------------------------------- */
                            sprintf(buffer, "[TCPIP-DNP] 개별장치 통신절체...Device=%d, changeTick=%d ", devid+1, opr->cpuChangeTick);
                            LogFile_MPU (opr, EVENT_CHANGE_CPU, buffer, strlen(buffer)); 
                     
                            pause(1000);
                        
                            opr->cpuChangeTick++;
                        
                            if(opr->wdtDebug)
                            {   
                                //???
                                Debug(console,"\n--------------------------------------------\n");
                                Debug(console,"SCAN[%2d] > %02d:%02d:%02d *** TCPIP-DNP Device scan Fail... CPU Change (%d)...!\n",
                                	 scanid+1, rtc->hour, rtc->min, rtc->sec, opr->cpuChangeTick);
                                Debug(console,"--------------------------------------------\n");
                            }
                        
                            /* OFFLINE 이벤트 발생... */
                            dnp->online = 2;
                        }
                       
                    }                                            
                } 

                /* ---------------------------------------- */
                /* 계전기 이중화에 따른 IP 구성정보 변경    */   
                /* ---------------------------------------- */                 
                if(dnp->dualIED == SET)		dnp->activeChan = (dnp->activeChan + 1) & 0x01;
                else						dnp->activeChan = 0;                
                                
		        // hkkim
		        // 
           	    dnp->connectStatus = RESET;
          		
          		if ( dnp->SocketFd > 0 )
               	{  // 여기오면 logic   rror
		        	printf("%s Dev[%d] logic err...\n", __FUNCTION__, devid+1);	               		
               	 	close (dnp->SocketFd) ;          
           	    	dnp->SocketFd =  -1 ;		        		        
		    	}
		        //pause(500);
		    }   
            else   // 2025-08-24 오후 4:28:28 
            {    
            	// 성공...
            	// _initNetClientSCAN 안에 있는 건데  일단 동작 구분을 위해 밖으로
		        /* -------------------------------- */
		        /*  DNP 통신의 경우.... Flag Reset  */
		        /* -------------------------------- */       
		      
		        //  dnp->sndLinkSts = SET; ????
		        // ???
		        //TODO
		        dnp->sndFcvBit  = FCV_BIT;
		        dnp->sndFcbBit  = FCB_BIT;


				dnp->sndLinkSts = RESET ; 		                
		        
		        dnp->thSndSeqno = 0;        
		        dnp->acSndSeqno = 0;
		        dnp->ACseqFail  = 0;
		        
		        /* -------------------------------- */
		        /*  계전기 Socket 연결상태.. SET    */
		        /* -------------------------------- */        
		        dnp->connectStatus = SET;    		                	            		       
	   		}
                            
	        continue;
		}
	    
	    

        /* ---------------------------------------- */
        /* make Command: 보호계전기 송수신 루틴...  */
        /* 보낸 byte 수를 return 한다. <=0 는 error 처리  2025-10-28 오전 1:48:17  */ 
        /* ---------------------------------------- */
        if(makeCmdRELAY_TCP( COM_TCPIP, scanid, devid) > 0)
        {
            reqTimeTick = 150;
            reqTimeTick = 250;
            
            /* ------------------------ */
            /* 모듈별 Time out 처리     */
            /* ------------------------ */
            // hkkim
            // for(timeout = 0; timeout < reqTimeTick; timeout++)
            {  // Serial 이 아니므로  for 문은 제거한다.
                usleep(2000);
                // readDataLink_TCP가 정상 수신한 경우, 유효 최소 byte는 10
                // MAX 255 인데 ..broadcaset는 엄청 크게 해서..
                if((rxcnt = readDataLink_TCP( scanid, devid, dnpData)) > 0)
                {
                	// 2025-10-10 오후 2:17:06 
                    if ( rxcnt == TK_DNP_BROADCAST ) 
                    { 	
                    		Debug(console,"dev%02d> RX  DNP LINK LAYER Broadcast...!\n", devid+1);
           	        		dnp->connectStatus = RESET; // ????? broadcast를 받으면 다시 link reset을 받아야 하나 ??                    	
                    	    continue  ;
                    	     
                 	}
                    reqTimeTick = 150;
                	timeout = 0;
                	
//                	errCount    = 0;
                    dnp->comFailTick  = 0;
                    //opr->cpuChangeTick = 0;          
                    opr->scanComSts[scanid] = 1;            // SCAN-PORT 통신상태
                    
                    dnpData->rcvCount = rxcnt;
                    dnp->timeOutTick  = 0;
                	offlineTick = 0;
                
                    if((opr->dnpDebug == devid) || (opr->scanDebug == scanid))  
                    		DumpDNP_rcv(console, "RXD: ", &dnpData->rcvFrame, rxcnt);

                    /* ---------------------------- */
                    /* check Function Code ...      */
                    /* ---------------------------- */
                    retVal = chkRcvFrame(COM_TCPIP, scanid, devid, dnpData);
                                      
                    // hkkim
                    if( retVal < 0)  // write fail 시...
                    {
                     	    if((opr->dnpDebug == devid) || (opr->scanDebug == scanid))
                    					Debug(console,"dev%02d> Err chkRcvFrame retval[%d]!\n", devid+1, retVal);
           	        		dnp->connectStatus = RESET;
                    		close (dnp->SocketFd) ;
           	        		dnp->SocketFd =  -1 ; 
                    	
                    }   
                                                                                                 
                    if((retVal == FUNC_USER_DATA) || (retVal == FUNC_UNCONFIRM))
                    {
                        /* ---------------------------- */
                        /* check Online                 */
                        /* ---------------------------- */
                        if(dnp->online != SET)    
                        {
                            if((opr->dnpDebug == devid) || (opr->scanDebug == scanid))  Debug(console,"tcpip> dev(%02d)  online...!\n", devid + 1);
                            setOnline(devid);
                        }
        
                    	dnp->comRcvCount++;
                    	// FC가 USER_DATA인 경우, Trans header 확인후 응답까지 처리한다 현재는 return 1 항상...
                    	// app confirm등 tx가 있는 것 같은데 TODO로...
                        retVal = checkDataLink(COM_TCPIP, scanid, devid, dnpData);
                        
                       // if(retVal)	break; 위에서 for 제거
                    }
                    else if(retVal == RCV_ACK) // 그럼 여기는 올 수없는 것인가 ...
                    {
                        if((opr->dnpDebug == devid) || (opr->scanDebug == scanid))                       
                       Debug(console,"dev%02d> rcv  LINK-LAYER ACK...!\n", devid+1);

                    }
                }
#if 0   // 2025-10-10 오후 2:18:08   위로이동 
                else if(rxcnt == 0)  // broadcaset
       	        {      	        	
       	            // if((opr->dnpDebug == devid) || (opr->scanDebug == scanid))
                    Debug(console,"dev%02d> Rx DNP LINK LAYER Broadcast...!\n", devid+1);
           	        dnp->connectStatus = RESET; // ????? broadcast를 받으면 다시 link reset을 받아야 하나 ??
                    break;        

                }                
                else if(rxcnt == -1)
       	        {
       	            if((opr->dnpDebug == devid) || (opr->scanDebug == scanid))
                    Debug(console,"dev%02d> *** TCP/IP read ...Packet Error..%d!\n", devid+1, errCount);
	                    
                    if(++errCount > 5)
                    {    
                        dnp->connectStatus = RESET;
                        errCount = 0;
                        break;        
                    }

         	  		dnp->connectStatus = RESET;
              		close (dnp->SocketFd) ;
           	  		dnp->SocketFd =  -1 ;                       
                    
                }
#endif           
                // 0 : EOF
                // TIMEOUT
                // SYS_ERRIR
                //  PROTOCOL ERR    
			    else // readDataLink_TCP failed  EOF or Timeout 
			    {	
       	            if ( rxcnt == 0)
                    	Debug(console,"Dev[%02d]> readDataLink_TCP Peer Closed Socket[%d]\n", devid+1,dnp->SocketFd );       
                    else 
                    {		            	       	           
	       	            if((opr->dnpDebug == devid) || (opr->scanDebug == scanid))
	                    Debug(console,"Err>  Dev[%02d] readDataLink_TCP Error Socket[%d] Closed ! \n", devid+1,dnp->SocketFd );
                	}
           	        dnp->connectStatus = RESET;
                    close (dnp->SocketFd) ;
           	        dnp->SocketFd =  -1 ;   
 					pause(1000);             	        

  
                 }

#if 0                
                else if(rxcnt == -2)
   	            {
       	            if((opr->dnpDebug == devid) || (opr->scanDebug == scanid))
                    Debug(console,"dev%02d> *** TCP/IP un Connected...Failed...!\n", devid+1);
           	        dnp->connectStatus = RESET;
                    break;        
                }
#endif                 
            }/*timeout */ 
           
        }
        else // if makeCmd failed
        {        	        	
       	      if((opr->dnpDebug == devid) || (opr->scanDebug == scanid))
                  Debug(console,"ERR> Dev[%02] MakeCmdRELAY_TCP Failed...!\n", devid+1);   
                        
         	  dnp->connectStatus = RESET;
              close (dnp->SocketFd) ;
           	  dnp->SocketFd =  -1 ;    
 			  pause(500);           	        
           	  continue ;           	        
              //  break;        	        	
        }
        /* ------------------------------------ */
        /* RS485 통신라인에 대한 재 초기화 ...  */ 
        /* ------------------------------------ */      
        
        // 의미 없네... 
        if(oldsec == rtc->sec)  continue;
        oldsec = rtc->sec;
        
	}/* while */

    return (0);
}


/*
*   DNP-TCPIP() 
*   network 인 경우 각 device 마다 socket을 만들어 준다.
*/
int  make_tcpip_dnp_Call(int scanid, SCAN_CONFIG *scan)
{
    int         i;
    int         devid;
    int         index, state;
    DNP_DEVICE      *dnp;
    
    /* 등록된 계전기별 Thread 호출...       */
    for(i=0; i< scan->devNum; i++)
    {
        devid = scan->device[i] & MAX_MODULE_MASK;
        if(devid >= MAX_MODULE) continue;
            
        dnp = (DNP_DEVICE *) dnpcfg[devid];
        
        dnp->tcpipThread = SET;
        
        /* ------------------------------------------------ */
		/* 	ACTIVE 상태에 따라서 Thread 실행 	            */
		/* ------------------------------------------------ */
        if(dnp->scan == 0)  continue;
        
        //printf("scan%02d> index=%d, DNP-TCPIP(%02d) Task Call... !\n", scanid+1, i, devid+1);
                            
        MOD_THREAD[devid].tid        = 0;
        sprintf(MOD_THREAD[devid].name, "%s%d","DNP", devid+1);
        
        MOD_THREAD[devid].priority   = 0;
        MOD_THREAD[devid].options    = 0;       // 비실행 Flag
        MOD_THREAD[devid].entryPt = scan_tcpip_dnp;  		// TCPIP 방식 MODBUS-THREAD			    
        MOD_THREAD[devid].options    = SET;	
	    MOD_THREAD[devid].arg        = 0;
    
        ThreadActive[devid] = SET;
    
      	index = devid;      // 해당 계전기 번호를 연계
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
void    *scan_dnp_thread_TCP(int  *arg)
{
    int     i,scanid;
    int		comPort;
    int     devid=0, devIndex;
    int     chanComFail;
    int     chanReset;
    //int     txcnt;
    //int     rxcnt, retVal;
    int     oldsec=0;   // 어디에 쓰지 ?????
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
    if(taskDelay > 5000)	taskDelay = 5000;	
//    if(taskDelay > 2000)	taskDelay = 2000;	
	

    printf("SCAN[%d] DNP-TCPIP Client Activated ...(COM-delay = %d ms, Offline Count=%d) !\n", 
    	     scanid+1, taskDelay, scan->offCount);
    printf("total devices[%02d] : ", scan->devNum );
    for(i=0; i< scan->devNum; i++)  printf(" %2d", scan->device[i] +1 );
    printf("\n");


    sprintf(buffer, "%s SCAN[%d] DNP-TCPIP PROCESS Activated ...(COM-delay = %d ms, Offline Count=%d) !", 
    	TARGET_NAME, scanid+1, taskDelay, scan->offCount);
    LogFile_MPU ( opr, NOT_DEFINED, buffer, strlen(buffer));
    
            


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
    make_tcpip_dnp_Call(scanid, scan); 
    
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

        opr->devScanCount[scanid]++;
				
        if(oldsec == rtc->sec)  continue;
        oldsec = rtc->sec;

		/* each every one sec */
		
        /* ---------------------------------------- */
        /* 해당 채널상 : 전체 통신라인 이상시....   */
        /* ---------------------------------------- */
        chanFailTick = SET;
        for(i=0; i< scan->devNum; i++)    
        {
            devid = scan->device[i] & MAX_MODULE_MASK;
            dnp = (DNP_DEVICE *) dnpcfg[devid];
            
            if(dnp->online == SET)  
            {
                chanReset    = 0;
                chanFailTick = RESET;
            }
        }
        
        /* ---------------------------------------- */
        /* RS485 통신라인에 대한 재 초기화 ...      */ 
        /* ---------------------------------------- */
        if((chanFailTick == SET) && (++chanReset > CHANNEL_TIMEOUT_TICK)) // CHANNEL_TIMEOUT_TICK = 30 
        {
            /* when 여긴 every 30sec */
            chanReset = 0;
            opr->scanComSts[scanid] = 0;
            
            /* ------------------------------------ */
            /* 해당 채널 : 전체 통신라인 이상시...  */
            /* ------------------------------------ */
            if((opr->dualCpuSts == SET) && (scuCfg->remoteMode == AUTO_MODE))
            {    

                if((scan->changeMode == CHANGE_ALL_DEVICE) && (++chanComFail > 5) &&
                			 (opr->cpuChangeTick < CHANGE_CPU_TICK))
// cpuChangeTick  용도가 뭐지..왔다 갔다 하는 것 막는다고했는데..
             			
                {
                   // chanComFail 에 의해 X 5 -- 150초맏...
                    chanComFail    = 0;

                    if(  (opr->scanDebug == scanid))		        			
		        			printf("SCAN[%02d]> all Devices offline, ask cpuChange\n", scanid+1);	                    

                    
                    opr->cpuChange = SET;
                    logEvent1(opr, sque, EVENT_CHANGE_CPU, CHANGE_ALL_DEVICE, 0, 0, opr->cpuMode);   
                    logHistoryQue1(opr, hque, EVENT_CHANGE_CPU, CHANGE_ALL_DEVICE, 0, 0, opr->cpuMode);   
                    
                    /* -------------------------------- */
                    /* LOG File 저장                    */
                    /* -------------------------------- */
                    sprintf(buffer, "[TCPIP-DNP] 개별장치 통신절체...Device=%d, changeTick=%d ", devid+1, opr->cpuChangeTick);
                    LogFile_MPU (opr, EVENT_CHANGE_CPU, buffer, strlen(buffer)); 

                    pause(1000);
                    
                    opr->cpuChangeTick++;

                    if(opr->wdtDebug)
                    {    
                        Debug(console,"\n--------------------------------------------\n");
                        Debug(console,"scan %2d> %02d:%02d:%02d *** TCPIP-DNP Channel Com-Fail... CPU Change (%d)...!\n", scanid, rtc->hour, rtc->min, rtc->sec, opr->cpuChangeTick);
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
