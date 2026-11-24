
#include	"localLib.h"
#include    "external.h"

extern  int     hostChanRead(int hostid, int chid, byte *rxbuf, int reqCount);


extern  int     hostVMERead(int hostid, int chid, byte *rxbuf);
extern  int     vmeHostInitial(int hostid, int chid);
extern  int     vmeHostChanReset(int hostid, int chid);
extern  int     chkRcvLink_IEC( int hostid, int chid, IEC_DATA_LINK *iecData);

extern  int	    funcASDU_single_command(int hostid, IEC_DATA_LINK *iecData, byte *appData);
extern  int     checkAPPFrame_IEC(int hostid, int chid, IEC_DATA_LINK *iecData);
extern  int	    funcASDU_M_SP_NA(int hostid, IEC_DATA_LINK *iecData);
extern  int	    funcASDU_M_SP_TA(int hostid, IEC_DATA_LINK *iecData);
extern  int	    funcASDU_normal_measure_NB(int hostid, IEC_DATA_LINK *iecData);
extern  int     sendDataLink_IEC(int hostid, int chid,IEC_DATA_LINK *iecData, int count);

extern  TASK_INFO	    *taskPtr;
extern  int             termExec;

extern  int             wdtHostFlag[MAX_HOST];
extern  byte            vmeRxbuf[MAX_HOST][1024];

/* HOST 통신용 ASYNC PORT */
extern  TTY_DESC tty_HostChan[MAX_CPUPORT];

/* -------------------------------------------- */
/*  IEC-보호계전기 Relative Variable Define     */
/* -------------------------------------------- */
//IEC_DATA_LINK   dataLinkIEC[MAX_HOST];

