
#include	"localLib.h"
#include    "external.h"


/*
*/
void controlResetLANDIS(int hostid)
{
    
    opr->armPoint       = 0;
    opr->armTCF         = 0;
    opr->handOffTimer   = 0;    /* 3 sec */
    opr->cntStatus      = RESET;
    printf("* Control Reset ...!\n");
    
#if 0    
    int     i;
    word    inmsg;

    opr.ioid    = 0;
    opr.point   = 0;
    opr.tcf     = 0;
    opr.port    = 0;
    opr.pointState = 0;    
    opr.cntStatus = RESET;
    opr.handOffTimer = 0;

    printf("* Control Reset ...!\n");
#endif    
}

/*
*
*/
int controlArmLANDIS(int hostid, int cntPoint)
{
    int     cntrDevice, cntrPoint;
    //HOST_DCB *host;
    CONTROL_INFO    *hostCntr;
    //DNP_DEVICE      *dnp;
    //SCAN_CONFIG     *scan;    

    if(cntPoint >= MAX_DNP_DO_POINT)
    {
        Debug(console, "l&g%2d> *ARM Fail : Invalid HOST Control Point ... %d \n", hostid, cntPoint);
        return (NOK);        
    }
    
    /* ---------------------------------------- */
    /*  상위 HOST 제어명령(ARM) Assign ...      */  
    /* ---------------------------------------- */                      
    hostCntr = (CONTROL_INFO *) &landis->controlInfo[cntPoint];
    cntrDevice = hostCntr->devNo;
    cntrPoint  = hostCntr->devPt;

    /* SYSTEM 절체시... */
    if(hostCntr->type  == SYSTEM_CONTROL)
    {
        Debug(console,"l&g%2d> * CPU Change : devid = %d, point=%d\n", hostid, cntrDevice, cntrPoint);
        opr->handOffTimer = 3000;    /* 3 sec */
        opr->cntStatus    = ARM;
        return(OK);
    }
         
    if((cntrDevice < 0) || (cntrDevice >= MAX_DNP_DEVICE))
    {
        Debug(console,"l&g%2d> * ARM Fail : devid = %d, point=%d\n", hostid, cntrDevice, cntPoint);
        return (NOK);
    }

    opr->handOffTimer = 3000;    /* 3 sec */
    opr->cntStatus    = ARM;
    
    return(OK);
                                
}


