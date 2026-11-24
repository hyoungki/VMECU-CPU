/*
 *  VLANE SIO : Channel TASK
*/

#include	"localLib.h"
#include    "external.h"

extern  int     dnpChanRead(int fid, int devid, byte *rxbuf, int reqCount);
extern  int     dnpChanWrite(int fid, int devid, byte *sndbuf, int sndCount);
extern  int     readDataLink_TCP( int scanid, int devid, DNP_DATA_LINK *dnpData);
extern  int     sendConfirm( int mode, int scanid, int devid, int code);

extern  int     storeAI_DNP(int devid, int point, float fdata, byte *lowdata);
extern  int     storeAI_FLOAT_DNP(int devid, int point, float fdata, byte *lowdata);

extern  int     dnpFlagClear( int devid);
extern  int     storeCOUNT_DNP(int devid, int point, int adata, byte *lowdata);
extern  int     storeAO_DNP(int devid, int point, int adata);
extern  int     curStore_BIT_DNP( int devid, byte *stsbuf, int dataSize);
extern  int     storeSOE_CU_DNP( int devid, int devPoint, int state, byte *buf);
extern  int     storeCOS_CU_DNP( int devid, int point, int state);


extern  int             termExec;
extern  TASK_INFO	    *taskPtr;

extern  PDPDTIME_DATE_TIME      dnpRTC;
extern  PDPDTIME_MS_SINCE_70    dnpRTCInfo;

extern	char 	modbusFormat[8][10];
extern  char 	modbusData[4][10];

static	byte	bitcode[8] = {0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80};

/*
 *  scan DI-POINT
 */
int check_SOE_modbus(int devid, byte *newsts, byte *oldsts, int count)
{
    int     i, j;
    byte    state,cosbuf;
    int     point, maxPoint;
    int     millisecond;
    int     debounce;
    byte    cursts, coschk;
    byte    soeBuffer[32];
    
    struct timeval rv; 
    time_t  tm;
    struct  tm  localtm;
    
    DNP_DEVICE  *dnp;
    POINT_BUF   *ptbuf;

    dnp = (DNP_DEVICE *) dnpcfg[devid];

    /* ---------------------------------------------------- */
    /* DIM 상태정보 ...                                     */
    /* ---------------------------------------------------- */
    for(i=0; i< count; i++)
    {
        cursts = newsts[i];
        coschk = cursts ^ dnp->df[i];
        dnp->df[i]  = newsts[i];
    
        /* cos check & cos time set */
        if( coschk != 0)
        {
            for(j=0; j< 8; j++)
            {
                point = i*8 + j;
                ptbuf = (POINT_BUF *) &dnp->diPtBuf[point];
            
                if(coschk & 0x01)
                {
                    /* cos point set & reset */
                    if(ptbuf->cos == 0)
                    {
                        if(cursts & 0x01)   ptbuf->cos = 0x80;
                        else                ptbuf->cos = 0x40;
                        //ptbuf->cosTime = costick;
                        ptbuf->costick = 0;
                    }
                    else    ptbuf->cos = 0;
                }/* if */
                coschk = coschk >> 1;
                cursts = cursts >> 1;
            }/* for */
        }
    }/* if cos != 0 */

    maxPoint = count * 8;
    for(point = 0; point < maxPoint; point++)
    {
        ptbuf = (POINT_BUF *) &dnp->diPtBuf[point];
        
        if(ptbuf->cos)
        {
            debounce = ++ptbuf->costick;
            //if( debounce >= 2)
            if( debounce >= opr->soeDebounce)
            {
                cosbuf = point | (ptbuf->cos & 0x80);
                //state  = ptbuf->cos & PT_FLAG_STS_ON;
                state  = (ptbuf->cos >> 7) & 0x01;

                /* ---------------------------------- */
                /* 현재시간 정보를 DNP Form 으로 변환 */
                /* ---------------------------------- */
                time(&tm);
                //localtm = localtime(&tm);
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
    
                soeBuffer[0] = point;           
                soeBuffer[1] = state;
                
                soeBuffer[2] = (dnpRTCInfo.leastSignificant) & 0xff;
                soeBuffer[3] = (dnpRTCInfo.leastSignificant >> 8) & 0xff;
                soeBuffer[4] = (dnpRTCInfo.mostSignificant) & 0xff;
                soeBuffer[5] = (dnpRTCInfo.mostSignificant >> 8)  & 0xff;
                soeBuffer[6] = (dnpRTCInfo.mostSignificant >> 16) & 0xff;
                soeBuffer[7] = (dnpRTCInfo.mostSignificant >> 24) & 0xff;
                
                if(opr->soeDebug)
                Debug(console,"dev%2d> MOD-SOE: pt=%2d, sts=%2x, (debounce=%2d)\n", devid+1, point+1, state, debounce);
                
                /* -------------------------------- */
                /*  SOE 정보 저장 (for MODBUS)      */
                /* -------------------------------- */
                storeSOE_CU_DNP(devid, point, state, &soeBuffer[2]); 

#if 0
                /* -------------------------------- */
                /*  COS 정보 저장 (for MODBUS)      */
                /* -------------------------------- */
                storeCOS_CU_DNP( devid, point, state);
#endif

                /* COS-Tag Clear... */
                ptbuf->cos = 0;
                ptbuf->costick = 0;

            }                
        }/* if cos */
        
    }/* for */
    return (0);
} 


