
#include	"localLib.h"
#include    "external.h"

extern  int     dnpChanWrite(int fid, int devid, byte *sndbuf, int sndCount);
extern  int     setOffline( int devid);
extern  int     setOnline( int devid);

extern  int     curStore_BIT_DNP( int devid, byte *stsbuf, int dataSize);
extern  int     storeAI_DNP(int devid, int point, float fdata, byte *lowdata);
extern  int     storeAI_FLOAT_DNP(int devid, int point, float fdata, byte *lowdata);

extern  int     vmeScanChanReset(int devid);
extern  void    vmeScanInitial(int scanid, int chid);
extern  int     dnpChanRead(int fid, int devid, byte *rxbuf, int reqCount);

extern  int     storeSOE_CU_DNP( int devid, int devPoint, int state, byte *buf);
extern  int     storeCOS_CU_DNP( int devid, int point, int state);

extern  TTY_DESC scanChan[MAX_CPUPORT];

extern  int     termExec;
extern  TASK_INFO	*taskPtr;

/* ---------------------------------------- */
/*  VMEBUS 관련 변수 초기화                 */
/* ---------------------------------------- */
extern  VME_SIODCB      *vmeSioDCB;
extern  VME_CHAN_DCB    *vmeChanDCB[SIO_CHANNEL_MAX];


/*
*/
int checkENQ(int scanid, int devid, byte *rxbuf)
{
    //if(read(fid, &rxbuf[0], 1) == 0 )   return (0);
    if(dnpChanRead(scanid, devid, &rxbuf[0], 1) == 0) return (0);   
    if(rxbuf[0] == LOC_ENQ) return (1);
    else                    return (0);
}


/*
*/
int rcvWithLOC(int scanid, int devid, byte *rxbuf)
{
    //int reqSize,rcvSize;
    int     size;
    int     rcvCount=0;
    byte    bcc=0, bcc_h, bcc_l;
    
    /* search STX .... */
    do{
        //if(read(fid, &rxbuf[0], 1) == 0 )   return (0);
        if(dnpChanRead(scanid, devid, &rxbuf[0], 1) == 0) return (0);    
        if(++rcvCount > 50) return (0);    
    } while(rxbuf[0] != LOC_STX);

    size = 1;
    rcvCount = 0;
    
    /* search STX .... */
    do{
        //if(read(fid, &rxbuf[0], 1) == 0 )   return (0);
        if(dnpChanRead(scanid, devid, &rxbuf[size], 1) == 0) return (0);    
        if(++rcvCount > 100) 
        {
            if(opr->dnpDebug == devid)
            Debug(console, "Locator[dev=%d] *** rcv ETX Error.... Count = %d !\n", devid, size);
            return (0);     
        }
    } while(rxbuf[size++] != LOC_ETX);

    /* ------------------------ */
    /* read BCC-H, BCC-L & CR   */
    /* ------------------------ */
    if(dnpChanRead(scanid, devid, &rxbuf[size], 3) == 0) return (0);    
        
    size = size + 3;
    
    bcc = genlrc(rxbuf, size - 3);
    
    bcc_h = (bcc >> 4) & 0x0f;
    bcc_h = bcc_h + '0';
    
    bcc_l = bcc & 0x0f;
    bcc_l = bcc_l + '0';
    
#if 0
    bcc_h = (bcc >> 4) & 0x0f;
    bcc_h = (bcc_h < 10) ? (bcc_h + '0') : (bcc_h + ('A'-10));
    
    bcc_l = bcc & 0x0f;
    bcc_l = (bcc_l < 10) ? (bcc_l + '0') : (bcc_l + ('A'-10));
#endif

    if((bcc_h != rxbuf[size-3]) || (bcc_l != rxbuf[size-2]))
    {
        if(opr->dnpDebug == devid)
        {
            Debug(console, "Locator[dev=%d] *** BCC Check Error.... BCC = %02x [H=%02x/L=%02x] %02x %02x..!\n", devid, bcc, bcc_h, bcc_l, rxbuf[size-3], rxbuf[size-2] );
            DumpBuff(console, "*RXD: ", (char *) rxbuf, size);
        }
        return (0);
    }                   
    
    return (size);
}

/*
*/
int sendPoll(int scanid, int devid, byte *txbuf)
{
    int txcnt;

    txcnt = 0;
    txbuf[txcnt++] = LOC_BEL;
    //write(fid, txbuf, txcnt);
    dnpChanWrite(scanid, devid, txbuf, txcnt);
    
    if(opr->dnpDebug == devid)
    { 
        Debug(console, "Locator[dev=%d] - Send Poll(BEL) !\n", devid);
        DumpBuff(console, "TXD: ", (char *) txbuf, txcnt);
    }
    return (0);
}    

/*
*/
int sendACK(int scanid, int devid, byte *txbuf)
{
    int txcnt;
    
    txcnt = 0;
    txbuf[txcnt++] = LOC_ACK;
    //write(fid, txbuf, txcnt);
    dnpChanWrite(scanid, devid, txbuf, txcnt);
    
    if(opr->dnpDebug == devid)
    { 
        Debug(console, ">> Locator[dev=%d] - Send ACK !\n", devid);
        DumpBuff(console, "TXD: ", (char *) txbuf, txcnt);
    }
    
    return (0);
}

/*
*/
int sendNACK(int scanid, int devid, byte   *txbuf)
{
    int txcnt;
    
    txcnt = 0;
    txbuf[txcnt++] = LOC_NACK;
    //write(fid, txbuf, txcnt);
    dnpChanWrite(scanid, devid, txbuf, txcnt);
    
    if(opr->dnpDebug == devid)
    { 
        Debug(console, ">> Locator[dev=%d] - Send NACK !\n", devid);
        DumpBuff(console,"TXD: ", (char *) txbuf, txcnt);
    }
    
    return (0);
}


/*
*   DEVICE POINT SOE 정보 ....
*/
int create_Locator_SOE(int devid, int devPt, int state)
{
    int     millisecond;
    
    DNP_DEVICE  *dnp;
    POINT_BUF   *diPtBuf;
    
    byte    soeBuffer[8];
    
    PDPDTIME_DATE_TIME      dnpRTC;
    PDPDTIME_MS_SINCE_70    dnpRTCInfo;
    
    struct timeval rv; 
    time_t  tm;
    struct  tm  localtm;
    
    /* ---------------------------------------- */
    /*  장치 포인트 관리...                     */
    /* ---------------------------------------- */
    dnp = (DNP_DEVICE *) dnpcfg[devid];
    
    diPtBuf = (POINT_BUF *) &dnp->diPtBuf[devPt];
    
    if(diPtBuf->config != SET)  return (0);
    
    /* 동일상태인경우... SKIP */
    if(diPtBuf->locStatus == state)  return (0);	
    
    diPtBuf->locStatus = state;                  // Device Point 상태
    
    /* ---------------------------------- */
    /* 현재시간 정보를 DNP Form 으로 변환 */
    /* ---------------------------------- */
    time(&tm);
    localtime_r (&tm, &localtm);
                
    gettimeofday(&rv, NULL);
    millisecond = (rv.tv_usec / 1000) % 1000;

    dnpRTC.year       = localtm.tm_year + 1900;
    dnpRTC.month      = localtm.tm_mon + 1;
    dnpRTC.day        = localtm.tm_mday;
    dnpRTC.hour       = localtm.tm_hour;
    dnpRTC.minute     = localtm.tm_min;
    dnpRTC.second     = localtm.tm_sec;
    dnpRTC.millisecond= millisecond;

    makeDnpTimeByte( &dnpRTCInfo, &dnpRTC);
    
    soeBuffer[0] = (dnpRTCInfo.leastSignificant) & 0xff;
    soeBuffer[1] = (dnpRTCInfo.leastSignificant >> 8) & 0xff;
    soeBuffer[2] = (dnpRTCInfo.mostSignificant) & 0xff;
    soeBuffer[3] = (dnpRTCInfo.mostSignificant >> 8)  & 0xff;
    soeBuffer[4] = (dnpRTCInfo.mostSignificant >> 16) & 0xff;
    soeBuffer[5] = (dnpRTCInfo.mostSignificant >> 24) & 0xff;
                
    if(opr->soeDebug)
    Debug(console,"dev%2d> Locator-SOE: pt=%2d, sts=%2x ... %s \n", devid+1, devPt+1, state, diPtBuf->nameStr);
                
    /* -------------------------------- */
    /*  SOE 정보 저장 (for MODBUS)      */
    /* -------------------------------- */
    dnp->initial = STATUS_DUMP_TICK;
    
    storeSOE_CU_DNP(devid, devPt, state, &soeBuffer[0]); 
                
#if 0
    /* -------------------------------- */
    /*  COS 정보 저장 (for MODBUS)      */
    /* -------------------------------- */
    storeCOS_CU_DNP( devid, devPt, state);
#endif
                                    
    return (0);     
}


