
#include	"localLib.h"
#include    "external.h"

extern  int     dnpChanRead(int fid, int devid, byte *rxbuf, int reqCount);
extern  int     checkInfoMODBUS(int devid, int stsCount, byte *rxbuf, int dataLen);
extern  int     checkBlockMODBUS(int scanid, int devid,  int opcode, byte *rxbuf, int dataLen);
extern  int     dnpFlagClear( int devid);
extern  void    vmeScanInitial(int scanid, int chid);
extern  int     dnpChanWrite(int fid, int devid, byte *sndbuf, int sndCount);
extern  int     setOffline( int devid);
extern  int     setOnline( int devid);
extern  int     vmeScanChanReset(int devid);


extern  TTY_DESC scanChan[MAX_CPUPORT];

extern  int             termExec;
extern  TASK_INFO	    *taskPtr;

extern  PDPDTIME_DATE_TIME      dnpRTC;
extern  PDPDTIME_MS_SINCE_70    dnpRTCInfo;

/* ---------------------------------------- */
/*  VMEBUS 관련 변수 초기화                 */
/* ---------------------------------------- */
extern  VME_SIODCB      *vmeSioDCB;
extern  VME_CHAN_DCB    *vmeChanDCB[SIO_CHANNEL_MAX];

char 	modbusFormat[8][10]= {{"BIT  "}, {"BYTE "}, {"SHORT"},{"WORD "},{"INT  "}, {"UINT "}, {"FLOAT"}};
char 	modbusData[4][10]= {{"DI-TYPE"}, {"AI-TYPE"}, {"DO-TYPE"},{"AO-TYPE "}};

/*
*/
int cnvTimeBCD(int data)
{
    int     index1, index2;
    byte    bcd;

    index1 = data / 10;
    index2 = data % 10;
    
    bcd = ((index1 << 4) & 0xf0) + (index2 & 0x0f);
    
    //Debug(console,"bcd = %2d ...%2x\n", data, bcd);
            
    return bcd;
}