/*
{
*
*/
int controlOprLANDIS( int hostid, int point, int state)
{
    int     i,j;
    int     mode = 0;
    int     cntrDevice, cntrPoint, cntrTCF, cntrType, cntrConfig;
    char    buffer[128];
    
    CONTROL_INFO    *hostCntr;
    DNP_DEVICE      *dnp;
    SCAN_CONFIG     *scan;    

    /* ---------------------------------------- */
    /*  상위 HOST 제어명령(ARM) Assign ...      */  
    /* ---------------------------------------- */                      
    hostCntr = (CONTROL_INFO *) &landis->controlInfo[point];
    cntrDevice = hostCntr->devNo;
    cntrPoint  = hostCntr->devPt;
    cntrType   = hostCntr->type;        // 제어타입 : 포인트 제어 / 시스템 제어
    cntrConfig = hostCntr->cntrConfig;  // 제어속성 : Pulse, Latch 제어
                    
    if(state == 1)          cntrTCF = 0x80;
    else if(state == 2)     cntrTCF = 0x40;
    else
    {
        Debug(console,"l&g%2d> *** Control Fail...!\n", hostid);
        return (0);
    }

    /* ---------------------------------------- */
    /*  HOST 제어 포인트가 아닌경우...          */  
    /* ---------------------------------------- */   
    if(hostCntr->config == RESET)   
    {
        if(opr->soeDebug)
        Debug(console,"l&g%2d> *** No Define HOST Operate : Point=%d \n", hostid, point+1);
                        
        /* -------------------------------- */
        /* LOG File 저장                    */
        /* -------------------------------- */
        sprintf(buffer, "<-- OPERATE...Host=%2d/Index=%3d, *No Define Point : state = %2x ", hostid+1, point+1, cntrTCF);
        LogFile_MPU (opr, EVENT_HOST_CNTR, buffer, strlen(buffer));   
                            
        return (0);    
    }
    

    /* -------------------------------- */
    /* 시스템 전체 쇄정/해정 명령시...  */
    /* -------------------------------- */
    if(cntrType == ALL_LOCK_CONTROL)
    {
    }
    /* -------------------------------- */
    /* 이중화 시스템 절체명령시...      */
    /* -------------------------------- */
    else if(cntrType  == SYSTEM_CONTROL)
    {
        Debug(console,"l&g%2d> * CPU Change : devid = %d, point=%d\n", hostid, cntrDevice, cntrPoint);
        
        opr->cpuChange = SET; 
        
        opr->cntStatus  = RESET;
        opr->armPoint   = 0;
        opr->armTCF     = 0;
        opr->cntTime    = 0;    
        opr->handOffTimer = 0;    /* 3 sec */
        return(OK);
    }
    
    opr->cntStatus  = RESET;
    opr->armPoint   = 0;
    opr->armTCF     = 0;
    opr->cntTime    = 0;    
    opr->handOffTimer = 0;    /* 3 sec */
    
    /* ---------------------------------------- */
    /* SCAN Task 제어정보 Setting ...           */
    /* ---------------------------------------- */
    for(i=0, mode = 0; i< MAX_SCAN_PORT; i++)
    {
        scan = (SCAN_CONFIG *) scanCfg[i];
        if(scan->devNum <= 0)   continue;
    
        for(j=0; j< scan->devNum; j++)
        {
            /* 제어대상 모듈을 search... */
            if(scan->device[j] == cntrDevice)
            {
                dnp = (DNP_DEVICE *) dnpcfg[cntrDevice];
                dnp->cntPoint = cntrPoint;
                dnp->cntTcf   = cntrTCF;        /* TRIP */
                dnp->cntrType = cntrConfig;         // 제어속성 : Pulse, Latch 제어
                
                /* --------------------------------- */
                /* 포인트 제어TYPE 에 따라서... 결정 */
                /* --------------------------------- */
                if(dnp->cntrType == DNP_AO_CONTROL)
                {
                    continue;
                }
                else
                {              
                    dnp->selectReq  = SET;
                    opr->cntrState 	= RESET;		// 제어단계별 상태 (ARM,ARM-RUN, OPR...)
                }
    
                opr->cntrDevId[i]   = cntrDevice;
                opr->cntrComTick[i] = 0;
                opr->cntrFlag[i]    = SET;

                /* ---------------------------------------- */
                /*  상위관제 제어 이벤트 생성 ...           */
                /*  -- 시스템 이벤트 : MMI 에서 참조        */
                /*  -- 상태 이벤트 : Console에서 참조       */
                /* ---------------------------------------- */
                if(cntrTCF == 0x80)         
                {
                    //logEvent( opr, sque, rtc, HOST_CONTROL, hostid+1, point, TRIP);
                    //logMsg(hque, rtc, HOST_CONTROL, HOST_CONTROL, hostid+1, point, TRIP);	

                    logEvent1( opr, sque, EVENT_HOST_CNTR, cntrDevice, cntrPoint, TRIP_CONTROL, hostid);         // 시스템 이벤트
                    logHistoryQue1( opr, hque, EVENT_HOST_CNTR, cntrDevice, cntrPoint, TRIP_CONTROL, hostid);        // 상태 이벤트
                                        
                }
                else if(cntrTCF == 0x40)    
                {
                    //logEvent( opr, sque, rtc, HOST_CONTROL, hostid+1, point, CLOSE);
                    //logMsg(hque, rtc, HOST_CONTROL, HOST_CONTROL, hostid+1, point, CLOSE);
                    
                    logEvent1( opr, sque, EVENT_HOST_CNTR, cntrDevice, cntrPoint, CLOSE_CONTROL, hostid);        // 시스템 이벤트
                    logHistoryQue1( opr, hque, EVENT_HOST_CNTR, cntrDevice, cntrPoint, CLOSE_CONTROL, hostid);       // 상태 이벤트
                }
    
                if(opr->soeDebug)
                Debug(console,"l&g%2d> OPERATE Info : Point=%d, code=%2x \n",  hostid, point, cntrTCF );
                
                return (OK);    
                break;
            }                            
        }
    }
        
    return (NOK);
}



