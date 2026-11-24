#include	"localLib.h"
#include    "external.h"

extern  void    setMaster();
extern  void    resetMaster();
extern  int     rcvWithLINK(TTY_DESC *tp, byte *rxbuf);
extern  int	    checkDualCPU(int mode);

extern	int     termExec;

extern	TTY_DESC 	linkPort;
extern	TASK_INFO	*taskPtr;

extern  PDPDTIME_DATE_TIME      dnpRTC;
extern  PDPDTIME_MS_SINCE_70    dnpRTCInfo;

extern  byte    bitcode[8];

static  byte    linkRxbuf[4096];
static  byte    linkTxbuf[4096];


/*
    convert Byte ordering ... 
*/
void convertRTC(byte *src, byte *des)
{
    src[0] = des[5];
    src[1] = des[4];
    src[2] = des[3];
    src[3] = des[2];
    src[4] = des[1];
    src[5] = des[0];

#if 0
    src[5] = des[5];
    src[4] = des[4];
    src[3] = des[3];
    src[2] = des[2];
    src[1] = des[1];
    src[0] = des[0];    
#endif

}


/*
*   이중화 CPU : 수신받은 SOE 정보를 처리함.
*/
int storeSOE_CU_LINK( int devid, int devPoint, int state, byte *buf)
{
    //int     i;
    int     rtuid;
    //int     point;
    int     portNum, ptNum;
    int     hostid, hostpt;
    uint    tick;
    int     rtuPort;
    int     dnpIndex;
    byte    front, rear;
    byte    status;
    char    buffer[128];
    
    DNP_DEVICE      *dnp;
    POINT_BUF       *diPoint;
    DNP_SOE_QUEUE   *dnpSOEQ; 
    DNP_SOEQ_ENTRY  *entrySOE;
    
    
    DIPOINT_INFO    *hostSts;
    HOST_DCB        *host;   
    SOE_Q           *rtuSOE;     /* rtu format */
    
    HOST_STSBUF     *hostB;
    LANDIS_SOE_Q    *soeQ;
        
    dnp = (DNP_DEVICE *) dnpcfg[devid];
    
    /* ------------------------------------ */
    /* 장치 포인트 정보 Check ...           */
    /* ------------------------------------ */
    if(devPoint >= MAX_DEV_DI_POINT)
    {
        Debug(console,"soe> *invalid point = %d ...!\n", devPoint + 1);
        return (0);
    }
    
    /* -------------------------------- */
    /* Device 현재상태정보 Update ...   */
    /* -------------------------------- */
    diPoint = (POINT_BUF *) &dnp->diPtBuf[devPoint];

    /* ---------------------------------------- */
    /* 미정의 포인트의 경우... Return           */
    /* ---------------------------------------- */
    if(diPoint->config == RESET)    return 0;
    if((diPoint->ptType != ALLPT) && (diPoint->ptType != SOEPT))   return 0;
                
    diPoint->status = state;
    
    /* -------------------------------- */
    /* Convert RTC Inform ...           */
    /* -------------------------------- */
    convertRTC( (byte *) &dnpRTCInfo, &buf[0]);
    makeDnpTimeRTC( &dnpRTC, &dnpRTCInfo);

    if(opr->soeDebug)
    Debug(console,"==> rcv SOE: [dev=%2d / point=%2d], state= %2x, %4d/%02d/%02d %02d:%02d:%02d-%d\n",
          devid+1,devPoint+1,state,dnpRTC.year,dnpRTC.month,dnpRTC.day,dnpRTC.hour,dnpRTC.minute,dnpRTC.second,dnpRTC.millisecond);
    
    /* -------------------------------- */
    /*  계전기 발생 SOE Time ...        */
    /* -------------------------------- */
    opr->soeYear    = dnpRTC.year;
    opr->soeMonth   = dnpRTC.month;
    opr->soeDay     = dnpRTC.day;
    opr->soeHour    = dnpRTC.hour;
    opr->soeMin     = dnpRTC.minute;
    opr->soeSec     = dnpRTC.second;
    opr->soeMilisec = dnpRTC.millisecond;
    
    /* ---------------------------------------- */
    /*  SOE 관련 이벤트 생성 ...                */
    /*  -- 시스템 이벤트 : MMI 에서 참조        */
    /*  -- 상태 이벤트 : Console에서 참조       */
    /* ---------------------------------------- */
    
    /* ----------------------------------------- */
    /* 계전기 통신 이벤트는 Active CPU 에서 생성 */
    /* ----------------------------------------- */
    //soeEvent1( opr, sque, EVENT_SOE, devid, devPoint, state, opr->cpuMode);            // 시스템 이벤트
    //soeHistoryQue1( opr, hque, EVENT_SOE, devid, devPoint, state, opr->cpuMode);       // 상태 이벤트     

    /* -------------------------------------------- */
    /* LOG File 저장                                */
    /* -------------------------------------------- */
    sprintf(buffer, "[link] Device=%2d, point=%3d, state=%d ... [%4d-%02d-%02d %02d:%02d:%02d-%03d ms] %s",  devid+1, devPoint+1, state,
        dnpRTC.year, dnpRTC.month, dnpRTC.day, dnpRTC.hour, dnpRTC.minute, dnpRTC.second, dnpRTC.millisecond, diPoint->nameStr);
    
    /* LOG File 저장 */
    LogFile_MPU(opr, EVENT_SOE, buffer, strlen(buffer));


    /* -------------------------------- */
    /* DNP HOST : SOE Queue Update ...  */
    /* -------------------------------- */
    for(hostid=0; hostid< MAX_HOST; hostid++)
    {
        /* ------------------------------------ */
        /*  상위 HOST별 포인트 참조...          */
        /* ------------------------------------ */
        dnpIndex = diPoint->hostIndex[hostid];             // 1'st DNP-HOST
        
        if((dnpIndex < 1) || (dnpIndex > MAX_DNP_DI_POINT)) continue;
        dnpIndex = dnpIndex - 1;
        
        host = (HOST_DCB *) hostDCB[hostid];
        
        if(host->hostDualMode == HOST_NOT_USE)	continue;
        if(host->hostProtocol != HOST_DNP)	continue;    
        
        hostSts = (DIPOINT_INFO *) &host->stateInfo[dnpIndex];

        /* -------------------------------------------- */  
        /*  SOE 발생시 상태 Update                      */     
        /* -------------------------------------------- */       
        if(hostSts->multiPoint)
        {
            if(state)   
            {
                /* -------------------------------------------- */
                /* 상태가 0-> 1 로 바뀌는 경우 ... SOE 생성     */
                /* 상태가 0이 아닌경우 ... continue             */
                /* -------------------------------------------- */
                if(host->multi_point[dnpIndex] == 0)
                {
                    host->multi_point[dnpIndex]++;
                    if(opr->soeDebug)
                    Debug(console,"=>Multi-SOE : dev=%d, point=%d, state = %d .... hostpoint=%d, status=%d \n", devid + 1, devPoint+ 1, state, dnpIndex+1, host->multi_point[dnpIndex]);
                }
                else
                {
                    host->multi_point[dnpIndex]++;
                    if(opr->soeDebug)
                    Debug(console,"=>Multi-SOE : dev=%d, point=%d, state = %d .... hostpoint=%d, status=%d \n", devid+ 1, devPoint+ 1, state, dnpIndex+1, host->multi_point[dnpIndex]);
                    continue;
                }
            }
            else
            {
                /* -------------------------------------------- */
                /* 상태가 1-> 0 로 바뀌는 경우 ... SOE 생성     */
                /* 상태가 0이 아닌경우 ... continue             */
                /* -------------------------------------------- */
                if(host->multi_point[dnpIndex] > 0)     
                {
                    host->multi_point[dnpIndex]--;
                    if(opr->soeDebug)
                    Debug(console,"=>Multi-SOE : dev=%d, point=%d, state = %d .... hostpoint=%d, status=%d \n", devid+1, devPoint+1, state, dnpIndex+1, host->multi_point[dnpIndex]);
                    if(host->multi_point[dnpIndex] != 0)    continue;
                }
                else    continue;
            }

            /* Multi Point에 의한 상태처리 */
            if (host->multi_point[dnpIndex])    host->sts_pointData[dnpIndex] = PT_FLAG_STS_ON | PT_FLAG_ONLINE;
            else                                host->sts_pointData[dnpIndex] = PT_FLAG_STS_OFF| PT_FLAG_ONLINE;
        }
        else
        {
            if (state)    
            {
            	host->sts_pointData[dnpIndex] = PT_FLAG_STS_ON | PT_FLAG_ONLINE;
            }
            else          
            {
            	host->sts_pointData[dnpIndex] = PT_FLAG_STS_OFF| PT_FLAG_ONLINE;
            }
        }            

        /* ---------------------------------------------------- */
        /* 2016년 3월 5일 - 공항철도 영종역, 비츠로시스 HOST    */
        /* HOST 별 SOE-Q 정보 저장 유무 지정                    */
        /*   - CPU 이중화 의 경우 : 1인 경우.... 저장하지 않음  */
        /* ---------------------------------------------------- */
        
        if(host->function == 1) continue;
        
        /* 2020.04.09 HOST 가 활성화 되어 있는 경우.... */
        if(host->hostActive != HOST_ACTIVE_STS) continue;
        	    
        /* ------------------------------------------------------------ */                
        /* Binary Change with TIME (SOE) Queueing...                    */
        /* ------------------------------------------------------------ */
        dnpSOEQ = (DNP_SOE_QUEUE *) &host->dnpSOEQ;
        front = dnpSOEQ->front& 0xff;
        rear  = dnpSOEQ->rear & 0xff;
        entrySOE = ( DNP_SOEQ_ENTRY *) &dnpSOEQ->soeQueue[front];

        /* ---------------------------------------------------------------- */
        /* QCode 7 : 8bit Single field Quantity, 포인트가 256 이하인 경우   */
        /* ---------------------------------------------------------------- */    
        if(host->diIndexWord == RESET)
        {
            entrySOE->soeBuf[0] = dnpIndex;                                     // 상위 포인트 번호 
            if (state)  entrySOE->soeBuf[1] = PT_FLAG_STS_ON | PT_FLAG_ONLINE;
            else        entrySOE->soeBuf[1] = PT_FLAG_STS_OFF| PT_FLAG_ONLINE;    
            entrySOE->soeBuf[2] = buf[0];
            entrySOE->soeBuf[3] = buf[1];
            entrySOE->soeBuf[4] = buf[2];
            entrySOE->soeBuf[5] = buf[3];
            entrySOE->soeBuf[6] = buf[4];
            entrySOE->soeBuf[7] = buf[5];
            
            dnpSOEQ->front  = (front + 1) & 0xff;       // HOST 통신용
            dnpSOEQ->front1 = (front + 1) & 0xff;       // 이중화 CPU 연계 
        }
        else
        {
            /* ---------------------------------------------------------------- */
            /* QCode 8 : 16bit Single field Quantity, 포인트가 256 이상인 경우  */
            /* ---------------------------------------------------------------- */    
            entrySOE->soeBuf[0] = dnpIndex;                                     // 상위 포인트 번호 
            entrySOE->soeBuf[1] = (dnpIndex >> 8) & 0xff;                       // 상위 포인트 번호 
            
            if (state)  entrySOE->soeBuf[2] = PT_FLAG_STS_ON | PT_FLAG_ONLINE;
            else        entrySOE->soeBuf[2] = PT_FLAG_STS_OFF| PT_FLAG_ONLINE;    
            entrySOE->soeBuf[3] = buf[0];
            entrySOE->soeBuf[4] = buf[1];
            entrySOE->soeBuf[5] = buf[2];
            entrySOE->soeBuf[6] = buf[3];
            entrySOE->soeBuf[7] = buf[4];
            entrySOE->soeBuf[8] = buf[5];
            
            dnpSOEQ->front  = (front + 1) & 0xff;       // HOST 통신용
            dnpSOEQ->front1 = (front + 1) & 0xff;       // 이중화 CPU 연계 
        }
                
        if(opr->soeDebug)
        Debug(console,"==> rcv H-SOE(%d): [dev=%2d / point=%2d] dnp-index=%d, state= %2x, [%4d/%02d/%02d %02d:%02d:%02d-%d] ... %s\n",
          hostid+1, devid+1, devPoint+1, dnpIndex+1, state, dnpRTC.year, dnpRTC.month, dnpRTC.day,
          dnpRTC.hour, dnpRTC.minute, dnpRTC.second,  dnpRTC.millisecond, diPoint->nameStr);          
        
    }
    

    /* ------------------------------- */
    /* HARRIS 주장치#1 rtu Queue       */
    /* ------------------------------- */
    if(opr->hostHarris == SET)
    {       
        /* Calculate rtc time */
        tick = (dnpRTC.hour*3600000) + (dnpRTC.minute * 60000) + (dnpRTC.second * 1000) + dnpRTC.millisecond;

        portNum = diPoint->dbport1;
        ptNum   = diPoint->dbpoint1;
    
        if((portNum > 0) && (portNum <= MAX_PORT) && (ptNum > 0) && (ptNum <= 64))
        {
        	host = (HOST_DCB *) hostDCB[0];
    	
        	if(host->hostProtocol == HOST_HARRIS)
    	    {
                portNum = diPoint->dbport1 - 1;
                ptNum   = diPoint->dbpoint1 -1;
        
                rtuid   = host->portdb[portNum].rtuid;
        
                host->portdb[portNum].pointData[ptNum] = state;
                rtuPort = host->portdb[portNum].harrisPort;

                if(rtuid > 0)
                {
                    /* first SOE Buffering... */
                    rtuSOE  = (SOE_Q *) &host->rtubuf[rtuid-1].soeq;

                    front = rtuSOE->front + 1;
                    rear  = rtuSOE->rear;
                    if(front != rear)
                    {
                        rtuSOE->que[rtuSOE->front][0] = (tick >> 24) & 0x07;
                        rtuSOE->que[rtuSOE->front][1] = (tick >> 18) & 0x3f;
                        rtuSOE->que[rtuSOE->front][2] = (tick >> 12) & 0x3f;
                        rtuSOE->que[rtuSOE->front][3] = (tick >> 6)  & 0x3f;
                        rtuSOE->que[rtuSOE->front][4] = tick & 0x3f;
                        rtuSOE->que[rtuSOE->front][5] = rtuPort * 2 + state;
                        rtuSOE->que[rtuSOE->front][6] = ptNum;
                        rtuSOE->front = (rtuSOE->front + 1) & 0xff;
                    }
                }
            }
        }

    	/* ------------------------------- */
        /* HARRIS 주장치#2 rtu Queue       */
        /* ------------------------------- */
        portNum = diPoint->dbport2;
        ptNum   = diPoint->dbpoint2;
    
        if((portNum > 0) && (portNum <= MAX_PORT) && (ptNum > 0) && (ptNum <= 64))
        {
        	host = (HOST_DCB *) hostDCB[1];
    	
        	if(host->hostProtocol == HOST_HARRIS)
    	    {    
                portNum = diPoint->dbport2 - 1;
                ptNum   = diPoint->dbpoint2 -1;

                rtuid   = host->portdb[portNum].rtuid;
        
                host->portdb[portNum].pointData[ptNum] = state;
                rtuPort = host->portdb[portNum].harrisPort;

                if(rtuid > 0)
                {
                    /* first SOE Buffering... */
                    rtuSOE  = (SOE_Q *) &host->rtubuf[rtuid-1].soeq;

                    front = rtuSOE->front + 1;
                    rear  = rtuSOE->rear;
                    if(front != rear)
                    {
                        rtuSOE->que[rtuSOE->front][0] = (tick >> 24) & 0x07;
                        rtuSOE->que[rtuSOE->front][1] = (tick >> 18) & 0x3f;
                        rtuSOE->que[rtuSOE->front][2] = (tick >> 12) & 0x3f;
                        rtuSOE->que[rtuSOE->front][3] = (tick >> 6)  & 0x3f;
                        rtuSOE->que[rtuSOE->front][4] = tick & 0x3f;
                        rtuSOE->que[rtuSOE->front][5] = rtuPort * 2 + state;
                        rtuSOE->que[rtuSOE->front][6] = ptNum;
                        rtuSOE->front = (rtuSOE->front + 1) & 0xff;
                    }
                }
            }
        }
    }

    /* ----------------------------------- */
    /* LANDIS HOST 현재상태정보 Update ... */
    /* ----------------------------------- */
    if(opr->hostLandis == SET)
    {    
        hostpt  = diPoint->landisIndex;
        //printf("landis> soe...%d\n", hostpt);
        if(hostpt > 0)
        {
            hostpt = hostpt - 1;
            hostB = (HOST_STSBUF *) hostStsBuf[hostpt];
            hostB->status   = state;

            /* ---------------------------- */
            /* update LANDIS SOE-buffer     */
            /* ---------------------------- */
            soeQ = (LANDIS_SOE_Q *) &landis->soeQ;
            front = soeQ->front + 1;
            rear  = soeQ->rear;
        
            if(front != rear)
            {
                /*
                 * Store Internal SOE
                */
                front = soeQ->front;
                if(state)   status = 0x80;
                else		status = 0;

                soeQ->que[front][0] = hostpt & 0xff;             /* point LSB */
                soeQ->que[front][1] = ((hostpt >> 8) & 0x7f) | status;    /* point LSB */
                soeQ->que[front][2] = dnpRTC.month;
                soeQ->que[front][3] = dnpRTC.day;
                soeQ->que[front][4] = dnpRTC.hour;
                soeQ->que[front][5] = dnpRTC.minute;
                soeQ->que[front][6] = dnpRTC.second;
                soeQ->que[front][7] = dnpRTC.millisecond & 0xff;
                soeQ->que[front][8] = (dnpRTC.millisecond >> 8) & 0xff;
    
                soeQ->front = (soeQ->front + 1) & 0xff;

                if(opr->soeDebug)
                printf("landis> SOE[front=%d, rear=%d] : point = %3d state = %d \n", soeQ->front, soeQ->rear, hostpt, status);
                /*printf("soe ... %d\n", hostpt);*/
            }
        }
    }
                         
    return 0;
}

