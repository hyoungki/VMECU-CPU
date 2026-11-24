#include 	"includes.h"
#include	"localLib.h"
#include    "external.h"




extern  int     hostChanRead(int hostid, int chid, byte *rxbuf, int reqCount);

extern  int     hostVMERead(int hostid, int chid, byte *rxbuf);
extern  void    checkCUstatus(int hostid, int chid);
extern  int     sendAppFrame_HOST( int hostid, int chid, int socketID, DNP_APP_FRAME *sndAppDnp);
extern  int     vmeHostInitial(int hostid, int chid);
extern  int     vmeHostChanReset(int hostid, int chid);
extern  int     chkRcvFrameHOST( int hostid, int chid, int socketID, DNP_DATA_LINK *dnpData);
extern  int     checkDataLinkHOST( int hostid, int chid, int socketID, DNP_DATA_LINK *dnpData);
extern  int     checkAPPFrame_HOST(int hostid, int chid, int socketID);

extern  void    sendConfirmHOST( int hostid, int chid, int socketID, int code);

extern  TASK_INFO	    *taskPtr;
extern  int             termExec;

extern  int             wdtHostFlag[MAX_HOST];
extern  byte            vmeRxbuf[MAX_HOST][1024];

/* HOST 통신용 ASYNC PORT */
extern  TTY_DESC tty_HostChan[MAX_CPUPORT];

/*****************************************
* handle response 
* 주어진 socketFd에서  link header 10 byte를 읽고 magic과 CRC검사후
* 만약 body가 있으면 body를 읽어 crc 검사를 한후, 총 link header byte수를 return 한다. 
* return :  1   get vaild response  
*  			0   TCP Connection closed
*           -1  Timeout
*           -2  select system error 
******************************************/
int  hostTCPIP_read(int hostid, int	hostSocketFd, byte	*rxbuf)
{
    int     i;
    int     result;
    int     size;
    int    length;
    int     frameCnt,left;
    int     dataLen;
    int		rxcnt;
    //int     reqSize;
    //byte    lrc;
    word    crc;

    rxcnt = 0;
    
    /* ---------------------------------------- */
    /*  READ MODBUS - Header                    */
    /* ---------------------------------------- */ 
	if((result =TKreadn(hostSocketFd, (char *) &rxbuf[0], 2, 100)) < 0)
   	{
        /*  such as timeout */
		if(result == -1) /* timeout */
    	{
        	//if(opr->hexDebug)
           	//Debug(console,"host%02d> *Header Read Timeout [%d]... \n", hostid, tid, result);			     			        
    	}
       	else
       	{
           	//if(opr->hexDebug) 
	    	//Debug(console,"host%02d> *Header Read fail [%d][%s] \n", hostid, tid, result, strerror(errno));
   	 	}
       	return result;
    }
        
    /* TCP/IP Socket Error 발생시 ... */
	if(result == 0)	return 0;

    if((rxbuf[0] != 0x05) || (rxbuf[1] != 0x64))    return (-1);
    rxcnt = 2;
        
    /* read Header */
	if((result =TKreadn(hostSocketFd, (char *) &rxbuf[2], 8, 100)) < 0)	return (-1);
	
	/*  check HEADER-CRC     */
    if(B013_ckdnpcrc(&rxbuf[0], 10) != 0)
    {
        crc = B013_mkdnpcrc( &rxbuf[0], 8);
        if(opr->hostDebug == hostid)
        {
        	Debug(console,"[%02d:%02d:%02d] ===========>\n", rtc->hour, rtc->min, rtc->sec);
            Debug(console,"hrcv%2d> *header - crc error cal=%04x, rcv=%02x-%02x!\n", hostid+1, crc, rxbuf[8],rxbuf[9]);
            //prn_rcv("hrcv-CRC:", &rxbuf[0], 10);
            DumpDNP_rcv(console, "hrcv-CRC:", &rxbuf[0], 10);
        }
        return (-1);
    }
    
    rxcnt  = 10;  

    length = rxbuf[2] - 5;      /* except control, src, des addr (5byte) */
    
    /* ------------------------------------ */
    /*  Datalink header만                    */
    /*  1) if CONFIRM frame received ...    */
    /*  2) if reset LINK frame received ... */
    /* ------------------------------------ */
    if(length <= 0)      
    {
        if(opr->hostDebug == hostid)  
            Debug(console,"hrcv%2d> no Data Length = %d\n", hostid+1, length);
        return (rxcnt);
    }
    else if(length > 255)     
    {
        //if(opr->hostDebug == hostid)  length가 unsigned byte 이면 255 이상은 못 나온댜.
        Debug(console,"hrcv%2d> *DNP rcv - Invalid Length = %d \n", hostid+1, length);
        return (-1);
    }
    
    frameCnt = (length / FRAME_LENGTH);
    left     = (length % FRAME_LENGTH);
    if(left)    frameCnt = frameCnt + 1;    
	
	/* Multi-user data block : execpt CRC ... */
    for(i = 0; i < frameCnt; i++)
    {
        if(length <= 0 )    break;

        if(length >= FRAME_LENGTH)  
        {
            dataLen = FRAME_LENGTH;
            size = dataLen + 2;
        }
        else                        
        {
            dataLen = length ;
            size    = dataLen + 2;
        }
        
        if((result =TKreadn(hostSocketFd, (char *) &rxbuf[rxcnt], size, 100)) < 0)	
        {
            if(opr->hostDebug == hostid)  
            Debug(console,"hrcv%2d> *Tail not Received.... %d\n", hostid+1, result);
            return (-1);
        }
                
        if(B013_ckdnpcrc(&rxbuf[rxcnt], size) != 0)
        {
            crc = B013_mkdnpcrc( &rxbuf[rxcnt], dataLen);
            if(opr->hostDebug == hostid)
            Debug(console,"hrcv%2d> *user data [%d] - crc error cal=%04x, rcv=%02x-%02x!\n",  hostid+1, i+1, crc, rxbuf[rxcnt + dataLen], rxbuf[rxcnt + dataLen + 1]);
            rxcnt = rxcnt + dataLen;
            return (-1);
        }

        rxcnt  = rxcnt + dataLen;   /* include crc-data */
        length = length - dataLen;
        
    }

    return (rxcnt);
	
	
}