/*
*   쯔다 고장점 - ANALOG Update 
*/
int update_LOC_DATE(int devid, int ptinx, int value)
{
    //int     j;
    //int     inx=0;
    //int     adata;
    //int     lowValue, highValue;
    
    //float   fdata, aiScale;
    
    DNP_DEVICE  *dnp;
    POINT_BUF   *aiPoint;
    
    dnp = (DNP_DEVICE *) dnpcfg[devid];
    
    aiPoint = (POINT_BUF *) &dnp->aiPtBuf[ptinx];
    
    if(aiPoint->config == 0)    return (0);
   
    if(opr->dnpDebug == devid)  
    Debug(console, "dev%02d> AI Update ... point= %d Value= %d\n", devid+1, ptinx+1, value);
         
    /* ------------------------------------ */
    /*  수집데이터에 의한... 값 Update      */
    /* ------------------------------------ */
    aiPoint->pointData = value;
    aiPoint->floatData = value * 1.0;  

    return (1);       
}
        
static	byte	bitcode[8] = {0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80};

/*
* ================================================
*   쯔다 고장점 - 상태 처리 ....
* ================================================
*/
int update_LOC_DI(int devid)
{
    int     i, point;
    int     index1,index2;
    //int     lowValue, highValue;
    //int     TF_valid1, TF_valid2;
    //byte    state, bufsts;
    byte    stsbuf[8];
    
    DNP_DEVICE  *dnp;
    POINT_BUF   *diPoint1, *diPoint2;
    POINT_BUF   *aiPoint1, *aiPoint2;

    dnp = (DNP_DEVICE *) dnpcfg[devid];
    
    /* ------------------------------------ */
    /*  0. RESET 상태 이벤트                */
    /* ------------------------------------ */
    diPoint1  = (POINT_BUF *) &dnp->diPtBuf[0];
    if(dnp->locEvent)
    {      
        if(diPoint1->locStatus == 0)
        {
            create_Locator_SOE(devid, 0, SET);  // RESET 이상상태...
        } 
        diPoint1->locStatus = 1;     // 이상
    }
    else
    {
        if(diPoint1->locStatus == 1)
        {
            create_Locator_SOE(devid, 0, RESET);  // RESET 정상상태...
        } 
        diPoint1->locStatus = 0;    // 정상
    }     

	/* 고장점 통신상태... */
	diPoint1  = (POINT_BUF *) &dnp->diPtBuf[1];
    if(diPoint1->locStatus == 0)
    {
        create_Locator_SOE(devid, 1, SET);  // 통신상태 이상상태...
        
    } 
    diPoint1->locStatus = 1;     // 이상
    
    /* ------------------------------------ */
    /*  1. TF 단락 이벤트                   */
    /* ------------------------------------ */
    aiPoint1 = (POINT_BUF *) &dnp->aiPtBuf[6];           // 거리정보
    diPoint1  = (POINT_BUF *) &dnp->diPtBuf[6];
    
    if(dnp->type == TYPE_LOCATOR1)		// 경산 SS
	{    	     
	    if(aiPoint1->locEvent == SET)   // TF 단락 발생
    	{
	        if(diPoint1->locStatus == 0)    create_Locator_SOE(devid, 6, SET);
    	    diPoint1->locStatus = 1;
	    }
    	else
	    {
    	    diPoint1->locStatus = 0;
    	}
          

	    /* ------------------------------------ */
    	/*  2. 상선/하선 이벤트                 */
	    /* ------------------------------------ */
    	aiPoint1 = (POINT_BUF *) &dnp->aiPtBuf[7];           // 상선 전류
	    aiPoint2 = (POINT_BUF *) &dnp->aiPtBuf[8];           // 하선 전류
    	diPoint1 = (POINT_BUF *) &dnp->diPtBuf[7];
	    diPoint2 = (POINT_BUF *) &dnp->diPtBuf[8];
    	diPoint1->locStatus = 0;
	    diPoint2->locStatus = 0;  
            
	    if(dnp->locEvent)       // 고장점 이벤트 시....
    	{
	        if(aiPoint1->pointOver +  aiPoint2->pointOver)  // 상선, 하선 NOR 상태시
    	    {
        	    if(aiPoint1->pointOver)     // 상선 NOR 발생시
            	{
	                if(aiPoint2->limitEvent)    // 하선 Limit 발생
    	            {
        	            if(diPoint2->locStatus == 0)    create_Locator_SOE(devid, 8, SET);      // 하선 이벤트
            	        diPoint2->locStatus = 1;    
                	}
	            }
    	        else
        	    {
            	    if(aiPoint1->limitEvent)    // 상선 Limit 발생
                	{
                    	if(diPoint1->locStatus == 0)    create_Locator_SOE(devid, 7, SET);      // 상선 이벤트
	                    diPoint1->locStatus = 1;    
    	            }
        	    }           
	        }
    	    else
        	{
	            if(aiPoint1->limitEvent +  aiPoint2->limitEvent)  // 상선, 하선 Limit 상태시
    	        {
        	        if(aiPoint1->locData > aiPoint2->locData)       
            	    {
                		if(diPoint1->locStatus == 0)    create_Locator_SOE(devid, 7, SET);      // 상선 이벤트
	                    diPoint1->locStatus = 1;     
    	            }
        	        else if(aiPoint1->locData < aiPoint2->locData)  
	                {
    	                if(diPoint2->locStatus == 0)    create_Locator_SOE(devid, 8, SET);      // 하선 이벤트     
        	            diPoint2->locStatus = 1;                      
            	    }
	                else
    	            {
        	            if(diPoint1->locStatus == 0)    create_Locator_SOE(devid, 7, SET);      // 상선 이벤트
            	        diPoint1->locStatus = 1;    
                    
                	    if(diPoint2->locStatus == 0)    create_Locator_SOE(devid, 8, SET);      // 하선 이벤트
                    	diPoint2->locStatus = 1;   
	                }
    	        }
        	}              
    	}

	    /* ---------------------------------------- */
	    /* ST#1 ~ #6 전류 이상                  */
    	/* ---------------------------------------- */
	    for(i=9; i <= 20; i++)
    	{ 
	        aiPoint1 = (POINT_BUF *) &dnp->aiPtBuf[i];          // AT 상선 전류
    	    diPoint1 = (POINT_BUF *) &dnp->diPtBuf[i];          // AT 전류 범위초과
        	diPoint2 = (POINT_BUF *) &dnp->diPtBuf[i + 14];     // AT 통신이상
        
	        diPoint1->locStatus = 0;
    	    diPoint2->locStatus = 0; 
    
	        if(aiPoint1->limitEvent)
    	    {     
        	    if(diPoint1->locStatus == 0)    create_Locator_SOE(devid, i, SET);
	            diPoint1->locStatus = 1;    // 전류이상      
    	    }     
        
	        if(aiPoint1->pointComFail)
    	    {     
        	    if(diPoint2->locStatus == 0)    create_Locator_SOE(devid, i+14, SET);
            	diPoint2->locStatus = 1;    // 통신이상    
	        }  
    	}
    }
    /* ---------------------------------------------------- */
    /*	모란 SS경우 : 아날로그 값이 있으면... 상태 SET 					*/
    /* ---------------------------------------------------- */
	else if(dnp->type == TYPE_LOCATOR2)		// 모란 SS
	{   
		aiPoint1 = (POINT_BUF *) &dnp->aiPtBuf[6];           // 거리정보
    	diPoint1  = (POINT_BUF *) &dnp->diPtBuf[6];
     	
     	if(aiPoint1->locData > 0)	
     	{
     		if(diPoint1->locStatus == 0)    create_Locator_SOE(devid, 6, SET);
     		diPoint1->locStatus = 1;
     	}
     	else						
     	{
     		diPoint1->locStatus = 0;	
     	}
     	     
	    /* ------------------------------------ */
    	/*  2. 상선/하선 이벤트                 */
	    /* ------------------------------------ */
    	aiPoint1 = (POINT_BUF *) &dnp->aiPtBuf[7];           // 상선 전류
	    aiPoint2 = (POINT_BUF *) &dnp->aiPtBuf[8];           // 하선 전류
    	diPoint1 = (POINT_BUF *) &dnp->diPtBuf[7];
	    diPoint2 = (POINT_BUF *) &dnp->diPtBuf[8];
    	diPoint1->locStatus = 0;
	    diPoint2->locStatus = 0;  
        
        if(aiPoint1->locData > 0)	
        {
			if(diPoint1->locStatus == 0)    create_Locator_SOE(devid, 7, SET);
        	diPoint1->locStatus = 1;
        }
     	else						diPoint1->locStatus = 0;
     	
     	if(aiPoint2->locData > 0)	
		{
			if(diPoint2->locStatus == 0)    create_Locator_SOE(devid, 8, SET);
			diPoint2->locStatus = 1;
		}
     	else						diPoint2->locStatus = 0;
     		
	    /* ---------------------------------------- */
	    /* ST#1 ~ #6 전류 이상                  */
    	/* ---------------------------------------- */
	    for(i=9; i <= 20; i++)
    	{ 
	        aiPoint1 = (POINT_BUF *) &dnp->aiPtBuf[i];          // AT 상선 전류
    	    diPoint1 = (POINT_BUF *) &dnp->diPtBuf[i];          // AT 전류 범위초과
        	diPoint2 = (POINT_BUF *) &dnp->diPtBuf[i + 14];     // AT 통신이상
        
	        diPoint1->locStatus = 0;
    	    diPoint2->locStatus = 0; 

			if(aiPoint1->locData > 0)	
			{
				if(diPoint1->locStatus == 0)    create_Locator_SOE(devid, i, SET);
				diPoint1->locStatus = 1;
			}
     		else						diPoint1->locStatus = 0;
    	}
    }    
    

    /* ---------------------------------------- */
    /*  LOCATOR 장치 - 상태정보 Update...       */
    /* ---------------------------------------- */
    bzero8248(stsbuf, 8);
    
    for(point=0; point < dnp->devDiPoint; point++)
    {
        diPoint1 = (POINT_BUF *) &dnp->diPtBuf[point];
        index1 = point / 8;        
        index2 = point % 8;

        if(index1 >= 5) break;
        if(diPoint1->locStatus)   stsbuf[index1] |= bitcode[index2];
    }
    
    //if(opr->msgDebug == devid) 
    Debug(console,"dev%02d> Locator Status dump : %02x %02x %02x %02x \n", devid+1, stsbuf[0], stsbuf[1], stsbuf[2], stsbuf[3]);
    
    memcpy(dnp->cursts, stsbuf, 4);

    /* ------------------------------------ */                    
    /* 상위 HOST Buffer Update...           */
    /* ------------------------------------ */      
    curStore_BIT_DNP( devid, &dnp->cursts[0], 4);
                                       
    return (0);     
}