/*----------------------------------------------------------------------------
* Function Name : setOffline()
* 수행내용: 정상적인 모듈이 Offline 되는 경우 해당모듈을 Offline 처리하는 함수
* ArgList :
*   1. ioid - Offline 되는 모듈번호
* Return  :
---------------------------------------------------------------------------- */
int setOffline_LINK( int devid)
{
    short   i, hostid;
    short   devNo;
    short   portNum;
    short   startPt, stopPt;

    DNP_DEVICE  *dnp;
    POINT_BUF   *diPoint;
    POINT_BUF   *aiPoint;
    HOST_DCB    *host;
    
    DIPOINT_INFO    *hostSts;
    AIPOINT_INFO    *hostAna;
    DNP_ANA_INPUT   *dnpAna; 
    
    /* DB-Update 시... RETURN */
    if(opr->dbUpdate == SET)    return (0);
        
    /* DEVICE Offline 처리 */
    if((devid < 0) || (devid >= MAX_MODULE))    return (0);
	
    dnp = (DNP_DEVICE *) dnpcfg[devid];
    
    //dnp->status |= DEV_OFFLINE;
    dnp->online     = 0;
    dnp->offlineTick= 0;
    dnp->initial    = RESET;        // 상태덤프 초기화
    dnp->sndLinkSts = RESET;

    /* ----------------------------------------- */
    /* 계전기 통신 이벤트는 Active CPU 에서 생성 */
    /* ----------------------------------------- */
    //logEvent1(opr, sque, EVENT_MOD_OFFLINE, devid, 0, 0, opr->cpuMode);
    //logHistoryQue1(opr, hque, EVENT_MOD_OFFLINE, devid, 0, 0, opr->cpuMode);

    if(opr->soeDebug)   
    Debug(console,">>> module *Offline ...dev=%2d!\n", devid+1);   
    
    /* ---------------------------------------- */
    /*  DNP HOST : 계전기 포인트 상태 OFFLINE   */    
    /* ---------------------------------------- */            
    for(hostid = 0; hostid < MAX_HOST; hostid++)
    {
        host = (HOST_DCB *) hostDCB[hostid];
        
        if(host->hostDualMode == HOST_NOT_USE)	continue;

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
                host->sts_pointData[i] &= (~PT_FLAG_ONLINE);      // 포인트 상태를 Offline으로...
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
                dnpAna->flag = 0;
            }
        }            
    }
                     
    /* ---------------------------------------- */
    /* HARRIS : OFFline에  따른 PORT Offline    */
    /* ---------------------------------------- */
    if(opr->hostHarris == SET)
    {                
        startPt = 0;
        stopPt  = dnp->devDiPoint;    
        for(i=startPt;i < stopPt; i++)
        {
            diPoint = (POINT_BUF *) &dnp->diPtBuf[i];
            if(diPoint->config == RESET)    continue;

            /* HOST-HARRIS#1 Port Offline SET */
            portNum = diPoint->dbport1 - 1 ;
            if((portNum >= 0) && (portNum < MAX_PORT))  
            {
            	host = (HOST_DCB *) hostDCB[0];
                host->portdb[portNum].portStatus |= OL;        /* 주장치 #1 PORT Assign */
            }

    		/* HOST-HARRIS#2 Port Offline SET */
            portNum = diPoint->dbport2 - 1 ;
            if((portNum >= 0) && (portNum < MAX_PORT))  
            {
            	host = (HOST_DCB *) hostDCB[1];
                host->portdb[portNum].portStatus |= OL;        /* 주장치 #1 PORT Assign */
            }
        }

        startPt = 0;
        stopPt  = dnp->devAiPoint;    
        for(i=startPt;i < stopPt; i++)
        {
            aiPoint = (POINT_BUF *) &dnp->aiPtBuf[i];
            if(aiPoint->config == RESET)    continue;

            /* HOST-HARRIS #1 Port Offline SET */
            portNum = aiPoint->dbport1 - 1 ;
            if((portNum >= 0) && (portNum < MAX_PORT))  
            {
            	host = (HOST_DCB *) hostDCB[0];
                host->portdb[portNum].portStatus |= OL;        /* 주장치 #1 PORT Assign */
            }

            /* HOST-HARRIS #2 Port Offline SET */
            portNum = aiPoint->dbport2 - 1 ;
            if((portNum >= 0) && (portNum < MAX_PORT))  
            {   
            	host = (HOST_DCB *) hostDCB[1];
                host->portdb[portNum].portStatus |= OL;        /* 주장치 #1 PORT Assign */
            }
        }           
    }    
    
    return (0);               
}

         