/*
*	MODBUS 명령어 처리...cmd를 만들고 보내지는 않는데... scan->virtual은 또 뭐냐 ..
*/
int makeCmdMODBUS( int mode, int scanid, int devid, byte *sndbuf)
{
	int		i;
	int		readCount, readIndex;
    int 	count=0;
    int 	opcode;
    int 	baseAddr, baseOffset, dataNum=0;
    int     blockBase=0, blockOffset=0;
    word    cntrBase=0, cntrValue=0;
    int     index1, index2;
    int		diWordNum;
    int 	point, cntrTCF;
    int     length;
    int     year;
    word    crc;
    
    DNP_DEVICE  *dnp;
    SCAN_CONFIG     *scan;
    POINT_BUF   *ptBuf;
    MODBUS_READ	    *readBlock=NULL, *timeBlock=NULL;
    MODBUS_WRITE    *writeBlock;

    LOCK_POINT_BUF  *locPoint;
    
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
            
	/* ---------------------------------------- */
	/* 	계전기 시각동기 여부 Check ... 			*/  
	/* ---------------------------------------- */  
    if(dnp->timeSyncReq)
    {
    	timeBlock = (MODBUS_READ *) &dnp->timeBlock;
    	if(timeBlock->useFlag == SET)	opcode = MODBUS_WR_MULTI_REG;
    	else
    	{
    		dnp->timeSyncReq = RESET;
    		return (0);
    	}		
	}
    else if(dnp->faultResetReq == SET)  opcode = MODBUS_WR_SINGLE_COIL;
    else if(dnp->selectReq == SET)      
    {
        /* -------------------------------------------- */
        /* 제어포인트의 Control-Base Check...           */
        /* -------------------------------------------- */
        point   = dnp->cntPoint;
        ptBuf = (POINT_BUF *) &dnp->doPtBuf[point];
        
        /* -------------------------------------------- */
    	/* 제어포인트별 정의된 Write 블록 찾음...       */
    	/* -------------------------------------------- */
    	writeBlock = (MODBUS_WRITE *) &dnp->writeBlock[ptBuf->writeBlockIndex];
    	
    	if((writeBlock->useFlag == 1) && (writeBlock->opcode == MODBUS_WR_SINGLE_REG))  opcode = MODBUS_WR_SINGLE_REG;          // opcode = 0x06
        else                                                                            opcode = MODBUS_WR_SINGLE_COIL;         // else 0x05
    }
    else if(dnp->operateReq == SET)     
    {
        /* -------------------------------------------- */
        /* 제어포인트의 Control-Base Check...           */
        /* -------------------------------------------- */
        point   = dnp->cntPoint;
        ptBuf = (POINT_BUF *) &dnp->doPtBuf[point];
        
        /* -------------------------------------------- */
    	/* 제어포인트별 정의된 Write 블록 찾음...       */
    	/* -------------------------------------------- */
    	writeBlock = (MODBUS_WRITE *) &dnp->writeBlock[ptBuf->writeBlockIndex];
    	
    	if((writeBlock->useFlag == 1) && (writeBlock->opcode == MODBUS_WR_SINGLE_REG))  opcode = MODBUS_WR_SINGLE_REG;          // opcode = 0x06
        else                                                                            opcode = MODBUS_WR_SINGLE_COIL;         // else 0x05
    }
    else if(dnp->eventDumpReq == SET)   opcode = MODBUS_RD_HOLD_REG;
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
    		if(dnp->modbusRcvOK == SET)
    		{     
    	        dnp->readIndex = (dnp->readIndex + 1) & 0x07;
    	    }
    	    
    		readIndex 	   = dnp->readIndex;
    		
    		/* -------------------------------------------- */
    		/* 각 블록의 정의된 READ 블록 찾음...           */
    		/* -------------------------------------------- */
    		readBlock = (MODBUS_READ *) &dnp->readBlock[readIndex];
    		
    		/* readBlock 이 정의되지 않은 경우 ... */
    		if(readBlock->useFlag == 0)	
    		{
RE_CHECK_OPCODE:    			
    			/* default READ-BLOCK */
    			if(++readCount > 64)
    			{
    				dnp->readIndex = 0;
    				readIndex 	   = dnp->readIndex;
    				readBlock = (MODBUS_READ *) &dnp->readBlock[readIndex];
    				
    				/* Default OP-CODE (0x04)*/
    				opcode = MODBUS_RD_INPUT_REG;
    				readBlock->dataFormat = FORMAT_WORD;
    				readBlock->dataNum = dnp->devDiPoint;
    		
    				/* DI(Bit 타입-2Byte/1Word), AI(Float 타입-4 Byte/2Word) 포인트 수 반영 */
    				index1   = dnp->devDiPoint / 16;
    				index2   = dnp->devDiPoint % 16;
    				
    				if(index2)	diWordNum = index1 + 1;
    				else		diWordNum = index1;
    				
    				baseAddr = 0;
        			dataNum  = diWordNum + (dnp->devAiPoint * 2);
        			if(dataNum > 100)	dataNum = 100;
        			
        			/* Defalt OPCODE 사용 */
        			dnp->defaultRead = SET;	
        			
        			if((opr->dnpDebug == devid) || (opr->scanDebug == scanid))
        			Debug(console,"MODBUS> dev=%2d...*default read...opcode=%d, base=%d, datanum=%d!\n", devid+1, opcode, baseAddr, dataNum);
        				
    				break;
    			}	
    			else	continue;
    		}
    		
    		/* -------------------------------------------- */
    		/*	해당 READ-BLOCK 내의 OPCODE 적용			*/
    		/* -------------------------------------------- */
    		opcode   = readBlock->opcode;
    		
    		/* -------------------------------------------- */
    		/*	데이터 타입(DI, AI) 에 따른 정보처리		*/
    		/* -------------------------------------------- */
    		if(readBlock->dataType == MODBUS_TYPE_DI)
    		{
				if((opcode == MODBUS_RD_INPUT_BIT) || (opcode == MODBUS_RD_INPUT_DIS) || (opcode == MODBUS_RD_HOLD_REG) || (opcode == MODBUS_RD_INPUT_REG))
    			{	
    				/* Base,Offset  Address 지정 */
    				blockBase   = readBlock->baseAddr;
    				blockOffset = readBlock->baseOffset;

                    dataNum = readBlock->dataNum;
   					if(dataNum == 0)    dataNum = dnp->devDiPoint;

                    /* ---------------------------------------- */    					    
					/*  read Format에 따른 데이터 개수 지정     */
					/* ---------------------------------------- */
    				if((readBlock->dataFormat == FORMAT_BIT) || (readBlock->dataFormat == FORMAT_BYTE))
    				{
    				    /* FORMAT_BIT : 개별 포인트 수 지정 */ 
    				    //Debug(console,"modbus> BIT read ... %d  %d\n", dataNum, readBlock->dataNum);
    				    
		    		}
		    		else if((readBlock->dataFormat == FORMAT_SHORT) || (readBlock->dataFormat == FORMAT_WORD))
    				{
    					index1 = dataNum / 16;
    					index2 = dataNum % 16;
    					
    					if(index2)	dataNum = index1 + 1;
    					else		dataNum = index1;	
    				}
		    		else
    				{
    					index1 = readBlock->dataNum / 16;
    					index2 = readBlock->dataNum % 16;
    					
    					if(index2)	dataNum = index1 + 1;
    					else		dataNum = index1;	
	    			}
    		
	    			/* Data Count Error Check ... */
    				if((dataNum < 0) || (dataNum > MAX_DEV_DI_POINT))	
    				{
    				    if((opr->dnpDebug == devid) || (opr->scanDebug == scanid))
    				    Debug(console,"modbus> dev=%2d...*Recheck... 1\n", devid+1);
    				    goto RE_CHECK_OPCODE;
    				}
    				dnp->defaultRead = RESET;	
    				
    				break;
    			}
    			else
    			{
    			    if((opr->dnpDebug == devid) || (opr->scanDebug == scanid))
    			    Debug(console,"modbus> dev=%2d...*Recheck... 2\n", devid+1);
    				goto RE_CHECK_OPCODE;
    			}	    			
    		}	
    		else if(readBlock->dataType == MODBUS_TYPE_AI)
    		{	
    			/* ---------------------------------------- */
    			/* 	아날로그 데이터 수집 주기 ... 좀 천천히 */
    			/* ---------------------------------------- */
    			if(++readBlock->commIndex > 4)
    			{
    				readBlock->commIndex = 0;
    			}
    			else	continue;

	    		if((opcode == MODBUS_RD_HOLD_REG) || (opcode == MODBUS_RD_INPUT_REG))       // MODBUS_RD_HOLD_REG = 0x03, MODBUS_RD_INPUT_REG = 0x04
    			{	
    				/* Base,Offset  Address 지정 */
    				blockBase   = readBlock->baseAddr;
    				blockOffset = readBlock->baseOffset;

                    /* ------------------------------------ */
					/* read Format에 따른 데이터 개수 지정  */
					/*  기본 : 16bit 단위 (word)            */
					/* ------------------------------------ */
		    		if((readBlock->dataFormat == FORMAT_SHORT) || (readBlock->dataFormat == FORMAT_WORD))
    				{
    					dataNum = readBlock->dataNum;
	    			}
		    		else if((readBlock->dataFormat == FORMAT_INT32) || (readBlock->dataFormat == FORMAT_UINT32) || (readBlock->dataFormat == FORMAT_FLOAT))
    				{
    					dataNum = readBlock->dataNum * 2;
	    			}
	    			else 
	    			{
	    			    if((opr->dnpDebug == devid) || (opr->scanDebug == scanid))
	    			    Debug(console,"modbus> dev=%2d...*Recheck... 3\n", devid+1);
	    				goto RE_CHECK_OPCODE;
	    			}	
    		
	    			/* Data Count Error Check ... */
    				if((dataNum < 0) || (dataNum > MAX_DEV_AI_POINT))	goto RE_CHECK_OPCODE;
    				dnp->defaultRead = RESET;	
    				break;
    			}
    			else
    			{
    			    if((opr->dnpDebug == devid) || (opr->scanDebug == scanid))
    			    Debug(console,"modbus> dev=%2d...*Recheck... 4\n", devid+1);
    				goto RE_CHECK_OPCODE;
    			}	
    		}
    		else
    		{
    		    if((opr->dnpDebug == devid) || (opr->scanDebug == scanid))
    		    Debug(console,"modbus> dev=%2d...*Recheck... 5\n", devid+1);
    			goto RE_CHECK_OPCODE;
    		}	
    	}
    	
    }
    
    /* ---------------------------------------- */
    /*  make MODBUS Packet(TCPIP 또는 RS232)    */
    /* ---------------------------------------- */
    if(mode == COM_TCPIP)
    {
        sndbuf[0] = 0x7E;               // Header
        sndbuf[1] = dnp->comSndCount;   // Seqno 번호
        sndbuf[2] = 0;                  // Header
        sndbuf[3] = 0;                  // Header
    
        sndbuf[4] = 0;                  // Length High
        sndbuf[5] = 0;                  // Length Low
    
        /* -------------------------------- */
        /*  가상계전기 : 통신처리           */    
        /* -------------------------------- */
        if(scan->virtualMode == 1)
        {
            sndbuf[6] = scan->comAddr;      // 실제 통신 Address 지정, 0,1,2..
            sndbuf[7] = opcode;    // 
            count = 8;
        }
        else
        {             
            sndbuf[6] = devid + 1;          // Address
            sndbuf[7] = opcode;             // 
            count = 8;        
        }
    }
    else
    {   
        /* -------------------------------- */
        /*  가상계전기 : 통신처리           */    
        /* -------------------------------- */
        if(scan->virtualMode == 1)
        {
            sndbuf[0] = scan->comAddr;      // 실제 통신 Address 지정, 0,1,2..
            sndbuf[1] = opcode;    // 
            count = 2;
        }
        else
        {                   
            sndbuf[0] = devid + 1;
            sndbuf[1] = opcode;    // 
            count = 2;
        }
    }

    /* 계전기 통신상태 : Clear... */
    dnp->modbusRcvOK = RESET;
    
	/* ---------------------------------------------------- */
	/*	지멘스 계전기의 경우... 이중화 상태/Flag 표기 		*/
	/* ---------------------------------------------------- */
    if((opr->dnpDebug == devid) || (opr->scanDebug == scanid))
    {
   		Debug(console,"\n --------------------------------[%02d/%02d-%02d:%02d:%02d]\n", 
      		dnp->activeChan, dnp->activeFlag, rtc->hour, rtc->min, rtc->sec); 		
    }
        
    switch(opcode)
    {
    case MODBUS_RD_INPUT_BIT:       				    // (0x01) BIT Stream Read */       
    case MODBUS_RD_INPUT_DIS:                           // (0x02) Read Single-BIT         
    case MODBUS_RD_HOLD_REG:        				    // (0x03) Read Holding Registers
    case MODBUS_RD_INPUT_REG:       				    // (0x04) Read Input Registers  
    	sndbuf[count++] = (blockOffset >> 8) & 0xff;	// Start Addr HIGH
        sndbuf[count++] = blockOffset & 0xff;			// Start Addr LOW
        
        if(opr->msgDebug == devid)  
            Debug(console,"dev%2d> modbus Read(%d) [opcode=%2d, Base/Offset= %5d/%5d, point=%2d] Type/fmt = %s/%s ... data req=%4d\n", devid+1, dnp->readIndex,
                opcode, blockBase, blockOffset, readBlock->dataNum, modbusData[readBlock->dataType], modbusFormat[readBlock->dataFormat], dataNum);
         
        sndbuf[count++] = (dataNum >> 8) & 0xff;        // Number of Register HIGH
        sndbuf[count++] = dataNum & 0xff;   		    // Number of Register LOW
        
        /* -------------------------------------------- */
        /*  수신되는 MODBUS 응당 데이터 수 ...          */
        /* -------------------------------------------- */
        if((opcode == MODBUS_RD_HOLD_REG) || (opcode == MODBUS_RD_INPUT_REG))   
        {
            dnp->reqDataCount = dataNum * 2;
            dnp->reqDataOpcode= opcode;
        }
        else if((opcode == MODBUS_RD_INPUT_BIT) || (opcode == MODBUS_RD_INPUT_DIS))
        {
            index1 = dataNum / 8;
            index2 = dataNum % 8;
            
            if(index2)  dnp->reqDataCount = index1 + 1;
            else        dnp->reqDataCount = index1;       
            
            dnp->reqDataOpcode= opcode;     
        }
        else
        {
            dnp->reqDataCount = 0;
        }                
            
        break;                

    case MODBUS_WR_SINGLE_COIL:     // (0x05) Write Single Coil     
    case MODBUS_WR_SINGLE_REG:      // (0x06) Write Single Register     
        
        /* -------------------------------------------- */
        /* 제어포인트의 Control-Base Check...           */
        /* -------------------------------------------- */
        point   = dnp->cntPoint;
        cntrTCF = dnp->cntTcf;
        ptBuf = (POINT_BUF *) &dnp->doPtBuf[point];
        
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
                    }      
                }
                
                dnp->selectReq  = 0;
                dnp->operateReq = 0;
                dnp->cntPoint   = 0;
                dnp->cntTcf     = 0;
                
                return (0);
            }
        }
            
        /* -------------------------------------------- */
    	/* 제어포인트별 정의된 Write 블록 찾음...       */
    	/* -------------------------------------------- */
    	writeBlock = (MODBUS_WRITE *) &dnp->writeBlock[ptBuf->writeBlockIndex];
        
        cntrBase = ptBuf->dbmax;     

        /* -------------------------------------------- */        
        /* 지멘스 계전기 ... 제어포인트 선정...         */
        /* -------------------------------------------- */       
        if(dnp->type == TYPE_MODBUS_SZ)
        {    
            if(ptBuf->dbmax > 0)
            {
                /* -------------------------------- */
                /* 지멘스계전기 TRIP/CLOSE  제어시  */
                /* -------------------------------- */
                if((ptBuf->ptConfig == MODBUS_TRIP_FIRST) || (ptBuf->ptConfig == MODBUS_CLOSE_FIRST))   cntrBase = ptBuf->dbmax - 1;
                else                                                                                    cntrBase = ptBuf->dbmax;                        
            }
            else
            {
                if(opr->soeDebug)
                Debug(console,"snd%2d> *** SIEMENSE Control Fail (dbmax == %d) : Point= %d \n", devid+1, ptBuf->dbmax, dnp->cntPoint+1);    
                dnp->operateReq = RESET;
                dnp->selectReq  = RESET;
                return 0;
            }   
        }
                   