/*
*/
int     getSTSLandis()
{
    int status;
    LANDIS_SOE_Q    *soeQ;
    LANDIS_COS_Q    *cosQ;    

    status = 0;
    soeQ = (LANDIS_SOE_Q *) &landis->soeQ;
    cosQ = (LANDIS_COS_Q *) &landis->cosQ;
    
    if(soeQ->front != soeQ->rear)   status |= FRAME_SLG;
    if(cosQ->front != cosQ->rear)   status |= FRAME_SCH;
/*    else                            status ^= ~(FRAME_SCH & 0xff);
*/
    if(landis->multiMsg == SET)     status |= FRAME_CON;
    if(landis->freeze)              status |= FRAME_FRZ;  
    if(landis->shortMsg == SET)     status |= FRAME_SHR;
    
#if 0
    printf("2>cosQ->front:[%d], cosQ->rear:[%d]\n", cosQ->front, cosQ->rear);
#endif

    /* 1개의 데이터 Frame을 넘어서는 경우 */   
    if(landis->rcvOpcode == OP_ANA_FORCE)
    {
        if((opr->landisANAPoint > 82) && (landis->anaDumpIndex == 0))           status |= FRAME_CON;
    }        
    
    return (status);
}

/*
* Function Code : 0 , Anaog Change Report 
*/
void landis_ana_cos(HOST_DCB *host)
{                          
    /* Exception Code */
    landis->exceptCode = EXCEPT_NAFC;
    landis->exceptPara = OP_ANA_COS;    /* Function Code */
    landis->exceptFlag = SET;      
}

/*
*/
void gen_ana_force(int hostid, byte *rxbuf)
{
    int     i;
    int     pointNum;
    int     startPoint,stopPoint;
    word    ptData1, ptData2;

    HOST_ANABUF *aiPoint;
    //HOST_DCB *host;

    startPoint = rxbuf[4] + rxbuf[5]*256;
    stopPoint  = rxbuf[6] + rxbuf[7]*256;

    if(opr->hexDebug == hostid)
    Debug(console, "l&g%02d> ANA FORCE : st=%d, stop=%d...total=%d !\n", hostid, startPoint, stopPoint, opr->landisANAPoint);
    
    /* ------------------------------------------------ */
    /* 아날로그 데이터수가 127 byte 를 초과하는 경우    */
    /* ------------------------------------------------ */
    if(opr->landisANAPoint > 82)
    {
        landis->anaDumpIndex = (landis->anaDumpIndex + 1) & 0x01;
        if(landis->anaDumpIndex == 0)
        {
            startPoint  = 0;
            stopPoint   = 82;
        }
        else
        {
            startPoint  = 82;
            stopPoint   = 82 + (opr->landisANAPoint - 82);
        }
    }

    /* ------------------------------------------------ */
    /* Dump 범위에 따른 처리                            */
    /* ------------------------------------------------ */
    if(startPoint > stopPoint)
    {
        if(opr->hexDebug == hostid)
        Debug(console,"*** ANA Point Error : start=%d, stop=%d... !\n",startPoint, stopPoint);
        landis->exceptCode = EXCEPT_INPA;
        landis->exceptPara = 0;
        landis->exceptFlag = SET;
        stopPoint = startPoint + 2;
    }

    if(stopPoint > opr->landisANAPoint)    
    {
        if(opr->hexDebug == hostid)
        Debug(console,"l&g%02d> ANA Point Over :  stop=%d, max = %d... !\n", hostid, stopPoint, opr->landisANAPoint);
        stopPoint = opr->landisANAPoint;
    }    

    /* ------------------------------------------------ */
    /* Report Point Allign with 2                       */
    /* ------------------------------------------------ */ 
    pointNum = (stopPoint - startPoint);    /* report Point Num */
    
    landis->dataBuf[landis->dataCount++] = startPoint & 0xff;
    landis->dataBuf[landis->dataCount++] = (startPoint >> 8) & 0xff;

    /* ------------------------------------------------ */            
    /* Report Point Data ... with 2 point               */
    /* ------------------------------------------------ */
    for(i=startPoint; i < stopPoint; i+= 2)
    {
        aiPoint = (HOST_ANABUF *) hostAnaBuf[i+0];
        ptData1 = aiPoint->pointData;
        
        aiPoint = (HOST_ANABUF *) hostAnaBuf[i+1];
        ptData2 = aiPoint->pointData;
        
        landis->dataBuf[landis->dataCount++] = (ptData1) & 0xff;
        landis->dataBuf[landis->dataCount++] = (( ptData1  >> 8) & 0x000f) +
                               (( ptData2 << 4) & 0x00f0);

        /* Point 수가 홀수인 경우 마지막 Byte 는 생략한다. */
        if((i+1) == pointNum)
        {
            landis->dataBuf[landis->dataCount] &= 0x0f;
            break; 
        }
        
        landis->dataBuf[landis->dataCount++] = (ptData2 >> 4) & 0xff;
    }

    landis->totalCount = landis->dataCount;  

}