/*
* ========================================================
*   고장점 장치 - 데이터 분류
* ========================================================
*/
int check_LOC_Event(int devid, char (*itemString)[16])
{
    int     i, j, strinx;
    int     tmpValue;
    int     lowLimit, highLimit; 
    int     locStrIndex=1;      
    
    DNP_DEVICE  *dnp;
    POINT_BUF   *aiPoint;

    /* --------------------------------------------------- */
    /*  고장점 장치 TYPE#1 : 년/월/일/시/분/초 정보 포함                */
    /*  고장점 장치 TYPE#2 : 년/월/일/시/분/초 정보 포함 않함             */
    /* --------------------------------------------------- */
    dnp = (DNP_DEVICE *) dnpcfg[devid];
    dnp->locEvent = 0;

    if(dnp->type == TYPE_LOCATOR1)        
    {
        locStrIndex = 1;    // 년월일정보 있음... index = 1
        if(opr->dnpDebug == devid)
        {
            Debug(console, "\n[LOC-TYPE#1]--------------------------------------------\n" );
            Debug(console, "%02d) Date Info  ... %s\n", 1, (char *) itemString[0]);
            Debug(console, "%02d) Distance   ... %s\n", 2, (char *) itemString[1]);
            Debug(console, "%02d) SS  UP-Line... %s\n", 3, (char *) itemString[2]);
            Debug(console, "%02d) SS  DN-Line... %s\n", 4, (char *) itemString[3]);
            Debug(console, "%02d) AT1 UP-Line... %s\n", 5, (char *) itemString[4]);
            Debug(console, "%02d) AT1 DN-Line... %s\n", 6, (char *) itemString[5]);
            Debug(console, "%02d) AT2 UP-Line... %s\n", 7, (char *) itemString[6]);
            Debug(console, "%02d) AT2 DN-Line... %s\n", 8, (char *) itemString[7]);
            Debug(console, "%02d) AT3 UP-Line... %s\n", 9, (char *) itemString[8]);
            Debug(console, "%02d) AT3 DN-Line... %s\n", 10, (char *) itemString[9]);
            Debug(console, "%02d) AT4 UP-Line... %s\n", 11, (char *) itemString[10]);
            Debug(console, "%02d) AT4 DN-Line... %s\n", 12, (char *) itemString[11]);
            Debug(console, "%02d) AT5 UP-Line... %s\n", 13, (char *) itemString[12]);
            Debug(console, "%02d) AT5 DN-Line... %s\n", 14, (char *) itemString[13]);
            Debug(console, "%02d) AT6 UP-Line... %s\n", 15, (char *) itemString[14]);
            Debug(console, "%02d) AT6 DN-Line... %s\n", 16, (char *) itemString[15]);
        
            Debug(console, "---------------------------------------\n");
        }
    }
    else if(dnp->type == TYPE_LOCATOR2)        
    {
        locStrIndex    = 0;    // 년월일정보 없음... index = 0
        dnp->locEvent  = SET;  // 무조건 현시....
        
        if(opr->dnpDebug == devid)
        {
            Debug(console, "\n[LOC-TYPE#2]--------------------------------------------\n" );
            Debug(console, "%02d) Date Info  ... NONE\n", 1);
            Debug(console, "%02d) Distance   ... %s\n", 2, (char *) itemString[0]);
            Debug(console, "%02d) SS  UP-Line... %s\n", 3, (char *) itemString[1]);
            Debug(console, "%02d) SS  DN-Line... %s\n", 4, (char *) itemString[2]);
            Debug(console, "%02d) AT1 UP-Line... %s\n", 5, (char *) itemString[3]);
            Debug(console, "%02d) AT1 DN-Line... %s\n", 6, (char *) itemString[4]);
            Debug(console, "%02d) AT2 UP-Line... %s\n", 7, (char *) itemString[5]);
            Debug(console, "%02d) AT2 DN-Line... %s\n", 8, (char *) itemString[6]);
            Debug(console, "%02d) AT3 UP-Line... %s\n", 9, (char *) itemString[7]);
            Debug(console, "%02d) AT3 DN-Line... %s\n", 10, (char *) itemString[8]);
            Debug(console, "%02d) AT4 UP-Line... %s\n", 11, (char *) itemString[9]);
            Debug(console, "%02d) AT4 DN-Line... %s\n", 12, (char *) itemString[10]);
            Debug(console, "%02d) AT5 UP-Line... %s\n", 13, (char *) itemString[11]);
            Debug(console, "%02d) AT5 DN-Line... %s\n", 14, (char *) itemString[12]);
            Debug(console, "%02d) AT6 UP-Line... %s\n", 15, (char *) itemString[13]);
            Debug(console, "%02d) AT6 DN-Line... %s\n", 16, (char *) itemString[14]);
        
            Debug(console, "---------------------------------------\n");
        }
    }
    
    /* ------------------------------------ */
    /*  1. 위치정보 처리                    */
    /* ------------------------------------ */
    aiPoint = (POINT_BUF *) &dnp->aiPtBuf[6];           // 거리정보
    aiPoint->pointComFail = 0;
    aiPoint->pointOver    = 0;
    aiPoint->locEvent     = 0;
    aiPoint->locData      = 0;   
    aiPoint->locEvent     = 0; 

		/* Distance 영역 비교 */
    if(strcmp("TFST", itemString[0 + locStrIndex]) == 0)              // TF 단락
    {
        if(opr->dnpDebug == devid)  Debug(console, "=> TFST Event \n"); 
        dnp->locEvent       = SET;
        aiPoint->locEvent   = SET;                      // TF 단락 사고
    }
    else if(strcmp("EROR", itemString[0 + locStrIndex]) == 0)         // 거리측정 오류
    {
        if(opr->dnpDebug == devid)  Debug(console,"=> EROR State \n"); 
        //dnp->locEvent       = SET;                      // 화면표시
        dnp->locEvent       = 0;                        // 화면표시 하지않음
        aiPoint->locEvent   = 0;                    
    }
    else if(strcmp("NOER", itemString[0 + locStrIndex]) != 0)         // 거리측정 오류
    {
        tmpValue = atoi(itemString[0 + locStrIndex]);
        aiPoint->locData = tmpValue;
        if(opr->dnpDebug == devid)  Debug(console,"=> 거리정보 = %d \n", aiPoint->locData); 
    }
    else
    {
        aiPoint->locData = 0;
        if(opr->dnpDebug == devid)  Debug(console,"=> 거리정보 측정오류 = %d \n", aiPoint->locData); 
    }  
    
    /* --------------------------------------- */
    /* 	무조껀 현시...				*/
    /* --------------------------------------- */
    if(dnp->type == TYPE_LOCATOR2)        
    {
        dnp->locEvent  		= SET;  // 무조건 현시....
        aiPoint->locEvent   = SET; 
    }
          

    /* ------------------------------------ */
    /*  2. SS UP Line 처리                  */
    /* ------------------------------------ */
    aiPoint = (POINT_BUF *) &dnp->aiPtBuf[7];           // SS-UP Line
    lowLimit  = aiPoint->modBase;
    highLimit = aiPoint->modIndex;

    aiPoint->pointComFail = 0;
    aiPoint->pointOver    = 0;
    aiPoint->locEvent     = 0;
    aiPoint->locData      = 0;   
    aiPoint->locEvent     = 0; 
    aiPoint->limitEvent   = 0; 
            
    if(strcmp("FFF", itemString[1 + locStrIndex]) == 0)               // 통신이상
    {
        aiPoint->pointComFail = SET;
    }
    else if(strcmp("NOR", itemString[1 + locStrIndex]) == 0)          // Overflow
    {
        aiPoint->pointOver    = SET;
    }
    else       
    {      
        if(strcmp("3FF", itemString[1 + locStrIndex]) == 0)           // 3FF 인경우 .. 이벤트
        {
            if(opr->dnpDebug == devid)  Debug(console,"=> SS-UP  Event\n"); 

            aiPoint->locEvent   = SET;
            dnp->locEvent       = SET;
        }
        else
        {      
            tmpValue = atoi(itemString[1 + locStrIndex]);
            aiPoint->locData = tmpValue * 10;
        
            /* -------------------------------------------------------- */
            /* SS-UP Line Limit Check.... [0...5] [modBase... modIndex] */
            /* -------------------------------------------------------- */
            if((( tmpValue < lowLimit) || ( tmpValue > highLimit)) && ((tmpValue < 0) || (tmpValue > 5)))
            {
                dnp->locEvent       = SET;
                aiPoint->limitEvent = SET;
            }
            
            if(opr->dnpDebug == devid)  Debug(console,"=> SS-UP Data = %d \n", aiPoint->locData); 
        }
    }

    /* ------------------------------------ */
    /*  2. SS DOWN Line 처리                  */
    /* ------------------------------------ */
    aiPoint = (POINT_BUF *) &dnp->aiPtBuf[8];           // SS-DOWN Line
    lowLimit  = aiPoint->modBase;
    highLimit = aiPoint->modIndex;
    
    aiPoint->pointComFail = 0;
    aiPoint->pointOver    = 0;
    aiPoint->locEvent     = 0;
    aiPoint->locData      = 0;   
    aiPoint->locEvent     = 0; 
    aiPoint->limitEvent   = 0; 
            
    if(strcmp("FFF", itemString[2 + locStrIndex]) == 0)               // 통신이상
    {
        aiPoint->pointComFail = SET;
    }
    else if(strcmp("NOR", itemString[2 + locStrIndex]) == 0)          // Overflow
    {
        aiPoint->pointOver    = SET;
    }
    else       
    {      
        if(strcmp("3FF", itemString[2 + locStrIndex]) == 0)           // 3FF 인경우 .. 이벤트
        {
            if(opr->dnpDebug == devid)  Debug(console,"=> SS-DOWN  Event\n"); 

            aiPoint->locEvent = SET;
            dnp->locEvent     = SET;
        }
        else
        {       
            tmpValue = atoi(itemString[2 + locStrIndex]);
            aiPoint->locData = tmpValue * 10;
        
            /* -------------------------------------------------------- */
            /* SS-DN Line Limit Check.... [0...5] [modBase... modIndex] */
            /* -------------------------------------------------------- */
            if((( tmpValue < lowLimit) || ( tmpValue > highLimit)) && ((tmpValue < 0) || (tmpValue > 5)))
            {
                dnp->locEvent       = SET;
                aiPoint->limitEvent = SET;
            }
            if(opr->dnpDebug == devid)   Debug(console,"=> SS-DOWN Data = %d \n", aiPoint->locData); 
        }
    }
                
    /* ------------------------------------ */
    /*  3. AT# Line 처리                    */
    /* ------------------------------------ */
    aiPoint = (POINT_BUF *) &dnp->aiPtBuf[9];           // SS-DOWN Line
    
    for(i=0; i < 6; i++)
    {
        for(j=0; j < 2; j++)
        {
            strinx = i*2 + j + 3 + locStrIndex;     // 데이터의 String index 
            lowLimit  = aiPoint->modBase;
            highLimit = aiPoint->modIndex;
            
            aiPoint->pointComFail = 0;
            aiPoint->pointOver    = 0;
            aiPoint->locData      = 0;
            aiPoint->locEvent     = 0;  
            aiPoint->limitEvent   = 0;
                
            if(strcmp("FFF", itemString[strinx]) == 0)               // 통신이상
            {
                aiPoint->pointComFail = SET;
            }
            else if(strcmp("NOR", itemString[strinx]) == 0)          // Overflow
            {
                aiPoint->pointOver    = SET;
            }
            else       
            {      
                if(strcmp("3FF", itemString[strinx]) == 0)           // 3FF 인경우 .. 이벤트
                {
                    if(opr->dnpDebug == devid)
                    {      
                        if(j==0)    Debug(console,"=> AT#%d - UP    Event\n", i+1); 
                        else        Debug(console,"=> AT#%d - Down  Event\n", i+1);     
                    }
                    dnp->locEvent = SET;
                    aiPoint->locEvent     = SET;  
                }
                else
                {       
                    tmpValue = atoi(itemString[strinx]);
                    aiPoint->locData = tmpValue * 10;
        
                    /* -------------------------------------------------------- */
                    /* SS-UP Line Limit Check.... [0...20] [modBase... modIndex] */
                    /* -------------------------------------------------------- */
                    if((( tmpValue < lowLimit) || ( tmpValue > highLimit)) && ((tmpValue < 0) || (tmpValue > 20)))
                    {
                        dnp->locEvent        = SET;
                        aiPoint->limitEvent  = SET;   
                    }
                    if(opr->dnpDebug == devid)
                    {     
                        if(j==0)    Debug(console,"=> AT#%d - UP    Data = %d \n", i+1, aiPoint->locData); 
                        else        Debug(console,"=> AT#%d - Down  Data = %d \n", i+1, aiPoint->locData);     
                    }
                }
            }
            
            /* AI Point 증가... */
            aiPoint++;
        }
    }   
                        
    return (dnp->locEvent);
}

