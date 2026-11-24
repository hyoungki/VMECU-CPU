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
int check_SOE_MPLC(int devid, byte *newsts, byte *oldsts, int count)
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
                Debug(console,"dev%2d> MPLC-SOE: pt=%2d, sts=%2x, (debounce=%2d) ... %s \n", devid+1, point+1, state, debounce, ptbuf->nameStr);
                
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
*	MODBUS BLOCK 처리 함수
*/
int	checkDevicePoint_MPLC(MODBUS_READ *read, int devid, int offset)
{
	int	i;
	//int	devPoint, refPoint;
	
	DNP_DEVICE  *dnp;
	//POINT_BUF	*aiPt;
	//POINT_BUF	*diPt;
	MPLC_POINT_BUF  *mplcX, *mplcM;
	
	dnp = (DNP_DEVICE *) dnpcfg[devid];

    if(read->opcode == MPLC_X)
    {        
    	for(i=0; i < dnp->mplc_Xinx; i++)
	    {
    	    mplcX = (MPLC_POINT_BUF *) &dnp->mplc_X[i];
	    
	        if(mplcX->config == 0)  continue;
	    
	        /* MPLC 인덱스와 일치하는 경우... */    
    	    if(mplcX->mplcIndex == offset)
	        {
	            if(opr->modDebug == devid)
			    {
    				Debug(console,"  check-X> dev=%2d/Point=%2d [ MPLC-index =%4d, Offset=%4d ]\n", devid+1, mplcX->devPoint+1, mplcX->mplcIndex, offset);
	    		}
	            return (mplcX->devPoint);
	        }
	    }           
	}
	else if(read->opcode == MPLC_M)
    {
        for(i=0; i < dnp->mplc_Minx; i++)
	    {
    	    mplcM = (MPLC_POINT_BUF *) &dnp->mplc_M[i];
	    
	        if(mplcM->config == 0)  continue;
	    
	        /* MPLC 인덱스와 일치하는 경우... */    
    	    if(mplcM->mplcIndex == offset)
	        {
	            if(opr->modDebug == devid)
			    {
    				Debug(console,"  check-M> dev=%2d/Point=%2d [ MPLC-index =%4d, Offset=%4d ]\n", devid+1, mplcM->devPoint+1, mplcM->mplcIndex, offset);
	    		}
	            return (mplcM->devPoint);
	        }
	    }                   
    }	    
	
	return (-1);
	
}


/*
*	rcv Message 처리 : READ_BLOCK
*/
int checkBlock_MPLC(int scanid, int devid,  int opcode, byte *rxbuf, int dataLen)
{
    int     i,j;
    int	    readIndex;
    int     index1, index2;
    int     wordCount;
    //int     adata;
    int     point, offset;
    int     blockBase, blockOffset;
    //float   *fptr1;
    //float   fdata, aiScale;
    byte	rcvSts[256];        // 포인트 증설(1024) 에 따른 버퍼용량 증대
    //byte    data[8], tempAI1[8];
    
    DNP_DEVICE      *dnp;
	MODBUS_READ	    *readBlock;
	//POINT_BUF       *aiPoint;
	POINT_BUF       *diPoint;
    
    /* ---------------------------------------- */
    /* 미쯔비시 계전기 구조체 찾음 ...          */
    /* ---------------------------------------- */
    dnp = (DNP_DEVICE *) dnpcfg[devid];
    
    /* 현재 수신된 데이터의 readBlock 찾음 ...  */
    readIndex = dnp->readIndex & 0x07;
    readBlock = (MODBUS_READ *) &dnp->readBlock[readIndex];

    /* Base,Offset  Address 지정 */
    blockBase   = readBlock->baseAddr;
    blockOffset = readBlock->baseOffset;
    		
    if(readBlock->opcode == MPLC_X)          
    {
        if(opr->msgDebug == devid)  
        Debug(console,"rcv%2d> mplc Read(%d) M-xxx [Base = %3d (%02x), word=%2d] \n", devid+1, dnp->readIndex, blockBase, blockBase, readBlock->dataNum );
    }
    else if(readBlock->opcode == MPLC_M)     
    {
        if(opr->msgDebug == devid)  
        Debug(console,"rcv%2d> mplc Read(%d) X-xxx [Base = %3d (%02x), word=%2d] \n", devid+1, dnp->readIndex, blockBase, blockBase, readBlock->dataNum );
    }
        
    if(opr->modDebug == devid)
    Debug(console,"\n >> --------------------------------------------------------------------------\n");

    /* ---------------------------------------- */
    /* 상태포인트 수만큼 상태포인트 처리...     */
    /* ---------------------------------------- */
    wordCount = dataLen / 2;
        
    /* 수신데이터가 홀수 Byte인 경우... */
    index2   = dataLen % 2;
    if(index2)
    {    
        wordCount = (dataLen / 2) + 1;
        bzero8248(rcvSts, wordCount*2);
    }
    
    for(i=0; i< dataLen; i++)
   	{
       	rcvSts[i] = rxbuf[i];      // LOW      DI status
    }
    	        
    if(opr->msgDebug == devid)  
    {
        Debug(console,"------------------------------------------------\n");
        DumpBuff(console,"RCV : ", &rcvSts[0], dataLen);
    }

    /* ---------------------------------------- */
    /* 계전기 현재 상태 Update */
    /* ---------------------------------------- */
    offset = blockBase;         // Base Address
    
    for(i=0; i < dataLen; i++)
    {
    	for(j =0; j < 8; j++, offset++)
    	{
    	    /* ------------------------------------ */
    	   	/* 	계전기내 포인트 Check... 			*/
       		/* ------------------------------------ */
       		point = checkDevicePoint_MPLC(readBlock, devid, offset);
    	    
    	    if(point >= 0)
   			{	
   				index1 = point / 8;
   				index2 = point % 8;
        		
        		diPoint = (POINT_BUF *) &dnp->diPtBuf[point];	
        		
   				/* 데이터베이스 정의된 포인트 상태 Update... */
   				if(rcvSts[i] & bitcode[j])  
   			    {
   			        dnp->cursts[index1] |= bitcode[index2];	
   			        //diPoint->status = 1;
   			    }
   				else                        
   			    {
   			        dnp->cursts[index1] &= (~bitcode[index2]);	   
   			        //diPoint->status = 0; 
                }   				    
   		        	    
   			}	
       	}
    }

#if 1            	
    /* ---------------------------------------- */
    /* SOE Check 및 상태정보 저장 ... 8 바이트  */
    /* ---------------------------------------- */                
    if(wordCount)
    {
       	check_SOE_MPLC( devid, &dnp->cursts[0], &dnp->df[0], wordCount*2);
       	curStore_BIT_DNP( devid, &dnp->cursts[0], wordCount*2);
        
       	if(opr->msgDebug == devid)  	DumpBuff(console,"STS2: ", dnp->cursts, wordCount *2);
    }
#endif
    	
    return (0);
    	
}