/***************************************************
*   FUNCTION : readDataLinkIEC()
*   receive DATA-LINK with CRC
***************************************************/
int readDataLinkIEC(int hostid, int chid, IEC_DATA_LINK *iecData)
{
    int     index;
    int     rxcnt, rcvCount;
    byte    *rxbuf, stxData;
    
#ifndef  ACE_LBS_RTU    
    int     i;
    byte    *vmedata;
#endif
    
    byte	chksum;
    word    timeout;

    HOST_DCB    *host;
    TTY_DESC    *tp;

    /* ------------------------------------ */
    /*  HOST 별 통신 Frame 설정             */
    /* ------------------------------------ */
    host = (HOST_DCB *) hostDCB[hostid];
    tp   = (TTY_DESC *) host->ttyPort[chid];
    
    /* ---------------------------------------------------- */
    /*  RTU-TYPE : ACE-LBS-6U... ASYNC  PORT Comm Parameter */
    /*  RTU-TYPE : ACE-VMECU ... VMESIO PORT Comm Parameter */
    /* ---------------------------------------------------- */

#ifdef  ACE_LBS_RTU
    tp   = (TTY_DESC *) host->ttyPort[chid];

    rxcnt   = 0;
    rcvCount = 0;
    timeout = 0;
    index = 0;
    iecData->rcvFrameOK	= RESET;
    iecData->rcvCount 	= 0;
    iecData->frameType 	= 0;

    do {
        if(hostChanRead(hostid, chid, &stxData, 1) == 0) return (0);
        if(++timeout > 50)  return (0);
        
        /* check FRAME-TYPE : 고정 Frame, 가변 Frame */
        if(stxData == PHYS_FIXED_STX)
        {
        	iecData->frameType = IEC_LINK_FIXED_TYPE;       // 고정 Frame 지정
        	rxbuf = (byte *) &iecData->rcvFrame1;
        	rxbuf[0] = stxData;
        	break;
        }
        else if(stxData == PHYS_VARIABLE_STX)
        {
        	iecData->frameType = IEC_LINK_VARIABLE_TYPE;    // 가변 Frame 지정
        	rxbuf = (byte *) &iecData->rcvFrame2;
        	rxbuf[0] =stxData;
        	break;
        }
        else 
        {
        	iecData->frameType = IEC_LINK_NULL_TYPE;
        }    
            
    } while(1);

    /* ------------------------------------ */
    /* FRAME Type별 수신하고자 하는 Data 수 */
    /* ------------------------------------ */
	if(iecData->frameType == IEC_LINK_FIXED_TYPE)
	{
		iecData->reqCount = PHYS_FIXED_LENGTH - 1;
		rxcnt = 1;
	}
	else if(iecData->frameType == IEC_LINK_VARIABLE_TYPE)
	{
	    if(hostChanRead(hostid, chid, &rxbuf[1], 3) == 0) return (0);
		iecData->reqCount = rxbuf[1] + 6 - 4;
		rxcnt = 4;
	}
	
	/* ------------------------------------ */
    /* Data Body ...                        */
    /* ------------------------------------ */
    if((rcvCount = hostChanRead(hostid, chid, &rxbuf[rxcnt], iecData->reqCount)) != iecData->reqCount) 
    {
        if(opr->hostDebug == hostid)
        printf("hrcv%2d> *rcv IEC data - Tail Not Received ... req=%02x, rcv=%02x !\n", hostid+1, iecData->reqCount, rcvCount);
        return (0);
    }
    
    rxcnt = rxcnt + rcvCount;
    
    /* ------------------------------------ */
    /* FRAME ERROR 검출 : Chksum ...		*/
    /* - HEADER 를 제외한 나머지 영역		*/
    /* ------------------------------------ */
	if(iecData->frameType == IEC_LINK_FIXED_TYPE)		// 고정 Frame의 경우
	{
		chksum = genSumByte( &rxbuf[1], rxcnt - 3); 
	}
	else												// 가변 Frame의 경우
	{
		chksum = genSumByte( &rxbuf[4], rxcnt - 6); 
	}
	
    if(chksum != rxbuf[rxcnt - 2])
	{
		if(opr->hostDebug == hostid)
        printf("hrcv%2d> *rcv IEC data - Chksum Error ... cal=%02x, rcv=%02x !\n", hostid+1, chksum, rxbuf[rxcnt - 2]);
        return (0);
	}	

	return (rxcnt);
	
#else
    
    rxcnt   = 0;
    rcvCount = 0;
    timeout = 0;
    index = 0;
    iecData->rcvFrameOK	= RESET;
    iecData->rcvCount 	= 0;
    iecData->frameType 	= 0;

    vmedata = (byte *) &vmeRxbuf[hostid][0];
    
    /* ------------------------------------------------ */
    /*  VMEBUS 에서 수신된 Packet Data 수신...          */
    /* ------------------------------------------------ */
    rcvCount = hostVMERead(hostid, chid, vmedata);
    if(rcvCount <= 0)   return (0);
    
    /* ------------------------------------ */
    /*  IEC 프로토콜 : HEADER 찾음          */
    /* ------------------------------------ */
    do {
        stxData = vmedata[index];
        if(++timeout > 50)  return (0);
        
        /* check FRAME-TYPE : 고정 Frame, 가변 Frame */
        if(stxData == PHYS_FIXED_STX)
        {
        	iecData->frameType = IEC_LINK_FIXED_TYPE;       // 고정 Frame 지정
        	rxbuf = (byte *) &iecData->rcvFrame1;
        	rxbuf[0] = vmedata[index];
        	break;
        }
        else if(stxData == PHYS_VARIABLE_STX)
        {
        	iecData->frameType = IEC_LINK_VARIABLE_TYPE;    // 가변 Frame 지정
        	rxbuf = (byte *) &iecData->rcvFrame2;
        	rxbuf[0] = vmedata[index];
        	break;
        }
        else 
        {
        	iecData->frameType = IEC_LINK_NULL_TYPE;
        }
        
        index++;
                		
    } while(1);

    index++;
    
	/* ------------------------------------ */
    /* FRAME Type별 수신하고자 하는 Data 수 */
    /* ------------------------------------ */
	if(iecData->frameType == IEC_LINK_FIXED_TYPE)
	{
		iecData->reqCount = PHYS_FIXED_LENGTH - 1;
		rxcnt = 1;
	}
	else if(iecData->frameType == IEC_LINK_VARIABLE_TYPE)
	{
		for(i=0; i < 3; i++)    rxbuf[i + 1] = vmedata[index++];	
		iecData->reqCount = rxbuf[1] + 6 - 4;
		rxcnt = 4;
	}

    /* ------------------------------------ */
    /* Data Body ...                        */
    /* ------------------------------------ */
    for(i=0; i < iecData->reqCount; i++)    rxbuf[rxcnt++] = vmedata[index++];	
    
	/* ------------------------------------ */
    /* FRAME ERROR 검출 : Chksum ...		*/
    /* - HEADER 를 제외한 나머지 영역		*/
    /* ------------------------------------ */
	if(iecData->frameType == IEC_LINK_FIXED_TYPE)		// 고정 Frame의 경우
	{
		chksum = genSumByte( &rxbuf[1], rxcnt - 3); 
	}
	else												// 가변 Frame의 경우
	{
		chksum = genSumByte( &rxbuf[4], rxcnt - 6); 
	}
	
	if(chksum != rxbuf[rxcnt - 2])
	{
		//if(opr->hostDebug == hostid)
        printf("hrcv%2d> *rcv IEC data - Chksum Error ... cal=%02x, rcv=%02x !\n", hostid+1, chksum, rxbuf[rxcnt - 2]);
        return (0);
	}	

	return (rxcnt);

#endif
        
}