/*
*   고장점 데이터 처리...
*/
int rcvHandler_LOC(int devid, int dataLen, byte *rxbuf)
{
    int     i, j, inx;
    int     stringNum = 0;
    int     ptinx = 0;
    int     index = 0;
    int     retValue;
    char    itemString[16][16];
    int     tmp1,tmp10,tmp100,tmp1000;
    int     year, month, day, hour, min, sec;
    //int     value, aiIndex;
    
    int     adata;
    float   fdata, aiScale;
    
    DNP_DEVICE  *dnp;
    POINT_BUF   *aiPoint;
    
    dnp = (DNP_DEVICE *) dnpcfg[devid];
    
    /* ------------------------------------------------ */
    /*  Packet 내 정보객체별 구분... (스페이스 로 구분)             */
    /* ------------------------------------------------ */
    for(i=0; i < dataLen; i++)
    {
        if(rxbuf[i] == 0x20)
        {
            itemString[ptinx][index] = '\0';
            //prndat("* ", itemString[ptinx], strlen( itemString[ptinx]));

            ptinx++;
            index = 0;
            stringNum++;
            continue;
        }
        else if(rxbuf[i] == 0x03)
        {
            itemString[ptinx][index] = '\0';
            //prndat("* ", itemString[ptinx], strlen( itemString[ptinx]));

            ptinx++;
            index = 0;
            stringNum++;
            break;
        }
        
        itemString[ptinx][index++] = rxbuf[i];
    }
    
    /* --------------------------------------------------- */
    /*  고장점 장치 TYPE#1 : 년/월/일/시/분/초 정보 포함                */
    /*  고장점 장치 TYPE#2 : 년/월/일/시/분/초 정보 포함 않함             */
    /* --------------------------------------------------- */
    if(dnp->type == TYPE_LOCATOR1)
    {       
        /* --------------------------- */
        /*  수집 년월일-시분초 정보 추출 */
        /* --------------------------- */
        tmp1000= itemString[0][0] - 0x30;
        tmp100 = itemString[0][1] - 0x30;
        tmp10  = itemString[0][2] - 0x30;
        tmp1   = itemString[0][3] - 0x30;
        year   =  tmp1000*1000 + tmp100*100 + tmp10*10 + tmp1;
  
        tmp10  = itemString[0][4] - 0x30;
        tmp1   = itemString[0][5] - 0x30;
        month  =  tmp10*10 + tmp1;
    
        tmp10  = itemString[0][6] - 0x30;
        tmp1   = itemString[0][7] - 0x30;
        day    =  tmp10*10 + tmp1;
    
        tmp10  = itemString[0][8] - 0x30;
        tmp1   = itemString[0][9] - 0x30;
        hour   =  tmp10*10 + tmp1;
    
        tmp10  = itemString[0][10] - 0x30;
        tmp1   = itemString[0][11] - 0x30;
        min    =  tmp10*10 + tmp1;
    
        tmp10  = itemString[0][12] - 0x30;
        tmp1   = itemString[0][13] - 0x30;
        sec    =  tmp10*10 + tmp1;
    
        /* ------------------------------------ */
        /*  수집 로그정보 처리  (미보고)        */
        /* ------------------------------------ */    
        update_LOC_DATE(devid, 0, year);      // ANALOG #1 년도 정보
        update_LOC_DATE(devid, 1, month);     // ANALOG #1 년도 정보
        update_LOC_DATE(devid, 2, day);       // ANALOG #1 년도 정보
        update_LOC_DATE(devid, 3, hour);      // ANALOG #1 년도 정보
        update_LOC_DATE(devid, 4, min);       // ANALOG #1 년도 정보
        update_LOC_DATE(devid, 5, sec);       // ANALOG #1 년도 정보
    
        if(opr->dnpDebug == devid)
        {       
            Debug(console, "LOC-TYPE#1 : Check String ... %d\n",  stringNum);   
            Debug(console, "* rcv RTC... %4d/%02d/%02d-%02d:%02d:%02d\n", year, month, day, hour, min, sec);
        }
    }
    else
    {
        /* --------------------------- */
        /*  수집 년월일-시분초 정보 추출 */
        /* --------------------------- */
        year   =  rtc->year;
        month  =  rtc->month;
        day    =  rtc->day;
        hour   =  rtc->hour;
        min    =  rtc->min;
        sec    =  rtc->sec;
    
        /* ------------------------------------ */
        /*  수집 로그정보 처리  (미보고)        */
        /* ------------------------------------ */    
        update_LOC_DATE(devid, 0, year);      // ANALOG #1 년도 정보
        update_LOC_DATE(devid, 1, month);     // ANALOG #1 년도 정보
        update_LOC_DATE(devid, 2, day);       // ANALOG #1 년도 정보
        update_LOC_DATE(devid, 3, hour);      // ANALOG #1 년도 정보
        update_LOC_DATE(devid, 4, min);       // ANALOG #1 년도 정보
        update_LOC_DATE(devid, 5, sec);       // ANALOG #1 년도 정보
            
        if(opr->dnpDebug == devid)
        {       
            Debug(console, "LOC-TYPE#2 : Check String ... %d\n",  stringNum);   
            Debug(console, "* rcv RTC... NULL\n");
        }
    }       
   
    /* -------------------------------------------- */
    /* LOC 이벤트 생성... Check                     */
    /* -------------------------------------------- */
    retValue = check_LOC_Event(devid, (char *) itemString[0]);

    /* -------------------------------------------- */
    /* LOC 이벤트 발생시 ... 고장점 정보 생성       */
    /* -------------------------------------------- */
    for(i=0, inx=0; i < dnp->devAiPoint; i++)
    {
        aiPoint = (POINT_BUF *) &dnp->aiPtBuf[i];
        if(aiPoint->config == 0)    continue;

        /* ---------------------------------------- */
        /* Locator 아날로그 정보 Update...          */
        /* ---------------------------------------- */
        if(dnp->locEvent)   adata = aiPoint->locData;
        else                adata = 0;
                        
        fdata = (float ) adata;
        
        /* ---------------------------------------- */
        /*  계전기별 포인트 지정 => 스케일링...     */
        /* ---------------------------------------- */
        if(aiPoint->ptConfig)
        {   
            aiScale = 1.0;
            for(j=0; j < aiPoint->ptConfig; j++) aiScale = (aiScale * 0.1);
            fdata = (aiScale * adata);
        }
        	
      	if(opr->msgDebug == devid)
        {
       		Debug(console,"(1)%4.2f ", fdata);
       		if((++inx % 8) == 0)  Debug(console,"\n");
        }
        			        			
      	/* ------------------------------------ */
       	/* Analog Data Assign into Buffer ...   */
        /* ------------------------------------ */
        storeAI_DNP( devid, i, fdata, (byte *)&fdata);    
    }
    
    /* ---------------------------------------- */
    /*  STATUS 정보 Updaqte...                  */
    /* ---------------------------------------- */
    update_LOC_DI(devid);
                  
    return (0);    
    
}

