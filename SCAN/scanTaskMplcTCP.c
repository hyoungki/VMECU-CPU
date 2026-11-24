#include	"localLib.h"
#include    "external.h"

extern  int     setOffline( int devid);
extern  int     setOnline( int devid);
extern  int     checkEXPcode(int scanid, int devid, int expCode);
extern  int     dnpFlagClear( int devid);

extern  int     _initNetClientSCAN(int scanid, int devid, int actChanID);
extern  int     _reConnectClientPLC(int scanid, int devid, int actChanID);

extern  int checkBlock_MPLC(int scanid, int devid,  int opcode, byte *rxbuf, int dataLen);

extern  TTY_DESC scanChan[MAX_CPUPORT];

extern  int             termExec;
extern  TASK_INFO	    *taskPtr;

extern  PDPDTIME_DATE_TIME      dnpRTC;
extern  PDPDTIME_MS_SINCE_70    dnpRTCInfo;


extern  THREAD_ENTRY    MOD_THREAD[MAX_MODULE];

extern  int     ThreadActive[MAX_MODULE];


/*
*/
int rcvHandler_MPLC(int scanid, int devid, byte *rxbuf, int rxcnt)
{
    //int 	count=0;
    int     i;
    int 	opcode;
    //int 	stsCount;
    int 	dataLen;
    int     mode=0;
    int     point;
    word    cntrBase;
    int		readIndex;
    char    buffer[128];
    
    DNP_DEVICE  *dnp;
    POINT_BUF   *ptBuf;
    LOCK_POINT_BUF  *locPoint;
    MODBUS_READ	*readBlock;

    dnp = (DNP_DEVICE *) dnpcfg[devid];    

    opcode = rxbuf[0];

    switch(opcode)
    {
    case MPLC_RD_RESP:       // (0x01) Read Single-BIT

        readIndex 	   = dnp->readIndex;
    		
    	/* -------------------------------------------- */
    	/* 각 블록의 정의된 READ 블록 찾음...           */
    	/* -------------------------------------------- */
    	readBlock = (MODBUS_READ *) &dnp->readBlock[readIndex];

        dataLen = rxcnt - 2;
        checkBlock_MPLC(scanid, devid, opcode, &rxbuf[2], dataLen);  
        break;
    
    case MPLC_WR_RESP:       // (0x04) Read Single-BIT
        /* -------------------------------------------- */
        /* 제어포인트의 Control-Base Check...           */
        /* -------------------------------------------- */
        point   = dnp->cntPoint;
        ptBuf = (POINT_BUF *) &dnp->doPtBuf[point];
        cntrBase = ptBuf->modIndex;         // M-xxx 제어포인트 인덱스 (INDEX Address)
        
        /* -------------------------------------------- */
        /* 제어 Flag 상태에 따른 후속제어 ...           */
        /* -------------------------------------------- */
        if(dnp->selectReq == SET)  
        {
            dnp->selectReq = RESET;
            dnp->operateReq= RESET;
            
            /* ---------------------------------------- */
            /* SELECT 제어이후... Clear 제어상태 요청   */
            /* ---------------------------------------- */
            dnp->clearReq  = SET;
            dnp->clearTick = 0;
            
            if(opr->soeDebug)
            Debug(console,"rcv%2d> rcv Select OK ...point=%d, TCF[TRIP=0x80, CLOSE=0x40]= %2x \n", devid+1, dnp->cntPoint+1, dnp->cntTcf);
            
            /* 계전기 제어정보 : SELECT */
            logHistoryQue1( opr, hque, EVENT_MOD_SELECT, devid, dnp->cntPoint, dnp->cntTcf, 0);       // CONSOLE - 제어 이벤트  
            
            /* -------------------------------- */
            /* LOG File 저장                    */
            /* -------------------------------- */
            sprintf(buffer, "--> mplc> Select OK : Dev=%d, PT=%d, TCF[TRIP=0x80, CLOSE=0x40]= %2x... ", devid+1, dnp->cntPoint+1, dnp->cntTcf);
            LogFile_MPU (opr, EVENT_MOD_SELECT, buffer, strlen(buffer));      
        }
        else if(dnp->operateReq == SET) 
        {
            dnp->selectReq = RESET;
            dnp->operateReq= RESET;  
            dnp->clearReq  = RESET;
            dnp->clearTick = 0;
            
            if(opr->soeDebug)
            Debug(console,"rcv%2d> rcv Operate OK ...point=%d, TCF[TRIP=0x80, CLOSE=0x40]= %2x \n", devid+1, dnp->cntPoint+1, dnp->cntTcf);
            
            
            /* 계전기 제어정보 : SELECT */
            logHistoryQue1( opr, hque, EVENT_MOD_OPERATE, devid, dnp->cntPoint, dnp->cntTcf, 0);       // CONSOLE - 제어 이벤트     
            
            /* -------------------------------- */
            /* LOG File 저장                    */
            /* -------------------------------- */
            sprintf(buffer, "--> mplc> Operate OK : Dev=%d, PT=%d, TCF[TRIP=0x80, CLOSE=0x40]= %2x... ", devid+1, dnp->cntPoint+1, dnp->cntTcf);
            LogFile_MPU (opr, EVENT_MOD_OPERATE, buffer, strlen(buffer));   

            /* ------------------------------------------------------------ */
            /*  계전기 전체 쇄정/해정 포인트인 경우...                      */
            /*  - 전체 쇄정/해정 성공시...개별 쇄정/해정 제어설정...        */
            /*  - 개별 쇄정/해정 성공시...개별 쇄정/해정 제어상태 Clear     */
            /* ------------------------------------------------------------ */
            if(ptBuf->ptType == DLOCKPT)
            {
                if(opr->soeDebug) 
                Debug(console,"--> mplc> Device(%2d) 전체 쇄정/해정... RESET OK ...point=%d ... TCF[해정=0x80, 쇄정=0x40]= %2x\n", devid+1, dnp->cntPoint+1, dnp->cntTcf);
               
                /* ------------------------------------- */
                /* 개별 쇄정/해정 포인트 ... 제어설정... */
                /* ------------------------------------- */
                mode = 0;
                for(i=0; i < dnp->locPtNum; i++)
                {
                    locPoint = (LOCK_POINT_BUF *) &dnp->locPoint[i];
                    
                    if(locPoint->useFlag == 0)          continue;
                    if(dnp->cntTcf == CLOSE_CONTROL)    continue;       // 개별포인트 쇄정(CLOSE)... 무시 
                        
                    /* 계전기 쇄정포인트 ... 제어상태 지정 */
                    locPoint->cntrState = dnp->cntTcf;  
                    locPoint->cntrReq   = SET; 

                    /* 전체 해정(TRIP) 제어가 있는경우.... SET */                    
                    mode = SET;
                    
                    if(opr->soeDebug) 
                    Debug(console,"--> mplc> Device(%02d) 개별 쇄정/해정...point=%d ...SET  TCF[해정=0x80, 쇄정=0x40]= %2x\n", devid+1, locPoint->devPt, locPoint->cntrState);
                }
                
                /* --------------------------------------- */
                /* 전체 해정(TRIP) 제어가 있는경우.... SET */ 
                /* --------------------------------------- */
                if(mode == SET) dnp->lock_allControl = SET;
            }     
            else if(ptBuf->ptType == LOCKPT)    // 개별 쇄정/해정 포인트
            {
                for(i=0; i <dnp->locPtNum; i++)
                {
                    locPoint = (LOCK_POINT_BUF *) &dnp->locPoint[i];
                    if(locPoint->useFlag == 0)  continue;
                    
                    /* ---------------------------------- */
                    /* 개별 쇄정포인트 ... 제어상태 Clear */
                    /* ---------------------------------- */
                    if(locPoint->devPt == (dnp->cntPoint + 1))
                    {       
                        locPoint->cntrState = 0;  
                        locPoint->cntrReq   = RESET;  
                        if(opr->soeDebug) 
                        Debug(console,"--> mplc> Device(%02d) 개별 쇄정/해정...point=%d ... RESET [base=%04x (%04d)]\n", devid+1, dnp->cntPoint+1, cntrBase, cntrBase);
                    }
                }
            }
                
            dnp->cntPoint = 0;
            dnp->cntTcf   = 0;
        }
        dnp->faultResetReq = RESET;        
        break;    

    default:
        if(opr->msgDebug == devid)
        Debug(console,"dev%2d> *mplc : invalid Opcode = %2x\n", devid+1, opcode);
        return (0);
        break;        
    }
    
    dnp->mplcRcvOK = SET; 
    
    return (1);
    
}