/*
*/
void gen_ind_cos(int hostid, byte *rxbuf)
{
    byte    rear;
    LANDIS_COS_Q    *cosQ;
    
    cosQ = (LANDIS_COS_Q *) &landis->cosQ;

    while(cosQ->front != cosQ->rear)
    {
        rear  = cosQ->rear & 0xff;
        landis->dataBuf[landis->dataCount++] = cosQ->que[rear][0];      /* point # */
        landis->dataBuf[landis->dataCount++] = cosQ->que[rear][1];      /* state & point */
        cosQ->rear = (cosQ->rear + 1) & 0xff;
        if(landis->dataCount > 100) break;
    }
    
    landis->totalCount = landis->dataCount; 

}

/*
*  soe force report, ind force report
*/
void gen_ind_force(int hostid, byte *rxbuf)
{
    int     i,j;
    int     point, pointNum;
    int     startPoint,stopPoint;
    byte    stsbuf, chgbuf;
    int index1, index2;  
    HOST_STSBUF *diPoint;

    startPoint = rxbuf[4] + rxbuf[5]*256;
    stopPoint  = rxbuf[6] + rxbuf[7]*256;

    if(stopPoint > opr->landisSTSPoint)    
    {
        if(opr->hexDebug == hostid)
        Debug(console,"l&g> IND Dump Over : stop=%d, max=%d... !\n", stopPoint, opr->landisSTSPoint);
        stopPoint = opr->landisSTSPoint;
    }
        
    if(startPoint > stopPoint)
    {
        if(opr->hexDebug == hostid)
        Debug(console,"l&g> IND Point Error : start=%d, stop=%d... !\n",startPoint, stopPoint);
        landis->exceptCode = EXCEPT_INPA;
        landis->exceptPara = 0;
        landis->exceptFlag = SET;
        stopPoint = startPoint + 2;
    }
    
    pointNum = (stopPoint - startPoint);

    if(opr->hexDebug == hostid)
    Debug(console,"l&g> IND FORCE : st=%d, stop=%d...Total=%d !\n", startPoint, stopPoint, pointNum);
            
    index1   = pointNum / 8;
    index2   = pointNum % 8;
    
    landis->dataBuf[landis->dataCount++] = startPoint & 0xff;
    landis->dataBuf[landis->dataCount++] = (startPoint >> 8) & 0xff;
    
    /* SOE Buffer Assign */
    for(i=0, point= startPoint; i < index1; i++)
    {
        for(j=0, stsbuf=0, chgbuf=0; j < 8; j++, point++)
        {
            diPoint = (HOST_STSBUF *) hostStsBuf[point];
            if(diPoint->status) stsbuf = stsbuf | (1 << j); 
            if(diPoint->soechg) chgbuf = chgbuf | (1 << j);
            
            /* check Flag Clear ... */
            diPoint->soechg  = 0;
            diPoint->chgTick = 0;            
        }
        landis->dataBuf[landis->dataCount++] = stsbuf;
        landis->dataBuf[landis->dataCount++] = chgbuf;
    }
            
    /* 나머지 포인트 정보 Assign ... */            
    if(index2)
    {            
        for(i=0, stsbuf=0, chgbuf=0; i < index2; point++, i++)
        {
            diPoint = (HOST_STSBUF *) hostStsBuf[point];
            if(diPoint->status) stsbuf = stsbuf | (1 << i); 
            if(diPoint->soechg) chgbuf = chgbuf | (1 << i);    
            
            /* check Flag Clear ... */
            diPoint->soechg  = 0;
            diPoint->chgTick = 0;        
        }
        landis->dataBuf[landis->dataCount++] = stsbuf;
        landis->dataBuf[landis->dataCount++] = chgbuf;   
    }
    landis->totalCount = landis->dataCount;
}