/*
*
*/
void locator_initial(int devid)
{
    int     i;
    //int     hostid, devNo;
    int     adata;
    float   fdata;
    byte    stsbuf[8];
    
    DNP_DEVICE      *dnp;
    POINT_BUF   *aiPoint, *diPoint;
    
    //DIPOINT_INFO    *hostSts;
    //AIPOINT_INFO    *hostAna;
    //DNP_ANA_INPUT   *dnpAna; 
    //HOST_DCB        *host;
    
    
    dnp = (DNP_DEVICE *) dnpcfg[devid];
    
    dnp->locEvent = 0;

    for(i=0; i < dnp->devDiPoint; i++)
    {
        diPoint = (POINT_BUF *) &dnp->diPtBuf[i];
        if(diPoint->locStatus == 1)    create_Locator_SOE(devid, i, RESET);
        diPoint->locStatus  = 0;
    }
    
    bzero8248(stsbuf, 8);
    memcpy(dnp->cursts, stsbuf, 5);

    /* ------------------------------------ */                    
    /* 상위 HOST Buffer Update...           */
    /* ------------------------------------ */      
    curStore_BIT_DNP( devid, &dnp->cursts[0], 5);
    
    /* -------------------------------------------- */
    /* LOC 이벤트 발생시 ... 고장점 정보 생성       */
    /* -------------------------------------------- */
    for(i=0; i < dnp->devAiPoint; i++)
    {
        aiPoint = (POINT_BUF *) &dnp->aiPtBuf[i];
        if(aiPoint->config == 0)    continue;

        aiPoint->pointData  = 0;
        aiPoint->floatData  = 0;
        aiPoint->pointOver  = 0;
        aiPoint->locEvent   = 0;
        aiPoint->limitEvent = 0;
        aiPoint->locData    = 0;
        
        /* ---------------------------------------- */
        /* Locator 아날로그 정보 Update...          */
        /* ---------------------------------------- */
        adata = 0;
        fdata = (float ) adata;
        
      	/* ------------------------------------ */
       	/* Analog Data Assign into Buffer ...   */
        /* ------------------------------------ */
        storeAI_DNP( devid, i, fdata, (byte *)&fdata);    
    }
    
    if(opr->dnpDebug == devid)
    Debug(console, ">> Locator[dev=%d] - Buffer Cleared !\n", devid); 
}