/*
*	MPLC 명령어 처리...
*/
int makeCmd_MPLC( int mode, int scanid, int devid, byte *sndbuf)
{
	int		i;
	int		readCount, readIndex;
    int 	count=0;
    int 	opcode;
    int 	dataNum=0;
    int     blockBase=0, blockOffset=0;
    word    cntrBase=0, cntrValue=0;
    //int     index1, index2;
    //int		diWordNum;
    int 	point, cntrTCF;
    //int     length;
    //int     year;
    //word    crc;
    
    DNP_DEVICE      *dnp;
    SCAN_CONFIG     *scan;
    POINT_BUF       *ptBuf;
    MODBUS_READ	    *readBlock=NULL;
    MODBUS_WRITE    *writeBlock=NULL;
    LOCK_POINT_BUF  *locPoint;

    //LOCK_POINT_BUF  *locPoint;
    
    dnp = (DNP_DEVICE *) dnpcfg[devid];    

    /* SCAN-TASK 별 모듈구성 정보 */ 
    scan = (SCAN_CONFIG *) scanCfg[scanid];
    
    /* ------------------------------------------------ */
    /*   계전기 OFFLINE : 주기적으로 ONLINE Check ..    */
    /* ------------------------------------------------ */
    if(scan->changeMode == 0)
    {    
        if(dnp->online == 0)
        {
            if(dnp->timeOutTick++ > 10)     dnp->timeOutTick = 0;
            else                            return (0);
        }
    }

    /* ------------------------------------------------ */
    /*   계전기 제어후 ... CLEAR 실행 Check ..          */
    /* ------------------------------------------------ */
    if(dnp->clearReq == SET)      
    {
        if(dnp->clearTick++ > 5)
        {
            dnp->operateReq = SET;
            dnp->clearTick  = 0;
        }      
    }
                
    if(dnp->selectReq == SET)      
    {
        point   = dnp->cntPoint;
        cntrTCF = dnp->cntTcf;
        ptBuf = (POINT_BUF *) &dnp->doPtBuf[point];
        
        /* -------------------------------------------- */
    	/* 제어포인트별 정의된 Write 블록 찾음...       */
    	/* -------------------------------------------- */
    	writeBlock = (MODBUS_WRITE *) &dnp->writeBlock[ptBuf->writeBlockIndex];
    	opcode = MPLC_WR_WORD;      // word read.... 0x04
    }
    else if(dnp->operateReq == SET)     
    {
        point   = dnp->cntPoint;
        cntrTCF = dnp->cntTcf;
        ptBuf = (POINT_BUF *) &dnp->doPtBuf[point];
        
        /* -------------------------------------------- */
    	/* 제어포인트별 정의된 Write 블록 찾음...       */
    	/* -------------------------------------------- */
    	writeBlock = (MODBUS_WRITE *) &dnp->writeBlock[ptBuf->writeBlockIndex];
    	opcode = MPLC_WR_WORD;      // word read.... 0x04                                                          opcode = MODBUS_WR_SINGLE_COIL;         // else 0x05
    }
    else                                
    {
    	/* ------------------------------------------------ */
    	/*	계전기 READ-BLOCK 내 적용 Opcode  지정 		    */
    	/* ------------------------------------------------ */
    	readCount = 0;
    	dataNum   = 0;
    	for(;;)
    	{
    	    /* -------------------------------------------- */
    		/* 계전기 통신상태: 정의된 READ 블록 찾음...    */
    		/* -------------------------------------------- */
    		if(dnp->mplcRcvOK == SET)   // 정상통신후 index 증가
    		{     
    	        dnp->readIndex = (dnp->readIndex + 1) & 0x07;
    	    }
    	    
    		readIndex = dnp->readIndex;
    		
    		/* -------------------------------------------- */
    		/* 각 블록의 정의된 READ 블록 찾음...           */
    		/* -------------------------------------------- */
    		readBlock = (MODBUS_READ *) &dnp->readBlock[readIndex];
    		
    		if(readBlock->useFlag == 0)	    /* readBlock 이 정의되지 않은 경우 ... */
    		{
    			/* default READ-BLOCK */
    			if(++readCount > 16)
    			{
    				dnp->readIndex = 0;
    				
        			if((opr->dnpDebug == devid) || (opr->scanDebug == scanid))
        			Debug(console,"MPLC> dev=%2d...*Default read ...return  \n", devid+1);
        			
        			return (0);	
    			}	
    			else	continue;
    		}
    		
    		/* -------------------------------------------- */
    		/*	해당 READ-BLOCK 내의 OPCODE 적용			*/
    		/* -------------------------------------------- */
    		opcode = MPLC_RD_WORD;      // word read.... 0x01
    		
    		/* Base,Offset  Address 지정 */
    		blockBase   = readBlock->baseAddr;
    		blockOffset = readBlock->baseOffset;

            /* Dump WORD 지정 */
            dataNum = readBlock->dataNum;
   			if(dataNum == 0)    dataNum = 4;        // Default Word
   			    
   	        break;		    
    		
    	} /* for */
    	
    }
    
    /* ---------------------------------------- */
    /*  make MODBUS Packet(TCPIP 또는 RS232)    */
    /* ---------------------------------------- */
    sndbuf[0] = opcode;             // OPCODE
    sndbuf[1] = 0xff;               // PLC No
    sndbuf[2] = 0x0a;               // ACPU Timeout (L)
    sndbuf[3] = 0x00;               // ACPU Timeout (H)
    count = 4;


	/* ---------------------------------------------------- */
	/*	지멘스 계전기의 경우... 이중화 상태/Flag 표기 		*/
	/* ---------------------------------------------------- */
    if((opr->dnpDebug == devid) || (opr->scanDebug == scanid))
    {
   		Debug(console,"\n --------------------------------[%02d:%02d:%02d]\n", rtc->hour, rtc->min, rtc->sec); 		
    }
        
    switch(opcode)
    {
    case MPLC_RD_WORD:       				   
        /* ---------------------------- */ 
        /* Device Number .. LSB First */
        /* ---------------------------- */
        sndbuf[count++] = blockBase & 0xff;
        sndbuf[count++] = (blockBase >> 8) & 0xff;
        sndbuf[count++] = (blockBase >> 16) & 0xff;
        sndbuf[count++] = (blockBase >> 24) & 0xff;
        
        /* ---------------------------- */
        /* Device Code                  */
        /* ---------------------------- */
        sndbuf[count++] = 0x20;
        if(readBlock->opcode == MPLC_X)          
        {
            sndbuf[count++] = 0x58;     // X-index    
            if(opr->msgDebug == devid)  
            Debug(console,"dev%2d> mplc Read(%d) X-xxx [opcode=%3d, Base = %3d (%02x), word-point=%2d] \n", devid+1, dnp->readIndex,
                opcode, blockBase, blockBase, readBlock->dataNum );
        }
        else if(readBlock->opcode == MPLC_M)     
        {
            sndbuf[count++] = 0x4D;     // M-index
            if(opr->msgDebug == devid)  
            Debug(console,"dev%2d> mplc Read(%d) M-xxx [opcode=%3d, Base = %3d (%02x), word-point=%2d] \n", devid+1, dnp->readIndex,
                opcode, blockBase, blockBase, readBlock->dataNum );
        }

        /* ---------------------------- */
        /* Data Number */
        /* ---------------------------- */
        sndbuf[count++] = dataNum;  // read WORD 수
        sndbuf[count++] = 0x00;
        
        /* 예상 응답 Size */
        dnp->mplcRcvSize = dataNum * 2 + 2;    
        break;

    case MPLC_WR_WORD:
        sndbuf[count++] = 0x01;     // Number of Device Point
        sndbuf[count++] = 0x00;
        
        /* -------------------------------------------- */
        /* 제어포인트의 Control-Base Check...           */
        /* -------------------------------------------- */
        point   = dnp->cntPoint;
        cntrTCF = dnp->cntTcf;
        ptBuf = (POINT_BUF *) &dnp->doPtBuf[point];

        /* -------------------------------------------- */
    	/* 제어포인트별 정의된 Write 블록 찾음...       */
    	/* -------------------------------------------- */
    	writeBlock = (MODBUS_WRITE *) &dnp->writeBlock[ptBuf->writeBlockIndex];
    	cntrBase = ptBuf->modIndex;         // M-xxx 제어포인트 인덱스 (INDEX Address)
    	
        /* -------------------------------------------- */
        /* 개별 쇄정/해정 포인트...쇄정(CLOSE)제어 무시 */
        /* -------------------------------------------- */
        if(ptBuf->ptType == LOCKPT)
        {
            if(cntrTCF ==  CLOSE_CONTROL)
            {
                for(i=0; i < dnp->locPtNum; i++)
                {
                    locPoint = (LOCK_POINT_BUF *) &dnp->locPoint[i];
                    if(locPoint->devPt == (dnp->cntPoint + 1))
                    {
                        locPoint->cntrReq = 0;
                        locPoint->cntrState = 0;
                        if(opr->soeDebug)
                        Debug(console,"dev%2d> *개별 쇄정/해정 .... 쇄정(CLOSE) 제어 무시... Point= %d [base=%04x (%04d)]\n", devid+1, dnp->cntPoint+1, cntrBase, cntrBase);
                        break;
                    }      
                }
                
                dnp->selectReq  = 0;
                dnp->operateReq = 0;
                dnp->clearReq   = 0;
                dnp->cntPoint   = 0;
                dnp->cntTcf     = 0;
                
                return (0);
            }
        }
                
        /* ------------------------------------------------ */
        /*  포인트 제어속성별 처리                          */
        /* ------------------------------------------------ */
        /* -------------------------------------------------------------------------- */
        /* ptConfig == 4 : 미쯔비시 : 동일 Addr 인터락 제어 수행  */  
        /* ptConfig == 5 : 미쯔비시 : 동일 Addr TRIP(2)/CLOSE(1) 제어후 RESET 수행  */  
        /* -------------------------------------------------------------------------- */  
        if (ptBuf->ptConfig == MPLC_INTERLOCK)
        {
        	if(dnp->selectReq)
            {       
              	cntrBase = ptBuf->modIndex;         // M-xxx 제어포인트 인덱스 (INDEX Address)
                cntrValue = cntrTCF;
                    
                if(opr->soeDebug)
                Debug(console,"snd%2d> Write-SINGLE(op=%02d) (cfg=%d) : Point= %d, INTER-LOCK(Select) ... [base=%04x/%04d, value=%04x]\n", devid+1, opcode, ptBuf->ptConfig, dnp->cntPoint+1, cntrBase, cntrBase, cntrValue);
                
#ifdef	MPLC_TEST
                dnp->selectReq = 0;			// 강제종료 
                dnp->clearReq  = SET;
                dnp->clearTick = 0;
#endif                
            }
            else if(dnp->operateReq)
            {       
              	cntrBase = ptBuf->modIndex;         // M-xxx 제어포인트 인덱스 (INDEX Address)
                cntrValue = cntrTCF;
                    
                if(opr->soeDebug)
                Debug(console,"snd%2d> Write-SINGLE(op=%02d) (cfg=%d) : Point= %d, INTER-LOCK(Operate) ... [base=%04x/%04d, value=%04x]\n", devid+1, opcode, ptBuf->ptConfig, dnp->cntPoint+1, cntrBase, cntrBase, cntrValue);

#ifdef	MPLC_TEST                
                dnp->operateReq = 0;		// 강제종료 
                dnp->clearReq   = RESET;
                dnp->clearTick = 0;
#endif                
            }
    	}	
        else if (ptBuf->ptConfig == MODBUS_TC_PLUSE)
        {
            if(dnp->selectReq)
            {       
                if(cntrTCF == TRIP_CONTROL)
                {
                    cntrBase = ptBuf->modIndex;         // M-xxx 제어포인트 인덱스 (INDEX Address)
                    cntrValue = 0x00;
                    
                    if(opr->soeDebug)
                    Debug(console,"snd%2d> Write-SINGLE(op=%02d) (cfg=%d) : Point= %d, TRIP-SELECT.. SET  [base=%04x, value=%04x]\n", devid+1, opcode, ptBuf->ptConfig, dnp->cntPoint+1, cntrBase, cntrValue);
                }
                else if(cntrTCF == CLOSE_CONTROL)
                {
                	cntrBase = ptBuf->modIndex;         // M-xxx 제어포인트 인덱스 (INDEX Address)
                    cntrValue = 0x01;
                    
                    if(opr->soeDebug)
                    Debug(console,"snd%2d> Write-SINGLE(op=%02d) (cfg=%d) : Point= %d, CLOSE-SELECT... SET  [base=%04x, value=%04x]\n", devid+1, opcode, ptBuf->ptConfig, dnp->cntPoint+1, cntrBase, cntrValue);
                }
            }
            else if(dnp->operateReq)          
            {
                if(cntrTCF == TRIP_CONTROL)
                {
                    cntrBase = ptBuf->modIndex;         // M-xxx 제어포인트 인덱스 (INDEX Address)
                    cntrValue = 0x00;
                    
                    if(opr->soeDebug)
                    Debug(console,"snd%2d> Write-SINGLE(op=%02d) (cfg=%d) : Point= %d, TRIP-OPERATE... SET  [base=%04x, value=%04x]\n", devid+1, opcode, ptBuf->ptConfig, dnp->cntPoint+1, cntrBase, cntrValue);
                }
                else if(cntrTCF == CLOSE_CONTROL)
                {
                	cntrBase = ptBuf->modIndex;         // M-xxx 제어포인트 인덱스 (INDEX Address)
                    cntrValue = 0x01;
                    
                    if(opr->soeDebug)
                    Debug(console,"snd%2d> Write-SINGLE(op=%02d) (cfg=%d) : Point= %d, CLOSE-OPERATE... SET  [base=%04x, value=%04x]\n", devid+1, opcode, ptBuf->ptConfig, dnp->cntPoint+1, cntrBase, cntrValue);
                }
            }
        }
        else if(ptBuf->ptConfig == MODBUS_TRIP_PLUSE)        // TRIP 제어 선행...
        {       
            if(dnp->selectReq == SET)
            {
                if(cntrTCF == TRIP_CONTROL)
                {
                    cntrBase = ptBuf->modIndex;         // M-xxx 제어포인트 인덱스 (INDEX Address)
                    cntrValue= 0x01;
                    
                    if(opr->soeDebug)
                    Debug(console,"dev%2d> TRIP-FIRST (cfg=%d) : Point= %d, TRIP-PULSE... SET  [base=%04x (%04d), value=%04x]\n", devid+1, ptBuf->ptConfig, dnp->cntPoint+1, cntrBase, cntrBase, cntrValue);
                }
                else if(cntrTCF == CLOSE_CONTROL)
                {
                    cntrBase = ptBuf->modIndex + 1;     // M-xxx 제어포인트 인덱스 (INDEX Address + 1)
                    cntrValue= 0x01;
                       
                    if(opr->soeDebug)
                    Debug(console,"dev%2d> TRIP-FIRST (cfg=%d) : Point= %d, CLOSE-PULSE... SET  [base=%04x (%04d), value=%04x]\n", devid+1, ptBuf->ptConfig, dnp->cntPoint+1, cntrBase, cntrBase, cntrValue);
                }
            }
            else if(dnp->operateReq == SET)
            {
                if(cntrTCF == TRIP_CONTROL)
                {
                    cntrBase = ptBuf->modIndex;        // M-xxx 제어포인트 인덱스
                    cntrValue= 0x00;
                       
                    if(opr->soeDebug)
                    Debug(console,"dev%2d> TRIP-FIRST (cfg=%d) : Point= %d, TRIP-PULSE... *RESET  [base=%04x (%04d), value=%04x]\n", devid+1, ptBuf->ptConfig, dnp->cntPoint+1, cntrBase, cntrBase, cntrValue);
                }
                else if(cntrTCF == CLOSE_CONTROL)
                {
                    cntrBase = ptBuf->modIndex + 1;        // M-xxx 제어포인트 인덱스
                    cntrValue= 0x00;
                       
                    if(opr->soeDebug)
                    Debug(console,"dev%2d> TRIP-FIRST (cfg=%d) : Point= %d, CLOSE-PULSE... *RESET  [base=%04x (%04d), value=%04x]\n", devid+1, ptBuf->ptConfig, dnp->cntPoint+1, cntrBase, cntrBase, cntrValue);
                }
            }
        }
        /* ------------------------------------------------ */
        /*  DEFAULT 제어방식 : CLOSE 제어 선행...           */
        /* ------------------------------------------------ */
        else           
        {       
            if(dnp->selectReq == SET)
            {
                if(cntrTCF == CLOSE_CONTROL)
                {
                    cntrBase = ptBuf->modIndex;         // M-xxx 제어포인트 인덱스 (INDEX Address)
                    cntrValue= 0x01;
                    
                    if(opr->soeDebug)
                    Debug(console,"dev%2d> CLOSE-FIRST (cfg=%d) : Point= %d, CLOSE-PULSE... SET  [base=%04x (%04d), value=%04x]\n", devid+1, ptBuf->ptConfig, dnp->cntPoint+1, cntrBase, cntrBase, cntrValue);
                }
                else if(cntrTCF == TRIP_CONTROL)
                {
                    cntrBase = ptBuf->modIndex + 1;     // M-xxx 제어포인트 인덱스 (INDEX Address + 1)
                    cntrValue= 0x01;
                       
                    if(opr->soeDebug)
                    Debug(console,"dev%2d> CLOSE-FIRST (cfg=%d) : Point= %d, TRIP-PULSE... SET  [base=%04x (%04d), value=%04x]\n", devid+1, ptBuf->ptConfig, dnp->cntPoint+1, cntrBase, cntrBase, cntrValue);
                }
            }
            else if(dnp->operateReq == SET)
            {
                if(cntrTCF == CLOSE_CONTROL)
                {
                    cntrBase = ptBuf->modIndex;        // M-xxx 제어포인트 인덱스
                    cntrValue= 0x00;
                       
                    if(opr->soeDebug)
                    Debug(console,"dev%2d> CLOSE-FIRST (cfg=%d) : Point= %d, CLOSE-PULSE... *RESET  [base=%04x (%04d), value=%04x]\n", devid+1, ptBuf->ptConfig, dnp->cntPoint+1, cntrBase, cntrBase, cntrValue);
                }
                else if(cntrTCF == TRIP_CONTROL)
                {
                    cntrBase = ptBuf->modIndex + 1;        // M-xxx 제어포인트 인덱스
                    cntrValue= 0x00;
                       
                    if(opr->soeDebug)
                    Debug(console,"dev%2d> CLOSE-FIRST (cfg=%d) : Point= %d, TRIP-PULSE... *RESET  [base=%04x (%04d), value=%04x]\n", devid+1, ptBuf->ptConfig, dnp->cntPoint+1, cntrBase, cntrBase, cntrValue);
                }
            }
        }

        /* ---------------------------------- */            
        /* Device Number .. LSB First */
        /* ---------------------------------- */    
        sndbuf[count++] = cntrBase & 0xff;
        sndbuf[count++] = (cntrBase >> 8) & 0xff;
        sndbuf[count++] = (cntrBase >> 16) & 0xff;
        sndbuf[count++] = (cntrBase >> 24) & 0xff;
            
        sndbuf[count++] = 0x20;
        sndbuf[count++] = 0x4D;     // M-index
        sndbuf[count++] = cntrValue;     // ON 제어
        
        /* 예상 응답 Size */
        dnp->mplcRcvSize = 2;    
        break;
                
    default:
        Debug(console,"mplc-cmd> dev=%2d...* Defaut Opcode(%2d)... return !\n", devid+1, opcode);
        
        /* 예상 응답 Size */
        dnp->mplcRcvSize = 0;   
        return (0);
        break;        
    }
    
    /* 계전기 통신상태 : Clear... */
    dnp->mplcRcvOK  = RESET;
    dnp->mplcOpcode = opcode;
    
    
    return (count);

}