/*
*   GIPAM-115 TYPE
*   - 30001        : DI Status
*   - 30002        : DO Status
*   - 30003, 30004 : FAULT Status
*/
int checkInfoMODBUS(int devid, int stsCount, byte *rxbuf, int dataLen)
{
    int 	i,j;
    int 	count=0;
    int 	index1, index2, inx;
    //int 	opcode;
    int 	point;
    float   *fptr1;
    float   fdata, aiScale;
    byte    data[8];
    
    DNP_DEVICE  *dnp;
    POINT_BUF       *aiPoint;

    dnp = (DNP_DEVICE *) dnpcfg[devid];
    
    if(opr->msgDebug == devid)
    Debug(console,"dev%2d> rcv sts point = %d, rcv Data Count = %d\n", devid+1, stsCount, dataLen);   

    /* ---------------------------------------- */
    /*  상태포인트 영역                         */
    /* ---------------------------------------- */
    for(i=0; i< stsCount; i++)
    {
        dnp->cursts[i*2 + 0] = rxbuf[i*2 + 1];      // LOW      DI status
        dnp->cursts[i*2 + 1] = rxbuf[i*2 + 0];      // HIGH     DI status        
    }
    
    /* ---------------------------------------- */
    /* SOE Check 및 상태정보 저장 ... 8 바이트  */
    /* ---------------------------------------- */                
    if(stsCount)
    {
        check_SOE_modbus( devid, &dnp->cursts[0], &dnp->df[0], stsCount*2);
        curStore_BIT_DNP( devid, &dnp->cursts[0], stsCount*2);
        
        if(opr->msgDebug == devid)  DumpBuff(console,"STS1: ", dnp->cursts, stsCount *2);
    }
    
    dataLen -= (stsCount * 2);      // 상태포인트 이후 ... 아날로그 
    
    if(dataLen <= 0)    return (0);

    index1 = dataLen / 4;
    index2 = dataLen % 4;

    /* -------------------------------- */
    /* 아날로그 포인트 영역             */
    /* -------------------------------- */
    count   = stsCount *2;
    inx     = 0;
    point   = 0;
    for(i=0; i < index1; i++)
    {
        /* float  to int    32.1 -> 321 */
        fptr1 = (float *) &rxbuf[count];
        fdata = (float ) *fptr1;
        
#if 1            
        /* ---------------------------------------- */
        /*  계전기별 포인트 지정 => 스케일링...     */
        /* ---------------------------------------- */
        aiPoint = (POINT_BUF *) &dnp->aiPtBuf[point];
        if(aiPoint->ptConfig)
        {   
            aiScale = 1.0;
            for(j=0; j < aiPoint->ptConfig; j++) aiScale = (aiScale * 0.1);
            fdata = (aiScale * fdata);
        }
            
        /* 변환된 Low 데이터 */            
        memcpy( data, &fdata, 4);
#endif
        
        if(opr->msgDebug == devid)
        {
            Debug(console,"%4.2f ", fdata);
            if((++inx % 10) == 0)  Debug(console,"\n");
        }
        
        /* Analog Data Assign into Buffer ... */
        storeAI_FLOAT_DNP( devid, point++, fdata, data);
        count += 4;
    }
    
    if(opr->msgDebug == devid)   Debug(console,"\n");   
    return (0);
}

