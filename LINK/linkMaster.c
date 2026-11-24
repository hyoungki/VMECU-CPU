
#include	"localLib.h"
#include    "external.h"

extern  int     storeSOE_CU_LINK( int devid, int devPoint, int state, byte *buf);
extern  int     storeDEV_CU_LINK( int devid, int devPt, int state, byte *buf);

extern  int     storeAI_FLOAT_LINK(int devid, int point, float fdata, byte *lowdata);
extern  int     curStore_BIT_LINK( int devid, byte *stsbuf, int dataSize);
extern  int     checkControl_LINK(int cntrDevice, int cntrPoint, int cntrTCF, int cntrHost, int onTime, int offTime);

extern  void    setMaster();
extern  void    resetMaster();
extern  int     rcvWithLINK(TTY_DESC *tp, byte *rxbuf);
extern  int	    checkDualCPU(int mode);


extern	int     termExec;

extern  byte    bitcode[8];

extern	TTY_DESC 	linkPort;
extern	TASK_INFO	*taskPtr;

static  byte    linkRxbuf[4096];
static  byte    linkTxbuf[4096];


static  int     scanDevNo = 0;
static  int     linkDevNo = 0;

/*
* ========================================================================
*   MASTER-CPU 메세지 구성
* ========================================================================
*/
int	 makeLinkCmd( byte *sndbuf)
{
    int     i;
	int     count=0;
	int		sndCount;
	int		opcode;
 	int     dbSize;
 	int		startPoint, pointNum, reportPoint;
    int     dumpCount;
    int     point, index1=0, index2=0;
    
    byte    efront, erear;
    word    hfront, hrear;
    byte    front, rear, Qnum;
	byte	*bfptr;
	
	DB_HOST_CONFIG		*dbHost;
	DB_SCAN_CONFIG		*dbScan;
    DB_MODBUS_BLOCK		*dbModbus;
    DNP_DEVICE          *dnp;
    POINT_BUF           *aiPoint;
    POINT_BUF           *devPoint;
    POINT_BUF           *diPoint;
    
    struct timeval      cuTimeVal;
    struct tm           cTime;
    int                 milisec;
    
    opcode = 0;

	/* ---------------------------- */
	/*	OPCODE 처리 				*/
	/* ---------------------------- */
	if(opr->cpuChange)                  
    {
        opcode = SIM_CHANGE_CPU;
    } 
    // DB는 무조건 Master에서 변해야 되는구나...
	else if(linkCfg->timeSync)		opcode = SIM_TIME_DOWN;
	else if(linkCfg->chksumReq)		opcode = SIM_CHKSUM;	
	else if(linkCfg->sysDbDown)		opcode = SIM_HOST_DOWN;		
	else if(linkCfg->deviceDbDown)	opcode = SIM_SCAN_DOWN;	
	else if(linkCfg->harrisDbDown)	opcode = SIM_PORT_DOWN;	
	else if(linkCfg->modbusDbDown)	opcode = SIM_MODBUS_DOWN;	
	else if(linkCfg->pointDbDown)	opcode = SIM_POINT_DOWN;						
	//else							opcode = SIM_GPOLL;
	else
	{
	    /* ------------------------------------ */
	    /* 1) 이중화 CPU : 이벤트 정보 전송...  */
	    /* ------------------------------------ */
	    /* 	sque    = (SYSTEM_QUE *)  &shmPtr->localEvent; */
	    if(opr->dualCpuSts == SET) // 상대편 CPU 가 있는지 확인용 SYS_DUAL_CPU_BIT ( PEER_IN) 
	    {  
	       /* master/slav 구분이 없다는 것은 뭐지.. */
	        efront = sque->front & 0xff;
	        erear  = linkCfg->eventRear & 0xff;
	        // 나의 Event 보낸다.
	        if(efront != erear)     opcode = SIM_EVENT_DOWN;      // CU 이벤트 정보    


#if 0
	        hfront = hque->front & HISTORY_QUE_MASK;
	        hrear  = linkCfg->historyRear & HISTORY_QUE_MASK;
	        if(hfront != hrear)     opcode = SIM_HISTORY_DOWN;      // CU 이벤트 정보    
#endif
	            	        
	        /* ------------------------------------ */
	        /* 2) 이중화 CPU : SOE Cheak...         */
	        /* ------------------------------------ */
	        if(opr->runMode == LOCAL_MASTER)
	        {   
	        // event는 Master에서만 발생하니까..
	        	/* scan에서 발생한 SOE 정보 */ 
	            front = linkCfg->front & 0xff;
    	        rear  = linkCfg->rear & 0xff;
	       
	            if(front != rear)
	            {
	                opcode = SIM_SOE_DUMP;      // 계전기 SOE 정보    
	            }        
	        }   
	    }
	      
        /* ------------------------------------ */
	    /* 2) 이중화 CPU : SOE 가 없는경우 ..   */
	    /* ------------------------------------ */
        if((opcode != SIM_EVENT_DOWN) && (opcode != SIM_HISTORY_DOWN) && (opcode != SIM_SOE_DUMP))
        {      	    
    	    /* ------------------------------------ */
	        /* 계전기 상태/아날로그 정보 연계       */
	        /* ------------------------------------ */
            for(i=0 ; i < MAX_MODULE; i++)
            {
                ++scanDevNo;
                if(scanDevNo >= MAX_MODULE) 
                {
                    scanDevNo = 0; // 한 바뀌 돌고나서 status 와 analog 변경
                    linkCfg->chgOpcode = (linkCfg->chgOpcode + 1) & 0x01;
                }
                
                if(scanDevNo == 0)      
                {
                    opcode = SIM_DEVICE_INFO;       // 계전기 통신상태...
                    break;
                }
                else
                {   
                    linkDevNo = (scanDevNo - 1) & MAX_MODULE_MASK;      
                    if(linkCfg->chgOpcode)  opcode = SIM_POINT_STATUS;  // 계전기 상태포인트 정보 
                    else                    opcode = SIM_POINT_ANALOG;  // 계전기 계측포인트 정보 
                
                    dnp = (DNP_DEVICE *) dnpcfg[linkDevNo];
                    if(dnp->scan)   break;    
                }
            }
	    }            
	}    

	/* ---------------------------- */
    /* 	CPU 이중화 - 제어정보       */
    /* ---------------------------- */
    if((opr->runMode == LOCAL_SLAVE) && (linkCfg->cntFlag == SET))  
    {
        /* 연속제어 실패시... */
        if(++linkCfg->cntSndTick > 5)
        {
            linkCfg->cntDev     = 0;
            linkCfg->cntPoint   = 0;
            linkCfg->cntTCF     = 0;
            linkCfg->cntFlag    = 0;
            linkCfg->cntHost    = 0;
            linkCfg->cntSndTick = 0;
            Debug(console,"master> snd CONTROL-Info ... Cleared ! \n");
            return (0);
        }
        else
        {
            opcode = SIM_CONTROL_INFO;
        }
    }
    
    /* 여기부터는 OPCODE에 따른 body 구성 */
    
    
	/* ---------------------------- */
    /* 	송신 Frame 구성      		*/
    /* ---------------------------- */
    sndbuf[0] = 0x7e;            /* stx      */
    sndbuf[1] = 0x7e;            /* stx      */
    sndbuf[2] = 0;               /* size     */
    sndbuf[3] = 0;               /* size     */
    
    /* ---------------------------- */
    /* MASTER 상태 표시             */
    /* ---------------------------- */
    if(opr->runMode == LOCAL_MASTER)    sndbuf[4] = opcode | 0x80;      /* MASTER 상태 표기 */
    else                                sndbuf[4] = opcode;             /* opcode + 1*/    
    
    sndbuf[5] = linkCfg->runStatus;         // CU 동작상태
    sndbuf[6] = linkCfg->sndSeqno;          // CU 통신상태 Seqno
    
    count = 7;

	switch(opcode)
	{
    case SIM_CHANGE_CPU:
        sndbuf[count++] = 0;
        sndbuf[count++] = 0;
        printf("master> snd CPU-Change... A\n");
        break;
    
    case SIM_DEVICE_INFO:                       // 0x38,  계전기 제어정보 연계
        if(opr->runMode == LOCAL_MASTER)
        {   
            for(i=0; i < MAX_DEV_POINT; i++)
            {  
                devPoint = (POINT_BUF *) devPtBuf[i];       // Device Point 정보 : 개별 계전기, 1,2 ~ 64 
                sndbuf[count++] = devPoint->status;         // Device Point 상태정보
            }
        }
        else
        {
        }     
        break;
        	    
    case SIM_CONTROL_INFO:                      // 0x36, 계전기 제어정보 연계 
        /* ---------------------------------------- */
        /*  CPU 이중화가 아닌경우...                */
        /* ---------------------------------------- */
        if((opr->dualCpuSts == SET) && (opr->runMode == LOCAL_SLAVE))
        {
            sndbuf[7]  = linkCfg->cntDev;
            sndbuf[8]  = linkCfg->cntPoint;
            sndbuf[9]  = linkCfg->cntTCF;
            sndbuf[10] = linkCfg->cntFlag;
            sndbuf[11] = linkCfg->cntHost;
            count = 12;
        }
        else        
        {
            sndbuf[7] = 0;       
            sndbuf[8] = 0;    
            sndbuf[9] = 0;       
            sndbuf[10]= 0;   
            sndbuf[11]= 0;           
            count = 12;
        }
        break;

    case SIM_EVENT_DOWN:                        // 0x03, CU 이벤트 연계
        efront = sque->front & 0xff;
        erear  = linkCfg->eventRear & 0xff;
        Qnum = (efront - erear + 256) % 256;  // ( front -rear+1 + max ) && 0xff)
        
        if(Qnum)
        {
            if(Qnum > 32)   Qnum = 32;
                        
            sndbuf[7] = Qnum;
            count = 8; // sndbuf의 위치 같은데...
                    
            for(i=0; i< Qnum; i++)
            {
                erear  = linkCfg->eventRear & 0xff;
                
                /* SOE 이벤트 내용을 Copy */
                memcpy( &sndbuf[count], (byte *) &sque->queue[erear], sizeof(SYSLOG_FORM));
                count += sizeof(SYSLOG_FORM);
                        
                linkCfg->eventRear  = (linkCfg->eventRear + 1) & 0xff;
            }
            erear  = linkCfg->eventRear & 0xff;
                
            if(opr->soeDebug)
            Debug(console,"master> LINK-Event Q send... front = %d, rear = %d  >> Qnum=%d \n", efront, erear, Qnum);
        }
        else
        {
            sndbuf[7] = 0;
            count = 8;
        }      
        
        break;
        
    case SIM_HISTORY_DOWN:                        // 0x06, CU 이벤트 연계
        hfront = hque->front & HISTORY_QUE_MASK;
        hrear  = linkCfg->historyRear & HISTORY_QUE_MASK;
        Qnum = (hfront - hrear + HISTORY_QUE_MAX) % HISTORY_QUE_MAX;
        
        if(Qnum)
        {
            if(Qnum > 32)   Qnum = 32;
                        
            sndbuf[7] = Qnum;
            count = 8;
                    
            for(i=0; i< Qnum; i++)
            {
                hrear  = linkCfg->historyRear & HISTORY_QUE_MASK;
                
                /* SOE 이벤트 내용을 Copy */
                memcpy( &sndbuf[count], (byte *) &hque->queue[hrear], sizeof(SYSLOG_FORM));
                count += sizeof(SYSLOG_FORM);
                        
                linkCfg->historyRear  = (linkCfg->historyRear + 1) & HISTORY_QUE_MASK;
            }
                
            if(opr->soeDebug)
            Debug(console,"master> LINK-History Q send... front = %d, rear = %d  >> Qnum=%d \n", hfront, hrear, Qnum);
        }
        else
        {
            sndbuf[7] = 0;
            count = 8;
        }      
        
        break;
                
                	    
    case SIM_SOE_DUMP:                          // 0x34, 계전기 SOE 정보 Update 
        /* ---------------------------------------- */
        /*  CPU 이중화가 아닌경우...                */
        /* ---------------------------------------- */
        if((opr->dualCpuSts == SET) && (opr->runMode == LOCAL_MASTER))
        {
            front = linkCfg->front & 0xff;
	        rear  = linkCfg->rear & 0xff;
	        
	        Qnum = (front - rear + 256) % 256;
	        
	        if(Qnum)
            {
                if(Qnum > 16)   Qnum = 16;
                        
                sndbuf[7] = Qnum;
                count = 8;
                    
                for(i=0; i< Qnum; i++)
                {
                    /* SOE 이벤트 내용을 Copy */
                    memcpy( &sndbuf[count], (byte *) &linkCfg->linkSOE[rear], sizeof(LINK_SOE));
                    count += sizeof(LINK_SOE);
                        
                    rear  = (rear + 1) & 0xff;
                }
                
                linkCfg->report = Qnum;
                    
                if(opr->soeDebug)
                Debug(console,"master> LINK-SOE Queue-send... front = %d, rear = %d  >> Qnum=%d \n", front, rear, Qnum);
            }
            else         
            {
                sndbuf[7] = 0;      // soe-Q 갯수
                count = 8;
            }
        }
        else         
        {
            sndbuf[7] = 0;      // soe-Q 갯수
            count = 8;
        }
             
        break;
        
    case SIM_POINT_ANALOG:                          // 0x32, 계전기 계측포인트 POINT 정보 Scan
        /* ---------------------------------------- */
        /*  CPU 이중화가 아닌경우...                */
        /* ---------------------------------------- */
        if((opr->dualCpuSts == SET) && (opr->runMode == LOCAL_MASTER))
        {
            dnp = (DNP_DEVICE *) dnpcfg[linkDevNo];
            sndbuf[7] = linkDevNo;          // 계전기 번호     
            sndbuf[8] = 0;                  // Dump 포인트 수      
            count = 9;
            
            /* ------------------------------------ */
       	    /* 2014.08.12 계측 포인트가 없는 경우.  */
       	    /* ------------------------------------ */
       	    if(dnp->devAiPoint <= 0)    break;
        	        
            /* -------------------------------- */
            /* 계전기별 아날로그(Float) 전송    */
            /* -------------------------------- */
            dumpCount = 0;
            for(i=0; i < MAX_DEV_AI_POINT; i++)
            {
                aiPoint = (POINT_BUF *) &dnp->aiPtBuf[i];
                if(aiPoint->config == 0)    continue;
                
                sndbuf[count++] = (i >> 8) & 0xff;      // 포인트 번호
                sndbuf[count++] = i & 0xff;             // 포인트 번호
                memcpy( &sndbuf[count], &aiPoint->floatData, 4);
                count += 4;
                
                dumpCount++;
                
                /* 보고되는 아날로그 데이터 제한 */
                if(dumpCount > 128)  break;    
            }
            sndbuf[8] = dumpCount;                  // Dump 포인트 수     
        }
        else      
        {
            sndbuf[7] = linkDevNo;
            sndbuf[8] = 0;          
            count = 9;
        }
        
        break;
        	    
    case SIM_POINT_STATUS:                          // 0x30, 계전기 상태포인트 POINT 정보 Scan
        /* ---------------------------------------- */
        /*  CPU 이중화가 아닌경우...                */
        /* ---------------------------------------- */
        if((opr->dualCpuSts == SET) && (opr->runMode == LOCAL_MASTER))
        {
            dnp = (DNP_DEVICE *) dnpcfg[linkDevNo];
            
            sndbuf[7] = linkDevNo;          // 계전기 번호     
    	    sndbuf[8] = 0;                  // 데이터 사이즈 (Byte 수)     
        	count = 9;
            
       	    /* ------------------------------------ */
       	    /* 2014.08.12 상태 포인트가 없는 경우.  */
       	    /* ------------------------------------ */
       	    if(dnp->devDiPoint <= 0)    break;
        	        
       	    bfptr = (byte *) &sndbuf[9];
            bzero((byte *) &bfptr[0], (dnp->devDiPoint / 8) + 1);
                
      	    /* ------------------------------------ */
            /* DEVICE 별 포인트 정보 Update...      */
            /* ------------------------------------ */
            for(point = 0; point < dnp->devDiPoint; point++)
            {
                diPoint = (POINT_BUF *) &dnp->diPtBuf[point];
                    
                index1 = point / 8;
                index2 = point % 8;
                    
                /* ------------------------------------ */
                /*  포인트 타입 : 반전 포인트인 경우... */
                /* ------------------------------------ */
                if(diPoint->status == 1)    bfptr[index1] |= bitcode[index2];  
            }
                
            if(index2)  
            {
                sndbuf[8] = index1 + 1;     // 데이터 사이즈 (Byte 수)     
                count = count + index1 + 1;
            }
            else
            {  
                sndbuf[8] = index1;         // 데이터 사이즈 (Byte 수)     
                count = count + index1;      
            }       

#if 0
            /* ---------------------------------------- */
            /*  계전기 Online : 수신덴 상태데이터 전송  */
            /*         Offline: 계전기 버퍼내용 전송    */
            /* ---------------------------------------- */
            if((dnp->online == SET) && (dnp->initial >= STATUS_DUMP_TICK))
            {	
	            sndbuf[7] = linkDevNo;          // 계전기 번호     
    	        sndbuf[8] = dnp->stsByteCnt;    // 데이터 사이즈 (Byte 수)     
        	    count = 9;
            
                /* -------------------------------- */
                /* 계전기별 상태정보(BIT) 전송      */
                /* -------------------------------- */
            	memcpy( &sndbuf[count], &dnp->linkSts[0], dnp->stsByteCnt);
            	count = count + dnp->stsByteCnt;
            }
            else
            {
            	sndbuf[7] = linkDevNo;          // 계전기 번호     
    	        sndbuf[8] = 0;                  // 데이터 사이즈 (Byte 수)     
        	    count = 9;
            }	            
#endif
            
        }
        else
        {
            sndbuf[7] = linkDevNo;
            sndbuf[8] = 0;          
            count = 9;
        }
                           
        break;
        	    
	case SIM_CHKSUM:                                    // 0x01, CPU Database Chksum Dump
		//Debug(console,"master> snd chksum Check ....\n");
        /* MASTER-CPU 의 HOST 통신상태정보 전송 ... */
        sndbuf[count++] = opr->mmiHostTemp[0];
        sndbuf[count++] = opr->mmiHostTemp[1];
        sndbuf[count++] = opr->mmiHostTemp[2];
        sndbuf[count++] = opr->mmiHostTemp[3];
        sndbuf[count++] = opr->mmiHostTemp[4];
	
		break;

	case SIM_TIME_DOWN:                             // 0x10,  Time Sync Down
        /* CU 시각정보 추출...  */
        gettimeofday(&cuTimeVal, NULL);
        memcpy( &sndbuf[count], (byte *) &cuTimeVal, sizeof(struct timeval));
        count = count + sizeof(struct timeval);
        
        localtime_r (&cuTimeVal.tv_sec, &cTime);
        milisec = cuTimeVal.tv_usec / 1000;
        
        if(opr->linkDebug)
        Debug(console, "master> TIME Sync Down : %4d/%2d/%2d %02d:%02d:%02d-%03d \n", cTime.tm_year + 1900, cTime.tm_mon +1, cTime.tm_mday, cTime.tm_hour, cTime.tm_min, cTime.tm_sec, milisec);
            	
        break;
        
	case SIM_GPOLL:                         // 0x70
	    sndbuf[count++] = 0;
	    sndbuf[count++] = 0;  
		break;		

	case SIM_HOST_DOWN:                             // 0x50        HOST Config Database Down 
		dbSize = (sizeof(DB_HOST_CONFIG)*MAX_DB_HOST) + (sizeof(DB_HOST_PARAMETER)*MAX_DB_HOST) + sizeof(DB_CU_CONFIG);
		dbHost = (DB_HOST_CONFIG *) &rtudb->hostCfg[0];
		memcpy( &sndbuf[count], (byte *) dbHost,  dbSize);
		count += dbSize;
		
		//if(opr->linkDebug)
		Debug(console,"master> snd HOST-DB ... size = %d\n", dbSize);
		break;
				
	case SIM_SCAN_DOWN:                             // 0x52,   SCAN Config Database Down 
		dbSize = (sizeof(DB_SCAN_CONFIG)* MAX_DB_SCAN_PORT) + (sizeof(DB_DEVICE_CONFIG)*MAX_DB_MODULE);
		dbScan = (DB_SCAN_CONFIG *) &rtudb->scanConfig[0];
		
		memcpy( &sndbuf[count], (byte *) dbScan,  dbSize);
		count += dbSize;
		
		//if(opr->linkDebug)
		Debug(console,"master> snd SCAN-DB ... size = %d\n", dbSize);
		break;

	case SIM_PORT_DOWN:                             // 0x54, Port Config Database Down 
		dbSize = 128 + 1 + 256;
		memcpy( &sndbuf[count], (byte *) &rtudb->portdb1[0],  dbSize);
		count += dbSize;
		//if(opr->linkDebug)
		Debug(console,"master> snd HARRIS & LANDIS-DB ... size = %d\n", dbSize);
        break;

	case SIM_MODBUS_DOWN:                           // 0x60, MODBUS Profile Block Database Down
		dbSize = sizeof(DB_MODBUS_BLOCK) * 8;
		
		dbModbus = (DB_MODBUS_BLOCK *) &rtudb->modbusBlock[0];
        memcpy( &sndbuf[count], (byte *) dbModbus, dbSize);
        count += dbSize;
        //if(opr->linkDebug)
        Debug(console,"master> snd MODBUS-DB ... size = %d\n", dbSize);
		break;

	case SIM_POINT_DOWN:                            // 0x58, Point Config Database Down
		pointNum    = 16;
		startPoint  = linkCfg->dbDownPoint;
		reportPoint = startPoint + pointNum;
		
		/* ------------------------------------ */
		/* 실제 등록된 포인트 Check...          */
		/* ------------------------------------ */
		if(reportPoint >= MAX_DB_POINT)
		{
			pointNum = 	MAX_DB_POINT - linkCfg->dbDownPoint;
			linkCfg->endFlag  = 0x80;	// point update flag...
		}	

		linkCfg->dbDownIndex = pointNum;
			
		sndbuf[count++] = (startPoint >> 8) & 0xff;
		sndbuf[count++] = startPoint & 0xff;
		sndbuf[count++] = pointNum | linkCfg->endFlag;
		
		bfptr   = (byte *) &rtudb->devPtConfig[startPoint - 1];
        dbSize  = pointNum * sizeof(DEVICE_PTCFG);
        
        //if(opr->linkDebug)
        Debug(console,"master> snd POINT-DB ...start=%d, num=%d, size = %d\n", startPoint, pointNum, dbSize);
        
        memcpy( &sndbuf[count], (byte *) bfptr, dbSize);
        count += dbSize;
		break;
		
	default :
		break;	
	}
	
    sndCount = count + 1;
                     
    sndbuf[2] = (sndCount >> 8) & 0xff;     /* Total size: MSB */
    sndbuf[3] = sndCount & 0xff;            /* Total size: LSB */
    sndbuf[count] = genlrc(sndbuf, count);
    count++;
    
    return(count);
        	
}