/*****************************************
* handle response 
* 
* return :  1   get vaild response  
*  			0   TCP Connection closed
*           -1  Timeout
*           -2  select system error 
******************************************/
int  rcvWithMplc_TCPIP( int scanid, int devid, byte *rxbuf)
{
    int     result;
    int     reqSize;
    int     count;

    DNP_DEVICE      *dnp;

    /* --------------------------------------- */
    /* CLIENT 프로세서에 해당하는 구조체 ... */
    /* --------------------------------------- */
    dnp = (DNP_DEVICE *) dnpcfg[devid];
    reqSize = dnp->mplcRcvSize;
    
    if(dnp->connectStatus == RESET)
    {
        return (-2);
    }
        
    count = 0;

    /* ---------------------------------------- */
    /*  READ MODBUS - Header                    */
    /* ---------------------------------------- */ 
	if((result =TKreadn(dnp->SocketFd, (char *) rxbuf, reqSize, 1000)) < 0)
    {
        /*  such as timeout */
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

    return (result);
                                                                                                              
}

/* 초기 인터락 Check */
int	check_InterLock(DNP_DEVICE *dev)
{
	int			i;
	byte		front;
	POINT_BUF	*diPoint;
	DNP_DEVICE	*interDNP;
	
	/* ------------------------------------ */
    /* 초기 상태덤프 후 ... 이벤트 발생     */
    /* ------------------------------------ */
    if(dev->initial < STATUS_DUMP_TICK)   return (0);
    	
	/* -------------------------------- */
    /* Device 현재상태정보 Update ...   */
    /* -------------------------------- */
    for(i=0; i < dev->devDiPoint; i++)
   	{ 
    	diPoint = (POINT_BUF *) &dev->diPtBuf[i];
    	
    	/* ---------------------------------------- */
    	/* 미정의 포인트의 경우... Return           */
    	/* ---------------------------------------- */
    	if(diPoint->config == RESET)    return 0;
    	
    	if(diPoint->interLockPoint == SET)
		{
			if(opr->soeDebug)
			Debug(console, "INIT> Interlock Point ... DI [dev=%2d/pt=%2d] => DO [dev=%2d/pt=%2d] ... state = %d \n", dev->ioid, i+1, diPoint->lock_Dev, diPoint->lock_Point, diPoint->status );
			
			interDNP = (DNP_DEVICE *) dnpcfg[diPoint->lock_Dev-1];
			
			front = interDNP->interLockFront & 0x07;
			
			interDNP->interInfo[front][0] = diPoint->lock_Point;			// 제어 포인트
			interDNP->interInfo[front][1] = diPoint->status;				// 제어 상태
			
			interDNP->interLockFront = (interDNP->interLockFront + 1) & 0x07;
		}
    }


	return 0;
	
}


/* -------------------------------------------------------- */
/*  SCAN THREAD Start Routine ....                          */
/* -------------------------------------------------------- */
void    *scan_tcpip_mplc(int  *arg)
{
    int     i;
    int     scanid;
    //int		comPort;
    //int     activeChan;
    int     devid;
    int     chanComFail;
    int     chanReset;
    int     txcnt;
    int     rxcnt, rxVal;
    int     opcode, endCode;
    int     reConnectCount;
    int     reqTimeTick;
    int     oldsec=0;
    int     taskDelay;
    int     lockCntrMode;
    int     offlineTick;
    int     errCount;
    int     timeout;
    int		netMasterSET;
    //byte    rcvAddr, opcode, expCode;
    byte    *rxbuf, *txbuf;
    byte	front, IL_point, IL_state;
    
    char    buffer[128];
    
    DNP_DEVICE      *dnp;
    SCAN_CONFIG     *scan;
    LOCK_POINT_BUF  *locPoint;
	
	/* ------------------------------------ */
	/* THREAD Argment Assign ...            */
	/* ------------------------------------ */
	devid = (int ) *arg;
	
	/* ------------------------------------ */
    /* SCAN-TASK 별 모듈구성 정보           */ 
    /* ------------------------------------ */
    dnp = (DNP_DEVICE *) dnpcfg[devid];
    dnp->activeTick = 0;
    
    scanid = dnp->scanPort;
    scan = (SCAN_CONFIG *) scanCfg[scanid];
    
    taskDelay = scan->comDelay * 10;
    if(taskDelay < 10)		taskDelay = 10;
    if(taskDelay > 2000)	taskDelay = 2000;	
        
    printf(" >> TCPIP-미쯔비시 (scan=%d/dev=%02d) Thread Activated (Delay = %d ms, IP = %s / port=%d )...\n", scanid+1, devid+1, taskDelay, dnp->ipString, dnp->netPort);
    
    txbuf = (byte *) &dnp->sndBuffer[0];
    rxbuf = (byte *) &dnp->rcvBuffer[0];
    
    dnp->activeChan     = 0; 
    
    dnp->modbusRcvFail  = 0;	
    dnp->reqDataCount   = 0;
    dnp->reqDataOpcode  = 0;
    
    dnp->interLockCheck = 0;	// 초기 인터락 check 상태
    
    offlineTick = 0;

    /* ------------------------------------ */
    /*  장치별 TCPIP-CLIENT initial...      */
    /* ------------------------------------ */
   	if (_initNetClientSCAN(scanid, devid, dnp->activeChan) == ERROR)
    { 
        if((opr->dnpDebug == devid) || (opr->scanDebug == scanid))
        printf("TCPIP[%02d]> *** Network Initial... Fail...\n", devid+1);	
        ThreadActive[devid] = NO;
    }
    
    chanComFail = 0;
    chanReset   = 0;
    errCount    = 0;
    reConnectCount = 0;
    
    if(opr->runMode == LOCAL_SLAVE)		netMasterSET = 0;
    else								netMasterSET = SET;	
    
    while(ThreadActive[devid])
	{
	    pause(taskDelay);

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
                Debug(console,"dev%02d> *CPU Mode Change => SLAVE ... Socket Close(%d) ...!\n", devid+1, dnp->SocketFd);
                close( dnp->SocketFd);  
            }
                  
			/* ---------------------------------------- */
			/*	SLAVE 모드에서 수신된 데이터를 처리함   */
			/* ---------------------------------------- */
			dnp->connectStatus = RESET;
			dnp->connectFail = 0;
			netMasterSET = 0;
			reConnectCount   = 3;   // 재접속시간을 최소화...
			pause(500);
			continue;
		}
		else
        {
            if(netMasterSET == 0)
            {
                Debug(console,"dev%02d> *CPU Mode Change => MASTER ... !\n", devid+1);
                opr->scanRestart = SET;
                pause(100);
            }
                    
            netMasterSET = SET;
        }		     

//**********************************
//	시험용
//**********************************
#ifdef	MPLC_TEST
		dnp->connectStatus = SET;
#endif
		
        /* ------------------------------------- */
	    /*  접속실패에 따른 재접속...            */
	    /* ------------------------------------- */
	    if(dnp->connectStatus == RESET)
	    {
            if(++dnp->connectFail > scan->offCount)
            {  
                dnp->connectFail = 0;
                //close( plc->SocketFd);  

                if((opr->dnpDebug == devid) || (opr->scanDebug == scanid))    Debug(console,"dev%02d> * retry Connect....!\n", devid+1);
                
                _reConnectClientPLC(scanid, devid, dnp->activeChan);  
                              
                if(++reConnectCount > 3)
                {
                    reConnectCount = 0;
                    close( dnp->SocketFd);  

                    if((opr->dnpDebug == devid) || (opr->scanDebug == scanid))
                    Debug(console,"dev%02d> * RE Initial...CLIENT....(%2d)!\n", devid+1, dnp->comFailTick);

                    dnp->activeTick = 0;
                    
                    /* ---------------------------------------- */
                    /* 계전기 이중화에 따른 IP 구성정보 변경    */   
                    /* ---------------------------------------- */                 
                    if((dnp->dualIED) && (dnp->type == TYPE_MODBUS_SZ))
                    {
                        dnp->activeChan = (dnp->activeChan + 1) & 0x01;
                        
                        if(++offlineTick > 4)
                        {
                            offlineTick = 0;
                            /* ------------------------------------------------ */
                            /*  장치별 Offline 판정...                          */
                            /* ------------------------------------------------ */
            	            if(dnp->online != RESET)  
                            {
                                if((opr->dnpDebug == devid) || (opr->scanDebug == scanid))
                                Debug(console,"tcpip> dev(%02d)  *dual...offline...!\n", devid + 1);
                                setOffline(devid);
                                dnp->online = RESET;
                            }
                        }    
                    }
                    else 
                    { 
                        dnp->activeChan = 0;
                        
                        /* ------------------------------------------------ */
                        /*  장치별 Offline 판정...                          */
                        /* ------------------------------------------------ */
        	            if(dnp->online != RESET)  
                        {
                            if((opr->dnpDebug == devid) || (opr->scanDebug == scanid))
                            Debug(console,"tcpip> dev(%02d)  *offline...!\n", devid + 1);
                            setOffline(devid);
                            dnp->online = RESET;
                        }
                    }     
                    
                    /* ---------------------------------------- */
                    /*  장치별 TCPIP 초기화...                  */
                    /* ---------------------------------------- */
                    if(_initNetClientSCAN(scanid, devid, dnp->activeChan) == ERROR)
                    { 
                        if((opr->dnpDebug == devid) || (opr->scanDebug == scanid))
                        Debug(console,"dev%02d> *** Target No Response Re-Initial... Fail...\n" , devid+1);	
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
                                sprintf(buffer, "[MPLC-TCPIP] 전체장치 통신절체...Device=%d, changeTick=%d ", devid+1, opr->cpuChangeTick);
                                LogFile_MPU (opr, EVENT_CHANGE_CPU, buffer, strlen(buffer));  
                        
                                pause(1000);
                        
                                opr->cpuChangeTick++;
                        
                                if(opr->wdtDebug)
                                {   
                                    Debug(console,"\n--------------------------------------------\n");
                                    Debug(console,"scan %2d> %02d:%02d:%02d *** TCPIP-MPLC Device scan Fail... CPU Change (%d)...!\n", scanid+1, rtc->hour, rtc->min, rtc->sec, opr->cpuChangeTick);
                                    Debug(console,"--------------------------------------------\n");
                                }
                        
                                /* OFFLINE 이벤트 발생... */
                                dnp->online = 2;
                            }
                        
                        }
                        
                        dnp->connectStatus = RESET;
                    }  
                }
	        }
	        
	        continue;
	    }

        opr->devScanCount[scanid]++;
        
        /* -------------------------------------------- */
        /* 계전기 쇄정/해정 제어...                     */
        /* - 전체 쇄정/해정 제어시...개별포인트 제어    */
        /* -------------------------------------------- */
        if(dnp->lock_allControl == SET)
        {
            lockCntrMode = RESET;
            
            for(i=0; i < dnp->locPtNum; i++)
            {
                locPoint = (LOCK_POINT_BUF *) &dnp->locPoint[i];
                
                if(locPoint->useFlag == 0)  continue;
                if(locPoint->cntrReq == 0)  continue;                    
               
                /* ---------------------------------------- */
                /* 초기 제어정보 설정...                    */
                if((dnp->selectReq + dnp->operateReq + dnp->clearReq) == 0)
                {
                    dnp->cntPoint = locPoint->devPt - 1;    // 제어포인트   지정
                    dnp->cntTcf   = locPoint->cntrState;     // 제어상태     지정
                    
                    if(opr->soeDebug)
                    {      
                        Debug(console, "\n======================================================\n");  
                        Debug(console, "***> Device(%2d) LOCK Point (%2d)... cntr=%02x  ... SET \n",  devid+1, dnp->cntPoint+1, dnp->cntTcf );
                        Debug(console, "======================================================\n"); 
                    }
                                
                    lockCntrMode   = SET;
                    dnp->selectReq = SET;
                    
                    break;
                }
                else
                {
                    lockCntrMode = SET;
                }                
            }    
            
            if(lockCntrMode == RESET)    
            {
                dnp->lock_allControl = RESET;  
                if(opr->soeDebug)
                {                      
                    Debug(console, "\n======================================================\n");     
                    Debug(console, "***> Device(%2d) ALL-LOCK Point Control... END \n",  devid+1 );
                    Debug(console, "======================================================\n");  
                }
            }
        }
        
        /* -------------------------------------------- */
        /* 계전기 인터락  제어...                     */
        /* -------------------------------------------- */
		if(dnp->interLockFront)
		{
			front = (dnp->interLockFront - 1) & 0x07;
			IL_point = dnp->interInfo[front][0];
			IL_state = dnp->interInfo[front][1];
			dnp->interLockFront = front;
			
			dnp->cntPoint = IL_point - 1;    // 제어포인트   지정
            dnp->cntTcf   = IL_state;    	 // 제어상태     지정
			dnp->selectReq = SET;

			if(opr->soeDebug)
			Debug(console, "LOCK-CNTR> Device(%2d) LOCK Point (%2d)... cntr=%02x  ... SET \n",  devid+1, IL_point, IL_state );
		}
					        
        /* ---------------------------------------- */
        /* MODBUS : 보호계전기 송수신 루틴...       */
        /* ---------------------------------------- */
        if((txcnt = makeCmd_MPLC( COM_TCPIP, scanid, devid, txbuf)) > 0)
        {
            if((opr->dnpDebug == devid) || (opr->scanDebug == scanid))   DumpBuff(console,"TXD: ", txbuf, txcnt);
            
            dnp->comSndCount++;

#ifndef	MPLC_TEST               
            /* -------------------------------- */
		    /* Network Response...              */
    		/* -------------------------------- */    
    		if(dnp->connectStatus == SET)
		    {
		        if((rxVal = tkWriteTCP( dnp->SocketFd , txbuf, txcnt, 1000)) < 0 )
    			{
            		if((opr->dnpDebug == devid) || (opr->scanDebug == scanid))
            		Debug(console, "dev%02d> [MODBUS-TCP] **** net Send Error [%d / %d]...socket = %d !\n", devid+1, txcnt,  rxVal, dnp->SocketFd);
            		/* -------------------------------------------- */
                    /* LOG File 저장                                */
                    /* -------------------------------------------- */
                    sprintf(buffer, "dev%02d> [MODBUS-TCP] **** net Send Error [%d / %d]...socket = %d !", devid+1, txcnt,  rxVal, dnp->SocketFd);
                    LogFile_MPU ( opr, NOT_DEFINED, buffer, strlen(buffer));
                    pause(1000);
                    
			    	dnp->connectStatus = RESET;
			    }
            
                /* -------------------------------- */
                /* 접속이상에 따른 송신실패시...    */
                /* -------------------------------- */
                if( rxVal != txcnt)
                {
                    if((opr->dnpDebug == devid) || (opr->scanDebug == scanid))
                    Debug(console,"dev%02d> [MODBUS-TCP] * net Send Size Error [%d %d]...!\n", devid+1, txcnt,  rxVal);
                    
                    /* -------------------------------------------- */
                    /* LOG File 저장                                */
                    /* -------------------------------------------- */
                    sprintf(buffer, "dev%02d> [MODBUS-TCP] *net Send Size Error [%d %d]...!", devid+1, txcnt,  rxVal);
                    LogFile_MPU ( opr, NOT_DEFINED, buffer, strlen(buffer));
                    pause(1000);
                                
                    dnp->connectStatus = RESET;
                }
            }         
#endif

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
            for(timeout = 0; timeout < reqTimeTick; timeout++)
            {
                usleep(1000);            
                if((rxcnt = rcvWithMplc_TCPIP( scanid, devid, rxbuf) ) > 0)
                {
                    /* ---------------------------------------- */
                    /*  수신 패킷에 대한 오류 Check...          */
                    /* ---------------------------------------- */
                    opcode  = rxbuf[0];
                    if((opcode & 0x80) == 0)
                    {
                        Debug(console,"mplc-rcv> * Invalid OPCODE = %02x\n", opcode);
                        break;
                    }
                    
                    endCode = rxbuf[1];
                    if(endCode != 0)
                    {
                        Debug(console,"mplc-rcv> * Invalid END-CODE = %02x\n", endCode);
                        break;
                    }
                          
                    	
                    if((opr->dnpDebug == devid) || (opr->scanDebug == scanid))	DumpBuff(console,"RXD: ", rxbuf, rxcnt);

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

                    /* ------------------------------------------------ */                
                    /* 수신부 데이터 처리  */
                    /* ------------------------------------------------ */
                    rcvHandler_MPLC(scanid, devid, &rxbuf[0], rxcnt);
            
                    break;
                }
                else
                {
#ifndef	MPLC_TEST                	
            	    if(rxcnt == 0)
        	        {
        	            if((opr->dnpDebug == devid) || (opr->scanDebug == scanid))
	                    Debug(console,"dev%02d> [MPLC-TCP] *** TCP/IP read ...0 Connection...Failed...!\n", devid+1);
            	        dnp->connectStatus = RESET;
                        break;        
	                }
	                else if(rxcnt == -1)
        	        {
        	            if((opr->dnpDebug == devid) || (opr->scanDebug == scanid))
	                    Debug(console,"dev%02d> [MPLC-TCP] *** TCP/IP read ...Packet Error..%d!\n", devid+1, errCount);
	                    
	                    if(++errCount > 5)
	                    {   
	                        dnp->connectStatus = RESET;
	                        errCount = 0;
                            break;        
                        }
	                }
	                else if(rxcnt == -2)
       	            {
        	            if((opr->dnpDebug == devid) || (opr->scanDebug == scanid))
	                    Debug(console,"dev%02d> [MPLC-TCP] *** TCP/IP un Connected...Failed...!\n", devid+1);
            	        dnp->connectStatus = RESET;
                        break;        
	                }
	                
	                break;
#endif	     	                
	            }
                       
            }	            
            
        }

        	    	    
	    if(oldsec == rtc->sec)  continue;
        oldsec = rtc->sec;

