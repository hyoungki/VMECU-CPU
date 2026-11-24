/*
 *****************************************************************************
 * SYSTEM   : SUBWAY-Line2  RTU-CCU (지하철2호선용 원격소장치 : 공통제어부 )
 * FileName : CHGMAP.C
 * File 내용: 모듈에서 취득한 정보를 처리하여 상위로 전송하기위한 내부처리
              를 수행하는 루틴
 * Designed : TRATEK ... by LHS
 *****************************************************************************
*/

#include	"localLib.h"
#include	"external.h"

extern  void    convertRTC(byte *src, byte *des);

                                          /* convert                         */
extern  PDPDTIME_DATE_TIME      dnpRTC;
extern  PDPDTIME_MS_SINCE_70    dnpRTCInfo;


/*----------------------------------------------------------------------------
* Function Name : storeAO_DNP_LONG()  O30V3 32bit without flag
* 수행내용: HIMAP 계전기 AO 포인트 정보를 내부 Buffer 로 데이터화 하는 함수
* ArgList :
*   1. devid - device Number [0..31]
*   2. point - device 내 포인트 번호
*   3. adata - 포인트 값
* Return  :
* HIMAP 적용
---------------------------------------------------------------------------- */
int storeAO_DNP_LONG( int devid, int point, long adata)
{
#if 0    
    int     portNum, ptNum;
    int     pointIndex;
    long    convDataH, convDataL;
    int     hostPt;

    DNP_DEVICE  *dnp;
    POINT_BUF   *aoPoint;


    dnp = (DNP_DEVICE *) dnpcfg[devid];

    /* ------------------------------------------------ */
    /* DEVICE 별 Point Buffer Assign ...                */
    /* ------------------------------------------------ */
    pointIndex = point;
    aoPoint = (POINT_BUF *) &dnp->aoPtBuf[pointIndex];

    aoPoint->status     = PT_FLAG_ONLINE;
    aoPoint->pointData  = (int) adata;
    //aoPoint->dnpData    = adata;


    return (0);
    
    /* ------------------------------------------------- */
    /* HARRIS HOST Conversio Data into 12 Bit resolution */
    /* 0 = 0,     0x7FF = + max full    */
    /* 0xfff= 0,  0x800 = - max full    */
    /* ------------------------------------------------- */
    convDataH = adata;
    convDataL = adata;

    if(aoPoint->dbmax)
    {
        convDataH = 2047 * adata;
        convDataH = convDataH / aoPoint->dbmax;
    }
    
    /* ------------------------------------------------- */
    /* LANDIS HOST Conversio Data into 12 Bit resolution */
    /* 0x800 : 0, 0xFFF = + max full                     */
    /* 0x7ff : 0, 0     = - max full                     */
    /* ------------------------------------------------- */
    if(aoPoint->dbmax)
    {
        if(adata > 0)
        {
            convDataL = 2047 * adata;
            convDataL = convDataL / aoPoint->dbmax;
            convDataL = convDataL + 2048;
            if(convDataL > 4095) convDataL = 4095;
        }
        else
        {
            adata = adata * (-1);
            convDataL = 2048 * adata;
            convDataL = convDataL / aoPoint->dbmax;
            convDataL = 2048 - convDataL;
            if(convDataL < 0)    convDataL = 0;
        }
    }
#endif

    return (0);
}

/*----------------------------------------------------------------------------
* Function Name : storeCOS()
* 수행내용: 수신된 COS 정보를  내부 운영 스트럭쳐로 데이터화 하는 함수
* ArgList :
*   1. bfptr - 수신된 COS 정보의 Pointer
*   2. count - 수신된 COS 정보의 데이터 수
* Return  :
* GIPAM2000 적용
---------------------------------------------------------------------------- */
int storeAO_DNP(int devid, int point, int adata)         
{
#if 0    
    int     portNum, ptNum;
    int     pointIndex;
    long    convDataH, convDataL;
    int     hostPt;
	int     anaData;
	
    DNP_DEVICE  *dnp;
    POINT_BUF   *aoPoint;


    dnp = (DNP_DEVICE *) dnpcfg[devid];

    /* analog(voltage,current) data /10  */
    anaData = adata;
    adata   = adata / 10;

    /* ------------------------------------------------ */
    /* DEVICE 별 Point Buffer Assign ...                */
    /* ------------------------------------------------ */
    pointIndex = point;
    aoPoint = (POINT_BUF *) &dnp->aoPtBuf[pointIndex];

    aoPoint->status     = 0x01;
    aoPoint->pointData  = adata;
    //aoPoint->dnpData    = adata;

    return (0);
            
    /* ------------------------------------------------- */
    /* HARRIS HOST Conversio Data into 12 Bit resolution */
    /* 0 = 0,     0x7FF = + max full    */
    /* 0xfff= 0,  0x800 = - max full    */
    /* ------------------------------------------------- */
    if(aoPoint->dbmax)
    {
        convDataH = 2047 * anaData;
        convDataH = convDataH / aoPoint->dbmax;
        if(dnp->type == TYPE_GIPAM2000)	convDataH = convDataH / 10;
    }
    else
    {
        convDataH = anaData;
        if(dnp->type == TYPE_GIPAM2000)	convDataH = convDataH / 10;            
    }

    /* ------------------------------------------------- */
    /* LANDIS HOST Conversio Data into 12 Bit resolution */
    /* 0x800 : 0, 0xFFF = + max full    */
    /* 0x7ff : 0, 0     = - max full    */
    /* ------------------------------------------------- */
    if(aoPoint->dbmax)
    {
        if(adata > 0)
        {
            convDataL = 2047 * anaData;
            convDataL = convDataL / aoPoint->dbmax;
            if(dnp->type == TYPE_GIPAM2000)	convDataL = convDataL / 10;                
            convDataL = convDataL + 2048;
        }
        else
        {
            adata = adata * (-1);
            convDataL = 2048 * anaData;
            convDataL = convDataL / aoPoint->dbmax;
            if(dnp->type == TYPE_GIPAM2000)	convDataL = convDataL / 10;
            convDataL = 2048 - convDataL;
        }
    }
    else
    {
        convDataL = adata;
    }

#endif    
    
    return (0);
}