/*
*/
void gen_soe_log(int hostid, byte *rxbuf)
{
    byte    rear;
    int     point;
    //int		clrPt;
    LANDIS_SOE_Q    *soeQ;
    //HOST_STSBUF     *diPoint;

    point = 0;    

    soeQ = (LANDIS_SOE_Q *) &landis->soeQ;
    while(soeQ->front != soeQ->rear)
    {
        rear    = soeQ->rear & 0xff;
        landis->dataBuf[landis->dataCount++] = soeQ->que[rear][0];    /* point LSB */
        landis->dataBuf[landis->dataCount++] = soeQ->que[rear][1];    /* point MSB */
        landis->dataBuf[landis->dataCount++] = soeQ->que[rear][2];    /* month */
        landis->dataBuf[landis->dataCount++] = soeQ->que[rear][3];    /* day */
        landis->dataBuf[landis->dataCount++] = soeQ->que[rear][4];    /* hour */
        landis->dataBuf[landis->dataCount++] = soeQ->que[rear][5];    /* min */
        landis->dataBuf[landis->dataCount++] = soeQ->que[rear][6];    /* sec */
        landis->dataBuf[landis->dataCount++] = soeQ->que[rear][7];    /* milisec LSB */
        landis->dataBuf[landis->dataCount++] = soeQ->que[rear][8];    /* milisec MSB */
        
        if(opr->soeDebug)
        {
            Debug(console,"landis> SOE-LOG[front=%d, rear=%d] \n", soeQ->front, soeQ->rear);
            DumpBuff(console, "SOE: ", (char *)&soeQ->que[rear], 9);
        }
        
        soeQ->rear = (soeQ->rear + 1) & 0xff;
#if 0
		clrPt = soeQ->que[rear][0] + ((soeQ->que[rear][1] << 8) & 0x7f);
		diPoint = (HOST_STSBUF *) hostStsBuf[clrPt];
        diPoint->chgTick = 0;        
#endif
        
        if(++point > 10)    break;
    }
    
    landis->totalCount = landis->dataCount;    
}

/*
*/
void gen_adc_ref(int hostid, byte *rxbuf)
{
    int     i;
    int     point, pointNum;
    int     startPoint,stopPoint;
    word    ptData1, ptData2;
    word    refData[64];
    //HOST_ANABUF *aiPoint;

    /* Assign Reference Point */
    bzero(refData, 64*2);
    
    for(i=0, point=0; i< opr->landisANACard; i++)
    {
        refData[point++] = 2048;      /* 0  */
        refData[point++] = 205;       /* - 90% Full */
        refData[point++] = 3890;      /* + 90% Full */
        
#if 0
        refData[point++] = 1842;      /* +90% Full */
        refData[point++] = 0;         /*   0% Full */
        refData[point++] = -1842;     /* - 90% Full */      
#endif
    }
    startPoint = 0;
    stopPoint  = point;

    /* Report Point Allign with 2 */ 
    pointNum = (stopPoint - startPoint + 1);    /* report Point Num */    

    if(opr->hexDebug == hostid)
    Debug(console,"l&g%02d> ANA REF : Card = %d, start=%d, stop=%d... !\n", hostid, opr->landisANACard, startPoint, stopPoint);
        
    landis->dataBuf[landis->dataCount++] = startPoint & 0xff;
    landis->dataBuf[landis->dataCount++] = (startPoint >> 8) & 0xff;

    /* Analog Data Assign */
    for(i=startPoint; i < stopPoint; i+= 2)
    {
        ptData1 = refData[i+0];
        ptData2 = refData[i+1];
        
        landis->dataBuf[landis->dataCount++] = (ptData1) & 0xff;
        landis->dataBuf[landis->dataCount++] = (( ptData1  >> 8) & 0x000f) +
                               (( ptData2 << 4) & 0x00f0);
                               
        /* Point 수가 홀수인 경우 마지막 Byte 는 생략한다. */
        if((i+1) == pointNum)
        {
            landis->dataBuf[landis->dataCount] &= 0x0f;
            break; 
        }
        landis->dataBuf[landis->dataCount++] = (ptData2 >> 4) & 0xff;
    }
    
    landis->totalCount = landis->dataCount;
}