#ifdef	MPLC_TEST
		if(oldsec == 0)		dnp->initial = STATUS_DUMP_TICK;
#endif

		/* 초기 기동후... 인터락 상태 제어 */
		if(dnp->interLockCheck == 0)
		{
			if(dnp->initial == STATUS_DUMP_TICK)
			{
				check_InterLock(dnp);
				dnp->interLockCheck = SET;
			}			
		}
			
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
	
	return (0);
	
}

/*
*/
int  make_tcpip_mplc_Call(int scanid, SCAN_CONFIG *scan)
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
        
        if(dnp->scan == 0)  continue;
        
        printf("scan%02d> index=%d, 미쯔비시-TCPIP(%02d) Task Call... !\n", scanid+1, i, devid+1);
                            
        MOD_THREAD[devid].tid        = 0;
        sprintf(MOD_THREAD[devid].name, "%s%d","IED", devid+1);
        
        MOD_THREAD[devid].priority   = 0;
        MOD_THREAD[devid].options    = 0;       // 비실행 Flag
    
        /* ------------------------------------------------ */
		/* 	ACTIVE 상태에 따라서 Thread 실행 	            */
		/* ------------------------------------------------ */
		if(scan->active == RESET)	continue;
        
        MOD_THREAD[devid].entryPt = scan_tcpip_mplc;  		// TCPIP 방식 MODBUS-THREAD			    
    			    
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
void    *scan_mplc_thread_TCP(int  *arg)
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

    printf(" %s SCAN[%d] 미쯔비시-TCPIP PROCESS Activated ...(COM-delay = %d ms, Offline Count=%d) !\n", TARGET_NAME, scanid, taskDelay, scan->offCount);

    sprintf(buffer, "%s SCAN[%d] 미쯔비시-TCPIP PROCESS Activated ...(COM-delay = %d ms, Offline Count=%d) !", TARGET_NAME, scanid, taskDelay, scan->offCount);
    LogFile_MPU ( opr, NOT_DEFINED, buffer, strlen(buffer));
    
#if 0            
    printf("SCAN[%d] device Num = %02d ... ", scanid, scan->devNum );
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
    make_tcpip_mplc_Call(scanid, scan); 
    
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
            devid = scan->device[i] & 0x3f;
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
        	            devid = scan->device[i] & 0x3f;
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