#if 0
#define MODBUS_TRIP_FIRST       0       // TRIP/CLOSE 제어시 : TRIP  선행제어
#define MODBUS_CLOSE_FIRST      1       // TRIP/CLOSE 제어시 : CLOSE 선행제어
#define MODBUS_RESET_CNTR       2       // RESET 제어시
#define MODBUS_ONOFF_CNTR       3       // ON/OFF 제어시

#define MODBUS_TC_PLUSE         5       // TRIP/CLOSE 제어후 RESET : 동일 Address, TRIP (2), CLOSE (1), RESET(0)
#define MODBUS_TRIP_PLUSE       6       // PLUSE 제어후 RESET : TRIP  First, 제어시(1), RESET(0)      
#define MODBUS_CLOSE_PLUSE      7       // PLUSE 제어후 RESET : CLOSE First, 제어시(1), RESET(0)      
     
#define MODBUS_SBO_LG           8       // LG  계전기 대상 SELECT-OPERATE 제어시 : CLOSE First
#define MODBUS_SBO_ICE          11      // ICE 계전기 대상 SELECT-OPERATE 제어시 : FAULT-RESET 
#endif
                   
        /* ------------------------------------------------------------------------ */
        /* ptConfig == 0 : 지멘스제어기 TRIP/CLOSE 제어 (TRIP 선행)                 */
        /* ptConfig == 1 : 지멘스제어기 TRIP/CLOSE 제어 (CLOSE 선행)                */
        /* ptConfig == 2 : 지멘스제어기 RESET  제어                                 */
        /* ptConfig == 3 : 지멘스제어기 BIT 제어                                    */
        /* ptConfig == 5 : 미쯔비시 : 동일 Addr TRIP(2)/CLOSE(1) 제어후 RESET 수행  */       
        /* ptConfig == 6 : 미쯔비시 : TRIP  선행 제어(1) 후 RESET 수행              */       
        /* ptConfig == 7 : 미쯔비시 : CLOSE 선행 제어(1) 후 RESET 수행              */       
        /* ptConfig == 8 : LG제어기 : Select-Operate 제어                           */
        /* ptConfig == 11: 지멘스제어기 Fault-Locator RESET 제어                    */
        /* ------------------------------------------------------------------------ */

        if(ptBuf->ptConfig == MODBUS_TRIP_FIRST)            /* 지멘스제어기 TRIP/CLOSE  제어시 : TRIP 선행 */
        {
            if(cntrTCF == TRIP_CONTROL)
            {
                cntrBase = cntrBase + 0;
                cntrValue = 0xff00;
                    
                if(opr->soeDebug)
                Debug(console,"snd%2d> Write-SINGLE(op=%02d) (cfg=0) : Point= %d, TRIP-OPERATE   [base=%04x, value=%04x]\n", devid+1, opcode, dnp->cntPoint+1, cntrBase, cntrValue);
            }
            else if(cntrTCF == CLOSE_CONTROL)
            {
                cntrBase = cntrBase + 1;
                cntrValue = 0xff00;
                    
                if(opr->soeDebug)
                Debug(console,"snd%2d> Write-SINGLE(op=%02d) (cfg=0) : Point= %d, CLOSE-OPERATE  [base=%04x, value=%04x]\n", devid+1, opcode, dnp->cntPoint+1, cntrBase, cntrValue);
            }
        }
        else if(ptBuf->ptConfig == MODBUS_CLOSE_FIRST)            /* 지멘스제어기 TRIP/CLOSE  제어시 : CLOSE 선행 */
        {
            if(cntrTCF == TRIP_CONTROL)
            {
                cntrBase = cntrBase + 1;
                cntrValue = 0xff00;
                  
                if(opr->soeDebug)
                Debug(console,"snd%2d> Write-SINGLE(op=%02d) (cfg=1) : Point= %d, TRIP-OPERATE   [base=%04x, value=%04x]\n", devid+1, opcode, dnp->cntPoint+1, cntrBase, cntrValue);
            }
            else if(cntrTCF == CLOSE_CONTROL)
            {
                cntrBase = cntrBase + 0;
                cntrValue = 0xff00;
                   
                if(opr->soeDebug)
                Debug(console,"snd%2d> Write-SINGLE(op=%02d) (cfg=1) : Point= %d, CLOSE-OPERATE  [base=%04x, value=%04x]\n", devid+1, opcode, dnp->cntPoint+1, cntrBase, cntrValue);
            }
        }
        else if((ptBuf->ptConfig == MODBUS_RESET_CNTR) || (ptBuf->ptConfig == MODBUS_ONOFF_CNTR))       /* 지멘스제어기 RESET 제어시 */
        {
            if(cntrTCF == TRIP_CONTROL)         
            {
                if(writeBlock->useFlag == 0)    cntrValue = 0x0000;         // 해정 : 0x0000
                else                            cntrValue = writeBlock->tripData;    
                   
                if(ptBuf->ptConfig == 2)
                {            
                    if(opr->soeDebug)
                    Debug(console,"snd%2d> Write-SINGLE(op=%02d) (cfg=2) : Point= %d, RESET-TRIP [base=%04x, value=%04x]\n", devid+1, opcode, dnp->cntPoint+1, cntrBase, cntrValue);     
                }
                else if(ptBuf->ptConfig == 3)
                {   
                    if(opr->soeDebug)
                    Debug(console,"snd%2d> Write-SINGLE(op=%02d) (cfg=3) : Point= %d, BIT-TRIP   [base=%04x, value=%04x]\n", devid+1, opcode, dnp->cntPoint+1, cntrBase, cntrValue);  
                }
            }
            else if(cntrTCF == CLOSE_CONTROL)   
            {
                if(writeBlock->useFlag == 0)    cntrValue = 0xff00;         // 쇄정 : 0xff00
                else                            cntrValue = writeBlock->closeData;        
                  
                if(ptBuf->ptConfig == 2)
                {     
                    if(opr->soeDebug)
                    Debug(console,"snd%2d> Write-SINGLE(op=%02d) (cfg=2) : Point= %d, RESET-CLOSE [base=%04x, value=%04x]\n", devid+1, opcode, dnp->cntPoint+1, cntrBase, cntrValue);    
                }
                else if(ptBuf->ptConfig == 3)
                {
                    if(opr->soeDebug)
                    Debug(console,"snd%2d> Write-SINGLE(op=%02d) (cfg=3) : Point= %d, BIT-CLOSE   [base=%04x, value=%04x]\n", devid+1, opcode, dnp->cntPoint+1, cntrBase, cntrValue);     
                }
            }
        }
        /* -------------------------------------------- */
        /*  LG 계전기 기준 : Select-Operate 제어 (SBO)  */
        /* - CLOSE_SELECT [0], CLOSE_OPERATE [1]        */
        /* - TRIP_SELECT  [2], TRIP_OPERATE  [3]        */
        /* -------------------------------------------- */
        else if(ptBuf->ptConfig == MODBUS_SBO_LG)       /* SELECT/OPERATE 제어시 */
        {
            if((dnp->selectReq + dnp->operateReq) == 0)    break;
                
            if(cntrTCF == TRIP_CONTROL)
            {
                if(writeBlock->useFlag == 0)    cntrValue = 0xff00;
                else                            cntrValue = writeBlock->tripData;   
                    
                if(dnp->selectReq)          
                {
                    cntrBase = cntrBase + TRIP_SELECT;
                    if(opr->soeDebug)
                    Debug(console,"snd%2d> Write-SINGLE(op=%02d) (cfg=8) : Point= %d, TRIP-SELECT  [base=%04x, value=%04x]\n", devid+1, opcode, dnp->cntPoint+1,cntrBase, cntrValue);
                }
                else if(dnp->operateReq)    
                {
                    cntrBase = cntrBase + TRIP_OPERATE;
                    if(opr->soeDebug)
                    Debug(console,"snd%2d> Write-SINGLE(op=%02d) (cfg=8) : Point= %d, TRIP-OPERATE [base=%04x, value=%04x]\n", devid+1, opcode, dnp->cntPoint+1,cntrBase, cntrValue);
                }
                else break;
            }
            else if(cntrTCF == CLOSE_CONTROL)
            {
                if(writeBlock->useFlag == 0)    cntrValue = 0xff00;
                else                            cntrValue = writeBlock->closeData;    
                    
                if(dnp->selectReq)          
                {
                    cntrBase = cntrBase + CLOSE_SELECT;
                    if(opr->soeDebug)
                    Debug(console,"snd%2d> Write-SINGLE(op=%02d) (cfg=8) : Point= %d, CLOSE-SELECT   [base=%04x, value=%04x]\n", devid+1, opcode, dnp->cntPoint+1,cntrBase, cntrValue);
                }
                else if(dnp->operateReq)    
                {
                    cntrBase = cntrBase + CLOSE_OPERATE;
                    if(opr->soeDebug)
                    Debug(console,"snd%2d> Write-SINGLE(op=%02d) (cfg=8) : Point= %d, CLOSE-OPERATE  [base=%04x, value=%04x]\n", devid+1, opcode, dnp->cntPoint+1,cntrBase, cntrValue);
                }
                else break;     
            }                
        }    
        /* -------------------------------------------- */
        /*  ICE 계전기 기준 : Fault-Locator RESET (SBO) */
        /* - Select  : TRIP 데이터 제어                 */ 
        /* - Operate : CLOSE 데이터 제어                */
        /* -------------------------------------------- */
        else if(ptBuf->ptConfig == MODBUS_SBO_ICE)       /* SELECT/OPERATE 제어시 */
        {
            if((dnp->selectReq + dnp->operateReq) == 0)    break;
                
            if(dnp->selectReq)          
            {
                if(writeBlock->useFlag == 0)    cntrValue = 0x0000;
                else                            cntrValue = writeBlock->tripData;   
                        
                if(opr->soeDebug)
                Debug(console,"snd%2d> Write-RESET(op=%02d) (cfg=11) : Point= %d, RESET-SELECT  [base=%04x, value=%04x]\n", devid+1, opcode, dnp->cntPoint+1,cntrBase, cntrValue);    
            }
            else if(dnp->operateReq)          
            {
                if(writeBlock->useFlag == 0)    cntrValue = 0x0001;
                else                            cntrValue = writeBlock->closeData;   
                        
                if(opr->soeDebug)
                Debug(console,"snd%2d> Write-RESET(op=%02d) (cfg=11) : Point= %d, RESET-OPERATE  [base=%04x, value=%04x]\n", devid+1, opcode, dnp->cntPoint+1,cntrBase, cntrValue);    
            }
            else
               break;
        } 
        /* -------------------------------------------------------------------------- */
        /* ptConfig == 5 : 미쯔비시 : 동일 Addr TRIP(2)/CLOSE(1) 제어후 RESET 수행  */  
        /* -------------------------------------------------------------------------- */  
        else if (ptBuf->ptConfig == MODBUS_TC_PLUSE)
        {
            if(dnp->selectReq)
            {       
                if(cntrTCF == TRIP_CONTROL)
                {
                    cntrBase = cntrBase + 0;
                    if(writeBlock->useFlag == 0)    cntrValue = 0x0002;
                    else                            cntrValue = writeBlock->tripData; 
                    
                    if(opr->soeDebug)
                    Debug(console,"snd%2d> Write-SINGLE(op=%02d) (cfg=%d) : Point= %d, TRIP-PULSE... SET  [base=%04x, value=%04x]\n", devid+1, opcode, ptBuf->ptConfig, dnp->cntPoint+1, cntrBase, cntrValue);
                }
                else if(cntrTCF == CLOSE_CONTROL)
                {
                    cntrBase = cntrBase + 0;
                    if(writeBlock->useFlag == 0)    cntrValue = 0x0001;
                    else                            cntrValue = writeBlock->closeData;   
                    
                    if(opr->soeDebug)
                    Debug(console,"snd%2d> Write-SINGLE(op=%02d) (cfg=%d) : Point= %d, CLOSE-PULSE... SET  [base=%04x, value=%04x]\n", devid+1, opcode, ptBuf->ptConfig, dnp->cntPoint+1, cntrBase, cntrValue);
                }
            }
            else if(dnp->operateReq)          
            {
                pause(200);
                
                cntrBase = cntrBase + 0;
                cntrValue = 0x0000;
                    
                if(opr->soeDebug)
                Debug(console,"snd%2d> Write-SINGLE(op=%02d) (cfg=%d) : Point= %d, PULSE-CLOSE... *RESET  [base=%04x, value=%04x]\n", devid+1, opcode, ptBuf->ptConfig, dnp->cntPoint+1, cntrBase, cntrValue);
            }
        }
        /* ptConfig == 6 : 미쯔비시 : TRIP  선행 제어(1) 후 RESET 수행              */       
        else if (ptBuf->ptConfig == MODBUS_TRIP_PLUSE)
        {
            if(dnp->selectReq)
            {       
                if(cntrTCF == TRIP_CONTROL)
                {
                    cntrBase = cntrBase + 0;
                    if(writeBlock->useFlag == 0)    cntrValue = 0x0001;
                    else                            cntrValue = writeBlock->tripData; 
                    
                    if(opr->soeDebug)
                    Debug(console,"snd%2d> Write-SINGLE(op=%02d) (cfg=%d) : Point= %d, TRIP-PULSE... SET  [base=%04x, value=%04x]\n", devid+1, opcode, ptBuf->ptConfig, dnp->cntPoint+1, cntrBase, cntrValue);
                }
                else if(cntrTCF == CLOSE_CONTROL)
                {
                    cntrBase = cntrBase + 1;
                    if(writeBlock->useFlag == 0)    cntrValue = 0x0001;
                    else                            cntrValue = writeBlock->closeData;   
                    
                    if(opr->soeDebug)
                    Debug(console,"snd%2d> Write-SINGLE(op=%02d) (cfg=%d) : Point= %d, CLOSE-PULSE... SET  [base=%04x, value=%04x]\n", devid+1, opcode, ptBuf->ptConfig, dnp->cntPoint+1, cntrBase, cntrValue);
                }
            }
            else if(dnp->operateReq)          
            {
                pause(200);
                
                if(cntrTCF == TRIP_CONTROL)     cntrBase = cntrBase + 0;
                if(cntrTCF == CLOSE_CONTROL)    cntrBase = cntrBase + 1;    
                cntrValue = 0x0000;
                    
                if(opr->soeDebug)
                Debug(console,"snd%2d> Write-SINGLE(op=%02d) (cfg=%d) : Point= %d, PULSE-CLEAR ... *RESET  [base=%04x, value=%04x]\n", devid+1, opcode, ptBuf->ptConfig, dnp->cntPoint+1, cntrBase, cntrValue);
            }            
        }
        /* ptConfig == 7 : 미쯔비시 : CLOSE 선행 제어(1) 후 RESET 수행              */        
        else if (ptBuf->ptConfig == MODBUS_CLOSE_PLUSE)
        {
            if(dnp->selectReq)
            {       
                if(cntrTCF == TRIP_CONTROL)
                {
                    cntrBase = cntrBase + 1;
                    if(writeBlock->useFlag == 0)    cntrValue = 0x0001;
                    else                            cntrValue = writeBlock->tripData; 
                    
                    if(opr->soeDebug)
                    Debug(console,"snd%2d> Write-SINGLE(op=%02d) (cfg=%d) : Point= %d, TRIP-PULSE... SET  [base=%04x, value=%04x]\n", devid+1, opcode, ptBuf->ptConfig, dnp->cntPoint+1, cntrBase, cntrValue);
                }
                else if(cntrTCF == CLOSE_CONTROL)
                {
                    cntrBase = cntrBase + 0;
                    if(writeBlock->useFlag == 0)    cntrValue = 0x0001;
                    else                            cntrValue = writeBlock->closeData;   
                    
                    if(opr->soeDebug)
                    Debug(console,"snd%2d> Write-SINGLE(op=%02d) (cfg=%d) : Point= %d, CLOSE-PULSE... SET  [base=%04x, value=%04x]\n", devid+1, opcode, ptBuf->ptConfig, dnp->cntPoint+1, cntrBase, cntrValue);
                }
            }
            else if(dnp->operateReq)          
            {
                pause(200);
                
                if(cntrTCF == TRIP_CONTROL)     cntrBase = cntrBase + 1;
                if(cntrTCF == CLOSE_CONTROL)    cntrBase = cntrBase + 0;    
                cntrValue = 0x0000;
                    
                if(opr->soeDebug)
                Debug(console,"snd%2d> Write-SINGLE(op=%02d) (cfg=%d) : Point= %d, PULSE-CLEAR ... *RESET  [base=%04x, value=%04x]\n", devid+1, opcode, ptBuf->ptConfig, dnp->cntPoint+1, cntrBase, cntrValue);
            }                        
        }
        else
        {
            if(opr->soeDebug)
            Debug(console,"snd%2d> *** Write-SINGLE(op=%02d)  Fail(cfg=%d): Point= %d, base=%04x .... ERROR  \n", devid+1, opcode, ptBuf->ptConfig, dnp->cntPoint+1, cntrBase);    
            dnp->operateReq = RESET;
            dnp->selectReq  = RESET;
            return 0;
        }    

        sndbuf[count++] = (cntrBase >> 8) & 0xff;       // Start Addr HIGH
        sndbuf[count++] = cntrBase & 0xff;              // Start Addr LOW
        
        sndbuf[count++] = (cntrValue >> 8) & 0xff;      // Control Value HIGH
        sndbuf[count++] = cntrValue & 0xff;             // Control Value LOW
        
        break;
        

    case MODBUS_WR_MULTI_REG:       // (0x10) Write Single Register 
        if(dnp->timeSyncReq)
        {
            /* ------------------------------------------------ */
            /*  TIME-SYNC 시 Start Addr 지정 ...                */
            /*  지멘스 계전기 = 100, LG 계전기 = 6000           */
            /* ------------------------------------------------ */
            baseOffset = 6000;
            baseOffset = timeBlock->baseOffset;
            sndbuf[count++] = (baseOffset >> 8) & 0xff;         // Start Addr HIGH
            sndbuf[count++] = baseOffset & 0xff;
            sndbuf[count++] = 0;                                // Number of REG
            sndbuf[count++] = 7;
			sndbuf[count++] = 14;               				// byte count;
			
			/* RTC 시간정보 Update ... */
            year = (rtc->year % 100) + 2000;
            
            /* ------------------------------------------------ */
            /* 계전기 타입 구분 : 지멘스, IEC                   */
            /* ------------------------------------------------ */
            if(timeBlock->dataType == MODBUS_TYPE_DI)
			{
	            sndbuf[count++] = cnvTimeBCD(year / 100);
    	        sndbuf[count++] = cnvTimeBCD(year % 100);
        	    sndbuf[count++] = cnvTimeBCD(rtc->month / 100);
            	sndbuf[count++] = cnvTimeBCD(rtc->month % 100);
	            sndbuf[count++] = cnvTimeBCD(rtc->day / 100);
    	        sndbuf[count++] = cnvTimeBCD(rtc->day % 100);
        	    sndbuf[count++] = cnvTimeBCD(rtc->hour / 100);
            	sndbuf[count++] = cnvTimeBCD(rtc->hour % 100);
	            sndbuf[count++] = cnvTimeBCD(rtc->min / 100);
    	        sndbuf[count++] = cnvTimeBCD(rtc->min % 100);
        	    sndbuf[count++] = cnvTimeBCD(rtc->sec / 100);
            	sndbuf[count++] = cnvTimeBCD(rtc->sec % 100);
            
	            sndbuf[count++] = cnvTimeBCD(0);
    	        sndbuf[count++] = cnvTimeBCD(0);
    	    }
    	    else
    	   	{ 	
				sndbuf[count++] = (year >> 8) &0xff;
    	        sndbuf[count++] = year;
        	    sndbuf[count++] = (rtc->month >> 8) &0xff;
            	sndbuf[count++] = rtc->month;
	            sndbuf[count++] = (rtc->day >> 8) &0xff;
    	        sndbuf[count++] = rtc->day;
        	    sndbuf[count++] = (rtc->hour >> 8) &0xff;
            	sndbuf[count++] = rtc->hour;
	            sndbuf[count++] = (rtc->min >> 8) &0xff;
    	        sndbuf[count++] = rtc->min;
        	    sndbuf[count++] = (rtc->sec >> 8) &0xff;
            	sndbuf[count++] = rtc->sec;
            
	            /* --------------------------------- */
    	        /* IEC Time Sync...					*/
        	    /* --------------------------------- */
            	sndbuf[count++] = 1;
	            sndbuf[count++] = 1;
    		}
    		        
            if(opr->msgDebug == devid)
            Debug(console,"dev%2d> Time-Set Req (base=%d) : %4d/%2d/%2d %02d:%02d:%02d\n", devid+1, baseOffset,
                year, rtc->month, rtc->day, rtc->hour, rtc->min, rtc->sec);	  
        }
        
        /* ------------------------------------------------ */
        /* 제어포인트의 Control-Base Check...               */
        /*  지멘스 계전기와 LG 계전기간의 제어방식 차이     */
        /* ------------------------------------------------ */
        if(dnp->type == TYPE_MODBUS_SZ)
        {    
            dnp->timeSyncReq = 0;
        }
        
        break;

    default:
        Debug(console,"modbus> dev=%2d...* Defaut Opcode(%2d)... return !\n", devid+1, opcode);
        break;        
    }
    
    /* ------------------------------------------------ */
    /* MODBUS Command Message...                        */
    /* ------------------------------------------------ */
    if(mode == COM_TCPIP)
    {
        length = count - 6;    
        sndbuf[4] = (length >> 8) & 0xff;          // Header
        sndbuf[5] = length & 0xff;          // Header
    }
    else
    {    
        crc = genCrc_modbus(sndbuf, count);
        sndbuf[count++] = crc & 0xff;
        sndbuf[count++] = (crc >> 8) & 0xff;
    }
    
    return (count);

    

}


