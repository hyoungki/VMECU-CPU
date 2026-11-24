
#include	"localLib.h"
#include    "external.h"

extern  int	    threadCount;
extern	int     socketServerFd;         /* for Server */

static  byte    simRxbuf[4][4096];
static  byte    simTxbuf[4][4096];

static  int		threadActive[4];


/**
**
**/
int mkResponseSIM(int hostid, byte *txbuf, byte *rxbuf, int rxcnt)
{
    int     ioid, point,index;
    int		scanid, mode;
    int     startPt, ptNum;
    int     opcode;
    int     dataLen, dbSize;
    int     count, sndCount;
    int     cntrPoint, cntrType;
    int     stspoint, anapoint;
    char    buffer[128];
        
    byte    *bfptr;
    
    DNP_DEVICE  *dnp;         
    POINT_BUF   *relayPoint;
    POINT_BUF       *ptBuf;
    SCAN_CONFIG     *scan;

	DB_HOST_CONFIG		*dbHost;
    //DB_DEVICE_CONFIG	*dbDev;
    DB_MODBUS_BLOCK		*dbModbus;
    DB_SCAN_CONFIG      *dbScan;

    opcode = rxbuf[4] & 0x7f;
    
    dataLen = rxcnt - 6;
    
    /* -------------------- */
    /* 송신 Frame 구성      */
    /* -------------------- */
    txbuf[0] = 0x7e;            /* stx      */
    txbuf[1] = 0x7e;            /* stx      */
    txbuf[2] = 0;               /* size     */
    txbuf[3] = 0;               /* size     */
    txbuf[4] = opcode;          /* opcode + 1*/
    //txbuf[5] = opr->cuStatus;   /* status   */
    count = 5;

	switch(opcode)
	{
	case SIM_GPOLL: 	/* GPoll */
		if((opr->cpuMode == CPU_MASTER) && (linkCfg->dbDownFlag == SET))
		{
			txbuf[count++]  = 1;
    	    txbuf[count++]  = 1;
    	}
    	else
    	{
			txbuf[count++]  = 0;
    	    txbuf[count++]  = 0;    		
    	}	
        break;
        
	case SIM_DEV_STATUS:	/* DEVICE Status Read */
        if(opr->cpuMode == CPU_MASTER)  txbuf[count++] = opr->masterSts;
        else                            txbuf[count++] = opr->slaveSts;    

        /* HOST 운영정보 */            
        txbuf[count++] = hostDCB[0]->hostStatus;  
        txbuf[count++] = hostDCB[1]->hostStatus;   
        txbuf[count++] = hostDCB[2]->hostStatus;   
        txbuf[count++] = hostDCB[3]->hostStatus;   
        txbuf[count++] = hostDCB[4]->hostStatus;   
        
        /* 계전기 운영정보 */   
        for(ioid = 0; ioid < MAX_MODULE; ioid++)
        {
            dnp = (DNP_DEVICE *) dnpcfg[ioid];
            txbuf[count++]  = dnp->online; 
            txbuf[count++]  = (dnp->comSndCount >> 8) & 0xff;
            txbuf[count++]  = dnp->comSndCount & 0xff;
            txbuf[count++]  = (dnp->comRcvCount >> 8) & 0xff;
            txbuf[count++]  = dnp->comRcvCount & 0xff;            
        }        
        break;		
        
	case SIM_POINT_SCAN:    /* 계전기 POINT 정보 Scan */
        ioid = (rxbuf[5] - 1) & MAX_MODULE_MASK;
        dnp = (DNP_DEVICE *) dnpcfg[ioid];
        
        txbuf[count++] = ioid + 1;		// Module No
        txbuf[count++] = 0;				// point Num - MSB 
        txbuf[count++] = 0;				// point Num - LSB 
        stspoint = 0;	
        anapoint = 0;
        
        /* --------------------------------------- */
        /* 상태포인트 정보 : 포인트당 4바이트 표현 */
        /* --------------------------------------- */
        for(point=0; point< MAX_DEV_DI_POINT; point++)
        {
            relayPoint = (POINT_BUF *) &dnp->diPtBuf[point];    
            if(relayPoint->config == SET)
            {
            	txbuf[count++]  = ((point + 1) >> 8) & 0x7f;	// MSB - 상태포인트 지정 
            	txbuf[count++]  = (point + 1);	                // LSB - 상태포인트 
            	
                txbuf[count++]  = 0;
                txbuf[count++]  = 0;
                txbuf[count++]  = 0;
                txbuf[count++]  = relayPoint->status & 0xff;
                stspoint++;
                
                //if(opr->simDebug)
                //printf("dev%2d> STS point = %2d, data= %4.2f\n", ioid+1, point+1, relayPoint->status);
            }
        }
        
        /* --------------------------------------- */
        /* 계측포인트 정보 : 포인트당 4바이트 표현 */
        /* --------------------------------------- */
        for(point=0; point< MAX_DEV_AI_POINT; point++)
        {
            relayPoint = (POINT_BUF *) &dnp->aiPtBuf[point];
            if(relayPoint->config == SET)
            {
            	txbuf[count++]  = ((point + 1) >> 8) | 0x80;	    // MSB - 아날로그 포인트 지정 
            	txbuf[count++]  = (point + 1);	                // LSB - 아날로그 포인트 
            	
            	/* FLOAT 형으로 데이터 보고... */
            	bfptr = (byte *) &relayPoint->floatData;
            	
                txbuf[count++]  = (bfptr[0]) & 0xff;
                txbuf[count++]  = (bfptr[1]) & 0xff;
                txbuf[count++]  = (bfptr[2]) & 0xff;
                txbuf[count++]  = (bfptr[3]) & 0xff;
                anapoint++;
                
                if(opr->simDebug)
                Debug(console, "dev%2d> ANA point = %2d, data= %4.2f\n", ioid+1, point+1, relayPoint->floatData);
            }
        }
        
        /* 2 Byte 로 포인트 번호 표시 */
        txbuf[6] = ((stspoint + anapoint) >> 8) & 0xff ;			// point Num 
        txbuf[7] = (stspoint + anapoint) & 0xff ;				// point Num 
        
        if(opr->simDebug)
        Debug(console, "=>Data Report dev=%2d...sts(db=%2d/%2d), ana(db=%2d/%2d\n", ioid + 1, dnp->devDiPoint, stspoint, dnp->devAiPoint, anapoint);
        break;
        		
	case SIM_POINT_CNTR:    /* 계전기 POINT 제어 */
        ioid 	  = (rxbuf[5] - 1) & 0x3f;
        cntrPoint = (rxbuf[6] - 1) & 0xff;
        cntrType  = rxbuf[7];

		if((cntrType != TRIP_CONTROL) && (cntrType != CLOSE_CONTROL))
		{
			Debug(console, "dev%2d> *** Invalid Control... point=%2d, control=%2x\n", ioid+1, cntrPoint+1, cntrType);
			return 0;
		}	
			  
        /* ---------------------------------------- */
        /* SCAN Task 제어정보 Setting ...           */
        /* ---------------------------------------- */
        for(scanid=0, mode=0; scanid< MAX_SCAN_PORT; scanid++)
        {
            scan = (SCAN_CONFIG *) scanCfg[scanid];
            if(scan->devNum <= 0)   continue;
            
            for(index=0; index< scan->devNum; index++)
            {
                /* 제어대상 모듈을 search... */
                if(scan->device[index] == ioid)
                {
                    dnp = (DNP_DEVICE *) dnpcfg[ioid];
                    ptBuf = (POINT_BUF *) &dnp->doPtBuf[cntrPoint];
                    
                    dnp->cntPoint  = cntrPoint;
                    dnp->cntTcf    = cntrType;              /* TRIP: 0x80, CLOSE:0x40 */
                    dnp->cntrType = ptBuf->ptConfig;         // 제어속성 : Pulse, Latch 제어
                    dnp->selectReq = SET;
                    
                    logEvent1( opr, sque, EVENT_LOCAL_CNTR, ioid, cntrPoint, cntrType, opr->cpuMode);
                    logHistoryQue1( opr, hque, EVENT_LOCAL_CNTR, ioid, cntrPoint, cntrType, opr->cpuMode);        // 상태 이벤트
                    
                    /* -------------------------------- */
                    /* LOG File 저장                    */
                    /* -------------------------------- */
                    sprintf(buffer, "<-- SIM> Control INFO : Dev=%d, PT=%d, TCF[TRIP=0x80, CLOSE=0x40]= %2x... ", ioid+1, cntrPoint+1, cntrType);
                    LogFile_MPU (opr, EVENT_LOCAL_CNTR, buffer, strlen(buffer));    
                    
                    /* SCAN 구조체에 제어정보 Update... */
                    opr->cntrDevId[scanid]   = ioid;
                    opr->cntrComTick[scanid] = 0;
                    opr->cntrFlag[scanid]    = SET;
					
					mode = SET;
                    if(opr->simDebug)
                    Debug(console, "* Control OPR: dev=%d, point=%d, TCF[TRIP=0x80, CLOSE=0x40] =%2x...!\n", ioid+1, dnp->cntPoint+1, dnp->cntTcf);
                }                            
            }
        }
        
        if(mode == 0)
        {
        	Debug(console, "dev%2d> *** Invalid Control... point=%2d, control=%2x\n", ioid+1, cntrPoint+1, cntrType);
			return 0;
        }
        	
       	txbuf[count++] = rxbuf[5];		/* 제어 모듈 (1...32) */
       	txbuf[count++] = rxbuf[6];		/* 제어 포인트 (1...128) */
       	txbuf[count++] = rxbuf[7];		/* 제어 상태 (TRIP:0x80, CLOSE:0x40) */
        break;
        
	case SIM_TIME_DOWN:     /* Time Sync Down */
    	opr->year 	= rxbuf[5] + 2000;
    	opr->month 	= rxbuf[6];
    	opr->day 	= rxbuf[7];
    	opr->hour	= rxbuf[8];
    	opr->min	= rxbuf[9];
    	opr->sec	= rxbuf[10];
    	opr->milisec = 0;

    	opr->rtcUpdateFlag = SET;
    	opr->vmecuTimeSet   = SET;   // LINK 간 Time Set

		/* TIME-SYNC 관련 이벤트 생성 */    	
    	logEvent1(opr, sque, EVENT_SIM_TIMESET, 0, 0, 0, opr->cpuMode);  
    	logHistoryQue1(opr, hque, EVENT_SIM_TIMESET, 0, 0, 0, opr->cpuMode);  
    	
    	sprintf(buffer, "<-- SIM> Time SYNC ... rcv : %4d/%2d/%2d %02d:%02d:%02d ", opr->year, opr->month, opr->day, opr->hour, opr->min, opr->sec);
        LogFile_MPU (opr, EVENT_SIM_TIMESET, buffer, strlen(buffer));     
        
    	if(opr->simDebug)
    	Debug(console, "sim> TIME Sync Down : %4d/%2d/%2d %02d:%02d:%02d \n",
    		opr->year, opr->month, opr->day, opr->hour, opr->min, opr->sec);
        break;
        		
	case SIM_TIME_UP:       /* Time Sync Up */
        txbuf[count++]  = (rtc->year % 100);
        txbuf[count++]  = rtc->month;  
        txbuf[count++]  = rtc->day;  
        txbuf[count++]  = rtc->hour;  
        txbuf[count++]  = rtc->min;  
        txbuf[count++]  = rtc->sec;  
        break;    		

	case SIM_HOST_DOWN:     /* HOST Config Database Down */
		
		dbSize = (sizeof(DB_HOST_CONFIG)*MAX_DB_HOST) + (sizeof(DB_HOST_PARAMETER)*MAX_DB_HOST) + sizeof(DB_CU_CONFIG);
		dbHost = (DB_HOST_CONFIG *) &rtudb->hostCfg[0];
		
        if(dataLen >= dbSize)   dataLen = dbSize;
    	memcpy( (byte *) dbHost, &rxbuf[5], dataLen);
    	
    	/* 데이터베이스 저장 ... */
        if(dbFileWrite(rtudb, opr->cpuMode) < 0)
        {
            sprintf(buffer, "%s", "[WR] *DB Write Error... ");
            LogFile_MPU ( opr, NOT_DEFINED, buffer, strlen(buffer)); 
        }
            
        txbuf[count++]  = 0; 
        
        if(opr->simDebug)   Debug(console, "sim> HOST Config-DB Down ... rcv=%d\n", dataLen); 
        opr->hostDBDown = SET;
        
        
        linkCfg->sysDbDown = SET;
		
        break;
        		
	case SIM_HOST_UP:       /* HOST Config Database Up */
		dataLen = (sizeof(DB_HOST_CONFIG)*MAX_DB_HOST) + (sizeof(DB_HOST_PARAMETER)*MAX_DB_HOST) + sizeof(DB_CU_CONFIG);
		
        dbHost = (DB_HOST_CONFIG *) &rtudb->hostCfg[0];
        memcpy(&txbuf[count], (byte *) dbHost, dataLen);
        count = count + dataLen;    
        if(opr->simDebug)   Debug(console, "sim> HOST Config-DB Upload ...snd=%d\n", dataLen); 
        break;
        		
	case SIM_SCAN_DOWN:     /* SCAN Config Database Down */
		dbSize = (sizeof(DB_SCAN_CONFIG)* MAX_DB_SCAN_PORT) + (sizeof(DB_DEVICE_CONFIG)*MAX_DB_MODULE);
		
		dbScan = (DB_SCAN_CONFIG *) &rtudb->scanConfig[0];
		
        if(dataLen >= dbSize)   dataLen = dbSize;
        memcpy( (byte *) dbScan, &rxbuf[5], dataLen);
        
        /* 데이터베이스 저장 ... */
        if(dbFileWrite(rtudb, opr->cpuMode) < 0)
        {
            sprintf(buffer, "%s", "[WR] *DB Write Error... ");
            LogFile_MPU ( opr, NOT_DEFINED, buffer, strlen(buffer)); 
        }
        
        txbuf[count++]  = 0;   
        
        if(opr->simDebug)   Debug(console, "sim> SCAN Config-DB Down ... rcv=%d\n", dataLen);   
        opr->scanDBDown = SET;  
        
        linkCfg->deviceDbDown = SET;
		   
        break;
        		
	case SIM_SCAN_UP:       /* SCAN Config Database Up */
        dataLen = (sizeof(DB_SCAN_CONFIG)* MAX_DB_SCAN_PORT) + (sizeof(DB_DEVICE_CONFIG)*MAX_DB_MODULE);
        dbScan = (DB_SCAN_CONFIG *) &rtudb->scanConfig[0];
        memcpy(&txbuf[count], (byte *) dbScan, dataLen);
        count = count + dataLen;   
        
        if(opr->simDebug)   Debug(console, "sim> SCAN Config-DB Upload ... snd=%d\n", dataLen);   
        break;
        		
	case SIM_PORT_DOWN:     /* Port Config Database Down */
		if(dataLen >= 128)   dataLen = 128;
        memcpy(&rtudb->portdb1[0], &rxbuf[5], dataLen);  /* 32x2x3 port */
        
        /* 데이터베이스 저장 ... */
        if(dbFileWrite(rtudb, opr->cpuMode) < 0)
        {
            sprintf(buffer, "%s", "[WR] *DB Write Error... ");
            LogFile_MPU ( opr, NOT_DEFINED, buffer, strlen(buffer)); 
        }
        
        txbuf[count++]  = 0;    
        if(opr->simDebug)   Debug(console, "sim> Port DB Down ... rcv=%d\n", dataLen);   
        
        linkCfg->harrisDbDown = SET;
			
        break;
        
	case SIM_PORT_UP:       /* Port Config Database Up */
		dataLen = 128;
        memcpy(&txbuf[count], &rtudb->portdb1[0], dataLen);
        count = count + dataLen;     
        if(opr->simDebug)   Debug(console, "sim> Port DB Upload ...snd=%d\n",dataLen ); 
        break;

	case SIM_CHASSIS_DOWN:  /* Port Config Database Down */
        if(dataLen >= 257)   dataLen = 257;
        memcpy(&rtudb->chassisNum, &rxbuf[5], dataLen);
        
        /* 데이터베이스 저장 ... */
        if(dbFileWrite(rtudb, opr->cpuMode) < 0)
        {
            sprintf(buffer, "%s", "[WR] *DB Write Error... ");
            LogFile_MPU ( opr, NOT_DEFINED, buffer, strlen(buffer)); 
        }
        
        if(opr->simDebug)   Debug(console, "sim> Chassis db Down ... rcv=%d\n", dataLen);
        
        break;
        		 
	case SIM_CHASSIS_UP:    /* Port Config Database Up */
		dataLen = 256 + 1;
        memcpy(&txbuf[count], &rtudb->chassisNum, dataLen);
        count = count + dataLen; 
        if(opr->simDebug)   Debug(console, "sim> Chassis db Upload ... snd=%d\n", dataLen);
        break;        
        
	case SIM_POINT_DOWN:    /* Point Config Database Down */
		startPt = (rxbuf[6] + rxbuf[5] * 256) - 1;
        ptNum   = rxbuf[7] & 0x7f;
        if(ptNum > 16)  ptNum = 16;
        if(startPt < 0) startPt = 0;
        		
		if((startPt + ptNum) > MAX_DB_POINT)
		{
			Debug(console, "sim> Point DB Down : *** Invalid Point num = %d\n", startPt + ptNum);
			return 0;
		}	
			       
        bfptr   = (byte *) &rtudb->devPtConfig[startPt];
        memcpy( (byte *) bfptr, &rxbuf[8], ptNum * sizeof(DEVICE_PTCFG));
        
        /* 데이터베이스 저장 ... */
        if(dbFileWrite(rtudb, opr->cpuMode) < 0)
        {
            sprintf(buffer, "%s", "[WR] *DB Write Error... ");
            LogFile_MPU ( opr, NOT_DEFINED, buffer, strlen(buffer)); 
        }
        
        txbuf[count++]  = rxbuf[5]; 
        txbuf[count++]  = rxbuf[6]; 
        txbuf[count++]  = rxbuf[7];
         
        if(opr->simDebug)   Debug(console, "sim> Point Down : start=%d, ptnum=%d\n", startPt, ptNum);
        
        /* 포인트 전송종료 Check ... */
        if(rxbuf[7] & 0x80)
        {	
        	Debug(console, "sim> Point Down OK .... Point Initial...Start\n");	
        	opr->pointDBDown = SET;
        	
        	linkCfg->endFlag = 0x80;
        	
        	/* -------------------------------------------- */
            /*  이중화 구성시... 포인트 DB 전송...          */
            /* -------------------------------------------- */
            linkCfg->dbDownPoint  = startPt + 1;
            linkCfg->dbDownIndex  = ptNum;
            
            
            linkCfg->pointDbDown  = SET;
        }
        else
        {
        	linkCfg->endFlag = RESET;
        }	

            	
        break;
        
	case SIM_POINT_UP:      /* Point Config Database Up */
        startPt = (rxbuf[6] + rxbuf[5] * 256) - 1;
        ptNum   = rxbuf[7] & 0x7f;
        if(ptNum > 16)  ptNum = 16;
       	if(startPt < 0) startPt = 0;

        if((startPt + ptNum) > MAX_DB_POINT)
		{
			Debug(console, "sim> Point DB Upload : *** Invalid Point num = %d\n", startPt + ptNum);
			return 0;
		}	
		
        bfptr   = (byte *) &rtudb->devPtConfig[startPt];            
        
        txbuf[count++] = rxbuf[5];  /* startPT MSB */
        txbuf[count++] = rxbuf[6];  /* startPT LSB */
        txbuf[count++] = rxbuf[7];  /* Point Num */        
        
        memcpy(&txbuf[count], bfptr, ptNum * sizeof(DEVICE_PTCFG));
        count = count + (ptNum * sizeof(DEVICE_PTCFG));     
        
        if(opr->simDebug)   Debug(console, "sim> Point Upload : start=%d, ptnum=%d... size=%d\n", startPt, ptNum, sizeof(DEVICE_PTCFG));
        break;
        
	case SIM_MODBUS_DOWN:   /* MODBUS Profile Block Database Down */
		dbSize = sizeof(DB_MODBUS_BLOCK) * 8;
		
		dbModbus = (DB_MODBUS_BLOCK *) &rtudb->modbusBlock[0];
        if(dataLen >= dbSize)   dataLen = dbSize;
        	
        memcpy( (byte *) dbModbus, &rxbuf[5], dataLen);
        
        /* 데이터베이스 저장 ... */
        if(dbFileWrite(rtudb, opr->cpuMode) < 0)
        {
            sprintf(buffer, "%s", "[WR] *DB Write Error... ");
            LogFile_MPU ( opr, NOT_DEFINED, buffer, strlen(buffer)); 
        }
        
        txbuf[count++]  = 0;   
        
        if(opr->simDebug)   Debug(console, "sim> MODBUS Config-DB Down ... rcv=%d\n", dataLen);   
        opr->modbusDBDown = SET;  
        
        linkCfg->modbusDbDown = SET;
        break;
        		
	case SIM_MODBUS_UP:     /* MODBUS Profile Block Database Up */
        dataLen = sizeof(DB_MODBUS_BLOCK) * 8;

		dbModbus = (DB_MODBUS_BLOCK *) &rtudb->modbusBlock[0];
        memcpy(&txbuf[count], (byte *) dbModbus, dataLen);
        count = count + dataLen;   
        
        if(opr->simDebug)   Debug(console, "sim> MODBUS Config-DB Upload ... snd=%d\n", dataLen);   
        break;
        		
	default :
		Debug(console, "sim> *** Invalid Opcode = %2x\n", opcode);
		return (0);
	}

    sndCount = count + 1;
                     
    txbuf[2] = (sndCount >> 8) & 0xff;     /* Total size: MSB */
    txbuf[3] = sndCount & 0xff;            /* Total size: LSB */
    txbuf[count] = genlrc(txbuf, count);
    count++;
    
    return(count);
                    
}


