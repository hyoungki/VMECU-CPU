#include	"localLib.h"
#include    "external.h"

extern  int     storeAI_DNP(int devid, int point, float fdata, byte *lowdata);
extern  int     storeAI_FLOAT_DNP(int devid, int point, float fdata, byte *lowdata);

extern  int     curStore_BIT_DNP( int devid, byte *stsbuf, int dataSize);
extern  int     storeSOE_CU_DNP( int devid, int devPoint, int state, byte *buf);
extern  int     storeCOS_CU_DNP( int devid, int point, int state);

extern  int     setOffline( int devid);
extern  int     setOnline( int devid);

extern  int             termExec;
extern  TASK_INFO	    *taskPtr;

extern  PDPDTIME_DATE_TIME      dnpRTC;
extern  PDPDTIME_MS_SINCE_70    dnpRTCInfo;

/* 내부 기능모듈 */
extern  VME_MODULE      *vmeModule[MAX_MODULE];

/* IOMODULE BUS 관련 */
extern  void    *iombus_ptr ;                                   // Function 모듈 Base Addr
extern  word    soeTimer;

/*
 *  DI Module
 */
//static  uint    ditime;
//static  byte    cursts,coschk;
//static  byte    debounce;
//static  byte    costick;

static	byte	bitcode[8] = {0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80};

static char modType[20][12]    = { {"NULL      "},{"DIM-32    "}, {"DOM-32    "}, {"AIM-32    "}, {"DIOM-8    "}, 
                                   {"DNP-HIMAP "},{"DNP-GIPAM "}, {"DNP-FRTU  "}, {"DNP-PROPAC"}, {"DNP-VIPAM "},
                                   {"MODBUS(SM)"},{"MODBUS(LG)"}, {"RSEERVED1 "}, {"RESERVED2 "}, {"RESERVED3 "},{"TUNNEL   "}};
                                    
/*
*   VME Module 초기화 
*/
int  vmeModuleInit(int scanid)
{
    int i;
    
    DNP_DEVICE  *dnp;
    VME_MODULE  *iom;
    //VME_DATA    *vData;
    //VME_SOEQ    *vSoeQ;
    //VME_CONTROL *vCntr;    // 모듈부 제어 영역
    
    /* ------------------------------------ */
    /*  VMECU-V2 : VME 기능모듈 탑재형      */
    /*  - 기능모듈 : Processor 탑재형       */
    /*  - 공유메모리 정보 연계              */
    /* ------------------------------------ */
    for(i = 0; i < MAX_MODULE; i++)
    {
        dnp = (DNP_DEVICE *) dnpcfg[i];
        
        /* VME 기능모듈이 아닌경우...continue */
        if((dnp->type != TYPE_DIM) && (dnp->type != TYPE_DOM) && (dnp->type != TYPE_AIM))  continue;
     
        dnp->vmeInitial = RESET;
        dnp->vmeChkFlag = SET;
        dnp->initial	= RESET;        // 장치별 초기화 
        
        if(opr->runMode == LOCAL_SLAVE)     continue;
               
        /* ------------------------------------ */
        /* VME BUS 상의 모듈 포인터 & 초기화    */
        /* ------------------------------------ */
        iom = (VME_MODULE *) vmeModule[i];
        
        iom->soeFlag    = 0;
        iom->cfgError   = 0;
        iom->cpuActCount= 0;
        iom->timeSyncFlag = 0;
        
        iom->resetFlag  = SET;          // 모듈부 재기동...
        
    }
    
    Debug(console,"scan%2d> VME Module Config End...!\n", scanid + 1);
    
    return (0);    
}

/*
* 상태감시모듈 (DIM) : 주기적 Status Dump
*/
int curStore_vmeDI(int ioid, DNP_DEVICE *dnp, VME_MODULE *iom)
{
    int     index1, index2;
    int     point;
    byte    stsbuf[16];

    VME_DATA    *vdata;

    if(iom->initState == RESET) 
    {
        if(opr->dnpDebug == ioid) 
        Debug(console,"vme%02d> DIM...Not initial...return !\n", ioid+1);
        return (0);
    }
    
    /* 모듈 타입 비교... */
    if(dnp->type != iom->modType)
    {
        if(opr->dnpDebug == ioid) 
        Debug(console,"vme%02d> DIM stsdump...*invalid Module Type (%04x) !\n", ioid+1, iom->modType);
        return (0);
    }

    /* -------------------------------------------- */
    /* 	LBS-RTU : Local-Master 인경우... 제어실행   */
    /* -------------------------------------------- */
    if(opr->runMode == LOCAL_SLAVE)
    {
        return (0);
    }   
            
    vdata = (VME_DATA *) &iom->modData;
    
    bzero8248(stsbuf, 16);

    /* ---------------------------------------- */
    /*  ACE_LBS_4U    : DI 포인트 16/Module     */
    /*  LBS_6U, VMECU : DI 포인트 32/Module     */
    /* ---------------------------------------- */
#ifdef  ACE_LBS_4U

    /* DIM 상태정보 ...Update 				*/
    for(point = 0; point < 16; point++)
    {
        index1 = point / 8;        
        index2 = point % 8;

        /* 모듈 포인트 상태정보 */        
        if(vdata->pointData[point] == SET)   stsbuf[index1] |= bitcode[index2];
    }
    
    if(opr->msgDebug == ioid) 
    Debug(console,"vme%02d> DI stsdump [%02d:%02d:%02d]...%02x %02x \n", ioid+1, rtc->hour, rtc->min, rtc->sec, stsbuf[0],stsbuf[1]);
    
    memcpy(dnp->cursts, stsbuf, 2);

    /* ------------------------------------ */                    
    /* 상위 HOST Buffer Update...           */
    /* ------------------------------------ */      
    curStore_BIT_DNP( ioid, &dnp->cursts[0], 2);
    
#else
    /* DIM 상태정보 ...Update 				*/
    for(point = 0; point < 32; point++)
    {
        index1 = point / 8;        
        index2 = point % 8;

        /* 모듈 포인트 상태정보 */        
        if(vdata->pointData[point] == SET)   stsbuf[index1] |= bitcode[index2];
    }
    
    if(opr->msgDebug == ioid) 
    Debug(console,"vme%02d> DI stsdump [%02d:%02d:%02d]...%02x %02x %02x %02x\n", ioid+1, rtc->hour, rtc->min, rtc->sec, stsbuf[0],stsbuf[1], stsbuf[2], stsbuf[3]); 
    
    memcpy(dnp->cursts, stsbuf, 4);

    /* ------------------------------------ */                    
    /* 상위 HOST Buffer Update...           */
    /* ------------------------------------ */      
    curStore_BIT_DNP( ioid, &dnp->cursts[0], 4);
#endif
    
    dnp->statusDump = RESET;
    
    return (0);
}