/**
**
**/
int rcvHandler_master(byte *rxbuf, int rxcnt, byte *txbuf)
{
    int     i, index;
    int     dnpPoint, hostid;
    int     Qnum;
    int     opcode, eCode;
    int     dataLen;
    int     count=0, sndCount, dumpCount;
    int     devid, devPt, state;
    int     startPoint;
    int     cntrDevice, cntrPoint, cntrTCF, cntrHost, cntrFlag, onTime, offTime;
    int     stsByteCnt, anapoint;
    float   *fptr;
    float   fdata;
    
    char    buffer[128];
    
    word	chksum, romsum;
    
    word    hfront;
    byte    efront;
    byte    *bfptr;
    
    DNP_DEVICE      *dnp;    
    LINK_SOE        *linkSoe;
    POINT_BUF       *devPoint;
    SYSLOG_FORM     *rcvEvent;
    HOST_DCB        *host;
    
    struct timeval      cuTimeVal;
    struct tm           cTime;
    int                 milisec;
    
    /* MASTER-Bit Mask... */
    opcode = rxbuf[4] & 0x7f;
    linkCfg->rcvStatus = rxbuf[5];
    linkCfg->rcvSeqno  = rxbuf[6];
    
    dataLen = rxcnt - 8;                // except ... STX, size, opcode, status, seq...LRC */
    
    /* -------------------- */
    /* 송신 Frame 구성      */
    /* -------------------- */
    txbuf[0] = 0x7e;            /* stx      */
    txbuf[1] = 0x7e;            /* stx      */
    txbuf[2] = 0;               /* size     */
    txbuf[3] = 0;               /* size     */
    
    /* ---------------------------- */
    /* MASTER 상태 표시             */
    /* ---------------------------- */
    if(opr->runMode == LOCAL_MASTER)    txbuf[4] = opcode | 0x80;      /* MASTER 상태 표기 */
    else                                txbuf[4] = opcode;             /* opcode + 1*/    
    
    txbuf[5] = linkCfg->runStatus;          // CU 동작상태
    txbuf[6] = linkCfg->rcvSeqno;           // 수신 seqno 
    count = 7;
    
	switch(opcode)
	{
    case SIM_CHANGE_ACK: // 어디서 쓰나..못 찾겠다. 꽤꼬리..
        opr->cpuChange = 0;
        
        printf("master> rcv CPU-CHANGE....ACK \n");
        
        if(opr->runMode == LOCAL_MASTER)    resetMaster();
        else                                setMaster();
        return (0);
        break;

    case SIM_CHANGE_CPU:
        
        printf("master> rcv CPU-CHANGE....\n");
        opr->cpuChange = 0;
        
        if(opr->runMode == LOCAL_MASTER)    resetMaster();
        else                                setMaster();
        return (0);

        break;
                	    
    case SIM_DEVICE_INFO:                       // 0x38,  계전기 제어정보 연계    
        if(opr->runMode == LOCAL_SLAVE)
        {   
            /* -------------------------------- */
            /* 계전지별 Online 상태정보 Update  */
            /* -------------------------------- */
            for(i=0; i < dataLen; i++)
            {
                devPoint = (POINT_BUF *) devPtBuf[i];       // Device Point 정보 : 개별 계전기, 1,2 ~ 64 
                devPoint->status = rxbuf[7 + i];            // Device Point 상태정보  
                devPoint->oldsts = rxbuf[7 + i];            // Device Point 상태정보  

                /* 전체 계전기 통신상태 */                                        
                if(i == 0)
                {
                    if(devPoint->status == 0)   
                    {
                    	for(hostid = 0; hostid < MAX_HOST; hostid++)	opr->allDevOnline[hostid] = SET;
                    }
                    else                        
                    {
                    	for(hostid = 0; hostid < MAX_HOST; hostid++)	opr->allDevOnline[hostid] = RESET;    
                    }
                }
                        
                /* 계전기별 통신상태 Update... */                    
                if((i >0 ) && (i < MAX_MODULE))
                {
                    dnp = (DNP_DEVICE *) dnpcfg[i-1];
                    
                    if(devPoint->status == 0)   dnp->online = 1;        // 계전기 통신정상
                    else                        dnp->online = 0;        // 계전기 통신이상 
                }
                
                /* --------------------------------------------- */
                /* DNP/IEC HOST : 계전기 통신상태 포인트 Update .*/
                /* --------------------------------------------- */
                for(hostid=0; hostid < MAX_HOST; hostid++)
                {
                    host = (HOST_DCB *) hostDCB[hostid];
                    if(host->hostDualMode == HOST_NOT_USE)	continue;
        
                    dnpPoint = devPoint->hostIndex[hostid] - 1;             // SDP POINT : 상위 호스트#1 Index 번호 [1..1024]
                    if((dnpPoint < 0) || (dnpPoint >= MAX_DNP_DI_POINT))    continue;         

                    /* DNP HOST - 0: 정상, 1: 이상 */
                    if(devPoint->status == 1)   
                    {
                        host->sts_pointData[dnpPoint] = PT_FLAG_STS_ON  | PT_FLAG_ONLINE;       // [1] : 통신이상, Offline
                    }
                    else
                    {
                        host->sts_pointData[dnpPoint] = PT_FLAG_STS_OFF | PT_FLAG_ONLINE;       // [0] : 통신정상, Online     
                    }
                }
            }
        }
        else
        {
            
        }     
        return (0);
        break;	    

    case SIM_CONTROL_ACK:
        linkCfg->cntDev     = 0;
        linkCfg->cntPoint   = 0;
        linkCfg->cntTCF     = 0;
        linkCfg->cntFlag    = 0;
        linkCfg->cntHost    = 0;
        linkCfg->cntSndTick = 0;
        if(opr->soeDebug)
        Debug(console,"master> rcv Control ACK \n");
        return (0);
        break;
                   
    case SIM_CONTROL_INFO:                      // 0x36, 계전기 제어정보 연계     
        if(opr->runMode == LOCAL_SLAVE)
        {
            linkCfg->cntDev     = 0;
            linkCfg->cntPoint   = 0;
            linkCfg->cntTCF     = 0;
            linkCfg->cntFlag    = 0;
            linkCfg->cntHost    = 0;
        }
        else
        {
            if(opr->soeDebug)
            Debug(console,"master> rcv Control : dev=%d, point=%d, tcf=%2x\n",  rxbuf[7], rxbuf[8], rxbuf[9]);

            cntrDevice = rxbuf[7];
            cntrPoint  = rxbuf[8];
            cntrTCF    = rxbuf[9];
            cntrFlag   = rxbuf[10];
            cntrHost   = rxbuf[11];
            
            onTime     = 1000;
            offTime    = 1000;
            
            if(cntrFlag != SET) break;
                
            checkControl_LINK(cntrDevice, cntrPoint, cntrTCF, cntrHost, onTime, offTime);
            
            /* ------------------------------------ */
            /*  CONTROL-ACK 전송                */
            /* ------------------------------------ */
            txbuf[4] = SIM_CONTROL_ACK;
            txbuf[5] = linkCfg->runStatus;          // CU 동작상태
            txbuf[6] = linkCfg->rcvSeqno;           // 수신 seqno 
            txbuf[7] = rxbuf[7];
            txbuf[8] = rxbuf[8];
            txbuf[9] = rxbuf[9];
            txbuf[10]= rxbuf[10];
            txbuf[11]= rxbuf[11];
                 
            count = 12;
            break;
        }
        return (0);    
        break;

    case SIM_HISTORY_ACK:
        return (0);
        break;
        
    case SIM_HISTORY_UP:
        Qnum   = rxbuf[7];  // SOE Queue
        
        if(opr->soeDebug)
        Debug(console,"master> rcv History : Receive Q = %d\n", Qnum);
        
        if(Qnum)
        {
            /* -------------------------------------------- */
            /* Active-CPU 로부터 수신받은 SOE 정보 처리     */
            /* -------------------------------------------- */
            for(i=0; i < Qnum; i++)
            {   
                index   = i*sizeof(SYSLOG_FORM);
                rcvEvent = (SYSLOG_FORM *) &rxbuf[8 + index];

                hfront = hque->front & HISTORY_QUE_MASK;
                
                memcpy((byte *) &hque->queue[hfront], (byte *) rcvEvent, sizeof(SYSLOG_FORM));
                
                hque->front = (hque->front + 1) & HISTORY_QUE_MASK;
                if(hque->front == 0)    hque->overlab = 1;
            }

            /* 수신된 CPU 이벤트 동기화... */            
            linkCfg->historyRear = hque->front;
            
            /* initial database pointer */
            chksum = gensum((byte *) hque, sizeof(SYSTEM_QUE) - 2);
            hque->chksum = chksum;
                    
            /* ------------------------------------ */
            /*  SOE -ACK 전송                */
            /* ------------------------------------ */
            txbuf[4] = SIM_EVENT_ACK;
            txbuf[5] = linkCfg->runStatus;          // CU 동작상태
            txbuf[6] = linkCfg->rcvSeqno;           // 수신 seqno 
            txbuf[7] = Qnum;                        // 수신 Q num
            count = 8;
                
            if(opr->soeDebug)
            Debug(console,"master> snd History-ACK : Receive SOE-Q = %d\n", Qnum);
            break;
        }

        return (0);                
        break;                        	    
        
    case SIM_EVENT_ACK:
        Qnum   = rxbuf[7];  // SOE Queue
        
        if(opr->soeDebug)
        Debug(console,"master> rcv Event-ACK ... rcv = %d \n", Qnum);
            
        return (0);
        break;
        
    case SIM_EVENT_UP:
        Qnum   = rxbuf[7];  // SOE Queue
        
        if(opr->soeDebug)
        Debug(console,"master> rcv Event : Receive Q = %d\n", Qnum);
        
        if(Qnum)
        {
            /* -------------------------------- */
            /* NVRAM Access 참조                */
            /* -------------------------------- */
            for(i=0; i< 10; i++)
            {
                if(opr->sramAccess == SET) pause(10);
                else break;                        
            }
            
            opr->squeWrite = SET;    
            
            /* -------------------------------------------- */
            /* Active-CPU 로부터 수신받은 SOE 정보 처리     */
            /* -------------------------------------------- */
            for(i=0; i < Qnum; i++)
            {   
                index   = i*sizeof(SYSLOG_FORM);
                rcvEvent = (SYSLOG_FORM *) &rxbuf[8 + index];

                efront = sque->front & 0xff;
                
                memcpy((byte *) &sque->queue[efront], (byte *) rcvEvent, sizeof(SYSLOG_FORM));
                
                sque->front = (sque->front + 1) & SYSTEM_QUE_MASK;
                if(sque->front == 0)    sque->overlab = 1;
            }

            /* 수신된 CPU 이벤트 동기화... */            
            linkCfg->eventRear = sque->front;
            
            /* MMI 이벤트 정보 */
            opr->mmiFront   = sque->front;
            opr->mmiOverlab = sque->overlab;
            
            /* initial database pointer */
            chksum = gensum((byte *) sque, sizeof(SYSTEM_QUE) - 2);
            sque->chksum = chksum;
            
            opr->squeWrite = RESET;    
                    
            /* ------------------------------------ */
            /*  SOE -ACK 전송                */
            /* ------------------------------------ */
            txbuf[4] = SIM_EVENT_ACK;
            txbuf[5] = linkCfg->runStatus;          // CU 동작상태
            txbuf[6] = linkCfg->rcvSeqno;           // 수신 seqno 
            txbuf[7] = Qnum;                        // 수신 Q num
            count = 8;
                
            if(opr->soeDebug)
            Debug(console,"master> snd Event-ACK : Receive SOE-Q = %d\n", Qnum);
            break;
        }

        return (0);                
        break;                        	    

    case SIM_SOE_DUMP:                          // 0x34, 계전기 SOE 정보 Update     
        if(opr->runMode == LOCAL_MASTER)
        {         
        }
        else
        {
            Qnum   = rxbuf[7];  // SOE Queue
            
            if(opr->soeDebug)
            Debug(console,"master> rcv SOE : Receive SOE-Q = %d\n", Qnum);

            if(Qnum)
            {
                /* -------------------------------------------- */
                /* Active-CPU 로부터 수신받은 SOE 정보 처리     */
                /* -------------------------------------------- */
                for(i=0; i < Qnum; i++)
                {   
                    index   = i*sizeof(LINK_SOE);
                    linkSoe = (LINK_SOE *) &rxbuf[8 + index];

                    eCode    = linkSoe->eventCode;          // 0x20 : SOE, 0x30 : 장치포인트
                    devid    = linkSoe->devid;
                    devPt    = linkSoe->devPoint;
                    state    = linkSoe->state;
                    
                    /* -------------------------------- */
                    /*  SOE 정보 저장 (for MODBUS)      */
                    /* -------------------------------- */
                    if(eCode == 0x20)       storeSOE_CU_LINK(devid, devPt, state, &linkSoe->timeBuf[0]);  
                    else if(eCode == 0x30)  storeDEV_CU_LINK(devid, devPt, state, &linkSoe->timeBuf[0]);                         
                }
                
                /* ------------------------------------ */
                /*  SOE -ACK 전송                */
                /* ------------------------------------ */
                txbuf[4] = SIM_SOE_ACK;
                txbuf[5] = linkCfg->runStatus;          // CU 동작상태
                txbuf[6] = linkCfg->rcvSeqno;           // 수신 seqno 
                txbuf[7] = Qnum;                        // 수신 Q num
                count = 8;
                
                if(opr->soeDebug)
                Debug(console,"master> snd SOE-ACK : Receive SOE-Q = %d\n", Qnum);
                break;
            }
                
        }
        return (0);    
        break;

    case SIM_SOE_ACK:
        Qnum   = rxbuf[7];  // SOE Queue
        
        if(linkCfg->report == Qnum)
        {
            if(opr->soeDebug)
            Debug(console,"master> rcv SOE-ACK ... Report = %d, rcv = %d \n", linkCfg->report, Qnum);
            
            /* SOE 전송후 ... rear 포인트 증가 */
            linkCfg->rear = (linkCfg->rear + Qnum) & 0xff;  
            linkCfg->report = 0;
        }
        else
        {
            if(opr->soeDebug)
            Debug(console,"master> rcv SOE-ACK ...MissMatch Report = %d, rcv = %d \n", linkCfg->report, Qnum);
            linkCfg->rear = linkCfg->front;
            linkCfg->report = 0;
        }      
        return (0);                
        break;
                        	    
    case SIM_POINT_ANALOG:                          // 0x32, 계전기 계측포인트 POINT 정보 Scan    
        devid      = rxbuf[7];      // 계전기 번호     
        dumpCount  = rxbuf[8];      // 포인트 수   
        
        /* -------------------------------------------------------- */
        /*  SLAVE 모드 : Slave CPU로부터 계전기별 계측 Update...    */
        /* -------------------------------------------------------- */
        if((opr->runMode == LOCAL_SLAVE) && (devid < MAX_MODULE))
        {  
            dnp = (DNP_DEVICE *) dnpcfg[devid];
            
            bfptr = (byte *) &rxbuf[9];
            count = 0;
            
            /* ---------------------------------------- */
            /*  수신된 포인트 수만큼... Update...       */
            /* ---------------------------------------- */
            for(i=0; i< dumpCount; i++)
            {
                anapoint = (bfptr[count + 0]*256) + bfptr[count+1];     // 계측포인트 번호 (0,1,2,...)
                count = count + 2;
                
                fptr = (float *) &bfptr[count];                         // 계측 데이터(Float)
                count = count + 4;
                
                /* 기본 데이터 지정 */
                fdata = (float ) *fptr;
                
                /* ---------------------------------------- */
                /* 2014.12.11 수신된 POINT 예외처리...      */
                /* ---------------------------------------- */
                if(anapoint >= MAX_DEV_AI_POINT)
                {
                    Debug(console,"link> rcv dev=%2d, *Analog Dump - *Invalid AI-Point (%d) \n", devid+1, anapoint);
                    break;
                }
                
                /* ------------------------------------ */               
                /* 계측포인트 데이터.. Update ...       */
                /* ------------------------------------ */   
                storeAI_FLOAT_LINK( devid, anapoint, fdata, (byte *)fptr);
            }
            
            if(opr->slaveDebug) Debug(console, "slave : ANA(%02d) Data Updated...!\n", devid+1);
        }
        return (0);    
        break;
        	    
    case SIM_POINT_STATUS:                          // 0x30, 계전기 상태포인트 POINT 정보 Scan    
        devid      = rxbuf[7] & MAX_MODULE_MASK;      // 계전기 번호     
        stsByteCnt = rxbuf[8];      // 데이터 사이즈 (Byte 수)     
        
        if((devid < 0) || (devid >= MAX_MODULE))
        {
            return (0);
        }      
        
        dnp = (DNP_DEVICE *) dnpcfg[devid];
        /* -------------------------------------------------------- */
        /*  SLAVE 모드 : Slave CPU로부터 계전기별 상태 Update...    */
        /* -------------------------------------------------------- */
        if((opr->runMode == LOCAL_SLAVE) && (devid < MAX_MODULE))
        {    
            /* ---------------------------------------- */
            /*  계전기별 상태데이터 Update...           */
            /* ---------------------------------------- */
            if(stsByteCnt > 0)  curStore_BIT_LINK( devid, &rxbuf[9], stsByteCnt);
            
            if(opr->slaveDebug) 
            {
                Debug(console, "slave : STS(%02d) Data Updated...size= %2d \n", devid+1, stsByteCnt);
                DumpBuff(console," ", &rxbuf[7], stsByteCnt);
            }
        }
        return (0);
        break;
        	    
	case SIM_GPOLL: 	/* GPoll */
	    return (0);
        break;
        
	case SIM_CHKSUM:                                    // 0x01, CPU Database Chksum Dump    
		chksum = rxbuf[7] * 256 + rxbuf[8];
		romsum = gensum((byte *)rtudb, sizeof(RTU_DATABASE) - 2);
		
		if(opr->linkDebug)	
	    {	
	        Debug(console,"----------------------------------------------\n");    
		    Debug(console,"master> rcv Chksum = %4x, romsum = %4x\n", chksum, romsum);
		    Debug(console,"----------------------------------------------\n");   
		}
		
		linkCfg->chksumReq = RESET;
		linkCfg->endFlag   = RESET;
		
		if(chksum != romsum)
		{
			Debug(console,"master> *** Check DB Failed....!\n");
			
			linkCfg->sysDbDown 	  = SET;
			linkCfg->deviceDbDown = SET;
			linkCfg->harrisDbDown = SET;
			linkCfg->pointDbDown  = SET;
			linkCfg->modbusDbDown = SET;
			
			linkCfg->dbDownFlag   = SET;
			linkCfg->dbDownPoint = 1;
			linkCfg->dbDownIndex = 16;
			linkCfg->endFlag     = RESET;
			
			/* DB 전송중 Flag 처리 */
			opr->dbCheckFlag  = SET;
		}

        /* ---------------------------------- */
        /* SLAVE-CPU의 HOST통신상태 Update... */
        /* ---------------------------------- */		
   		opr->rcvMmiHost[0] = rxbuf[9];
    	opr->rcvMmiHost[1] = rxbuf[10];
	    opr->rcvMmiHost[2] = rxbuf[11];
	    opr->rcvMmiHost[3] = rxbuf[12];
	    opr->rcvMmiHost[4] = rxbuf[13];
		return (0);
		break;
			        
	case SIM_TIME_ACK:                             // 0x10,  Time Sync Down    
		linkCfg->timeSync  = RESET;
		linkCfg->chksumReq = SET;
		
		//if(opr->linkDebug)
        Debug(console, "master> rcv TIME Down ACK \n");
		return (0);
        break;
        		
	case SIM_TIME_UP:                               // 0x12, Time Sync Up

        /* ---------------------------------------- */  
        /* CU 시각정보 추출...                      */   
        /* ---------------------------------------- */   
        memcpy( &cuTimeVal, &rxbuf[7], sizeof(struct timeval));

        localtime_r (&cuTimeVal.tv_sec, &cTime);
        milisec = cuTimeVal.tv_usec / 1000;
        
        //if(opr->linkDebug)
        Debug(console, "master> rcv TIME Sync Up : %4d/%2d/%2d %02d:%02d:%02d-%03d\n", cTime.tm_year + 1900, cTime.tm_mon +1, cTime.tm_mday, cTime.tm_hour, cTime.tm_min, cTime.tm_sec, milisec);
        
        /* ---------------------------------------- */  
        /* CU 시각정보 Update...                    */   
        /* ---------------------------------------- */  
        if(settimeofday(&cuTimeVal, NULL) < 0) 
        {
            Debug(console,"master> *** TIMESET-FAIL : %4d/%2d/%2d %02d:%02d:%02d-%03d \n", cTime.tm_year + 1900, cTime.tm_mon +1, cTime.tm_mday, cTime.tm_hour, cTime.tm_min, cTime.tm_sec, milisec);
	    }
	    else
	    {        
            opr->hostDnpTimeSET = SET;
        
            /* TIME-SYNC 관련 이벤트 생성 */    	
            //logEvent1( opr, sque, EVENT_LOCAL_TIMESET, hostid, 0, 0, opr->cpuMode);
            logHistoryQue1( opr, hque, EVENT_LOCAL_TIMESET, 0, 0, 0, opr->cpuMode);
            
            /* -------------------------------------------- */
            /* LOG File 저장                                */
            /* -------------------------------------------- */
            sprintf(buffer, "master> rcv TIME Sync Up : %4d/%2d/%2d %02d:%02d:%02d-%03d", cTime.tm_year + 1900, cTime.tm_mon +1, cTime.tm_mday, cTime.tm_hour, cTime.tm_min, cTime.tm_sec, milisec);
            LogFile_MPU ( opr, EVENT_LOCAL_TIMESET, buffer, strlen(buffer));
            
        }
        
        /* ------------------------------------ */
        /*  TIME-ACK 전송                       */
        /* ------------------------------------ */
        txbuf[4] = SIM_TIME_ACK;
        txbuf[5] = linkCfg->runStatus;          // CU 동작상태
        txbuf[6] = linkCfg->rcvSeqno;           // 수신 seqno
        count = 7;
        
        memcpy( &txbuf[count], &rxbuf[7], sizeof(struct timeval));
        count = count + sizeof(struct timeval);                         		
        
        //if(opr->linkDebug)
        Debug(console, "master> snd TIME Down ACK \n");
                    		
        break;    		

	case SIM_HOST_DOWN:                             // 0x50        HOST Config Database Down     
		linkCfg->sysDbDown	= RESET;
		if(opr->linkDebug)
		Debug(console,"master> rcv HOST-DB ...down OK\n");
		return (0);
        break;
        		
	case SIM_SCAN_DOWN:                             // 0x52,   SCAN Config Database Down     
		linkCfg->deviceDbDown = RESET;
		if(opr->linkDebug)
		Debug(console,"master> rcv SCAN-DB ...down OK\n");
		return (0);
        break;
        		
	case SIM_PORT_DOWN:                             // 0x54, Port Config Database Down     
		linkCfg->harrisDbDown = RESET;
		if(opr->linkDebug)
		Debug(console,"master> rcv HARRIS-DB ...down OK\n");
		return (0);
        break;
        
	case SIM_POINT_DOWN:                            // 0x58, Point Config Database Down    
		startPoint = rxbuf[7]*256 + rxbuf[8];
		
		linkCfg->dbDownPoint = startPoint + linkCfg->dbDownIndex;

		/* 포인트 DB 전송 종료... */		
		if((linkCfg->dbDownPoint >= MAX_DB_POINT) || (linkCfg->endFlag == 0x80))    
		{	
			linkCfg->pointDbDown = RESET;
			linkCfg->endFlag     = 0;
			
			opr->dbCheckFlag     = RESET;
			linkCfg->chksumReq   = SET;
			Debug(console,"**** master DB-DOWN End...!\n");
		}
		
		if(opr->linkDebug)	
		Debug(console,"master> rcv POINT-DB ...start= %d , num=%d...down OK\n", startPoint, linkCfg->dbDownIndex);
		return (0);
        break;
        
	case SIM_MODBUS_DOWN:                           // 0x60, MODBUS Profile Block Database Down    
		linkCfg->modbusDbDown = RESET;
		if(opr->linkDebug)
		Debug(console,"master> rcv MODBUS-DB ...down OK\n");
		return (0);
        break;

	default :
	    if(opr->linkDebug)
		Debug(console,"master> *** Invalid Opcode = %2x\n", opcode);
		return (0);
	}

    sndCount = count + 1;

    txbuf[2] = (sndCount >> 8) & 0xff;     /* Total size: MSB */
    txbuf[3] = sndCount & 0xff;            /* Total size: LSB */
    txbuf[count] = genlrc(txbuf, count);
    count++;
    return(count);
    
}