/*****************************************
* handle response 
* 
* return :  1   get vaild response  
*  			0   TCP Connection closed
*           -1  Timeout
*           -2  select system error 
******************************************/
int  get_responseSIM(int hostid, int	hostSocketFd, byte	*rxbuf)
{
    int     result;
    //int     size;
    word    errCount;
    int		rxcnt;
    int     reqSize;
    byte    lrc;

    rxcnt = 0;
    errCount = 0;
    /* ---------------------------------------- */
    /*  READ MODBUS - Header                    */
    /* ---------------------------------------- */ 
    do{
		if((result =TKreadn(hostSocketFd, (char *) &rxbuf[0], 1, 100)) < 0)
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
        
        if(++errCount > 50)	return (-1);
       	usleep(100);
        
    }while(rxbuf[0] != 0x7E);

    /* TCP/IP Socket Error 발생시 ... */
	if(result == 0)	return 0;

    /* read STX, SIZE */
	if((result =TKreadn(hostSocketFd, (char *) &rxbuf[1], 3, 100)) < 0)	return (-1);
	
	if(rxbuf[1] != 0x7E)	return (-1);		    
    
    reqSize = (rxbuf[2]*256 + rxbuf[3]) - 4;   /* except stx, size */ 

    /* read BODY */
    if((result =TKreadn(hostSocketFd, (char *) &rxbuf[4], reqSize, 100)) != reqSize)	
    {
        if(opr->simDebug)
        Debug(console, "sim%02d> * Tail not Received... req=%d, rcv=%d\n", hostid+1, reqSize, result);     
        return (-1);
    }

    /* receive body */
    rxcnt = reqSize + 4;
    lrc = genlrc(rxbuf, rxcnt);
        
    if(lrc == 0)   
    {
        return (rxcnt);
    }
    else    
    {
        if(opr->simDebug)    
        {
            Debug(console, "sim> *LRC Error ...cal=%2x, rcv=%2x!\n", lrc, rxbuf[rxcnt-1]);
            DumpBuff(console,"*LRC: ", rxbuf, rxcnt);
        }                
        return(-1);
    }
    
}