/*
*   IEC-HOST 수신부 Addres Check ...
*/
int	chkAddress_IEC(int hostid, int addr, int rcvAddr)
{

	if(addr != rcvAddr)
	{
		//if(opr->hostDebug == hostid)
        printf("hrcv%2d> *1 HOST Addr mismatch : [rcv %2d, cu-host %2d] ... !\n", hostid, rcvAddr, addr);

		return (0);
	}	
		
	return (1);
}


/* -------------------------------------------------------- */
/*  IEC-HOST MAIN 프로그램 Start Routine ....               */
/* -------------------------------------------------------- */
int    hostIECThread(int hostid)
{
	int		oldsec= 0;
	int     retVal;
	int     rxcnt;
	int		sndCount;
	char    buffer[128];
	
	IEC_DATA_LINK   *iecData;

	HOST_DCB        *host;
	
	IEC_FIX_FRAME	*rcvFixLink;
    IEC_VAR_FRAME	*rcvVarLink;
    IEC_VAR_FRAME	*rcvControl;

#ifdef  ACE_LBS_RTU    
    TTY_DESC        *ttyHost;
#endif
    
	/* ---------------------------------------- */
	/*  HOST별 구조체 초기화  ...               */
	/* ---------------------------------------- */
    host = (HOST_DCB *) hostDCB[hostid];        /* 주장치 #1 속성정의 */
    
    host->online[MASTER_PORT] = 0;              // default OFFLINE
    host->online[SLAVE_PORT]  = 0;              // default OFFLINE
    host->comFailTick[MASTER_PORT] = 0;
    host->comFailTick[SLAVE_PORT]  = 0;

    /* HOST 별 데이터 구조체 */
	iecData = (IEC_DATA_LINK *) &host->dataLinkIEC;
	
	opr->hostIEC    = SET;
	
	opr->iecControl[hostid] = CONTROL_CLEAR;
	
	opr->IEC_cpuChange = 0;
    opr->IEC_ChangeTick= 0;

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
            printf("host%02d> *IEC-HOST-Dual(%d)Serial Com Port Open...ERROR...return = %d \n", hostid+1, host->masterChan, retVal);
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
            printf("host%02d> *IEC-HOST-Dual(%d) Serial Com Port Open...ERROR...return = %d \n", hostid+1, host->slaveChan, retVal);
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
            printf("host%02d> *IEC-HOST-Single(%d) Serial Com Port Open...ERROR...return = %d \n", hostid+1, host->masterChan, retVal);
            printf("===========================================\n");
            pause(1000);
            pause(1000);
        }
        
        host->ttyPort[MASTER_PORT] = (TTY_DESC *) ttyHost;
    }
    else
    {
    }              
    