/*
*   MODBUS 통신 - 예외상황
*/
int checkEXPcode(int scanid, int devid, int expCode)
{
    DNP_DEVICE  *dnp;
    SCAN_CONFIG     *scan;

    /* SCAN-TASK 별 모듈구성 정보 */ 
    scan = (SCAN_CONFIG *) scanCfg[scanid];
    dnp  = (DNP_DEVICE *) dnpcfg[devid];
    
    switch(expCode)
    {
    case 0x01:
        if((opr->dnpDebug == devid) || (opr->scanDebug == scanid))
        Debug(console,"dev%2d> *** rcv Exception [ILLEGAL FUNCTION] \n", devid+1);
        break;
    case 0x02:
        if((opr->dnpDebug == devid) || (opr->scanDebug == scanid))
        Debug(console,"dev%2d> *** rcv Exception [ILLEGAL DATA Address] \n", devid+1);
        break;
    case 0x03:
        if((opr->dnpDebug == devid) || (opr->scanDebug == scanid))
        Debug(console,"dev%2d> *** rcv Exception [ILLEGAL DATA Value] \n", devid+1);
        break;                        
    case 0x04:
        if((opr->dnpDebug == devid) || (opr->scanDebug == scanid))
        Debug(console,"dev%2d> *** rcv Exception [SLAVE DEVICE FAILURE] \n", devid+1);
        break;        
    case 0x10:
        if((opr->dnpDebug == devid) || (opr->scanDebug == scanid))
        Debug(console,"dev%2d> *** rcv Exception [Event/Fault Record] \n", devid+1);
        dnp->eventDumpReq = 0;
        break;        
    case 0x11:
        if((opr->dnpDebug == devid) || (opr->scanDebug == scanid))
        Debug(console,"dev%2d> *** rcv Exception [SBO Timeout] \n", devid+1);
        break;        
    case 0x12:
        if((opr->dnpDebug == devid) || (opr->scanDebug == scanid))
        Debug(console,"dev%2d> *** rcv Exception [ILLEGAL ADU Length] \n", devid+1);
        break;        
    case 0x13:
        if((opr->dnpDebug == devid) || (opr->scanDebug == scanid))
        Debug(console,"dev%2d> *** rcv Exception [LOCAL MODE] \n", devid+1);
        break;  

    case 0x14:
        if((opr->dnpDebug == devid) || (opr->scanDebug == scanid))
        Debug(console,"dev%2d> *** rcv Exception [MAIN_NAK] \n", devid+1);
        break;              
          
    default:
        if((opr->dnpDebug == devid) || (opr->scanDebug == scanid))
        Debug(console,"dev%2d> *** rcv Exception [???? Code] ... %2x\n", devid+1, expCode);
        break;        
    }

    /* -------------------------------- */
    /* 제어상태 Clear....               */
    /* -------------------------------- */
    dnp->selectReq      = RESET;
    dnp->operateReq     = RESET;  
    dnp->faultResetReq  = RESET;
    dnp->cntPoint = 0;
    dnp->cntTcf   = 0;         
    
    return (0);        
}