/*
*
*/
void locator_Data_initial(int devid)
{
    int     i;
    int     hostid, devNo;
    
    DNP_DEVICE      *dnp;
    POINT_BUF   *aiPoint, *diPoint;
    
    DIPOINT_INFO    *hostSts;
    AIPOINT_INFO    *hostAna;
    DNP_ANA_INPUT   *dnpAna; 
    HOST_DCB        *host;
    
    
    dnp = (DNP_DEVICE *) dnpcfg[devid];
    
    dnp->locEvent = 0;

    for(i=0; i < dnp->devDiPoint; i++)
    {
        diPoint = (POINT_BUF *) &dnp->diPtBuf[i];
        diPoint->locStatus  = 0;
    }
    
    for(i=0; i < dnp->devAiPoint; i++)
    {
        aiPoint = (POINT_BUF *) &dnp->aiPtBuf[i];
        
        aiPoint->pointData  = 0;
        aiPoint->floatData  = 0;
        aiPoint->pointOver  = 0;
        aiPoint->locEvent   = 0;
        aiPoint->limitEvent = 0;
        aiPoint->locData    = 0;
    }


    /* ---------------------------------------- */
    /*  DNP HOST : 계전기 포인트 상태 OFFLINE   */    
    /* ---------------------------------------- */            
    for(hostid = 0; hostid < MAX_HOST; hostid++)
    {
        host = (HOST_DCB *) hostDCB[hostid];
        
        if(host->hostDualMode == HOST_NOT_USE)	continue;
        if(host->hostProtocol != HOST_DNP)  continue;
            
        /* ---------------------------------------- */
        /*  개별장치별 현재상태 처리 (Offline)      */
        /* ---------------------------------------- */
        for(i = 0; i < MAX_DNP_DI_POINT; i++)
        {
            hostSts = (DIPOINT_INFO *) &host->stateInfo[i];
            if(hostSts->config == RESET)    continue;
            
            devNo = hostSts->devNo;
            
            /* OFFLINE 이 발생한 Device 번호 */
            if(devid == devNo)  
            {
                host->sts_pointData[i] |= PT_FLAG_ONLINE;      // 포인트 상태를 Offline으로...
            }
        }
        
        /* ---------------------------------------- */
        /*  개별장치별 현재계측값 처리 (Offline)    */
        /* ---------------------------------------- */
        for(i = 0; i < MAX_DNP_AI_POINT; i++)
        {
            hostAna = (AIPOINT_INFO *) &host->analogInfo[i];
            if(hostAna->config == RESET)    continue;
            
            devNo    = hostAna->devNo;
            
            /* OFFLINE 이 발생한 Device 번호 */
            if(devid == devNo)
            {
                dnpAna = (DNP_ANA_INPUT *) &host->ana_pointData[i];
                dnpAna->flag = PT_FLAG_ONLINE;
            }
        }            
    }
        
    
}