/*
*   이중화 CPU : 수신받은 SOE 정보를 처리함.
*/
int storeDEV_CU_LINK( int devid, int devPt, int pointState, byte *buf)
{
    int     rtuid;
    int     portNum, ptNum;
    int     hostid, hostpt;
    uint    tick;
    int     rtuPort;
    int     dnpIndex;
    byte    front, rear;
    byte    status;
    
    POINT_BUF       *devPoint;
    
    DNP_SOE_QUEUE   *dnpSOEQ; 
    DNP_SOEQ_ENTRY  *entrySOE;
    
    //DIPOINT_INFO    *hostSts;
    HOST_DCB        *host;   
    SOE_Q           *rtuSOE;     /* rtu format */
    
    HOST_STSBUF     *hostB;
    LANDIS_SOE_Q    *soeQ;
    
    PDPDTIME_DATE_TIME      dnpRTC;
    PDPDTIME_MS_SINCE_70    dnpRTCInfo;
    
    byte    soeBuffer[16];
    char    buffer[128];
    
    
    /* ------------------------------------ */
    /* 장치 포인트 정보 Check ...           */
    /* ------------------------------------ */
    if(devPt >= MAX_DEV_POINT)
    {
        Debug(console,"soe> *invalid Device point = %d ...!\n", devPt);
        return (0);
    }
    
    devPoint = (POINT_BUF *) devPtBuf[devPt];
    
    /* ---------------------------------------- */
    /* 미정의 포인트의 경우... Return           */
    /* ---------------------------------------- */
    if(devPoint->config == RESET)    return 0;
        
    devPoint->status = pointState;
    devPoint->oldsts = pointState;
    
    /* ---------------------------------------- */
    /* 계전기 통신이벤트 경우... 정보처리       */
    /* ---------------------------------------- */
    if((devPt > 0) && (devPt <= MAX_MODULE))
    {
        if(pointState == 0)             // 계전기 Online 
        {
            //printf("link> rcv Device(%d) Comm event... Online !\n", devPt);
        }
        else if(pointState == 1)         // 계전기 Offline
        {
            setOffline_LINK( devPt - 1);    
        }
        else
        {
            printf("link> *Invalid rcv Device(%d) Comm event... state = %d !\n", devPt, pointState);     
            return (0);
        }               
    }
            
         
    /* -------------------------------- */
    /* Convert RTC Inform ...           */
    /* -------------------------------- */
    convertRTC( (byte *) &dnpRTCInfo, &buf[0]);
    makeDnpTimeRTC( &dnpRTC, &dnpRTCInfo);

    if(opr->soeDebug)
    Debug(console,"==> rcv DEV-SOE: [point=%2d], state= %2x, %4d/%02d/%02d %02d:%02d:%02d-%03d   %s\n", devPt, pointState,
          dnpRTC.year,dnpRTC.month,dnpRTC.day,dnpRTC.hour,dnpRTC.minute,dnpRTC.second,dnpRTC.millisecond, devPoint->nameStr);

    /* -------------------------------------------- */
    /* LOG File 저장                                */
    /* -------------------------------------------- */
    sprintf(buffer, "[link] Device-SOE %2d...state = %d,  %s", devPt, pointState, devPoint->nameStr);
    LogFile_MPU(opr, EVENT_SOE, buffer, strlen(buffer));
    
    /* ------------------------------------------------ */
    /*  DNP HOST ...장치포인트 상태 Update...           */    
    /* ------------------------------------------------ */            
    for(hostid = 0; hostid < MAX_HOST; hostid++)
    {
        host = (HOST_DCB *) hostDCB[hostid];
        
        if(host->hostDualMode == HOST_NOT_USE)	continue;
        if(host->hostProtocol != HOST_DNP)	    continue;   
        
        dnpIndex = devPoint->hostIndex[hostid] - 1;     // 해당 장치포인트의 DNP-Index
        if((dnpIndex < 0) || (dnpIndex >= MAX_DNP_DI_POINT))    continue;     
        
        if (pointState == 0)   
        {
            host->sts_pointData[dnpIndex] = PT_FLAG_STS_OFF | PT_FLAG_ONLINE;
        }
        else                
        {
            host->sts_pointData[dnpIndex] = PT_FLAG_STS_ON  | PT_FLAG_ONLINE;                   
        }

        /* ---------------------------------------------------- */
        /* 2016년 3월 5일 - 공항철도 영종역, 비츠로시스 HOST    */
        /* HOST 별 SOE-Q 정보 저장 유무 지정                    */
        /*   - CPU 이중화 의 경우 : 1인 경우.... 저장하지 않음  */
        /* ---------------------------------------------------- */
        if(host->function == 1) continue;
                                
		/* HOST 가 활성화 되어 있는 경우.... */
        if(host->hostActive != HOST_ACTIVE_STS) continue;
        	
        /* ---------------------------------------------------------------- */
        /* QCode 7 : 8bit Single field Quantity, 포인트가 256 이하인 경우   */
        /* ---------------------------------------------------------------- */    
        if(host->diIndexWord == RESET)
        {
            soeBuffer[0] = dnpIndex;
            if (pointState == 0)     soeBuffer[1] = PT_FLAG_STS_OFF | PT_FLAG_ONLINE;
            else                soeBuffer[1] = PT_FLAG_STS_ON | PT_FLAG_ONLINE;  
        
            soeBuffer[2] = (dnpRTCInfo.leastSignificant) & 0xff;
            soeBuffer[3] = (dnpRTCInfo.leastSignificant >> 8) & 0xff;
            soeBuffer[4] = (dnpRTCInfo.mostSignificant) & 0xff;
            soeBuffer[5] = (dnpRTCInfo.mostSignificant >> 8)  & 0xff;
            soeBuffer[6] = (dnpRTCInfo.mostSignificant >> 16) & 0xff;
            soeBuffer[7] = (dnpRTCInfo.mostSignificant >> 24) & 0xff;

            /* Binary Change with TIME (SOE) Queueing... */
            dnpSOEQ = (DNP_SOE_QUEUE *) &host->dnpSOEQ;
            front = dnpSOEQ->front& 0xff;
            rear  = dnpSOEQ->rear & 0xff;
            entrySOE = ( DNP_SOEQ_ENTRY *) &dnpSOEQ->soeQueue[front];

            entrySOE->soeBuf[0] = soeBuffer[0];
            entrySOE->soeBuf[1] = soeBuffer[1];
            entrySOE->soeBuf[2] = soeBuffer[2];
            entrySOE->soeBuf[3] = soeBuffer[3];
            entrySOE->soeBuf[4] = soeBuffer[4];
            entrySOE->soeBuf[5] = soeBuffer[5];
            entrySOE->soeBuf[6] = soeBuffer[6];
            entrySOE->soeBuf[7] = soeBuffer[7];
        
            dnpSOEQ->front  = (front + 1) & 0xff;       // HOST 통신용
            dnpSOEQ->front1 = (front + 1) & 0xff;       // 이중화 CPU 연계 
        }
        else
        {
            /* ---------------------------------------------------------------- */
            /* QCode 8 : 16bit Single field Quantity, 포인트가 256 이상인 경우  */
            /* ---------------------------------------------------------------- */    
            soeBuffer[0] = dnpIndex;
            soeBuffer[1] = (dnpIndex >> 8) & 0xff;
            if (pointState == 0)     soeBuffer[2] = PT_FLAG_STS_OFF | PT_FLAG_ONLINE;
            else                soeBuffer[2] = PT_FLAG_STS_ON | PT_FLAG_ONLINE;  
        
            soeBuffer[3] = (dnpRTCInfo.leastSignificant) & 0xff;
            soeBuffer[4] = (dnpRTCInfo.leastSignificant >> 8) & 0xff;
            soeBuffer[5] = (dnpRTCInfo.mostSignificant) & 0xff;
            soeBuffer[6] = (dnpRTCInfo.mostSignificant >> 8)  & 0xff;
            soeBuffer[7] = (dnpRTCInfo.mostSignificant >> 16) & 0xff;
            soeBuffer[8] = (dnpRTCInfo.mostSignificant >> 24) & 0xff;

            /* Binary Change with TIME (SOE) Queueing... */
            dnpSOEQ = (DNP_SOE_QUEUE *) &host->dnpSOEQ;
            front = dnpSOEQ->front& 0xff;
            rear  = dnpSOEQ->rear & 0xff;
            entrySOE = ( DNP_SOEQ_ENTRY *) &dnpSOEQ->soeQueue[front];

            entrySOE->soeBuf[0] = soeBuffer[0];
            entrySOE->soeBuf[1] = soeBuffer[1];
            entrySOE->soeBuf[2] = soeBuffer[2];
            entrySOE->soeBuf[3] = soeBuffer[3];
            entrySOE->soeBuf[4] = soeBuffer[4];
            entrySOE->soeBuf[5] = soeBuffer[5];
            entrySOE->soeBuf[6] = soeBuffer[6];
            entrySOE->soeBuf[7] = soeBuffer[7];
            entrySOE->soeBuf[8] = soeBuffer[8];
        
            dnpSOEQ->front  = (front + 1) & 0xff;       // HOST 통신용
            dnpSOEQ->front1 = (front + 1) & 0xff;       // 이중화 CPU 연계 
                                
        }                
        
        if(opr->soeDebug)
        Debug(console,"==> H-DEV-SOE(%d): [ point=%2d] dnp-index=%d, state= %2x, [%4d/%02d/%02d %02d:%02d:%02d-%d] ... %s\n",
            hostid+1, devPt, dnpIndex+1, pointState, dnpRTC.year, dnpRTC.month, dnpRTC.day,
            dnpRTC.hour, dnpRTC.minute, dnpRTC.second,  dnpRTC.millisecond, devPoint->nameStr);      
    }        
        
        
    /* ------------------------------- */
    /* HARRIS 주장치#1 rtu Queue       */
    /* ------------------------------- */
    if(opr->hostHarris == SET)
    {       
        /* Calculate rtc time */
        tick = (dnpRTC.hour*3600000) + (dnpRTC.minute * 60000) + (dnpRTC.second * 1000) + dnpRTC.millisecond;

        portNum = devPoint->dbport1;
        ptNum   = devPoint->dbpoint1;
    
        if((portNum > 0) && (portNum <= MAX_PORT) && (ptNum > 0) && (ptNum <= 64))
        {
        	host = (HOST_DCB *) hostDCB[0];
    	
        	if(host->hostProtocol == HOST_HARRIS)
    	    {
                portNum = devPoint->dbport1 - 1;
                ptNum   = devPoint->dbpoint1 -1;
        
                rtuid   = host->portdb[portNum].rtuid;
        
                host->portdb[portNum].pointData[ptNum] = pointState;
                rtuPort = host->portdb[portNum].harrisPort;

                if(rtuid > 0)
                {
                    /* first SOE Buffering... */
                    rtuSOE  = (SOE_Q *) &host->rtubuf[rtuid-1].soeq;

                    front = rtuSOE->front + 1;
                    rear  = rtuSOE->rear;
                    if(front != rear)
                    {
                        rtuSOE->que[rtuSOE->front][0] = (tick >> 24) & 0x07;
                        rtuSOE->que[rtuSOE->front][1] = (tick >> 18) & 0x3f;
                        rtuSOE->que[rtuSOE->front][2] = (tick >> 12) & 0x3f;
                        rtuSOE->que[rtuSOE->front][3] = (tick >> 6)  & 0x3f;
                        rtuSOE->que[rtuSOE->front][4] = tick & 0x3f;
                        rtuSOE->que[rtuSOE->front][5] = rtuPort * 2 + pointState;
                        rtuSOE->que[rtuSOE->front][6] = ptNum;
                        rtuSOE->front = (rtuSOE->front + 1) & 0xff;
                    }
                }
            }
        }

    	/* ------------------------------- */
        /* HARRIS 주장치#2 rtu Queue       */
        /* ------------------------------- */
        portNum = devPoint->dbport2;
        ptNum   = devPoint->dbpoint2;
    
        if((portNum > 0) && (portNum <= MAX_PORT) && (ptNum > 0) && (ptNum <= 64))
        {
        	host = (HOST_DCB *) hostDCB[1];
    	
        	if(host->hostProtocol == HOST_HARRIS)
    	    {    
                portNum = devPoint->dbport2 - 1;
                ptNum   = devPoint->dbpoint2 -1;

                rtuid   = host->portdb[portNum].rtuid;
        
                host->portdb[portNum].pointData[ptNum] = pointState;
                rtuPort = host->portdb[portNum].harrisPort;

                if(rtuid > 0)
                {
                    /* first SOE Buffering... */
                    rtuSOE  = (SOE_Q *) &host->rtubuf[rtuid-1].soeq;

                    front = rtuSOE->front + 1;
                    rear  = rtuSOE->rear;
                    if(front != rear)
                    {
                        rtuSOE->que[rtuSOE->front][0] = (tick >> 24) & 0x07;
                        rtuSOE->que[rtuSOE->front][1] = (tick >> 18) & 0x3f;
                        rtuSOE->que[rtuSOE->front][2] = (tick >> 12) & 0x3f;
                        rtuSOE->que[rtuSOE->front][3] = (tick >> 6)  & 0x3f;
                        rtuSOE->que[rtuSOE->front][4] = tick & 0x3f;
                        rtuSOE->que[rtuSOE->front][5] = rtuPort * 2 + pointState;
                        rtuSOE->que[rtuSOE->front][6] = ptNum;
                        rtuSOE->front = (rtuSOE->front + 1) & 0xff;
                    }
                }
            }
        }
    }

    if(opr->hostLandis)
    {
        hostpt  = devPoint->landisIndex;
        //printf("landis> soe...%d\n", hostpt);
        if(hostpt > 0)
        {
            hostpt = hostpt - 1;
            hostB = (HOST_STSBUF *) hostStsBuf[hostpt];
            hostB->status   = pointState;

            /* ---------------------------- */
            /* update LANDIS SOE-buffer     */
            /* ---------------------------- */
            soeQ = (LANDIS_SOE_Q *) &landis->soeQ;
            front = soeQ->front + 1;
            rear  = soeQ->rear;
        
            if(front != rear)
            {
                /*
                 * Store Internal SOE
                */
                front = soeQ->front;
                if(pointState)  status = 0x80;
                else		    status = 0;

                soeQ->que[front][0] = hostpt & 0xff;             /* point LSB */
                soeQ->que[front][1] = ((hostpt >> 8) & 0x7f) | status;    /* point LSB */
                soeQ->que[front][2] = dnpRTC.month;
                soeQ->que[front][3] = dnpRTC.day;
                soeQ->que[front][4] = dnpRTC.hour;
                soeQ->que[front][5] = dnpRTC.minute;
                soeQ->que[front][6] = dnpRTC.second;
                soeQ->que[front][7] = dnpRTC.millisecond & 0xff;
                soeQ->que[front][8] = (dnpRTC.millisecond >> 8) & 0xff;
    
                soeQ->front = (soeQ->front + 1) & 0xff;

                if(opr->soeDebug)
                printf("landis> SOE[front=%d, rear=%d] : point = %3d state = %d \n", soeQ->front, soeQ->rear, hostpt, status);
                /*printf("soe ... %d\n", hostpt);*/
            }
        }
    }   
    
    return 0;
}
         