/***************************************************
*   FUNCTION : rcvWithModbus()
*   receive DATA-LINK with CRC
***************************************************/
int rcvWithModbus( int scanid, int devid, byte *rxbuf)
{
    //int     i,j, size;
    int     rxcnt;
    int     opcode;
    int     sizeMode = 0;
    int     reqSize, rcvSize;
    int     expCode;
    int     modAddr;
    word    crc, rcvCrc;
    word    timeout;

    DNP_DEVICE  *dnp;
    SCAN_CONFIG     *scan;


    scan = (SCAN_CONFIG *) scanCfg[scanid];
    dnp = (DNP_DEVICE *) dnpcfg[devid];
    
    /* -------------------------------------------- */
    /*  가상계전기 : 통신 Address 지정              */
    /* -------------------------------------------- */
    if(scan->virtualMode == 1)  modAddr = scan->comAddr;
    else                        modAddr = dnp->ioid;
              
    timeout = 0;
    rxcnt = 0;

#if 0    
    /* ---------------------------- */
    /*  receive STX(0x0564)         */
    /* ---------------------------- */
    do {
        if(dnpChanRead(scanid, devid, &rxbuf[0], 1) == 0) return (0);
        if(++timeout > 50)  return (0);
    } while(rxbuf[0] != dnp->ioid);
#endif

    /* ---------------------------- */
    /*  receive STX(0x0564)         */
    /* ---------------------------- */
    do {
        if(dnpChanRead(scanid, devid, &rxbuf[0], 1) == 0) return (0);
        if(++timeout > 50)  return (0);
    } while(rxbuf[0] != modAddr);
    
    
    if(dnpChanRead(scanid, devid, &rxbuf[1], 1) == 0) return (0);
    opcode = rxbuf[1];
    rxcnt  = 2;
    
    /* ---------------------------------------- */
    /*  Exception Codes ....                    */
    /* ---------------------------------------- */
    if(opcode & 0x80)
    {
        reqSize = 3;    
        if((rcvSize = dnpChanRead(scanid, devid, &rxbuf[rxcnt], reqSize)) != reqSize)
        {
            if((opr->dnpDebug == devid) || (opr->scanDebug == scanid))
            Debug(console,"rcv%2d> 1. rcv Time out  ... req=%d, rcv=%d \n", devid+1, reqSize, rcvSize);
            return(0);
        }
        
        rxcnt = rcvSize + 2;
        crc = genCrc_modbus(rxbuf, rxcnt);
        
        if(crc != 0)
        {
            rcvCrc = rxbuf[rxcnt-2] + rxbuf[rxcnt-1]*256;
            if((opr->dnpDebug == devid) || (opr->scanDebug == scanid))
            {        
                Debug(console,"rcv%2d> *CRC Error... rcv=%4x, cal=%4x \n", devid+1, rcvCrc, crc);
                DumpBuff(console,"*CRC: ", rxbuf, rxcnt);
            }
            return (0);
        }
        
        /* ---------------------------------------- */
        /*  예외처리(EXP) 발생시 ... Offline 처리   */
        /* ---------------------------------------- */
        expCode = rxbuf[2];
        checkEXPcode(scanid, devid, expCode);
        
        return (0);
    }
    
    switch(opcode)
    {
    case MODBUS_RD_INPUT_BIT:
    case MODBUS_RD_INPUT_DIS:       // (0x02) Read Single-BIT            
    case MODBUS_RD_HOLD_REG:        // (0x03) Read Holding Registers
    case MODBUS_RD_INPUT_REG:       // (0x04) Read Input Registers  
        if(dnpChanRead(scanid, devid, &rxbuf[rxcnt++], 1) == 0) return (0);
        reqSize = rxbuf[2] + 2;
        sizeMode = 1;
        break;

    case MODBUS_WR_SINGLE_COIL:     // (0x05) Write Single Coil 
    case MODBUS_WR_SINGLE_REG:      // (0x06) Write Single Register
    case MODBUS_WR_MULTI_REG:       // (0x10) Write Single Register 
        reqSize = 4 + 2;
        sizeMode = 0;
        break;            

    default:
        if((opr->dnpDebug == devid) || (opr->scanDebug == scanid))   
        Debug(console,"rcv%2d> *Invalid Opcode ... rcv=%2x \n", devid+1, opcode);
        return (0);
        break;                    
    }

    pause(10);
    
    /* ------------------------------------ */
    /*  rcv BODY Frame...                   */
    /* ------------------------------------ */
    if((rcvSize = dnpChanRead(scanid, devid, &rxbuf[rxcnt], reqSize)) != reqSize)
    {
        /* OPCODE 에 따른 수신데이터 ... */
        if(sizeMode)    rxcnt = rcvSize + 3;        // include byte-count
        else            rxcnt = rcvSize + 2;        // exclude byte-count
        
        if((opr->dnpDebug == devid) || (opr->scanDebug == scanid)) 
        {
            Debug(console,"rcv%2d> 1. rcv Time out  ... req=%d, rcv=%d \n", devid+1,reqSize, rcvSize);
            DumpBuff(console,"*RXD: ", rxbuf, rxcnt);
        }
        return(0);
    }
    
    if(sizeMode)    rxcnt = rcvSize + 3;        // include byte-count
    else            rxcnt = rcvSize + 2;        // exclude byte-count
    crc = genCrc_modbus(rxbuf, rxcnt);
        
    if(crc != 0)
    {
        rcvCrc = rxbuf[rxcnt-2] + rxbuf[rxcnt-1]*256;
        if((opr->dnpDebug == devid) || (opr->scanDebug == scanid)) 
        {
            Debug(console,"rcv%2d> *CRC Error... rcv=%4x, cal=%4x \n", devid+1, rcvCrc, crc);
            DumpBuff(console,"*CRC: ", rxbuf, rxcnt);
        }
        return (0);
    }
            
    return (rxcnt);

}