/*
*	MODBUS BLOCK 처리 함수
*/
int	checkDevicePoint(MODBUS_READ *read, int devid, int offset)
{
	int	i;
	int	devPoint, refPoint;
	
	DNP_DEVICE  *dnp;
	POINT_BUF	*aiPt;
	POINT_BUF	*diPt;
	
	dnp = (DNP_DEVICE *) dnpcfg[devid];
	
	//if(opr->msgDebug == devid) 	    Debug(console,"check%2d> offset=%d... devPoint=%d\n", devid+1, offset, dnp->devAiPoint);
	
	/* ---------------------------------------- */
	/*	READ-BLOCK 데이터 타입별 정보 검색 		*/
	/* ---------------------------------------- */
	if(read->dataType == MODBUS_TYPE_AI)
	{	
	    refPoint = read->baseAddr + read->baseOffset + offset;		// 수신데이터 포인트 정보
	    
		for(i=0; i < dnp->devAiPoint; i++)
		{
			aiPt = (POINT_BUF *) &dnp->aiPtBuf[i];
			if(aiPt->config == RESET)	continue; // 사용안함 
		
			devPoint = aiPt->modBase + aiPt->modIndex;					// 계전기내 포인트 정보
		
		    //if(opr->msgDebug == devid) 	    
		    //Debug(console,"check%2d> AI offset=%d... devPoint=%d / refPt=%d\n", devid+1, offset, devPoint, refPoint);
			
			if(refPoint == devPoint)	
			{
				if(opr->modDebug == devid)
				{
					Debug(console,"\n  check-AI> dev=%2d/Point=%2d, offset=%4d, [ref=%3d, dev=%3d] ", aiPt->devNo, aiPt->devPt, offset, refPoint, devPoint);
				}
					
				return (i);	
			}
		}
	}
	else if(read->dataType == MODBUS_TYPE_DI)
	{	
	    refPoint = read->baseAddr + read->baseOffset + offset;		// 수신데이터 포인트 정보
	    
		for(i=0; i < dnp->devDiPoint; i++)
		{
			diPt = (POINT_BUF *) &dnp->diPtBuf[i];
			if(diPt->config == RESET)	continue;
		
			devPoint = diPt->modBase + diPt->modIndex;					// 계전기내 포인트 정보
		
		    //if(opr->msgDebug == devid) 	    
		    //Debug(console,"check%2d> DI offset=%d... devPoint=%d / refPt=%d\n", devid+1, offset, devPoint, refPoint);
			if(refPoint == devPoint)	
			{
				if(opr->modDebug == devid)
				{
					Debug(console,"  check-DI> dev=%2d/Point=%2d, offset=%4d, [ref=%3d, dev=%3d]\n", diPt->devNo, diPt->devPt, offset, refPoint, devPoint);
				}
				
				return (i);	
			}
		}
	}
	
	return (-1);
}