/*----------------------------------------------------------------------------
* Function Name : curStore_BIT_DNP()
* 수행내용: 수신된 바이트 정보에서 각 포인트 상태정보를 Mapping 함
* ArgList :
*   1. devid    - 업데이트 하고자 하는 디바이스 번호
*   2. stsbuf   - 수신된 바이트  스트링
*   3. dataSize - 수신된 바이트 수
* Return  :
---------------------------------------------------------------------------- */
int curStore_BIT_LINK( int devid, byte *stsbuf, int dataSize)
{
    int     i,j;
    int     point, portNum, ptNum;
    //int     pointIndex;
    int     hostid, hostPt;
    byte    buf;
    word    dnpIndex ;

    DNP_DEVICE      *dnp;
    POINT_BUF       *diPoint;
    DIPOINT_INFO    *hostSts;
    HOST_DCB        *host;   // DNP HOST
    HOST_STSBUF     *hostB;
    
    dnp = (DNP_DEVICE *) dnpcfg[devid];

    if(dataSize > MAX_DI_DATA_COUNT)
    {
        dataSize = MAX_DI_DATA_COUNT;
    }
    else if(dataSize == 0)	return(0);

    /* -------------------------------------------------------- */
    /*  수신된 바이트 정보에서 각 포인트 정보를 추출함...       */
    /* -------------------------------------------------------- */
    for(i = 0, point=0; i < dataSize; i++)
    {
        buf = stsbuf[i];

        /* store current into harris-queue */
        for(j = 0; j < 8; j++, buf= buf >> 1, point++)
        {
            /* Limit Max Point */
            if(point >= dnp->devDiPoint)    break;
            
            /* ------------------------------------ */
            /* DEVICE 별 포인트 정보 Update...      */
            /* ------------------------------------ */
            /* Point Buffer Assign ... */
            diPoint = (POINT_BUF *) &dnp->diPtBuf[point];

#if 0            
            /* ------------------------------------ */
            /*  포인트 타입 : 반전 포인트인 경우... */
            /* ------------------------------------ */
            if(diPoint->ptConfig == 1)
            {
                diPoint->status = (~buf) & 1;       // 포인트 상태를 반전...
            }
            else
            {
                diPoint->status = buf & 1;
            }
#endif

            diPoint->status = buf & 1;
                                
            if(diPoint->config == RESET)    continue;
			
            /* ------------------------------------ */ 
            /* DNP HOST Data Assign...              */
            /*  => DNP-HOST 별 상태포인트 정보 저장 */
            /* ------------------------------------ */ 
            for(hostid = 0; hostid < MAX_HOST; hostid++)
            {
                host = (HOST_DCB *) hostDCB[hostid];
                
                if(host->hostDualMode == HOST_NOT_USE)	continue;
                    
                dnpIndex = diPoint->hostIndex[hostid];             // 1'st DNP-HOST
                
                if((dnpIndex > 0) && (dnpIndex <= MAX_DNP_DI_POINT))
                {
                    dnpIndex = dnpIndex - 1;
                    
                    /* ---------------------------------------- */
                    /* HOST 별 상태포인트 정보                  */
                    /*  MULTI Point 처리 :                      */
                    /*    - 상태에 따라서 복합처리              */
                    /* ---------------------------------------- */
                    hostSts = (DIPOINT_INFO *) &host->stateInfo[dnpIndex];

                    if (diPoint->status)    
                    {
                        host->sts_pointData[dnpIndex] = PT_FLAG_STS_ON | PT_FLAG_ONLINE;
                    }
                    else                    
                    {
                        host->sts_pointData[dnpIndex] = PT_FLAG_STS_OFF| PT_FLAG_ONLINE;
                    }
                    
                    /* Device Offline : 상태정보 표기 */
                    if(dnp->online != 1)   host->sts_pointData[dnpIndex] &= (~PT_FLAG_ONLINE);      // 포인트 상태를 Offline으로...
                }                           
            }
            
            /* ---------------------------------------- */ 
            /* HARRIS-HOST가 정의된 경우... 데이터 처리 */
            /* ---------------------------------------- */ 
            if(opr->hostHarris == SET)
            { 
                /* ------------------------------------ */ 
                /* HARRIS#1 HOST Data Assign...         */
                /* ------------------------------------ */ 
                portNum = diPoint->dbport1;
                ptNum   = diPoint->dbpoint1;
                if((portNum > 0) && (portNum <= MAX_PORT) && (ptNum > 0) && (ptNum <= 64))
                {
                	host = (HOST_DCB *) hostDCB[0];
                    portNum = diPoint->dbport1 - 1;
                    ptNum   = diPoint->dbpoint1 -1;
    
                    if(diPoint->status) host->portdb[portNum].pointData[ptNum] = 1;
                    else                host->portdb[portNum].pointData[ptNum] = 0;
                }
            
                /* ------------------------------------ */ 
                /* HARRIS#2 HOST Data Assign...         */
                /* ------------------------------------ */ 
                portNum = diPoint->dbport2;
                ptNum   = diPoint->dbpoint2;
            
                if((portNum > 0) && (portNum <= MAX_PORT) && (ptNum > 0) && (ptNum <= 64))
                {
                	host = (HOST_DCB *) hostDCB[1];
                    portNum = diPoint->dbport2 - 1;
                    ptNum   = diPoint->dbpoint2 -1;

                    if(diPoint->status) host->portdb[portNum].pointData[ptNum] = 1;
                    else                host->portdb[portNum].pointData[ptNum] = 0;
                }
            }
            
            /* ---------------------------------------- */ 
            /* LANDIS-HOST가 정의된 경우... 데이터 처리 */
            /* ---------------------------------------- */ 
            if(opr->hostLandis == SET)
            { 
                hostPt = diPoint->landisIndex;
                if((hostPt > 0) && (hostPt <= opr->landisSTSPoint))
                {
                    hostPt = hostPt - 1;
                    hostB = (HOST_STSBUF *) hostStsBuf[hostPt];
                    hostB->status   = diPoint->status;
                }
            }
                        
        } /* for */
    }

    dnp->statusDump = RESET;
    
    /* 초기 상태덤프 후 ... 이벤트 발생 */
    if(++dnp->initial > STATUS_DUMP_TICK)   dnp->initial = STATUS_DUMP_TICK;
    return (0);
}