/*
 *  scan DI-POINT
 */
int checkSOE_vmeDI(int ioid, DNP_DEVICE *dnp, VME_MODULE *iom)
{
    byte    front, rear;
    int     point, state, hour, min, sec, milisec;
    VME_SOEQ    *vsoeQ;
    SOE_INFO    *soeInfo;
    byte    soeBuffer[32];
 
    
    if(iom->initState == RESET) 
    {
        if(opr->dnpDebug == ioid) 
        Debug(console,"vme%02d> DIM...Not initial...return !\n", ioid+1);
        return (0);
    }

    /* 모듈 타입 비교... */
    if(dnp->type != iom->modType)
    {
        if(opr->dnpDebug == ioid) 
        Debug(console,"vme%02d> DIM check SOE...*invalid Module Type (%04x) !\n", ioid+1, iom->modType);
        return (0);
    }

    /* ------------------------------------ */
    /*  SOE 상태정보 Update...              */
    /* ------------------------------------ */
    if(iom->soeFlag == RESET) 
    {
        return (0);
    }
    
    vsoeQ = (VME_SOEQ *) &iom->modSoeQ;
    front = vsoeQ->soeFront & 0x3f;
    rear  = dnp->soeRear & 0x3f;
    
    if(opr->msgDebug == ioid)   
    Debug(console,"vme%02d> DI soeQ ... front=%d, rear = %d \n", ioid+1, front, rear);
    
    while(front != rear)
    {
        soeInfo = (SOE_INFO *) &vsoeQ->soeInfo[rear];
        point   = soeInfo->pointNo - 1;
        state   = soeInfo->pointState;
        
        hour    = soeInfo->soeHour;
        min     = soeInfo->soeMin;
        sec     = soeInfo->soeSec;
        milisec = soeInfo->soeMilisec;
        
        rear    = (rear + 1) & 0x3f;
        dnp->soeRear = rear;

        /* ------------------------------------ */
        /*  SOE-POINT 오정보 시...              */
        /* ------------------------------------ */
        if((point < 0) || (point >= dnp->devDiPoint))
        {
            iom->soeFlag = RESET;
            vsoeQ->soeFront = 0;
            dnp->soeRear    = 0;
            
            dnp->vmeInitial = RESET;
            Debug(console,"vme%02d> *** VME BUS Error... MAX DI-point (%d) Re-Initial...!\n", ioid+1, point);
            return (0);
        }

#if 1   
        /* ------------------------------------ */                  
        /*   - DIM Time : SOE Time 으로 정의    */   
        /* ------------------------------------ */
        /* 현재시간 정보를 DNP Form 으로 변환   */
        /* ------------------------------------ */
		dnpRTC.year     = (rtc->year%100) + 2000;
        dnpRTC.month    = rtc->month;
        dnpRTC.day   	= rtc->day;
        dnpRTC.hour     = hour;
        dnpRTC.minute   = min;
        dnpRTC.second   = sec;
        dnpRTC.millisecond= milisec;

        makeDnpTimeByte( &dnpRTCInfo, &dnpRTC);
#endif

#if 0
        /* -------------------------------------------- */
        /* 2014-12-13 장재터널...                       */
        /*   - DIM SOE Time : CPU 시각으로 정의         */
        /* -------------------------------------------- */
        /* 현재시간 정보를 DNP Form 으로 변환           */
        /* -------------------------------------------- */
        time(&tm);
        //localtm = localtime(&tm);
        localtime_r (&tm, &localtm);
        
        gettimeofday(&rv, NULL);
        millisecond = (rv.tv_usec / 1000) % 1000;

        dnpRTC.year       = localtm->tm_year + 1900;
        dnpRTC.month      = localtm->tm_mon + 1;
        dnpRTC.day        = localtm->tm_mday;
        dnpRTC.hour       = localtm->tm_hour;
        dnpRTC.minute     = localtm->tm_min;
        dnpRTC.second     = localtm->tm_sec;
        dnpRTC.millisecond= millisecond;

        makeDnpTimeByte( &dnpRTCInfo, &dnpRTC);
#endif
                    
                    
        soeBuffer[0] = point;           
        soeBuffer[1] = (state << 7) & 0x80;
                
        soeBuffer[2] = (dnpRTCInfo.leastSignificant) & 0xff;
        soeBuffer[3] = (dnpRTCInfo.leastSignificant >> 8) & 0xff;
        soeBuffer[4] = (dnpRTCInfo.mostSignificant) & 0xff;
        soeBuffer[5] = (dnpRTCInfo.mostSignificant >> 8)  & 0xff;
        soeBuffer[6] = (dnpRTCInfo.mostSignificant >> 16) & 0xff;
        soeBuffer[7] = (dnpRTCInfo.mostSignificant >> 24) & 0xff;
                
        if(opr->soeDebug)
        Debug(console,"vme%2d> VME-SOE: pt=%2d, sts=%2x...%4d/%2d/%2d-%02d:%02d:%02d-%03d\n", ioid+1, point, state,
                    rtc->year, rtc->month, rtc->day, hour, min, sec, milisec);
                
        /* -------------------------------- */
        /*  SOE 정보 저장 (for BUS)         */
        /*  - LBS RTU : DIM 이중화...       */
        /* -------------------------------- */
        if(opr->runMode == LOCAL_MASTER)
        {      
            storeSOE_CU_DNP(ioid, point, state, &soeBuffer[2]); 

#if 0            
            /* -------------------------------- */
            /*  COS 정보 저장 (for MODBUS)      */
            /* -------------------------------- */
            storeCOS_CU_DNP( ioid, point, state);
#endif
                
        }
    }
    
    iom->soeFlag = RESET;
    
    return (0);
} 