/*
*/
int rcvHandlerMODBUS(int scanid, int devid, byte *rxbuf)
{
    //int 	count=0;
    int     i;
    int 	opcode;
    int 	stsCount;
    int 	dataLen;
    int     mode;
    int     point;
    //word    crc;
    //int		readIndex;
    char    buffer[128];
    
    DNP_DEVICE  *dnp;
    POINT_BUF   *ptBuf;
    LOCK_POINT_BUF  *locPoint;
    //MODBUS_READ	*readBlock, *timeBlock;

    dnp = (DNP_DEVICE *) dnpcfg[devid];    
    
    opcode = rxbuf[1];

    switch(opcode)
    {
    case MODBUS_RD_INPUT_BIT:       // (0x01) Read Single-BIT
    case MODBUS_RD_INPUT_DIS:       // (0x02) Read Single-BIT    
    case MODBUS_RD_HOLD_REG:        // (0x03) Read Holding Registers	
    case MODBUS_RD_INPUT_REG:       // (0x04) Read Input Registers  	
    		
        dataLen = rxbuf[2];

        /* -------------------------------------------- */
        /*  수신요구 데이터와 수신데이터 수 비교        */
        /* -------------------------------------------- */
        if((dnp->reqDataOpcode != opcode) || (dnp->reqDataCount != dataLen))
        {   
            if((opr->dnpDebug == devid) || (opr->scanDebug == scanid))
                Debug(console,"dev%2d> *modbus rcv Data Fail....Opcode=%02x/%02x, Data Req=%2d, Rcv=%2d \n", devid+1, dnp->reqDataOpcode, opcode, dnp->reqDataCount, dataLen); 
                
            dnp->modbusRcvFail++;    
            break;
        }
        
        /* -------------------------------------------- */
        /*  MODBUS 계전기 타입에 따른 데이터 처리       */
        /* -------------------------------------------- */
        if(dnp->defaultRead == SET)
        {
        	/* 상태포인트 수만큼 상태포인트 처리...이후 아날로그 */
        	stsCount = dnp->devDiPoint / 16;
        	stsCount = stsCount & 0x0e;         // 항상 짝수로 전달...
        
        	checkInfoMODBUS(devid, stsCount, &rxbuf[3], dataLen);        //GIPAM115
        }	
        else							
        {   // handle body
        	checkBlockMODBUS(scanid, devid, opcode, &rxbuf[3], dataLen);        
        }
        
        /* -------------------------------------------- */
        /*  계전기 통신상태 : 통신성공 처리...          */
        /* -------------------------------------------- */
        dnp->modbusRcvOK = SET;        
        break;                

    case MODBUS_WR_SINGLE_COIL:     // (0x05) Write Single Coil    
    case MODBUS_WR_SINGLE_REG:      // (0x06) Write Single Register     
        
        /* -------------------------------------------- */
        /* 제어포인트의 Control-Base Check...           */
        /* -------------------------------------------- */
        point   = dnp->cntPoint;
        ptBuf = (POINT_BUF *) &dnp->doPtBuf[point];
        
        /* -------------------------------------------- */
        /* 제어 Flag 상태에 따른 후속제어 ...           */
        /* -------------------------------------------- */
        if(dnp->selectReq == SET)  
        {
            dnp->selectReq = RESET;
            dnp->operateReq= SET;
            
            if(opr->soeDebug)
            Debug(console,"rcv%2d> rcv SINGLE(op=%02d) select OK ...point=%d\n", devid+1, opcode, dnp->cntPoint+1);
            
            /* 계전기 제어정보 : SELECT */
            logHistoryQue1( opr, hque, EVENT_MOD_SELECT, devid, dnp->cntPoint, dnp->cntTcf, 0);       // CONSOLE - 제어 이벤트  
            
            /* -------------------------------- */
            /* LOG File 저장                    */
            /* -------------------------------- */
            sprintf(buffer, "--> MODBUS> Select OK : Dev=%d, PT=%d, TCF[TRIP=0x80, CLOSE=0x40]= %2x... ", devid+1, dnp->cntPoint+1, dnp->cntTcf);
            LogFile_MPU (opr, EVENT_MOD_SELECT, buffer, strlen(buffer));      
        }
        else if(dnp->operateReq == SET) 
        {
            dnp->selectReq = RESET;
            dnp->operateReq= RESET;  

            if(opr->soeDebug)
            Debug(console,"rcv%2d> rcv SINGLE(op=%02d) operate OK ...point=%d\n", devid+1, opcode, dnp->cntPoint+1);
            
            /* 계전기 제어정보 : SELECT */
            logHistoryQue1( opr, hque, EVENT_MOD_OPERATE, devid, dnp->cntPoint, dnp->cntTcf, 0);       // CONSOLE - 제어 이벤트     
            
            /* -------------------------------- */
            /* LOG File 저장                    */
            /* -------------------------------- */
            sprintf(buffer, "--> MODBUS> Operate OK : Dev=%d, PT=%d, TCF[TRIP=0x80, CLOSE=0x40]= %2x... ", devid+1, dnp->cntPoint+1, dnp->cntTcf);
            LogFile_MPU (opr, EVENT_MOD_OPERATE, buffer, strlen(buffer));   

            /* ------------------------------------------------------------ */
            /*  계전기 전체 쇄정/해정 포인트인 경우...                      */
            /*  - 전체 쇄정/해정 성공시...개별 쇄정/해정 제어설정...        */
            /*  - 개별 쇄정/해정 성공시...개별 쇄정/해정 제어상태 Clear     */
            /* ------------------------------------------------------------ */
            if(ptBuf->ptType == DLOCKPT)
            {
                Debug(console,"==> Device(%2d) 전체 쇄정/해정... RESET OK ...point=%d ... TCF[해정=0x80, 쇄정=0x40]= %2x\n", devid+1, dnp->cntPoint+1, dnp->cntTcf);
               
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
                    Debug(console,"==> Device(%02d) 개별 쇄정/해정...point=%d ... TCF[해정=0x80, 쇄정=0x40]= %2x\n", devid+1, locPoint->devPt, locPoint->cntrState);
                    
                    mode = SET;
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
                        Debug(console,"==> Device(%02d) 개별 쇄정/해정...point=%d ... *CLEAR \n", devid+1, locPoint->devPt);
                    }
                }
            }
                
            dnp->cntPoint = 0;
            dnp->cntTcf   = 0;
        }
        dnp->faultResetReq = RESET;
        break;
        
        
    case MODBUS_WR_MULTI_REG:       // (0x10) Write Single Register 
        if(dnp->timeSyncReq)    
        {
            dnp->timeSyncReq = 0;
            if(opr->msgDebug == devid)
            Debug(console,"dev%2d> rcv Time-Sync ...OK !\n", devid+1);
        }
        
        break;

    default:
        if(opr->msgDebug == devid)
            Debug(console,"dev%2d> *modbus : invalid Opcode = %2x\n", devid+1, opcode);
        return (0);
        break;        
    }
    
    return (1);    
    
}