/*----------------------------------------------------------------------------
* Function Name : storeAI_FLOAT_DNP()
---------------------------------------------------------------------------- */
int storeAI_FLOAT_LINK(int devid, int point, float fdata, byte *lowdata)
{
    int     portNum, ptNum;
    int     pointIndex;
    int     hostid, hostPt;
    int     anaData, adata;
    //word    front, rear;
    //int     delta;
    //float   delta_f;
    long    convData;
    int     dnpIndex;
    //byte    tmpData[8];
    //float   *fptr1, fdata1;
    //byte    *lowdata1;

    DNP_DEVICE      *dnp;
    POINT_BUF       *aiPoint;
    HOST_DCB        *host;   // DNP HOST
    DNP_ANA_INPUT   *anaPoint;
    HOST_ANABUF     *hostB;
    //ANALOG_COA_ENTRY    *anaCOA;

    dnp = (DNP_DEVICE *) dnpcfg[devid];
    
    /* Point Buffer Assign ... */
    pointIndex = point;
    aiPoint = (POINT_BUF *) &dnp->aiPtBuf[pointIndex];
    
    aiPoint->status     = 0x01;
    aiPoint->pointData  = fdata;    /* Float Data ASSIGN */
    aiPoint->floatData  = fdata;
    
    /* -------------------------------------------- */
    /*  HARRIS/LANDIS Data Formattiong...           */
    /* -------------------------------------------- */
    adata = fdata;     /* Float Data ASSIGN */
    
    /* analog(voltage,current) data /10   32bit float */
  	anaData = adata;
        
    /* ------------------------------------ */ 
    /* DNP HOST Data Assign...              */
    /*  => DNP-HOST 별 상태포인트 정보 저장 */
    /* ------------------------------------ */
    for(hostid = 0; hostid < MAX_HOST; hostid++)
    {
        host = (HOST_DCB *) hostDCB[hostid];

        if(host->hostDualMode == HOST_NOT_USE)	continue;
                        
		/* -------------------------------------------- */
    	/*  DNP Data Formattiong...           */
    	/* -------------------------------------------- */
        dnpIndex = aiPoint->hostIndex[hostid];             // 1'st DNP-HOST

        if((dnpIndex > 0) && (dnpIndex <= MAX_DNP_AI_POINT))
        {
            dnpIndex = dnpIndex - 1;
            anaPoint = (DNP_ANA_INPUT *) &host->ana_pointData[dnpIndex];

            anaPoint->flag = PT_FLAG_ONLINE;

            /* Device Offline : 아날로그 정보 표기 */
            if(dnp->online != 1)    anaPoint->flag = 0;
            else                    anaPoint->flag = PT_FLAG_ONLINE;        
                        
            anaPoint->pointData = (int) fdata;

            anaPoint->lowdata[0] = lowdata[3];
            anaPoint->lowdata[1] = lowdata[2];
            anaPoint->lowdata[2] = lowdata[1];
            anaPoint->lowdata[3] = lowdata[0];

        }     
    }

    /* ------------------------------------------------- */
   	/* HARRIS HOST Conversio Data into 12 Bit resolution */
   	/* ------------------------------------------------- */
    if(opr->hostHarris == SET)
    {
        /* ------------------------------------------------- */
       	/* HARRIS HOST Conversio Data into 12 Bit resolution */
   	    /* 0 = 0,     0x7FF = + max full    */
       	/* 0xfff= 0,  0x800 = - max full    */
   	    /* ------------------------------------------------- */
        if(aiPoint->dbmax)
   	    {
            if(dnp->type == TYPE_GIPAM2000)     convData = 2047 * anaData;
   	        else 						        convData = 2047 * adata;
       	    convData = convData / aiPoint->dbmax;
        }
   	    else
        {  
   	        /* dbmax를 0XFFFF로 간주 */
       	    if(dnp->type == TYPE_GIPAM2000)     convData = 2047 * anaData;
            else 						        convData = 2047 * adata;
   	    }        

        for(hostid = 0; hostid < MAX_HOST; hostid++)
        {
            host = (HOST_DCB *) hostDCB[hostid];
            if(host->hostDualMode == HOST_NOT_USE)	continue;

      		/* HOST HARRIS Buffer Assign ... */     
  	    	if(hostid == 0)
  		    {	                                 
      			portNum = aiPoint->dbport1 - 1;                                          
   	    		ptNum   = aiPoint->dbpoint1 - 1;  
   		    }
       		else if(hostid == 1)
  	    	{	                                 
  		    	portNum = aiPoint->dbport2 - 1;                                          
   			    ptNum   = aiPoint->dbpoint2 - 1;  
   		    }
       		else continue;	

   	    	if((portNum >= 0) && (portNum < MAX_PORT) && (ptNum >= 0) && (ptNum < 64))
   		    {                                                                        
   	    	    host->portdb[portNum].pointData[ptNum] = convData;  
   		    }                                         
   	    }                  
   	    
    }
    
    /* ------------------------------------------------- */
   	/* LANDIS HOST Conversio Data into 12 Bit resolution */
   	/* ------------------------------------------------- */
    if(opr->hostLandis == SET)
    {
        /* ------------------------------------------------- */
        /* LANDIS HOST Conversio Data into 12 Bit resolution */
        /* 0x800 : 0, 0xFFF = + max full                     */
        /* 0x7ff : 0, 0     = - max full                     */
        /* ------------------------------------------------- */
        if(aiPoint->dbmax)
        {
            if(adata > 0)
            {
        		if(dnp->type == TYPE_GIPAM2000)     convData = 2047 * anaData;
	            else 								convData = 2047 * adata;                
                convData = convData / aiPoint->dbmax;
                //if(dnp->type == TYPE_GIPAM2000)	convData = convData / 10;                
                convData = convData + 2048;
                if(convData > 4095) convData = 4095;
            }
            else
            {
    	    	if(dnp->type == TYPE_GIPAM2000)    
	    		{
		    		anaData = anaData * (-1);
			    	convData = 2048 * anaData;
    			}
                else 								
                {
                	adata = adata * (-1);
            	    convData = 2048 * adata;                  
                }
            
                convData = convData / aiPoint->dbmax;
                //if(dnp->type == TYPE_GIPAM2000)	convData = convData / 10;                
                convData = 2048 - convData;
                if(convData < 0)    convData = 0;
            }
        }
        else
        {
            convData = adata & 0x0fff;
        }
    
        /* LANDIS HOST Buffer Assign ... */
      	hostPt = aiPoint->landisIndex;
   	    if((hostPt > 0) && (hostPt < MAX_HOST_ANA_PT))
       	{
   	        hostPt = hostPt - 1;
       	    hostB = (HOST_ANABUF *) hostAnaBuf[hostPt];
       	    hostB->pointData = convData;
   	        hostB->convData  = convData;
   	    }
   	
    }
            
    return (0);
    
}   


/*
*   CPU 이중화 - 제어정보 연계
*/
int checkControl_LINK(int cntrDevice, int cntrPoint, int cntrTCF, int cntrHost, int onTime, int offTime)
{
    int i,j,mode;
    char    buffer[128];
    
    //HOST_DCB        *host;        
    DNP_DEVICE      *dnp;         
    SCAN_CONFIG     *scan; 
    POINT_BUF       *ptBuf;       
    
    
    /* ---------------------------------------- */
    /* SCAN Task 제어정보 Setting ...           */
    /* ---------------------------------------- */
    for(i=0, mode = 0; i< MAX_SCAN_PORT; i++)
    {
        scan = (SCAN_CONFIG *) scanCfg[i];
        if((scan->devNum <= 0) || (scan->active == 0))  continue;
        
        for(j=0; j< scan->devNum; j++)
        {
            /* -------------------------------------------- */
            /*  제어대상 모듈을 search...                   */
            /* -------------------------------------------- */
            if(scan->device[j] == cntrDevice)
            {
                opr->rcvONtime  = onTime;       // 수신한 제어시간 정보
                opr->rcvOFFtime = offTime;
        
                /* CPU모드에 따른 제어정보 .... */
                dnp = (DNP_DEVICE *) dnpcfg[cntrDevice];
                ptBuf = (POINT_BUF *) &dnp->doPtBuf[cntrPoint];
                
                dnp->cntPoint = cntrPoint;
                dnp->cntTcf   = cntrTCF;                /* TRIP */
                dnp->cntrType = ptBuf->ptConfig;         // 제어속성 : Pulse, Latch 제어
    
                /* --------------------------------- */
                /* 포인트 제어TYPE 에 따라서... 결정 */
                /* --------------------------------- */
                if(dnp->cntrType == DNP_AO_CONTROL)
                {
                    dnp->aoVpoint     = cntrPoint;     /* point */
                    dnp->aoVpointData = 0xaa;   /* data 0 */
    
                    dnp->anaVOutSelect  = SET;
      	            opr->cntrState 		= RESET;		// 제어단계별 상태 (ARM,ARM-RUN, OPR...)
                        				    
                    if(opr->soeDebug)
                    {
                        Debug(console, "--------------------------------------------\n");       
                        Debug(console,"link> rcv AO Control Operate : dev=%d, point=%d ... Value=%2x\n", cntrDevice +1, dnp->aoVpoint, dnp->aoVpointData);
                    }
                }
                else
                {              
                    dnp->selectReq  = SET;
                }
            
                /* ---------------------------- */
                /* SCAN-Task : 제어정보 전달... */
                /* ---------------------------- */
                opr->cntrDevId[i]   = cntrDevice;
                opr->cntrComTick[i] = 0;
                opr->cntrFlag[i]    = SET;
            
                /* ---------------------------------------- */
                /*  상위관제 제어 이벤트 생성 ...           */
                /*  -- 시스템 이벤트 : MMI 에서 참조        */
                /*  -- 상태 이벤트 : Console에서 참조       */
                /* ---------------------------------------- */
            
                /* LINK ... 제어이력 저장 */
                logEvent1( opr, sque, EVENT_HOST_CNTR, cntrDevice, cntrPoint, cntrTCF, cntrHost);
                logHistoryQue1( opr, hque, EVENT_HOST_CNTR, cntrDevice, cntrPoint, cntrTCF, cntrHost);        // 상태 이벤트
            
                /* -------------------------------------------- */
                /* LOG File 저장                                */
                /* -------------------------------------------- */
                sprintf(buffer, "LINK-OPERATE Info : dev=%d, Point=%d, TCF = %2x ... from HOST(%d) ", cntrDevice+1, cntrPoint+1, cntrTCF, cntrHost+1);
                LogFile_MPU ( opr, EVENT_HOST_CNTR, buffer, strlen(buffer));
            
                if(opr->soeDebug)
                Debug(console,"LINK-OPERATE Info : dev=%d, Point=%d, TCF = %2x ... from HSOT(%d)\n", cntrDevice+1, cntrPoint+1, cntrTCF, cntrHost+1);
            }            
        }
    }              

    return (0);                              
}