/*
*   LANDIS-HOST 샤시구성정보 Upload
*/
void gen_rtu_config(int hostid, byte *rxbuf)
{
    int     i, j;
    
    if(opr->hexDebug == hostid)
    Debug(console,"l&g%02d> RTU Config : chassisNum=%d, ANA=%d, STS=%d, ACC=%d!\n", hostid, opr->chassisNum, 
        opr->landisANACard, opr->landisSTSCard, opr->landisACCCard);
    
    landis->dataBuf[landis->dataCount++] = opr->chassisNum;   /* max chassis number */
    
    for(i=0; i< opr->chassisNum; i++)
    {
        landis->dataBuf[landis->dataCount++] = i;        /* chassis number */

        for(j=0; j< 16; j++)
        {
            if(rtudb->chassis[i][j] == LANDIS_ANA)       /* ANA Card id */   
                landis->dataBuf[landis->dataCount++] = 1;           
            else if(rtudb->chassis[i][j] == LANDIS_SOE)  /* SOE Card id */   
                landis->dataBuf[landis->dataCount++] = 28;           
            else if(rtudb->chassis[i][j] == LANDIS_ACC)  /* ACC Card id */   
                landis->dataBuf[landis->dataCount++] = 8;           
            else                                        /* NULL card */
                landis->dataBuf[landis->dataCount++] = 31;           
        }
    }
    
    landis->totalCount = landis->dataCount;

}

/*
*/
int gen_sbo_sel(int hostid, byte *rxbuf)
{
	//short   devid,point;
    //int     i, ioid,mode;
    //int     hostpt;
    //int     portNum, ptNum;
    //POINT_BUF *doPoint;

    opr->armPoint   = rxbuf[4];
    opr->armTCF     = ((rxbuf[5] >> 7) & 0x01) + 1;
    opr->cntTime    = rxbuf[5] & 0x7f;
    
    if(opr->soeDebug)
    Debug(console,"l&g%02d> SELECT  Info : Point=%d, code=%2x, cntTime=%d\n", hostid,opr->armPoint,opr->armTCF,opr->cntTime);

    if(controlArmLANDIS(hostid, opr->armPoint) != OK) 
    {
        controlResetLANDIS(hostid);
        printf("l&g> HOST-arm failed .... !\n");                       
       	return(NOK);    /* ARM fail */  
    }  

    landis->dataBuf[landis->dataCount++] = rxbuf[4];  /* point */
    landis->dataBuf[landis->dataCount++] = rxbuf[5];  /* tcf */

    landis->totalCount = landis->dataCount;
    
    return(OK);
                                        
}

/*
*/
int gen_sbo_opr(int hostid, byte *rxbuf)
{
    //int     tcf;
    //int     port, point;

    /* opr.point   = rxbuf[4]+1; */
    if(rxbuf[4] != opr->armPoint)
    {
        Debug(console,"l&g> SBO OPR POINT fail : point=%d, rcv=%d ... !\n",  opr->armPoint, rxbuf[4]);
        controlResetLANDIS(hostid);     
        return(0);                    /* ARM fail */
    }
           
    if(opr->cntStatus != ARM)
    {
        controlResetLANDIS(hostid);               
        Debug(console,"l&g> SBO OPR MODE fail: point=%d, tcf=%d, timer=%d ... !\n",
            opr->armPoint, opr->tcf, opr->cntTime);
        return(0);                    /* ARM fail */
    }

    if(opr->soeDebug)
    Debug(console,"l&g%02d> OPERATE  Info : Point=%d, code=%2x, cntTime=%d\n", hostid,opr->armPoint,opr->armTCF,opr->cntTime);
    
    controlOprLANDIS(hostid, opr->armPoint, opr->armTCF);
	
    landis->dataBuf[landis->dataCount++] = rxbuf[4];    /* point */
    landis->totalCount = landis->dataCount;
    return(0);  
}