/* ------------------------------------------------------- */
/*  SCAN THREAD Start Routine ....              */
/*  - ASYNC 용 타스크                           */
/* ------------------------------------------------------- */
void    *scan_locator_thread(int  *arg)
{
    int     i,scanid;
    int		comPort;
    int     devid, devIndex;
    int     retVal;
    int     oldhour=0, oldsec=0;
    int		taskDelay;
    int     startDev, stopDev;
    int     vmeChanActive;
    int     comFailMin;
    int     dataIndex, dataLen;
    int     timeout;
    
    byte    rxcnt;
    
    word        locTick; 
    word        waitTick; 
    word        rtyTick; 
    byte    *txbuf, *rxbuf;
    char    buffer[128];
    
    //int    	timedelay;
    
    DNP_DEVICE      *dnp=NULL;
	TTY_DESC        *scanPort=NULL;
	SCAN_CONFIG     *scan=NULL;
	//struct timeval         tv;
	
	/* -------------------------------------------- */
	/* THREAD Argment Assign ...                    */
	/* -------------------------------------------- */
	scanid = (int ) *arg;

    /* SCAN-TASK 별 모듈구성 정보 */ 
    scan = (SCAN_CONFIG *) scanCfg[scanid];
    comPort = scan->comPort;
    vmeChanActive = RESET;
    
    /* -------------------------------------------- */
    /* SCAN THREAD - 통신 Delay 설정                */
    /* -------------------------------------------- */
    taskDelay = scan->comDelay * 10;
    if(taskDelay < 5)		taskDelay = 50;
    if(taskDelay > 2000)	taskDelay = 2000;
   
    pause(1000);
     	
    /* -------------------------------------------- */
    /* SCAN THREAD - MASTER : 통신관련 변수 초기화  */
    /* -------------------------------------------- */
    if(comPort < MAX_CPUPORT)
    {
        scanPort = (TTY_DESC *) &scanChan[comPort];
        
        /* COM1 통신속도 반영 */
        if(scan->comSpeed == SPEED_1200)        scanPort->baudRate = 1200;
        else if(scan->comSpeed == SPEED_2400)   scanPort->baudRate = 2400;
        else if(scan->comSpeed == SPEED_4800)   scanPort->baudRate = 4800;    
        else if(scan->comSpeed == SPEED_9600)   scanPort->baudRate = 9600;
        else if(scan->comSpeed == SPEED_19200)  scanPort->baudRate = 19200;
        else if(scan->comSpeed == SPEED_38400)  scanPort->baudRate = 38400;
        else if(scan->comSpeed == SPEED_57600)  scanPort->baudRate = 57600;
        else                                    scanPort->baudRate = 38400;
        
        retVal = PortInitial((TTY_DESC *) scanPort);
        if(retVal == -1)
        {
            termExec = 0;
            Debug(console, "\n===========================================\n");
            Debug(console,"scan> *Serial Com Port Open...ERROR...return = %d \n", retVal);
            Debug(console,"===========================================\n");
            pause(1000);
            pause(1000);
        }
        
        startDev = 0;
        stopDev  = scan->devNum;
        
        for(i = startDev; i < stopDev; i++)
        {
            devid = scan->device[i] & MAX_MODULE_MASK;
            dnp = (DNP_DEVICE *) dnpcfg[devid];
            
            dnp->ttyPort = (TTY_DESC *) scanPort;
            dnp->offCount = scan->offCount;
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
            
            dnp->vmeChan = (VME_CHAN_DCB *) vmeChanDCB[comPort - VME_COM1];
            dnp->ttyPort = NULL;
            
            /* VMECHAN 프로토콜 타입 지정 */
            vmeChanActive = SET;
            dnp->vmeChan->protocolType = SCAN_ASYNC;
            dnp->vmeChan->cfgSpeed     = scan->comSpeed;
            dnp->vmeChan->cfgType      = COM_RS232;
            
            dnp->vmeChan->rxFront   = 0;
            dnp->vmeChan->rxRear    = 0;
            
            dnp->offCount = scan->offCount;
        }                
        
        vmeScanInitial(scanid, comPort - VME_COM1);
    }        
    else
    {
 	    while(termExec)
	    {
	        if(opr->wdtDebug)
	        Debug(console,">>> *Invalid SCAN[%d] COM-PORT  ... !\n", scanid+1);
	        
	        opr->scanWDT[scanid] = 0;
	        pause(1000);
	    }        
    }

    printf(" %s SCAN[%02d] LOCATOR-PROCESS Activated...[TYPE = %d], (COM-PORT=%02d, Delay = %d ms, Offline Count=%d) !\n", TARGET_NAME, scanid+1, (dnp->type - TYPE_DNP_SIEMENS), scan->comPort, taskDelay, scan->offCount);
    
    sprintf(buffer, "%s SCAN[%02d] LOCATOR-PROCESS Activated...[TYPE = %d] (COM-PORT=%02d, Delay = %d ms, Offline Count=%d) !", TARGET_NAME, scanid+1, (dnp->type - TYPE_DNP_SIEMENS), scan->comPort, taskDelay, scan->offCount);
    LogFile_MPU ( opr, NOT_DEFINED, buffer, strlen(buffer));
    
#if 1
    Debug(console,"SCAN[%02d] device Num = %02d ... ", scanid+1, scan->devNum );
    for(i=0; i< scan->devNum; i++)  
    {
        Debug(console," %2d", scan->device[i]);
    }
    Debug(console,"\n");
#endif

    /* -------------------------------------------------------- */
    /* 등록된 Device 가 없는 경우                               */
    /* -------------------------------------------------------- */
    while((scan->devNum == 0) || (scan->active == 0))   
	{
	    if(termExec == 0)   break;

        if(opr->wdtDebug)	Debug(console,"SCAN%2d> *** No Assign DEVICE ...!\n", scanid+1);
        opr->scanWDT[scanid]    = 0;
        opr->scanComSts[scanid] = 2;         // 미정의

        pause(1000);
	}
	
	scan->scaninit = 4;
	            
    devIndex    = 0;		// 쯔다 고장점은 Multi 구성이 않됨.... 필수적으로 1:1 구성
    comFailMin  = 0;

    /* ---------------------------------------- */
    /* 현재 선택된 계전기 : 송수신...           */
    /* ---------------------------------------- */
    devid = scan->device[devIndex] & MAX_MODULE_MASK;
    dnp = (DNP_DEVICE *) dnpcfg[devid];
    
    if((dnp->type == TYPE_LOCATOR1) || (dnp->type == TYPE_LOCATOR2))   
	{
	}
	else
	{
	    while(termExec)   
	    {
            if(opr->wdtDebug)	Debug(console,"SCAN%2d> *** No Assign LOCATOR... DEVICE ...!\n", scanid+1);
            opr->scanWDT[scanid]    = 0;
            opr->scanComSts[scanid] = 2;         // 미정의

            pause(1000);
	    }
	    
	}
	
	/* ---------------------------------------- */
	/*  LOCATOR 계전기 - 정보 초기화            */  
	/* ---------------------------------------- */
    txbuf = (byte *) &dnp->sndBuffer[0];
    rxbuf = (byte *) &dnp->rcvBuffer[0];
    
    locator_Data_initial(devid);

#if 0
    /* -------------------------------------------- */
    /* 고장점 초기 상태 = ONLINE                    */
    /* -------------------------------------------- */
    setOnline(devid);
#endif
    
    if(dnp->type == TYPE_LOCATOR1)
  	{  	
    	dnp->initialLoc = SET;
    	dnp->locMode = LOC_ENQ_SEARCH;
    	dnp->txCount = 0;
    	dnp->rxCount = 0;
    	oldhour = rtc->hour;
    }
    else
    {  	
    	dnp->initialLoc = 0;
    	dnp->locMode = LOC_ENQ_SEARCH;
    	dnp->txCount = 0;
    	dnp->rxCount = 0;
    	oldhour = rtc->hour;
    }
    
    locTick = 0;
    waitTick = 0;
    rtyTick  = 0;
    
	while(termExec)
	{
	    /* -------------------------------------------- */
	    /*  계전기별 통신 지연                          */
	    /* -------------------------------------------- */
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
			comFailMin = 0;             // 통신이상 Clear
            dnp->comFailTick = 0;
            
            /* ------------------------ */
            /* 모듈별 Time out 처리     */
            /* ------------------------ */
            if(vmeChanActive == RESET)
			{
	            for(timeout = 0; timeout < 20; timeout++)
    	        {
        	        usleep(2000);            
            	    rcvWithLOC(scanid, devid, rxbuf);
            	}
            }	
            
			continue;
		}

	    /* ---------------------------------------- */
        /* 현재 선택된 계전기 : 송수신...           */
        /* ---------------------------------------- */
        devid = scan->device[devIndex] & MAX_MODULE_MASK;
        dnp = (DNP_DEVICE *) dnpcfg[devid];
        if(dnp->scan == RESET)   continue;    

        /* 고장점 장치인 경우.... */
        if((dnp->type != TYPE_LOCATOR1) && (dnp->type != TYPE_LOCATOR2))   continue;
        opr->devScanCount[scanid]++;
        
        /* ---------------------------------------- */    
        /* Data receive Time out ... 5 sec */
        /* ---------------------------------------- */
        if(dnp->locMode == LOC_DATA_READY)
        {
            if(++waitTick  > 50)
            { 
                waitTick = 0;
                if(opr->dnpDebug == devid)
                Debug(console, ">> Locator[dev=%d] - Data Time out !\n", devid);
                
                sendNACK(scanid, devid, txbuf); 
                if(++rtyTick > 5)
                {
                    waitTick = 0;
                    rtyTick  = 0;
                    dnp->initialLoc = SET;
                    dnp->locMode = LOC_ENQ_SEARCH;          
                }
            }
        }
        
        /* ------------------------------------------------ */
        /*  초기기동시와 제어시... POLL 수행...             			*/
        /* ------------------------------------------------ */
        if((dnp->initialLoc == SET) || (dnp->selectReq == SET))
        {
            /* 2018년 5월 17일...Poll 수행시... ComFailMin Clear */
            comFailMin = 0;             // 통신이상 Clear
            
            /* self Request Time -> 10 sec .. */
            if(++locTick > 50)
            { 
                /* 데이터 초기화 */
                locator_initial(devid);
            
            	/* 이벤트 설정상태 ... Clear */
                if(dnp->selectReq == SET)
                {
                    create_Locator_SOE(devid, 6, 0);
                    create_Locator_SOE(devid, 7, 0);
                    create_Locator_SOE(devid, 8, 0);
                }
                      
                locTick = 0;
                
                /* -------------------------------------------- */
                /*  POLL 명령을 주는 방법...                    */
                /* -------------------------------------------- */
                if(dnp->type == TYPE_LOCATOR1)
            	{   
            	    if(opr->dnpDebug == devid)
            	    Debug(console, ">> Locator-TYPE#1 [dev=%d] - Poll send !\n", devid); 	      
            	    
	                sendPoll(scanid, devid, txbuf); 
  	                dnp->locMode = LOC_ENQ_SEARCH;          
    	            dnp->txCount++;

      	            dnp->initialLoc = 2;
        	        dnp->selectReq  = 0;
          		}
          		/* -------------------------------------------- */
                /*  초기 기동시에만... POLL 명령...                    */
                /* -------------------------------------------- */
          		else if((dnp->type == TYPE_LOCATOR2) && (dnp->initialLoc == SET))
            	{   
            	    if(opr->dnpDebug == devid)
            	    printf(">> Locator-TYPE#2 [dev=%d] - INIT-Poll send !\n", devid);  	   
            	       
	                sendPoll(scanid, devid, txbuf); 
  	                dnp->locMode = LOC_ENQ_SEARCH;          
    	            dnp->txCount++;

      	            dnp->initialLoc = 2;
        	        dnp->selectReq  = 0;
          		}
          			
          		dnp->selectReq  = 0;      
                waitTick = 0;
                rtyTick  = 0;
                
            }
        }
        
        /* receive ENQ */
        if(dnp->locMode == LOC_ENQ_SEARCH)
        {
            if((rxcnt=checkENQ(scanid, devid, rxbuf)) > 0)
            {
                if(opr->dnpDebug == devid)
                {
                    Debug(console, ">> Locator[dev=%d] - search ENQ O.K !\n", devid);
                    DumpBuff(console, "RXD: ", rxbuf, rxcnt);
                }
                 
                sendACK(scanid, devid, txbuf);
                waitTick = 0;
                locTick  = 0;
                rtyTick  = 0;
                dnp->locMode = LOC_DATA_READY;        
            }

            /* ---------------------------- */
            /* POLL 재전송                  */ 
            /* ---------------------------- */           
            if(dnp->initialLoc == 2)
            {      
                if(++locTick > 300)     // 30초 
                { 
                    locTick = 0;
                    if(dnp->type == TYPE_LOCATOR1)		dnp->initialLoc = SET;
                    dnp->locMode = LOC_ENQ_SEARCH;
                }
            }
        }       
        else if(dnp->locMode == LOC_DATA_READY)
        {   
            if((rxcnt = rcvWithLOC(scanid, devid, rxbuf)) > 0)
            {
                sendACK(scanid, devid, txbuf);
                
                comFailMin = 0;             // 통신이상 Clear
                dnp->rxCount++;
                dnp->locMode = LOC_ENQ_SEARCH; 
                dnp->initialLoc = 0;
                
                waitTick = 0;
                locTick  = 0;
                rtyTick  = 0;
                
                if(opr->dnpDebug == devid)
                {
                    Debug(console, ">> Locator[dev=%d] - rcv Msg O.K !\n", devid);
                    DumpBuff(console,"RXD: ", rxbuf, rxcnt);
                }
                
                dataIndex = 1;
                dataLen   = 0;
                
                /* -------------------------------------------- */
                /*  수신 패킷에 대한 처리... (0x02 다음의 space는 스킵..)    */
                /* -------------------------------------------- */
                for(i=1; i < rxcnt; i++)
                {
                    if(rxbuf[i] != 0x20)
                    {
                        dataIndex = i;
                        dataLen = rxcnt - dataIndex;
                        
                        rcvHandler_LOC(devid, dataLen, &rxbuf[dataIndex]);
                        break;
                    }      
                }
                 
                if(dnp->online != SET)   setOnline(devid);
                dnp->online = SET;
            }
        }          
        
        if(oldsec == rtc->sec)  continue;
        oldsec = rtc->sec; 
        
        /* ------------------------------------ */
        /* TYPE2 (모란) : 주기적인 Poll 생략    */
        /* ------------------------------------ */
        if(dnp->type == TYPE_LOCATOR2)	
       	{
       		/* ------------------------------------ */
        	/* 고장점 장치 통신이상 Check......     */ 
	        /* ------------------------------------ */       
    	    if(oldhour == rtc->hour)  continue;
        	oldhour = rtc->hour;
        
        	if(rtc->hour == 9)
      		{
				/* VMECHAN 프로토콜 타입 지정 */
                if(vmeChanActive == SET)    
                {
                    vmeScanChanReset(devid);
                    dnp->vmeChan->protocolType = SCAN_ASYNC;
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
                
                dnp->locMode = LOC_ENQ_SEARCH;
                
                Debug(console, ">> Locator[dev=%d] - PORT Initial...!\n", devid);
      		}
      		  		
       		continue;
       	}
       	
        	
        /* 1 시간 단위 Check */     
        if(dnp->initialLoc)
        {
            if(++comFailMin > 120)
            {
                comFailMin = 0;
                
                /* VMECHAN 프로토콜 타입 지정 */
                if(vmeChanActive == SET)    
                {
                    vmeScanChanReset(devid);
                    dnp->vmeChan->protocolType = SCAN_ASYNC;
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
            
            	Debug(console, ">> Locator[dev=%d] - PORT Initial...!\n", devid);
            
                /* 계전기 Offline 처리 */
                if(dnp->online != RESET)    setOffline(devid);
                dnp->online = RESET;
            }
        }
        else
        {
            if(++comFailMin > 3800)
            {
                comFailMin = 0;
                
                /* VMECHAN 프로토콜 타입 지정 */
                if(vmeChanActive == SET)    
                {
                    vmeScanChanReset(devid);
                    dnp->vmeChan->protocolType = SCAN_ASYNC;
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
				
				Debug(console, ">> Locator[dev=%d] - PORT Initial...!\n", devid);
				            
                /* 계전기 Offline 처리 */
                if(dnp->online != RESET)    setOffline(devid);
                dnp->online = RESET;
            }
        }
        
        /* ------------------------------------ */
        /* 고장점 장치 통신이상 Check......     */ 
        /* ------------------------------------ */       
        if(oldhour == rtc->hour)  continue;
        oldhour = rtc->hour;
        
        dnp->initialLoc = SET;
        comFailMin = 0;
        locTick = 0;
        
	}/* while */

    return (0);
}