/**
**  SLAVE-CPU 수신메세지 처리...
**  opcode를 받건..또는 더 급한 opcode가 있건 data를 보낸다..
**/
int mkResponse_slave(byte *txbuf, byte *rxbuf, int rxcnt)
{
    int     i,index;
    int     hostid;
    int     Qnum;
    int     devid,devPt, state, startPt, ptNum;
    int     opcode, eCode;
    int     dataLen, dbSize;
    int     count, sndCount;
    int     point, index1=0, index2=0;
    
    int     dnpPoint;
    int     anapoint;
    int     stsByteCnt, dumpCount;
    int     cntrDev, cntrPoint, cntrTCF, cntrHost, cntrFlag, onTime, offTime;
    float   *fptr;
    float   fdata;
    
    char    buffer[128];
    
    byte    front, rear;
    word    hfront, hrear;
    byte    efront, erear;
    byte    *bfptr;
    word    chksum;   
    
    DNP_DEVICE          *dnp;         
	DB_HOST_CONFIG		*dbHost;
	DB_SCAN_CONFIG		*dbScan;
    DB_MODBUS_BLOCK		*dbModbus;
    POINT_BUF           *aiPoint;
    POINT_BUF           *diPoint;
    POINT_BUF           *devPoint;
    LINK_SOE            *linkSoe;
    SYSLOG_FORM         *rcvEvent;
    HOST_DCB            *host;
    
    struct timeval      cuTimeVal;
    struct tm           cTime;
    int                 milisec;
    
    
    /* MASTER-Bit Mask... */
    opcode = rxbuf[4] & 0x7f;
    linkCfg->rcvStatus = rxbuf[5];
    linkCfg->rcvSeqno  = rxbuf[6];
    
    dataLen = rxcnt - 8;                // except ... STX, size, opcode, status, seq...LRC */

    /* ---------------------------------------- */
    /*  MMI로부터의 절체 명령...                     */
    /* ---------------------------------------- */
    
    /* 응답인대  받은 것과 관련없이 보낼 것이 있으면 먼저 한다. */
    if(opr->cpuChange)                      opcode = SIM_CHANGE_CPU;
    /* slave mode에서 host로 부터  timesync 받을 수 있지..*/
    else if(linkCfg->timeSync)			    opcode = SIM_TIME_UP;
    else
    {
        if(opr->dualCpuSts == SET) // ?? 엉 dual 이 아니면 tx가 안 되는데. 아.. 여긴 받은 것 처리니까 SET 일수 밖에 없는데
        {
            /* --------------------------------------------------------- */
            /* SLAVE-CPU : HOST 제어정보가 있는 경우...제어정보 연계     */
            /* --------------------------------------------------------- */
            if(opr->runMode == LOCAL_SLAVE)
            {
                if((opcode != SIM_CONTROL_ACK) && (opcode != SIM_SOE_DUMP))
                {   
                    /* 연속 제어실패시.... */    
                    if(linkCfg->cntFlag == SET)
                    {
                        if(++linkCfg->cntSndTick > 5)
                        {
                            linkCfg->cntDev     = 0;
                            linkCfg->cntPoint   = 0;
                            linkCfg->cntTCF     = 0;
                            linkCfg->cntFlag    = 0;
                            linkCfg->cntHost    = 0;
                            linkCfg->cntSndTick = 0;
                            Debug(console,"slave> snd CONTROL-Info ... Cleared ! \n");
                            return (0);
                        }
                        else      
                        {
                            opcode = SIM_CONTROL_INFO;
                        }
                    }
                    else
                    {
                        /* ------------------------------------ */
        	            /* 1) 이중화 CPU : 이벤트 정보 전송...  */
                	    /* ------------------------------------ */
        	            efront = sque->front & 0xff;
	                    erear  = linkCfg->eventRear & 0xff;
	       
    	                if(efront != erear)     opcode = SIM_EVENT_UP;      // CU 이벤트 정보    
#if 0
    	                else
    	                {
    	                    hfront = hque->front & 0xff;
	                        hrear  = linkCfg->historyRear & 0xff;
	       
    	                    if(hfront != hrear)     opcode = SIM_HISTORY_UP;      // CU 이벤트 정보    
    	                }           
#endif
                    }             
                }
            }
            else if(opr->runMode == LOCAL_MASTER)
            {
                /* ------------------------------------ */
        	    /* 1) 이중화 CPU : SOE Cheak...         */
	            /* ------------------------------------ */
	            if((opcode != SIM_SOE_ACK) && (opcode != SIM_CONTROL_INFO))
	            {        
        	        front = linkCfg->front & 0xff;
	                rear  = linkCfg->rear & 0xff;
	       
	       								// 잠시만  
					// SIM_SOE_DUMP는 Master일때만 생성, 즉 내가 SIM_SOE_DUMP를 받으면 난 slave 인데.
					// 밑에서 처리가...	       								
	                if(front != rear)   opcode = SIM_SOE_DUMP;      // 계전기 SOE 정보    
	                else
	                {
	                    /* ------------------------------------ */
        	            /* 1) 이중화 CPU : 이벤트 정보 전송...  */
                	    /* ------------------------------------ */
        	            efront = sque->front & 0xff;
	                    erear  = linkCfg->eventRear & 0xff;
	       
    	                if(efront != erear)     opcode = SIM_EVENT_UP;      // CU 이벤트 정보
#if 0
    	                else
    	                {
    	                    hfront = hque->front & 0xff;
	                        hrear  = linkCfg->historyRear & 0xff;
	       
    	                    if(hfront != hrear)     opcode = SIM_HISTORY_UP;      // CU 이벤트 정보    
    	                }                  
#endif
    	                
	                }
	            }
	        }
        }  
    }        
            
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
    case SIM_CHANGE_CPU:
        opr->cpuChange = 0;
        
        printf("slave> rcv CPU-CHANGE....ACK \n");
        
        if(opr->runMode == LOCAL_MASTER)    resetMaster();
        else                                setMaster();
        break;
        	    
    case SIM_DEVICE_INFO:
        /* ---------------------------------------- */
        /*  CPU 이중화가 아닌경우...                */
        /* ---------------------------------------- */
        if(opr->dualCpuSts == SET)
        {
            if(opr->runMode == LOCAL_MASTER)
            {   
                for(i=0; i < MAX_DEV_POINT; i++)
                {  
                    devPoint = (POINT_BUF *) devPtBuf[i];       // Device Point 정보 : 개별 계전기, 1,2 ~ 64 
                    txbuf[count++] = devPoint->status;          // Device Point 상태정보
                }
            }
            else
            {
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
                    if((i > 0 ) && (i < MAX_MODULE))
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
        }
        break;

    case SIM_CONTROL_ACK:
        linkCfg->cntDev     = 0;
        linkCfg->cntPoint   = 0;
        linkCfg->cntTCF     = 0;
        linkCfg->cntFlag    = 0;
        linkCfg->cntHost    = 0;
        linkCfg->cntSndTick = 0;
        if(opr->soeDebug)
        Debug(console,"slave> rcv Control ACK \n");
        return (0);
        break;
                	    
    case SIM_CONTROL_INFO:
        /* ---------------------------------------- */
        /*  CPU 이중화가 아닌경우...                */
        /* ---------------------------------------- */
        if(opr->dualCpuSts == SET)
        {
            /* ---------------------------------------- */
            /*  CPU 이중화가 아닌경우...                */
            /* ---------------------------------------- */
            if(opr->runMode == LOCAL_MASTER)
            {
                cntrDev     = rxbuf[7];
                cntrPoint   = rxbuf[8];
                cntrTCF     = rxbuf[9];
                cntrFlag    = rxbuf[10];
                cntrHost    = rxbuf[11];
            
                onTime      = 1000;
                offTime     = 1000;
            
                if(cntrFlag != SET) break;
                
                checkControl_LINK(cntrDev, cntrPoint, cntrTCF, cntrHost, onTime, offTime);
            
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
            }
            else        
            {
                /* ---------------------------------------- */
                /*  SLAVE-CPU : MASTER-CPU로 제어정보 전달  */
                /* ---------------------------------------- */
                txbuf[7] = linkCfg->cntDev;
                txbuf[8] = linkCfg->cntPoint;
                txbuf[9] = linkCfg->cntTCF;
                txbuf[10] = linkCfg->cntFlag;
                txbuf[11] = linkCfg->cntHost;
                count = 12;
            
                if(opr->soeDebug)
                Debug(console,"slave> snd Control : dev=%d, point=%d, tcf=%2x ... HOST=%d \n",  linkCfg->cntDev, linkCfg->cntPoint, linkCfg->cntTCF, linkCfg->cntHost);
            }
        
        }
        else        
        {
            txbuf[7] = 0;      
            txbuf[8] = 0;    
            txbuf[9] = 0;    
            txbuf[10] = 0;    
            txbuf[11] = 0;    
            count = 12;
        }
        
        break;	    

    case SIM_HISTORY_ACK:
        return (0);
        break;

    case SIM_HISTORY_UP:                        // 0x03, CU 이벤트 연계
        hfront = hque->front & HISTORY_QUE_MASK;
        hrear  = linkCfg->historyRear & HISTORY_QUE_MASK;
        Qnum = (hfront - hrear + HISTORY_QUE_MAX) % HISTORY_QUE_MAX;
        
        if(Qnum)
        {
            if(Qnum > 32)   Qnum = 32;
                        
            txbuf[7] = Qnum;
            count = 8;
                    
            for(i=0; i< Qnum; i++)
            {
                hrear  = linkCfg->historyRear & HISTORY_QUE_MASK;
                
                /* SOE 이벤트 내용을 Copy */
                memcpy( &txbuf[count], (byte *) &hque->queue[hrear], sizeof(SYSLOG_FORM));
                count += sizeof(SYSLOG_FORM);
                        
                linkCfg->historyRear  = (linkCfg->historyRear + 1) & HISTORY_QUE_MASK;
            }
                
            if(opr->soeDebug)
            Debug(console,"slave> LINK-History Q Upload... front = %d, rear = %d  >> Qnum=%d \n", hfront, hrear, Qnum);
        }
        else
        {
            txbuf[7] = 0;
            count = 8;
        }      
        break;
                
    case SIM_HISTORY_DOWN:
        Qnum   = rxbuf[7];  // SOE Queue
        
        if(opr->soeDebug)
        Debug(console,"slave> rcv History : Receive Q = %d\n", Qnum);
        
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
            chksum = gensum((byte *) hque, sizeof(HISTORY_QUE) - 2);
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
            Debug(console,"slave> snd History-ACK : Receive Q = %d\n", Qnum);
            break;
        }

        return (0);                
        break;                    
        

    case SIM_EVENT_ACK:
        Qnum   = rxbuf[7];  // SOE Queue
        
        if(opr->soeDebug)
        Debug(console,"slave> rcv Event-ACK : Receive Q = %d\n", Qnum);
        
        return (0);
        break;

    case SIM_EVENT_UP:                        // 0x03, CU 이벤트 연계
        efront = sque->front & 0xff;
        erear  = linkCfg->eventRear & 0xff;
        Qnum = (efront - erear + 256) % 256;
        
        if(Qnum)
        {
            if(Qnum > 16)   Qnum = 16;
                        
            txbuf[7] = Qnum;
            count = 8;

            
            for(i=0; i< Qnum; i++)
            {
                erear  = linkCfg->eventRear & 0xff;
                
                /* SOE 이벤트 내용을 Copy */
                memcpy( &txbuf[count], (byte *) &sque->queue[erear], sizeof(SYSLOG_FORM));
                count += sizeof(SYSLOG_FORM);
                        
                linkCfg->eventRear  = (linkCfg->eventRear + 1) & 0xff;
            }
            
            erear  = linkCfg->eventRear & 0xff;
                
            if(opr->soeDebug)
            Debug(console,"slave> LINK-Event Q Upload... front = %d, rear = %d  >> Qnum=%d \n", efront, erear, Qnum);
        }
        else
        {
            txbuf[7] = 0;
            count = 8;
        }      
        break;
                
    case SIM_EVENT_DOWN:
        Qnum   = rxbuf[7];  // SOE Queue
        
        if(opr->soeDebug)
        Debug(console,"slave> rcv Event : Receive Q = %d\n", Qnum);
        
        if(Qnum)
        {
            /* -------------------------------------------- */
            /* Active-CPU 로부터 수신받은 SOE 정보 처리     */
            /* -------------------------------------------- */
            
            /* -------------------------------- */
            /* NVRAM Access 참조                */
            /* -------------------------------- */
            for(i=0; i< 10; i++)
            {
                if(opr->sramAccess == SET) pause(10);
                else break;                        
            }
                                
            opr->squeWrite = SET;    
                            
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
            Debug(console,"slave> snd Event-ACK : Receive Q = %d\n", Qnum);
            break;
        }

        return (0);                
        break;                        	    
        
    case SIM_SOE_ACK:
        Qnum   = rxbuf[7];  // SOE Queue
        
        if(linkCfg->report == Qnum)
        {
            if(opr->soeDebug)
            Debug(console,"slave> rcv SOE-ACK ... Report = %d, rcv = %d \n", linkCfg->report, Qnum);
            
            /* SOE 전송후 ... rear 포인트 증가 */
            linkCfg->rear = (linkCfg->rear + Qnum) & 0xff; 
            linkCfg->report = 0; 
        }
        else
        {
            if(opr->soeDebug)
            Debug(console,"slave> rcv SOE-ACK ...MissMatch Report = %d, rcv = %d \n", linkCfg->report, Qnum);
            linkCfg->rear = linkCfg->front;
            linkCfg->report = 0;
        }      
        return (0);                  
        break;
        
    case SIM_SOE_DUMP:
        /* ---------------------------------------- */
        /*  CPU 이중화가 아닌경우...                */
        /* ---------------------------------------- */
        if(opr->dualCpuSts == SET)
        {
            if(opr->runMode == LOCAL_SLAVE)
            {
                Qnum   = rxbuf[7];  // SOE Queue
            
                if(opr->soeDebug)
                Debug(console,"slave> rcv SOE : qnum = %d\n",  Qnum);

                if(Qnum)
                {
                    for(i=0; i < Qnum; i++)
                    {   
                        index = i*sizeof(LINK_SOE);
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
                    /*  SOE -ACK 전송                       */
                    /* ------------------------------------ */
                    txbuf[4] = SIM_SOE_ACK;
                    txbuf[5] = linkCfg->runStatus;          // CU 동작상태
                    txbuf[6] = linkCfg->rcvSeqno;           // 수신 seqno 
                    txbuf[7] = Qnum;                        // 수신 Q num
                    count = 8;
                    
                    if(opr->soeDebug)
                    Debug(console,"slave> snd SOE-ACK : Receive SOE-Q = %d\n", Qnum);
                
                }
                
            }
            else
            {
                front = linkCfg->front & 0xff;
	            rear  = linkCfg->rear & 0xff;
	        
	            Qnum = (front - rear + 256) % 256;
	        
    	        if(Qnum)
                {
                    if(Qnum > 16)   Qnum = 16;
                        
                    txbuf[7] = Qnum;
                    count = 8;
                    
                    for(i=0; i< Qnum; i++)
                    {
                        /* SOE 이벤트 내용을 Copy */
                        memcpy( &txbuf[count], (byte *) &linkCfg->linkSOE[rear], sizeof(LINK_SOE));
                        count += sizeof(LINK_SOE);
                        
                        rear = (rear + 1) & 0xff;
                    }
                    
                    linkCfg->report = Qnum;
                    
                    if(opr->soeDebug)
                    Debug(console,"slave> LINK-SOE Queue-send... front = %d, rear = %d  >> Qnum=%d \n", front, rear, Qnum);
                
                }
            }  
        }
        else       
        {
            txbuf[7] = 0;       // soe-Q 갯수     
            count = 8;
        }

        break;
        	    
    case SIM_POINT_ANALOG:
        /* ---------------------------------------- */
        /*  CPU 이중화가 아닌경우...                */
        /* ---------------------------------------- */
        if(opr->dualCpuSts != SET)
        {
            txbuf[7] = rxbuf[7];    // 수신한 계전기 번호      
            txbuf[8] = 0;           // Dump 포인트 수
            count = 9;
            break;
        }
        
        devid      = rxbuf[7];      // 계전기 번호     
        dumpCount  = rxbuf[8];      // 포인트 수     
        
        /* -------------------------------------------------------------------- */
        /*  LOCAL_SLAVE 모드 : Master CPU로부터 계전기별 계측데이터 Update...   */
        /*  LOCAL_MASTER모드 : Slave CPU에서 Master로 계전기별 계측데이터 전송  */
        /* -------------------------------------------------------------------- */
        if(opr->runMode == LOCAL_SLAVE)
        {    
            dnp = (DNP_DEVICE *) dnpcfg[devid];
            
            bfptr = (byte *) &rxbuf[9];         // 계측데이터 시작지점...
            count = 0;
            
            /* ---------------------------------------- */
            /*  수신된 포인트 수만큼... Update...       */
            /* ---------------------------------------- */
            for(i=0; i< dumpCount; i++)
            {
                anapoint = (bfptr[count+ 0]*256) + bfptr[count + 1];              // 계측포인트 번호 (0,1,2,...)
                count = count + 2;
                
                fptr = (float *) &bfptr[count];         // 계측 데이터(Float)
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
                
            txbuf[7] = devid;           // 계전기 번호     
            txbuf[8] = 0;               // 데이터 사이즈 (Byte 수)     
            count = 9;                    
        }
        else    // MASTER 인경우...
        {
            dnp = (DNP_DEVICE *) dnpcfg[devid];
            
            txbuf[7] = devid;          // 계전기 번호     
            txbuf[8] = 0;              // Dump 포인트 수      
            count = 9;
            
            dumpCount = 0;
            
            /* ---------------------------------------- */
            /*  계전기 계측데이터 ... 송신메세지 구성   */
            /* ---------------------------------------- */
            for(i=0; i < MAX_DEV_AI_POINT; i++)
            {
                aiPoint = (POINT_BUF *) &dnp->aiPtBuf[i];
                if(aiPoint->config == 0)    continue;
                
                txbuf[count++] = (i >> 8) & 0xff;                   // 계측포인트 번호 (0,1,2,...)
                txbuf[count++] = i & 0xff;                          // 계측포인트 번호 (0,1,2,...)
                memcpy( &txbuf[count], &aiPoint->floatData, 4);     // 계측 데이터(Float)
                count += 4;
                dumpCount++;    
                
                /* 보고되는 아날로그 데이터 제한 */
                if(dumpCount > 128)  break;    
            }
            
            txbuf[8] = dumpCount;                                   // Dump 포인트 수      
                        
        }    
        break;
        	    
    case SIM_POINT_STATUS:
        /* ---------------------------------------- */
        /*  CPU 이중화가 아닌경우...                */
        /* ---------------------------------------- */
        if(opr->dualCpuSts != SET)
        {
            txbuf[7] = rxbuf[7];    // 수신한 계전기 번호      
            txbuf[8] = 0;           // Dump 포인트 수
            count = 9;
            break;
        }
        
        devid      = rxbuf[7];      // 계전기 번호     
        stsByteCnt = rxbuf[8];      // 데이터 사이즈 (Byte 수)     
        
        /* -------------------------------------------------------------------- */
        /*  LOCAL_SLAVE 모드 : Master CPU로부터 계전기별 상태데이터 Update...   */
        /*  LOCAL_MASTER모드 : Slave CPU에서 Master로 계전기별 상태데이터 전송  */
        /* -------------------------------------------------------------------- */
        if(opr->runMode == LOCAL_SLAVE)
        {    
            dnp = (DNP_DEVICE *) dnpcfg[devid];
            
            /* ---------------------------------------- */
            /*  계전기별 상태데이터 Update...           */
            /* ---------------------------------------- */
            if(stsByteCnt > 0)  curStore_BIT_LINK( devid, &rxbuf[9], stsByteCnt);
          
            if(opr->slaveDebug) 
            {
                Debug(console, "slave : STS(%02d) Data Updated...size= %2d \n", devid+1, stsByteCnt);
                DumpBuff(console," ", &rxbuf[9], stsByteCnt);
            }
                
            txbuf[7] = devid;           // 계전기 번호     
            txbuf[8] = 0;               // 데이터 사이즈 (Byte 수)     
            count = 9;
        }
        else // LOCAL_MASTER
        {
            dnp = (DNP_DEVICE *) dnpcfg[devid];
            
            txbuf[7] = devid;           // 계전기 번호     
    	    txbuf[8] = 0;                  // 데이터 사이즈 (Byte 수)     
        	count = 9;
            
       	    /* ------------------------------------ */
       	    /* 2014.08.12 상태 포인트가 없는 경우.  */
       	    /* ------------------------------------ */
       	    if(dnp->devDiPoint <= 0)    break;
        	        
       	    bfptr = (byte *) &txbuf[9];
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
                txbuf[8] = index1 + 1;     // 데이터 사이즈 (Byte 수)     
                count = count + index1 + 1;
            }
            else
            {  
                txbuf[8] = index1;         // 데이터 사이즈 (Byte 수)     
                count = count + index1;      
            }       
            
#if 0            
            /* ---------------------------------------- */
            /*  계전기 Online : 수신덴 상태데이터 전송  */
            /*         Offline: 계전기 버퍼내용 전송    */
            /* ---------------------------------------- */
            if((dnp->online == SET) && (dnp->initial >= STATUS_DUMP_TICK))
            {	
	            txbuf[7] = devid;          		// 계전기 번호     
    	        txbuf[8] = dnp->stsByteCnt;    	// 데이터 사이즈 (Byte 수)     
        	    count = 9;
        	    
            	memcpy( &txbuf[count], &dnp->linkSts[0], dnp->stsByteCnt);
            	count = count + dnp->stsByteCnt;
            }
            else
            {
            	txbuf[7] = devid;           // 계전기 번호     
    	        txbuf[8] = 0;               // 데이터 사이즈 (Byte 수)     
        	    count = 9;
            }	            
#endif            
        }    
        
        break;
        	    
                   	    
	case SIM_CHKSUM: 	/* GPoll */
		chksum = gensum((byte *)rtudb, sizeof(RTU_DATABASE) - 2);
		txbuf[count++]  = (chksum >> 8) & 0xff;
        txbuf[count++]  = chksum & 0xff;
        
        /* ---------------------------------- */
        /* MASTER-CPU의 HOST 통신상태 수신... */
        /* ---------------------------------- */
        opr->rcvMmiHost[0] = rxbuf[7];
  		opr->rcvMmiHost[1] = rxbuf[8];
    	opr->rcvMmiHost[2] = rxbuf[9];
	    opr->rcvMmiHost[3] = rxbuf[10];
	    opr->rcvMmiHost[4] = rxbuf[11];
		    
        /* ---------------------------------- */
        /* SLAVE-CPU의 HOST통신상태 전송...   */
        /* ---------------------------------- */
        txbuf[count++] = opr->mmiHostTemp[0];
        txbuf[count++] = opr->mmiHostTemp[1];
        txbuf[count++] = opr->mmiHostTemp[2];
        txbuf[count++] = opr->mmiHostTemp[3];
        txbuf[count++] = opr->mmiHostTemp[4];
        
        if(opr->linkDebug)	
		Debug(console,"slave> snd Chksum = %4x\n", chksum);
		break;
				
	case SIM_GPOLL: 	/* GPoll */
		txbuf[count++]  = 0;
        txbuf[count++]  = 0;
        break;
        
	case SIM_TIME_DOWN:     /* Time Sync Down */
        /* ---------------------------------------- */  
        /* CU 시각정보 추출...                      */   
        /* ---------------------------------------- */   
        memcpy( &cuTimeVal, &rxbuf[7], sizeof(struct timeval));

        localtime_r (&cuTimeVal.tv_sec, &cTime);
        milisec = cuTimeVal.tv_usec / 1000;
        
        //if(opr->linkDebug)
        Debug(console, "slave> rcv TIME Sync Down : %4d/%2d/%2d %02d:%02d:%02d-%03d\n", cTime.tm_year + 1900, cTime.tm_mon +1, cTime.tm_mday, cTime.tm_hour, cTime.tm_min, cTime.tm_sec, milisec);
        
        /* ---------------------------------------- */  
        /* CU 시각정보 Update...                    */   
        /* ---------------------------------------- */  
        if(settimeofday(&cuTimeVal, NULL) < 0) 
        {
            Debug(console,"slave> *** TIMESET-FAIL : %4d/%2d/%2d %02d:%02d:%02d-%03d \n", cTime.tm_year + 1900, cTime.tm_mon +1, cTime.tm_mday, cTime.tm_hour, cTime.tm_min, cTime.tm_sec, milisec);
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
            sprintf(buffer, "slave> rcv TIME Sync Down : %4d/%2d/%2d %02d:%02d:%02d-%03d ", cTime.tm_year + 1900, cTime.tm_mon +1, cTime.tm_mday, cTime.tm_hour, cTime.tm_min, cTime.tm_sec, milisec);
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
        Debug(console, "slave> snd TIME Down ACK \n");
        break;

    case SIM_TIME_ACK:                             // 0x10,  Time Sync Down    
		linkCfg->timeSync  = RESET;
		
		//if(opr->linkDebug)
        Debug(console, "slave> rcv TIME Down ACK \n");
		return (0);
        break;
                		
	case SIM_TIME_UP:       /* Time Sync Up */

    	/* CU 시각정보 추출...  */
        gettimeofday(&cuTimeVal, NULL);
        memcpy( &txbuf[count], (byte *) &cuTimeVal, sizeof(struct timeval));
        count = count + sizeof(struct timeval);
        
        localtime_r (&cuTimeVal.tv_sec, &cTime);
        milisec = cuTimeVal.tv_usec / 1000;
        
        linkCfg->timeSync = 0;      // cmd Clear...
        
        //if(opr->linkDebug)
        Debug(console, "slave> TIME Sync Up : %4d/%2d/%2d %02d:%02d:%02d-%03d \n", cTime.tm_year + 1900, cTime.tm_mon +1, cTime.tm_mday, cTime.tm_hour, cTime.tm_min, cTime.tm_sec, milisec);
        	
        break;    		

	case SIM_HOST_DOWN:     /* HOST Config Database Down */
		
		dbSize = (sizeof(DB_HOST_CONFIG)*MAX_DB_HOST) + (sizeof(DB_HOST_PARAMETER)*MAX_DB_HOST) + sizeof(DB_CU_CONFIG);
		dbHost = (DB_HOST_CONFIG *) &rtudb->hostCfg[0];
		
        if(dataLen >= dbSize)   dataLen = dbSize;
    	memcpy( (byte *) dbHost, &rxbuf[7], dataLen);
    	
    	/* 데이터베이스 저장 ... */
        if(dbFileWrite(rtudb, opr->cpuMode) < 0)
        {
            sprintf(buffer, "%s", "[WR] *DB Write Error... ");
            LogFile_MPU ( opr, NOT_DEFINED, buffer, strlen(buffer)); 
        }
            
        txbuf[count++]  = 0; 
        
        //if(opr->linkDebug)   
        Debug(console,"slave> HOST Config-DB Down ... rcv=%d\n", dataLen); 
        opr->hostDBDown = SET;
        break;
        		
	case SIM_SCAN_DOWN:     /* SCAN Config Database Down */
		dbSize = (sizeof(DB_SCAN_CONFIG)* MAX_DB_SCAN_PORT) + (sizeof(DB_DEVICE_CONFIG)*MAX_DB_MODULE);
		
		dbScan = (DB_SCAN_CONFIG *) &rtudb->scanConfig[0];
        if(dataLen >= dbSize)   dataLen = dbSize;
        memcpy( (byte *) dbScan, &rxbuf[7], dataLen);
        
        /* 데이터베이스 저장 ... */
        if(dbFileWrite(rtudb, opr->cpuMode) < 0)
        {
            sprintf(buffer, "%s", "[WR] *DB Write Error... ");
            LogFile_MPU ( opr, NOT_DEFINED, buffer, strlen(buffer)); 
        }
        
        txbuf[count++]  = 0;   
        
        //if(opr->linkDebug)   
        Debug(console,"slave> SCAN Config-DB Down ... rcv=%d\n", dataLen);   
        opr->scanDBDown = SET;     
        break;
        		
	case SIM_PORT_DOWN:     /* Port Config Database Down */
		if(dataLen >= 385)   dataLen = 385;
        memcpy(&rtudb->portdb1[0], &rxbuf[7], dataLen);  /* 32x2x3 port */
        
        /* 데이터베이스 저장 ... */
        if(dbFileWrite(rtudb, opr->cpuMode) < 0)
        {
            sprintf(buffer, "%s", "[WR] *DB Write Error... ");
            LogFile_MPU ( opr, NOT_DEFINED, buffer, strlen(buffer)); 
        }
        
        txbuf[count++]  = 0;    
        //if(opr->linkDebug)   
        Debug(console,"slave> Port DB Down ... rcv=%d\n", dataLen);   
        break;
        
	case SIM_POINT_DOWN:    /* Point Config Database Down */
		startPt = (rxbuf[8] + rxbuf[7] * 256) - 1;
        ptNum   = rxbuf[9] & 0x7f;
        if(ptNum > 16)  ptNum = 16;
        	
        if((startPt + ptNum) >= MAX_DB_POINT)		    
		{
			Debug(console,"slave> Point DB Down : *** Invalid Point num = %d\n", startPt + ptNum);
			return 0;
		}	
			       
        bfptr   = (byte *) &rtudb->devPtConfig[startPt];
        
        memcpy( (byte *) bfptr, &rxbuf[10], ptNum * sizeof(DEVICE_PTCFG));
        
        txbuf[count++]  = rxbuf[7];     // 시작 포인트
        txbuf[count++]  = rxbuf[8]; 
        txbuf[count++]  = rxbuf[9];     // 포인트 갯수
         
        //if(opr->linkDebug)   
        Debug(console,"slave> rcv Point Down : start=%d, ptnum=%d\n", startPt, ptNum);
        
        /* 포인트 전송종료 Check ... */
        if(rxbuf[9] & 0x80)
        {	
            /* 데이터베이스 저장 ... */
            if(dbFileWrite(rtudb, opr->cpuMode) < 0)
            {
                sprintf(buffer, "%s", "[WR] *DB Write Error... ");
                LogFile_MPU ( opr, NOT_DEFINED, buffer, strlen(buffer)); 
            }
            
        	Debug(console,"\n==================================================\n");
        	Debug(console,"slave> Point Down OK .... Point Initial...Start\n");	
        	Debug(console,"==================================================\n");
        	opr->pointDBDown = SET;
        	
        }
        break;
        
	case SIM_MODBUS_DOWN:   /* MODBUS Profile Block Database Down */
		dbSize = sizeof(DB_MODBUS_BLOCK) * 8;
		
		dbModbus = (DB_MODBUS_BLOCK *) &rtudb->modbusBlock[0];
        if(dataLen >= dbSize)   dataLen = dbSize;
        	
        memcpy( (byte *) dbModbus, &rxbuf[7], dataLen);
        
        /* 데이터베이스 저장 ... */
        if(dbFileWrite(rtudb, opr->cpuMode) < 0)
        {
            sprintf(buffer, "%s", "[WR] *DB Write Error... ");
            LogFile_MPU ( opr, NOT_DEFINED, buffer, strlen(buffer)); 
        }
        
        txbuf[count++]  = 0;   
        
        //if(opr->linkDebug)   
        Debug(console,"slave> MODBUS Config-DB Down ... rcv=%d\n", dataLen);   
        opr->modbusDBDown = SET;  
        break;
        		
	default :
		Debug(console,"slave> *** Invalid Opcode = %2x\n", opcode);
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
* =======================================================================
*   SLAVE-CPU 주처리 프로세서
* =======================================================================
*/
void    linkSlave()
{
    //int     i,j;
    //int     fid;
	byte	runtick;
	int     mstStatus, masterTick;
	int		oldsec=0, oldHour=0;
    
    byte	*rxbuf, *txbuf;
	int		rxcnt, txcnt;
    char    buffer[128];    
    //ulong    	timedelay;
    //struct  timespec setclock;               // DO제어시...Relay 동작시간 Check
    //struct  timespec curclock;               // DO제어시...Relay 동작시간 Check
    
    
    printf(" %s LINK-SLAVE[%s] PROCESS Activated ... !\n", TARGET_NAME,linkPort.name);        
    
    sprintf(buffer, "%s LINK-SLAVE PROCESS Activated ... !", TARGET_NAME);    
    LogFile_MPU ( opr, NOT_DEFINED, buffer, strlen(buffer));
    
  	rxbuf = (byte *) &linkRxbuf[0];
	txbuf = (byte *) &linkTxbuf[0];
	
	/* -------------------------------------------- */
	/*  SLVAE CPU의 수신버퍼 Clear...               */
	/* -------------------------------------------- */
	while((rxcnt = rcvWithLINK(&linkPort, rxbuf)) > 0)
    {
        if(rxcnt > 128) DumpBuff(console,"*RXS: ", rxbuf, 128);
        else            DumpBuff(console,"*RXS: ", rxbuf, rxcnt);  

        taskPtr->wdtCount = 0;
		pause(20);
    }
    
    runtick = 0;
    linkCfg->comFailTick = 0;
    linkCfg->timeSync    = 0;       // SLAVE Time Flag Cleared...
    linkCfg->offline     = 2;
        
    opr->checkLinkOK   = SET;		// CPU 링크상태 확인....
    masterTick = 0;
    
	while(termExec)
	{
	    taskPtr->wdtCount = 0;
		pause(20);

        /* ---------------------------------------- */
        /*  CPU 운영모드 : SLAVE  모드              */
        /* ---------------------------------------- */
        if((rxcnt = rcvWithLINK(&linkPort, rxbuf)) > 0)
        {
            linkCfg->comFailTick = 0;

            if(opr->linkDebug)  
            {
                if(rxcnt > 128) DumpBuff(console,"RXS: ", rxbuf, 128);
                else            DumpBuff(console,"RXS: ", rxbuf, rxcnt);    
            }
            
            /* ---------------------------------------- */
            /*  CPU 모두 Master 인 경우...Slave Reset   */
            /* ---------------------------------------- */
            mstStatus = rxbuf[4] & 0x80;
            if((opr->runMode == LOCAL_MASTER) && (mstStatus == 0x80))
            {   
                if(++masterTick > 10)
                {
                    resetMaster();
		            Debug(console,"link> *return =>  RESET Master !\n");   
		            masterTick = 0;
		        }
            }
            else
                masterTick = 0;
                
            /* 수신메세지 처리...   */            
            txcnt = mkResponse_slave(txbuf, rxbuf, rxcnt);
            
            if(linkCfg->offline != RESET)
            {    
                logEvent1(opr, sque, EVENT_LINK_ONLINE, 0, 0, 0, opr->cpuMode); 
                logHistoryQue1(opr, hque, EVENT_LINK_ONLINE, 0, 0, 0, opr->cpuMode); 
                
                /* -------------------------------------------- */
                /* LOG File 저장                                */
                /* -------------------------------------------- */
                sprintf(buffer, "%s", "LINK Online ");
                LogFile_MPU ( opr, EVENT_LINK_ONLINE, buffer, strlen(buffer));
                
            }
            linkCfg->offline   = RESET;
            
            if(txcnt)
            {
                PortWrite( (TTY_DESC *) &linkPort, txbuf, txcnt);
                if(opr->linkDebug)  
                {
                    if(txcnt > 128) DumpBuff(console,"TXS: ", txbuf, 128);
                    else            DumpBuff(console,"TXS: ", txbuf, txcnt);    
                }
            }
			
        }

		if(oldsec == rtc->sec)  continue;
        oldsec = rtc->sec;

        /* ---------------------------------------- */
        /* 주기적으로 CPU 장착상태를 Check...       */
        /* ---------------------------------------- */
	    if(linkCfg->cmdFlag == SET) checkDualCPU(1);    	 
        else                        checkDualCPU(0);    	 	           

        /* ---------------------------------------- */
        /* MASTER 통신이상시 ... MASTER 모드로 운영 */
        /* ---------------------------------------- */
		if(++linkCfg->comFailTick > 5)
		{
		    linkCfg->comFailTick = 0;
		    
		    linkCfg->cntDev     = 0;
            linkCfg->cntPoint   = 0;
            linkCfg->cntTCF     = 0;
            linkCfg->cntFlag    = 0;
            linkCfg->cntHost    = 0;
            linkCfg->cntSndTick = 0;
            
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
            
		    linkCfg->offline   = SET;
		    
		    /* Master 모드 Check... */
		    if(opr->runMode == LOCAL_SLAVE)
		    {
		        Debug(console,">> *CPU LINK Fail... Check..ACT !\n");
		        linkCfg->cmdFlag = SET;
		    }
		 
		    PortClose((TTY_DESC *) &linkPort);  pause(100);
            PortInitial((TTY_DESC *) &linkPort); pause(100);    
        }       


        if(oldHour == rtc->hour)  continue;
        oldHour = rtc->hour;
    
	}/* while */

    resetMaster();
}