/*
*   MASTER-CPU Main()
*/
void linkMaster()
{
    //int     i;
    //int     fid;
	byte	runtick;
	int     slvStatus, slaveTick;
	int		oldsec=0, oldHour;
	int     linkDBdown;
	//int     oldmin;
	int		initLink;
    char    buffer[128];
    
    byte	*rxbuf, *txbuf;
	int		rxcnt, txcnt;
		
    printf(" %s LINK-MASTER[%s] PROCESS Activated ... !\n", TARGET_NAME, linkPort.name);
    
    sprintf(buffer, "%s LINK-MASTER PROCESS Activated ... !", TARGET_NAME);
    LogFile_MPU ( opr, NOT_DEFINED, buffer, strlen(buffer));
 
 	rxbuf = (byte *) &linkRxbuf[0];
	txbuf = (byte *) &linkTxbuf[0];
	
    runtick = 0;

    linkCfg->comFailTick = 0;
    // 일단 처음에 기본으로 2 가지는 확인..
    linkCfg->chksumReq = SET;
    linkCfg->timeSync  = SET;
    linkCfg->offline   = 2;

	opr->checkLinkOK   = SET;		// CPU 링크상태 확인....
		
	oldHour = rtc->hour;
	initLink = 0;
	slaveTick = 0;
	
	while(termExec)
	{
	    taskPtr->wdtCount = 0;
		  pause(100);

        if((rxcnt = rcvWithLINK(&linkPort, rxbuf)) > 0)
        {
            linkCfg->comFailTick = 0;
        	
            if(opr->linkDebug)  
            {
                if(rxcnt > 128) DumpBuff(console,"RXM: ", rxbuf, 128);
                else            DumpBuff(console,"RXM: ", rxbuf, rxcnt);    
            }
            
            /* ---------------------------------------- */
            /*  CPU 모두 Slave 인 경우...Masert SET     */
            /* ---------------------------------------- */
            slvStatus = rxbuf[4] & 0x80;
            if((opr->runMode == LOCAL_SLAVE) && (slvStatus == 0x00))
            {   
                if(++slaveTick > 3)
                {
                    setMaster();
        	        Debug(console,"link> *return =>  SET Master !\n");   
		            slaveTick = 0;
		        }
            }
            else
                slaveTick = 0;
            
            txcnt = rcvHandler_master(rxbuf, rxcnt, txbuf);
            
            if(txcnt > 6)
            {
                PortWrite( (TTY_DESC *) &linkPort, txbuf, txcnt);
                if(opr->linkDebug)      DumpBuff(console,"TXM: ", txbuf, txcnt);    
            }
                  
            /* LINK-OFFLINE 이벤트 생성...  */
            if(linkCfg->offline != RESET)
            {    
                logEvent1(opr, sque, EVENT_LINK_ONLINE, 0, 0, 0, opr->cpuMode);  
                logHistoryQue1(opr, hque, EVENT_LINK_ONLINE, 0, 0, 0, opr->cpuMode);  
                
                /* -------------------------------------------- */
                /* LOG File 저장                                */
                /* -------------------------------------------- */
                sprintf(buffer, "%s", "LINK Online ");
                LogFile_MPU ( opr, EVENT_LINK_ONLINE, buffer, strlen(buffer));
                
                
                /* 통신 복귀시.... 이중화 상태 Check... */
                linkCfg->cmdFlag = SET;
            }
            
            linkCfg->offline = RESET;
            linkCfg->sndSeqno++;          // CU 통신상태 Seqno
            
        }
        else
        {
            if((txcnt = makeLinkCmd(txbuf)) > 0)
            {	
                PortWrite( (TTY_DESC *) &linkPort, txbuf, txcnt);
                if(opr->linkDebug)  
                {
                    if(txcnt > 128) DumpBuff(console,"TXM: ", txbuf, 128);
                    else            DumpBuff(console,"TXM: ", txbuf, txcnt);    
                }
            } 

            /* 데이터베이스 전송 상태 */            
            linkDBdown = linkCfg->sysDbDown + linkCfg->deviceDbDown + linkCfg->harrisDbDown + linkCfg->modbusDbDown;
	    
            /* DB 전송에 따른 Delay...      */
            if(linkCfg->chksumReq == SET)   pause(100); 
            else if(linkDBdown)             
            {
                linkDBdown = 0;
                pause(500); 
            }
            else if(linkCfg->pointDbDown)
            {
                if(linkCfg->endFlag)    pause(500);     
            }              
                                      
        }      	
            
        /* ------------------------------------ */
        /*  매초단위 Function ...               */
        /* ------------------------------------ */
        if(oldsec == rtc->sec)  continue;
        oldsec = rtc->sec;

        if(oldsec == 0) 
        {
            if(opr->dbCheckFlag == RESET)  linkCfg->chksumReq = SET;       // 1분단위 DB Check...    
        }       
            
        /* ------------------------------------ */
        /* 주기적으로 CPU 장착상태를 Check...          */
        /* ------------------------------------ */
	    if(linkCfg->cmdFlag == SET) checkDualCPU(1);    	 
        else                        checkDualCPU(0);    	 	    
		
		
        /* ------------------------------------ */
        /*  LINK Timeout 처리...                */
        /* ------------------------------------ */
   		if(++linkCfg->comFailTick > 5)
   		{
   			initLink = 0;
       		
       		linkCfg->comFailTick = 0;	
		    
		    linkCfg->cntDev     = 0;
            linkCfg->cntPoint   = 0;
            linkCfg->cntTCF     = 0;
            linkCfg->cntFlag    = 0;
            linkCfg->cntHost    = 0;
            linkCfg->cntSndTick = 0;
            
			linkCfg->chksumReq = SET;
   			linkCfg->timeSync  = SET;
   			opr->dbCheckFlag   = RESET;
				
			if(opr->linkDebug)	Debug(console,"link> *** SLAVE-CPU Com Fail...!\n");  
			
			if(linkCfg->offline != SET)
			{    
			    logEvent1(opr, sque, EVENT_LINK_OFFLINE, 0, 0, 0, opr->cpuMode);  
			    logHistoryQue1(opr, hque, EVENT_LINK_OFFLINE, 0, 0, 0, opr->cpuMode);  
			    
			    /* -------------------------------------------- */
                /* LOG File 저장                                */
                /* -------------------------------------------- */
                sprintf(buffer, "%s", "*LINK Offline ");
                LogFile_MPU ( opr, EVENT_LINK_OFFLINE, buffer, strlen(buffer));
			}
			
			linkCfg->offline = SET;
			
			/* Slave 모드 Check... */
			if(opr->runMode == LOCAL_SLAVE)  /* 내가 SLAVE 이니까...상대(MASTER)가 죽었을 수 도있지.. PEER-IN, SCU MODE 확인 */
		    {
		        Debug(console,">> *CPU LINK Fail... Check..ACT !\n");
		        linkCfg->cmdFlag = SET;
		    }
			
			PortClose((TTY_DESC *) &linkPort);  pause(100);
            PortInitial((TTY_DESC *) &linkPort); pause(100);    
       	}		
       	 
        
	}/* while */
	
	resetMaster();

}