/*****************************************
* 2025-10-10 오후 4:08:02   hostTCPIO_read를 변경  hostTcpipComm에서만 사용 
*handle response 
* 
* return :  1   get vaild response  
*  			0   TCP Connection closed
*           -1  Timeout
*           -2  select system error 
******************************************/
int  tk_hostTCPIP_read(int hostid, int	hostSocketFd, byte	*rxbuf)
{
    int     i;
    int     result;
    int     size;
    int    length;
    int     frameCnt,left;
    int     dataLen;
    int		rxcnt;
    //int     reqSize;
    //byte    lrc;
    word    crc;
    
    int timeout =0;
    int rxReqSize ;

    rxcnt = 0;
    
    bzero( rxbuf , TK_DNP_LINK_HEADER_SIZE  ) ;  
    
 
    // STEP1  read link header(10byte)
    rxReqSize = TK_DNP_LINK_HEADER_SIZE ;
    
    // 일단 1.5초//
    do{
    	// 4가지 응답  1. UserData  2. EOF  3. Timeout  4. SysErr
		if((result = tk_net_readn( hostSocketFd , (char *) &rxbuf[0], rxReqSize, 2000)) <=0)
    	{
	        // step 1  timeout
	        if(result == TK_TIMEOUT ) /* timeout -1 */
	        {
     			if(opr->hostDebug == hostid)
                Debug(console,"hrcv%2d> DNP LINK HEADER  Read Timeout [%d]... \n", hostid+1, result);		
                
 //           Debug(console,"hrcv%2d> *header - crc error cal=%04x, rcv=%02x-%02x!\n", hostid+1, crc, rxbuf[8],rxbuf[9]);
                
                return TK_TIMEOUT ;  	     			        
	        }

	        // step2  EOF, 0
	        if(result == TK_SOCKET_EOF ) 
	        {
	            // if((opr->dnpDebug == devid) || (opr->scanDebug == scanid))
                Debug(console,"hrcv%2d> readDataLink_TCP : Peer close retVal[%d]... \n", hostid+1, result);			     			        
	        	return TK_SOCKET_EOF ; 
	        }
            else // system error 
            {
                //if((opr->dnpDebug == devid) || (opr->scanDebug == scanid))
		        Debug(console,"hrcv%2d> Socket Err [%d][%s] \n", hostid+1, result, strerror(errno));
		        return TK_SYS_ERR ; 
            }
            //return result;    // timout 또는 0x05 못 찾은 경우로 < 0 값.
        }
        
        rxReqSize = rxReqSize - result ; 
//        printf("readDataLink_TCP read %d left %d\r\n",result, rxReqSize ) ;
        // 1 mS 씩 50번 50mS ... 

//        if(++timeout > 50)	return (-1);        
        if(++timeout > 5)	return TK_TIMEOUT ;
       	//usleep(100);

    }while( rxReqSize !=0 );
    
    //printf("Check MAagic\r\n");
    // STEP2  Check 0x0564  and address 
    if ( rxbuf[0] != 0x05 || rxbuf[1] != 0x64)
    {	
         Debug(console,"hrcv%2d> DNP MAGIC Number Err [%02X:%02X]\n", hostid+1, rxbuf[0], rxbuf[1]);	    
    	return TK_DNP_PROTOCOL_ERR ; 
    }

    /* ------------------------ */
    /*  check LINK HEADER CRC   */
    /* ------------------------ */    
    if(B013_ckdnpcrc(&rxbuf[0], 10) != 0)
    {
        crc = B013_mkdnpcrc( &rxbuf[0], 8);
     	if(opr->hostDebug == hostid)
        {
            Debug(console,"hrcv%2d> DNP LINK header - crc error cal=%04x, rcv=%02x-%02x!\n",
                hostid+1, crc, rxbuf[8],rxbuf[9]);
            //prn_rcv("rcv-CRC: ", &rxbuf[0], 10);
            DumpDNP_rcv(console, "rcv-CRC:", &rxbuf[0], 10);
        }
        return (TK_DNP_PROTOCOL_ERR);
    }
    
#if 0    
    /* ---------------------------------------- */
    /*  READ MODBUS - Header                    */
    /* ---------------------------------------- */ 
	if((result =TKreadn(hostSocketFd, (char *) &rxbuf[0], 2, 100)) < 0)
   	{
        /*  such as timeout */
		if(result == -1) /* timeout */
    	{
        	//if(opr->hexDebug)
           	//Debug(console,"host%02d> *Header Read Timeout [%d]... \n", hostid, tid, result);			     			        
    	}
       	else
       	{
           	//if(opr->hexDebug) 
	    	//Debug(console,"host%02d> *Header Read fail [%d][%s] \n", hostid, tid, result, strerror(errno));
   	 	}
       	return result;
    }
        
    /* TCP/IP Socket Error 발생시 ... */
	if(result == 0)	return 0;

    if((rxbuf[0] != 0x05) || (rxbuf[1] != 0x64))    return (-1);
    rxcnt = 2;
        
    /* read Header */
	if((result =TKreadn(hostSocketFd, (char *) &rxbuf[2], 8, 100)) < 0)	return (-1);
	
	/*  check HEADER-CRC     */
    if(B013_ckdnpcrc(&rxbuf[0], 10) != 0)
    {
        crc = B013_mkdnpcrc( &rxbuf[0], 8);
        if(opr->hostDebug == hostid)
        {
        	Debug(console,"[%02d:%02d:%02d] ===========>\n", rtc->hour, rtc->min, rtc->sec);
            Debug(console,"hrcv%2d> *header - crc error cal=%04x, rcv=%02x-%02x!\n", hostid+1, crc, rxbuf[8],rxbuf[9]);
            //prn_rcv("hrcv-CRC:", &rxbuf[0], 10);
            DumpDNP_rcv(console, "hrcv-CRC:", &rxbuf[0], 10);
        }
        return (-1);
    }
#endif     
    rxcnt  = 10;  

    length = rxbuf[2] - 5;      /* except control, src, des addr (5byte) */
    
    /* ------------------------------------ */
    /*  Datalink header만                    */
    /*  1) if CONFIRM frame received ...    */
    /*  2) if reset LINK frame received ... */
    /* ------------------------------------ */
    if(length <= 0)      
    {
        if(opr->hostDebug == hostid)  
            Debug(console,"hrcv%2d>  No DNP_APP_DATA, Link  Length = %d\n", hostid+1, length);
        return (rxcnt);
    }
    else if(length > 255)     
    {
        //if(opr->hostDebug == hostid)  
        Debug(console,"hrcv%2d> *DNP rcv - Invalid Length = %d \n", hostid+1, length);
        return (-1);
    }
    
    frameCnt = (length / FRAME_LENGTH);
    left     = (length % FRAME_LENGTH);
    if(left)    frameCnt = frameCnt + 1;    
	
	/* Multi-user data block : execpt CRC ... */
    for(i = 0; i < frameCnt; i++)
    {
        if(length <= 0 )    break;

        if(length >= FRAME_LENGTH)  
        {
            dataLen = FRAME_LENGTH; //16
            size = dataLen + 2;
        }
        else                        
        {
            dataLen = length ;
            size    = dataLen + 2;
        }
#if 0        
        if((result =TKreadn(hostSocketFd, (char *) &rxbuf[rxcnt], size, 100)) < 0)	
        {
            if(opr->hostDebug == hostid)  
            Debug(console,"hrcv%2d> *Tail not Received.... %d\n", hostid+1, result);
            return (-1);
        }
#endif 
        /* read USER data  */
        // time 100->1000
        if((result =tk_net_readn(hostSocketFd, (char *) &rxbuf[rxcnt], size, 2000)) != (size))	
        {

	        // step 1  timeout
	        if(result == TK_TIMEOUT ) /* timeout */
	        {
                if(opr->hostDebug == hostid)  
                Debug(console,"hrcv%02d> *link body Read Timeout [%d]... \n", hostid+1, result);		
                return TK_TIMEOUT ;  	     			        
	        }
	        			
	        // step2  EOF 
	        if(result == TK_SOCKET_EOF ) 
	        {
	            // if((opr->dnpDebug == devid) || (opr->scanDebug == scanid))
                Debug(console,"hrcv%02d> *Peer close socket  [%d]... \n", hostid+1, result);			     			        
	        	return TK_SOCKET_EOF ; 
	        }
	        
            else // system error 
            {
                //if((opr->dnpDebug == devid) || (opr->scanDebug == scanid))
		        Debug(console,"hrcv%02d> Socket Err [%d][%s] \n", hostid+1, result, strerror(errno));
		        return TK_SYS_ERR ; 
            }

        }
                
        if(B013_ckdnpcrc(&rxbuf[rxcnt], result) != 0)
        {
            crc = B013_mkdnpcrc( &rxbuf[rxcnt], dataLen);
            if(opr->hostDebug == hostid)
            Debug(console,"hrcv%2d> *user data [%d] - crc error cal=%04x, rcv=%02x-%02x!\n",  hostid+1, i+1, crc, rxbuf[rxcnt + dataLen], rxbuf[rxcnt + dataLen + 1]);
            rxcnt = rxcnt + dataLen;
            return (-1);
        }

        rxcnt  = rxcnt + dataLen;   /* include crc-data */
        length = length - dataLen;
        
    }

    return (rxcnt);
	
	
}