/*
 *  제어모듈 : Control-Inform 연계 
 */
int check_vmeDO(int ioid, DNP_DEVICE *dnp, VME_MODULE *iom)
{
    word    chksum, ontime;
    word    cntrData[4];    
    char    buffer[128];
    
    VME_CONTROL *vcntr;

    if(iom->initState == RESET) 
    {
        if(opr->dnpDebug == ioid) 
        Debug(console,"vme%02d> DOM...Not initial...return !\n", ioid+1);
        return (0);
    }
    
    /* -------------------------------------------- */
    /* 	LBS-RTU : Local-Master 인경우... 제어실행   */
    /* -------------------------------------------- */
    if(opr->runMode == LOCAL_SLAVE)
    {
        return (0);
    }   
            
    /* -------------------------------------------- */
    /* 	DO 모듈 제어유무 확인 						*/
    /* -------------------------------------------- */
    if(dnp->selectReq == SET)
    {
        /* 상위 제어시간 정보 적용 */
        if((opr->rcvONtime <= 500 ) || (opr->rcvONtime > 10000))    ontime = 500;          // 수신한 제어시간 정보
        else                                                        ontime = opr->rcvONtime;
        
       	/* 제어정보 추출...			*/
        cntrData[0] = dnp->cntPoint + 1;       					// 제어 포인트 (1...16)
        if(dnp->cntTcf == 0x80)         cntrData[1] = 1;        // 제어 상태 (TRIP )
        else if(dnp->cntTcf == 0x40)    cntrData[1] = 2;        // 제어 상태 (CLOSE)
        else                            cntrData[1] = 0;
        	
        cntrData[2] = ontime / 100;                             // 100ms 단위
        
        chksum = cntrData[0] + cntrData[1] + cntrData[2];
        chksum = gensum ((byte *) &cntrData[0], 6);
        
        /* 제어정보 Write...(VME)			*/
        vcntr = (VME_CONTROL *) &iom->modControl;
        
        vcntr->cntrPoint = cntrData[0];     // 제어 포인트 (1...16)
        //for(i=0; i<200; i++) j = i+1;   
        vcntr->cntrState = cntrData[1];     // 제어 상태 (TRIP )
        //for(i=0; i<200; i++) j = i+1;   
        vcntr->relayTime = cntrData[2];
        //for(i=0; i<200; i++) j = i+1;   
        vcntr->chksum    = chksum;
        //for(i=0; i<200; i++) j = i+1;   
        
        vcntr->controlFlag = SET;
        pause(10);
        
        if(opr->msgDebug == ioid) 
        {    
            Debug(console,"%02d> DO control(cpu)...point=%2d, Sts = %2x, relay = %d ...chksum=%4x \n", ioid+1, cntrData[0], cntrData[1],cntrData[2], chksum);
            Debug(console,"%02d> DO control(vme)...point=%2d, Sts = %2x, relay = %d ...chksum=%4x \n", ioid+1, vcntr->cntrPoint, vcntr->cntrState,vcntr->relayTime, vcntr->chksum);
        }
        
        /* 계전기 제어정보 : SELECT */
        logHistoryQue1( opr, hque, EVENT_MOD_OPERATE, ioid, dnp->cntPoint, dnp->cntTcf, 0);       // CONSOLE - 제어 이벤트     
        
        /* -------------------------------- */
        /* LOG File 저장                    */
        /* -------------------------------- */
        sprintf(buffer, "--> BUS> Operate OK : Dev=%d, PT=%d, TCF[TRIP=0x80, CLOSE=0x40]= %2x... ", ioid+1, dnp->cntPoint+1, dnp->cntTcf);
        LogFile_MPU (opr, EVENT_MOD_OPERATE, buffer, strlen(buffer));   
                
        dnp->selectReq = RESET;
        dnp->cntPoint  = RESET;
        dnp->cntTcf    = RESET;        
        
    }      
    return (0);      
} 

#if 0
/*
 *  check_VmeControl()
 *   - PTSRTU-4500 RTU DO모듈 적용 
 *   - TRIP/CLOSE 제어방식 제어 
 */