/*----------------------------------------------------------------------------
* Function Name : storeAI_DNP()
---------------------------------------------------------------------------- */
int storeAI_DNP(int devid, int point, float fdata, byte *lowdata)
{
    int     portNum, ptNum;
    int     pointIndex;
    int     hostid, hostPt;
    int     anaData;
    int     dnpIndex;
    //int     delta;
    int     adata;
    //word    front, rear;
    float   *fptr1;
    //float   delta_f;
    long    convData;
    //float   fdata, *fptr1;
    //byte    data[8];

    DNP_DEVICE      *dnp;
    POINT_BUF       *aiPoint;
    HOST_DCB        *host;   // DNP HOST
    DNP_ANA_INPUT   *anaPoint;
    //ANALOG_COA_ENTRY    *anaCOA;
    HOST_ANABUF     *hostB;

    dnp = (DNP_DEVICE *) dnpcfg[devid];

    /* -------------------------------- */
    /* 지멘스 통신상태 : Active Flag    		*/
    /* -------------------------------- */
    if(dnp->type == TYPE_MODBUS_SZ)
    {	
	    if((dnp->activeMode == SET) && (dnp->activeFlag == RESET))
    	{
	        if(opr->msgDebug == devid)
    	    Debug(console,"dev%2d> store_AI... Active Status -> *RESET* ... return\n", devid+1);   
        	return (0); 
    	}
	}
	
    /* DB-Update 시... RETURN */
    if(opr->dbUpdate == SET)    return (0);
    
    /* ---------------------------------------- */
    /* 2014.12.11 수신된 POINT 예외처리...      		*/
    /* ---------------------------------------- */
    if((point >= MAX_DEV_AI_POINT) || (point >= dnp->devAiPoint))
    {
        Debug(console,"*** dev=%2d, func: storeAI_DNP() - *Invalid AI-Point (%d) !\n", devid+1, point);
        return (0); 
    }
            
    
    adata = (int) fdata;
    
   	/* analog(voltage,current) data /10   32bit float */
   	anaData = adata;
    	
    /* ------------------------------------------------ */
    /* DEVICE 별 Point Buffer Assign ...                */
    /* ------------------------------------------------ */
    pointIndex = point;
    aiPoint = (POINT_BUF *) &dnp->aiPtBuf[pointIndex];
    if(aiPoint->config != SET)  return 0;
        
    aiPoint->status     = PT_FLAG_ONLINE;
    aiPoint->pointData  = fdata;  /* GIPAM ASSIGN */
    aiPoint->floatData  = fdata;

    /* ------------------------------------ */ 
    /* DNP HOST Data Assign...              */
    /*  => DNP-HOST 별 상태포인트 정보 저장 */
    /* ------------------------------------ */ 
    for(hostid = 0; hostid < MAX_HOST; hostid++)
    {
        host = (HOST_DCB *) hostDCB[hostid];
        if(host->hostDualMode == HOST_NOT_USE)	continue;
       	if(host->hostProtocol != HOST_DNP)  continue;

		/* -------------------------------- */
    	/* DNP HOST Conversio Data 			*/
    	/* -------------------------------- */
        dnpIndex = aiPoint->hostIndex[hostid];             // 1'st DNP-HOST
        
        if((dnpIndex > 0) && (dnpIndex <= MAX_DNP_AI_POINT))
        {
            dnpIndex = dnpIndex - 1;
            anaPoint = (DNP_ANA_INPUT *) &host->ana_pointData[dnpIndex];
            anaPoint->flag = PT_FLAG_ONLINE;
            anaPoint->pointData = adata;
        
            /* ---------------------------------------------------- */
            /* 2009.05.26 INTEGER 값을 FLOAT 값으로 변환, 저장함    */
            /* ---------------------------------------------------- */
            fptr1 = (float *) &lowdata[0];
            fdata = (float ) *fptr1;
            
            anaPoint->floatData = fdata;
            
            anaPoint->lowdata[0] = lowdata[3];
            anaPoint->lowdata[1] = lowdata[2];
            anaPoint->lowdata[2] = lowdata[1];
            anaPoint->lowdata[3] = lowdata[0];
            
            //if(hostid == 1)	Debug(console,"AI(%3d) : %4.2f \n", dnpIndex, fdata);
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

		/* ---------------------------------------- */
		/*	HARRIS-HOST #1 Analog Update...			*/
		/* ---------------------------------------- */
		portNum = aiPoint->dbport1 - 1;                                          
  		ptNum   = aiPoint->dbpoint1 - 1;  
   	    
   	    if((portNum >= 0) && (portNum < MAX_PORT) && (ptNum >= 0) && (ptNum < 64))
   	    {   
   	    	host = (HOST_DCB *) hostDCB[0];                                                                     
   	   	    host->portdb[portNum].pointData[ptNum] = convData;  
   	    }   
   		
   		/* ---------------------------------------- */
		/*	HARRIS-HOST #2 Analog Update...			*/
		/* ---------------------------------------- */
		portNum = aiPoint->dbport2 - 1;                                          
  		ptNum   = aiPoint->dbpoint2 - 1;  
   	    
   	    if((portNum >= 0) && (portNum < MAX_PORT) && (ptNum >= 0) && (ptNum < 64))
   	    {   
   	    	host = (HOST_DCB *) hostDCB[1];                                                                     
   	   	    host->portdb[portNum].pointData[ptNum] = convData;  
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


/*----------------------------------------------------------------------------
* Function Name : storeCOUNT_DNP()
---------------------------------------------------------------------------- */
int storeCOUNT_DNP(int devid, int point, int adata, byte *lowdata)
{
    int     portNum, ptNum;
    int     pointIndex;
    int     hostid, hostPt;
    int     anaData;
    int     dnpIndex ;
    //int     delta;
    //float   delta_f;
    //word    front, rear;
    
    float   fdata, *fptr1;
    long    convData;
    //float   fdata, *fptr1;
    //byte    data[8];

    DNP_DEVICE      *dnp;
    POINT_BUF       *aiPoint;
    HOST_DCB        *host;   // DNP HOST
    DNP_ANA_INPUT   *anaPoint;
    //ANALOG_COA_ENTRY    *anaCOA;
    HOST_ANABUF     *hostB;
    
    /* DB-Update 시... RETURN */
    if(opr->dbUpdate == SET)    return (0);
        
    dnp = (DNP_DEVICE *) dnpcfg[devid];

	/* ---------------------------------------- */
    /* 2014.12.11 수신된 POINT 예외처리...      		*/
    /* ---------------------------------------- */
    if((point >= MAX_DEV_COUNT_POINT) || (point >= dnp->devBCPoint))
    {
    	if(opr->msgDebug == devid)
        Debug(console,"*** dev=%2d, func: storeCOUNT_DNP() - *Invalid AI-Point (%d) !\n", devid+1, point);
        return (0); 
    }
    
    /* analog(voltage,current) data /10   32bit float */
    anaData = adata;
    
    /* ------------------------------------------------ */
    /* DEVICE 별 Point Buffer Assign ...                */
    /* ------------------------------------------------ */
    pointIndex = point;
    aiPoint = (POINT_BUF *) &dnp->countBuf[pointIndex];			// COUNTER 버퍼...
    if(aiPoint->config != SET)  return 0;
        
    aiPoint->status     = PT_FLAG_ONLINE;
    aiPoint->pointData  = adata;  /* GIPAM ASSIGN */
    aiPoint->floatData  = (float) adata;
    
    /* ------------------------------------ */ 
    /* DNP HOST Data Assign...              */
    /*  => DNP-HOST 별 상태포인트 정보 저장 */
    /* ------------------------------------ */ 
    for(hostid = 0; hostid < MAX_HOST; hostid++)
    {
        host = (HOST_DCB *) hostDCB[hostid];
        if(host->hostDualMode == HOST_NOT_USE)	continue;
		if(host->hostProtocol != HOST_DNP)  continue;
			
		/* -------------------------------- */
    	/* DNP HOST Conversio Data 			*/
    	/* -------------------------------- */
        dnpIndex = aiPoint->hostIndex[hostid];             // 1'st DNP-HOST
        
        if((dnpIndex > 0) && (dnpIndex <= MAX_DNP_AI_POINT))
        {
            dnpIndex = dnpIndex - 1;
            anaPoint = (DNP_ANA_INPUT *) &host->ana_pointData[dnpIndex];
            anaPoint->flag = PT_FLAG_ONLINE;
            anaPoint->pointData = adata;
        
            /* ---------------------------------------------------- */
            /* 2009.05.26 INTEGER 값을 FLOAT 값으로 변환, 저장함    */
            /* ---------------------------------------------------- */
            fptr1 = (float *) &lowdata[0];
            fdata = (float ) *fptr1;
            
            anaPoint->floatData = fdata;
            
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
        
       	    /* float to int 변한시 x10 */         
       	    //if(dnp->type == TYPE_GIPAM2000)	    convData = convData / 10;      
        }
   	    else
        {  
   	        /* dbmax를 0XFFFF로 간주 */
       	    if(dnp->type == TYPE_GIPAM2000)     convData = 2047 * anaData;
            else 						        convData = 2047 * adata;

   	        /* float to int 변한시 x10 */         
       	    //if(dnp->type == TYPE_GIPAM2000)	    convData = convData / 10;            
   	    }        

		/* ---------------------------------------- */
		/*	HARRIS-HOST #1 Analog Update...			*/
		/* ---------------------------------------- */
		portNum = aiPoint->dbport1 - 1;                                          
  		ptNum   = aiPoint->dbpoint1 - 1;  
   	    
   	    if((portNum >= 0) && (portNum < MAX_PORT) && (ptNum >= 0) && (ptNum < 64))
   	    {   
   	    	host = (HOST_DCB *) hostDCB[0];                                                                     
   	   	    host->portdb[portNum].pointData[ptNum] = convData;  
   	    }   
   		
   		/* ---------------------------------------- */
		/*	HARRIS-HOST #2 Analog Update...			*/
		/* ---------------------------------------- */
		portNum = aiPoint->dbport2 - 1;                                          
  		ptNum   = aiPoint->dbpoint2 - 1;  
   	    
   	    if((portNum >= 0) && (portNum < MAX_PORT) && (ptNum >= 0) && (ptNum < 64))
   	    {   
   	    	host = (HOST_DCB *) hostDCB[1];                                                                     
   	   	    host->portdb[portNum].pointData[ptNum] = convData;  
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



/*----------------------------------------------------------------------------
* Function Name : check_anang_change()
---------------------------------------------------------------------------- */
int check_anang_change(int hostid, int point, float fdata, byte *lowdata)
{
#if 0    
    int     millisecond;
    byte    front, rear;


    struct timeval rv; 
    time_t  tm;
    struct  tm  *localtm;
    
    HOST_DCB        *host;   // DNP HOST
    DNP_COA_QUEUE   *coaQ;
    DNP_COAQ_ENTRY  *entryCOA;
    
    host = (HOST_DCB *) hostDCB[hostid];
    coaQ = (DNP_COA_QUEUE *) &host->dnpCOAQ;

    return (0);
    
    /* ---------------------------------- */
    /* 현재시간 정보를 DNP Form 으로 변환 */
    /* ---------------------------------- */
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
    
    /* Binary Change with TIME (SOE) Queueing... */
    front = coaQ->front& 0xff;
    rear  = coaQ->rear & 0xff;
    entryCOA = ( DNP_COAQ_ENTRY *) &coaQ->coaQueue[front];
    
    entryCOA->pointIndex = point;
    entryCOA->pointFlag  = PT_FLAG_ONLINE;
    entryCOA->pointData[0] = lowdata[3];
    entryCOA->pointData[1] = lowdata[2];
    entryCOA->pointData[2] = lowdata[1];
    entryCOA->pointData[3] = lowdata[0];
    
    entryCOA->dnpTime[0] = (dnpRTCInfo.leastSignificant) & 0xff;
    entryCOA->dnpTime[1] = (dnpRTCInfo.leastSignificant >> 8) & 0xff;
    entryCOA->dnpTime[2] = (dnpRTCInfo.mostSignificant) & 0xff;
    entryCOA->dnpTime[3] = (dnpRTCInfo.mostSignificant >> 8)  & 0xff;
    entryCOA->dnpTime[4] = (dnpRTCInfo.mostSignificant >> 16) & 0xff;
    entryCOA->dnpTime[5] = (dnpRTCInfo.mostSignificant >> 24) & 0xff;
    
    coaQ->front = (front + 1) & 0xff;
    
    Debug(console,">> COA [fr=%d/re=%d]: hostid=%d, point=%d, data=%d\n", coaQ->front, coaQ->rear, hostid+1, point, (int) fdata);
#endif
    
    return (0);
}

/*----------------------------------------------------------------------------
* Function Name : storeAI_FLOAT_DNP()
---------------------------------------------------------------------------- */
int storeAI_FLOAT_DNP(int devid, int point, float fdata, byte *lowdata)
{
    int     portNum, ptNum;
    int     pointIndex;
    int     hostid, hostPt;
    int     anaData, adata;
    long    convData;
    int     dnpIndex;

    DNP_DEVICE  *dnp;
    POINT_BUF   *aiPoint;
    HOST_DCB    *host;   // DNP HOST
    DNP_ANA_INPUT   *anaPoint;
    HOST_ANABUF     *hostB;
    
    dnp = (DNP_DEVICE *) dnpcfg[devid];

    /* -------------------------------- */
    /* 지멘스 통신상태 : Active Flag    		*/
    /* -------------------------------- */
    if(dnp->type == TYPE_MODBUS_SZ)
    {
    	if((dnp->activeMode == SET) && (dnp->activeFlag == RESET))
	    {
    	    if(opr->msgDebug == devid)
        	Debug(console,"dev%2d> store_AI_FLOAT... Active Status -> *RESET* ... return\n", devid+1);   
	        return (0); 
    	}
    }
    
    /* DB-Update 시... RETURN */
    if(opr->dbUpdate == SET)    return (0);

    /* ---------------------------------------- */
    /* 2014.12.11 수신된 POINT 예외처리...      		*/
    /* ---------------------------------------- */
    if((point >= MAX_DEV_AI_POINT) || (point >= dnp->devAiPoint))
    {
        Debug(console,"*** dev=%2d, func: storeAI_FLOAT_DNP() - *Invalid AI-Point (%d) !\n", devid+1, point);
        return (0); 
    }
            
    /* Point Buffer Assign ... */
    pointIndex = point;
    aiPoint = (POINT_BUF *) &dnp->aiPtBuf[pointIndex];
    
    if(aiPoint->config != SET)  return 0;
        
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
        if(host->hostProtocol != HOST_DNP)  continue;
        	    
		/* -------------------------------------------- */
    	/*  DNP Data Formattiong...           */
    	/* -------------------------------------------- */
        dnpIndex = aiPoint->hostIndex[hostid];             // 1'st DNP-HOST

        if((dnpIndex > 0) && (dnpIndex <= MAX_DNP_AI_POINT))
        {
            dnpIndex = dnpIndex - 1;
            anaPoint = (DNP_ANA_INPUT *) &host->ana_pointData[dnpIndex];

            anaPoint->flag = PT_FLAG_ONLINE;
            anaPoint->pointData = (int) fdata;
            anaPoint->floatData = fdata;

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

		/* ---------------------------------------- */
		/*	HARRIS-HOST #1 Analog Update...			*/
		/* ---------------------------------------- */
		portNum = aiPoint->dbport1 - 1;                                          
  		ptNum   = aiPoint->dbpoint1 - 1;  
   	    
   	    if((portNum >= 0) && (portNum < MAX_PORT) && (ptNum >= 0) && (ptNum < 64))
   	    {   
   	    	host = (HOST_DCB *) hostDCB[0];                                                                     
   	   	    host->portdb[portNum].pointData[ptNum] = convData;  
   	    }   
   		
   		/* ---------------------------------------- */
		/*	HARRIS-HOST #2 Analog Update...			*/
		/* ---------------------------------------- */
		portNum = aiPoint->dbport2 - 1;                                          
  		ptNum   = aiPoint->dbpoint2 - 1;  
   	    
   	    if((portNum >= 0) && (portNum < MAX_PORT) && (ptNum >= 0) && (ptNum < 64))
   	    {   
   	    	host = (HOST_DCB *) hostDCB[1];                                                                     
   	   	    host->portdb[portNum].pointData[ptNum] = convData;  
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
        
    
/*----------------------------------------------------------------------------
* Function Name : curStore_BIT_DNP()
* 수행내용: 수신된 바이트 정보에서 각 포인트 상태정보를 Mapping 함
* ArgList :
*   1. devid    - 업데이트 하고자 하는 디바이스 번호
*   2. stsbuf   - 수신된 바이트  스트링
*   3. dataSize - 수신된 바이트 수
* Return  :
---------------------------------------------------------------------------- */
int curStore_BIT_DNP( int devid, byte *stsbuf, int dataSize)
{
    int     i,j;
    int     point, portNum, ptNum;
    //int     pointIndex;
    int     hostid, hostPt;
    byte    buf, state;
    word  dnpIndex ;

    DNP_DEVICE      *dnp;
    POINT_BUF       *diPoint;
    DIPOINT_INFO    *hostSts;
    HOST_DCB        *host;   // DNP HOST
    HOST_STSBUF     *hostB;
    
    /* DB-Update 시... RETURN */
    if(opr->dbUpdate == SET)    return (0);
        
    /* ------------------------------------ */
    /*  계전기별 최대포인트 Check...        */
    /* ------------------------------------ */
    if(dataSize > MAX_DI_DATA_COUNT)
    {
        if(opr->msgDebug == devid)
        Debug(console,"dev%2d> *rcv invalid data size ...[rcv=%d, MAX=%d] \n", devid+1,dataSize, MAX_DI_DATA_COUNT );
        
        /* 2010년 7월30일 : 최대포인트를 넘어서는 경우 최대포인트까지만 데이터 처리... */
        dataSize = MAX_DI_DATA_COUNT;
    }

    dnp = (DNP_DEVICE *) dnpcfg[devid];
    
    /* ------------------------------------ */
    /*  이중화 CPU 정보 연계                */
    /* ------------------"------------------ */
    memcpy(dnp->linkSts, stsbuf, dataSize);
    dnp->stsByteCnt = dataSize;
    
    /* -------------------------------------------------------- */
    /*  수신된 바이트 정보에서 각 포인트 정보를 추출함...       */
    /* -------------------------------------------------------- */
    for(i = 0, point=0; i < dataSize; i++)
    {
        buf = stsbuf[i];

        /* store current into harris-queue */
        for(j = 0; j < 8; j++, buf= buf >> 1, point++)
        {
            /* 계전기별 최대 상태포인트 수 Check... */
            if(point >= dnp->devDiPoint)    break;
            
            /* ------------------------------------ */
            /* DEVICE 별 포인트 정보 Update...      */
            /* ------------------------------------ */
            diPoint = (POINT_BUF *) &dnp->diPtBuf[point];
            
            /* ------------------------------------ */
            /*  포인트 타입 : 반전 포인트인 경우... */
            /* ------------------------------------ */
            if(diPoint->ptConfig == 1)  state = (~buf) & 1;       // 포인트 상태를 반전...
            else                        state = buf & 1;
            
            /* 해당 DI 포인트를 사용하지 않는경우... */
            if(diPoint->config == RESET)    
            {
                diPoint->status = state;
                continue;
            }
                
            /* -------------------------------------------- */
            /* 현재값과 dump 상태간 차이가 있는 경우...     */
            /* -------------------------------------------- */    
            if(diPoint->status != state )
            {
                if(opr->soeDebug)
                Debug(console,"dev%2d> *** DIM Status Dump mismatch ...[pt=%2d, sts=%2x, rcv=%2x] %s\n", devid+1, point+1, diPoint->status, state, diPoint->nameStr);
            }     
            
            /* 모듈/포인트부 상태 Update... */    
            diPoint->status = state;
                                
			/* -------------------------------------------- */
            /* 지멘스 : 계전기 통신상태 : Active Flag       */
            /* -------------------------------------------- */
            if(dnp->type == TYPE_MODBUS_SZ)
            {	
	            if((dnp->activeMode == SET) && (diPoint->ptType == ACTPT))
    	        {
        	        if(diPoint->status == 1)
            	    {
            	        if(opr->msgDebug == devid)
                	    Debug(console,"dev%2d> SIEMENSE: Active Flag ...   SET : pt=%2d, sts=%2x\n", devid+1, point+1, diPoint->status);
                    	dnp->activeFlag = SET;
	                }
    	            else
        	        {
        	            if(opr->msgDebug == devid)
            	        Debug(console,"dev%2d> SIEMENSE: Active Flag ...*RESET : pt=%2d, sts=%2x\n", devid+1, point+1, diPoint->status);
                	    dnp->activeFlag = RESET;
	                }    
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
            
            /* ------------------------------------ */ 
            /* DNP & IEC HOST Data Assign...        */
            /*  => DNP-HOST 별 상태포인트 정보 저장 */
            /* ------------------------------------ */ 
            for(hostid = 0; hostid < MAX_HOST; hostid++)
            {
                host = (HOST_DCB *) hostDCB[hostid];

                /* HOST 가 미정의 경우...*/
                if(host->hostDualMode == HOST_NOT_USE)  continue;
               	if(host->hostProtocol != HOST_DNP)  	continue;
                
                /* HOST별 상태포인트 ... */    
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

                    if(hostSts->multiPoint)
                    {
                        /* ------------------------------------ */
                        /* 초기화시 Multi-Point 상태 처리       */
                        /* ------------------------------------ */
                        if(dnp->initial == RESET)
                        {
                            if (diPoint->status)    host->multi_point[dnpIndex]++;
                        }

                        /* ------------------------------------ */
                        /* Multi Point에 의한 상태처리 : HOST   */
                        /* ------------------------------------ */
                        if (host->multi_point[dnpIndex])    host->sts_pointData[dnpIndex] = PT_FLAG_STS_ON | PT_FLAG_ONLINE;
                        else                                host->sts_pointData[dnpIndex] = PT_FLAG_STS_OFF| PT_FLAG_ONLINE;
                    }
                    else
                    {
                        if (diPoint->status)    
                        {
                            host->sts_pointData[dnpIndex] = PT_FLAG_STS_ON | PT_FLAG_ONLINE;
                        }
                        else
                        {
                            host->sts_pointData[dnpIndex] = PT_FLAG_STS_OFF| PT_FLAG_ONLINE;
                        }
                    }
                }                           
            }
        } /* for */
    }

    dnp->statusDump = RESET;
    dnp->initDevStatus = SET;	//HOST 통신상태 Check
    
    /* ------------------------------------ */
    /* 초기 상태덤프 후 ... 이벤트 발생     */
    /* ------------------------------------ */
    if(++dnp->initial > STATUS_DUMP_TICK)   dnp->initial = STATUS_DUMP_TICK;

    if(opr->msgDebug == devid)
    {    
        Debug(console,"rcv%2d> current Bit  Dump ... OK \n", devid+1);
        DumpBuff(console,"STS1: ", stsbuf, dataSize);
    }

    return (0);
}

/*----------------------------------------------------------------------------
* Function Name : curStore_BYTE_DNP()
* 수행내용: 수신된 바이트 정보에서 각 포인트 상태정보를 Mapping 함
* ArgList :
*   1. devid    - 업데이트 하고자 하는 디바이스 번호
*   2. stsbuf   - 수신된 바이트  스트링
*   3. dataSize - 수신된 바이트 수
* Return  :
---------------------------------------------------------------------------- */
int curStore_BYTE_DNP( int devid, byte *stsbuf, int dataSize)
{
    int     i,j;
    int     point, portNum, ptNum;
    int     hostid, hostPt;
    byte    buf;
    word  dnpIndex ;

    DNP_DEVICE      *dnp;
    POINT_BUF       *diPoint;
    DIPOINT_INFO    *hostSts;
    HOST_DCB        *host;   // DNP HOST
    HOST_STSBUF     *hostB;
    
    /* DB-Update 시... RETURN */
    if(opr->dbUpdate == SET)    return (0);
        
    /* ------------------------------------ */
    /*  계전기별 최대포인트 Check...        */
    /* ------------------------------------ */
    if(dataSize > MAX_DEV_DI_POINT)
    {
        if(opr->msgDebug == devid)
        Debug(console,"dev%2d> *rcv invalid DI point-num ...[rcv=%d, MAX=%d] \n", devid+1,dataSize, MAX_DEV_DI_POINT );
        
        /* 2010년 7월30일 : 최대포인트를 넘어서는 경우 최대포인트까지만 데이터 처리... */
        dataSize = MAX_DEV_DI_POINT;
    }

    dnp = (DNP_DEVICE *) dnpcfg[devid];
    
    /* ------------------------------------ */
    /*  이중화 CPU 정보 연계                */
    /* ------------------------------------ */
    memcpy(dnp->linkSts, stsbuf, dataSize);
    dnp->stsByteCnt = dataSize;
    
    /* -------------------------------------------------------- */
    /*  수신된 바이트 정보에서 각 포인트 정보를 추출함...       */
    /* -------------------------------------------------------- */
    for(i = 0, point=0; i < dataSize; i++)
    {
        buf = stsbuf[i];

        /* store current into harris-queue */
        for(j = 0; j < 8; j++, buf= buf >> 1, point++)
        {
            /* 계전기별 최대 상태포인트 수 Check... */
            if(point >= dnp->devDiPoint)    break;
            
            /* ------------------------------------ */
            /* DEVICE 별 포인트 정보 Update...      */
            /* ------------------------------------ */
            diPoint = (POINT_BUF *) &dnp->diPtBuf[point];
            
            /* ------------------------------------ */
            /*  포인트 타입 : 반전 포인트인 경우... */
            /* ------------------------------------ */
            if(diPoint->ptConfig == 1)  diPoint->status = (~buf) & 1;       // 포인트 상태를 반전...
            else                        diPoint->status = buf & 1;
                                
            if(diPoint->config == RESET)    continue;

			/* ------------------------------------------------ */
            /* 지멘스 계전기 : 계전기 통신상태 : Active Flag    */
            /* ------------------------------------------------ */
            if(dnp->type == TYPE_MODBUS_SZ)
            {	
	            if((dnp->activeMode == SET) && (diPoint->ptType == ACTPT))
    	        {
        	        if(diPoint->status == 1)
            	    {
            	        if(opr->msgDebug == devid)
                	    Debug(console,"dev%2d> Active Flag ...   SET : pt=%2d, sts=%2x\n", devid+1, point+1, diPoint->status);
                    	dnp->activeFlag = SET;
	                }
    	            else
        	        {
        	            if(opr->msgDebug == devid)
            	        Debug(console,"dev%2d> Active Flag ...*RESET : pt=%2d, sts=%2x\n", devid+1, point+1, diPoint->status);
                	    dnp->activeFlag = RESET;
	                }    
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
            
            /* ------------------------------------ */ 
            /* DNP & IEC HOST Data Assign...        */
            /*  => DNP-HOST 별 상태포인트 정보 저장 */
            /* ------------------------------------ */ 
            for(hostid = 0; hostid < MAX_HOST; hostid++)
            {
                host = (HOST_DCB *) hostDCB[hostid];

                /* HOST 가 미정의 경우...*/
                if(host->hostDualMode == HOST_NOT_USE)  continue;
                if(host->hostProtocol != HOST_DNP)  	continue;	
                
                /* HOST별 상태포인트 ... */    
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

                    if(hostSts->multiPoint)
                    {
                        /* ------------------------------------ */
                        /* 초기화시 Multi-Point 상태 처리       */
                        /* ------------------------------------ */
                        if(dnp->initial == RESET)
                        {
                            if (diPoint->status)    host->multi_point[dnpIndex]++;
                        }

                        /* ------------------------------------ */
                        /* Multi Point에 의한 상태처리 : HOST   */
                        /* ------------------------------------ */
                        if (host->multi_point[dnpIndex])    host->sts_pointData[dnpIndex] = PT_FLAG_STS_ON | PT_FLAG_ONLINE;
                        else                                host->sts_pointData[dnpIndex] = PT_FLAG_STS_OFF| PT_FLAG_ONLINE;
                    }
                    else
                    {
                        if (diPoint->status)    
                        {
                            host->sts_pointData[dnpIndex] = PT_FLAG_STS_ON | PT_FLAG_ONLINE;
                        }
                        else
                        {
                            host->sts_pointData[dnpIndex] = PT_FLAG_STS_OFF| PT_FLAG_ONLINE;
                        }
                    }
                }                           
            }
        } /* for */
    }

    dnp->statusDump = RESET;
    dnp->initDevStatus = SET;	//HOST 통신상태 Check
    
    /* ------------------------------------ */
    /* 초기 상태덤프 후 ... 이벤트 발생     */
    /* ------------------------------------ */
    if(++dnp->initial > STATUS_DUMP_TICK)   dnp->initial = STATUS_DUMP_TICK;

    if(opr->msgDebug == devid)
    {    
        Debug(console,"rcv%2d> current Bit  Dump ... OK \n", devid+1);
        DumpBuff(console,"STS1: ", stsbuf, dataSize);
    }

    return (0);
}


/*----------------------------------------------------------------------------
* Function Name : storeCOS_CU_DNP()
* 수행내용: 수신된 COS 정보를  내부 운영 스트럭쳐로 데이터화 하는 함수
* ArgList :
*   1. bfptr - 수신된 COS 정보의 Pointer
*   2. count - 수신된 COS 정보의 데이터 수
* Return  :
---------------------------------------------------------------------------- */
int storeCOS_CU_DNP( int devid, int point, int state)
{
    //int     i;
    int     rtuid;
    int     portNum=0, ptNum=0;
    int     hostid, hostpt;
    //uint    tick;
    int     rtuPort;
    int     dnpPoint, dnpIndex;
    byte    front, rear;
    //byte    front2, rear2;
    char    buffer[128];

    DNP_DEVICE      *dnp;
    POINT_BUF       *diPoint;
    DNP_COS_QUEUE   *dnpCOSQ;  
    //DNP_COS_QUEUE   *dnpClass2;  
    DNP_COSQ_ENTRY  *entryCOS;
    DIPOINT_INFO    *hostSts;
    HOST_DCB        *host;   
    
    HOST_STSBUF *hostB;
    LANDIS_COS_Q    *cosQ;
    COS_Q           *rtuCOS;     /* rtu format */    

    
    /* DB-Update 시... RETURN */
    if(opr->dbUpdate == SET)    return (0);
        
    dnp = (DNP_DEVICE *) dnpcfg[devid];

#if 1    
    /* 초기 상태덤프 후 ... 이벤트 발생 */
    if(dnp->initial < STATUS_DUMP_TICK)   return (0);
#endif

    /* -------------------------------- */
    /* 지멘스 통신상태 : Active Flag    */
    /* -------------------------------- */
    if(dnp->type == TYPE_MODBUS_SZ)
    {	
	    if((dnp->activeMode == SET) && (dnp->activeFlag == RESET))
    	{
        	if(opr->msgDebug == devid)
	        Debug(console,"dev%2d> check COS... Active Status -> *RESET* ... return\n", devid+1);   
    	    return (0); 
    	}
    }
    
    /* calculate Point */
    dnpPoint = point;
    if(dnpPoint >= MAX_DEV_DI_POINT)
    {
        Debug(console,"cos> *invalid point = %d ...!\n", dnpPoint+1);
        return (0);
    }

	if(opr->soeDebug)
    Debug(console,"=> COS : dev=%d, point=%d, state = %d\n", devid+1, point+1, state);
    
    /* ------------------------------------ */
    /* Device 현재상태정보 Update ...       */
    /* ------------------------------------ */
    diPoint = (POINT_BUF *) &dnp->diPtBuf[dnpPoint];
    
    /* ---------------------------------------- */
    /* 미정의 포인트의 경우... Return           */
    /* ---------------------------------------- */
    if(diPoint->config == RESET)    return 0;
        
    if((diPoint->ptType != ALLPT) && (diPoint->ptType != COSPT) && (diPoint->ptType != SOEPT))   return 0;
        
    /* ------------------------------------ */
    /*  포인트 타입 : 반전 포인트인 경우... */
    /* ------------------------------------ */
    if(diPoint->ptConfig == 1)
    {
        if(state)   state = 0;
        else        state = 1;    
    }

    diPoint->status = state;

    /* COS 정보는 이벤트 생성 않함.... */
    if(diPoint->ptType == COSPT)
    {      
        logEvent1( opr, sque, EVENT_SOE, devid, dnpPoint, state, opr->cpuMode);            // 시스템 이벤트
    }

    logHistoryQue1( opr, hque, EVENT_SOE, devid, dnpPoint, state, opr->cpuMode);       // 상태 이벤트     
    
    /* -------------------------------------------- */
    /* LOG File 저장                                */
    /* -------------------------------------------- */
    sprintf(buffer, "[COS] Device=%2d, point=%3d, state=%d ... %s",   devid+1, dnpPoint+1, state, diPoint->nameStr);    
    LogFile_MPU(opr, EVENT_SOE, buffer, strlen(buffer));
    
    /* -------------------------------- */
    /* DNP HOST : SOE Queue Update ...  */
    /* -------------------------------- */
    for(hostid=0; hostid< MAX_HOST; hostid++)
    {
        dnpIndex = diPoint->hostIndex[hostid];             // 1'st DNP-HOST
        
        if((dnpIndex < 1) || (dnpIndex > MAX_DNP_DI_POINT)) continue;
        dnpIndex = dnpIndex - 1;
        
        host = (HOST_DCB *) hostDCB[hostid];
        
        if(host->hostDualMode == HOST_NOT_USE)	continue;
        if(host->hostProtocol != HOST_DNP)  continue;
                    
        hostSts = (DIPOINT_INFO *) &host->stateInfo[dnpIndex];
        
        /* -------------------------------------------- */  
        /*  COS 발생시 상태 Update                      */     
        /* -------------------------------------------- */       
        if(hostSts->multiPoint)
        {
            if(state)   
            {
                /* -------------------------------------------- */
                /* 상태가 0-> 1 로 바뀌는 경우 ... COS 생성     */
                /* 상태가 0이 아닌경우 ... continue             */
                /* -------------------------------------------- */
                if(host->multi_point[dnpIndex] == 0)
                {
                    host->multi_point[dnpIndex]++;
                    
                    if(opr->soeDebug)
                    Debug(console,"=>Multi-COS : dev=%d, point=%d, state = %d .... hostpoint=%d, status=%d \n", devid+1, point+1, state, dnpIndex, host->multi_point[dnpIndex]);
                }
                else
                {
                    host->multi_point[dnpIndex]++;
                    Debug(console,"=>Multi-COS : dev=%d, point=%d, state = %d .... hostpoint=%d, status=%d \n", devid+1, point+1, state, dnpIndex, host->multi_point[dnpIndex]);
                    continue;
                }
            }
            else
            {
                /* -------------------------------------------- */
                /* 상태가 1-> 0 로 바뀌는 경우 ... COS 생성     */
                /* 상태가 0이 아닌경우 ... continue             */
                /* -------------------------------------------- */
                if(host->multi_point[dnpIndex] > 0)     
                {
                    host->multi_point[dnpIndex]--;
                    if(opr->soeDebug)
                    Debug(console,"=>Multi-COS : dev=%d, point=%d, state = %d .... hostpoint=%d, status=%d \n", devid+1, point+1, state, dnpIndex, host->multi_point[dnpIndex]);
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

        /* Binary Change without TIME (COS) Queueing... */
        dnpCOSQ = (DNP_COS_QUEUE *) &host->dnpCOSQ;
        front = dnpCOSQ->front& 0xff;
        rear  = dnpCOSQ->rear & 0xff;
        entryCOS = ( DNP_COSQ_ENTRY *) &dnpCOSQ->cosQueue[front];
                
        /* ---------------------------------------------------------------- */
        /* QCode 7 : 8bit Single field Quantity, 포인트가 256 이하인 경우   */
        /* ---------------------------------------------------------------- */    
        if(host->diIndexWord == RESET)
        {
            /* ------------------------------------------------------------ */
            /* QCode 7 : Single field Quantity, 포인트가 256 이하인 경우    */
            /* ------------------------------------------------------------ */
            entryCOS->cosBuf[0] = dnpIndex;                                     // 상위 포인트 번호
            if (state)  entryCOS->cosBuf[1] = PT_FLAG_STS_ON | PT_FLAG_ONLINE;
            else        entryCOS->cosBuf[1] = PT_FLAG_STS_OFF| PT_FLAG_ONLINE;    
        
            dnpCOSQ->front = (front + 1) & 0xff;
        }
        else
        {
            /* ---------------------------------------------------------------- */
            /* QCode 8 : 16bit Single field Quantity, 포인트가 256 이상인 경우  */
            /* ---------------------------------------------------------------- */    
            entryCOS->cosBuf[0] = dnpIndex;                                     // 상위 포인트 번호 
            entryCOS->cosBuf[1] = (dnpIndex >> 8) & 0xff;                       // 상위 포인트 번호 
            if (state)  entryCOS->cosBuf[2] = PT_FLAG_STS_ON | PT_FLAG_ONLINE;
            else        entryCOS->cosBuf[2] = PT_FLAG_STS_OFF| PT_FLAG_ONLINE;    
        }        
        
        if(opr->soeDebug)
        Debug(console,"==> H-COS(%d): index=%d [dev=%d/point=%d], state= %2x \n", hostid+1, dnpIndex, devid+1, point+1, state);
                
    }


    /* --------------------------- */
    /* HARRIS 주장치 #1 COS Assign */
    /* --------------------------- */
    if(opr->hostHarris == SET)
    {       
        portNum = diPoint->dbport1;
        ptNum   = diPoint->dbpoint1;

        if((portNum > 0) && (portNum <= MAX_PORT) && (ptNum > 0) && (ptNum <= 64))
        {
        	host = (HOST_DCB *) hostDCB[0];
    	
            portNum = diPoint->dbport1 - 1;
            ptNum   = diPoint->dbpoint1 - 1;

            rtuid   = host->portdb[portNum].rtuid;
        
            if(state)   host->portdb[portNum].pointData[ptNum] = 1;
            else        host->portdb[portNum].pointData[ptNum] = 0;

            if(rtuid > 0)
            {                
                rtuPort = host->portdb[portNum].harrisPort;
                rtuCOS  = (COS_Q *) &host->rtubuf[rtuid - 1].cosq;

                front = rtuCOS->front + 1;
                rear  = rtuCOS->rear;
                if(front != rear)
                {
                    rtuCOS->que[rtuCOS->front][0] = rtuPort * 2 + state;
                    rtuCOS->que[rtuCOS->front][1] = ptNum;

                    rtuCOS->front = (rtuCOS->front + 1) & 0xff;
                    
                    if(opr->soeDebug)
                    Debug(console,"==> HARRIS-COS(%d): index=%d [port=%d/point=%d], state= %2x \n", 1, portNum+1, ptNum+1, state);
                }
            }
        }

        /* --------------------------- */
        /* HARRIS 주장치 #1 COS Assign */
        /* --------------------------- */
        portNum = diPoint->dbport2;
        ptNum   = diPoint->dbpoint2;

        if((portNum > 0) && (portNum <= MAX_PORT) && (ptNum > 0) && (ptNum <= 64))
        {
    	    host = (HOST_DCB *) hostDCB[1];
    	
            portNum = diPoint->dbport2 - 1;
            ptNum   = diPoint->dbpoint2 - 1;

            rtuid   = host->portdb[portNum].rtuid;
        
            if(state)   host->portdb[portNum].pointData[ptNum] = 1;
            else        host->portdb[portNum].pointData[ptNum] = 0;

            if(rtuid > 0)
            {                
                rtuPort = host->portdb[portNum].harrisPort;
                rtuCOS  = (COS_Q *) &host->rtubuf[rtuid - 1].cosq;

                front = rtuCOS->front + 1;
                rear  = rtuCOS->rear;
                if(front != rear)
                {
                    rtuCOS->que[rtuCOS->front][0] = rtuPort * 2 + state;
                    rtuCOS->que[rtuCOS->front][1] = ptNum;

                    rtuCOS->front = (rtuCOS->front + 1) & 0xff;
                    
                    if(opr->soeDebug)
                    Debug(console,"==> HARRIS-COS(%d): index=%d [port=%d/point=%d], state= %2x \n", 2, portNum+1, ptNum+1, state);
                }
            }
        }
    }
    
    /* --------------------------- */
    /* LANDIS 주장치 #1 COS Assign */
    /* --------------------------- */
    if(opr->hostLandis == SET)
    {          
        /* ------------------------------------ */
        /* LANDIS HOST 현재상태정보 Update ... */
        /* ------------------------------------ */
        hostpt  = diPoint->landisIndex;
    
        if(hostpt > 0)
        {
    	    //printf("landis> COS: dnpPoint=%d, state= %2x ... host = %d\n", dnpPoint, state, hostpt);
    	
            hostpt = hostpt - 1;

            hostB = (HOST_STSBUF *) hostStsBuf[hostpt];
            if(state)   hostB->status   = 1;
            else        hostB->status   = 0;

            hostB->chgTick++;
#if 0
            if(hostB->chgTick >= 2)  hostB->soechg = 0x40;
            else                    hostB->soechg = 0x0;
#endif
            /* ---------------------------- */
            /* update LANDIS COS-buffer     */
            /* ---------------------------- */
            cosQ = ( LANDIS_COS_Q *) &landis->cosQ;

            front = (cosQ->front & 0xff) + 1;
            rear  = cosQ->rear & 0xff;
            if(front != rear)
            {
                front = (cosQ->front & 0xff);
                cosQ->que[front][0] = hostpt & 0xff;
                cosQ->que[front][1] = ((state << 7) & 0x80) + ((hostpt >> 8) & 0x0f);
/*
                cosQ->que[front][1] = ((state << 7) & 0x80) + ((hostpt >> 8) & 0x0f) + hostB->soechg;
*/
                cosQ->front = (cosQ->front + 1) & 0xff;
                landis->soeChange = SET;
                
                if(opr->soeDebug)
                Debug(console,"==> LANDIS-COS(%d): index=%d [port=%d/point=%d], state= %2x \n", 2, portNum+1, ptNum+1, state);
            }            
        }
        
    } 
    
    return 0;
    
}



/*----------------------------------------------------------------------------
* Function Name : storeSOE_CU_DNP()
* 수행내용: 수신된 SOE 정보를  내부 운영 스트럭쳐로 데이터화 하는 함수
* ArgList :
*   1. bfptr - 수신된 SOE 정보의 Pointer
*   2. count - 수신된 SOE 정보의 데이터 수
*  struct of buf
* | point |  status | time(5) | time(4) | time(3) | time(2) | time(1) | time(0) |     
* time format은 DNP Format 
* Return  :
---------------------------------------------------------------------------- */
int storeSOE_CU_DNP( int devid, int devPoint, int state, byte *buf)
{
    //int     i;
    int     rtuid;
    //int     point;
    int     portNum=0, ptNum=0;
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
    //DNP_SOE_QUEUE   *dnpClass1;  
    DNP_SOEQ_ENTRY  *entrySOE;
    
    DIPOINT_INFO    *hostSts;
    HOST_DCB        *host;   
    
    SOE_Q           *rtuSOE;     /* rtu format */
    HOST_STSBUF     *hostB;
    LANDIS_SOE_Q    *soeQ;
    
    LINK_SOE        *linkSoe;
    
    /* DB-Update 시... RETURN */
    if(opr->dbUpdate == SET)    return (0);
        
    dnp = (DNP_DEVICE *) dnpcfg[devid];

	/* -------------------------------- */
    /* 지멘스 통신상태 : Active Flag    */
    /* -------------------------------- */
    if(dnp->type == TYPE_MODBUS_SZ)
    {	
	    if((dnp->activeMode == SET) && (dnp->activeFlag == RESET))
    	{
        	if(opr->msgDebug == devid)
	        Debug(console,"dev%2d> check SOE... Active Status -> *RESET* ... return\n", devid+1);   
    	    return (0); 
    	}
    }
    
#if 1
    /* ------------------------------------ */
    /* 초기 상태덤프 후 ... 이벤트 발생     */
    /* ------------------------------------ */
    if(dnp->initial < STATUS_DUMP_TICK)   return (0);
#endif

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
    
    if(diPoint->ptType == ALLPT)        // SOE 와 COS를 동시 발생
    {      
        /* -------------------------------- */
        /*  SOE 포인트 : COS 자동 발생 ...  */
        /* -------------------------------- */
        storeCOS_CU_DNP( devid, devPoint, state);
    }
    
    /* ---------------------------------------- */
    /*  포인트 타입 : 반전 포인트인 경우...     */
    /* ---------------------------------------- */
    if(diPoint->ptConfig == 1)
    {
        if(state)   state = 0;
        else        state = 1;    
    }
        
    diPoint->status = state;
    
    /* ---------------------------------------- */
    /*  LINK SOE 정보 생성 ...                  */
    /* ---------------------------------------- */
    if((opr->dualCpuSts == SET) && (opr->runMode == LOCAL_MASTER))
    {        
        front = linkCfg->front & 0xff;
        linkSoe = (LINK_SOE *) &linkCfg->linkSOE[front];
        {
            linkSoe->eventCode = 0x20;              // 계전기 SOE
            linkSoe->devid    = devid;
            linkSoe->devPoint = devPoint;
            linkSoe->state = state;

            memcpy(&linkSoe->timeBuf[0], &buf[0], 6);
            linkCfg->front = (linkCfg->front + 1) & 0xff;        
        }
        
        if(opr->soeDebug)   Debug(console, "master> LINK-SOE create...dev=%d, point=%d, state=%d\n", devid, devPoint,state);
    }
    
    
    /* -------------------------------- */
    /* Convert RTC Inform ...           */
    /* -------------------------------- */
    convertRTC( (byte *)&dnpRTCInfo, (byte *)&buf[0]);
    makeDnpTimeRTC( &dnpRTC, &dnpRTCInfo);

    if(opr->soeDebug)
    Debug(console,"==> SOE: [dev=%2d / point=%2d], state= %2x, %4d/%02d/%02d %02d:%02d:%02d-%d ... %s\n",
          devid+1,devPoint+1,state,dnpRTC.year,dnpRTC.month,dnpRTC.day,dnpRTC.hour,dnpRTC.minute,dnpRTC.second,dnpRTC.millisecond, diPoint->nameStr);
    
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
    soeEvent1( opr, sque, EVENT_SOE, devid, devPoint, state, opr->cpuMode);            // 시스템 이벤트
    soeHistoryQue1( opr, hque, EVENT_SOE, devid, devPoint, state, opr->cpuMode);       // 상태 이벤트     

    /* -------------------------------------------- */
    /* LOG File 저장                                */
    /* -------------------------------------------- */
    sprintf(buffer, "Device=%2d, point=%3d, state=%d ... [%4d-%02d-%02d %02d:%02d:%02d-%03d ms] %s",  devid+1, devPoint+1, state,
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
        Debug(console,"==> H-SOE(%d): [dev=%2d / point=%2d] dnp-index=%d, state= %2x, [%4d/%02d/%02d %02d:%02d:%02d-%d] ... %s\n",
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
                        
                        if(opr->soeDebug)
                        Debug(console,"==> Harris-SOE(%d): [dev=%2d / point=%2d] port=%d, point=%d, state= %2x, [%4d/%02d/%02d %02d:%02d:%02d-%d] ... %s\n",
                            1, devid+1, devPoint+1, portNum+1, ptNum+1, state, dnpRTC.year, dnpRTC.month, dnpRTC.day,
                            dnpRTC.hour, dnpRTC.minute, dnpRTC.second,  dnpRTC.millisecond, diPoint->nameStr);   
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
                        
                        if(opr->soeDebug)
                        Debug(console,"==> Harris-SOE(%d): [dev=%2d / point=%2d] port=%d, point=%d, state= %2x, [%4d/%02d/%02d %02d:%02d:%02d-%d] ... %s\n",
                            2, devid+1, devPoint+1, portNum+1, ptNum+1, state, dnpRTC.year, dnpRTC.month, dnpRTC.day,
                            dnpRTC.hour, dnpRTC.minute, dnpRTC.second,  dnpRTC.millisecond, diPoint->nameStr); 
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
* Function Name : storeCOA_DNP()
* 수행내용: 수신된 COA 정보를  내부 운영 스트럭쳐로 데이터화 하는 함수
* ArgList :
* Return  :
---------------------------------------------------------------------------- */
int storeCOA_DNP( int fid, int devid, byte * buf, int variation)
{
#if 0    
    int     i;
    int     ioid, rtuid;
    int     point;
    int     portNum, ptNum;
    int     hostid;
    uint    tick;
    int     state, ptstate;
    int     rtuPort;
    int     dnpPoint, dnpIndex;
    byte    front, rear;
    byte    front1, rear1;
    int     hostpt;
    byte    status;
    

    DNP_DEVICE      *dnp;
    POINT_BUF       *aiPoint;
    DNP_COA_QUEUE   *dnpCOAQ; 
    DNP_COAQ_ENTRY  *entryCOA;
    HOST_DCB        *dnpHost;   
    
    dnp = (DNP_DEVICE *) dnpcfg[devid];
    
    if(dnp->initial < STATUS_DUMP_TICK)   return (0);

    /* calculate Point & status */
    dnpPoint = buf[0];
    state    = (buf[1] >> 7) & 0x01;

    if(dnpPoint >= MAX_DEV_AI_POINT)
    {
        Debug(console,"soe> *invalid point = %d ...!\n", dnpPoint+1);
        return (0);
    }

    /* -------------------------------- */
    /* Device 현재상태정보 Update ...   */
    /* -------------------------------- */
    aiPoint = (POINT_BUF *) &dnp->aiPtBuf[dnpPoint];
    aiPoint->status = state;

    if(aiPoint->config == RESET)    return 0;
    
    /* -------------------------------- */
    /* DNP HOST : SOE Queue Update ...  */
    /* -------------------------------- */
    for(hostid=0; hostid< MAX_HOST; hostid++)
    {
        dnpIndex = aiPoint->hostIndex[hostid];             // 1'st DNP-HOST
        
        //if(i==0)        dnpIndex = aiPoint->dnp_index_first;            // 1'st DNP-HOST
        //else if(i==1)   dnpIndex = aiPoint->dnp_index_second;           // 2'nd DNP-HOST
        //else if(i==2)   dnpIndex = aiPoint->dnp_index_third;            // 3'rd DNP-HOST        
        
        if((dnpIndex < 1) || (dnpIndex > MAX_DNP_AI_POINT)) continue;
        
        dnpHost = (HOST_DCB *) hostDCB[hostid];
        
        /* Binary Change with TIME (SOE) Queueing... */
        dnpCOAQ = (DNP_COA_QUEUE *) &dnpHost->dnpCOAQ;
        front = dnpCOAQ->front& 0xff;
        rear  = dnpCOAQ->rear & 0xff;
        entryCOA = ( DNP_COAQ_ENTRY *) &dnpCOAQ->coaQueue[front];

        /* ------------------------------------------------------------ */
        /* QCode 7 : Single field Quantity, 포인트가 256 이하인 경우    */
        /* ------------------------------------------------------------ */    
        entryCOA->pointIndex = dnpIndex - 1;                  // 상위 포인트 번호 
        entryCOA->pointFlag  = buf[1];                        // 포인트 Status

        if(variation == 3)      // 32-BIT Analog Change Event with Time         
        {
            entryCOA->pointData[0] = buf[2]; 
            entryCOA->pointData[1] = buf[3];
            entryCOA->pointData[2] = buf[4];
            entryCOA->pointData[3] = buf[5];
        
            entryCOA->dnpTime[0] = buf[6];
            entryCOA->dnpTime[1] = buf[7];
            entryCOA->dnpTime[2] = buf[8];
            entryCOA->dnpTime[3] = buf[9];
            entryCOA->dnpTime[4] = buf[10];
            entryCOA->dnpTime[5] = buf[11];
        }         
        if(variation == 4)      // 16-BIT Analog Change Event with Time         
        {
            entryCOA->pointData[0] = buf[2]; 
            entryCOA->pointData[1] = buf[3];
            entryCOA->pointData[2] = 0;
            entryCOA->pointData[3] = 0;
        
            entryCOA->dnpTime[0] = buf[4];
            entryCOA->dnpTime[1] = buf[5];
            entryCOA->dnpTime[2] = buf[6];
            entryCOA->dnpTime[3] = buf[7];
            entryCOA->dnpTime[4] = buf[8];
            entryCOA->dnpTime[5] = buf[9];
        }
        else
            return (0);
            
        dnpCOAQ->front = (front + 1) & 0xff;
        
    }

#endif
                                  
    return 0;
    
}


/*
*   LANDIS/HARRIS 계전기 상태정보 
*/
int modChkOffline_LANDIS(int devid, int offline)
{
    short   i;
    short   hostpt;
    //short   point;
    //short   portNum;
    //short   startPt, stopPt;
    byte    front, rear;
    byte    status;
    //int     dnpPoint;
    int     devNo;    

    //DNP_DEVICE  *dnp;
    //HOST_DCB    *host;
    POINT_BUF   *hostDev;
    LANDIS_SOE_Q    *soeQ;
    HOST_STSBUF     *hostB;
    
    /* -------------------------------------------- */
    /*  LANDIS SOE Information                      */
    /* -------------------------------------------- */
    for(i=1; i< MAX_DNP_DEVICE; i++)
    {
        //hostDev = (DEVPOINT_INFO *) &landis->deviceInfo[i];
        hostDev = (POINT_BUF *) devPtBuf[i];
        
        if(hostDev->config == RESET)    continue;
            
        devNo   = hostDev->devNo;
        hostpt  = hostDev->landisIndex;
        
        if(hostpt > 0)  hostpt = hostpt - 1;
        else            continue;
        
        /* ------------------------------ */
        /* 2019.01.24 LANDIS 상태정보 update */
        /* ------------------------------ */
        hostB = (HOST_STSBUF *) hostStsBuf[hostpt];
        hostB->status   = offline;
                    
        /* OFFLINE 이 발생한 Device 번호 */
        if(devid == devNo)
        {
            /* SOE Queue... */
            dnpRTC.year       = rtc->year;
            dnpRTC.month      = rtc->month;
            dnpRTC.day        = rtc->day;
            dnpRTC.hour       = rtc->hour;
            dnpRTC.minute     = rtc->min;
            dnpRTC.second     = rtc->sec;
            dnpRTC.millisecond= 0;

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
                if(offline)     status = 0x80;
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
                Debug(console, "landis-mod> H-SOE(com) [front=%d, rear=%d] : point = %3d state = %02x \n", soeQ->front, soeQ->rear, hostpt+1, status);
                /*printf("soe ... %d\n", hostpt);*/
            }
        }
    }
    
    return(0);
}


/*
*   계전기 통신 Online/Offline 이벤트 처리
*/
int modChkOfflineDNP(int devid, int offline)
{
    short   hostid, rtuid;
    short   rtuPort;
    short   portNum, ptNum;
    byte    front, rear;
    int     dnpIndex;
    //int     devNo, devPt;   
    int     milisec; 
    uint    tick;    
    byte    soeBuffer[32];

    HOST_DCB        *host;
    POINT_BUF       *devPoint;
    
    DNP_SOE_QUEUE   *dnpSOEQ; 
    DNP_COS_QUEUE   *dnpCOSQ;  
    
    DNP_SOEQ_ENTRY  *entrySOE;
    DNP_COSQ_ENTRY  *entryCOS;
    LINK_SOE        *linkSoe;
    
    SOE_Q           *rtuSOE;     /* rtu format */
    COS_Q           *rtuCOS;     /* rtu format */  

    PDPDTIME_DATE_TIME      dnpRTC;
    PDPDTIME_MS_SINCE_70    dnpRTCInfo;
    
    struct timeval rv; 
    time_t  tm;
    struct  tm  localtm;
        
    /* DB-Update 시... RETURN */
    if(opr->dbUpdate == SET)    return (0);

    /* ------------------------------------ */
    /* 계전기별 - 장치포인트 참조           */
    /* ------------------------------------ */
    devPoint = (POINT_BUF *) devPtBuf[devid + 1];   // 계전기 번호, 1/2...
    devPoint->status = offline;                     // [1] : 통신이상, Offline , [0] : 통신정상, Online       
    
    /* ------------------------------------ */
    /* 현재시간 정보를 DNP Form 으로 변환   */
    /* ------------------------------------ */
    time(&tm);
    //localtm = localtime(&tm);
    localtime_r (&tm, &localtm);
                
    gettimeofday(&rv, NULL);
    milisec = (rv.tv_usec / 1000) % 1000;

    dnpRTC.year       = localtm.tm_year + 1900;
    dnpRTC.month      = localtm.tm_mon + 1;
    dnpRTC.day        = localtm.tm_mday;
    dnpRTC.hour       = localtm.tm_hour;
    dnpRTC.minute     = localtm.tm_min;
    dnpRTC.second     = localtm.tm_sec;
    dnpRTC.millisecond= milisec;

    makeDnpTimeByte( &dnpRTCInfo, &dnpRTC);

    /* ---------------------------------------- */
    /*  LINK SOE 정보 생성 ...                  */
    /* ---------------------------------------- */
    if((opr->dualCpuSts == SET) && (opr->runMode == LOCAL_MASTER))
    {        
        front = linkCfg->front & 0xff;
        linkSoe = (LINK_SOE *) &linkCfg->linkSOE[front];
        {
            linkSoe->eventCode = 0x30;              // 장치포인트 이벤트 
            linkSoe->devid     = devid;
            linkSoe->devPoint  = devid + 1;         // Device Point Index
            linkSoe->state     = offline;           // 1: Offline, 0: Online     

            linkSoe->timeBuf[0] = (dnpRTCInfo.leastSignificant) & 0xff;
            linkSoe->timeBuf[1] = (dnpRTCInfo.leastSignificant >> 8) & 0xff;
            linkSoe->timeBuf[2] = (dnpRTCInfo.mostSignificant) & 0xff;
            linkSoe->timeBuf[3] = (dnpRTCInfo.mostSignificant >> 8)  & 0xff;
            linkSoe->timeBuf[4] = (dnpRTCInfo.mostSignificant >> 16) & 0xff;
            linkSoe->timeBuf[5] = (dnpRTCInfo.mostSignificant >> 24) & 0xff;
                    
            linkCfg->front = (linkCfg->front + 1) & 0xff;        
        }
        
        if(opr->soeDebug)   
        Debug(console, ">> LINK-SOE Device Comm Create...dev=%d, offline =%d\n", devid + 1, offline);
    }
    
            
    /* -------------------------------------------- */
    /*  DNP HOST : 계전기 통신상태 포인트 Update    */    
    /* -------------------------------------------- */            
    for(hostid = 0; hostid < MAX_HOST; hostid++)
    {
        host = (HOST_DCB *) hostDCB[hostid];
        
        if(host->hostDualMode == HOST_NOT_USE)	continue;
        if(host->hostProtocol != HOST_DNP)   continue;	
        	
        /* 장치포인트의 HOST 정보 추출 */
        dnpIndex = devPoint->hostIndex[hostid] - 1;
        if((dnpIndex < 0) || (dnpIndex >= MAX_DNP_DI_POINT))    continue;         
        
        /* ---------------------------------------- */
        /*  개별장치별 통신상태                     */
        /* ---------------------------------------- */
        if (offline == 0)       // Online
        {
            host->sts_pointData[dnpIndex] = PT_FLAG_STS_OFF | PT_FLAG_ONLINE;
        }
        else                    // Offline
        {
            host->sts_pointData[dnpIndex] = PT_FLAG_STS_ON  | PT_FLAG_ONLINE;                   
        }

        /* ---------------------------------------------------------------- */
        /* QCode 7 : 8bit Single field Quantity, 포인트가 256 이하인 경우   */
        /* ---------------------------------------------------------------- */    
        if(host->diIndexWord == RESET)
        {
            soeBuffer[0] = dnpIndex;
            if (offline == 0)   soeBuffer[1] = PT_FLAG_STS_OFF | PT_FLAG_ONLINE;
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
            if (offline == 0)   soeBuffer[2] = PT_FLAG_STS_OFF | PT_FLAG_ONLINE;
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
        Debug(console,"==> H-SOE-COM(%d): [dev=%2d] dnp-index=%d, offline= %2x, [%4d/%02d/%02d %02d:%02d:%02d-%d] ... %s\n",
            hostid+1, devid+1, dnpIndex+1, offline, dnpRTC.year, dnpRTC.month, dnpRTC.day,
            dnpRTC.hour, dnpRTC.minute, dnpRTC.second,  dnpRTC.millisecond, devPoint->nameStr);      
     
        /* ------------------------------------------------ */
        /* 2021.10.20 : 계전기 통신이상 포인트 .... COS/SOE 발생보완 	*/
        /*  SOE 포인트 : ALLPT 의 경우 COS 자동 발생 ...  			*/
        /* ------------------------------------------------ */
        if(devPoint->ptType == ALLPT)        // SOE 와 COS를 동시 발생
    	{      
	    	/* Binary Change without TIME (COS) Queueing... */
        	dnpCOSQ = (DNP_COS_QUEUE *) &host->dnpCOSQ;
        	front = dnpCOSQ->front& 0xff;
	        rear  = dnpCOSQ->rear & 0xff;
        	entryCOS = ( DNP_COSQ_ENTRY *) &dnpCOSQ->cosQueue[front];
                
	        /* ---------------------------------------------------------------- */
    	    /* QCode 7 : 8bit Single field Quantity, 포인트가 256 이하인 경우   */
        	/* ---------------------------------------------------------------- */    
	        if(host->diIndexWord == RESET)
    	    {
        	    /* ------------------------------------------------------------ */
	            /* QCode 7 : Single field Quantity, 포인트가 256 이하인 경우    */
    	        /* ------------------------------------------------------------ */
        	    entryCOS->cosBuf[0] = dnpIndex;                                     // 상위 포인트 번호
            	if(offline == 0)	entryCOS->cosBuf[1] = PT_FLAG_STS_OFF | PT_FLAG_ONLINE;
            	else        		entryCOS->cosBuf[1] = PT_FLAG_STS_ON | PT_FLAG_ONLINE;    
        
            	dnpCOSQ->front = (front + 1) & 0xff;
        	}
	        else
    	    {
        	    /* ---------------------------------------------------------------- */
            	/* QCode 8 : 16bit Single field Quantity, 포인트가 256 이상인 경우  */
	            /* ---------------------------------------------------------------- */    
    	        entryCOS->cosBuf[0] = dnpIndex;                                     // 상위 포인트 번호 
        	    entryCOS->cosBuf[1] = (dnpIndex >> 8) & 0xff;                       // 상위 포인트 번호 
        	    if(offline == 0)	entryCOS->cosBuf[2] = PT_FLAG_STS_OFF | PT_FLAG_ONLINE;
            	else        		entryCOS->cosBuf[2] = PT_FLAG_STS_ON | PT_FLAG_ONLINE;   
    	    }        
        
	        if(opr->soeDebug)
    	    Debug(console,"==> H-COS-COM(%d): [dev=%2d] dnp-index=%d [dev=%d/point=%d], offline= %2x \n", hostid+1, devid+1, dnpIndex+1, offline);
        }            
    }        

    /* -------------------------------------------- */
    /*  HARRIS HOST : 계전기 통신상태 포인트 Update */    
    /* -------------------------------------------- */      
    if(opr->hostHarris == SET)
    {                
        /* ---------------------------------------- */
        /*  개별장치별 통신상태                     */
        /* ---------------------------------------- */
        /* Calculate rtc time */
        tick = (dnpRTC.hour*3600000) + (dnpRTC.minute * 60000) + (dnpRTC.second * 1000) + dnpRTC.millisecond;
            
        /* 주장치-HARRIS #1 Device Comm 상태정보 */
        portNum = devPoint->dbport1 - 1 ;
        ptNum   = devPoint->dbpoint1 - 1;

        if((portNum >= 0) && (portNum < MAX_PORT) && (ptNum >= 0) && (ptNum < 64))
        {
        	host = (HOST_DCB *) hostDCB[0];
        	
            if(offline == 0)    host->portdb[portNum].pointData[ptNum] = 0;
            else                host->portdb[portNum].pointData[ptNum] = 1;

            rtuid   = host->portdb[portNum].rtuid;

            host->portdb[portNum].pointData[ptNum] = offline;
            rtuPort = host->portdb[portNum].harrisPort;

            if(rtuid > 0)
            {
                /* ---------------------------------------- */
                /* RTU - SOE Queue                          */
                /* ---------------------------------------- */
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
                    rtuSOE->que[rtuSOE->front][5] = rtuPort * 2 + offline;
                    rtuSOE->que[rtuSOE->front][6] = ptNum;
                    rtuSOE->front = (rtuSOE->front + 1) & 0xff;
                    
                    if(opr->soeDebug)
                    Debug(console, "==> Harris-SOE(%d) : [dev=%2d]  [rtuid=%d/port=%d/point=%d]....state=%d\n", 1, devid+1, rtuid, portNum+1,ptNum+1, offline);
                }
                
                /* ---------------------------------------- */
                /* RTU - COS Queue                          */
                /* ---------------------------------------- */
                rtuCOS  = (COS_Q *) &host->rtubuf[rtuid - 1].cosq;

                front = rtuCOS->front + 1;
                rear  = rtuCOS->rear;
                if(front != rear)
                {
                    rtuCOS->que[rtuCOS->front][0] = rtuPort * 2 + offline;
                    rtuCOS->que[rtuCOS->front][1] = ptNum;

                    rtuCOS->front = (rtuCOS->front + 1) & 0xff;
                    
                    if(opr->soeDebug)
                    Debug(console, "==> Harris-COS(%d) : [devid=%2d]  [rtuid=%d/port=%d/point=%d]....state=%d\n", 1, devid+1, rtuid, portNum+1,ptNum+1, offline);
                }
                
            }
        }
        
        /* 주장치-HARRIS #2 Device Comm 상태정보 */
        portNum = devPoint->dbport2 - 1 ;
        ptNum   = devPoint->dbpoint2 - 1;

        if((portNum >= 0) && (portNum < MAX_PORT) && (ptNum >= 0) && (ptNum < 64))
        {
        	host = (HOST_DCB *) hostDCB[1];
        	
            if(offline == 0)    host->portdb[portNum].pointData[ptNum] = 0;
            else                host->portdb[portNum].pointData[ptNum] = 1;

            rtuid   = host->portdb[portNum].rtuid;

            host->portdb[portNum].pointData[ptNum] = offline;
            rtuPort = host->portdb[portNum].harrisPort;

            if(rtuid > 0)
            {
                /* ---------------------------------------- */
                /* RTU - SOE Queue                          */
                /* ---------------------------------------- */
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
                    rtuSOE->que[rtuSOE->front][5] = rtuPort * 2 + offline;
                    rtuSOE->que[rtuSOE->front][6] = ptNum;
                    rtuSOE->front = (rtuSOE->front + 1) & 0xff;
                    
                    if(opr->soeDebug)
                    Debug(console, "==> Harris-SOE(%d) : [dev=%2d]  [rtuid=%d/port=%d/point=%d]....state=%d\n", 2, devid+1, rtuid, portNum+1,ptNum+1, offline);
                }
                
                /* ---------------------------------------- */
                /* RTU - COS Queue                          */
                /* ---------------------------------------- */
                rtuCOS  = (COS_Q *) &host->rtubuf[rtuid - 1].cosq;

                front = rtuCOS->front + 1;
                rear  = rtuCOS->rear;
                if(front != rear)
                {
                    rtuCOS->que[rtuCOS->front][0] = rtuPort * 2 + offline;
                    rtuCOS->que[rtuCOS->front][1] = ptNum;

                    rtuCOS->front = (rtuCOS->front + 1) & 0xff;
                    
                    if(opr->soeDebug)
                    Debug(console, "==> Harris-COS(%d) : [devid=%2d]  [rtuid=%d/port=%d/point=%d]....state=%d\n", 2, devid+1, rtuid, portNum+1,ptNum+1, offline);
                }
                
            }
        }
        
    }    

    return (0);
}
                            
              
/*----------------------------------------------------------------------------
* Function Name : setOffline()
* 수행내용: 정상적인 모듈이 Offline 되는 경우 해당모듈을 Offline 처리하는 함수
* ArgList :
*   1. ioid - Offline 되는 모듈번호
* Return  :
---------------------------------------------------------------------------- */
int setOffline( int devid)
{
    short   i, hostid;
    short   devNo;
    short   portNum;
    short   startPt, stopPt;
    char    buffer[128];

    word    millisecond;
    time_t  tm;
    struct  tm  localtm;
    struct timeval rv; 
    
    DNP_DEVICE  *dnp;
    POINT_BUF   *diPoint;
    POINT_BUF   *aiPoint;
    POINT_BUF       *devPoint;
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

    /* ------------------------------------ */
    /* 계전기별 - 장치포인트 참조           */
    /* ------------------------------------ */
    devPoint = (POINT_BUF *) devPtBuf[devid + 1];   // 계전기 번호, 1/2...
    devPoint->status = 1;                           // [1] : 통신이상, Offline , [0] : 통신정상, Online     

    /* 시간정보 추출 */
    time(&tm);
    localtime_r (&tm, &localtm);
    
    gettimeofday(&rv, NULL);
    millisecond = (rv.tv_usec / 1000) % 1000;
    
    if(opr->soeDebug || (opr->dnpDebug == devid) )    
    Debug(console,">>> module *Offline ...dev=%2d! [%02d:%02d:%02d-%03d] \n", devid+1,
        localtm.tm_hour, localtm.tm_min, localtm.tm_sec, millisecond);  
            
    logEvent1(opr, sque, EVENT_MOD_OFFLINE, devid, 0, 0, opr->cpuMode);
    logHistoryQue1(opr, hque, EVENT_MOD_OFFLINE, devid, 0, 0, opr->cpuMode);
        
    /* -------------------------------------------- */
    /*  장치 포인트가 정의된 경우... 이벤트 생성    */
    /* -------------------------------------------- */
    if(devPoint->config == SET)
    {      
        /* ---------------------------------------- */
        /* 계전기 Offline 이벤트 처리...            */
        /* ---------------------------------------- */
        modChkOfflineDNP(devid, 1);    // OFFLINE Event 
        modChkOffline_LANDIS(devid+1, 1);
    }
	
    /* -------------------------------------------- */
    /* LOG File 저장                                */
    /* -------------------------------------------- */
    sprintf(buffer, "Device=%2d... %s", devid + 1, dnp->nameStr);
    LogFile_MPU(opr, EVENT_MOD_OFFLINE, buffer, strlen(buffer));
        
    /* ---------------------------------------- */
    /*  DNP HOST : 계전기 포인트 상태 OFFLINE   */    
    /* ---------------------------------------- */            
    for(hostid = 0; hostid < MAX_HOST; hostid++)
    {
        host = (HOST_DCB *) hostDCB[hostid];
       
        /* 고장점 장치인 경우.... Offline 처리 변경 */
        if((dnp->type == TYPE_LOCATOR1) || (dnp->type == TYPE_LOCATOR2))   break;
             
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
*   MULTI Point 관련 초기화 
*/
int initial_multi(int ioid)
{
    int     i;
    int     point;
    int     devid;
    //int     pointIndex;
    int     hostid;
    //byte    buf;
    word  dnpIndex ;

    DNP_DEVICE      *dnp;
    POINT_BUF       *diPoint;
    DIPOINT_INFO    *hostSts;
    HOST_DCB        *host;   // DNP HOST
    
    /* DB-Update 시... RETURN */
    if(opr->dbUpdate == SET)    return (0);
        
    /* ------------------------------------ */ 
    /* DNP HOST Data Assign...              */
    /*  => DNP-HOST 별 상태포인트 정보 저장 */
    /* ------------------------------------ */ 
    for(hostid = 0; hostid < MAX_HOST; hostid++)
    {
        host = (HOST_DCB *) hostDCB[hostid];
        if(host->hostDualMode == HOST_NOT_USE)  continue;
        for(i=0; i< MAX_DNP_DI_POINT; i++)  host->multi_point[i] = 0;
        //Debug(console,"init> %d %d %d %d\n", host->multi_point[0],host->multi_point[1],host->multi_point[2],host->multi_point[3]);
    }
    
    /* -------------------------------- */
    /* DEVICE 별 포인트 정보 Update...  */
    /* -------------------------------- */
    for(devid=0; devid< MAX_MODULE; devid++)
    {
        dnp = (DNP_DEVICE *) dnpcfg[devid];
        if(dnp->scan == 0)  continue;
        
        if(dnp->online != SET)  continue;	
        
        for(point=0; point< dnp->devDiPoint; point++)
        {
            /* Point Buffer Assign ... */
            diPoint = (POINT_BUF *) &dnp->diPtBuf[point];
            if(diPoint->config == RESET)    continue;
            
            /* ------------------------------------ */ 
            /* DNP HOST Data Assign...              */
            /*  => DNP-HOST 별 상태포인트 정보 저장 */
            /* ------------------------------------ */ 
            for(hostid = 0; hostid < MAX_HOST; hostid++)
            {
                host = (HOST_DCB *) hostDCB[hostid];
                dnpIndex = diPoint->hostIndex[hostid];             // 1'st DNP-HOST
                
                if(host->hostDualMode == HOST_NOT_USE)	continue;
                    
                if((dnpIndex > 0) && (dnpIndex <= MAX_DNP_DI_POINT))
                {
                    dnpIndex = dnpIndex - 1;

                    /* ---------------------------------------- */
                    /* HOST 별 상태포인트 정보                  */
                    /*  MULTI Point 처리 :                      */
                    /*    - 상태에 따라서 복합처리              */
                    /* ---------------------------------------- */
                    hostSts = (DIPOINT_INFO *) &host->stateInfo[dnpIndex];

                    if(hostSts->multiPoint)
                    {
                        /* ------------------------------------ */
                        /* 초기화시 Multi-Point 상태 처리       */
                        /* ------------------------------------ */
                        if (diPoint->status)    host->multi_point[dnpIndex]++;
    
                        /* ------------------------------------ */
                        /* Multi Point에 의한 상태처리 : HOST   */
                        /* ------------------------------------ */
                        if (host->multi_point[dnpIndex])    host->sts_pointData[dnpIndex] = PT_FLAG_STS_ON | PT_FLAG_ONLINE;
                        else                                host->sts_pointData[dnpIndex] = PT_FLAG_STS_OFF| PT_FLAG_ONLINE;
                    }
                    else
                    {
                        if (diPoint->status)    
                        {
                        	host->sts_pointData[dnpIndex] = PT_FLAG_STS_ON | PT_FLAG_ONLINE;
                        }	
                        else
                        {
                        	host->sts_pointData[dnpIndex] = PT_FLAG_STS_OFF| PT_FLAG_ONLINE;
                        }
                    }
                }                           
            }
    
        } /* for */
        
    }    
    
    if(opr->soeDebug)
    Debug(console,">>> initial_multi ...end\n");
    
    return (0);
}

/*----------------------------------------------------------------------------
* Function Name : setOnline()
* 수행내용: Offline된 모듈이 정상적으로 Online 되는 경우 해당모듈을 Online 처리하는 함수
* ArgList :
*   1. ioid - Online 되는 모듈번호
* Return  :
---------------------------------------------------------------------------- */
int setOnline(int devid)
{
    short   i, hostid;
    short   portNum;
    short   devNo;
    short   startPt, stopPt;
    char    buffer[128];

    word    millisecond;
    time_t  tm;
    struct  tm  localtm;
    struct timeval rv; 
    
    DNP_DEVICE      *dnp;
    POINT_BUF       *diPoint;
    POINT_BUF       *aiPoint;
    POINT_BUF       *devPoint;
    HOST_DCB        *host;

    DIPOINT_INFO    *hostSts;
    AIPOINT_INFO    *hostAna;
    DNP_ANA_INPUT   *dnpAna; 
    
    /* DB-Update 시... RETURN */
    if(opr->dbUpdate == SET)    return (0);
        
    /* DEVICE Online 처리 */
    if((devid < 0) || (devid >= MAX_MODULE))    return (0);

    dnp = (DNP_DEVICE *) dnpcfg[devid];
    dnp->online     = SET;
    dnp->initial    = RESET;        // 상태덤프 초기화
    dnp->offlineTick = 0;

    /* 시간정보 추출 */
    time(&tm);
    localtime_r (&tm, &localtm);
    
    gettimeofday(&rv, NULL);
    millisecond = (rv.tv_usec / 1000) % 1000;
    
    if(opr->soeDebug)   
    Debug(console,">>> module Online ...dev=%2d! [%02d:%02d:%02d-%03d] \n", devid+1,
        localtm.tm_hour, localtm.tm_min, localtm.tm_sec, millisecond);    
    
    /* ------------------------------------ */
    /* 계전기별 - 장치포인트 참조           */
    /* ------------------------------------ */
    devPoint = (POINT_BUF *) devPtBuf[devid + 1];   // 계전기 번호, 1/2...
    devPoint->status = 0;                           // [1] : 통신이상, Offline , [0] : 통신정상, Online     
    
    logEvent1(opr, sque, EVENT_MOD_ONLINE, devid, 0, 0, opr->cpuMode);
    logHistoryQue1(opr, hque, EVENT_MOD_ONLINE, devid, 0, 0, opr->cpuMode);
            
    /* -------------------------------------------- */
    /*  장치 포인트가 정의된 경우... 이벤트 생성    */
    /* -------------------------------------------- */
    if(devPoint->config == SET)
    {      
        /* ---------------------------------------- */
        /* 계전기 Offline 이벤트 처리...            */
        /* ---------------------------------------- */
        modChkOfflineDNP(devid, 0);    // ONLINE Event
        modChkOffline_LANDIS(devid+1, 0);
    }
    
    /* -------------------------------------------- */
    /* LOG File 저장                                */
    /* -------------------------------------------- */
    sprintf(buffer, "Device=%2d... %s", devid + 1, dnp->nameStr);
    LogFile_MPU(opr, EVENT_MOD_ONLINE, buffer, strlen(buffer));
      
    initial_multi(devid);
 
    /* ONLINE 된 시점에서 초기데이터 덤프 */
    dnp->timeSyncReq = SET;
    dnp->statusDump  = SET;
    dnp->analogDump  = SET;

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
    
    /* ---------------------------------------- */
    /* HARRIS : ONline에  따른 PORT Online      */
    /* ---------------------------------------- */
    if(opr->hostHarris == SET)
    {        
        startPt = 0;
        stopPt  = dnp->devDiPoint;    
        for(i=startPt;i < stopPt; i++)
        {
            diPoint = (POINT_BUF *) &dnp->diPtBuf[i];
            if(diPoint->config == RESET)    continue;

            /* HOST-HARRIS #1 Port Offline SET */
            portNum = diPoint->dbport1 - 1 ;
            if((portNum >= 0) && (portNum < MAX_PORT))  
            {
            	host = (HOST_DCB *) hostDCB[0];
                host->portdb[portNum].portStatus &= (~OL);        /* 주장치 #1 PORT Assign */
            }
        
            /* HOST-HARRIS #2 Port Offline SET */
            portNum = diPoint->dbport2 - 1 ;
            if((portNum >= 0) && (portNum < MAX_PORT))  
            {
            	host = (HOST_DCB *) hostDCB[1];
                host->portdb[portNum].portStatus &= (~OL);        /* 주장치 #1 PORT Assign */
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
                host->portdb[portNum].portStatus &= (~OL);        /* 주장치 #1 PORT Assign */
            }
        
            /* HOST-HARRIS #2 Port Offline SET */
            portNum = aiPoint->dbport2 - 1 ;
            if((portNum >= 0) && (portNum < MAX_PORT))  
            {
            	host = (HOST_DCB *) hostDCB[1];
                host->portdb[portNum].portStatus &= (~OL);        /* 주장치 #1 PORT Assign */
            }
        }                           
    }                    
    
    return (0);
}


/*
*   DEVICE POINT SOE 정보 ....
*/
int Create_Device_SOE(int devPt, int pointState)
{
    int     hostid, hostpt;
    int     dnpPoint;
    int     milisec;
    int     devNo, modPoint;
    int     portNum, ptNum;
    byte    front, rear;
    byte    status;
    uint    tick;
    int     rtuid;
    int     rtuPort;
    byte    soeBuffer[16];
    char    buffer[128];
    
    POINT_BUF   *devPoint;       
    HOST_DCB        *host;
    DNP_SOE_QUEUE   *dnpSOEQ; 
    DNP_COS_QUEUE   *dnpCOSQ;  
    DNP_SOEQ_ENTRY  *entrySOE;
    DNP_COSQ_ENTRY  *entryCOS;
    LINK_SOE        *linkSoe;
    SOE_Q           *rtuSOE;     /* rtu format */
    COS_Q           *rtuCOS;     /* rtu format */  
    
    HOST_STSBUF     *hostB;
    LANDIS_SOE_Q    *soeQ;
    
    PDPDTIME_DATE_TIME      dnpRTC;
    PDPDTIME_MS_SINCE_70    dnpRTCInfo;
    
    struct timeval rv; 
    time_t  tm;
    struct  tm  localtm;
    
    /* ---------------------------------------- */
    /*  장치 포인트 관리...                     */
    /* ---------------------------------------- */
    devPoint = (POINT_BUF *) devPtBuf[devPt];       // Device Point 정보
    
    if(devPoint->config != SET) return (0);
        
    devPoint->status = pointState;                  // Device Point 상태
    
    /* 계전기 포인트... */
    devNo = devPoint->devNo - 1;
    modPoint = devPoint->devPt - 1;
     
    /* ---------------------------------- */
    /* 현재시간 정보를 DNP Form 으로 변환 */
    /* ---------------------------------- */
    time(&tm);
    //localtm = localtime(&tm);
    localtime_r (&tm, &localtm);
                
    gettimeofday(&rv, NULL);
    milisec = (rv.tv_usec / 1000) % 1000;

    dnpRTC.year       = localtm.tm_year + 1900;
    dnpRTC.month      = localtm.tm_mon + 1;
    dnpRTC.day        = localtm.tm_mday;
    dnpRTC.hour       = localtm.tm_hour;
    dnpRTC.minute     = localtm.tm_min;
    dnpRTC.second     = localtm.tm_sec;
    dnpRTC.millisecond= milisec;

    makeDnpTimeByte( &dnpRTCInfo, &dnpRTC);
                          
    /* -------------------------------------------- */
    /* 수신 SOE 이벤트 시간 추출                    */
    /* -------------------------------------------- */
    if(opr->soeDebug)
    Debug(console, ">> create SOE ... Device-Point = %3d, state = %d ...[%4d/%2d/%2d %02d:%02d:%02d-%d ms] %s\n", 
        devPt + 1, pointState, localtm.tm_year + 1900, localtm.tm_mon +1, localtm.tm_mday, localtm.tm_hour, localtm.tm_min, 
        localtm.tm_sec, milisec, devPoint->nameStr);

    /* ---------------------------------------- */
    /*  SOE 관련 이벤트 생성 ...                */
    /*  -- 시스템 이벤트 : MMI 에서 참조        */
    /*  -- 상태 이벤트 : Console에서 참조       */
    /* ---------------------------------------- */
    if(devPt == 0)      // 전체 통신상태 포인트의 경우...
    {
        /* ---------------------------------------- */
        /*  이벤트 생성은 하지않고... History 만    */
        /* ---------------------------------------- */
        //logEvent1( opr, sque, EVENT_SOE, devNo, modPoint, pointState, devPt);          // 시스템 이벤트
        logHistoryQue1( opr, hque, EVENT_SOE, devNo, modPoint, pointState, devPt);       // 상태 이벤트   
    }
    else
    {            
        logEvent1( opr, sque, EVENT_SOE, devNo, modPoint, pointState, devPt);            // 시스템 이벤트
        logHistoryQue1( opr, hque, EVENT_SOE, devNo, modPoint, pointState, devPt);       // 상태 이벤트     
    }
    
    /* -------------------------------------------- */
    /* LOG File 저장                                */
    /* -------------------------------------------- */
    sprintf(buffer, "Device-SOE %2d...state = %d,  %s", devPt, pointState, devPoint->nameStr);
    LogFile_MPU(opr, EVENT_SOE, buffer, strlen(buffer));
                
    /* ---------------------------------------- */
    /*  LINK SOE 정보 생성 ...                  */
    /* ---------------------------------------- */
    if((opr->dualCpuSts == SET) && (opr->runMode == LOCAL_MASTER))
    {        
        front = linkCfg->front & 0xff;
        linkSoe = (LINK_SOE *) &linkCfg->linkSOE[front];
        {
            linkSoe->eventCode = 0x30;              // 장치포인트 SOE
            linkSoe->devid     = devPt;
            linkSoe->devPoint  = devPt;             // Device Point Index
            linkSoe->state     = pointState;

            //memcpy(&linkSoe->timeBuf[0], &dnpRTCInfo, 6);
            linkSoe->timeBuf[0] = (dnpRTCInfo.leastSignificant) & 0xff;
            linkSoe->timeBuf[1] = (dnpRTCInfo.leastSignificant >> 8) & 0xff;
            linkSoe->timeBuf[2] = (dnpRTCInfo.mostSignificant) & 0xff;
            linkSoe->timeBuf[3] = (dnpRTCInfo.mostSignificant >> 8)  & 0xff;
            linkSoe->timeBuf[4] = (dnpRTCInfo.mostSignificant >> 16) & 0xff;
            linkSoe->timeBuf[5] = (dnpRTCInfo.mostSignificant >> 24) & 0xff;
                    
            linkCfg->front = (linkCfg->front + 1) & 0xff;        
        }
        
        if(opr->soeDebug)   Debug(console, ">> Device-Point : LINK-SOE create...point=%d, status =%d\n", devPt+1, pointState);
    }

	/* ---------------------------------------------------- */
	/*	DEVICE-EVENT : DNP-HOST 							*/   
	/* ---------------------------------------------------- */         
    for(hostid=0; hostid < MAX_HOST; hostid++)
    {
        host = (HOST_DCB *) hostDCB[hostid];
        if(host->hostDualMode == HOST_NOT_USE)	continue;
        if(host->hostProtocol != HOST_DNP)  continue;
        	    
        dnpPoint = devPoint->hostIndex[hostid] - 1;         // SDP POINT : 상위 호스트#1 Index 번호 [1..1024]
        
        if((dnpPoint < 0) || (dnpPoint >= MAX_DNP_DI_POINT))    continue;         

        /* ---------------------------------------------------------------- */
        /* QCode 7 : 8bit Single field Quantity, 포인트가 256 이하인 경우   */
        /* ---------------------------------------------------------------- */    
        if(host->diIndexWord == RESET)
        {
            soeBuffer[0] = dnpPoint;
            if(pointState == 0)			soeBuffer[1] = 0x01;
            else 						soeBuffer[1] = 0x81;	
    
            soeBuffer[2] = (dnpRTCInfo.leastSignificant) & 0xff;
            soeBuffer[3] = (dnpRTCInfo.leastSignificant >> 8) & 0xff;
            soeBuffer[4] = (dnpRTCInfo.mostSignificant) & 0xff;
            soeBuffer[5] = (dnpRTCInfo.mostSignificant >> 8)  & 0xff;
            soeBuffer[6] = (dnpRTCInfo.mostSignificant >> 16) & 0xff;
            soeBuffer[7] = (dnpRTCInfo.mostSignificant >> 24) & 0xff;

            /* Binary Change with TIME (SOE) Queueing... */
            dnpSOEQ = (DNP_SOE_QUEUE *) &host->dnpSOEQ;
            front = dnpSOEQ->front& 0xff;
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
            //dnpSOEQ->front1 = (front + 1) & 0xff;       // 이중화 CPU 연계 
        }
        else
        {
            /* ---------------------------------------------------------------- */
            /* QCode 8 : 16bit Single field Quantity, 포인트가 256 이하인 경우  */
            /* ---------------------------------------------------------------- */    
            soeBuffer[0] = dnpPoint;
            soeBuffer[1] = (dnpPoint >> 8) & 0xff;
            
            if(pointState == 0)			soeBuffer[2] = 0x01;
            else 						soeBuffer[2] = 0x81;	
    
            soeBuffer[3] = (dnpRTCInfo.leastSignificant) & 0xff;
            soeBuffer[4] = (dnpRTCInfo.leastSignificant >> 8) & 0xff;
            soeBuffer[5] = (dnpRTCInfo.mostSignificant) & 0xff;
            soeBuffer[6] = (dnpRTCInfo.mostSignificant >> 8)  & 0xff;
            soeBuffer[7] = (dnpRTCInfo.mostSignificant >> 16) & 0xff;
            soeBuffer[8] = (dnpRTCInfo.mostSignificant >> 24) & 0xff;

            /* Binary Change with TIME (SOE) Queueing... */
            dnpSOEQ = (DNP_SOE_QUEUE *) &host->dnpSOEQ;
            front = dnpSOEQ->front& 0xff;
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
            //dnpSOEQ->front1 = (front + 1) & 0xff;       // 이중화 CPU 연계 
        }
            
        if(opr->soeDebug)
        Debug(console,"==> H-SOE(%d): [Device Point-%2d] dnp-index=%d, state= %2x, [%4d/%02d/%02d %02d:%02d:%02d-%d] ... %s\n",
            hostid+1, devPt+1, dnpPoint+1, pointState, dnpRTC.year, dnpRTC.month, dnpRTC.day, dnpRTC.hour,dnpRTC.minute,dnpRTC.second,dnpRTC.millisecond, devPoint->nameStr); 
            

    	/* ------------------------------------------------ */
        /* 2021.10.20 : 계전기 통신이상 포인트 .... COS/SOE 발생보완 	*/
        /*  SOE 포인트 : ALLPT 의 경우 COS 자동 발생 ...  			*/
        /* ------------------------------------------------ */
        if(devPoint->ptType == ALLPT)        // SOE 와 COS를 동시 발생
    	{      
	    	/* Binary Change without TIME (COS) Queueing... */
        	dnpCOSQ = (DNP_COS_QUEUE *) &host->dnpCOSQ;
        	front = dnpCOSQ->front& 0xff;
	        rear  = dnpCOSQ->rear & 0xff;
        	entryCOS = ( DNP_COSQ_ENTRY *) &dnpCOSQ->cosQueue[front];
                
	        /* ---------------------------------------------------------------- */
    	    /* QCode 7 : 8bit Single field Quantity, 포인트가 256 이하인 경우   */
        	/* ---------------------------------------------------------------- */    
	        if(host->diIndexWord == RESET)
    	    {
        	    /* ------------------------------------------------------------ */
	            /* QCode 7 : Single field Quantity, 포인트가 256 이하인 경우    */
    	        /* ------------------------------------------------------------ */
        	    entryCOS->cosBuf[0] = dnpPoint;                                     // 상위 포인트 번호
            	if(pointState == 0)	entryCOS->cosBuf[1] = PT_FLAG_STS_OFF | PT_FLAG_ONLINE;
            	else        		entryCOS->cosBuf[1] = PT_FLAG_STS_ON | PT_FLAG_ONLINE;    
        
            	dnpCOSQ->front = (front + 1) & 0xff;
        	}
	        else
    	    {
        	    /* ---------------------------------------------------------------- */
            	/* QCode 8 : 16bit Single field Quantity, 포인트가 256 이상인 경우  */
	            /* ---------------------------------------------------------------- */    
    	        entryCOS->cosBuf[0] = dnpPoint;                                     // 상위 포인트 번호 
        	    entryCOS->cosBuf[1] = (dnpPoint >> 8) & 0xff;                       // 상위 포인트 번호 
        	    if(pointState == 0)	entryCOS->cosBuf[2] = PT_FLAG_STS_OFF | PT_FLAG_ONLINE;
            	else        		entryCOS->cosBuf[2] = PT_FLAG_STS_ON | PT_FLAG_ONLINE;   
    	    }        
        
	        if(opr->soeDebug)
    	    Debug(console,"==> H-COS(%d): [Device Point-%2d] dnp-index=%d [dev=%d/point=%d], offline= %2x \n", hostid+1, devPt+1, dnpPoint+1, pointState);
        }            
                    
    }        

	/* ---------------------------------------------------- */
	/*	DEVICE-EVENT : HARRIS-HOST 							*/   
	/* ---------------------------------------------------- */    
    if(opr->hostHarris == SET)
    {
        /* Calculate rtc time */
        tick = (dnpRTC.hour*3600000) + (dnpRTC.minute * 60000) + (dnpRTC.second * 1000) + dnpRTC.millisecond;

		/* -------------------------------------------- */
		/*	HOST[0] HARRIS 적용 							*/
		/* -------------------------------------------- */
        portNum = devPoint->dbport1;
        ptNum   = devPoint->dbpoint1;
    
        if((portNum > 0) && (portNum <= MAX_PORT) && (ptNum > 0) && (ptNum <= 64))
        {
        	host = (HOST_DCB *) hostDCB[0];			// Default HOST[0]
    	
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
                        
                    if(opr->soeDebug)
  	                Debug(console,"==> HARRIS-SOE(%d): [port=%d/point=%d], state= %2x \n", 1, portNum+1, ptNum+1, pointState);
                }
                
                /* first COS Buffering... */
   	            rtuCOS  = (COS_Q *) &host->rtubuf[rtuid - 1].cosq;

       	       	front = rtuCOS->front + 1;
           	    rear  = rtuCOS->rear;
            	if(front != rear)
       	        {
   	       	        rtuCOS->que[rtuCOS->front][0] = rtuPort * 2 + pointState;
       	       	    rtuCOS->que[rtuCOS->front][1] = ptNum;

           	       	rtuCOS->front = (rtuCOS->front + 1) & 0xff;
                    
               	    if(opr->soeDebug)
                	Debug(console,"==> HARRIS-COS(%d): [port=%d/point=%d], state= %2x \n", 1, portNum+1, ptNum+1, pointState);
                }
            }
        }        
        
        /* -------------------------------------------- */
		/*	HOST[1] HARRIS 적용 							*/
		/* -------------------------------------------- */
        portNum = devPoint->dbport2;
        ptNum   = devPoint->dbpoint2;
    
        if((portNum > 0) && (portNum <= MAX_PORT) && (ptNum > 0) && (ptNum <= 64))
        {
        	host = (HOST_DCB *) hostDCB[1];			// Default HOST[0]
    	
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
                        
                    if(opr->soeDebug)
  	                Debug(console,"==> HARRIS-SOE(%d): [port=%d/point=%d], state= %2x \n", 2, portNum+1, ptNum+1, pointState);
                }
                
                /* first COS Buffering... */
   	            rtuCOS  = (COS_Q *) &host->rtubuf[rtuid - 1].cosq;

       	       	front = rtuCOS->front + 1;
           	    rear  = rtuCOS->rear;
            	if(front != rear)
       	        {
   	       	        rtuCOS->que[rtuCOS->front][0] = rtuPort * 2 + pointState;
       	       	    rtuCOS->que[rtuCOS->front][1] = ptNum;

           	       	rtuCOS->front = (rtuCOS->front + 1) & 0xff;
                    
               	    if(opr->soeDebug)
                	Debug(console,"==> HARRIS-COS(%d): [port=%d/point=%d], state= %2x \n", 2, portNum+1, ptNum+1, pointState);
                }
            }
        }
    }
            
    /* ---------------------------------------------------- */
	/*	DEVICE-EVENT : LANDIS-HOST 							*/   
	/* ---------------------------------------------------- */    
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
                Debug(console, "==> LANDIS-SOE[front=%d, rear=%d] : point = %3d state = %02x \n", soeQ->front, soeQ->rear, hostpt+1, status);
            }
        }
    }        
        
    return (0);     
}


/*
*	2026.07.07 HOST별 전체 통신상태 처리....
*   DEVICE POINT SOE 정보 ....
*/
int Create_ALL_Device_SOE(int hostid, int devPt, int pointState)
{
    int     hostpt;
    int     dnpPoint;
    int     milisec;
    int     devNo, modPoint;
    int     portNum, ptNum;
    byte    front, rear;
    byte    status;
    uint    tick;
    int     rtuid;
    int     rtuPort;
    byte    soeBuffer[16];
    char    buffer[128];
    
    POINT_BUF   *devPoint;       
    HOST_DCB        *host;
    DNP_SOE_QUEUE   *dnpSOEQ; 
    DNP_COS_QUEUE   *dnpCOSQ;  
    DNP_SOEQ_ENTRY  *entrySOE;
    DNP_COSQ_ENTRY  *entryCOS;
    //LINK_SOE        *linkSoe;
    SOE_Q           *rtuSOE;     /* rtu format */
    COS_Q           *rtuCOS;     /* rtu format */  
    
    HOST_STSBUF     *hostB;
    LANDIS_SOE_Q    *soeQ;
    
    PDPDTIME_DATE_TIME      dnpRTC;
    PDPDTIME_MS_SINCE_70    dnpRTCInfo;
    
    struct timeval rv; 
    time_t  tm;
    struct  tm  localtm;
    
    /* ---------------------------------------- */
    /*  장치 포인트 관리...                     */
    /* ---------------------------------------- */
    devPoint = (POINT_BUF *) devPtBuf[devPt];       // Device Point 정보
    
    if(devPoint->config != SET) return (0);
        
    devPoint->status = pointState;                  // Device Point 상태
    
    /* 계전기 포인트... */
    devNo = devPoint->devNo - 1;
    modPoint = devPoint->devPt - 1;
     
    /* ---------------------------------- */
    /* 현재시간 정보를 DNP Form 으로 변환 */
    /* ---------------------------------- */
    time(&tm);
    //localtm = localtime(&tm);
    localtime_r (&tm, &localtm);
                
    gettimeofday(&rv, NULL);
    milisec = (rv.tv_usec / 1000) % 1000;

    dnpRTC.year       = localtm.tm_year + 1900;
    dnpRTC.month      = localtm.tm_mon + 1;
    dnpRTC.day        = localtm.tm_mday;
    dnpRTC.hour       = localtm.tm_hour;
    dnpRTC.minute     = localtm.tm_min;
    dnpRTC.second     = localtm.tm_sec;
    dnpRTC.millisecond= milisec;

    makeDnpTimeByte( &dnpRTCInfo, &dnpRTC);
                          
    /* -------------------------------------------- */
    /* 수신 SOE 이벤트 시간 추출                    */
    /* -------------------------------------------- */
    if(opr->soeDebug)
    Debug(console, ">> create SOE ... ALL Device-Point = %3d, state = %d ...[%4d/%2d/%2d %02d:%02d:%02d-%d ms] %s\n", 
        devPt + 1, pointState, localtm.tm_year + 1900, localtm.tm_mon +1, localtm.tm_mday, localtm.tm_hour, localtm.tm_min, 
        localtm.tm_sec, milisec, devPoint->nameStr);

    /* ---------------------------------------- */
    /*  SOE 관련 이벤트 생성 ...                */
    /*  -- 시스템 이벤트 : MMI 에서 참조        */
    /*  -- 상태 이벤트 : Console에서 참조       */
    /* ---------------------------------------- */
    if(devPt == 0)      // 전체 통신상태 포인트의 경우...
    {
        /* ---------------------------------------- */
        /*  이벤트 생성은 하지않고... History 만    */
        /* ---------------------------------------- */
        //logEvent1( opr, sque, EVENT_SOE, devNo, modPoint, pointState, devPt);          // 시스템 이벤트
        logHistoryQue1( opr, hque, EVENT_SOE, devNo, modPoint, pointState, devPt);       // 상태 이벤트   
    }
    else
    {            
        logEvent1( opr, sque, EVENT_SOE, devNo, modPoint, pointState, devPt);            // 시스템 이벤트
        logHistoryQue1( opr, hque, EVENT_SOE, devNo, modPoint, pointState, devPt);       // 상태 이벤트     
    }
    
    /* -------------------------------------------- */
    /* LOG File 저장                                */
    /* -------------------------------------------- */
    sprintf(buffer, "ALL Device-SOE %2d...state = %d,  %s", devPt, pointState, devPoint->nameStr);
    LogFile_MPU(opr, EVENT_SOE, buffer, strlen(buffer));

#if 0                
    /* ---------------------------------------- */
    /*  LINK SOE 정보 생성 ...                  */
    /* ---------------------------------------- */
    if((opr->dualCpuSts == SET) && (opr->runMode == LOCAL_MASTER))
    {        
        front = linkCfg->front & 0xff;
        linkSoe = (LINK_SOE *) &linkCfg->linkSOE[front];
        {
            linkSoe->eventCode = 0x30;              // 장치포인트 SOE
            linkSoe->devid     = devPt;
            linkSoe->devPoint  = devPt;             // Device Point Index
            linkSoe->state     = pointState;

            //memcpy(&linkSoe->timeBuf[0], &dnpRTCInfo, 6);
            linkSoe->timeBuf[0] = (dnpRTCInfo.leastSignificant) & 0xff;
            linkSoe->timeBuf[1] = (dnpRTCInfo.leastSignificant >> 8) & 0xff;
            linkSoe->timeBuf[2] = (dnpRTCInfo.mostSignificant) & 0xff;
            linkSoe->timeBuf[3] = (dnpRTCInfo.mostSignificant >> 8)  & 0xff;
            linkSoe->timeBuf[4] = (dnpRTCInfo.mostSignificant >> 16) & 0xff;
            linkSoe->timeBuf[5] = (dnpRTCInfo.mostSignificant >> 24) & 0xff;
                    
            linkCfg->front = (linkCfg->front + 1) & 0xff;        
        }
        
        if(opr->soeDebug)   Debug(console, ">> Device-Point : LINK-SOE create...point=%d, status =%d\n", devPt+1, pointState);
    }
#endif

	/* ---------------------------------------------------- */
	/*	DEVICE-EVENT : DNP-HOST 							*/   
	/* ---------------------------------------------------- */         
    host = (HOST_DCB *) hostDCB[hostid];
    if(host->hostProtocol == HOST_DNP)  	
    {
        dnpPoint = devPoint->hostIndex[hostid] - 1;         // SDP POINT : 상위 호스트#1 Index 번호 [1..1024]
        
        if((dnpPoint < 0) || (dnpPoint >= MAX_DNP_DI_POINT))    return (0);         

        /* ---------------------------------------------------------------- */
        /* QCode 7 : 8bit Single field Quantity, 포인트가 256 이하인 경우   */
        /* ---------------------------------------------------------------- */    
        if(host->diIndexWord == RESET)
        {
            soeBuffer[0] = dnpPoint;
            if(pointState == 0)			soeBuffer[1] = 0x01;
            else 						soeBuffer[1] = 0x81;	
    
            soeBuffer[2] = (dnpRTCInfo.leastSignificant) & 0xff;
            soeBuffer[3] = (dnpRTCInfo.leastSignificant >> 8) & 0xff;
            soeBuffer[4] = (dnpRTCInfo.mostSignificant) & 0xff;
            soeBuffer[5] = (dnpRTCInfo.mostSignificant >> 8)  & 0xff;
            soeBuffer[6] = (dnpRTCInfo.mostSignificant >> 16) & 0xff;
            soeBuffer[7] = (dnpRTCInfo.mostSignificant >> 24) & 0xff;

            /* Binary Change with TIME (SOE) Queueing... */
            dnpSOEQ = (DNP_SOE_QUEUE *) &host->dnpSOEQ;
            front = dnpSOEQ->front& 0xff;
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
            //dnpSOEQ->front1 = (front + 1) & 0xff;       // 이중화 CPU 연계 
        }
        else
        {
            /* ---------------------------------------------------------------- */
            /* QCode 8 : 16bit Single field Quantity, 포인트가 256 이하인 경우  */
            /* ---------------------------------------------------------------- */    
            soeBuffer[0] = dnpPoint;
            soeBuffer[1] = (dnpPoint >> 8) & 0xff;
            
            if(pointState == 0)			soeBuffer[2] = 0x01;
            else 						soeBuffer[2] = 0x81;	
    
            soeBuffer[3] = (dnpRTCInfo.leastSignificant) & 0xff;
            soeBuffer[4] = (dnpRTCInfo.leastSignificant >> 8) & 0xff;
            soeBuffer[5] = (dnpRTCInfo.mostSignificant) & 0xff;
            soeBuffer[6] = (dnpRTCInfo.mostSignificant >> 8)  & 0xff;
            soeBuffer[7] = (dnpRTCInfo.mostSignificant >> 16) & 0xff;
            soeBuffer[8] = (dnpRTCInfo.mostSignificant >> 24) & 0xff;

            /* Binary Change with TIME (SOE) Queueing... */
            dnpSOEQ = (DNP_SOE_QUEUE *) &host->dnpSOEQ;
            front = dnpSOEQ->front& 0xff;
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
            //dnpSOEQ->front1 = (front + 1) & 0xff;       // 이중화 CPU 연계 
        }
            
        if(opr->soeDebug)
        Debug(console,"==> H-SOE(%d): [Device Point-%2d] dnp-index=%d, state= %2x, [%4d/%02d/%02d %02d:%02d:%02d-%d] ... %s\n",
            hostid+1, devPt+1, dnpPoint+1, pointState, dnpRTC.year, dnpRTC.month, dnpRTC.day, dnpRTC.hour,dnpRTC.minute,dnpRTC.second,dnpRTC.millisecond, devPoint->nameStr); 
            

    	/* ------------------------------------------------ */
        /* 2021.10.20 : 계전기 통신이상 포인트 .... COS/SOE 발생보완 	*/
        /*  SOE 포인트 : ALLPT 의 경우 COS 자동 발생 ...  			*/
        /* ------------------------------------------------ */
        if(devPoint->ptType == ALLPT)        // SOE 와 COS를 동시 발생
    	{      
	    	/* Binary Change without TIME (COS) Queueing... */
        	dnpCOSQ = (DNP_COS_QUEUE *) &host->dnpCOSQ;
        	front = dnpCOSQ->front& 0xff;
	        rear  = dnpCOSQ->rear & 0xff;
        	entryCOS = ( DNP_COSQ_ENTRY *) &dnpCOSQ->cosQueue[front];
                
	        /* ---------------------------------------------------------------- */
    	    /* QCode 7 : 8bit Single field Quantity, 포인트가 256 이하인 경우   */
        	/* ---------------------------------------------------------------- */    
	        if(host->diIndexWord == RESET)
    	    {
        	    /* ------------------------------------------------------------ */
	            /* QCode 7 : Single field Quantity, 포인트가 256 이하인 경우    */
    	        /* ------------------------------------------------------------ */
        	    entryCOS->cosBuf[0] = dnpPoint;                                     // 상위 포인트 번호
            	if(pointState == 0)	entryCOS->cosBuf[1] = PT_FLAG_STS_OFF | PT_FLAG_ONLINE; // 0x01 online
            	else        		entryCOS->cosBuf[1] = PT_FLAG_STS_ON | PT_FLAG_ONLINE;  // 0x81 offline   
        
            	dnpCOSQ->front = (front + 1) & 0xff;
        	}
	        else
    	    {
        	    /* ---------------------------------------------------------------- */
            	/* QCode 8 : 16bit Single field Quantity, 포인트가 256 이상인 경우  */
	            /* ---------------------------------------------------------------- */    
    	        entryCOS->cosBuf[0] = dnpPoint;                                     // 상위 포인트 번호 
        	    entryCOS->cosBuf[1] = (dnpPoint >> 8) & 0xff;                       // 상위 포인트 번호 
        	    if(pointState == 0)	entryCOS->cosBuf[2] = PT_FLAG_STS_OFF | PT_FLAG_ONLINE;
            	else        		entryCOS->cosBuf[2] = PT_FLAG_STS_ON | PT_FLAG_ONLINE;   
    	    }        
        
	        if(opr->soeDebug)
    	    Debug(console,"==> H-COS(%d): [Device Point-%2d] dnp-index=%d [dev=%d/point=%d], offline= %2x \n", hostid+1, devPt+1, dnpPoint+1, pointState);
        }            
    }
                    
	/* ---------------------------------------------------- */
	/*	DEVICE-EVENT : HARRIS-HOST 							*/   
	/* ---------------------------------------------------- */    
	if(host->hostProtocol == HOST_HARRIS) 
    //if(opr->hostHarris == SET)
    {
        /* Calculate rtc time */
        tick = (dnpRTC.hour*3600000) + (dnpRTC.minute * 60000) + (dnpRTC.second * 1000) + dnpRTC.millisecond;

		/* -------------------------------------------- */
		/*	HOST[0] HARRIS 적용 							*/
		/* -------------------------------------------- */
        portNum = devPoint->dbport1;
        ptNum   = devPoint->dbpoint1;
    
        if((portNum > 0) && (portNum <= MAX_PORT) && (ptNum > 0) && (ptNum <= 64))
        {
        	host = (HOST_DCB *) hostDCB[0];			// Default HOST[0]
    	
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
                        
                    if(opr->soeDebug)
  	                Debug(console,"==> HARRIS-SOE(%d): [port=%d/point=%d], state= %2x \n", 1, portNum+1, ptNum+1, pointState);
                }
                
                /* first COS Buffering... */
   	            rtuCOS  = (COS_Q *) &host->rtubuf[rtuid - 1].cosq;

       	       	front = rtuCOS->front + 1;
           	    rear  = rtuCOS->rear;
            	if(front != rear)
       	        {
   	       	        rtuCOS->que[rtuCOS->front][0] = rtuPort * 2 + pointState;
       	       	    rtuCOS->que[rtuCOS->front][1] = ptNum;

           	       	rtuCOS->front = (rtuCOS->front + 1) & 0xff;
                    
               	    if(opr->soeDebug)
                	Debug(console,"==> HARRIS-COS(%d): [port=%d/point=%d], state= %2x \n", 1, portNum+1, ptNum+1, pointState);
                }
            }
        }        
        
        /* -------------------------------------------- */
		/*	HOST[1] HARRIS 적용 							*/
		/* -------------------------------------------- */
        portNum = devPoint->dbport2;
        ptNum   = devPoint->dbpoint2;
    
        if((portNum > 0) && (portNum <= MAX_PORT) && (ptNum > 0) && (ptNum <= 64))
        {
        	host = (HOST_DCB *) hostDCB[1];			// Default HOST[0]
    	
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
                        
                    if(opr->soeDebug)
  	                Debug(console,"==> HARRIS-SOE(%d): [port=%d/point=%d], state= %2x \n", 2, portNum+1, ptNum+1, pointState);
                }
                
                /* first COS Buffering... */
   	            rtuCOS  = (COS_Q *) &host->rtubuf[rtuid - 1].cosq;

       	       	front = rtuCOS->front + 1;
           	    rear  = rtuCOS->rear;
            	if(front != rear)
       	        {
   	       	        rtuCOS->que[rtuCOS->front][0] = rtuPort * 2 + pointState;
       	       	    rtuCOS->que[rtuCOS->front][1] = ptNum;

           	       	rtuCOS->front = (rtuCOS->front + 1) & 0xff;
                    
               	    if(opr->soeDebug)
                	Debug(console,"==> HARRIS-COS(%d): [port=%d/point=%d], state= %2x \n", 2, portNum+1, ptNum+1, pointState);
                }
            }
        }
    }
            
    /* ---------------------------------------------------- */
	/*	DEVICE-EVENT : LANDIS-HOST 							*/   
	/* ---------------------------------------------------- */   
	if(host->hostProtocol == HOST_LANDIS)  
    //if(opr->hostLandis)
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
                Debug(console, "==> LANDIS-SOE[front=%d, rear=%d] : point = %3d state = %02x \n", soeQ->front, soeQ->rear, hostpt+1, status);
            }
        }
    }        
        
    return (0);     
}



/*----------------------------------------------------------------------------
* Function Name : deviceComUpdate()
* 수행내용: Device 장치별 통신상태정보 Update 함수
* 대상 계전기 : 디바이스 포인트로 등록된 계전기만 대상으로 함.
---------------------------------------------------------------------------- */
/* 2026.07.07 host 전체 통신상테 관련 수정 by hslee */

int deviceComUpdate()
{
    int     ioid;

 /* 2026.07.07 전체계전기 통신상테->HOST별 처리 */    
    int     anyOffline[MAX_HOST];
    int     hostid, dnpPoint;
    int		hostpt;
    int     devPtIndex;
    int     portNum, ptNum;
        
    DNP_DEVICE  *dnp;
    POINT_BUF   *devPoint;
    HOST_DCB    *host;
    HOST_STSBUF *hostB;
    
    
    /* DB-Update 시... RETURN */
    if(opr->dbUpdate == SET)    return (0);
    
    /* 2026.07.07 전체계전기 통신상테->HOST별 처리 */
    for(hostid=0; hostid < MAX_HOST; hostid++)
    {
    	anyOffline[hostid] = 0;
	}	
	    
    /* ---------------------------------------------- */
    /* DNP HOST Device1 - Device64 의 상태정보 Update */
    /* ---------------------------------------------- */
    for(ioid = 0; ioid < MAX_MODULE; ioid++)
    {
        dnp      = (DNP_DEVICE *) dnpcfg[ioid];         // 대상 계전기 정보 : 0,1,2..
        devPoint = (POINT_BUF *) devPtBuf[ioid+1];      // Device Point 정보 : 개별 계전기, 1,2 ~ 64
        
        /* ---------------------------------------------------- */
        /*  계전기 통신상태 포인트로 등록된 계전기만 대상...    */
        /* ---------------------------------------------------- */
        if(devPoint->config != SET) continue;
            
        if(dnp->scan == 0)  
        {
            devPoint->status = 1;   // [1] : 통신이상, Offline
            continue;
        }
        
        /* DEVICE 통신상태 update... */        
        if(dnp->online == SET)	devPoint->status = 0;   // [0] : 통신정상, Online 
        else           			devPoint->status = 1;   // [1] : 통신이상, Offline
        
        
        /* --------------------------------------------- */
        /* DNP/IEC HOST : 계전기 통신상태 포인트 Update .*/
        /* --------------------------------------------- */
        for(hostid=0; hostid < MAX_HOST; hostid++)
        {
            host = (HOST_DCB *) hostDCB[hostid];
            if(host->hostDualMode == HOST_NOT_USE)	continue;
            if(host->hostProtocol != HOST_DNP)  	continue;	
            
             // 해당 계전에 연계된 모든 host 
            dnpPoint = devPoint->hostIndex[hostid] - 1;             // SDP POINT : 상위 호스트#1 Index 번호 [1..1024]
            
            // 일단 devPtBtuf에  host가 연결된 것만 고려하네.
            if((dnpPoint < 0) || (dnpPoint >= MAX_DNP_DI_POINT))    continue;         

            /* DNP HOST - 0: 정상, 1: 이상 */
            if(devPoint->status == 1)   
            {
                host->sts_pointData[dnpPoint] = PT_FLAG_STS_ON  | PT_FLAG_ONLINE;       // [1] : 통신이상, Offline   0x81
                
                /* 2026.07.07 전체계전기 통신상테->HOST별 처리 */
                anyOffline[hostid]++;
            }
            else  
            {
                host->sts_pointData[dnpPoint] = PT_FLAG_STS_OFF | PT_FLAG_ONLINE;       // [0] : 통신정상, Online     1 
            }
        }

		/* ---------------------------------------- */ 
        /* 2019.01.24 LANDIS-HOST가 정의된 경우... 데이터 처리 */
        /* ---------------------------------------- */ 
		if(opr->hostLandis)
    	{	
        	hostpt  = devPoint->landisIndex;
        	//printf("landis> soe...%d\n", hostpt);
        	if(hostpt > 0)
        	{
            	hostpt = hostpt - 1;
            	hostB = (HOST_STSBUF *) hostStsBuf[hostpt];
            	hostB->status   = devPoint->status;
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
            portNum = devPoint->dbport1;
            ptNum   = devPoint->dbpoint1;
            if((portNum > 0) && (portNum <= MAX_PORT) && (ptNum > 0) && (ptNum <= 64))
            {
                host = (HOST_DCB *) hostDCB[0];
                portNum = devPoint->dbport1 - 1;
                ptNum   = devPoint->dbpoint1 -1;
    
                if(devPoint->status)    host->portdb[portNum].pointData[ptNum] = 1;
                else                    host->portdb[portNum].pointData[ptNum] = 0;
            }
            
            /* ------------------------------------ */ 
            /* HARRIS#2 HOST Data Assign...         */
            /* ------------------------------------ */ 
            portNum = devPoint->dbport2;
            ptNum   = devPoint->dbpoint2;
            
            if((portNum > 0) && (portNum <= MAX_PORT) && (ptNum > 0) && (ptNum <= 64))
            {
              	host = (HOST_DCB *) hostDCB[1];
                portNum = devPoint->dbport2 - 1;
                ptNum   = devPoint->dbpoint2 -1;

                if(devPoint->status)    host->portdb[portNum].pointData[ptNum] = 1;
                else                    host->portdb[portNum].pointData[ptNum] = 0;
            }
                            
        }
        
    }

    /* -------------------------------------------- */
    /* 전체 통신상태 DEVICE-POINT 정보 Update ...   */
    /* -------------------------------------------- */
    devPtIndex = INDEX_ALL_DEVICE;                      // 전체 계전기 통신상태 포인트
    devPoint = (POINT_BUF *) devPtBuf[devPtIndex];      // Device Point 정보   누가 이것을 update 하지 ?

    /* ------------------------------------ */
    /* DNP HOST : 전체통신상태 포인트 Update ...  */
    /* ------------------------------------ */
    for(hostid=0; hostid < MAX_HOST; hostid++)
    {
        host = (HOST_DCB *) hostDCB[hostid];
        if(host->hostDualMode == HOST_NOT_USE)	continue;
        if(host->hostProtocol != HOST_DNP)  	continue;	
        
        dnpPoint = devPoint->hostIndex[hostid] - 1;         // SDP POINT : 상위 호스트#1 Index 번호 [1..1024]
        if((dnpPoint < 0) || (dnpPoint >= MAX_DNP_DI_POINT))    continue;         

#if 0
        /* DNP HOST - 0: 정상, 1: 이상 */
        if(devPoint->status == 1)   
        {
         //   host->sts_pointData[dnpPoint] = PT_FLAG_STS_ON  | PT_FLAG_ONLINE; //80 1
            host->sts_pointData[dnpPoint] = PT_FLAG_STS_OFF | PT_FLAG_ONLINE;   //0 1
        }
        else // online                        
        {
         //   host->sts_pointData[dnpPoint] = PT_FLAG_STS_OFF | PT_FLAG_ONLINE;   //0 1
            host->sts_pointData[dnpPoint] = PT_FLAG_STS_ON  | PT_FLAG_ONLINE; //80 1
        }
#else
        /* DNP HOST - 0: 정상, 1: 이상 */
        if(anyOffline[hostid])   // host에 속한 계전기 중 하나가 offline 
        {
            host->sts_pointData[dnpPoint] = PT_FLAG_STS_ON  | PT_FLAG_ONLINE; //80 +  1
         //   host->sts_pointData[dnpPoint] = PT_FLAG_STS_OFF | PT_FLAG_ONLINE;   //0 1
        }
        else // online                        
        {
              host->sts_pointData[dnpPoint] = PT_FLAG_STS_OFF | PT_FLAG_ONLINE;   //0 + 1
         //   host->sts_pointData[dnpPoint] = PT_FLAG_STS_ON  | PT_FLAG_ONLINE; //80 1
        }


#endif      
        
        // opr->allDevOnline[hostid]  이건 기존 것으로 event 만들기 위해서..
        
        /* ---------------------------------- */
		/* 2026.07.07 전체계전기 통신상테->HOST별 처리 */
		/* ---------------------------------- */
    	if(anyOffline[hostid]) // offline
    	{      
	        if(opr->allDevOnline[hostid] == SET)
    	    {
        	    if(opr->soeDebug)
            	Debug(console,">> HOST(%d) SOE : ALL Device Offline ... SET  offline count =%d\n", hostid+1, anyOffline[hostid]);
            
	            /* DEVICE-SOE 생성 */
    	        Create_ALL_Device_SOE( hostid, devPtIndex, 0x01);   // 통신이상
        	}

	        /* ---------------------------- */
    	    /* 전체계전기 통신상태 => 이상  */   
        	/* ---------------------------- */                 
	        devPoint->status = 1;                       // 0 : 정상, 1: 이상
    	    opr->allDevOnline[hostid] = RESET;
	    }
    	else
	    {
    	    if(opr->allDevOnline[hostid] == RESET)
        	{
	            if(opr->soeDebug)
    	        Debug(console,">> HOST(%d) SOE : ALL Device Offline ... RESET  offline count =%d\n", anyOffline[hostid]);
            
        	    /* DEVICE-SOE 생성 */
            	Create_ALL_Device_SOE(hostid, devPtIndex, 0x00);   // 통신정상
        	}
        
        	/* ---------------------------- */      
	        /* 전체계전기 통신상태 => 정상  */ 
    	    /* ---------------------------- */                   
        	devPoint->status = 0;                       // 0 : 정상, 1: 이상
        	opr->allDevOnline[hostid] = SET;
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
        portNum = devPoint->dbport1;
        ptNum   = devPoint->dbpoint1;
        if((portNum > 0) && (portNum <= MAX_PORT) && (ptNum > 0) && (ptNum <= 64))
        {
            host = (HOST_DCB *) hostDCB[0];
            portNum = devPoint->dbport1 - 1;
            ptNum   = devPoint->dbpoint1 -1;
    
            if(devPoint->status)    host->portdb[portNum].pointData[ptNum] = 1;
            else                    host->portdb[portNum].pointData[ptNum] = 0;
        }
        
        /* ------------------------------------ */ 
        /* HARRIS#2 HOST Data Assign...         */
        /* ------------------------------------ */ 
        portNum = devPoint->dbport2;
        ptNum   = devPoint->dbpoint2;
        
        if((portNum > 0) && (portNum <= MAX_PORT) && (ptNum > 0) && (ptNum <= 64))
        {
          	host = (HOST_DCB *) hostDCB[1];
            portNum = devPoint->dbport2 - 1;
            ptNum   = devPoint->dbpoint2 -1;

            if(devPoint->status)    host->portdb[portNum].pointData[ptNum] = 1;
            else                    host->portdb[portNum].pointData[ptNum] = 0;
        }
                        
    }
    
    /* ---------------------------------------- */ 
    /* 2019.01.24 LANDIS-HOST가 정의된 경우... 데이터 처리 */
    /* ---------------------------------------- */ 
	if(opr->hostLandis)
    {	
       	hostpt  = devPoint->landisIndex;
       	//printf("landis> soe...%d\n", hostpt);
       	if(hostpt > 0)
       	{
           	hostpt = hostpt - 1;
           	hostB = (HOST_STSBUF *) hostStsBuf[hostpt];
           	hostB->status   = devPoint->status;
		}
	}		
		        
    return (0);
}


/*
* =================================================
*  시스템 디바이스 포인트 생성
* =================================================
*/
int check_Device_SOE()
{
    int 	index;
    int 	hostid, dnpPoint;
    int     portNum, ptNum;
    int		hostpt;
    
    HOST_DCB    *host;
    POINT_BUF   *devPoint;
    HOST_STSBUF *hostB;
    
    /* ---------------------------------------------------------------- */
    /*  자동/수동(65) ~ 61850-B 동작(88) 까지의 SOE 정보 생성           */
    /* ---------------------------------------------------------------- */
    for(index = INDEX_AUTO_MANUAL; index <= INDEX_CPU_B_STS; index++)
    {
        devPoint = (POINT_BUF *)   devPtBuf[index];     // SDP 포인트 Config 정보   
        if(devPoint->config != SET) continue;

        /* ------------------------------------ */
        /* DNP HOST : Device 포인트 Update ...  */
        /* ------------------------------------ */
        for(hostid=0; hostid < MAX_HOST; hostid++)
        {
            host = (HOST_DCB *) hostDCB[hostid];
            if(host->hostDualMode == HOST_NOT_USE)	continue;
            if(host->hostProtocol != HOST_DNP)  continue;	
        
            dnpPoint = devPoint->hostIndex[hostid] - 1;         // SDP POINT : 상위 호스트#1 Index 번호 [1..1024]
            if((dnpPoint < 0) || (dnpPoint >= MAX_DNP_DI_POINT))    continue;         

            /* DNP HOST - 0: 이상, 1: 정상 */
            if(devPoint->status == 1)   host->sts_pointData[dnpPoint] = PT_FLAG_STS_ON  | PT_FLAG_ONLINE;
            else                        host->sts_pointData[dnpPoint] = PT_FLAG_STS_OFF | PT_FLAG_ONLINE;        
        }

		/* ---------------------------------------- */ 
	    /* HARRIS-HOST가 정의된 경우... 데이터 처리 */
    	/* ---------------------------------------- */ 
	    if(opr->hostHarris == SET)
    	{ 
	        /* ------------------------------------ */ 
    	    /* HARRIS#1 HOST Data Assign...         */
        	/* ------------------------------------ */ 
	        portNum = devPoint->dbport1;
    	    ptNum   = devPoint->dbpoint1;
        	if((portNum > 0) && (portNum <= MAX_PORT) && (ptNum > 0) && (ptNum <= 64))
	        {
    	        host = (HOST_DCB *) hostDCB[0];
        	    portNum = devPoint->dbport1 - 1;
            	ptNum   = devPoint->dbpoint1 -1;
    
	            if(devPoint->status)    host->portdb[portNum].pointData[ptNum] = 1;
    	        else                    host->portdb[portNum].pointData[ptNum] = 0;
        	}
        
        	/* ------------------------------------ */ 
	        /* HARRIS#2 HOST Data Assign...         */
    	    /* ------------------------------------ */ 
	        portNum = devPoint->dbport2;
    	    ptNum   = devPoint->dbpoint2;
        
	        if((portNum > 0) && (portNum <= MAX_PORT) && (ptNum > 0) && (ptNum <= 64))
    	    {
        	  	host = (HOST_DCB *) hostDCB[1];
	            portNum = devPoint->dbport2 - 1;
    	        ptNum   = devPoint->dbpoint2 -1;

        	    if(devPoint->status)    host->portdb[portNum].pointData[ptNum] = 1;
            	else                    host->portdb[portNum].pointData[ptNum] = 0;
        	}
     	}                   

		/* ---------------------------------------- */ 
	    /* 2019.01.24 LANDIS-HOST가 정의된 경우... 데이터 처리 */
    	/* ---------------------------------------- */ 
		if(opr->hostLandis)
    	{	
	       	hostpt  = devPoint->landisIndex;
       		//printf("landis> soe...%d\n", hostpt);
    	   	if(hostpt > 0)
	       	{
    	       	hostpt = hostpt - 1;
        	   	hostB = (HOST_STSBUF *) hostStsBuf[hostpt];
           		hostB->status   = devPoint->status;
			}
		}		
	    
		//printf("(test) index=%d ... old=%d, cur=%d\n", index, devPoint->oldsts, devPoint->status);
		
        /* -------------------------------------------- */                   
        /* DEVICE 포인트 상태변화 Check...              */
        /* -------------------------------------------- */  
        if(devPoint->oldsts != devPoint->status)
        {
            /* DEVICE-SOE 생성 */
            Create_Device_SOE(index, devPoint->status);   
        }
        
        devPoint->oldsts = devPoint->status;                  
        
        
    }
 
    return (0);
}


/*
* Device Point : 수동/자동 상태, 0:자동, 1:수동
*/
void    check_AutoManual()
{
    int     devPtIndex;
    POINT_BUF   *devPoint;
        
    devPtIndex = INDEX_AUTO_MANUAL;                     // Device Point : 수동/자동 상태, 0:자동, 1:수동
    devPoint = (POINT_BUF *) devPtBuf[devPtIndex];      // Device Point 정보
    
    if(devPoint->config == 0)   return;
    
    if(scuCfg->online == SET)  
    {
        if(scuCfg->remoteMode == AUTO_MODE) 
        {
            /* 수동모드(1) => 자동모드(0) 변화 Check */
            devPoint->status = 0;       // 자동모드 : 0
        }
        else
        {
            /* 자동모드(0) => 수동모드(1) 변화 Check */
            devPoint->status = 1;       // 수동모드 : 0
        } 

    }
    
    //printf("(test) => AUTO/Manual.... index=%d, status=%d\n", devPtIndex, devPoint->status);
    
}

/*
* Device Point-66 : CPU 동작모드, 0:SDP-A 동작, 1:SDP-B 동작 
*/
void    check_cpu_runMode()
{
    int     devPtIndex;
    
    POINT_BUF   *devPoint;
        
    devPtIndex = INDEX_CPURUN_STS;                      // Device Point-66 : CPU 동작모드, 0:SDP-A 동작, 1:SDP-B 동작  
    devPoint = (POINT_BUF *) devPtBuf[devPtIndex];      // Device Point 정보
    
    if(devPoint->config == 0)   return;
    
    /* ---------------------------------------- */
    /* 현재 Active 한 SDP 장치 지정...          */
    /* ---------------------------------------- */
    if(opr->cpuMode == CPU_MASTER)          devPoint->status = 0;   // SDP-A 동작 : 0
    else if(opr->cpuMode == CPU_SLAVE)      devPoint->status = 1;   // SDP-B 동작 : 1 
    else    return;
                   
}        


/*
* Device Point-67 : SDP-A 동작상태, 0:정상, 1:이상  
* Device Point-68 : SDP-B 동작상태, 0:정상, 1:이상  
*/
void    check_cpu_runState()
{
    int     devPtIndex;
    
    POINT_BUF   *devPoint;

    if(opr->dualCpuSts == SET)  // CPU 이중화 여부 결정...
    {
        if(opr->cpuMode == CPU_MASTER)
        {
            /* ---------------------------- */
            /* SDP-A 정상 Check             */
            /* ---------------------------- */
            devPtIndex = INDEX_CPU_A_STS;                       // Device Point-67 : SDP-A 동작상태, 0:정상, 1:이상 
            devPoint = (POINT_BUF *) devPtBuf[devPtIndex];      // Device Point 정보

            devPoint->status = 0;   // SDP-A 정상 : 0    
            
            /* ---------------------------- */
            /* SDP-B 정상 Check             */
            /* ---------------------------- */
            devPtIndex = INDEX_CPU_B_STS;                       // Device Point-68 : SDP-B 동작상태, 0:정상, 1:이상  
            devPoint = (POINT_BUF *) devPtBuf[devPtIndex];      // Device Point 정보
            
            if(linkCfg->offline == SET)        devPoint->status = 1;   // SDP-B 이상 : 1 , LINK 실패시
            else                               devPoint->status = 0;   // SDP-B 정상 : 0    
        }
        else
        {
            /* ---------------------------- */
            /* SDP-B 정상 Check             */
            /* ---------------------------- */
            devPtIndex = INDEX_CPU_B_STS;                       // Device Point-68 : SDP-B 동작상태, 0:정상, 1:이상  
            devPoint = (POINT_BUF *) devPtBuf[devPtIndex];      // Device Point 정보

            devPoint->status = 0;   // SDP-B 정상 : 0    
            
            /* ---------------------------- */
            /* SDP-A 정상 Check             */
            /* ---------------------------- */
            devPtIndex = INDEX_CPU_A_STS;                       // Device Point-67 : SDP-A 동작상태, 0:정상, 1:이상 
            devPoint = (POINT_BUF *) devPtBuf[devPtIndex];      // Device Point 정보
            
            if(linkCfg->offline == SET)        devPoint->status = 1;   // SDP-A 이상 : 1 , LINK 실패시
            else                               devPoint->status = 0;   // SDP-A 정상 : 0    
        }
    }
    else
    {
        if(opr->cpuMode == CPU_MASTER)
        {
            /* ---------------------------- */
            /* SDP-A 정상 Check             */
            /* ---------------------------- */
            devPtIndex = INDEX_CPU_A_STS;                       // Device Point-67 : SDP-A 동작상태, 0:정상, 1:이상 
            devPoint = (POINT_BUF *) devPtBuf[devPtIndex];      // Device Point 정보

            devPoint->status = 0;   // SDP-A 정상 : 0    
            
            /* ---------------------------- */
            /* SDP-B 정상 Check             */
            /* ---------------------------- */
            devPtIndex = INDEX_CPU_B_STS;                       // Device Point-68 : SDP-B 동작상태, 0:정상, 1:이상  
            devPoint = (POINT_BUF *) devPtBuf[devPtIndex];      // Device Point 정보
            
            devPoint->status = 1;   // SDP-B 이상 : 1 , LINK 실패시
        }
        else 
        {
            /* ---------------------------- */
            /* SDP-B 정상 Check             */
            /* ---------------------------- */
            devPtIndex = INDEX_CPU_B_STS;                       // Device Point-68 : SDP-B 동작상태, 0:정상, 1:이상  
            devPoint = (POINT_BUF *) devPtBuf[devPtIndex];      // Device Point 정보

            devPoint->status = 0;   // SDP-B 정상 : 0    
            
            /* ---------------------------- */
            /* SDP-A 정상 Check             */
            /* ---------------------------- */
            devPtIndex = INDEX_CPU_A_STS;                       // Device Point-67 : SDP-A 동작상태, 0:정상, 1:이상 
            devPoint = (POINT_BUF *) devPtBuf[devPtIndex];      // Device Point 정보
            
            devPoint->status = 1;   // SDP-A 이상 : 1 , LINK 실패시
        }
    }
}        


/*----------------------------------------------------------------------------
* Function Name : systemStsUpdate()
* 수행내용: 이중화시스템 관련 상태정보 Update
* ArgList :
* Return  :
---------------------------------------------------------------------------- */
void systemStsUpdate()
{
    check_AutoManual();         // Device Point-65 : 수동/자동 상태, 0:자동, 1:수동
    check_cpu_runMode();        // Device Point-66 : CPU 동작모드, 0:SDP-A 동작, 1:SDP-B 동작 
    check_cpu_runState();       // Device Point-72, 73 : SDP-A/B 동작모드, 0:정상, 1:이상 
    
    /* ---------------------------------------- */
    /*  DEVICE 포인트 SOE 정보 추출 ...         */
    /* ---------------------------------------- */
    check_Device_SOE();
    
#if 0    
    check_AutoManual();         // Device Point-65 : 수동/자동 상태, 0:자동, 1:수동
    check_sdp_status();         // Device Point-70 : SDP 동작상태, 0:정상, 1:이상  
    check_sdp_runMode();        // Device Point-71 : SDP 동작모드, 0:SDP-A 동작, 1:SDP-B 동작 
    
    check_sdp_runState();       // Device Point-72, 73 : SDP-A/B 동작모드, 0:정상, 1:이상 
    
    check_rtu_status();         // Device Point-74,75,76 : RTU 동작모드, 0:SDP-A 동작, 1: SDP-B 동작     
    check_scada_status();       // Device Point-77,78,79 : 전철제어반 동작모드, 0:SDP-A 동작, 1: SDP-B 동작
    check_remote_status();      // Device Point-80,81,82 : 원격진단 동작모드, 0:SDP-A 동작, 1: SDP-B 동작   
    
    check_elecq_status();       // Device Point-83,84,85 : 전력품질 동작모드, 0:SDP-A 동작, 1: SDP-B 동작  
    check_61850_status();       // Device Point-86,87,88 : 61850 동작모드, 0:SDP-A 동작, 1: SDP-B 동작       
#endif
    
}
        
         
 
                                                    
/* ............. end of " chgmap.c " ...................................... */