/* 데이터 link 만 수신하나 보다.. */
/***************************************************
*   FUNCTION : readDataLink()
*   receive DATA-LINK with CRC
***************************************************/
int readDataLinkHOST(int hostid, int chid, int socketID, DNP_DATA_LINK *dnpData)
{
    int     i,size;
    int     rxcnt,rcvCount;
    //int     retVal;
    int     length;
    int     frameCnt,left;
    int     dataLen;
    int     desAddr, srcAddr;
    byte    *rxbuf;
    
#ifndef ACE_LBS_4U    
    int     vmecnt;
    byte    *vmedata;
#endif
    
    word    crc;
    word    timeout;
    DL_FRAME    *rcvData;
    //DNP_DEVICE  *dnp;
    HOST_DCB    *host;

    /* ------------------------------------ */
    /*  HOST 별 통신 Frame 설정             */
    /* ------------------------------------ */
    host = (HOST_DCB *) hostDCB[hostid];

    rxcnt   = 0;
    rcvCount = 0;
    timeout = 0;

    /* ---------------------------------------------------- */
    /*  HOST - TCPIP 의 경우...                             */
    /* ---------------------------------------------------- */
    if(host->hostComType == COM_TCPIP)
    {
        rcvData = (DL_FRAME *) &dnpData->rcvFrame;
        rxbuf = (byte *) &rcvData->start1;

        /* ------------------------------------------------ */
        /*  VMEBUS 에서 수신된 Packet Data 수신...     ??     */
        /* hostTCPIP_read에 size가 없네 */
        /* ------------------------------------------------ */
        rcvCount = tk_hostTCPIP_read(hostid, socketID, rxbuf);
//        rcvCount = hostTCPIP_read(hostid, socketID, rxbuf);
        
        /* Timeout 이나 Connection Fail... */
        /* 이 함수롤 call한 곳에서 err 처리 해야되는군...가보자...*/
        if(rcvCount <= 0)   return (rcvCount);
            
        /* ------------------------ */
        /*  check DNP-ADDR ...      */
        /* ------------------------ */
        desAddr = rcvData->desAddr[0] + (rcvData->desAddr[1]*256);
        srcAddr = rcvData->srcAddr[0] + (rcvData->srcAddr[1]*256);
    
        if(desAddr == 0xffff)
        {
           // return 은 rcvCount...
            if(opr->hostDebug == hostid) Debug(console,"hrcv%2d> Broadcast Addr .. !\n", hostid+1);
            dnpData->broadCast = SET;
        }
        else
        {
            dnpData->broadCast = RESET;

            if(desAddr != host->rtuAddr)
            {
                if(opr->hostDebug == hostid)    
                {
                	//prn_rcv("*RXM:", rxbuf, rcvCount);
                	DumpDNP_rcv(console, "RXM*:", rxbuf, rcvCount);
                }
                
                if(opr->hexDebug == hostid) 
                Debug(console,"hrcv%2d> *1 HOST Addr mismatch : rcv %4x, cu-host %4x.. !\n", hostid+1, desAddr, host->rtuAddr);
                return (-2);
            }
            else if(srcAddr != host->hostid)
            {
                if(opr->hostDebug == hostid)    
                {
                	//prn_rcv("*RXM:", rxbuf, rcvCount);
                	DumpDNP_rcv(console, "*RXM:", rxbuf, rcvCount);
                }
                if(opr->hexDebug == hostid)
                Debug(console,"hrcv%2d> *1 RTU  Addr mismatch : rcv %4x, cu-rtu %4x.. !\n", hostid+1, srcAddr, host->hostid);
                return (-2);
            }
        }
    
        return (rcvCount);
        
    }

    /* ---------------------------------------------------- */
    /*  RTU-TYPE : ACE-LBS-6U... ASYNC  PORT Comm Parameter */
    /*  RTU-TYPE : ACE-VMECU ... VMESIO PORT Comm Parameter */
    /* ---------------------------------------------------- */

#ifdef  ACE_LBS_RTU

    /* ---------------------------------------------------- */
    /*  HOST - ACE-LBS-6U  ASYNC PORT 의 경우...            */
    /* ---------------------------------------------------- */
    rcvData = (DL_FRAME *) &dnpData->rcvFrame;

    rxcnt   = 0;
    rcvCount = 0;
    timeout = 0;
    
    rxbuf = (byte *) &rcvData->start1;
    
    /* ---------------------------- */
    /*  receive STX(0x0564)         */
    /* ---------------------------- */
RCV_START:
    do {
        if(hostChanRead(hostid, chid, &rxbuf[0], 1) == 0) return (0);
        if(++timeout > 50)  return (0);
    } while(rxbuf[0] != 0x05);


    if(hostChanRead(hostid, chid, &rxbuf[1], 1) == 0) return (0);
    if(rxbuf[1] != 0x64)    goto    RCV_START;
    rxcnt = 2;

    /* read Header */
    if(hostChanRead(hostid, chid, &rxbuf[rxcnt], 8) != 8)
    {
        return(0);
    }
    
    if(B013_ckdnpcrc(&rxbuf[0], 10) != 0)
    {
        crc = B013_mkdnpcrc( &rxbuf[0], 8);
        if(opr->hostDebug == hostid)
        {
            Debug(console,"hrcv%2d> *header - crc error cal=%04x, rcv=%02x-%02x!\n", hostid+1, crc, rxbuf[8],rxbuf[9]);
            DumpDNP_rcv(console, "hrcv-CRC:", &rxbuf[0], 10);
        }
        return (0);
    }

    /* ------------------------ */
    /*  check DNP-ADDR ...      */
    /* ------------------------ */
    desAddr = rcvData->desAddr[0] + (rcvData->desAddr[1]*256);
    srcAddr = rcvData->srcAddr[0] + (rcvData->srcAddr[1]*256);
        
    if(desAddr == 0xffff)
    {
        if(opr->hostDebug == hostid)
        Debug(console,"hrcv%2d> Broadcast Addr .. !\n", hostid + 1);
        dnpData->broadCast = RESET;
        return (0);
    }
    else
    {
        dnpData->broadCast = RESET;

        if(desAddr != host->rtuAddr)
        {
            if(opr->hexDebug == hostid) 
            Debug(console,"hrcv%2d> *1 HOST Addr mismatch : rcv %4x, cu-host %4x.. !\n", hostid+1, desAddr, host->rtuAddr);
            return (0);
        }
        else if(srcAddr != host->hostid)
        {
            if(opr->hexDebug == hostid)
            Debug(console,"hrcv%2d> *1 RTU  Addr mismatch : rcv %4x, cu-rtu %4x.. !\n", hostid+1, srcAddr, host->hostid);
            return (0);
        }
    }

//NEXT_PROCESS:    
    rxcnt  = 10;                /* user data */
    length = rxbuf[2] - 5;      /* except control, src, des addr (5byte) */

    /* ------------------------------------ */
    /*  1) if CONFIRM frame received ...    */
    /*  2) if reset LINK frame received ... */
    /* ------------------------------------ */
    if(length == 0)     
    {
        if(opr->hexDebug == hostid)
        Debug(console,"rcv%2d> no Data Length = %d\n", hostid+1, length);
        return (rxcnt);
    }
    else if(length > 255)     
    {
        if(opr->hexDebug == hostid)
        Debug(console,"rcv%2d> *DNP rcv - Invalid Length = %d\n", hostid+1, length);
        return (0);
    }
    
    frameCnt = (length / FRAME_LENGTH);
    left     = (length % FRAME_LENGTH);
    if(left)    frameCnt = frameCnt + 1;

    /* Multi-user data block : execpt CRC ... */
    for(i = 0; i < frameCnt; i++)
    {
        if(length <= 0 )    break;

        if(length >= FRAME_LENGTH)  dataLen = FRAME_LENGTH;
        else                        dataLen = length;

        /* read USER data  */
        if((size = hostChanRead(hostid, chid, &rxbuf[rxcnt], dataLen+2)) != (dataLen+2))
        {
            if(opr->hexDebug == hostid)
            Debug(console,"hrcv%2d> 1. rcv Time out  ... req=%d, rcv=%d \n", hostid+1, dataLen+2,size);
            return(0);
        }

        if(B013_ckdnpcrc(&rxbuf[rxcnt], size) != 0)
        {
            crc = B013_mkdnpcrc( &rxbuf[rxcnt], dataLen);
            if(opr->hexDebug == hostid)
            Debug(console,"hrcv%2d> *user data [%d] - crc error cal=%04x, rcv=%02x-%02x!\n",
                hostid+1, i+1, crc, rxbuf[rxcnt + dataLen], rxbuf[rxcnt + dataLen + 1]);
            rxcnt = rxcnt + dataLen;
            return (0);
        }

        rxcnt  = rxcnt + dataLen;   /* include crc-data */
        length = length - dataLen;
    }

    return (rxcnt);
    
#else

    /* ---------------------------------------------------- */
    /*  HOST - ACE-VMECU-NET-V7 VMESIO 의 경우...           */
    /* ---------------------------------------------------- */
    rcvData = (DL_FRAME *) &dnpData->rcvFrame;

    rxcnt   = 0;
    rcvCount = 0;
    timeout = 0;
    
    rxbuf = (byte *) &rcvData->start1;
    vmedata = (byte *) &vmeRxbuf[hostid][0];

    /* ------------------------------------------------ */
    /*  VMEBUS 에서 수신된 Packet Data 수신...          */
    /* ------------------------------------------------ */
    rcvCount = hostVMERead(hostid, chid, vmedata);
    if(rcvCount <= 0)   return (0);
    
    /* check HEADER */
    if(vmedata[0]!= 0x05) return (0);
    if(vmedata[1]!= 0x64) return (0);
    rxcnt = 2;

    /*  check HEADER-CRC     */
    if(B013_ckdnpcrc(&vmedata[0], 10) != 0)
    {
        crc = B013_mkdnpcrc( &vmedata[0], 8);
        if(opr->hostDebug == hostid)
        {
        	Debug(console,"[%02d:%02d:%02d] ===========>\n", rtc->hour, rtc->min, rtc->sec);
            Debug(console,"hrcv%2d> *vme header - crc error cal=%04x, rcv=%02x-%02x!\n", hostid+1, crc, vmedata[8],vmedata[9]);
            //prn_rcv("hrcv-CRC:", &vmedata[0], 10);
            DumpDNP_rcv(console, "hrcv-CRC:", &vmedata[0], 10);
        }
        return (0);
    }
    
    rxcnt  = 10;  
    memcpy(rxbuf, vmedata, rxcnt);        
    
    /* ------------------------ */
    /*  check DNP-ADDR ...      */
    /* ------------------------ */
    desAddr = rcvData->desAddr[0] + (rcvData->desAddr[1]*256);
    srcAddr = rcvData->srcAddr[0] + (rcvData->srcAddr[1]*256);
    
    if(desAddr == 0xffff)
    {
        if(opr->hostDebug == hostid) Debug(console,"hrcv%2d> Broadcast Addr .. !\n", hostid+1);
        dnpData->broadCast = SET;
    }
    else
    {
        dnpData->broadCast = RESET;

        if(desAddr != host->rtuAddr)
        {
            if(opr->hexDebug == hostid) 
            Debug(console,"hrcv%2d> *1 HOST Addr mismatch : rcv %4x, cu-host %4x.. !\n", hostid+1, desAddr, host->rtuAddr);
            return (0);
        }
        else if(srcAddr != host->hostid)
        {
            if(opr->hexDebug == hostid)
            Debug(console,"hrcv%2d> *1 RTU  Addr mismatch : rcv %4x, cu-rtu %4x.. !\n", hostid+1, srcAddr, host->hostid);
            return (0);
        }
    }
    
//NEXT_PROCESS_HOST:    
    rxcnt  = 10;                /* user data */
    vmecnt = rxcnt;
    length = vmedata[2] - 5;      /* except control, src, des addr (5byte) */

    /* ------------------------------------ */
    /*  1) if CONFIRM frame received ...    */
    /*  2) if reset LINK frame received ... */
    /* ------------------------------------ */
    if(length == 0)     
    {
        //memcpy(rxbuf, vmedata, rxcnt);
        if(opr->hostDebug == hostid)  Debug(console,"hrcv%2d> no Data Length = %d\n", hostid+1, length);
        return (rxcnt);
    }
    
    frameCnt = (length / FRAME_LENGTH);
    left     = (length % FRAME_LENGTH);
    if(left)    frameCnt = frameCnt + 1;

    /* Multi-user data block : execpt CRC ... */
    for(i = 0; i < frameCnt; i++)
    {
        if(length <= 0 )    break;

        if(length >= FRAME_LENGTH)  
        {
            dataLen = FRAME_LENGTH;
            size = dataLen + 2;
        }
        else                        
        {
            dataLen = length ;
            size    = dataLen + 2;
        }

        if(B013_ckdnpcrc(&vmedata[vmecnt], size) != 0)
        {
            crc = B013_mkdnpcrc( &vmedata[vmecnt], dataLen);
            if(opr->hostDebug == hostid)
            Debug(console,"hrcv%2d> *user data [%d] - crc error cal=%04x, rcv=%02x-%02x!\n",
                hostid+1, i+1, crc, vmedata[vmecnt + dataLen], vmedata[vmecnt + dataLen + 1]);
            rxcnt = rxcnt + dataLen;
            return (0);
        }

        memcpy(&rxbuf[rxcnt], &vmedata[vmecnt], dataLen);
        
        rxcnt  = rxcnt + dataLen;   /* include crc-data */
        length = length - dataLen;
        
        vmecnt += size;
    }

    return (rxcnt);

#endif
        

}