int check_VmeControl(int ioid, DNP_DEVICE *dnp, VME_MODULE *iom)
{
    //int     i, j;
    int     point=0;
    int     index1, index2;
    word    maskData;
    word    chksum;
    word    cntrData[4]; 
    int     relayONTime; 
    ulong    	timedelay;
      
    VME_CONTROL *vcntr;
    POINT_BUF   *ptbuf;

    /* -------------------------------------------- */
    /*  해당 계전기 - 정보연계 확인...              */
    /* -------------------------------------------- */
    if(iom->initState == RESET) 
    {
        if(opr->dnpDebug == ioid) 
        Debug(console,"vme%02d> DO...Not initial...return !\n", ioid+1);
        return (0);
    }
    
    /* 모듈 타입 비교... */
    if(dnp->type != iom->modType)
    {
        if(opr->dnpDebug == ioid) 
        Debug(console,"vme%02d> DO control...*invalid Module Type (%04x) !\n", ioid+1, iom->modType);
        return (0);
    }
    
    /* -------------------------------------------- */
    /* 	LBS-RTU : Local-Master 인경우... 제어실행   */
    /* -------------------------------------------- */
    if(opr->runMode == LOCAL_SLAVE)
    {
        return (0);
    }   
    
    /* -------------------------------------------- */
    /* 	DO 모듈 제어유무 확인 						*/
    /* -------------------------------------------- */
    if(dnp->selectReq == SET)
    {
        /* 계전기내 제어포인트 참조 */
        ptbuf = (POINT_BUF *) &dnp->doPtBuf[dnp->cntPoint];
        
        if(opr->cntrState == RESET)
        {
            if(dnp->cntTcf == TRIP_CONTROL)          point = dnp->cntPoint*2 + 0;
            else if(dnp->cntTcf == CLOSE_CONTROL)    point = dnp->cntPoint*2 + 1;
            else
            {
                if(opr->soeDebug) 
                Debug(console,"vme%02d> *DO...Control Fail : mis Type = %d !\n", ioid+1, dnp->cntTcf );
                
                dnp->selectReq = RESET;
                dnp->cntTcf    = 0;
                dnp->cntPoint  = 0;
                return (0);
            }
                                           
            index1 = point / 16;
            index2 = point % 16;
            
            /* 모듈별 제어데이터... SETk */
            dnp->ptsCntrData[index1] |= (1 << index2);
            
            opr->cntrState = OPR_RUN;
            
            clock_gettime(CLOCK_REALTIME, &opr->setclock);     // check Time Start
            
            /* ------------------------------------ */
            /*  농어촌공사 PTS4500-RTU 제어시       */
            /* ------------------------------------ */
            dnp->pts4500_CntrReq = SET;
            
            if(opr->soeDebug)
            Debug(console,"scan> IOM-CNTR(ARM) : ioid=%d, point=%2x, sts = %2x ....cnt = %04x %04x\n", ioid+1, 
                dnp->cntPoint+1, dnp->cntTcf, dnp->ptsCntrData[0],dnp->ptsCntrData[1]); 
        }
        else if(opr->cntrState == OPR_RUN)
        {
            clock_gettime(CLOCK_REALTIME, &opr->curclock);     // check Time Start
            
            // 릴레이 동작시간 Check...
            timedelay = calclock( (struct  timespec ) opr->setclock, (struct  timespec ) opr->curclock);                  // Calc Delay Time
            
            if(opr->rcvONtime <= 100)   relayONTime = ptbuf->cntrTime;
            else                        relayONTime = opr->rcvONtime;    
            
            if(timedelay >= relayONTime)
            //if(timedelay >= ptbuf->cntrTime)
            {
                if(dnp->cntTcf == TRIP_CONTROL)          point = dnp->cntPoint*2 + 0;
                else if(dnp->cntTcf == CLOSE_CONTROL)    point = dnp->cntPoint*2 + 1;
                                    
                index1 = point / 16;
                index2 = point % 16;
            
                maskData = (1 << index2);
                        
                //Debug(console,"point=%d, index1=%d, index2=%d...mask = %04x %04x\n",dnp->cntPoint, index1, index2, maskData, ~maskData); 
                
                /* 모듈별 제어데이터... Mask */
                dnp->ptsCntrData[index1] &= (~maskData);
            
                /* ------------------------------------ */
                /*  농어촌공사 PTS4500-RTU 제어시       */
                /* ------------------------------------ */
                dnp->pts4500_CntrReq = SET;
                    
                if(opr->soeDebug)
            	Debug(console,"scan> IOM-CNTR(OPR) : (%2d-%d) TimeOut ... ioid=%d, point=%2x, sts = %2x ....cnt = %04x %04x\n", timedelay,relayONTime , ioid+1,
            	    dnp->cntPoint+1, dnp->cntTcf, dnp->ptsCntrData[0],dnp->ptsCntrData[1]); 
            	
                dnp->cntTcf     = RESET;
                dnp->selectReq  = RESET;
                dnp->cntPoint   = RESET;
                
                opr->cntrState  = RESET;
                opr->cntrData   = RESET;
            }
        }
    }
    
    /* ------------------------------------ */
    /*  농어촌공사 PTS4500-RTU 제어시       */
    /* ------------------------------------ */
    
    if(dnp->pts4500_CntrReq == SET)
    {
        cntrData[0] = dnp->ptsCntrData[0];      /* Point#1  ~ Point#16 제어상태 */
        cntrData[1] = dnp->ptsCntrData[1];      /* Point#17 ~ Point#32 제어상태 */
        cntrData[2] = ~cntrData[0];
        cntrData[3] = ~cntrData[1];
        
        chksum = gensum ((byte *) &cntrData[0], 8);
        
        /* ------------------------------------ */
        /* 제어정보 Write...(VME)			    */
        /* ------------------------------------ */
        vcntr = (VME_CONTROL *) &iom->modControl;
        
        vcntr->cntrPoint = cntrData[0];     // 제어 포인트 (1...16)
        //for(i=0; i<100; i++) j = i+1;   
        
        vcntr->cntrState = cntrData[1];     // 제어 상태 (TRIP )
        //for(i=0; i<100; i++) j = i+1;   
        
        vcntr->relayTime = cntrData[2];
        //for(i=0; i<100; i++) j = i+1;   
        
        vcntr->reserved = cntrData[3];
        //for(i=0; i<100; i++) j = i+1;
           
        vcntr->chksum    = chksum;
        //for(i=0; i<100; i++) j = i+1;   
        
        /* ------------------------------------ */
        /* 제어정보 Write...(PTS4500 제어 => 2) */
        /* ------------------------------------ */
        vcntr->controlFlag = 2;         
        pause(10);
        
        if(opr->msgDebug == ioid) 
        {    
            Debug(console,"%02d> DO control(cpu)..[%04x %04x - %04x %04x]...chksum=%4x \n", ioid+1, cntrData[0], cntrData[1],cntrData[2],cntrData[3], chksum);
            Debug(console,"%02d> DO control(vme)..[%04x %04x - %04x %04x]...chksum=%4x \n", ioid+1, vcntr->cntrPoint, vcntr->cntrState,vcntr->relayTime, vcntr->reserved, vcntr->chksum);
        }
        
        dnp->pts4500_CntrReq = 0;
        //dnp->cntPoint  = 0;
        //dnp->cntTcf    = 0;        
        
    }    

    return (0);                
} 
#endif


/*
 *  scan DI-POINT
 */