#else                
	/* -------------------------------------------- */
    /*  VMESIO Channel Comm Parameter 초기화        */
    /* -------------------------------------------- */
    host->vmeChan[MASTER_PORT]->protocolType = SCAN_IEC;            // 8bit, Even Parity
    host->vmeChan[MASTER_PORT]->cfgType      = host->hostComType;
    host->vmeChan[MASTER_PORT]->cfgSpeed     = host->hostComSpeed;
    
    host->vmeChan[SLAVE_PORT]->protocolType  = SCAN_IEC;            // 8bit, Even Parity
    host->vmeChan[SLAVE_PORT]->cfgType       = host->hostComType;
    host->vmeChan[SLAVE_PORT]->cfgSpeed      = host->hostComSpeed;
    
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
        printf(" %s HOST[%d] IEC-THREAD *DUAL [%d/%d]*  Activated ... !\n", TARGET_NAME, hostid+1, host->masterChan, host->slaveChan);
        sprintf(buffer, "%s HOST[%d] IEC-THREAD *DUAL [%d/%d]*  Activated ... !", TARGET_NAME, hostid+1, host->masterChan, host->slaveChan);
        LogFile_MPU ( opr, NOT_DEFINED, buffer, strlen(buffer));
    }
    else if(host->hostDualMode == HOST_SINGLE)      // 개별 구조인 경우 
    {
        printf(" %s HOST[%d] IEC-THREAD *SINGLE [%d/%d]*  Activated ... !\n", TARGET_NAME, hostid+1, host->masterChan, host->slaveChan);
        sprintf(buffer, "%s HOST[%d] IEC-THREAD *SINGLE [%d/%d]*  Activated ... !", TARGET_NAME, hostid+1, host->masterChan, host->slaveChan);
        LogFile_MPU ( opr, NOT_DEFINED, buffer, strlen(buffer));
    }        
    else         // 사용하지 않는 경우 
    {
        printf(" %s HOST[%d] IEC-THREAD *NOT Define [%d/%d]*  Activated ... !\n", TARGET_NAME, hostid+1, host->masterChan, host->slaveChan);
        sprintf(buffer, "%s HOST[%d] IEC-THREAD *NOT Define [%d/%d]*  Activated ... !", TARGET_NAME, hostid+1, host->masterChan, host->slaveChan);
        LogFile_MPU ( opr, NOT_DEFINED, buffer, strlen(buffer));
        
        while(termExec)
	    {
	        wdtHostFlag[hostid] = 0;
		    pause(1000);        
		    
		    opr->mmiHostSts[hostid] = 3;    // 사용않함.    
		    
		    if(opr->wdtDebug)
		    Debug(console,">> *HOST[%d] IEC-THREAD *NOT Define ... !\n", hostid+1);
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
        /*  IEC-MASTER 통신포트 : 상위 데이터 수신 처리 ...     */
        /* ---------------------------------------------------- */
        if((rxcnt = readDataLinkIEC( hostid, MASTER_PORT, iecData)) > 0)
        {
            sndCount = 0;
            
            host->comFailTick[MASTER_PORT]	= 0;
            opr->hostComSts[hostid] 		= 1;
            
            /* 통신 이중화의 경우 ...SLAVE 통신정상 */
            if(host->hostDualMode == HOST_DUAL) host->comFailTick[SLAVE_PORT]= 0; 
            
            /* ------------------------------------ */
    		/* 수신 메세지 처리 ...			        */
    		/* - 수신 FRAME-Type 별 응답		    */
    		/* ------------------------------------ */
			if(iecData->frameType == IEC_LINK_FIXED_TYPE)		// 고정 Frame의 경우
			{
				rcvFixLink = (IEC_FIX_FRAME *) &iecData->rcvFrame1;
				
				/* 수신메세지... Address 비교 */
				if(chkAddress_IEC(hostid, host->rtuAddr, rcvFixLink->address) == 0)	continue;
				    
				if(opr->hostDebug == hostid)
				{
					printf("-------------------------> [%02d:%02d:%02d] \n", rtc->hour, rtc->min, rtc->sec);		
			 		prn_rcv("RXM:", (char *)&rcvFixLink->start1, rxcnt);	
			 	}
			 	
			}
			else
			{
				rcvVarLink = (IEC_VAR_FRAME *) &iecData->rcvFrame2;	
				
				/* 수신메세지... Address 비교 */
				if(chkAddress_IEC(hostid, host->rtuAddr, rcvVarLink->address) == 0)	continue;
				    
				if(opr->hostDebug == hostid)
				{
					printf("-------------------------> [%02d:%02d:%02d] \n", rtc->hour, rtc->min, rtc->sec);	
					prn_rcv("RXM:", (char *)&rcvVarLink->start1, rxcnt);	
				}
				
			}
		
			iecData->rcvFrameOK	= SET;
            iecData->rcvCount	= rxcnt;
            
            if(host->online[MASTER_PORT] != 1)  
            {
                logEvent1(opr, sque, EVENT_HOST_ONLINE, hostid, MASTER_PORT, 0, hostid);  
                logHistoryQue1(opr, hque, EVENT_HOST_ONLINE, hostid, MASTER_PORT, 0, hostid);  
                
                /* -------------------------------------------- */
                /* LOG File 저장                                */
                /* -------------------------------------------- */
                sprintf(buffer, "IEC> ID=%2d, => MASTER-PORT Online ", hostid+1);
                LogFile_MPU ( opr, EVENT_HOST_ONLINE, buffer, strlen(buffer));
            }
            
            host->online[MASTER_PORT] = 1;
			
			/* ------------------------------------------------ */
            /* check DATA-LINK Layer  : 1차국 메세지 해석...    */
            /* ------------------------------------------------ */
			retVal = chkRcvLink_IEC(hostid, MASTER_PORT, iecData);
			if(retVal < 0)  continue;
			
			/* ------------------------------------------------ */
			/*  2015년 2월 7일 : CPU 절체후 연속적인 통신...    */
			/*  - RESET Link가 없더라도 통신수행...             */
			/* ------------------------------------------------ */
			host->rcvLinkSts = SET;             		// LINK RESET 상태 
			
			/* ------------------------------------------------ */
			/*  IEC-HOST 제어실행후 ... Termination 전송        */
			/* ------------------------------------------------ */    
            if(opr->iecControl[hostid] == CONTROL_OPERATE)
			{
			    rcvControl = (IEC_VAR_FRAME *) &iecData->rcvControl;	
			    sndCount   = funcASDU_single_command(hostid, iecData, &rcvControl->userData[5]);
			    iecData->sndFrameType = IEC_LINK_VARIABLE_TYPE;
			    
		        opr->iecControl[hostid] = CONTROL_CLEAR;
		        opr->IEC_ChangeTick= 3;

			    if(opr->soeDebug)   
			    printf(">> Control Termination Send...%d  : Control Clear...\n", sndCount);
			}        
		    else if(retVal == LINK_FUNC_CNFRM_DATA)		sndCount = checkAPPFrame_IEC( hostid, MASTER_PORT, iecData);
			else if(retVal == LINK_FUNC_UNCONFIRM)		sndCount = checkAPPFrame_IEC( hostid, MASTER_PORT, iecData);	
	        else if(retVal == LINK_FUNC_REQ_CLASS1)		sndCount = funcASDU_M_SP_TA(hostid, iecData);	/* ASDU => IEC_TYPE_M_SP_TA, Single-point information, with time tag */
    	    else if(retVal == LINK_FUNC_REQ_CLASS2)		
    	    {
    	        if(host->aiPtNum > 0)
    	        {
    	            if(host->analogDump) sndCount = funcASDU_normal_measure_NB( hostid, iecData); /* 아날로그... */
    	            else                 sndCount = funcASDU_M_SP_NA(hostid, iecData);	         /* ASDU => IEC_TYPE_M_SP_NA, Single-point information, without time tag */   
  	                host->analogDump = 0; 
    	        }
    	        else
    	        {          
    	            sndCount = funcASDU_M_SP_NA(hostid, iecData);	/* ASDU => IEC_TYPE_M_SP_NA, Single-point information, without time tag */
    	        }
    	    }
    	            
            /* ---------------------------- */
            /*  CU 응답메세지... 출력       */
            /* ---------------------------- */
           	if(sndCount)	sendDataLink_IEC(hostid, MASTER_PORT, iecData, sndCount);
           	continue;    
           	
        } // MASTER PORT      
        
        /* ---------------------------------------------------- */
        /*  SLAVE  통신포트 : 상위 데이터 수신 처리 ...         */
        /* ---------------------------------------------------- */        
        if(host->hostDualMode == HOST_DUAL)
        {
			/* ---------------------------------------------------- */
    	    /*  SLAVE 통신포트 : 상위 데이터 수신 처리 ...         */
        	/* ---------------------------------------------------- */
	        if((rxcnt = readDataLinkIEC( hostid, SLAVE_PORT, iecData)) > 0)
    	    {
    	        sndCount = 0;
    	        
        	    host->comFailTick[SLAVE_PORT]	= 0;
            	opr->hostComSts[hostid] 		= 1;
            
	            /* 통신 이중화의 경우 ...SLAVE 통신정상 */
    	        if(host->hostDualMode == HOST_DUAL) host->comFailTick[MASTER_PORT]= 0; 
            
                /* ------------------------------------ */
    		    /* 수신 메세지 처리 ...			        */
    		    /* - 수신 FRAME-Type 별 응답		    */
    		    /* ------------------------------------ */
				if(iecData->frameType == IEC_LINK_FIXED_TYPE)		// 고정 Frame의 경우
				{
					rcvFixLink = (IEC_FIX_FRAME *) &iecData->rcvFrame1;
				
				    if(chkAddress_IEC(hostid, host->rtuAddr, rcvFixLink->address) == 0)	continue;
				    
					if(opr->hostDebug == hostid)
					{
						printf("-------------------------> [%02d:%02d:%02d] \n", rtc->hour, rtc->min, rtc->sec);	
				 		prn_rcv("RXS:", (char *)&rcvFixLink->start1, rxcnt);	
				 	}
					
				}
				else
				{
					rcvVarLink = (IEC_VAR_FRAME *) &iecData->rcvFrame2;	
				
				    if(chkAddress_IEC(hostid, host->rtuAddr, rcvVarLink->address) == 0)	continue;
				        
					if(opr->hostDebug == hostid)
					{
						printf("-------------------------> [%02d:%02d:%02d] \n", rtc->hour, rtc->min, rtc->sec);	
						prn_rcv("RXS:", (char *)&rcvVarLink->start1, rxcnt);	
					}
					
				}
		
				iecData->rcvFrameOK	= SET;
    	        iecData->rcvCount	= rxcnt;
    	        
    	        if(host->online[SLAVE_PORT] != 1)  
                {
                    logEvent1(opr, sque, EVENT_HOST_ONLINE, hostid, SLAVE_PORT, 0, hostid);  
                    logHistoryQue1(opr, hque, EVENT_HOST_ONLINE, hostid, SLAVE_PORT, 0, hostid);  
                    
                    /* -------------------------------------------- */
                    /* LOG File 저장                                */
                    /* -------------------------------------------- */
                    sprintf(buffer, "IEC> ID=%2d, => SLAVE-PORT Online ", hostid+1);
                    LogFile_MPU ( opr, EVENT_HOST_ONLINE, buffer, strlen(buffer));
                }
                
	            host->online[SLAVE_PORT] = 1;
			
				/* ------------------------------------------------ */
                /* check DATA-LINK Layer  : 1차국 메세지 해석...    */
                /* ------------------------------------------------ */
				retVal = chkRcvLink_IEC(hostid, SLAVE_PORT, iecData);
			    if(retVal < 0)  continue;
			    
			    /* ------------------------------------------------ */
			    /*  2015년 2월 7일 : CPU 절체후 연속적인 통신...    */
			    /*  - RESET Link가 없더라도 통신수행...             */
			    /* ------------------------------------------------ */
			    host->rcvLinkSts = SET;             		// LINK RESET 상태 
			    
			    /* ------------------------------------------------ */
			    /*  IEC-HOST 제어실행후 ... Termination 전송        */
			    /* ------------------------------------------------ */   
			    if(opr->iecControl[hostid] == CONTROL_OPERATE)
			    {
			        rcvControl = (IEC_VAR_FRAME *) &iecData->rcvControl;	
			        sndCount   = funcASDU_single_command(hostid, iecData, &rcvControl->userData[5]);
			        iecData->sndFrameType = IEC_LINK_VARIABLE_TYPE;
			        
		            opr->iecControl[hostid] = CONTROL_CLEAR;
		            opr->IEC_ChangeTick= 3;
        	        if(opr->soeDebug)   
        	        printf(">> Control Termination Send...%d  : Control Clear...\n", sndCount);
			    }        
				else if(retVal == LINK_FUNC_CNFRM_DATA)		sndCount = checkAPPFrame_IEC( hostid, SLAVE_PORT, iecData);
				else if(retVal == LINK_FUNC_UNCONFIRM)		sndCount = checkAPPFrame_IEC( hostid, SLAVE_PORT, iecData);	
	            else if(retVal == LINK_FUNC_REQ_CLASS1)		sndCount = funcASDU_M_SP_TA(hostid, iecData);	/* ASDU => IEC_TYPE_M_SP_TA, Single-point information, with time tag */
        	    else if(retVal == LINK_FUNC_REQ_CLASS2)		
        	    {
    	            if(host->aiPtNum > 0)
    	            {
    	                if(host->analogDump) sndCount = funcASDU_normal_measure_NB( hostid, iecData); /* 아날로그... */
        	            else                 sndCount = funcASDU_M_SP_NA(hostid, iecData);	         /* ASDU => IEC_TYPE_M_SP_NA, Single-point information, without time tag */   
  	                    host->analogDump = 0; 
    	            }
    	            else
    	            {          
    	                sndCount = funcASDU_M_SP_NA(hostid, iecData);	/* ASDU => IEC_TYPE_M_SP_NA, Single-point information, without time tag */
    	            }
    	        }
    	        
                /* ---------------------------- */
                /*  CU 응답메세지... 출력       */
                /* ---------------------------- */
	           	if(sndCount)	sendDataLink_IEC(hostid, SLAVE_PORT, iecData, sndCount);
	            continue;	    
        	}	
        }
        
        
        if(oldsec == rtc->sec)	continue;
        oldsec = rtc->sec;	
        
        /* ------------------------------------ */
        /* HOST-DB 변경에 따른 재기동 ...       */
        /* ------------------------------------ */
        if(opr->hostRestart[hostid] == SET)
        {
            printf("host%02d> *** HOST TASK Restart... DB Chg ...\n", hostid+1);
            pause(1000);
            termExec = RESET;
        }
        
        /* -------------------------------------------------------- */
        /* IEC Control : CPU Change... 명령수신후 3초이후           */
        /* -------------------------------------------------------- */            
        if(opr->IEC_cpuChange == SET)
        {
            if(++opr->IEC_ChangeTick > 3)
            {
                opr->IEC_cpuChange = 0;
                opr->IEC_ChangeTick= 0;
                opr->cpuChange = SET; 
            }    
        }
              
        /* -------------------------------------------------------- */
        /* 상위 HOST별 통신상태 Check... (MASTER PORT)              */
        /* -------------------------------------------------------- */
        if(++host->comFailTick[MASTER_PORT] > 20)   
        {
            
            opr->hostComSts[hostid] = 0;
            opr->iecControl[hostid] = CONTROL_CLEAR;
            
            host->comFailTick[MASTER_PORT] = 0;
            
            if(host->online[MASTER_PORT] != 0)  
            {
                logEvent1(opr, sque, EVENT_HOST_OFFLINE, hostid, MASTER_PORT, 0, hostid);
                logHistoryQue1(opr, hque, EVENT_HOST_OFFLINE, hostid, MASTER_PORT, 0, hostid);
                
                /* -------------------------------------------- */
                /* LOG File 저장                                */
                /* -------------------------------------------- */
                sprintf(buffer, "IEC> ID=%2d, => MASTER-PORT *Offline ", hostid+1);
                LogFile_MPU ( opr, EVENT_HOST_OFFLINE, buffer, strlen(buffer));
            }
            host->online[MASTER_PORT] = 0;

    /* ---------------------------------------------------- */
    /*  RTU-TYPE : ACE-LBS-6U... ASYNC  PORT Comm Parameter */
    /*  RTU-TYPE : ACE-VMECU ... VMESIO PORT Comm Parameter */
    /* ---------------------------------------------------- */
#ifdef  ACE_LBS_RTU

            PortClose(host->ttyPort[MASTER_PORT]);  pause(100);
            PortInitial((TTY_DESC *) host->ttyPort[MASTER_PORT]); pause(100);
            if(opr->hostDebug == hostid)    
            Debug(console,">> IEC-HOST(%2d) : Master-ASYNC(%d) Comm Point initial...!\n", hostid+1, host->masterChan);
                
#else                     
            /* -------------------------------- */                        
            /* VME Channel Reinitial...         */
            /* VMECHAN 프로토콜 타입 지정       */
            /* -------------------------------- */       
            host->vmeChan[MASTER_PORT]->protocolType = SCAN_IEC;
            host->vmeChan[MASTER_PORT]->cfgType      = host->hostComType;
            host->vmeChan[MASTER_PORT]->cfgSpeed     = host->hostComSpeed;
            
            vmeHostChanReset(hostid, MASTER_PORT);
            vmeHostInitial(hostid, host->masterChan); 
            
            if(opr->hostDebug == hostid)    
            Debug(console,">> IEC-HOST(%2d) : Master Comm Port initial...!\n", hostid+1);
            
#endif            
        }		

        /* -------------------------------------------------------- */
		/*  DUAL 모드인 경우 : Master Port 가 OFFLINE 인 경우       */
		/* -------------------------------------------------------- */
        if(host->hostDualMode == HOST_DUAL)
        {
            if(++host->comFailTick[SLAVE_PORT] > 20)   
            {
                
                opr->hostComSts[hostid] = 0;
                opr->iecControl[hostid] = CONTROL_CLEAR;
                
                host->comFailTick[SLAVE_PORT] = 0;
            
                if(host->online[SLAVE_PORT] != 0)  
                {
                    logEvent1(opr, sque, EVENT_HOST_OFFLINE, hostid, SLAVE_PORT, 0, hostid);
                    logHistoryQue1(opr, hque, EVENT_HOST_OFFLINE, hostid, SLAVE_PORT, 0, hostid);
                    
                    /* -------------------------------------------- */
                    /* LOG File 저장                                */
                    /* -------------------------------------------- */
                    sprintf(buffer, "IEC> ID=%2d, => SLAVE-PORT *Offline ", hostid+1);
                    LogFile_MPU ( opr, EVENT_HOST_OFFLINE, buffer, strlen(buffer));
                }
                host->online[SLAVE_PORT] = 0;

    /* ---------------------------------------------------- */
    /*  RTU-TYPE : ACE-LBS-6U... ASYNC  PORT Comm Parameter */
    /*  RTU-TYPE : ACE-VMECU ... VMESIO PORT Comm Parameter */
    /* ---------------------------------------------------- */
#ifdef  ACE_LBS_RTU

                PortClose(host->ttyPort[SLAVE_PORT]);  pause(100);
                PortInitial((TTY_DESC *) host->ttyPort[SLAVE_PORT]); pause(100);
                
                if(opr->hostDebug == hostid)    
                Debug(console,">> IEC-HOST(%2d) : Slave-ASYNC(%d) Comm Point initial...!\n", hostid+1, host->slaveChan);
            
#else                  
                /* -------------------------------- */                        
                /* VME Channel Reinitial...         */
                /* VMECHAN 프로토콜 타입 지정       */
                /* -------------------------------- */       
                host->vmeChan[SLAVE_PORT]->protocolType = SCAN_IEC;
                host->vmeChan[SLAVE_PORT]->cfgType      = host->hostComType;
                host->vmeChan[SLAVE_PORT]->cfgSpeed     = host->hostComSpeed;
                
                vmeHostChanReset(hostid, SLAVE_PORT);
            
                vmeHostInitial(hostid, host->slaveChan);                    
                
                if(opr->hostDebug == hostid)
                Debug(console,">> IEC-HOST(%2d) : Slave Comm Port initial...!\n", hostid+1); 
                
#endif
                
            }
        }
        
	}/* while */

    return (0);
}