/* ============================================================ */
/*	HOST 인터페이스용 THREAD...									*/
/* ============================================================ */
void	*simInfComm(void *data)
{
	int		simid, nextid;
	
	int		flag = 1, length = sizeof(struct sockaddr_in);
	int		socketID;
	struct 	sockaddr_in	clientAddr;
	char	address[64];
	int		port;
	int     sendFlag;
	int     comFailTick;
	int		oldsec=0;
	int     rxVal;
	char    buffer[128];
	
	
	byte	*rxbuf, *txbuf;
	int		rxcnt, txcnt=0;

	/* -------------------------------------------- */
	/*	THREAD 내부 초기화 							*/
	/* -------------------------------------------- */
	simid  = threadCount++ & 0x03;
	nextid = (simid + 1) & 0x03;      	// 다음 Thread 종료.
	threadActive[nextid] = RESET;
	
	rxbuf = (byte *) &simRxbuf[simid][0];
	txbuf = (byte *) &simTxbuf[simid][0];

	/* -------------------------------------------- */
    /*	클라이언트 정보를 전역 자료에 입력한다. 	*/
    /* -------------------------------------------- */
	memcpy(&socketID, data, sizeof(int)); 
	memcpy(&clientAddr, (char *)data+sizeof(int), length); 
	
	setsockopt(socketServerFd, SOL_SOCKET, SO_REUSEADDR, (char*)&flag, sizeof(int));
	setsockopt(socketServerFd, SOL_SOCKET, SO_KEEPALIVE, (char*)&flag, sizeof(int));
	
	port = ntohs(clientAddr.sin_port);
	sprintf(address, "%s", inet_ntoa(clientAddr.sin_addr));
   	
   	printf(" %s SIM-THREAD(%d) Activated ...%s...port=%d,  socket = %d !\n", TARGET_NAME, simid, address,port, socketID);
	
	sprintf(buffer, "%s SIM-THREAD(%d) Activated ...%s...port=%d,  socket = %d !", TARGET_NAME, simid, address,port, socketID);
    LogFile_MPU ( opr, NOT_DEFINED, buffer, strlen(buffer));
    
	threadActive[simid] = SET;
	comFailTick = 0;
    sendFlag 	= 0;
	
	while(threadActive[simid])
	{
		pause(20);

        /* ------------------------------------- */
	    /*  메세지 수신 & Response               */
	    /* ------------------------------------- */	        	        
   		if((rxcnt = get_responseSIM(simid, socketID, rxbuf)) > 0)
    	{
            comFailTick = 0;
	        
            /* ----------------------------------------- */
            /*  수신한 데이터에 대한 패켓 처리 ... */    
            /* ----------------------------------------- */
            if(opr->simDebug)    DumpBuff(console,"SIM-RX: ", rxbuf, rxcnt);
            if(opr->simOnline == 0)  
            {
                Debug(console, "[*] PC-Simulator ONLINE...!\n");
            }
            opr->simOnline = 1;
            	        
            txcnt = mkResponseSIM(simid, txbuf, rxbuf, rxcnt);
            sendFlag = 1;
	    }
	    else
	    {
    	    if(rxcnt == 0)
	        {
	        	if(opr->simDebug)
	            Debug(console, "sim%02d> *TCP/IP read ...0 Connection...Failed...!\n", simid);
	            threadActive[simid] = RESET;
	        }
	    }

        /* -------------------------------------------- */
        /* 전송메세지에 대한 출력                       */  
        /* -------------------------------------------- */     
        if(sendFlag == 1)
        {
            sendFlag = 0;            
            if(txcnt >= 5) 
            {
                /* -------------------------------- */
                /* Network Response...              */
                /* -------------------------------- */    
                if((rxVal = tkWriteTCP( socketID ,(byte *) txbuf, txcnt, 1000)) < 0 )
            	{
                    if(opr->simDebug)  Debug(console, "sim%02d> * *** net Send Error [%d %d]...socket = %d !\n", simid, txcnt,  rxVal, socketID);		
	            }
	                
                if(opr->simDebug) DumpBuff(console,"SIM-TX: ", txbuf, txcnt);

                if( rxVal != txcnt)
                {
                    if(opr->simDebug)  Debug(console, "sim%02d> * net Send Error [%d %d]...!\n", simid, txcnt,  rxVal);
                    //threadActive[simid] = RESET;
                }                
            }
        }
        	 
	    if(oldsec == rtc->sec)	continue;
        oldsec = rtc->sec;	        

		/* ------------------------------------- */
	    /*  CLIENT - Timeout Check...			 */
	    /* ------------------------------------- */	     
		if(++comFailTick > 30)
		{
			comFailTick = 0;
			
			if(opr->simDebug)	
			Debug(console, "sim%02d> *CLIENT.. No Response...reConnect!\n", simid);
	        threadActive[simid] = RESET;
		}	
	}

	printf("sim> *THREAD (%d) ......END...!\n", simid);

    close(socketID);
    
    return (0);
}