int curStore_vmeAI(int ioid, DNP_DEVICE *dnp, VME_MODULE *iom)
{
    //int     i, j;
    int     point;
    short   wdata[32];
    short   adata;
    //short	valAiData;
    //float   *fptr1, *fptr2;
    float   fdata, aiScale;

    //POINT_BUF   *ptbuf;
    POINT_BUF       *aiPoint;
    VME_DATA    *vdata;

    if(iom->initState == RESET) 
    {
        if(opr->msgDebug == ioid) 
        Debug(console,"vme%02d> AIM...Not initial...return !\n", ioid+1);
        return (0);
    }
    
    /* 모듈 타입 비교... */
    if(dnp->type != iom->modType)
    {
        if(opr->dnpDebug == ioid) 
        Debug(console,"vme%02d> AIM analogDump...*invalid Module Type (%04x) !\n", ioid+1, iom->modType);
        return (0);
    }
        
    vdata = (VME_DATA *) &iom->modData;
   
   	if(opr->msgDebug == ioid)
    {
        Debug(console,"\n\n<vme AI-%02d> --------------------------------------------\n", ioid+1);
    }

    /* -------------------------------------------- */
    /* 	LBS-RTU : Local-Master 인경우... 제어실행   */
    /* -------------------------------------------- */
    if(opr->runMode == LOCAL_SLAVE)
    {
        return (0);
    }   
    
    /* ------------------------------------ */ 
    /* DIM 상태정보 ...Update 				*/
    /* ------------------------------------ */ 
    for(point = 0; point < 20; point++)
    {
    	/* ---------------------------------------- */
        /*  계전기별 포인트 지정 => 스케일링...     */
        /* ---------------------------------------- */
        aiPoint = (POINT_BUF *) &dnp->aiPtBuf[point];
        
        adata = vdata->pointData[point];
        wdata[point] = adata;
      	
      	if(aiPoint->ptType == MA20PT)		// 4-20mA 포인트
      	{	  
	        if(adata & 0x8000)		adata = 0;
	        else					adata = adata & 0x7fff;
			
			/* for 4-20mA : 0 - 32767 Convert */
			if(adata <= 0x1999)		adata = 0;
			else					adata = ((adata - 0x1999) * 0x7fff) / (0x7fff - 0x1999);	
        }
      	else 		// 1mA 포인트
      	{	  
	        if(adata & 0x8000)		
    	    {
        		//adata = (~adata) ^ 0x8000;
        		adata = (~adata) + 1;
        		adata = adata * (-1);
        	}
	        else
    	  	{
      			adata = adata & 0x7fff;
        	}
        
        }
        
        /* ---------------------------------------- */
        /*  DB-MAX 에 의한 내부 스케일링...         */
        /* ---------------------------------------- */
        if(aiPoint->dbmax)
        {
            aiScale = (float) aiPoint->dbmax / 0x7fff;
            fdata   = aiScale * adata;            
        }
        else
        {    
            /* 기본 데이터 지정 */
            fdata = (float ) adata;
        }
        
        /* ------------------------------------ */
        /* Analog Data Assign into Buffer ...   */
        /* ------------------------------------ */
        storeAI_DNP( ioid, point, fdata, (byte *) &fdata);

		if((opr->msgDebug == MAX_MODULE) || (opr->msgDebug == ioid))   
        {
            Debug(console,"[%02d] %05d/%4.2f \t", point, wdata[point], fdata);
            if((point & 0x03) == 0x03) Debug(console,"\n");
        }            
    }
    
    return (0);
    
} 


/*
*   VME 모듈 구성정보 초기화
*/
void  moduleDBinit(int ioid, DNP_DEVICE *dnp, VME_MODULE *iom)
{
    int     i;
    int     index1, index2;
    word    chksum;
    
    POINT_BUF   *ptBuf;
    VME_CONFIG  *vcfg;          // 모듈부 구성정보 영역 
    
    iom->cpuAccess = SET;       // CPU Access 상태 : Write 중
    
    vcfg = (VME_CONFIG *) &iom->modConfig;
    vcfg->updateFlag = 0;
    
    /* ------------------------------------------------ */
    /*  모듈부 포인트 구성정보 초기화 (max 32 point)    */
    /* ------------------------------------------------ */
    if(dnp->type == TYPE_DIM)
    {    
        vcfg->modPoint = 32;
        for(i=0; i < 32; i++)
        {
            ptBuf = (POINT_BUF *) &dnp->diPtBuf[i];
            vcfg->pointInfo[i][0] = ptBuf->ptType;          // 포인트 TYPE
            vcfg->pointInfo[i][1] = ptBuf->ptConfig;        // 포인트 Config
            vcfg->pointInfo[i][2] = ptBuf->dbmax;           // Debounce Time
            vcfg->pointInfo[i][3] = 0;                      // Reserved  
            
            //Debug(console,"dev(%2d)point(%2d) : %4d %4d %4d %4d\n", ioid, i,  ptBuf->ptType, ptBuf->ptConfig,ptBuf->dbmax);         
        }
    }
    else if(dnp->type == TYPE_DOM)
    {    
        vcfg->modPoint = 32;
        for(i=0; i < 16; i++)
        {
            index1 = i*2 + 0;
            index2 = i*2 + 1;
            
            ptBuf = (POINT_BUF *) &dnp->doPtBuf[i];
            
            vcfg->pointInfo[index1][0] = ptBuf->ptType;          // 포인트 TYPE
            vcfg->pointInfo[index1][1] = ptBuf->ptConfig;        // 포인트 Config
            vcfg->pointInfo[index1][2] = ptBuf->dbmax;           // 릴레이동작시간
            vcfg->pointInfo[index1][3] = 0;                      // Reserved            
            
            vcfg->pointInfo[index2][0] = ptBuf->ptType;          // 포인트 TYPE
            vcfg->pointInfo[index2][1] = ptBuf->ptConfig;        // 포인트 Config
            vcfg->pointInfo[index2][2] = ptBuf->dbmax;           // 릴레이동작시간
            vcfg->pointInfo[index2][3] = 0;                      // Reserved            
        }
    }
    else if(dnp->type == TYPE_AIM)
    {    
        vcfg->modPoint = 32;
        for(i=0; i < 16; i++)
        {
            index1 = i*2 + 0;
            index2 = i*2 + 1;
            
            ptBuf = (POINT_BUF *) &dnp->aiPtBuf[i];
            vcfg->pointInfo[index1][0] = ptBuf->ptType;          // 포인트 TYPE
            vcfg->pointInfo[index1][1] = ptBuf->ptConfig;        // 포인트 Config
            vcfg->pointInfo[index1][2] = ptBuf->dbmax;           // 릴레이동작시간
            vcfg->pointInfo[index1][3] = 0;                      // Reserved            
            
            vcfg->pointInfo[index2][0] = ptBuf->ptType;          // 포인트 TYPE
            vcfg->pointInfo[index2][1] = ptBuf->ptConfig;        // 포인트 Config
            vcfg->pointInfo[index2][2] = ptBuf->dbmax;           // 릴레이동작시간
            vcfg->pointInfo[index2][3] = 0;                      // Reserved              
        }
    }    
    
    /* DB영역 Chksum */
    chksum = gensum ((byte *) &vcfg->modPoint, 258);
    vcfg->chksum = chksum;
    
    /* CINFIG 영역 초기화 완료... */
    vcfg->updateFlag = SET;
    
    iom->cpuAccess = RESET;       // CPU Access 상태 : Write Clear
    
    if(opr->msgDebug == ioid)   
    Debug(console,"scan> VME Module(%2d) Config Initial...OK...!\n", ioid+1);
}