/*
*	rcv Message 처리 : READ_BLOCK
*/
int checkBlockMODBUS(int scanid, int devid,  int opcode, byte *rxbuf, int dataLen)
{
    int     i,j;
    int     count=0;
    int	    readIndex;
    int     index1, index2, inx=0;
    int     stsCount;
    int     adata;
    
    short int sadata ;
    unsigned int uadata  ; // 2026-07-14 오후 9:46:44
    int     point, offset, wordOffset;
    float   *fptr1;
    float   fdata, aiScale;
    byte	rcvSts[256];        // 포인트 증설(1024) 에 따른 버퍼용량 증대
    byte    data[8], tempAI1[8];
    
    DNP_DEVICE  *dnp;
	MODBUS_READ	*readBlock;
	POINT_BUF       *aiPoint;
	//POINT_BUF       *diPoint;
    
    dnp = (DNP_DEVICE *) dnpcfg[devid];
    
            
    /* 현재 수신된 데이터의 readBlock 찾음 ... */
    readIndex = dnp->readIndex & 0x07;  // 아마 보낼 때 write 했겠지..
    readBlock = (MODBUS_READ *) &dnp->readBlock[readIndex];

    if((opr->dnpDebug == devid) || (opr->scanDebug == scanid))  
        Debug(console,"rcv%2d> modbus Read(%d) [opcode=%2d, Base/Offset= %5d/%5d, point=%4d] Type/fmt = %s/%s ... rcvData=%d \n", devid+1, dnp->readIndex,
            opcode, readBlock->baseAddr, readBlock->baseOffset, readBlock->dataNum, modbusData[readBlock->dataType], modbusFormat[readBlock->dataFormat], dataLen);

    if(opr->modDebug == devid)
    Debug(console,"\n >> --------------------------------------------------------------------------\n ");
                       
	/* -------------------------------------------- */
	/*	READ BLOCK 데이터 타입별 정보 처리			*/
	/* -------------------------------------------- */
	if(readBlock->dataType == MODBUS_TYPE_DI)
	{
	    /* 상태포인트 수만큼 상태포인트 처리...*/
        stsCount = dataLen / 2;
        
        /* 수신데이터가 홀수 Byte인 경우... */
        index2   = dataLen % 2;
        if(index2)
        {    
            stsCount = (dataLen / 2) + 1;
            bzero8248(rcvSts, stsCount*2);
        }
        
            
	    if((readBlock->opcode == MODBUS_RD_INPUT_BIT) || (readBlock->opcode == MODBUS_RD_INPUT_DIS))
	    {
	        /* BIT 타입을 BYTE 타입으로 변경... */
            for(i=0; i< dataLen; i++)
        	{
            	rcvSts[i] = rxbuf[i];      // LOW      DI status
    	    }
	    }
	    else
	    {    
            /* WORD 타입을 BYTE 타입으로 변경... */
            for(i=0; i< stsCount; i++)
        	{
            	rcvSts[i*2 + 0] = rxbuf[i*2 + 1];      // LOW      DI status
        	    rcvSts[i*2 + 1] = rxbuf[i*2 + 0];      // HIGH     DI status 
    	    }
    	}
    	
    	if(opr->msgDebug == devid)  
    	{
    	    Debug(console,"------------------------------------------------\n");
    	    DumpBuff(console,"RCV : ", &rcvSts[0], dataLen);
    	}

        /* ---------------------------------------- */
    	/*	포인트 상태(ON)정보에 따른 재정렬...	*/
    	/* ---------------------------------------- */
    	bzero8248(dnp->cursts, dataLen);
    	
    	for(i=0, offset=0; i < dataLen; i++)
    	{
    		for(j =0; j < 8; j++, offset++)
    		{
    			/* 수신데이터... 1 의 경우 ... */
    			if(rcvSts[i] & bitcode[j])	
    			{	
	    			/* ------------------------------------ */
    		   		/* 	계전기내 포인트 Check... 			*/
        			/* ------------------------------------ */
        			point = checkDevicePoint(readBlock, devid, offset);
        			
        			if(point >= 0)
        			{	
        				index1 = point / 8;
        				index2 = point % 8;
        		
        				/* 데이터베이스 정의된 포인트 상태 Update... */
        				dnp->cursts[index1] |= bitcode[index2];	
        			}	
        		}		
        	}
        }
        
    	/* ---------------------------------------- */
    	/* SOE Check 및 상태정보 저장 ... 8 바이트  */
    	/* ---------------------------------------- */                
    	if(stsCount)
    	{
        	check_SOE_modbus( devid, &dnp->cursts[0], &dnp->df[0], stsCount*2);
        	curStore_BIT_DNP( devid, &dnp->cursts[0], stsCount*2);
        
        	if(opr->msgDebug == devid)  	DumpBuff(console,"STS2: ", dnp->cursts, stsCount *2);
    	}

	}
	else if(readBlock->dataType == MODBUS_TYPE_AI)
	{
	        
//		if((readBlock->dataFormat == FORMAT_SHORT) || (readBlock->dataFormat == FORMAT_WORD))
		if((readBlock->dataFormat == FORMAT_SHORT) ) // 데이터포캣 INT16
		{
			index1 = dataLen / 2;			// 수신 바이트 수   FORMAT_WORD = unsinged short (16bit)
			count  = 0;
			wordOffset = 0;
		    
		    
			
			for(i=0; i < index1; i++)
    		{
				sadata =   rxbuf[count + 1] + ((rxbuf[count+ 0] << 8) & 0xff00);
        		fdata = (float ) sadata;

				/* ------------------------------------ */
    	    	/* 	계전기내 포인트 Check... 			*/
        		/* ------------------------------------ */
        		point = checkDevicePoint(readBlock, devid, wordOffset);

        		if(point >= 0)
        		{	
        		    /* ---------------------------------------- */
                    /*  계전기별 포인트 지정 => 스케일링...     */
                    /* ---------------------------------------- */
                    aiPoint = (POINT_BUF *) &dnp->aiPtBuf[point];
                    if(aiPoint->ptConfig)
                    {   
                        aiScale = 1.0;
                        for(j=0; j < aiPoint->ptConfig; j++) aiScale = (aiScale * 0.1);
                     //   fdata = (aiScale * sadata);
                        fdata = (aiScale * fdata);
                    }
        	
        	        //fdata = (float ) adata;
        	        
        			if(opr->msgDebug == devid)
        			{
            			Debug(console,"(1)%4.2f [%04X] ", fdata, sadata);
            			if((++inx % 8) == 0)  Debug(console,"\n");
        			}
        			        			
        			/* ------------------------------------ */
        			/* Analog Data Assign into Buffer ...   */
        			/* ------------------------------------ */
        			storeAI_DNP( devid, point, fdata, (byte *)&fdata);
        		}
        		
        		count += 2;		
        		wordOffset++;        		
        	}	
		}
		else if(  (readBlock->dataFormat == FORMAT_WORD))
		{
			index1 = dataLen / 2;			// 수신 바이트 수   FORMAT_WORD = unsinged short (16bit)
			count  = 0;
			wordOffset = 0;
			
			for(i=0; i < index1; i++)
    		{
				uadata =   rxbuf[count + 1] + ((rxbuf[count+ 0] << 8) & 0xff00);
        		fdata = (float ) uadata;

				/* ------------------------------------ */
    	    	/* 	계전기내 포인트 Check... 			*/
        		/* ------------------------------------ */
        		point = checkDevicePoint(readBlock, devid, wordOffset);

        		if(point >= 0)
        		{	
        		    /* ---------------------------------------- */
                    /*  계전기별 포인트 지정 => 스케일링...     */
                    /* ---------------------------------------- */
                    aiPoint = (POINT_BUF *) &dnp->aiPtBuf[point];
                    if(aiPoint->ptConfig)
                    {   
                        aiScale = 1.0;
                        for(j=0; j < aiPoint->ptConfig; j++) aiScale = (aiScale * 0.1);
                        // fdata = (aiScale * uadata);
                        fdata = (aiScale * fdata);
                    }
        	
        	        //fdata = (float ) adata;
        	        
        			if(opr->msgDebug == devid)
        			{
            			Debug(console,"(2)%4.2f [%04X]", fdata, uadata);
            			if((++inx % 8) == 0)  Debug(console,"\n");
        			}
        			        			
        			/* ------------------------------------ */
        			/* Analog Data Assign into Buffer ...   */
        			/* ------------------------------------ */
        			storeAI_DNP( devid, point, fdata, (byte *)&fdata);
        		}
        		
        		count += 2;		
        		wordOffset++;        		
        	}	
		}
//		else if((readBlock->dataFormat == FORMAT_INT32) || (readBlock->dataFormat == FORMAT_UINT32))
		else if (readBlock->dataFormat == FORMAT_INT32)  		
		{
			index1 = dataLen / 4;			// 수신 바이트 수
			count  = 0;
			wordOffset = 0;
			
			for(i=0; i < index1; i++)
    		{
    		    /* ACE 의 경우 */
				adata =  rxbuf[count + 0] + ((rxbuf[count + 1] << 8) & 0xff00) +
						((rxbuf[count+ 2] << 16) & 0xff0000) + ((rxbuf[count+ 3] << 24) & 0xff000000);
						
        		fdata = (float ) adata;

				/* ------------------------------------ */
    	    	/* 	계전기내 포인트 Check... 			*/
        		/* ------------------------------------ */
        		point = checkDevicePoint(readBlock, devid, wordOffset);

        		if(point >= 0)
        		{		
        			if(opr->msgDebug == devid)
        			{
            			Debug(console,"(3)%4.2f ", fdata);
            			if((++inx % 8) == 0)  Debug(console,"\n");
        			}

                    /* ---------------------------------------- */
                    /*  계전기별 포인트 지정 => 스케일링...     */
                    /* ---------------------------------------- */
                    aiPoint = (POINT_BUF *) &dnp->aiPtBuf[point];
                    if(aiPoint->ptConfig)
                    {   
                        aiScale = 1.0;
                        for(j=0; j < aiPoint->ptConfig; j++) aiScale = (aiScale * 0.1);
//                        fdata = (aiScale * adata);
                        fdata = (aiScale * fdata);
                    }
                    
                    //fdata = (float ) adata;
                            			        			
        			/* ------------------------------------ */
        			/* Analog Data Assign into Buffer ...   */
        			/* ------------------------------------ */
        			storeAI_DNP( devid, point, fdata, (byte *) &fdata);
        		}
        		
        		count += 4;		
        		wordOffset += 2;        		
        	}				
		}
		else if( (readBlock->dataFormat == FORMAT_UINT32))
		{
			index1 = dataLen / 4;			// 수신 바이트 수
			count  = 0;
			wordOffset = 0;
			
			for(i=0; i < index1; i++)
    		{
    		    /* ACE 의 경우 */
				uadata =  rxbuf[count + 0] + ((rxbuf[count + 1] << 8) & 0xff00) +
						((rxbuf[count+ 2] << 16) & 0xff0000) + ((rxbuf[count+ 3] << 24) & 0xff000000);
						
        		fdata = (float ) uadata;

				/* ------------------------------------ */
    	    	/* 	계전기내 포인트 Check... 			*/
        		/* ------------------------------------ */
        		point = checkDevicePoint(readBlock, devid, wordOffset);

        		if(point >= 0)
        		{		
        			if(opr->msgDebug == devid)
        			{
            			Debug(console,"(4)%4.2f ", fdata);
            			if((++inx % 8) == 0)  Debug(console,"\n");
        			}

                    /* ---------------------------------------- */
                    /*  계전기별 포인트 지정 => 스케일링...     */
                    /* ---------------------------------------- */
                    aiPoint = (POINT_BUF *) &dnp->aiPtBuf[point];
                    if(aiPoint->ptConfig)
                    {   
                        aiScale = 1.0;
                        for(j=0; j < aiPoint->ptConfig; j++) aiScale = (aiScale * 0.1);
//                        fdata = (aiScale * adata);
                        fdata = (aiScale * fdata);
                    }
                    
                    //fdata = (float ) adata;
                            			        			
        			/* ------------------------------------ */
        			/* Analog Data Assign into Buffer ...   */
        			/* ------------------------------------ */
        			storeAI_DNP( devid, point, fdata, (byte *) &fdata);
        		}
        		
        		count += 4;		
        		wordOffset += 2;        		
        	}				
		}


		else if(readBlock->dataFormat == FORMAT_FLOAT)
		{
			index1 = dataLen / 4;			// 수신 바이트 수
			count  = 0;
			wordOffset = 0;
			
			if(opr->msgDebug == devid)
	        Debug(console,"rcv %2d> AI-Float index=%d, dateLen=%d / num=%d ... swep= %d \n", devid+1, readIndex,dataLen, index1, readBlock->swepEnb);
	    
			for(i=0; i < index1; i++)
    		{
                /* ---------------------------------------------------- */
        		/* 수신데이터의 FLOAT 형 변환                           */
        		/* 계전기 통신설정 : SWEP - OFF 지정                    */
        		/* 데이터 순서 : L0:L1:H0:H1                            */
        		/* SWEP=100의 경우: L1:L0:H1:H0                         */ 
        		/* SWEP=1  의 경우: H1:H0:L1:L0                         */ 
        		/* SWEP=2  의 경우: H0:H1:L0:L1                         */ 
        		/* ---------------------------------------------------- */
        	    if(readBlock->swepEnb == 100)       // 비츠로 VIDER-M20 
        	    {
        	        /* SWEP ON 의 경우... */
                    tempAI1[0] = rxbuf[count + 2];      // H1
                    tempAI1[1] = rxbuf[count + 3];      // H0
                    tempAI1[2] = rxbuf[count + 0];      // L1
                    tempAI1[3] = rxbuf[count + 1];      // L0
                
                    fptr1 = (float *) &tempAI1[0];
        	    }
        	    else if(readBlock->swepEnb == 1)       
        	    {
        	        /* SWEP ON 의 경우... */
                    tempAI1[0] = rxbuf[count + 0];      // H1
                    tempAI1[1] = rxbuf[count + 1];      // H0
                    tempAI1[2] = rxbuf[count + 2];      // L1
                    tempAI1[3] = rxbuf[count + 3];      // L0
                
                    fptr1 = (float *) &tempAI1[0];
        	    }
        	    else if(readBlock->swepEnb == 2)       
        	    {
        	        /* SWEP ON 의 경우... */
                    tempAI1[0] = rxbuf[count + 1];      // H1
                    tempAI1[1] = rxbuf[count + 0];      // H0
                    tempAI1[2] = rxbuf[count + 3];      // L1
                    tempAI1[3] = rxbuf[count + 2];      // L0
                
                    fptr1 = (float *) &tempAI1[0];
        	    }
        	    else
        	    {   
                    tempAI1[0] = rxbuf[count + 3];      // H1
                    tempAI1[1] = rxbuf[count + 2];      // H0
                    tempAI1[2] = rxbuf[count + 1];      // L1
                    tempAI1[3] = rxbuf[count + 0];      // L0
                
                    fptr1 = (float *) &tempAI1[0];
        		    //fptr1 = (float *) &rxbuf[count];    // 일반 MODBUS 계전기
                }
                
	        	fdata  = (float ) *fptr1;
        
	        	/* ------------------------------------ */
    	    	/* 	계전기내 포인트 Check... 			*/
        		/* ------------------------------------ */
        		point = checkDevicePoint(readBlock, devid, wordOffset);

        		if(point >= 0)
        		{		
                    /* ---------------------------------------- */
                    /*  계전기별 포인트 지정 => 스케일링...     */
                    /* ---------------------------------------- */
                    aiPoint = (POINT_BUF *) &dnp->aiPtBuf[point];
                    if(aiPoint->ptConfig)
                    {   
                        aiScale = 1.0;
                        for(j=0; j < aiPoint->ptConfig; j++) aiScale = (aiScale * 0.1);
                        fdata = (aiScale * fdata);
                    }
            
                    /* 변환된 Low 데이터 */            
                    memcpy( data, &fdata, 4);
        		    
        			if(opr->modDebug == devid)
        			{
            			Debug(console,"(5)%4.2f ", fdata);
            			if((++inx % 8) == 0)  Debug(console,"\n");
        			}
        		
        			/* Analog Data Assign into Buffer ... */
        			storeAI_FLOAT_DNP( devid, point, fdata, data);
        		}
        		count += 4;
        		wordOffset += 2;
        	}
        	
        	if(opr->msgDebug == devid)   Debug(console,"\n");  
        	
    	}
	}
	
	return (0);
	    		
}