/*
*   2016년 3월 5일 - 철도공항공사 영종역, 비츠로시스 HOST
*/
int check_UNSOLICT_HOST(int hostid, int chid , int socketID)
{
    int     i;
    int     count;
    byte    sndSeqNo;
    byte    front1, rear1, cosCount;
    byte    front2, rear2, soeCount;
    byte    *sndbuf;
    
    DNP_APP_FRAME   *sndAppDnp;
    
    HOST_DCB        *host;

    DNP_SOE_QUEUE   *soeQ;
    DNP_SOEQ_ENTRY  *entrySOE;
    DNP_COS_QUEUE   *cosQ;
    DNP_COSQ_ENTRY  *entryCOS;
    
    host = (HOST_DCB *) hostDCB[hostid];    
    
    /* -------------------------------------------------------- */
    /*  UNSOLICIT 메세지 허용여부                               */
    /* -------------------------------------------------------- */
    if(host->unsolEvent == 0)   return (0);

    /* ---------------------------------------- */
    /*  VAR=2, SOE 정보 추가                    */
    /* ---------------------------------------- */    
    soeQ = (DNP_SOE_QUEUE *) &host->dnpSOEQ;
    front2 = soeQ->front & 0xff;
    rear2  = soeQ->rear & 0xff;
    
    soeCount = (front2 - rear2 + 256) % 256;
    if(soeCount > 24)   soeCount = 24;
    
    /* ---------------------------------------- */
    /*  VAR=1, COS 정보 추가                    */
    /* ---------------------------------------- */  
    cosQ = (DNP_COS_QUEUE *) &host->dnpCOSQ;
    front1 = cosQ->front & 0xff;
    rear1  = cosQ->rear & 0xff;
    
    cosCount = (front1 - rear1 + 256) % 256;
    if(cosCount > 64)   cosCount = 64;
        
    if((cosCount == 0) && (soeCount == 0))  return (0);

    sndAppDnp = (DNP_APP_FRAME *) &host->sndUnsolFrame[chid];
    
    /* -------------------------------------------------------- */
    /* Make APPLICATION Message FORMAT :                        */
    /* -------------------------------------------------------- */
    checkCUstatus(hostid, chid);

    /* 데이터 포인트 */
    sndbuf = (byte *) &sndAppDnp->userData[0];
            
    count = 0;
    sndSeqNo = host->sndACseq + 0x1f;
    
    sndAppDnp->userData[0] = 0xc0 + sndSeqNo + CONFIRM_BIT;     /* AC */
    sndAppDnp->userData[1] = APP_RCV_UNSOLICIT;                 /* Function : 130 */
    sndAppDnp->userData[2] = host->rtuIIN[0];                   // IIN
    sndAppDnp->userData[3] = host->rtuIIN[1];                   // IIN
    count = 4;
    
    /* ---------------------------------------- */
    /*  VAR=1, COS 정보 추가                    */
    /* ---------------------------------------- */   
    if(cosCount > 0)
    {
        if(opr->soeDebug)
        Debug(console,"hsnd%2d> <--- Un-solicit COS-Q Report=%2d [front=%d, rear = %d] ... [%02d:%02d:%02d] \n", hostid+1, cosCount, 
            front1, rear1, rtc->hour, rtc->min, rtc->sec);
    
        /* ---------------------------------------------------------------- */
        /* QCode 7 : 8bit Single field Quantity, 포인트가 256 이하인 경우   */
        /* ---------------------------------------------------------------- */    
        if(host->diIndexWord == RESET)
        {            
            sndbuf[count++] = OBJ_BINARY_CHANGE;        // OBJ code             
            sndbuf[count++] = 1;                        // Variation #2
            sndbuf[count++] = 0x17;                     // QCode 7 : Single field Quantity, 포인트가 256 이하인 경우
            sndbuf[count++] = cosCount;                 // SOE Count Number
    
            for(i=0; i< cosCount; i++)
            {
                entryCOS = ( DNP_COSQ_ENTRY *) &cosQ->cosQueue[rear1];
                sndbuf[count++] = entryCOS->cosBuf[0];
                sndbuf[count++] = entryCOS->cosBuf[1];
                rear1 = (rear1 + 1) & 0xff;
            }
        }
        else
        {
            /* ---------------------------------------------------------------- */
            /* QCode 8 : 16bit Single field Quantity, 포인트가 256 이하인 경우  */
            /* ---------------------------------------------------------------- */    
            sndbuf[count++] = OBJ_BINARY_CHANGE;        // OBJ code             
            sndbuf[count++] = 1;                        // Variation #2
            sndbuf[count++] = 0x28;                     // QCode 8 : 16Bit Single field Quantity, 포인트가 256 이하인 경우
            sndbuf[count++] = cosCount;                 // SOE Count Number
            sndbuf[count++] = 0;                        // SOE Count Number
    
            for(i=0; i< cosCount; i++)
            {
                entryCOS = ( DNP_COSQ_ENTRY *) &cosQ->cosQueue[rear1];
                sndbuf[count++] = entryCOS->cosBuf[0];          // point 번호 : 하위 
                sndbuf[count++] = entryCOS->cosBuf[1];          // point 번호 : 상위 
                sndbuf[count++] = entryCOS->cosBuf[2];          // point state 
                                    
                rear1 = (rear1 + 1) & 0xff;
            }                                
        }
        
        /* AC-Confirm 시 Rear 정보 Update */
        host->cosReport = cosCount;
        host->cosRear   = rear1;
    }
    
    /* ---------------------------------------- */
    /*  VAR=2, SOE 정보 추가                    */
    /* ---------------------------------------- */    
    if(soeCount > 0)
    {
        if(opr->soeDebug)
        Debug(console,"hsnd%2d> <<<< Un-solicit SOE-Q Report=%2d [front=%d, rear = %d]   ... [%02d:%02d:%02d] \n", hostid+1, soeCount, 
                front2, rear2, rtc->hour, rtc->min, rtc->sec);      
    
        /* ---------------------------------------------------------------- */
        /* QCode 7 : 8bit Single field Quantity, 포인트가 256 이하인 경우   */
        /* ---------------------------------------------------------------- */    
        if(host->diIndexWord == RESET)
        {
            sndbuf[count++] = OBJ_BINARY_CHANGE;        // OBJ code             
            sndbuf[count++] = 2;                        // Variation #2
            sndbuf[count++] = 0x17;                     // QCode 7 : Single field Quantity, 포인트가 256 이하인 경우
            sndbuf[count++] = soeCount;                 // SOE Count Number
    
            for(i=0; i< soeCount; i++)
            {
                entrySOE = ( DNP_SOEQ_ENTRY *) &soeQ->soeQueue[rear2];
                sndbuf[count++] = entrySOE->soeBuf[0];
                sndbuf[count++] = entrySOE->soeBuf[1];
                sndbuf[count++] = entrySOE->soeBuf[2];
                sndbuf[count++] = entrySOE->soeBuf[3];
                sndbuf[count++] = entrySOE->soeBuf[4];
                sndbuf[count++] = entrySOE->soeBuf[5];
                sndbuf[count++] = entrySOE->soeBuf[6];
                sndbuf[count++] = entrySOE->soeBuf[7];
        
                rear2 = (rear2 + 1) & 0xff;
            }
        }
        else
        {
            /* ---------------------------------------------------------------- */
            /* QCode 8 : 16bit Single field Quantity, 포인트가 256 이하인 경우  */
            /* ---------------------------------------------------------------- */    
            sndbuf[count++] = OBJ_BINARY_CHANGE;        // OBJ code             
            sndbuf[count++] = 2;                        // Variation #2
            sndbuf[count++] = 0x28;                     // QCode 8 : Single field Quantity, 포인트가 256 이하인 경우
            sndbuf[count++] = soeCount;                 // SOE Count Number
            sndbuf[count++] = 0;                        // SOE Count Number
    
            for(i=0; i< soeCount; i++)
            {
                entrySOE = ( DNP_SOEQ_ENTRY *) &soeQ->soeQueue[rear2];
                sndbuf[count++] = entrySOE->soeBuf[0];          // point 번호 : 하위 
                sndbuf[count++] = entrySOE->soeBuf[1];          // point 번호 : 상위 
                sndbuf[count++] = entrySOE->soeBuf[2];          // point state 
            
                sndbuf[count++] = entrySOE->soeBuf[3];          // SOE Time 
                sndbuf[count++] = entrySOE->soeBuf[4];
                sndbuf[count++] = entrySOE->soeBuf[5];
                sndbuf[count++] = entrySOE->soeBuf[6];
                sndbuf[count++] = entrySOE->soeBuf[7];
                sndbuf[count++] = entrySOE->soeBuf[8];
        
                rear2 = (rear2 + 1) & 0xff;
            }
        }
                    
        /* AC-Confirm 시 Rear 정보 Update */
        host->soeReport = soeCount;
        host->soeRear = rear2;
    }
    
    /* ---------------------------------------- */
    /*  COS, SOE 정보가 있는 경우.... Write     */
    /* ---------------------------------------- */ 
    if(count > 4)
    {
                    
        if(opr->hexDebug == hostid)
        DumpBuff(console,"SND: ", &sndAppDnp->userData[0], count);
        
        sndAppDnp->sndCount = count;

        sendAppFrame_HOST( hostid, chid, socketID, sndAppDnp);
    }

    return (0);
    
}