/*
*   VME 모듈 구성정보 초기화
*/
void  moduleTIMEinit(int ioid, DNP_DEVICE *dnp, VME_MODULE *iom)
{
    //int     i,j;
    //int     index1, index2;
    //word    chksum;
    word    year, month, day, hour, min, sec;
    int     millisecond;

    struct timeval rv; 
    time_t  tm;
    struct  tm  localtm;
        
    /* ---------------------------------- */
    /* 현재시간 정보를 DNP Form 으로 변환 */
    /* ---------------------------------- */
    time(&tm);
    //localtm = localtime(&tm);
    localtime_r (&tm, &localtm);
    
    gettimeofday(&rv, NULL);
    millisecond = (rv.tv_usec / 1000) % 1000;

    iom->cpuAccess = SET;       // CPU Access 상태 : Write 중

    /* VME Module 시각동기... */
    year   = localtm.tm_year + 1900;
    month  = localtm.tm_mon + 1;
    day    = localtm.tm_mday;
    hour   = localtm.tm_hour;
    min    = localtm.tm_min;
    sec    = localtm.tm_sec;
    
    iom->year   = localtm.tm_year + 1900;
    iom->month  = localtm.tm_mon + 1;
    iom->day    = localtm.tm_mday;
    iom->hour   = localtm.tm_hour;
    iom->min    = localtm.tm_min;
    iom->sec    = localtm.tm_sec;
    
    iom->timeSyncFlag = SET;       // VME 시각동기 Flag
            
    iom->cpuAccess = RESET;       // CPU Access 상태 : Write Clear
    
    if(opr->msgDebug == ioid)   
    Debug(console,"=> VME TIME-Sync : module(%2d)....%04d/%2d/%2d-%02d:%02d:%02d-%03d\n", ioid+1,
       year, month, day, hour, min, sec, millisecond  );
    
    /* TIME-SYNC Clear... */
    dnp->timeSyncReq = RESET;
}
    
/*
*   VME 버스상의 모듈 : 유효성 검사 
*/
int checkVmeModule(int ioid, DNP_DEVICE *dnp, VME_MODULE *iom)
{
    //VME_MODULE      *iom;
    VME_SOEQ    *vsoeQ;

    /* -------------------------------- */
    /* VME BUS 상의 모듈 포인터...      */
    /* -------------------------------- */
    //iom = (VME_MODULE *) dnp->vmeModule;
    //iom = (VME_MODULE *) vmeModule[ioid];

    if(opr->dnpDebug == ioid)   
    {    
        Debug(console,"----------------------------------------------------\n");
        Debug(console,"[] DEVICE = %d, TYPE = %s ... ACT = %04x\n", ioid+1, modType[dnp->type], iom->cpuActCount);
        Debug(console,"vme> *** VME Module(%2d) : stx1=%4x, sts2=%4x  \n", ioid+1, iom->vmeStx1, iom->vmeStx2);  
        Debug(console,"vme> *** VME Module(%2d) : Module ID=%2d, TYPE = %04x \n", ioid+1, iom->modId, iom->modType );  
    }
    
    /* -------------------------------- */
    /*  VME 초기화 영역 Check...        */
    /* -------------------------------- */
    if((iom->vmeStx1 != 0x1234) || (iom->vmeStx2 != 0x1234))
    {
        if(opr->dnpDebug == ioid)   
        {
            Debug(console,"vme> *** VME Module(%2d) : Not Installed...stx1=%4x, sts2=%4x  \n", ioid+1, iom->vmeStx1, iom->vmeStx2);  
            Debug(console,"vme> *** VME Module(%2d) : mismatch Module TYPE ...[db = %04x /vmd = %04x] \n", ioid+1, dnp->type, iom->modType );  
        }
        return (0);
    }
    
    /* -------------------------------- */
    /*  모듈 타입/ID 비교...            */
    /* -------------------------------- */        
    if(dnp->type != iom->modType)
    {
        if(opr->dnpDebug == ioid)   
            Debug(console,"vme> *** VME Module(%2d) : mismatch Module TYPE ...[db = %04x/vmd = %04x] \n", ioid+1, dnp->type, iom->modType );  
        return (0);
    }
    else if(ioid != iom->modId)
    {
        if(opr->dnpDebug == ioid)   
            Debug(console,"vme> *** VME Module(%2d) : mismatch Module ID ...[db = %d/vmd = %d] \n", ioid+1, ioid, iom->modId );  
        return (0);
    }    
    
    /* -------------------------------- */
    /*  PFR/DBR 비교...                 */
    /* -------------------------------- */    
    if(iom->pfrFlag == SET)
    {
        moduleDBinit(ioid, dnp, iom);
        
        //iom->cfgError   = 2;        // CPU Clear
        //iom->dbDownFlag = 2;        // CPU Clear
        iom->pfrFlag    = 2;        // CPU Clear
        
        vsoeQ = (VME_SOEQ *) &iom->modSoeQ;
        dnp->soeRear = 0;
        vsoeQ->soeFront = 0;
        
         /* VME Module 시각동기... */
        moduleTIMEinit(ioid, dnp, iom);
        if(opr->dnpDebug == ioid)   
        Debug(console,"vme> *** VME Module(%2d) : Power Fail \n", ioid+1 );  
        
        return (1);
        return (0);
    }
    /* -------------------------------- */
    /*  PFR/DBR 비교...                 */
    /* -------------------------------- */    
    else if(iom->dbDownFlag == SET)
    {
        moduleDBinit(ioid, dnp, iom);
        
        //iom->cfgError   = 2;        // CPU Clear
        iom->dbDownFlag = 2;        // CPU Clear
        //iom->pfrFlag    = 2;        // CPU Clear
        
        vsoeQ = (VME_SOEQ *) &iom->modSoeQ;
        dnp->soeRear = 0;
        vsoeQ->soeFront = 0;
        
         /* VME Module 시각동기... */
        moduleTIMEinit(ioid, dnp, iom);
        if(opr->dnpDebug == ioid)   
        Debug(console,"vme> *** VME Module(%2d) : DB Dowb req \n", ioid+1 );  
        
        return (1);
        return (0);
    }
    /* -------------------------------- */
    /*  PFR/DBR 비교...                 */
    /* -------------------------------- */    
    else if(iom->cfgError == SET)
    {
        moduleDBinit(ioid, dnp, iom);
        
        iom->cfgError   = 2;        // CPU Clear
        //iom->dbDownFlag = 2;        // CPU Clear
        //iom->pfrFlag    = 2;        // CPU Clear
        
         /* VME Module 시각동기... */
        moduleTIMEinit(ioid, dnp, iom);
        
        if(opr->dnpDebug == ioid)   
        Debug(console,"vme> *** VME Module(%2d) : Config Error \n", ioid+1 );  
        
        return (1);
        return (0);
    }
    
    /* VME 초기화 완료... */
    dnp->vmeInitial = SET; 
    
    return 1;        
}