/*
*/
int gen_sbo_exe( int hostid, byte *buf)
{
    //int     i, ioid,mode;
    //int     hostpt;
    //int     portNum, ptNum;
    //POINT_BUF *doPoint;    
#if 0
    /* response msg ... */
    landis->dataBuf[landis->dataCount++] = buf[0];    /* point */
    landis->dataBuf[landis->dataCount++] = buf[1];    /* Timer */
    landis->dataBuf[landis->dataCount++] = buf[2];    /* point */
    landis->dataBuf[landis->dataCount++] = buf[3];    /* Timer */
    landis->dataBuf[landis->dataCount++] = buf[4];    /* sequence number */

    landis->totalCount = landis->dataCount;
    
    opr->point   = buf[0] + 1;
    opr->tcf     = ((buf[1] >> 7) & 0x01) + 1;
    opr->cntTime = buf[2] & 0x7f;
        
    /* check control-msg */
    if((buf[0] != (~buf[2] & 0xff)) || (buf[1] != (~buf[3] & 0xff)))
    {
        /* if missmatch Control point */
        landis->exceptFlag = SET;
        landis->exceptCode = EXCEPT_SEMIS;
        landis->exceptPara = opr->point;

        controlResetLANDIS(hostid);

        return (NOK);
    }

    for(i=0; i< MAX_DO_POINT; i++)
    {
        doPoint = (POINT_BUF *) &doPtBuf[i];
        hostpt  = doPoint->dbhostpt;
            
        if(hostpt == opr->point)
        {
            opr->realpoint =  i;
            if(controlArmLANDIS() != OK) 
            {
                controlResetLANDIS(hostid);
                printf("l&g> HOST-arm failed .... !\n");                       
                return(NOK);    /* ARM fail */  
            }        

            controlOprLANDIS();
            return (OK);                        
        }                 
    }

    return(NOK);    /* ARM fail */               

#endif    

    return (0);
}


/*
*/
int  gen_time_sync( int hostid, byte *rxbuf)
{
    int hour, min, sec, msecTick;   
    int     month, day;
    uint    tick;
    //byte    *bfptr;
    //word    crc;
        
    month = rxbuf[4];
    day   = rxbuf[5];
    hour  = rxbuf[6];
    min   = rxbuf[7];
    sec   = rxbuf[8];
    msecTick = rxbuf[9] + rxbuf[10]*256;
    tick = (hour*3600000) + (min*60*1000) + (sec*1000) + msecTick;
    rtc->rtctick = tick;

    //writeClock( rtc->year, month, day, hour, min, sec, msecTick);
    opr->year   = rtc->year;
    opr->month  = month;
    opr->day    = day;
    opr->week   = rtc->week;
    opr->hour   = hour;
    opr->min    = min;
    opr->sec    = sec;
        
    opr->rtcUpdateFlag = SET;
            

    if(landis->broadCast == SET)
    {
        Debug(console,"l&g> broadcast TIME SYNC : %4d/%2d/%2d - %2d:%2d:%2d - %d ... !\n",
            rtc->year, month,day, hour,  min, sec,msecTick);
            return (0);    
    }
    
    /*if(opr.chanDebug == activeFID)*/
    Debug(console,"l&g> TIME SYNC : %4d/%2d/%2d - %2d:%2d:%2d - %d ... !\n",  rtc->year, month, day, hour, min, sec,msecTick);
    
    return (0);
         
}

/*
*/
void gen_time_bias(int hostid, byte *rxbuf)
{

    landis->timeBias = rxbuf[4] + rxbuf[5]*256;
             
    if(opr->hexDebug == hostid) Debug(console,"l&g> TIME BIAS ...%d !\n", landis->timeBias);
    landis->processTime = 2;        
    landis->dataBuf[landis->dataCount++] = landis->processTime;     /* time bias */
    
    landis->totalCount = landis->dataCount;
}            


/*
*/
void gen_firmware(int hostid, byte *rxbuf)
{                                                                    

    if(opr->hexDebug == hostid) Debug(console,"l&g> Firmware : 010A\n");
    landis->dataBuf[landis->dataCount++] = 0x0A;     /* config Number */
    landis->dataBuf[landis->dataCount++] = 0x01;     /* config Number */

    landis->totalCount = landis->dataCount;   
}    

/*
*/
void landis_table_read(int hostid, byte *rxbuf)
{

    opr->timeBias = rxbuf[4] + rxbuf[5]*256;
             
    if(opr->hexDebug == hostid)   Debug(console,"l&g> Table Read ... \n");
    landis->dataBuf[landis->dataCount++] = 0;     /* Table Number */
    landis->dataBuf[landis->dataCount++] = 0;     /* Table Entry  */

    landis->totalCount = landis->dataCount;
}  

/*
*/ 
void notAllowFunc(int opcode, int hostid)
{                                                                    
    /* make except msg */
    landis->exceptFlag = SET;
    landis->exceptCode = EXCEPT_NAFC;
    landis->exceptPara = opcode;

    /* make response msg */
    landis->multiMsg = RESET;
    landis->sendSize   = 0;
    landis->sendPos    = 0;
    landis->totalCount = 0;
    
}