/* ------------------------------------------------------- */
/*  SCAN THREAD Start Routine ....             */
/* ------------------------------------------------------- */
void    *scan_modbus_thread(int  *arg)
{
    int     i,scanid;
    int		comPort;
    int     devid=0, devIndex;
    int     chanComFail;
    int     chanReset;
    int     txcnt;
    int     rxcnt, retVal;
    int     oldsec=0;
    int     reqTimeTick;
    int		taskDelay;
    int		rcvAddr;
    int     startDev, stopDev;
    int     timeout;
    int     lockCntrMode;
    int     vmeChanActive;
    byte    *rxbuf, *txbuf;
    char    buffer[128];
    //int    	timedelay;
    
    DNP_DEVICE      *dnp=NULL;
	SCAN_CONFIG     *scan;
	TTY_DESC        *scanPort=NULL;
	LOCK_POINT_BUF  *locPoint;
	//struct timeval         tv;
	
	/* -------------------------------------------- */
	/* THREAD Argment Assign ...                    */
	/* -------------------------------------------- */
	scanid = (int ) *arg;
	
    /* SCAN-TASK 별 모듈구성 정보 */ 
    scan = (SCAN_CONFIG *) scanCfg[scanid];
    comPort = scan->comPort;
    vmeChanActive = RESET;
    
    txbuf = (byte *) &scan->sndBuffer[0];
    rxbuf = (byte *) &scan->rcvBuffer[0];

    /* -------------------------------------------- */
    /* SCAN THREAD - MASTER : 통신관련 변수 초기화  */
    /* -------------------------------------------- */
    if(comPort < MAX_CPUPORT)
    {
        scanPort = (TTY_DESC *) &scanChan[comPort];
        
        /* COM1 통신속도 반영 */
        if(scan->comSpeed == SPEED_9600)        scanPort->baudRate = 9600;
        else if(scan->comSpeed == SPEED_19200)  scanPort->baudRate = 19200;
        else if(scan->comSpeed == SPEED_38400)  scanPort->baudRate = 38400;
        else if(scan->comSpeed == SPEED_57600)  scanPort->baudRate = 57600;
        else                                    scanPort->baudRate = 38400;
        
        PortInitial((TTY_DESC *) scanPort);
        
        startDev = 0;
        stopDev  = scan->devNum;
        
        for(i = startDev; i < stopDev; i++)
        {
            devid = scan->device[i] & MAX_MODULE_MASK;
            dnp = (DNP_DEVICE *) dnpcfg[devid];
            
            dnp->ttyPort = (TTY_DESC *) scanPort;
            dnpFlagClear(devid);
        }                
    }
    else if((comPort >= VME_COM1) && (comPort <= VME_COM8))           // VME-SIO 통신포트 #1
    {
        startDev = 0;
        stopDev  = scan->devNum;
        
        /* ------------------------ */
        /* SIO Board 를 사용함      */
        /* ------------------------ */
        opr->useSIOBoard = SET;
    
        for(i = startDev; i < stopDev; i++)
        {
            devid = scan->device[i] & MAX_MODULE_MASK;
            dnp = (DNP_DEVICE *) dnpcfg[devid];
            
            dnp->vmeChan  = (VME_CHAN_DCB *) vmeChanDCB[comPort - VME_COM1];
            dnp->ttyPort  = NULL;
            
            dnpFlagClear(devid);
            
            /* VMECHAN 프로토콜/통신속도 타입 지정 */
            vmeChanActive = SET;
            dnp->vmeChan->protocolType = SCAN_MODBUS;
            dnp->vmeChan->cfgSpeed     = scan->comSpeed;
            
            dnp->vmeChan->rxFront   = 0;
            dnp->vmeChan->rxRear    = 0;
        }     
        
        vmeScanInitial(scanid, scan->comPort - VME_COM1);           
    }            
    else
    {
	    while(termExec)
	    {
	        if(opr->wdtDebug)
	        Debug(console,">>> *Invalid SCAN[%d] COM-PORT  ...port=%d !\n", scanid+1, scan->comPort);
	        
	        opr->scanWDT[scanid] = 0;
	        pause(1000);
	    }        
    }

    taskDelay = scan->comDelay * 10;
    if(taskDelay < 10)		taskDelay = 10;
    if(taskDelay > 2000)	taskDelay = 2000;	

    if(scan->virtualMode == 1)
    {      
        printf(" %s SCAN[%02d] MODBUS PROCESS Activated ...Virtual (COM-PORT=%02d, COM-Addr = %d, Target-Addr = %d) !\n", TARGET_NAME, scanid+1, scan->comPort, scan->comAddr, scan->targetAddr);
    }
    else
    {
        printf(" %s SCAN[%02d] MODBUS PROCESS Activated ...(COM-PORT=%02d, Delay = %d ms, Offline Count=%d) !\n", TARGET_NAME, scanid+1, scan->comPort, taskDelay, scan->offCount);
        
    }
    
    sprintf(buffer, "%s SCAN[%02d] MODBUS PROCESS Activated ...(COM-PORT=%02d, Delay = %d ms, Offline Count=%d) !", TARGET_NAME, scanid+1, scan->comPort, taskDelay, scan->offCount);
    LogFile_MPU ( opr, NOT_DEFINED, buffer, strlen(buffer));
            
#if 1            
    printf("MOD-SCAN[%02d] device Num = %02d ... ", scanid, scan->devNum );
    for(i=0; i< scan->devNum; i++)  printf(" %2d", scan->device[i]);
    printf("\n");
#endif

    /* -------------------------------------------------------- */
    /* 등록된 Device 가 없는 경우                               */
    /* -------------------------------------------------------- */
    while((scan->devNum == 0) || (scan->active == 0))   
	{
	    if(termExec == 0)   break;

        if(opr->wdtDebug)	Debug(console,"SCAN%2d> *** No Assign DEVICE ...!\n", scanid+1);
        opr->scanWDT[scanid] = 0;
        opr->scanComSts[scanid] = 2;         // 미정의

        pause(1000);
	}

    scan->scaninit = 4;
	            
    devIndex    = 0;
    chanComFail = 0;
    chanReset   = 0;
    reqTimeTick = 150;


	while(termExec)
	{
	    /* ---------------------------------------- */
	    /*  계전기별 통신 지연                      */
	    /* ---------------------------------------- */
	    opr->scanWDT[scanid] = 0;
	    pause(taskDelay);

		/* ---------------------------------------------------- */
		/*	CPU 이중화 : LOCAL-SLAVE ... 대기모드 				*/
		/* ---------------------------------------------------- */
		if(opr->runMode == LOCAL_SLAVE)		
		{
			/* ---------------------------------------- */
			/*	SLAVE 모드에서 수신된 데이터를 처리함   */
			/* ---------------------------------------- */
			chanComFail = 0;
            chanReset   = 0;
            dnp->comFailTick = 0;
            
			opr->scanWDT[scanid] = 0;	
            /* ------------------------ */
            /* 모듈별 Time out 처리     */
            /* ------------------------ */
            if(vmeChanActive == RESET)
			{
	            for(timeout = 0; timeout < 20; timeout++)
    	        {
        	        usleep(2000);            
            	    if((rxcnt = rcvWithModbus( scanid, devid, rxbuf) ) > 0)
                	{
                		rcvAddr = rxbuf[0];
	                	if((opr->dnpDebug == devid) || (opr->scanDebug == scanid))	DumpBuff(console,"RXD: ", rxbuf, rxcnt);
    	            }
                
        	        pause(20);
            	}
            }			
			//if(vmeChanActive == RESET)	PortFlush((TTY_DESC *) scanPort);
			continue;
		}

				
        /* ---------------------------------------- */
        /* 계전기 DEVICE Comm Handling ...          */
        /* ---------------------------------------- */
        for(;;)
        {
            devid = scan->device[devIndex] & MAX_MODULE_MASK;
            dnp = (DNP_DEVICE *) dnpcfg[devid];
            
            if(++devIndex >= scan->devNum)
            {
                devIndex = 0;
                devid = scan->device[devIndex] & MAX_MODULE_MASK;
                    
                /* 계전기 초기데이터 수집 */
                if(scan->scaninit)
                {
                    if(--scan->scaninit == 0)  
                    {
                        Debug(console,"scan%2d> modbus module scan end ...!\n", scanid+1);
                    }        
                }
                
                if(opr->rtuinit)
                {
                    if(--opr->rtuinit == 0)  
                    {
                        Debug(console,"scan%2d> Module-Init scan end ...!\n", scanid);
                    }       
                }
                        
                opr->devScanCount[scanid]++;
                pause(50);
            }

            /* check DEVICE TYPE & SCAN Flag.... */
            devid = scan->device[devIndex] & MAX_MODULE_MASK;;
            dnp = (DNP_DEVICE *) dnpcfg[devid];
            
            if((dnp->type != 0) && (dnp->scan == SET))   break;
        }
        	    
        /* -------------------------------------------- */
        /* 제어명령시 우선적으로 제어대상 Device 고정   */
        /* - 제어어따른 상태감시로 일정시간 통신...     */
        /* -------------------------------------------- */
        if(opr->cntrFlag[scanid] == SET)
        {
            devid = opr->cntrDevId[scanid] & MAX_MODULE_MASK;
            dnp   = (DNP_DEVICE *) dnpcfg[devid];
            
            if(++opr->cntrComTick[scanid] > 10)
            {
                opr->cntrFlag[scanid]    = 0;
                opr->cntrComTick[scanid] = 0;
                dnp->selectReq           = RESET;
            }
        }
        
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
               
                /* 초기 제어정보 설정... */
                if((dnp->selectReq + dnp->operateReq) == 0)
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

        /* ---------------------------------------------------- */	
        /* VME Buffer Command Clear ...  						*/
        /* ---------------------------------------------------- */	
        if(vmeChanActive == SET)
        {    
            dnp->vmeChan->rxFront   = 0;
            dnp->vmeChan->rxRear    = 0;
        }
        
        /* ---------------------------------------- */
        /* MODBUS : 보호계전기 송수신 루틴...       */
        /* ---------------------------------------- */
        if((txcnt = makeCmdMODBUS( COM_RS485, scanid, devid, txbuf)) > 0)
        {
            dnpChanWrite( scanid, devid, txbuf, txcnt);
            
            if((opr->dnpDebug == devid) || (opr->scanDebug == scanid))   DumpBuff(console,"TXD: ", txbuf, txcnt);
            
            dnp->comSndCount++;

            reqTimeTick = 20;           // 수신지연 : 1000ms 정도 

                   
            /* ------------------------ */
            /* 모듈별 Time out 처리     */
            /* ------------------------ */
            for(timeout = 0; timeout < reqTimeTick; timeout++)
            {
                usleep(1000);            
                if((rxcnt = rcvWithModbus( scanid, devid, rxbuf) ) > 0)
                {
                    rcvAddr = rxbuf[0];
                    if((opr->dnpDebug == devid) || (opr->scanDebug == scanid))	DumpBuff(console,"RXD: ", rxbuf, rxcnt);

                    /* ------------------------------------ */
                    /* 수신패킷의 Address  Check...         */
                    /* ------------------------------------ */	
                    if(scan->virtualMode == 1)
                    {
                        if(rcvAddr != scan->comAddr)
                    	{
                    		if((opr->dnpDebug == devid) || (opr->scanDebug == scanid))	Debug(console,"dev%2d> *** rcv Invalid Addr...rcv = %d !\n", devid+1, rcvAddr);
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
                	                        
                    chanComFail = 0;
                    chanReset   = 0;
                    //opr->cpuChangeTick = 0;
                    opr->scanComSts[scanid] = 1;
                    
                    dnp->comRcvCount++;
                    dnp->comFailTick = 0;
                    
                    /* 정상적인 통신 패킷 처리...    */
                    retVal = rcvHandlerMODBUS(scanid, devid, rxbuf);
                    
                    if(retVal == 1)
                    {
                        if(dnp->online != SET)  
                        {
                            setOnline(devid);
                            opr->cpuChangeTick = 0;
                        }
                        
                        dnp->online = SET;
                    }
                        
                    break;
                }

            }

            /* -------------------------------------------- */
            /* 계전기별 통신이상에 따른 Offline 처리        */
            /* -------------------------------------------- */
            if(++dnp->comFailTick > scan->offCount)
            {
                if((opr->dnpDebug == devid) || (opr->scanDebug == scanid))
                Debug(console,"scan%02d> *MODBUS-Device(%d) Comm Fail..OffCount= %d : %d!\n", scanid+1, devid+1, scan->offCount, dnp->comFailTick);  
                
                dnp->comFailTick = 0;
                
                /* 계전기 Offline 처리 */
                if(dnp->online != RESET)  
                {
                    setOffline(devid);
                }

                dnp->online = RESET;
               
                if(vmeChanActive == SET)    
                {
                    vmeScanChanReset(devid);
                    dnp->vmeChan->protocolType = SCAN_MODBUS;
                    dnp->vmeChan->cfgSpeed     = scan->comSpeed;
                    dnp->vmeChan->cfgType      = COM_RS232;
                
                    vmeScanInitial(scanid, scan->comPort - VME_COM1);
                    
                    if((opr->dnpDebug == devid) || (opr->scanDebug == scanid))
                    Debug(console,"scan%02d> *Device(%d) Comm Fail..VME RESET \n", scanid+1, devid+1);   
                }
                
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
                        sprintf(buffer, "[MODBUS] 개별장치 통신절체...Device=%d, changeTick=%d ", devid+1, opr->cpuChangeTick);
                        LogFile_MPU (opr, EVENT_CHANGE_CPU, buffer, strlen(buffer));  
                        
                        pause(100);
                        
                        opr->cpuChangeTick++;
                        
                        if(opr->wdtDebug)
                        {   
                            Debug(console,"\n--------------------------------------------\n");
                            Debug(console,"scan %2d> %02d:%02d:%02d *** MODBUS Device scan Fail... CPU Change (%d)...!\n", scanid, rtc->hour, rtc->min, rtc->sec, opr->cpuChangeTick);
                            Debug(console,"--------------------------------------------\n");
                        }
                        
                        /* ---------------------- */
                        /* OFFLINE 이벤트 발생... */
                        /* ---------------------- */
                        dnp->online = 0;
                    }
                }    
            }  
        }
        
         
        if(oldsec == rtc->sec)  continue;
        oldsec = rtc->sec;

        /* -------------------------------------------- */
        /* 장치별 통신상태 Check : Device Com rate ...  */
        /* -------------------------------------------- */
        if(dnp->comSndCount >= 1000)
        {
            dnp->oldRcvCount = dnp->comRcvCount;
            dnp->comSndCount = 0;
            dnp->comRcvCount = 0;
        }
                
        /* ------------------------------------ */
        /* RS485 통신라인에 대한 재 초기화 ...  */ 
        /* ------------------------------------ */      
        if(++chanReset > CHANNEL_TIMEOUT_TICK)
        {
            chanReset = 0;
            opr->scanComSts[scanid] = 0;

            /* VMECHAN 프로토콜 타입 지정 */
            if(vmeChanActive == SET)    
            {
                vmeScanChanReset(devid);
                dnp->vmeChan->protocolType = SCAN_MODBUS;
                dnp->vmeChan->cfgSpeed     = scan->comSpeed;
                dnp->vmeChan->cfgType      = COM_RS232;
                
                vmeScanInitial(scanid, scan->comPort - VME_COM1);
            }
            else
            {
                PortClose((TTY_DESC *) scanPort);  pause(100);
                PortInitial((TTY_DESC *) scanPort); pause(100);
                
                dnp->ttyPort = (TTY_DESC *) scanPort;
            }
                
            if((opr->dnpDebug == devid) || (opr->scanDebug == scanid))
            {   
                Debug(console,"\n===================================================================\n"); 
                Debug(console,"scan02%d> *ALL-MODBUS Device(%d) Comm Fail...Reinitial...offCount=%d !\n", scanid+1, devid+1, scan->offCount);  
                Debug(console,"===================================================================\n"); 
            }
                
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
                    sprintf(buffer, "[MODBUS] 전체장치 통신절체...Device=%d, changeTick=%d ", devid+1, opr->cpuChangeTick);
                    LogFile_MPU (opr, EVENT_CHANGE_CPU, buffer, strlen(buffer)); 
                    
                    pause(100);
                    
                    opr->cpuChangeTick++;

                    if(opr->wdtDebug)
                    {    
                        Debug(console,"\n--------------------------------------------\n");
                        Debug(console,"scan %2d> %02d:%02d:%02d *** MODBUS Channel Com-Fail... CPU Change (%d)...!\n", scanid+1, rtc->hour, rtc->min, rtc->sec, opr->cpuChangeTick);
                        Debug(console,"--------------------------------------------\n");
                    }
                            
                    for(i=0; i< scan->devNum; i++)    
                    {
                        devid = scan->device[i] & MAX_MODULE_MASK;
                        dnp   = (DNP_DEVICE *) dnpcfg[devid];
                        dnpFlagClear(devid);
                        
                        /* OFFLINE 이벤트 발생... */
                        dnp->online = 0;
                    }
                }
            }    
        }
                        	    
	}/* while */

    return (0);
}