/* -------------------------------------------------------- */
/*  SCAN THREAD Start Routine ....                          */
/*      - VME BUS IOM 프로세서 (DI/DO/AI)                   */
/* -------------------------------------------------------- */
void    *scan_iom_thread(int  *arg)
{
    int     i;
    int     scanid;
    int		taskDelay;
    int     ioid, devIndex=0;
    //int     comFailTick;
    int     retVal;
    int     oldsec=0, cntrCount;
    int     busActFail;
    byte    runtick=0;
    char    buffer[128];
    
    DNP_DEVICE      *dnp;
    VME_MODULE      *iom;
	SCAN_CONFIG     *scan;
	
	/* -------------------------------------------- */
	/* THREAD Argment Assign ...                    */
	/* -------------------------------------------- */
	scanid = (int ) *arg;
	
    /* SCAN-TASK 별 모듈구성 정보 */ 
    scan = (SCAN_CONFIG *) scanCfg[scanid];

	taskDelay = scan->comDelay * 10;
    if(taskDelay < 10)		taskDelay = 100;
    if(taskDelay > 2000)	taskDelay = 2000;
    
    printf(" %s SCAN[%d] IOMODULE-PROCESS Activated ...(COM-Delay = %d ms, OffCount=%d) !\n", TARGET_NAME, scanid+1, taskDelay, scan->offCount);
    
    sprintf(buffer, "%s SCAN[%d] IOMODULE-PROCESS Activated ...(COM-Delay = %d ms, OffCount=%d) !", TARGET_NAME, scanid+1, taskDelay, scan->offCount);
    LogFile_MPU ( opr, NOT_DEFINED, buffer, strlen(buffer));

#if 1
    printf("SCAN[%d] device Num = %02d ... ", scanid+1, scan->devNum );
    for(i=0; i< scan->devNum; i++)  printf(" %2d", scan->device[i]);
    printf("\n");
#endif

    /* ------------------------------------ */
    /*  VME BUS 초기화                      */
    /* ------------------------------------ */
    vmeModuleInit(scanid);
            
    ioid        = 0;	
    soeTimer    = 0;
    cntrCount   = 0;
    busActFail  = 0;

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
	    
	while(termExec)
	{
	    //opr->scanWDT[scanid] = 0;
	    //pause(taskDelay);

        /* -------------------------------------------- */
        /*  LBS-RTU 의 경우 : DI 모듈 이중화...         */
        /*  - SLAVE 모드에서도 Read 해서... 중복않도록  */
        /* -------------------------------------------- */
#ifdef  ACE_LBS_RTU

#else
		/* -------------------------------------------- */
        /*  이중화 구성시...SLAVE 시 Continue       	*/
        /*  ACE-LBS-6U : VME 모듈부 Full 이중화         */
        /* -------------------------------------------- */
        if(opr->runMode == LOCAL_SLAVE) 
        {
            opr->scanWDT[scanid] = 0;   
            pause(100);
            continue;
        }
#endif
        
        /* -------------------------------------------- */
        /* 계전기 DEVICE Comm Handling ...              */
        /* -------------------------------------------- */
        if(++devIndex >= scan->devNum)
        {
            /* ---------------------------------------- */
            /*  모듈부 SCAN 완료후... Pause()           */
            /* ---------------------------------------- */
            pause(taskDelay);
            
            devIndex = 0;
            opr->scanWDT[scanid] = 0;                    
            
            if(scan->scaninit)
            {
                if(--scan->scaninit == 0)  Debug(console,"scan%2d> vme-module scan end ...!\n", scanid+1);
            }
            
            if(opr->rtuinit)
            {
                if(--opr->rtuinit == 0)  
                {
                    Debug(console,"scan%2d> Module-Init scan end ...!\n", scanid);
                }       
            }
                    
            //usleep(1000);
            //soeTimer++;   
            opr->devScanCount[scanid]++;        
            
            /* -------------------------------------------- */
            /*  BUS-Module 전체 동작이상시....CPU 절체      */
            /* -------------------------------------------- */
            if(++busActFail > 100)
            {
                busActFail = 0;
                opr->scanComSts[scanid] = 0;
                
                if((opr->dualCpuSts == SET) && (opr->runMode == LOCAL_MASTER) && (scuCfg->remoteMode == AUTO_MODE))
                {   
                    if(scan->changeMode != 0)
                    {      
                        opr->cpuChange = SET;

                        logEvent1(opr, sque, EVENT_CHANGE_CPU, 3, 0, 0, opr->cpuMode);  
                        logHistoryQue1(opr, hque, EVENT_CHANGE_CPU, 3, 0, 0, opr->cpuMode);   
                        
                        /* -------------------------------- */
                        /* LOG File 저장                    */
                        /* -------------------------------- */
                        sprintf(buffer, "[BUS] *VMEBUS Fail... ");
                        LogFile_MPU (opr, EVENT_CHANGE_CPU, buffer, strlen(buffer));  
                        
                        
                        Debug(console,"wdt> *** CPU Change\n");
                        pause(1000);
                    }
                
                    Debug(console,"\n==================================\n");    
                    Debug(console,"wdt> *** VME BUS Failed  %4d/%2d/%2d %02d:%02d:%02d\n", rtc->year, rtc->month, rtc->day, rtc->hour, rtc->min, rtc->sec);	        
                    Debug(console,"==================================\n");
                }
                
            }               
        }

        /* -------------------------------------------- */  
        /* check DEVICE TYPE & SCAN Flag....            */
        /* -------------------------------------------- */  
        ioid = scan->device[devIndex] & MAX_MODULE_MASK;
        dnp = (DNP_DEVICE *) dnpcfg[ioid];
        iom = (VME_MODULE *) vmeModule[ioid];
        
        if((dnp->type == 0) || (dnp->scan == 0)) continue;
        
        /* -------------------------------------------- */  
        /* VME 영역상의 모듈정보 처리...1초 단위        */
        /* -------------------------------------------- */  
        if(dnp->oldsec != rtc->sec) 
        {
            dnp->oldsec = rtc->sec;
            dnp->vmeChkFlag = SET;

            if((opr->runMode == LOCAL_MASTER) && (dnp->vmeReset == SET))
            {
                if(opr->dnpDebug == ioid)   
                Debug(console,"scan%2d> ***VME-Module(%02d) Re-Start...reboot...!\n", scanid+1, ioid+1);
                dnp->vmeReset = 0;
                iom->resetFlag = SET;
            }    
            else if(dnp->online == 0)
            {
                if((opr->runMode == LOCAL_MASTER) && (++dnp->vmeResetTick > 5))
                {
                    if(opr->dnpDebug == ioid)
                    {   
                        Debug(console,"\n******************************************************\n");
                        Debug(console,"scan%2d> ***VME-Module(%02d) Offline Re-Start...reboot...!\n", scanid+1, ioid+1);
                        Debug(console,"******************************************************\n");
                    }
                    dnp->vmeResetTick = 0;
                    iom->resetFlag = SET;
                }      
            }
            
            iom->cpuActCount++;
            
            /* VME Module 시각동기... */
            if(dnp->timeSyncReq)    moduleTIMEinit(ioid, dnp, iom);
                 
        }

        /* -------------------------------------------- */  
        /*  VME 모듈 Check :                            */
        /* -------------------------------------------- */  
        if(dnp->vmeChkFlag == SET)
        {   
            dnp->vmeChkFlag = RESET;         
            if((retVal = checkVmeModule(ioid, dnp, iom)) == 0)
            {
                dnp->vmeInitial = RESET;
                
                if(scan->scaninit == 0)
                {
                    /* ------------------------------------------------ */
                    /*  LBS-RTU 경우 ...SLAVE 모드에서도 BUS Access     */
                    /*  LOCAL-MASTER 의 경우 Online 판정                */
                    /* ------------------------------------------------ */
#ifdef  ACE_LBS_RTU
                    if(opr->runMode == LOCAL_MASTER)
                    {
                        if(dnp->online != 0)    setOffline(ioid);
                    }
#else                    
                    if(dnp->online != 0)    setOffline(ioid);
#endif                        
                }
                
                continue;
            }
            
            /* BUS 상의 모듈부 Check Flag...Clear */
            busActFail  = 0;
            opr->scanComSts[scanid] = 1;
            
            /* ------------------------------------------------ */
            /*  LBS-RTU 경우 ...SLAVE 모드에서도 BUS Access     */
            /*  LOCAL-MASTER 의 경우 Online 판정                */
            /* ------------------------------------------------ */
#ifdef  ACE_LBS_RTU
            if(opr->runMode == LOCAL_MASTER)
            {
                if(dnp->online != SET)    setOnline(ioid);
            }
#else            
            if(dnp->online != SET)    setOnline(ioid);
#endif            
        }
        else if(dnp->vmeInitial == RESET)
        {
            continue;
        }
        
        if(opr->dnpDebug == ioid)   
        Debug(console,"=>DEVICE = %d, TYPE = %s ...vme Module Access...[cpu=%4x, mod=%4x]\n", ioid+1, modType[dnp->type], iom->cpuActCount, iom->modActCount);
        
        switch(dnp->type)
        {
            case TYPE_DIM:
#if 1
                if((scan->scaninit > 0) || (dnp->statusDump == SET))    curStore_vmeDI(ioid, dnp, iom);
                else                                                    checkSOE_vmeDI(ioid, dnp, iom);
#endif
                //curStore_vmeDI(ioid, dnp, iom);                    
                break;

            case TYPE_DOM:
                check_vmeDO(ioid, dnp, iom);          /* 초기 DOM Version */
                //check_VmeControl(ioid, dnp, iom);		/* PTSRTU-4500 DOM Version - Latch 방식 포함 */
                break;

            case TYPE_AIM :
                ++runtick;
                if((runtick % 5) == 0)
                {    
                    curStore_vmeAI(ioid, dnp, iom);
                }
                break;

            default:
                break;
        } /* switch() */

        if(oldsec == rtc->sec)  continue;
        oldsec = rtc->sec;
    
        
                	    
	}/* while */

    return (0);
}