/* ------------------------------------------------------- */
/*  HOST MAIN 프로그램 Start Routine ....             */
/* ------------------------------------------------------- */
int    hostDnpThread(int hostid)
{
	byte	runtick;
	int		oldsec = 0;
	int     retVal;
	int     rxcnt;
	int     socketID=0;

    char    buffer[128];
    	
	DNP_DATA_LINK   *dnpData;

    DNP_APP_FRAME   *rcvAppDnp;
    DNP_APP_FRAME   *sndAppDnp;
	HOST_DCB        *host;
	DNP_SOE_QUEUE   *dnpSOEQ;

#ifdef  ACE_LBS_RTU	
	TTY_DESC        *ttyHost;
#endif
	
	/* -------------------------------- */
	/*  HOST별 Index ...                */
	/* -------------------------------- */
    host = (HOST_DCB *) hostDCB[hostid];                 /* 주장치 #1 속성정의 */
    
    host->hostActive = HOST_NOT_READY;          /* HOST 통신모드 : 미지정 */ 
    
    dnpSOEQ = (DNP_SOE_QUEUE *) &host->dnpSOEQ;
    dnpSOEQ->front = 0;
    dnpSOEQ->rear  = 0;
    
    dnpSOEQ->front1= 0;
    dnpSOEQ->rear1 = 0;
    
    host->sndTHseqno = 0;
    host->cosReport  = 0;
    host->cosRear    = 0;
    
    host->soeReport  = 0;
    host->soeRear    = 0;
            
    /* HOST 통신 초기상태 ...   */
    host->online[MASTER_PORT]      = 0;    // default OFFLINE
    host->online[SLAVE_PORT]       = 0;    // default OFFLINE
    host->comFailTick[MASTER_PORT] = 0;
    host->comFailTick[SLAVE_PORT]  = 0;

    runtick = 0;

    /* ---------------------------------------------------- */
    /*  RTU-TYPE : ACE-LBS-6U... ASYNC  PORT Comm Parameter */
    /*  RTU-TYPE : ACE-VMECU ... VMESIO PORT Comm Parameter */
    /* ---------------------------------------------------- */
#ifdef  ACE_LBS_RTU
    if(host->hostDualMode == HOST_DUAL)
    {
        /* ---------------------------- */
        /* MASTER-PORT 초기화           */
        /* ---------------------------- */
        ttyHost = (TTY_DESC *) &tty_HostChan[host->masterChan];
        
        /* COM1 통신속도 반영 */
        if(host->hostComSpeed == SPEED_9600)        ttyHost->baudRate = 9600;
        else if(host->hostComSpeed == SPEED_19200)  ttyHost->baudRate = 19200;
        else if(host->hostComSpeed == SPEED_38400)  ttyHost->baudRate = 38400;
        else if(host->hostComSpeed == SPEED_57600)  ttyHost->baudRate = 57600;
        else                                        ttyHost->baudRate = 38400;
        
        retVal = PortInitial((TTY_DESC *) ttyHost);
        if(retVal == -1)
        {
            termExec = 0;
            printf("\n===========================================\n");
            printf("host%02d> *HOST-Dual(%d)Serial Com Port Open...ERROR...return = %d \n", hostid+1, host->masterChan, retVal);
            printf("===========================================\n");
            pause(1000);
            pause(1000);
        }
        
        host->ttyPort[MASTER_PORT] = (TTY_DESC *) ttyHost;
        
        /* ---------------------------- */
        /* SLAVE-PORT 초기화           */
        /* ---------------------------- */
        ttyHost = (TTY_DESC *) &tty_HostChan[host->slaveChan];
        
        /* COM1 통신속도 반영 */
        if(host->hostComSpeed == SPEED_9600)        ttyHost->baudRate = 9600;
        else if(host->hostComSpeed == SPEED_19200)  ttyHost->baudRate = 19200;
        else if(host->hostComSpeed == SPEED_38400)  ttyHost->baudRate = 38400;
        else if(host->hostComSpeed == SPEED_57600)  ttyHost->baudRate = 57600;
        else                                        ttyHost->baudRate = 38400;
        
        retVal = PortInitial((TTY_DESC *) ttyHost);
        if(retVal == -1)
        {
            termExec = 0;
            printf("\n===========================================\n");
            printf("host%02d> *HOST-Dual(%d) Serial Com Port Open...ERROR...return = %d \n", hostid+1, host->slaveChan, retVal);
            printf("===========================================\n");
            pause(1000);
            pause(1000);
        }
        
        host->ttyPort[SLAVE_PORT] = (TTY_DESC *) ttyHost;
        
    }
    else if(host->hostDualMode == HOST_SINGLE)
    {
        /* ---------------------------- */
        /* MASTER-PORT 초기화           */
        /* ---------------------------- */
        ttyHost = (TTY_DESC *) &tty_HostChan[host->masterChan];
        
        /* COM1 통신속도 반영 */
        if(host->hostComSpeed == SPEED_9600)        ttyHost->baudRate = 9600;
        else if(host->hostComSpeed == SPEED_19200)  ttyHost->baudRate = 19200;
        else if(host->hostComSpeed == SPEED_38400)  ttyHost->baudRate = 38400;
        else if(host->hostComSpeed == SPEED_57600)  ttyHost->baudRate = 57600;
        else                                        ttyHost->baudRate = 38400;
        
        retVal = PortInitial((TTY_DESC *) ttyHost);
        if(retVal == -1)
        {
            termExec = 0;
            printf("\n===========================================\n");
            printf("host%02d> *HOST-Single(%d) Serial Com Port Open...ERROR...return = %d \n", hostid+1, host->masterChan, retVal);
            printf("===========================================\n");
            pause(1000);
            pause(1000);
        }
        
        host->ttyPort[MASTER_PORT] = (TTY_DESC *) ttyHost;
    }
    
#else

    /* ---------------------------------------------------- */
    /*  Default VME Channel Comm Parameter                  */
    /* ---------------------------------------------------- */
    host->vmeChan[MASTER_PORT]->protocolType = SCAN_DNP;
    host->vmeChan[MASTER_PORT]->cfgType      = host->hostComType;
    host->vmeChan[MASTER_PORT]->cfgSpeed     = host->hostComSpeed;
    
    host->vmeChan[SLAVE_PORT]->protocolType  = SCAN_DNP;
    host->vmeChan[SLAVE_PORT]->cfgType       = host->hostComType;
    host->vmeChan[SLAVE_PORT]->cfgSpeed      = host->hostComSpeed;

    //hostVMEInitial(hostid);    
    vmeHostInitial(hostid, host->masterChan);  
    vmeHostInitial(hostid, host->slaveChan);  
    
    vmeHostChanReset(hostid, MASTER_PORT);
    vmeHostChanReset(hostid, SLAVE_PORT);
    
    /* ------------------------ */
    /* SIO Board 를 사용함      */
    /* ------------------------ */
    opr->useSIOBoard = SET;
    
#endif
    
    /* ---------------------------------------------------- */
    /* ACE-RTU : 상위 HOST 통신포트 초기화                  */
    /* ---------------------------------------------------- */        
    if(host->hostDualMode == HOST_DUAL)             // 이중화 구조인 경우 
    {
        printf(" %s HOST[%d] DNP-THREAD *DUAL [%d/%d]*  Activated ... !\n", TARGET_NAME, hostid+1, host->masterChan, host->slaveChan);
        
        sprintf(buffer, "%s HOST[%d] DNP-THREAD *DUAL [%d/%d]*  Activated ... !", TARGET_NAME, hostid+1, host->masterChan, host->slaveChan);
        LogFile_MPU ( opr, NOT_DEFINED, buffer, strlen(buffer));
    }
    else if(host->hostDualMode == HOST_SINGLE)      // 개별 구조인 경우 
    {
        printf(" %s HOST[%d] DNP-THREAD *SINGLE [%d/%d]*  Activated ... !\n", TARGET_NAME, hostid+1, host->masterChan, host->slaveChan);
        
        sprintf(buffer, "%s HOST[%d] DNP-THREAD *SINGLE [%d/%d]*  Activated ... !", TARGET_NAME, hostid+1, host->masterChan, host->slaveChan);
        LogFile_MPU ( opr, NOT_DEFINED, buffer, strlen(buffer));
    }        
    else         // 사용하지 않는 경우 
    {
        printf(" %s HOST[%d] DNP-THREAD *NOT Define [%d/%d]*  Activated ... !\n", TARGET_NAME, hostid+1, host->masterChan, host->slaveChan);
        
        sprintf(buffer, "%s HOST[%d] DNP-THREAD *NOT Define [%d/%d]*  Activated ... !", TARGET_NAME, hostid+1, host->masterChan, host->slaveChan);
        LogFile_MPU ( opr, NOT_DEFINED, buffer, strlen(buffer));
        
        while(termExec)
	    {
	        wdtHostFlag[hostid] = 0;
		    pause(1000);        
		    
		    opr->mmiHostSts[hostid] = 3;    // 사용않함.    
		    
		    if(opr->wdtDebug)
		    Debug(console,">> *HOST[%d] DNP-THREAD *NOT Define ... !\n", hostid);
        }		    
    }

    
    host->activePort = MASTER_PORT;
    opr->hostRestart[hostid] = RESET;
    
	while(termExec)
	{
	    taskPtr->wdtCount = 0;
	    wdtHostFlag[hostid] = 0;
		pause(20);

		/* ---------------------------------------------------- */
		/*	CPU 이중화 : LOCAL-SLAVE ... 대기모드 				*/
		/* ---------------------------------------------------- */
		if(opr->runMode == LOCAL_SLAVE)	
		{
		    /* 2015.08.25 Update...SLAVE모드시 HOST Offline 처리 */
		    host->online[MASTER_PORT] = 0;
		    host->online[SLAVE_PORT]  = 0;
		    host->hostActive = HOST_NOT_READY;          /* HOST 통신모드 : 미지정 */ 
		    pause(1000);
		    
		    /* ------------------------------------ */
            /* HOST-DB 변경에 따른 재기동 ...       */
            /* ------------------------------------ */
            if(opr->hostRestart[hostid] == SET)
            {
                Debug(console, "host%02d> *** HOST TASK Restart... DB Chg ...\n", hostid+1);
                pause(1000);
                termExec = RESET;
            }
        
		    continue;
		}
			
        /* ---------------------------------------------------- */
        /*  MASTER 통신포트 : 상위 데이터 수신 처리 ...         */
        /* ---------------------------------------------------- */
        
        /* HOST 통신용 MASTER Buffer 포인트 */
        dnpData   = (DNP_DATA_LINK *) &host->dataLinkFrame[MASTER_PORT];
        sndAppDnp = (DNP_APP_FRAME *) &host->sndAppFrame[MASTER_PORT];
        rcvAppDnp = (DNP_APP_FRAME *) &host->rcvAppFrame[MASTER_PORT];
    
        // dnpData에 저장하겠지...
        if((rxcnt = readDataLinkHOST( hostid, MASTER_PORT, socketID, dnpData)) > 0)
        {
            host->comFailTick[MASTER_PORT]= 0;
            host->comFailTick[SLAVE_PORT] = 0;
            opr->hostComSts[hostid] = 1;
            
            /* 통신 이중화의 경우 ...SLAVE 통신정상 */
            if(host->hostDualMode == HOST_DUAL) host->comFailTick[SLAVE_PORT]= 0; 
                
            if(opr->hostDebug == hostid)
            {
                Debug(console,"\n-----------------------------------------[%02d/%02d-%02d:%02d:%02d]\n", 
        	        rtc->month, rtc->day, rtc->hour, rtc->min, rtc->sec);
                //prn_rcv("RXM:", &dnpData->rcvFrame, rxcnt);
                DumpDNP_rcv(console, "RXM:", &dnpData->rcvFrame, rxcnt);
            }
            
            dnpData->rcvCount        = rxcnt;
            if(host->online[MASTER_PORT] != 1)  
            {
                logEvent1(opr, sque, EVENT_HOST_ONLINE, hostid, MASTER_PORT, 0, hostid);  
                logHistoryQue1(opr, hque, EVENT_HOST_ONLINE, hostid, MASTER_PORT, 0, hostid);  
                
                /* -------------------------------------------- */
                /* LOG File 저장                                */
                /* -------------------------------------------- */
                sprintf(buffer, "DNP> ID=%2d, => MASTER-PORT Online ", hostid+1);
                LogFile_MPU ( opr, EVENT_HOST_ONLINE, buffer, strlen(buffer));
                
            }
            host->online[MASTER_PORT] = 1;
            
            /* ---------------------------- */
            /* check DATA-LINK Layer  ...   */
            /* ---------------------------- */
            retVal = chkRcvFrameHOST(hostid, MASTER_PORT, socketID, dnpData);
            if((retVal == FUNC_USER_DATA) || (retVal == FUNC_UNCONFIRM))
            {
                retVal = checkDataLinkHOST(hostid, MASTER_PORT, socketID, dnpData);
                if(retVal)
                {
                    checkAPPFrame_HOST( hostid, MASTER_PORT, socketID);
                }                    
            }
        } // MASTER PORT      
        
        /* ---------------------------------------------------- */
        /*  SLAVE  통신포트 : 상위 데이터 수신 처리 ...         */
        /* ---------------------------------------------------- */        
        if(host->hostDualMode == HOST_DUAL)
        {
            /* HOST 통신용 SLAVE Buffer 포인트 */
            dnpData   = (DNP_DATA_LINK *) &host->dataLinkFrame[SLAVE_PORT];
            sndAppDnp = (DNP_APP_FRAME *) &host->sndAppFrame[SLAVE_PORT];
            rcvAppDnp = (DNP_APP_FRAME *) &host->rcvAppFrame[SLAVE_PORT];
        
            if((rxcnt = readDataLinkHOST( hostid, SLAVE_PORT, socketID, dnpData)) > 0)
            {
                opr->hostComSts[hostid] = 1;
                
                host->comFailTick[MASTER_PORT]= 0;
                host->comFailTick[SLAVE_PORT] = 0;
                
                /* 통신 이중화의 경우 ...MASTER 통신정상 */
                if(host->hostDualMode == HOST_DUAL) host->comFailTick[MASTER_PORT] = 0; 
                    
                if(opr->hostDebug == hostid)
                {
                    Debug(console,"\n-----------------------------------------[%02d/%02d-%02d:%02d:%02d]\n", 
        	            rtc->month, rtc->day, rtc->hour, rtc->min, rtc->sec);
                    //prn_rcv("RXS:", &dnpData->rcvFrame, rxcnt);
                    DumpDNP_rcv(console, "RXS:", &dnpData->rcvFrame, rxcnt);
                }
                
                dnpData->rcvCount   = rxcnt;
                
                if(host->online[SLAVE_PORT] != 1)  
                {
                    logEvent1(opr, sque, EVENT_HOST_ONLINE, hostid, SLAVE_PORT, 0, hostid);  
                    logHistoryQue1(opr, hque, EVENT_HOST_ONLINE, hostid, SLAVE_PORT, 0, hostid);  
                    
                    /* -------------------------------------------- */
                    /* LOG File 저장                                */
                    /* -------------------------------------------- */
                    sprintf(buffer, "DNP> ID=%2d, => SLAVE-PORT Online ", hostid+1);
                    LogFile_MPU ( opr, EVENT_HOST_ONLINE, buffer, strlen(buffer));
                
                }
                host->online[SLAVE_PORT] = 1;
            
                /* ---------------------------- */
                /* check DATA-LINK Layer  ...   */
                /* ---------------------------- */
                retVal = chkRcvFrameHOST(hostid, SLAVE_PORT, socketID, dnpData);
                if((retVal == FUNC_USER_DATA) || (retVal == FUNC_UNCONFIRM))
                {
                    retVal = checkDataLinkHOST(hostid, SLAVE_PORT, socketID, dnpData);
                    if(retVal)
                    {
                        checkAPPFrame_HOST( hostid, SLAVE_PORT, socketID);
                    }                    
                }
            }   
        } // SLAVE PORT                 

        
        if(oldsec == rtc->sec)  continue;
        oldsec = rtc->sec;

#if 0
        /* for TEST ... */
        sendConfirmHOST( hostid, MASTER_PORT, 0, RCV_ACK);
#endif
        
        /* ------------------------------------ */
        /* HOST-DB 변경에 따른 재기동 ...       */
        /* ------------------------------------ */
        if(opr->hostRestart[hostid] == SET)
        {
            printf("host%02d> *** HOST TASK Restart... DB Chg ...\n", hostid+1);
            pause(1000);
            termExec = RESET;
        }
        
        /* ------------------------------------------------ */
        /*  2016년 3월 5일 - 공항철도 (영종역)-비츠로 HOST  */
        /*  UNSOLICT-메세지 전송  (초)                      */
        /*  - 실제 통신하는 포트로 전송함.                  */
        /* ------------------------------------------------ */
        retVal = check_UNSOLICT_HOST(hostid, host->activePort, socketID);
         
        /* -------------------------------- */
        /* 상위 HOST별 통신상태 Check...    */
        /* -------------------------------- */
        if(++host->comFailTick[MASTER_PORT] > 30)   
        {
            opr->hostComSts[hostid] = 0;
            
            host->comFailTick[MASTER_PORT] = 0;
            if(host->online[MASTER_PORT] != 0)  
            {
                logEvent1(opr, sque, EVENT_HOST_OFFLINE, hostid, MASTER_PORT, 0, hostid);
                logHistoryQue1(opr, hque, EVENT_HOST_OFFLINE, hostid, MASTER_PORT, 0, hostid);
                
                /* -------------------------------------------- */
                /* LOG File 저장                                */
                /* -------------------------------------------- */
                sprintf(buffer, "DNP> ID=%2d, => MASTER-PORT *Offline ", hostid+1);
                LogFile_MPU ( opr, EVENT_HOST_OFFLINE, buffer, strlen(buffer));
            }
            host->online[MASTER_PORT] = 0;
            host->hostActive = HOST_NOT_READY;      // OFFLINE

    /* ---------------------------------------------------- */
    /*  RTU-TYPE : ACE-LBS-6U... ASYNC  PORT Comm Parameter */
    /*  RTU-TYPE : ACE-VMECU ... VMESIO PORT Comm Parameter */
    /* ---------------------------------------------------- */
#ifdef  ACE_LBS_RTU

            PortClose(host->ttyPort[MASTER_PORT]);  pause(100);
            PortInitial((TTY_DESC *) host->ttyPort[MASTER_PORT]); pause(100);
            if(opr->hostDebug == hostid)    Debug(console,">> DNP-HOST(%2d) : Master-ASYNC(%d) Comm Point initial...!\n", hostid+1, host->masterChan);
                
#else            
            /* -------------------------------- */                        
            /* VME Channel Reinitial...         */
            /* VMECHAN 프로토콜 타입 지정       */
            /* -------------------------------- */       
            host->vmeChan[MASTER_PORT]->protocolType = SCAN_DNP;
            host->vmeChan[MASTER_PORT]->cfgType      = host->hostComType;
            host->vmeChan[MASTER_PORT]->cfgSpeed     = host->hostComSpeed;
            
            vmeHostChanReset(hostid, MASTER_PORT);

            if(opr->hostDebug == hostid)    Debug(console,">> DNP-HOST(%2d) : Master Comm Point initial...!\n", hostid+1);
                
            //hostVMEInitial(hostid);  
            vmeHostInitial(hostid, host->masterChan);  
#endif
            
        }
        
        /* DUAL 모드인 경우 : Master Port 가 OFFLINE 인 경우 */
        if(host->hostDualMode == HOST_DUAL)
        {
            if(++host->comFailTick[SLAVE_PORT] > 30)   
            {
                opr->hostComSts[hostid] = 0;
                
                host->comFailTick[SLAVE_PORT] = 0;
                if(host->online[SLAVE_PORT] != 0)  
                {
                    logEvent1(opr, sque, EVENT_HOST_OFFLINE, hostid, SLAVE_PORT, 0, hostid);
                    logHistoryQue1(opr, hque, EVENT_HOST_OFFLINE, hostid, SLAVE_PORT, 0, hostid);
                    
                    /* -------------------------------------------- */
                    /* LOG File 저장                                */
                    /* -------------------------------------------- */
                    sprintf(buffer, "DNP> ID=%2d, => SLAVE-PORT *Offline ", hostid+1);
                    LogFile_MPU ( opr, EVENT_HOST_OFFLINE, buffer, strlen(buffer));
                }
                
                host->online[SLAVE_PORT] = 0;
                host->hostActive = HOST_NOT_READY;      // OFFLINE

                /* ---------------------------------------------------- */
                /*  RTU-TYPE : ACE-LBS-6U... ASYNC  PORT Comm Parameter */
                /*  RTU-TYPE : ACE-VMECU ... VMESIO PORT Comm Parameter */
                /* ---------------------------------------------------- */
#ifdef  ACE_LBS_RTU

                PortClose(host->ttyPort[SLAVE_PORT]);  pause(100);
                PortInitial((TTY_DESC *) host->ttyPort[SLAVE_PORT]); pause(100);
                
                if(opr->hostDebug == hostid)    Debug(console,">> DNP-HOST(%2d) : Slave-ASYNC(%d) Comm Point initial...!\n", hostid+1, host->slaveChan);
            
#else            
                /* -------------------------------- */                        
                /* VME Channel Reinitial...         */
                /* VMECHAN 프로토콜 타입 지정       */
                /* -------------------------------- */       
                host->vmeChan[SLAVE_PORT]->protocolType = SCAN_DNP;
                host->vmeChan[SLAVE_PORT]->cfgType      = host->hostComType;
                host->vmeChan[SLAVE_PORT]->cfgSpeed     = host->hostComSpeed;
                
                vmeHostChanReset(hostid, SLAVE_PORT);
            
                if(opr->hostDebug == hostid)
                Debug(console,">> DNP-HOST(%2d) : Slave Comm Point initial...!\n", hostid+1);                           
                
                vmeHostInitial(hostid, host->slaveChan);  
#endif
                
            }
        }
                
	}/* while */

    return (0);
}
