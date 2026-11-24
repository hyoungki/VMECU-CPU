/*
 *  VLANE SIO : Channel TASK
*/

#include    "localLib.h"
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


static	byte	bitcode[8] = {0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80};

/*
*   write IIN  : Object-80, Variation-1, Qcode-0x0, Index-0
*/
void    sndACConfirm( int mode, int scanid, int devid)
{
    int     txcnt;
    int     rxVal;
    byte    *txbuf;
    byte    control, thHeader, acHeader;
    word    crc;
    DL_FRAME        *sndData;
    DNP_DATA_LINK   *dnpData;
    DNP_DEVICE  *dnp;     
    
    dnp = (DNP_DEVICE *) dnpcfg[devid];
    
    //dnpData  = (DNP_DATA_LINK *) &dataLinkFrame[fid];
    //sndData  = (DL_FRAME *) &dnpData->sndFrame;
    dnpData  = (DNP_DATA_LINK *) &dnp->dataLinkFrame;
    sndData  = (DL_FRAME *) &dnpData->sndFrame;

    control  = (dnp->sndFcbBit + dnp->sndFcvBit) + 0xc0 + FUNC_UNCONFIRM;
    control  = 0xc0 + FUNC_UNCONFIRM;
    
    control  = 0xe0 + FUNC_UNCONFIRM;

    txbuf = (byte *) &sndData->start1;

    /* check Frame-Control Bit */
    thHeader = TH_FIN_BIT + TH_FIR_BIT + dnp->thSndSeqno;
    acHeader = AC_FIN_BIT + AC_FIR_BIT + dnp->acSndSeqno;
    
    /* 2025-01-02 " TH/AH control... 받은대로 */
    thHeader = TH_FIN_BIT + TH_FIR_BIT + (dnp->rcvTH & 0x3f);
    acHeader = TH_FIN_BIT + TH_FIR_BIT + (dnp->rcvAC & 0x0f);
    
    /* ------------------------ */
    /*  Make HEADER frame ...   */
    /* ------------------------ */
    txcnt = 0;
    txbuf[0] = 0x05;
    txbuf[1] = 0x64;
    txbuf[2] = 3 + 5;                 /* include header, TH,AC,FC */
    txbuf[3] = control;               /* control */
    txbuf[4] = dnp->ioid & 0xff;
    txbuf[5] = (dnp->ioid >> 8) & 0xff;
    txbuf[6] = dnp->hostid & 0xff;
    txbuf[7] = (dnp->hostid >> 8) & 0xff;
    crc = B013_mkdnpcrc(txbuf, 8);

    txbuf[8] = crc & 0xff;
    txbuf[9] = (crc >> 8) & 0xff;
    
    txbuf[10] = thHeader;   /* TH */
    txbuf[11] = acHeader;   /* AC */
    txbuf[12] = 0x0;        /* FC : confirm */
    
    crc = B013_mkdnpcrc( &txbuf[10], 3);
    txbuf[13] = crc & 0xff;
    txbuf[14] = (crc >> 8) & 0xff;
    txcnt = 15;

    if(opr->dnpDebug == devid)   DumpDNP_snd(console,"TXD:", txbuf, txcnt);
    
    if(mode == COM_TCPIP)
    {
        if((rxVal = tkWriteTCP( dnp->SocketFd , txbuf, txcnt, 1000)) < 0 )
    	{
        	if((opr->dnpDebug == devid) || (opr->scanDebug == scanid))
        	Debug(console, "dev%02d> **** net Send Error [%d / %d]...socket = %d !\n", devid+1, txcnt,  rxVal, dnp->SocketFd);
		  	dnp->connectStatus = RESET;
		}      
    }
    else
    {
        //PortWrite( tp, txbuf, txcnt);
        dnpChanWrite( scanid, devid, txbuf, txcnt);
    }
    
    //if(opr->msgDebug == devid) Debug(console,"dev%2d> AC Confirm ...!\n", devid);
    
    pause(100);
}



/***************************************************
    FUNCTION : rcvIINCheck()
*   include TH,AC
***************************************************/
int     rcvIINCheck(int scanid, int devid)
{
    int     rcvCount;

    DNP_APP_FRAME   *rcvAppDnp;
    DNP_DEVICE      *dnp;

    dnp = (DNP_DEVICE *) dnpcfg[devid];
    //rcvAppDnp = (DNP_APP_FRAME *) &rcvAppFrame[fid];
    rcvAppDnp = (DNP_APP_FRAME *) &dnp->rcvAppFrame;
    rcvCount = rcvAppDnp->rcvSize;
    
    dnp->rcvAC      = rcvAppDnp->userData[0];
    dnp->rcvACseq   = dnp->rcvAC & 0x0f;
    
    dnp->rcvACfunc  = rcvAppDnp->userData[1];
    dnp->rcvIIN[0]  = rcvAppDnp->userData[2];
    dnp->rcvIIN[1]  = rcvAppDnp->userData[3];

#if 0
    /* ---------------------------------------------------- */
    /*  2018.03.02 ... Sequence Mismatch...                 */
    /*  송신부와 수신부 AC-Seq 차이가 있는경우... RESET     */
    /*  지속적인 Seqno-mismatch.... RESET-LINK      */
    /* ---------------------------------------------------- */
    if(dnp->rcvACseq != dnp->acSndSeqno)
    {   
        if(++dnp->ACseqFail > 10)
        {   
            dnp->ACseqFail = 0; 
            
            if(opr->dnpDebug == devid)
            {     
                Debug(console,"\n=========================================\n");   
                Debug(console,"dev%02d> *** rcv AC-Seqno mismatch... (%d %d) ... SYNC \n", devid, dnp->rcvACseq, dnp->acSndSeqno);
                Debug(console,"=========================================\n");   
            }
            
            dnp->acSndSeqno = dnp->rcvACseq;
            
            /* ------------------------------------ */
            /*  RESAET-LINK                     */
            /* ------------------------------------ */                  
            dnp->sndLinkSts = RESET;    // RESET-LINK 적용
            dnp->offlineTick = 0;
        }
    }
    else
    {
        dnp->ACseqFail = 0; 
    }
#endif          

#if 0
    /* ------------------------------------------------ */
    /* 2017년 1월 7일...                                */
    /* 송신 seq와 수신 seq 가 틀린경우... status dump   */
    /* ------------------------------------------------ */
    if(dnp->rcvACseq != dnp->acSndSeqno)
    {   
        if(opr->dnpDebug == devid)
        {     
            Debug(console,"\n=========================================\n");   
            Debug(console,"dev%02d> *** rcv AC-Seqno mismatch... (%d %d) ... SYNC \n", devid, dnp->rcvACseq, dnp->acSndSeqno);
            Debug(console,"=========================================\n");   
        }
            
        dnp->acSndSeqno = dnp->rcvACseq;
        dnp->statusDump = SET;
    }
#endif
    
        
    /* ---------------------------------------- */
    /* 계전기별 IIN 정보 : IIN0 Check ...       */
    /* ---------------------------------------- */
    if(dnp->rcvIIN[0] & BIT_DEV_RESTART)    dnp->devRestart= SET;
    else                                    dnp->devRestart= RESET;

    if(dnp->rcvIIN[0] & BIT_TIME_SYNC_REQ)  dnp->timeSyncReq= SET;

    /* ---------------------------------------- */
    /* CLASS Information Request...Check ...    */
    /* ---------------------------------------- */
    if(dnp->rcvIIN[0] & BIT_CLASS1_REQ)     dnp->class1Req = SET;
    else                                    dnp->class1Req = RESET;
        
    if(dnp->rcvIIN[0] & BIT_CLASS2_REQ)     dnp->class2Req = SET;
    else                                    dnp->class2Req = RESET;
        
    if(dnp->rcvIIN[0] & BIT_CLASS3_REQ)     dnp->class3Req = SET;
    else                                    dnp->class3Req = RESET;
        
    if(dnp->class1Req+dnp->class2Req+dnp->class3Req)    dnp->classReq = SET;
    else                                                dnp->classReq = RESET;
         
    if(dnp->rcvIIN[1] & BIT_BUFF_OVER)
    {
        if(opr->msgDebug == devid)
        Debug(console,"[%2d] ***Rcv IIN : BIT_BUFF_OVER set !\n", devid +1);
        
        /* 2020.06.05 IIN 상태비트 무시 기능 .... 계전기 AO-Point */
        if(dnp->devBCPoint == 0)
        {	
        	dnpFlagClear(devid);
        	return (0);   
        }
    }

    if(dnp->rcvIIN[1] & BIT_FUNC_FAIL)  
    {
        if(opr->msgDebug == devid)
        Debug(console,"[%2d] ***Rcv IIN : FUNCTION_FAIL set !\n", devid +1);
        
        /* 2020.06.05 IIN 상태비트 무시 기능 .... 계전기 AO-Point */
        if(dnp->devBCPoint == 0)
        {	
        	dnpFlagClear(devid);
        	return (0);   
        }
    }
    if(dnp->rcvIIN[1] & BIT_OBJECT_FAIL)  
    {
        if(opr->msgDebug == devid)
        Debug(console,"[%2d] ***Rcv IIN : BIT_OBJECT_UN_KNOWN set !\n", devid +1);

		/* 2020.06.05 IIN 상태비트 무시 기능 .... 계전기 AO-Point */
        if(dnp->devBCPoint == 0)
        {	
        	dnpFlagClear(devid);
        	return (0);   
        }
    }
    if(dnp->rcvIIN[1] & BIT_OUT_RANGE)  
    {
        if(opr->msgDebug == devid)
        Debug(console,"[%2d] ***Rcv IIN : BIT_OUT RANGE_INVALID set !\n", devid +1);

		/* 2020.06.05 IIN 상태비트 무시 기능 .... 계전기 AO-Point */
        if(dnp->devBCPoint == 0)
        {	
        	dnpFlagClear(devid);
        	return (0);   
        }
    }
            
    return (1);    
}        

/***************************************************
    FUNCTION : countDumpAssign()
*   include TH,AC
***************************************************/
int     countDumpAssign(int scanid, int devid)
{
    //int     ioid;
    int     rcvCount, dataPos, dataSize;
    byte    data[8];
    //byte    pointFlag;
    int     adata;
    int     count;
    //float   *fptr1, *fptr2;
    float   fdata;
    //long    *lptr1;
    //int     sign, exp, mant;
    int     index1, index2, point;
    //long    convData;
    //ulong   anaData;
                    
    DNP_APP_FRAME   *rcvAppDnp;
    DNP_DEVICE      *dnp;
        
    dnp = (DNP_DEVICE *) dnpcfg[devid];
    //rcvAppDnp = (DNP_APP_FRAME *) &rcvAppFrame[fid];
    rcvAppDnp = (DNP_APP_FRAME *) &dnp->rcvAppFrame;
    rcvCount  = rcvAppDnp->rcvSize;
    dataPos   = dnp->rcvDataPos;

    /* ------------------------------------ */
    /*  계전기 Counter Data Assign ...      */   
    /* ------------------------------------ */  
    if(dnp->rcvVar == 1)            // 32Bit Binary Count Input with FLAG
    {
        dataSize  = (dnp->rcvStop - dnp->rcvStart) + 1;
        dataSize  = dataSize * 5;        
    } 
    else if(dnp->rcvVar == 5)       // 32Bit Binary Count Input without FLAG
    {
        dataSize  = (dnp->rcvStop - dnp->rcvStart) + 1;
        dataSize  = dataSize * 4;        
    }        
    else
    {	
    	if(opr->msgDebug == devid)  Debug(console,"=> *** Invalid Binary Counter VAR (cpu:1, 5) rcv...%d\n", dnp->rcvVar);
        return (0);
	}
        
    if(dnp->rcvVar == 1)    // 32Bit Binary Count Input with FLAG
    {
        index1 = dataSize / 5;
        index2 = dataSize % 5;

        if(opr->msgDebug == devid)  Debug(console,"=> Binary Count(32Bit int/Flag) \n");
        	
        count = 0;        
        for(point = dnp->rcvStart; point <= dnp->rcvStop; point++)
        {
            /* ---------------------------------------- */
            /* 2014.12.11 수신된 POINT 예외처리...      */
            /* ---------------------------------------- */
            if(point >= MAX_DEV_COUNT_POINT)
            {
                Debug(console,"rcv> dev=%2d, *Binary Count(32Bit int/Flag) - *Invalid AI-Point (%d) atart=%d, end=%d..!\n", devid+1, point, dnp->rcvStart, dnp->rcvStop);
                break;
            }
        	data[4] = rcvAppDnp->userData[dataPos+0];	// Flag
        	
            data[0] = rcvAppDnp->userData[dataPos+1];
            data[1] = rcvAppDnp->userData[dataPos+2];
            data[2] = rcvAppDnp->userData[dataPos+3];
            data[3] = rcvAppDnp->userData[dataPos+4];
            
            adata =   data[0] + ((data[1] << 8) & 0xff00) + 
                        ((data[2] << 16) & 0xff0000) + ((data[3] << 24) & 0xff000000);
                          
            dataPos += 5;

            fdata = (float ) adata;
            
            /* ------------------------------------ */
            /* COUNTER Data Assign into Buffer ...  */
            /* ------------------------------------ */
            storeCOUNT_DNP( devid, point, adata, (byte *)&fdata);
            
            if(opr->msgDebug == devid)  
            {
                Debug(console," %3d (%4f)", adata, fdata);
                if((++count % 8) == 0)  Debug(console,"\n");
            }

        }
        if(opr->msgDebug == devid)  Debug(console,"\n");
        dnp->countDump = RESET;
    }    
    else if(dnp->rcvVar == 5)    // 32Bit Binary Count Input without FLAG
    {
        index1 = dataSize / 4;
        index2 = dataSize % 4;

        if(opr->msgDebug == devid)  Debug(console,"=> Binary Count(32Bit int/No Flag) \n");
        	
        count = 0;        
        for(point = dnp->rcvStart; point <= dnp->rcvStop; point++)
        {
            /* ---------------------------------------- */
            /* 2014.12.11 수신된 POINT 예외처리...      */
            /* ---------------------------------------- */
            if(point >= MAX_DEV_COUNT_POINT)
            {
                Debug(console,"rcv> dev=%2d, *Binary Count(32Bit int/No Flag) - *Invalid AI-Point (%d) atart=%d, end=%d..!\n", devid+1, point, dnp->rcvStart, dnp->rcvStop);
                break;
            }
            
            data[0] = rcvAppDnp->userData[dataPos+0];
            data[1] = rcvAppDnp->userData[dataPos+1];
            data[2] = rcvAppDnp->userData[dataPos+2];
            data[3] = rcvAppDnp->userData[dataPos+3];
            
            adata =   data[0] + ((data[1] << 8) & 0xff00) + 
                        ((data[2] << 16) & 0xff0000) + ((data[3] << 24) & 0xff000000);
                          
            dataPos += 4;

            fdata = (float ) adata;
            
            /* ------------------------------------ */
            /* COUNTER Data Assign into Buffer ...  */
            /* ------------------------------------ */
            storeCOUNT_DNP( devid, point, adata, (byte *)&fdata);
            
            if(opr->msgDebug == devid)  
            {
                Debug(console," %3d (%4f)", adata, fdata);
                if((++count % 8) == 0)  Debug(console,"\n");
            }

        }
        if(opr->msgDebug == devid)  Debug(console,"\n");
        dnp->countDump = RESET;
    }                      
                       
            
    return (dataSize);
}

/***************************************************
    FUNCTION : anaDumpAssign()
*   include TH,AC
***************************************************/
int     anaDumpAssign(int scanid, int devid)
{
    int     i;
    int     rcvCount, dataPos, dataSize=0;
    byte    data[8];
    byte    pointFlag;
    int     adata;
    short int	bdata;
    int     count;
    float   *fptr1;
    float   fdata, aiScale;
    //long    *lptr1;
    //int     sign, exp, mant;
    int     index1, index2, point;
    //long    convData;
    ulong   anaData;
                    
    DNP_APP_FRAME   *rcvAppDnp;
    DNP_DEVICE      *dnp;
    POINT_BUF       *aiPoint;
        
    dnp = (DNP_DEVICE *) dnpcfg[devid];
    //rcvAppDnp = (DNP_APP_FRAME *) &rcvAppFrame[fid];
    rcvAppDnp = (DNP_APP_FRAME *) &dnp->rcvAppFrame;
    rcvCount  = rcvAppDnp->rcvSize;
    dataPos   = dnp->rcvDataPos;

    /* ------------------------------------ */
    /*  계전기 ANALOG Data Assign ...       */   
    /* ------------------------------------ */  
    if(dnp->rcvVar == 1)            // 32Bit Analog Input with FLAG
    {
        dataSize  = (dnp->rcvStop - dnp->rcvStart) + 1;
        dataSize  = dataSize * 5;        
    } 
    else if(dnp->rcvVar == 2)            // 16Bit Analog Input with FLAG
    {
        dataSize  = (dnp->rcvStop - dnp->rcvStart) + 1;
        dataSize  = dataSize * 3;        
    } 
    else if(dnp->rcvVar == 3)       // 32Bit Analog Input without FLAG
    {
        dataSize  = (dnp->rcvStop - dnp->rcvStart) + 1;
        dataSize  = dataSize * 4;        
    }
    else if(dnp->rcvVar == 4)       // 16Bit Analog Input without FLAG
    {
        dataSize  = (dnp->rcvStop - dnp->rcvStart) + 1;
        dataSize  = dataSize * 2;        
    }  
    else if(dnp->rcvVar == 5)       // Short Float Analog Input with FLAG
    {
        dataSize  = (dnp->rcvStop - dnp->rcvStart) + 1;
        dataSize  = dataSize * 5;        
    }        
    
    /* -------------------------------------------------------- */
    /*  VAR-Type 에 따른 데이터 처리...                         */
    /* -------------------------------------------------------- */    
    if(dnp->rcvVar == 1)    /* 32bit analog input with Flag */    
    {
        index1 = dataSize / 5;
        index2 = dataSize % 5;

        if(opr->msgDebug == devid)  Debug(console,"=> Analog(32Bit int FLAG) \n");
        count = 0;        
        for(point = dnp->rcvStart; point <= dnp->rcvStop; point++)
        {
            /* ---------------------------------------- */
            /* 2014.12.11 수신된 POINT 예외처리...      */
            /* ---------------------------------------- */
            if(point >= MAX_DEV_AI_POINT)
            {
                Debug(console,"rcv> dev=%2d, *Analog(32Bit int FLAG) - *Invalid AI-Point (%d) atart=%d, end=%d..!\n", devid+1, point, dnp->rcvStart, dnp->rcvStop);
                break;
            }
                
            pointFlag = rcvAppDnp->userData[dataPos+0];
            data[0] = rcvAppDnp->userData[dataPos+1];
            data[1] = rcvAppDnp->userData[dataPos+2];
            data[2] = rcvAppDnp->userData[dataPos+3];
            data[3] = rcvAppDnp->userData[dataPos+4];
            
            adata =   data[0] + ((data[1] << 8) & 0xff00) + 
                        ((data[2] << 16) & 0xff0000) + ((data[3] << 24) & 0xff000000);
                          
            dataPos += 5;

            /* 기본 데이터 지정 */
            fdata = (float ) adata;
            
            /* ---------------------------------------- */
            /*  계전기별 포인트 지정 => 스케일링...     */
            /* ---------------------------------------- */
            aiPoint = (POINT_BUF *) &dnp->aiPtBuf[point];
            if(aiPoint->config != SET)  continue;
            if(aiPoint->ptConfig)
            {   
                aiScale = 1.0;
                for(i=0; i < aiPoint->ptConfig; i++) aiScale = (aiScale * 0.1);
                fdata = (aiScale * adata);
            }
            
            //fdata = (float ) adata;
            
            /* ------------------------------------ */
            /* Analog Data Assign into Buffer ...   */
            /* ------------------------------------ */
            storeAI_DNP( devid, point, fdata, (byte *) &fdata);
            
            if(opr->msgDebug == devid)  
            {
                Debug(console," %3d (%4f)", adata, fdata);
                if((++count % 8) == 0)  Debug(console,"\n");
            }

        }
        if(opr->msgDebug == devid)  Debug(console,"\n");
        dnp->anaOutDump = RESET;
        dnp->analogDump = RESET;
    }                       
    else if(dnp->rcvVar == 2)    /* 16bit analog input with Flag */    
    {
        index1 = dataSize / 3;
        index2 = dataSize % 3;
        
        if(opr->msgDebug == devid)  Debug(console,"=> Analog(16Bit int FLAG) \n");
        count = 0;
        for(point = dnp->rcvStart; point <= dnp->rcvStop; point++)
        {
            /* ---------------------------------------- */
            /* 2014.12.11 수신된 POINT 예외처리...      */
            /* ---------------------------------------- */
            if(point >= MAX_DEV_AI_POINT)
            {
                Debug(console,"rcv> dev=%2d, *Analog(16Bit int FLAG) - *Invalid AI-Point (%d) atart=%d, end=%d..!\n", devid+1, point, dnp->rcvStart, dnp->rcvStop);
                break;
            }
            
            pointFlag = rcvAppDnp->userData[dataPos+0];
            data[0]   = rcvAppDnp->userData[dataPos+1];
            data[1]   = rcvAppDnp->userData[dataPos+2];
            data[2]   = 0;
            data[3]   = 0;
            
            bdata =   data[0] + ((data[1] << 8) & 0xff00);		// short int 처리
            dataPos += 3;

            /* 기본 데이터 지정 */
            fdata = (float ) bdata;
            
            /* ---------------------------------------- */
            /*  계전기별 포인트 지정 => 스케일링...     */
            /* ---------------------------------------- */
            aiPoint = (POINT_BUF *) &dnp->aiPtBuf[point];
            if(aiPoint->config != SET)  continue;
            if(aiPoint->ptConfig)
            {   
                aiScale = 1.0;
                for(i=0; i < aiPoint->ptConfig; i++) aiScale = (aiScale * 0.1);
                fdata = (aiScale * bdata);
            }
            
            //fdata = (float ) adata;
            
            /* ------------------------------------ */
            /* Analog Data Assign into Buffer ...   */
            /* ------------------------------------ */
            storeAI_DNP( devid, point, fdata, (byte *)&fdata);
            
            if(opr->msgDebug == devid)  
            {
                Debug(console," %3f(%3d) ", fdata,bdata );
                if((++count % 8) == 0)  Debug(console,"\n");
            }

        }
        if(opr->msgDebug == devid)  Debug(console,"\n");
        dnp->anaOutDump = RESET;
        dnp->analogDump = RESET;
    }                       
    else if(dnp->rcvVar == 3)    /* HIMAP: 32bit analog input without Flag */    
    {
        index1 = dataSize / 4;
        index2 = dataSize % 4;

        if(opr->msgDebug == devid)  Debug(console,"=> Analog(32Bit int) \n");
        count = 0;        
        for(point = dnp->rcvStart; point <= dnp->rcvStop; point++)
        {
            /* ---------------------------------------- */
            /* 2014.12.11 수신된 POINT 예외처리...      */
            /* ---------------------------------------- */
            if(point >= MAX_DEV_AI_POINT)
            {
                Debug(console,"rcv> dev=%2d, *Analog(32Bit int) - *Invalid AI-Point (%d) atart=%d, end=%d..!\n", devid+1, point, dnp->rcvStart, dnp->rcvStop);
                break;
            }
            
            data[0] = rcvAppDnp->userData[dataPos+0];
            data[1] = rcvAppDnp->userData[dataPos+1];
            data[2] = rcvAppDnp->userData[dataPos+2];
            data[3] = rcvAppDnp->userData[dataPos+3];
            
            adata =   data[0] + ((data[1] << 8) & 0xff00) + 
                        ((data[2] << 16) & 0xff0000) + ((data[3] << 24) & 0xff000000);
                          
            dataPos += 4;

            /* 기본 데이터 지정 */
            fdata = (float ) adata;
            
            /* ---------------------------------------- */
            /*  계전기별 포인트 지정 => 스케일링...     */
            /* ---------------------------------------- */
            aiPoint = (POINT_BUF *) &dnp->aiPtBuf[point];
            if(aiPoint->config != SET)  continue;
            if(aiPoint->ptConfig)
            {   
                aiScale = 1.0;
                for(i=0; i < aiPoint->ptConfig; i++) aiScale = (aiScale * 0.1);
                fdata = (aiScale * adata);
            }
            
            //fdata = (float ) adata;
            
            /* ------------------------------------ */
            /* Analog Data Assign into Buffer ...   */
            /* ------------------------------------ */
            storeAI_DNP( devid, point, fdata, (byte *)&fdata);
            
            if(opr->msgDebug == devid)  
            {
                Debug(console," %3d (%4f)", adata, fdata);
                if((++count % 8) == 0)  Debug(console,"\n");
            }

        }
        if(opr->msgDebug == devid)  Debug(console,"\n");
        dnp->anaOutDump = RESET;
        dnp->analogDump = RESET;
    }                       
    else if(dnp->rcvVar == 4)    /* HIMAP: 16bit analog input */
    {
        index1 = dataSize / 2;
        index2 = dataSize % 2;
        
        if(opr->msgDebug == devid)  Debug(console,"=> Analog(16Bit int) \n");
        count = 0;
        for(point = dnp->rcvStart; point <= dnp->rcvStop; point++)
        {
            /* ---------------------------------------- */
            /* 2014.12.11 수신된 POINT 예외처리...      */
            /* ---------------------------------------- */
            if(point >= MAX_DEV_AI_POINT)
            {
                Debug(console,"rcv> dev=%2d, *Analog(16Bit int) - *Invalid AI-Point (%d) atart=%d, end=%d..!\n", devid+1, point, dnp->rcvStart, dnp->rcvStop);
                break;
            }
            
            data[0] = rcvAppDnp->userData[dataPos+0];
            data[1] = rcvAppDnp->userData[dataPos+1];
            data[2] = 0;
            data[3] = 0;
            
            bdata =   data[0] + ((data[1] << 8) & 0xff00);
            dataPos += 2;
            
            /* 기본 데이터 지정 */
            fdata = (float ) bdata;
            
            /* ---------------------------------------- */
            /*  계전기별 포인트 지정 => 스케일링...     */
            /* ---------------------------------------- */
            aiPoint = (POINT_BUF *) &dnp->aiPtBuf[point];
            if(aiPoint->config != SET)  continue;
                
            if(aiPoint->ptConfig)
            {   
                aiScale = 1.0;
                for(i=0; i < aiPoint->ptConfig; i++) aiScale = (aiScale * 0.1);
                fdata = (aiScale * bdata);
            }
            
            //fdata = (float ) adata;

            /* ------------------------------------ */
            /* Analog Data Assign into Buffer ...   */
            /* ------------------------------------ */
            storeAI_DNP( devid, point, fdata, (byte *)&fdata);
            
            if(opr->msgDebug == devid)  
            {
                Debug(console," %3d ", bdata);
                if((++count % 8) == 0)  Debug(console,"\n");
            }
            
        }
        if(opr->msgDebug == devid)  Debug(console,"\n");
        dnp->analogDump = RESET;
    }                           
    else if(dnp->rcvVar == 5)    /* GIPAM2000: Short Floatting Form */    
    {
        index1 = dataSize / 5;
        index2 = dataSize % 5;
        
        if(opr->msgDebug == devid)  Debug(console,"=> Analog(Float) with Flag \n");
        count = 0;
        for(point = dnp->rcvStart; point <= dnp->rcvStop; point++)
        {
            /* ---------------------------------------- */
            /* 2014.12.11 수신된 POINT 예외처리...      */
            /* ---------------------------------------- */
            if(point >= MAX_DEV_AI_POINT)
            {
                Debug(console,"rcv> dev=%2d, *Analog(Float with Flag) - *Invalid AI-Point (%d) atart=%d, end=%d..!\n", devid+1, point, dnp->rcvStart, dnp->rcvStop);
                break;
            }
            
            data[4] = rcvAppDnp->userData[dataPos+0];       // point status 
            
            data[3] = rcvAppDnp->userData[dataPos+1];
            data[2] = rcvAppDnp->userData[dataPos+2];
            data[1] = rcvAppDnp->userData[dataPos+3];
            data[0] = rcvAppDnp->userData[dataPos+4];
    
            /* goto next point */
            dataPos += 5;
            
            /* float  to int    32.1 -> 321 */
            fptr1 = (float *) &data[0];
            fdata = (float ) *fptr1;

#if 1            
            /* ---------------------------------------- */
            /*  계전기별 포인트 지정 => 스케일링...     */
            /* ---------------------------------------- */
            aiPoint = (POINT_BUF *) &dnp->aiPtBuf[point];
            if(aiPoint->config != SET)  continue;
            if(aiPoint->ptConfig)
            {   
                aiScale = 1.0;
                for(i=0; i < aiPoint->ptConfig; i++) aiScale = (aiScale * 0.1);
                fdata = (aiScale * fdata);
            }
            
            /* 변환된 Low 데이터 */            
            memcpy( data, &fdata, 4);
#endif
            
            anaData = (int) fdata;
            
            if(opr->msgDebug == devid)  
            {
                Debug(console,"%d) %4.2f/%4d  ", point, fdata,anaData );
                if((++count % 4) == 0)  Debug(console,"\n");
            }
            
            /* Analog Data Assign into Buffer ... */
            storeAI_FLOAT_DNP( devid, point, fdata, (byte *) data);
            
        }
        
        if(opr->msgDebug == devid)  Debug(console,"\n");
        dnp->analogDump = RESET;
    }                    
    
    return (dataSize);
}

/***************************************************
    FUNCTION : anaOutDumpAssign()
*   include TH,AC
***************************************************/
int     anaOutDumpAssign(int scanid, int devid)
{
    //int     ioid;
    int     rcvCount, dataPos, dataSize;
    byte    data[8];
    int     adata;
    float   *fptr1;
    //long    *lptr1;
    //int     sign, exp, mant;
    int     index1, index2, point;
    //long    convData;
                    
    DNP_APP_FRAME   *rcvAppDnp;
    DNP_DEVICE      *dnp;
        
    dnp = (DNP_DEVICE *) dnpcfg[devid];
    //rcvAppDnp = (DNP_APP_FRAME *) &rcvAppFrame[fid];
    rcvAppDnp = (DNP_APP_FRAME *) &dnp->rcvAppFrame;
    rcvCount  = rcvAppDnp->rcvSize;
    dataPos   = dnp->rcvDataPos;
    dataSize  = rcvCount - dataPos;

    dataSize  = (dnp->rcvStop - dnp->rcvStart) + 1;
    dataSize  = dataSize * 5;   
        
#if 0
    if(opr->msgDebug == devid)
    {    
        Debug(console,"rcv%2d> AO dump ... %d\n", devid+1, dataSize);
        DumpBuff(console,"ANA: ", &rcvAppDnp->userData[dataPos], dataSize); 
    }
#endif

    if(dnp->rcvVar == 3)    /* GIPAM2000: Short Floatting Form */    
    {
        index1 = dataSize / 5;
        index2 = dataSize % 5;
        if(index2 != 0)
        {
            Debug(console,"*** invalid data size ...! %s %d \n",__FILE__,__LINE__);
            return (0);
        }
        
        for(point = dnp->rcvStart; point < dnp->rcvStop; point++)
        {
            data[3] = rcvAppDnp->userData[dataPos+1];
            data[2] = rcvAppDnp->userData[dataPos+2];
            data[1] = rcvAppDnp->userData[dataPos+3];
            data[0] = rcvAppDnp->userData[dataPos+4];
    
            dataPos += 5;
            fptr1 = (float *) &data[0];
            adata = (*fptr1) * 10;
            
            /* Analog Data Assign into Buffer ... */
            storeAO_DNP( devid, point, adata);
        }
        dnp->anaOutDump = RESET;
    }
    
    return (dataSize);
}



/***************************************************
    FUNCTION : stsDumpAssign()
*   include TH,AC
***************************************************/
int     stsDumpAssign(int scanid, int devid)
{
    int     i;
    int     index1, index2;
    int     rcvCount, dataPos, dataSize;
    int     pointNum;
    int     modula;
    byte    pointData;
    byte    stsbuf[128];
                
    DNP_APP_FRAME   *rcvAppDnp;
    DNP_DEVICE      *dnp;
    //POINT_BUF       *diPoint;
    
    dnp = (DNP_DEVICE *) dnpcfg[devid];
    //rcvAppDnp = (DNP_APP_FRAME *) &rcvAppFrame[fid];
    rcvAppDnp = (DNP_APP_FRAME *) &dnp->rcvAppFrame;

    rcvCount  = rcvAppDnp->rcvSize;
    dataPos   = dnp->rcvDataPos;

    /* -------------------------------- */
    /* 해당 OBJ 구성 포인트 정보 발췌   */ 
    /* -------------------------------- */
    pointNum  = (dnp->rcvStop - dnp->rcvStart) + 1;
    
    if(dnp->rcvVar == 1)    /* Single-BIT Binary Input */   
    {
        dataSize  = pointNum / 8; 
        modula    = pointNum % 8;

        /* 8Bit 단위로 떨어지지않는 경우...*/
        if(modula)    dataSize = dataSize + 1;  

        if(opr->msgDebug == devid)
        {
            Debug(console,"rcv%2d> STS(bit) dump size[%d] \n", devid+1, dataSize); 
            DumpBuff(console,"STS: ", &rcvAppDnp->userData[dataPos], dataSize); 
        }
                    
        curStore_BIT_DNP( devid, &rcvAppDnp->userData[dataPos], dataSize);
        
        return (dataSize);
    }
    else if(dnp->rcvVar == 2)    /* Binary Input with Status*/   
    {
        //Debug(console,"rcv%2d> Status-byte dump size [%d] \n", devid+1, pointNum); 
        
        dataSize  = pointNum / 8; 
        modula    = pointNum % 8;

        /* 8Bit 단위로 떨어지지않는 경우...*/
        if(modula)    dataSize = dataSize + 1;  
            
        bzero(stsbuf, 128);
        
        for(i=0; i < pointNum; i++)
        {
            pointData = rcvAppDnp->userData[dataPos + i];
            
            index1  = i / 8; 
            index2  = i % 8;
            
            if(pointData & 0x80)    stsbuf[index1] |= bitcode[index2];
        }
        
        if(opr->msgDebug == devid)
        {
            Debug(console,"rcv%2d> STS(Byte->Bit) dump size[%d] \n", devid+1, dataSize); 
            DumpBuff(console,"STS: ", &stsbuf[0], dataSize); 
        }
        
        curStore_BIT_DNP( devid, &stsbuf[0], dataSize);
        
        return (pointNum);
    }
    
    return (0);
}

/*
    convert Byte ordering ... 
*/
void    convertRTC(byte *src, byte *des)
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



/***************************************************
    FUNCTION : soeDumpAssign()
*   rcv OBJ : 02, Var : 02, Qcode : 7    
*   include TH,AC
***************************************************/
int     soeDumpAssign(int fid, int devid)
{
    int     i;
    int     dataPos, dataSize=0;
    int     state, point;
    //int     pointIndex, pointBase;
    //int     hostPt, portNum, ptNum;
    byte    *buf;
                
    DNP_APP_FRAME   *rcvAppDnp;
    DNP_DEVICE      *dnp;
    //POINT_BUF       *diPoint;

    dnp = (DNP_DEVICE *) dnpcfg[devid];
    //rcvAppDnp = (DNP_APP_FRAME *) &rcvAppFrame[fid];
    rcvAppDnp = (DNP_APP_FRAME *) &dnp->rcvAppFrame;

    /* soe dump Data 위치 */
    dataPos   = dnp->rcvDataPos;

    /* ---------------------------------------------------------- */ 
    /* OBJ = 2, Var = 2 : SOE - Binary Input Change With TIME     */ 
    /* ---------------------------------------------------------- */   
    if(dnp->rcvVar == 2)    /* Binary Input with Time */   
    {
        if(dnp->rcvQcode == 8)              /* 16 bit 포인트 정보 */
        {
            /* SOE 정보당 9 Byte 구성 ,point-2, time-6,state-1 */
            dataSize  = dnp->rcvQnum * 9;
            
            for(i=0; i< dnp->rcvQnum; i++)
            {
                buf = (byte *) &rcvAppDnp->userData[dataPos];
                
                point = buf[0] + (buf[1]*256);          // 포인트 번호
                state = (buf[2] >> 7) & 0x01;;          // 포인트 상태정보 
                
                /* ------------------------------------ */
                /*  SOE Update 보완 (Qcode7 ,8)         */
                /* ------------------------------------ */
                storeSOE_CU_DNP(devid, point, state, &buf[3]); 
                dataPos = dataPos + 9;
            }
        }
        else if(dnp->rcvQcode == 7)         /* 8 bit 포인트 정보 */
        {        
            /* SOE 정보당 8 Byte 구성 ,point-1, time-6,state-1 */
            dataSize  = dnp->rcvQnum * 8;
        
            for(i=0; i< dnp->rcvQnum; i++)
            {
                buf = (byte *) &rcvAppDnp->userData[dataPos];

                point = buf[0];                         // 포인트 번호
                state = (buf[1] >> 7) & 0x01;;          // 포인트 상태정보 

                /* ------------------------------------ */
                /*  SOE Update 보완 (Qcode7 ,8)         */
                /* ------------------------------------ */
                storeSOE_CU_DNP(devid, point, state, &buf[2]); 
                //storeSOE_DNP(devid, buf[0], buf); 
                
                dataPos = dataPos + 8;
            }
        }
        
        if(opr->msgDebug == devid)
        Debug(console,"rcv%2d> SOE Dump ... Qcode=%d, datum= %d OK \n", devid+1, dnp->rcvQcode, dnp->rcvQnum);
        
        return (dataSize);
    }                    
    /* ------------------------------------------------------------- */ 
    /* OBJ = 2, Var = 1 : COS - Binary Input Change Without TIME     */ 
    /* ------------------------------------------------------------- */   
    else if(dnp->rcvVar == 1)    /* COS : Binary Input without Time  */   
    {
        if(dnp->rcvQcode == 8)              /* 16 bit 포인트 정보 */
        {
            /* COS 정보당 3 Byte 구성 ,point-2, state-1 */
            dataSize  = dnp->rcvQnum * 3;
        
            for(i=0; i< dnp->rcvQnum; i++)
            {
                buf = (byte *) &rcvAppDnp->userData[dataPos];

                point = buf[0] + (buf[1]*256);          // 포인트 번호
                state = (buf[2] >> 7) & 0x01;;          // 포인트 상태정보                
            
                /* ------------------------------------ */
                /*  SOE Update 보완 (Qcode7 ,8)         */
                /* ------------------------------------ */     
                storeCOS_CU_DNP(devid, point, state); 
                dataPos = dataPos + 3;
            }
        }
        else if(dnp->rcvQcode == 7)         /* 8 bit 포인트 정보 */
        {  
            /* COS 정보당 2 Byte 구성 ,point-1, state-1 */
            dataSize  = dnp->rcvQnum * 2;
        
            for(i=0; i< dnp->rcvQnum; i++)
            {
                buf = (byte *) &rcvAppDnp->userData[dataPos];

                point = buf[0];                         // 포인트 번호   
                state = (buf[1] >> 7) & 0x01;;          // 포인트 상태정보                
            
                /* ------------------------------------ */
                /*  SOE Update 보완 (Qcode7 ,8)         */
                /* ------------------------------------ */
                storeCOS_CU_DNP(devid, point, state); 
                dataPos = dataPos + 2;
            }
        }
        
        if(opr->msgDebug == devid)
        Debug(console,"rcv%2d> COS Dump ...Qcode=%d, datum= %d OK \n", devid+1, dnp->rcvQcode, dnp->rcvQnum);
        
        return (dataSize);
    }                    
    return (0);
}


/***************************************************
    FUNCTION : coaDumpAssign()
*   rcv OBJ : 32, Var : 1,2,3,4, Qcode : 7    
*   include TH,AC
***************************************************/
int     coaDumpAssign(int fid, int devid)
{
    int     i;
    int     dataPos, dataSize=0;
    int     eventSize;
    //int     state, point;
    //int     pointIndex, pointBase;
    //int     hostPt, portNum, ptNum;
    byte    *buf;

#if 1                
    DNP_APP_FRAME   *rcvAppDnp;
    DNP_DEVICE      *dnp;
    //POINT_BUF       *diPoint;

    dnp = (DNP_DEVICE *) dnpcfg[devid];
    //rcvAppDnp = (DNP_APP_FRAME *) &rcvAppFrame[fid];
    rcvAppDnp = (DNP_APP_FRAME *) &dnp->rcvAppFrame;

    /* soe dump Data 위치 */
    dataPos   = dnp->rcvDataPos;

    /* -------------------------------------------------------------------- */ 
    /* OBJ = 32, Var = 1 : COA - 32Bit Analog Change event without TIME     */ 
    /* -------------------------------------------------------------------- */  
    if(dnp->rcvVar == 1)        eventSize = 6;
    else if(dnp->rcvVar == 2)   eventSize = 4;
    else if(dnp->rcvVar == 3)   eventSize = 12;
    else if(dnp->rcvVar == 4)   eventSize = 10;
    else    return (0);                    
    
    /* COA 정보당 Byte 구성 */
    dataSize  = dnp->rcvQnum * eventSize;
        
    if(opr->soeDebug)
    {
        Debug(console,"rcv%2d> COA dump ... %d\n", devid+1, dnp->rcvQnum);
        DumpBuff(console,"DATA: ", &rcvAppDnp->userData[dataPos], dataSize); 
    }        
           
    for(i=0; i< dnp->rcvQnum; i++)
    {
        buf = (byte *) &rcvAppDnp->userData[dataPos];

        //storeCOA_DNP(devid, buf, dnp->rcvVar); 
        
        dataPos = dataPos + eventSize;
    }
        
    //if(opr->msgDebug == devid)
    //Debug(console,"rcv%2d> COA Dump ... OK \n", devid+1);
#endif
        
    return (dataSize);
    
}


/***************************************************
    FUNCTION : controlAssign()
*   include TH,AC
***************************************************/
int     controlAssign(int fid, int devid)
{
    //int     ioid;
    int     rcvCount, dataPos, dataSize;
    int     cntrState;
    char    buffer[128];
    //float   *fptr1, *fptr2;
    //int     sign, exp, mant;
    //int     index1, index2, point;
            
    DNP_APP_FRAME   *rcvAppDnp;
    DNP_DEVICE      *dnp;
    
    dnp = (DNP_DEVICE *) dnpcfg[devid];
    //rcvAppDnp = (DNP_APP_FRAME *) &rcvAppFrame[fid];
    rcvAppDnp = (DNP_APP_FRAME *) &dnp->rcvAppFrame;
    rcvCount  = rcvAppDnp->rcvSize;
    dataPos   = dnp->rcvDataPos;
    dataSize  = 12;

#if 0   
    Debug(console,"rcv%2d> control  ... %d\n", devid+1, dataSize);
    DumpBuff(console,"STS: ", &rcvAppDnp->userData[dataPos], dataSize); 
#endif
   
    if(dnp->rcvVar == 1)    /* Binary Input */   
    {
        if(dnp->selectReq)
        {
            /* 제어정보 - Response Status */
            cntrState = rcvAppDnp->userData[dataPos + 11];
            
            if(opr->soeDebug)
            Debug(console,"rcv%2d> select OK ...(%2d)\n", devid+1, cntrState);
            dnp->selectReq  = RESET;
            dnp->operateReq = SET;
            
            /* ---------------------------------------- */
            /* 계전기 제어이력 : SELECT                 */
            /* ---------------------------------------- */
            logHistoryQue1( opr, hque, EVENT_MOD_SELECT, devid, dnp->cntPoint, dnp->cntTcf, cntrState);       // CONSOLE - 제어 이벤트     
            
            /* -------------------------------- */
            /* LOG File 저장                    */
            /* -------------------------------- */
            sprintf(buffer, "--> DNP> Select OK : Dev=%d, PT=%d, TCF[TRIP=0x80, CLOSE=0x40]= %2x... ", devid+1, dnp->cntPoint+1, dnp->cntTcf);
            LogFile_MPU (opr, EVENT_MOD_SELECT, buffer, strlen(buffer));    
        }
        else if(dnp->operateReq)
        {
            /* 제어정보 - Response Status */
            cntrState = rcvAppDnp->userData[dataPos + 11];
            
            if(opr->soeDebug)
            Debug(console,"rcv%2d> operate OK ...(%2d)\n", devid+1, cntrState);
            
            dnp->selectReq  = RESET;
            dnp->operateReq = RESET;
            
            /* ---------------------------------------- */
            /* 계전기 제어이력 : SELECT                 */
            /* ---------------------------------------- */
            logHistoryQue1( opr, hque, EVENT_MOD_OPERATE, devid, dnp->cntPoint, dnp->cntTcf, cntrState);       // CONSOLE - 제어 이벤트 
            
            /* -------------------------------- */
            /* LOG File 저장                    */
            /* -------------------------------- */
            sprintf(buffer, "--> DNP> Operate OK : Dev=%d, PT=%d, TCF[TRIP=0x80, CLOSE=0x40]= %2x... ", devid+1, dnp->cntPoint+1, dnp->cntTcf);
            LogFile_MPU (opr, EVENT_MOD_OPERATE, buffer, strlen(buffer));    
            
            dnp->cntPoint   = 0;
            dnp->cntTcf     = 0;
            dnp->cntrType   = 0;
            
            /* ------------------------------------------ */
            /* 제어명령시 Device 고정...(제어후 상태취득) */
            /* ------------------------------------------ */
            opr->cntrDevId [fid]  = devid;
            opr->cntrComTick[fid] = 0;
            opr->cntrFlag[fid]    = SET;
            //opr->armFlag[fid]     = SET;
            
        }        
    }        
    
    return(dataSize);            
}

/***************************************************
    FUNCTION : checkDataLink()
*   include TH,AC
***************************************************/
int     rcvHandlerDNP( int mode, int scanid, int devid)
{
    int     rcvCount;
    int     dataPos;
    int     objDataCount;
        
    //DL_FRAME        *rcvData;
    DNP_APP_FRAME   *rcvAppDnp;
    DNP_DEVICE      *dnp;

    dnp = (DNP_DEVICE *) dnpcfg[devid];
    rcvAppDnp = (DNP_APP_FRAME *) &dnp->rcvAppFrame;
    rcvCount  = rcvAppDnp->rcvSize;

    /* ------------------------------------ */
    /* 수신부 IIN Check...                  */
    /* ------------------------------------ */
    if(rcvIINCheck(scanid, devid) ==  0)
    {
        return (0);
    }
    
    
    /* ------------------------------------ */
    /*  계전기 Confirm 요구시...            */
    /* ------------------------------------ */
    if(dnp->rcvAC & CONFIRM_BIT)    
    {
        /* ------------------------------------------------ */     
        /*  계전기 타입이 비츠로인 경우...                  */
        /*  UPDATE-2016.09.23 ... 수도권 경우 FRTU 동일지연 */
        /* ------------------------------------------------ */     
        //if((dnp->type == TYPE_VIPAM) || (dnp->type == TYPE_FRTU))     pause(100);          // 지연 응답...
        
        /* 계전기 포트번호 => 지연값 */
        pause(100);  
        if(dnp->reqAckDelay)
        {
            pause(dnp->reqAckDelay);
        }
        
        /* 2017년 1월 6일 ... SOE ACK 시 */ 
        sndACConfirm( mode, scanid, devid);
    }
    
    /* 송신 패킷에 대한 seq 증가 */
    if(rcvAppDnp->userData[1] == 0x81)         // 응답메세지...
    {       
        dnp->acSndSeqno = (dnp->acSndSeqno + 1) & 0x0f;
        dnp->acSndSeqno = (dnp->rcvACseq + 1) & 0x0f;  // 수신 
        
    }
    
    dataPos = 4;  /* AC & IIN area */             
    
    /* ----------------------------------- */
    /* check Multi-Obj 에 대한 처리...     */
    /* ----------------------------------- */
    while(dataPos < rcvCount)
    {
        dnp->rcvObj     = rcvAppDnp->userData[dataPos++];
        dnp->rcvVar     = rcvAppDnp->userData[dataPos++];
        dnp->rcvQcode   = (rcvAppDnp->userData[dataPos]) & 0x0f;
        dnp->rcvIndex   = (rcvAppDnp->userData[dataPos] >> 4) & 0x07;
        dataPos++;

        switch(dnp->rcvQcode)         /* data size check ... */
        {
            case 0:     /* Range: 8bit Start & Stop */
            case 3:     /* Range: 8bit Abs Start & Stop */    
                dnp->rcvStart   = rcvAppDnp->userData[dataPos++];
                dnp->rcvStop    = rcvAppDnp->userData[dataPos++];
                dnp->rcvDataPos = dataPos;
                break;
        
            case 1:     /* Range: 16bit Start & Stop */
            case 4:     /* Range: 16bit Abs Start & Stop */    
                dnp->rcvStart   = (rcvAppDnp->userData[dataPos+0] & 0x00ff) 
                                  + ((rcvAppDnp->userData[dataPos+1] << 8) & 0xff00);
                dataPos += 2;
                                                  
                dnp->rcvStop    = (rcvAppDnp->userData[dataPos+0] & 0x00ff) 
                                  + ((rcvAppDnp->userData[dataPos+1] << 8) & 0xff00);
                dataPos += 2;                  
                dnp->rcvDataPos = dataPos;    
                break;
        
            case 2:     /* Range: 32bit Start & Stop */
            case 5:     /* Range: 32bit Abs Start & Stop */    
                dnp->rcvStart   = (rcvAppDnp->userData[dataPos+0] & 0x000000ff) 
                                  + ((rcvAppDnp->userData[dataPos+1] << 8) & 0x0000ff00) 
                                  + ((rcvAppDnp->userData[dataPos+2] << 16) & 0x00ff0000)
                                  + ((rcvAppDnp->userData[dataPos+3] << 24) & 0xff000000);
                dataPos += 4;
                
                dnp->rcvStop    = (rcvAppDnp->userData[dataPos+0] & 0x000000ff)
                                  + ((rcvAppDnp->userData[dataPos+1] << 8) & 0x0000ff00) 
                                  + ((rcvAppDnp->userData[dataPos+2] << 16) & 0x00ff0000)
                                  + ((rcvAppDnp->userData[dataPos+3] << 24) & 0xff000000);
                dataPos += 4;
                                                  
                dnp->rcvDataPos = dataPos;                                                       
                break;
        
            case 6:     /* No Range */
                dnp->rcvStart   = 0;
                dnp->rcvStop    = 0;
                dnp->rcvDataPos = dataPos;
                break;

            case 7:     /* 8Bit Single Field quantity */             
                dnp->rcvStart   = 0;
                dnp->rcvStop    = 0;
                dnp->rcvQnum    = rcvAppDnp->userData[dataPos++];   /* quentity number */
                dnp->rcvDataPos = dataPos;
                break;

            case 8:     /* 16Bit Single Field quantity */             
                dnp->rcvStart   = 0;
                dnp->rcvStop    = 0;
                dnp->rcvQnum    = (rcvAppDnp->userData[dataPos+0] & 0x00ff) 
                                  + ((rcvAppDnp->userData[dataPos+1] << 8) & 0xff00);   
                dataPos += 2;                                  
                dnp->rcvDataPos = dataPos;
                break;

            case 9:     /* 32Bit Single Field quantity */             
                dnp->rcvStart   = 0;
                dnp->rcvStop    = 0;
                dnp->rcvQnum    = (rcvAppDnp->userData[dataPos] & 0x000000ff) + 
                                  ((rcvAppDnp->userData[dataPos+1] << 8)  & 0x0000ff00) +
                                  ((rcvAppDnp->userData[dataPos+2] << 16) & 0x00ff0000) +
                                  ((rcvAppDnp->userData[dataPos+3] << 24) & 0xff000000);   
                dataPos += 4;                                  
                dnp->rcvDataPos = dataPos;
                break;
                                    
            case 11:
                break;        
    
            default:
                break;
        }/*switch */
            
        if(opr->msgDebug == devid)
        Debug(console,"rcv%2d> -----> Response seq[%02x] OBJ=%2d, VAR=%2d, Index=%2d, QCode=%2d...Start=%2d, Stop=%2d, DataPos=%2d\n", 
            devid+1, dnp->rcvACseq, dnp->rcvObj, dnp->rcvVar, dnp->rcvIndex, dnp->rcvQcode, 
            dnp->rcvStart, dnp->rcvStop, dnp->rcvDataPos);
            
        switch(dnp->rcvObj)
        {
        case OBJ_BINARY_INPUT:     /* Binary Input */
            objDataCount = stsDumpAssign(scanid, devid);
            dataPos = dataPos + objDataCount;
            break;
            
        case OBJ_BINARY_CHANGE:     /* Binary Input with Time */
            objDataCount = soeDumpAssign(scanid, devid);
            dataPos = dataPos + objDataCount;
            break;            
            
        case OBJ_CONTROL_RELAY:    /* Binary Output */
            objDataCount = controlAssign(scanid, devid);
            dataPos = dataPos + objDataCount;
            break;

        case OBJ_BINARY_COUNTER:    /* Binary Counter */
            objDataCount = countDumpAssign(scanid, devid);
            dataPos = dataPos + objDataCount;
            break;
                                                 
        case OBJ_ANALOG_INPUT:    /* AI Input */
            objDataCount = anaDumpAssign(scanid, devid);
            dataPos = dataPos + objDataCount;
            break;
            
        case OBJ_ANALOG_CHANGE:     /* Analog Change Input with Time */
            objDataCount = coaDumpAssign(scanid, devid);
            dataPos = dataPos + objDataCount;
            return (0);
            break;            
            
        case OBJ_ANALOG_OUTPUT:    /* AO Input */
            objDataCount = anaOutDumpAssign(scanid, devid);
            dataPos = dataPos + objDataCount;
            return (0);
            break;

        case OBJ_AOUT_BLOCK:    /* AO Output */
            if(dnp->anaVOutSelect == SET)
            {
                dnp->anaVOutSelect = RESET;
                dnp->anaVOutOpr    = SET;
                
                if(opr->soeDebug)
                Debug(console,"rcv%2d> AO Control select OK ...\n", devid+1);
            }
            else if(dnp->anaVOutOpr == SET) 
            {   
                dnp->anaVOutSelect = RESET;
                dnp->anaVOutOpr    = RESET;
                
                if(opr->soeDebug)
                Debug(console,"rcv%2d> AO Control operate OK ...\n", devid+1);
            }               
            else
            {
                dnp->anaVOutSelect = RESET;
                dnp->anaVOutOpr    = RESET;
            }
            dataPos = rcvCount;
            return (0);
            break;            
            
        case OBJ_TIME_AND_DATE:    /* Analog Input */
            dnp->timeSyncReq = RESET;   
            
            Debug(console,"rcv%2d> Time sync OK... !\n", devid+1);
            dataPos = rcvCount;
            break;            

        default:
            dnp->sndFlag    = RESET;
            return (0);
            break; 
        }                       

    } /* while */

    dnp->sndFlag    = RESET;
    return (0);
}

/***************************************************
    FUNCTION : checkDataLink()
*   include TH,AC
***************************************************/
int     checkDataLink(int mode, int scanid, int devid, DNP_DATA_LINK *dnpData)
{
    //int     appControl;
    int     rcvCount;
    int     rcvPos;
    byte    thHeader, seqno;
    byte    *srcPtr;
    //byte    *buf;
    DL_FRAME        *rcvData;
    DNP_APP_FRAME   *rcvAppDnp;
    DNP_DEVICE      *dnp;

    dnp = (DNP_DEVICE *) dnpcfg[devid];
        
    rcvData = (DL_FRAME *) &dnpData->rcvFrame;
    rcvAppDnp = (DNP_APP_FRAME *) &dnp->rcvAppFrame;
    //buf = (byte *) &dnpData->rcvFrame;
    
    /* ------------------------------------ */
    /*  계전기 응답메세지 해석...           */
    /* ------------------------------------ */
    thHeader = rcvData->userData[0];
    seqno    = thHeader & 0x3f;
    
    /* ------------------------------------ */
    /* check APPF - FIR set ... 			*/
    /* ------------------------------------ */
    if(thHeader & TH_FIR_BIT) // transport header 
    {
#if 0       
        //if(opr->msgDebug == devid)
        Debug(console,"rcv%2d> [FIR set] ... seqno=%d\n", devid+1, seqno);
#endif
        rcvAppDnp->rcvPos   = 0;
        rcvAppDnp->rcvSize  = 0;
        
    }

    rcvPos  = rcvAppDnp->rcvPos;
    rcvCount= dnpData->rcvCount - 10 - 1;       /* header, TH */
    srcPtr  = (byte *) &rcvData->userData[1]; /* exclude th-header */
    
    /* check App Buffer size: max 2048 */
    if((rcvPos + rcvCount) >= MAX_APP_BUFFER)
    {
        if(opr->msgDebug == devid)
        Debug(console,"rcv%2d> *** APPF overflow .... %d  !\n", devid+1,rcvPos+rcvCount);
        rcvAppDnp->rcvPos = rcvAppDnp->rcvPos - rcvCount;
        rcvPos  = rcvAppDnp->rcvPos - rcvCount;
    }

    /* store APPF - Data ... */
    memcpy(&rcvAppDnp->userData[rcvPos], srcPtr, rcvCount);
    rcvAppDnp->rcvPos = rcvPos + rcvCount;
    
    /* ------------------------------------ */
    /* check APPF - FIN set ... 			*/
    /* ------------------------------------ */
    if(thHeader & TH_FIN_BIT)
    {
        rcvAppDnp->rcvSize = rcvAppDnp->rcvPos;

#if 0
        //if(opr->msgDebug == devid)
        Debug(console,"rcv%2d> [FIN set] ... seqno=%d, rcvSize=%d\n", devid+1, seqno, rcvAppDnp->rcvSize);
#endif

        if(rcvAppDnp->userData[1] == 0)
        {
            if(opr->msgDebug == devid)
            Debug(console,"rcv%2d> AC Confirm ...!\n", devid+1);
            return (1);
        }
        else if(rcvAppDnp->userData[1] == 0x81)         // 응답메세지...
        {
        	dnp->rcvTH 		=  thHeader;
        	dnp->thSndSeqno = (thHeader + 1) & 0x3f;
        	
        	
            rcvHandlerDNP( mode, scanid, devid);
            
            return (1);
        }
        //else if(rcvAppDnp->userData[1] == 0x82)         // 2018.03.14... 응답메세지...UNSolite...confirm 요청시... 
        //{
        //    rcvHandlerDNP( mode, scanid, devid);
        //    return (1);
        //}

    }
    
    return (0);
}

/**********************************

Func : 수신한 DNP Link Layer data 확인  
return : 유효한 FC 반환.  invalid면 -1 

2025-08-28 오후 5:53:42 : check retVal  , -1 이면 write 실패,..이유불문하고 소켓닫고 다시하자. 
아..serial에소 쓰이나...

TO-DO  default가 뭐지...일단 error 처리하자.

review : 2025-10-24 오후 12:41:23 

***********************************/
int     chkRcvFrame ( int mode, int scanid, int devid, DNP_DATA_LINK *dnpData)
{
    int retVal ; // 2025-08-28 오후 5:55:09 hkkim
    
    byte    control, function;
    //char    buffer[128];
    
    DL_FRAME        *rcvData;
    DNP_APP_FRAME   *sndAppDnp;
    DNP_DEVICE      *dnp;

    dnp = (DNP_DEVICE *) dnpcfg[devid];
    
    rcvData = (DL_FRAME *) &dnpData->rcvFrame;

    /* ---------------------------------------- */
    /* 수신 Data-Link Function code             */
    /* ---------------------------------------- */
    control  = rcvData->control;
    function = control & 0x0f;

    if(control & PRM_BIT) 
    {
        switch(function)
        {
        case FUNC_RESET_LINK:
        	// hkkim
            if ( (retVal = sendConfirm( mode, scanid, devid, RCV_ACK)) < 0)
            		return -1; 

            dnp->rcvLinkSts = SET;
            dnp->rcvFcvBit = FCV_BIT;
            dnp->rcvFcbBit = FCB_BIT;
            if(opr->msgDebug == devid)  Debug(console,"rcv%2d> RESET LINK rcv ... !\n", devid+1);
            break;

        case FUNC_RESET_USER:
        	// hkkim  sendConfirm fail 시 retunr -1 추가.
            if ( (retVal = sendConfirm( mode, scanid, devid, RCV_ACK)) < 0)
            		return -1;
            if(opr->msgDebug == devid)  Debug(console,"rcv%2d> RESET USER rcv ... !\n", devid+1);
            break;

        case FUNC_TEST:
        	// hkkim  sendConfirm fail 시 retunr -1 추가.
            if ( (retVal = sendConfirm( mode, scanid, devid, RCV_ACK)) < 0)
            		return -1;
            if(opr->msgDebug == devid)  Debug(console,"rcv%2d> TEST rcv ... !\n", devid+1);
            break;

        case FUNC_USER_DATA:
        	// hkkim  sendConfirm fail 시 retunr -1 추가.
            if ( (retVal = sendConfirm( mode, scanid, devid, RCV_ACK)) < 0)
            		return -1;
            if(opr->msgDebug == devid)  Debug(console,"rcv%2d> USER DATA rcv ... !\n", devid+1);
            break;

        case FUNC_UNCONFIRM:
            if(opr->msgDebug == devid)  Debug(console,"rcv%2d> UNCONFIRM rcv ... !\n", devid+1);
            break;

        case FUNC_REQUEST:
            if(opr->msgDebug == devid)  Debug(console,"rcv%2d> REQUEST rcv ... !\n", devid+1);
            break;
        default:
            if(opr->msgDebug == devid)  Debug(console,"rcv%2d> default func ... %d!\n", devid+1, function);
			// hkkim
            function =-1;
            break;
        }
    }
    else //보낸 놈이 Slave   난 Master
    {
        switch(function)
        {
        case RCV_ACK:
            if(dnp->sndLinkSts == RESET)
            {
                sndAppDnp = (DNP_APP_FRAME *) &dnp->sndAppFrame;
                sndAppDnp->sndSeqno = 1;

                dnp->sndLinkSts = SET;
                dnp->sndFcvBit = FCV_BIT;
                dnp->sndFcbBit = FCB_BIT;
                
                dnp->acSndSeqno = 0;
                dnp->ACseqFail  = 0;
                dnp->statusDump = SET;

                if(opr->msgDebug == devid)  
                Debug(console,"rcv%2d> SCAN RESET-ACK rcv ... !\n", devid+1);

#if 0                
                /* -------------------------------------------- */
                /* LOG File 저장                                */
                /* 2018.03.02 : RESET-LINK 메세지 삭제...           */
                /* -------------------------------------------- */
                sprintf(buffer, "rcv%2d> DNP - SCAN RESET-ACK rcv ... !\n", devid+1);
                LogFile_MPU ( opr, NOT_DEFINED, buffer, strlen(buffer));
#endif                
            }
            else
            {
                if(opr->msgDebug == devid)  Debug(console,"rcv%2d> ACK rcv ... !\n", devid+1);
            }
            break;

        case RCV_NACK:
            if(opr->msgDebug == devid)  Debug(console,"rcv%2d> NACK rcv ... !\n", devid+1);
            break;

        case RCV_RESPONSE: // protocol 상 LINK 상태 LINK-STATUS 
            if(opr->msgDebug == devid)  Debug(console,"rcv%2d> RESPONSE rcv ... !\n", devid+1);
            break;
            
        default:
        	// 2025-10-24 오후 12:41:36  change -1 to NOT_DEFINED_LINK_FUNCTION 
            function = NOT_DEFINED_LINK_FUNCTION ; /* 지원되지 않는 함수 -1*/
            break;
        }
    }
//    printf("%s : return %d\r\n",__FUNCTION__,function);
    return (function);
}

/***************************************************
*   FUNCTION : readDataLink()
*   receive DATA-LINK with CRC
***************************************************/
int readDataLink( int scanid, int devid, DNP_DATA_LINK *dnpData)
{
    int     i,size;
    int     rxcnt;
    //int     retVal;
    int    length;
    int     frameCnt,left;
    int     dataLen;
    int     desAddr, srcAddr;
    byte    *rxbuf;
    word    crc;
    word    timeout;
    DL_FRAME    *rcvData;
    DNP_DEVICE  *dnp;

    dnp = (DNP_DEVICE *) dnpcfg[devid];
    
    timeout = 0;
    rcvData = (DL_FRAME *) &dnpData->rcvFrame;

    rxcnt = 0;
    rxbuf = (byte *) &rcvData->start1;
    
    /* ---------------------------- */
    /*  receive STX(0x0564)         */
    /* ---------------------------- */
RCV_START:
    do {
        if(dnpChanRead(scanid, devid, &rxbuf[0], 1) == 0) return (0);
        if(++timeout > 50)  return (0);
    } while(rxbuf[0] != 0x05);


    if(dnpChanRead(scanid, devid, &rxbuf[1], 1) == 0) return (0);
    if(rxbuf[1] != 0x64)    goto    RCV_START;
    rxcnt = 2;

    /* read Header */
    if(dnpChanRead(scanid, devid, &rxbuf[rxcnt], 8) != 8)
    {
        return(0);
    }
    
    if(B013_ckdnpcrc(&rxbuf[0], 10) != 0)
    {
        crc = B013_mkdnpcrc( &rxbuf[0], 8);
        if((opr->dnpDebug == devid) || (opr->scanDebug == scanid))  
        {
            Debug(console,"rcv%2d> *header - crc error cal=%04x, rcv=%02x-%02x!\n",
                devid+1, crc, rxbuf[8],rxbuf[9]);
            // 2026-10-21 오전 10:04:26
            // DumpDNP_rcv(console, "CRC ", &rxbuf[0], 10);                
            DumpDNP_rcv(console, "rcv-CRC:", &rxbuf[0], 10);
        }
        return (0);
    }

    /* ------------------------ */
    /*  check DNP-ADDR ...      */
    /* ------------------------ */
    desAddr = rcvData->desAddr[0] + (rcvData->desAddr[1]*256);
    srcAddr = rcvData->srcAddr[0] + (rcvData->srcAddr[1]*256);
        
    if(desAddr == 0xffff)
    {
        if((opr->dnpDebug == devid) || (opr->scanDebug == scanid))   
        Debug(console,"rcv%2d> Broadcast Addr .. !\n", devid+1);
        dnpData->broadCast = RESET;
        return (0);
    }
    else
    {
        dnpData->broadCast = RESET;

        if(desAddr != dnp->hostid)
        {
            if((opr->dnpDebug == devid) || (opr->scanDebug == scanid))
            {
                Debug(console,"rcv%2d> *1 des Addr failed : rcv %4x, host %4x.. !\n", devid+1, desAddr, dnp->hostid);
                DumpDNP_rcv(console, "*rcv: ", &rxbuf[0], 10);
            }
            return (0);
        }
        else if(srcAddr != dnp->ioid)
        {
            if((opr->dnpDebug == devid) || (opr->scanDebug == scanid)) 
            {
                Debug(console,"rcv%2d> *1 src Addr failed : rcv %4x, fru %4x.. !\n", devid+1, srcAddr, dnp->ioid);
                DumpDNP_rcv(console, "*rcv: ", &rxbuf[0], 10);
            }
            return (0);
        }
    }

//NEXT_PROCESS:    
    rxcnt  = 10;                /* user data */
    length = rxbuf[2] - 5;      /* except control, src, des addr (5byte) */

    /* ------------------------------------ */
    /*  1) if CONFIRM frame received ...    */
    /*  2) if reset LINK frame received ... */
    /* ------------------------------------ */
    if(length == 0)     
    {
        if((opr->dnpDebug == devid) || (opr->scanDebug == scanid))
        Debug(console,"rcv%2d> no Data Length = %d\n", devid+1, length);
        return (rxcnt);
    }
    else if(length > 255)     
    {
        //if((opr->dnpDebug == devid) || (opr->scanDebug == scanid))
        Debug(console,"rcv%2d> *DNP rcv - Invalid Length = %d\n", devid+1, length);
        return (0);
    }
    
    frameCnt = (length / FRAME_LENGTH);
    left     = (length % FRAME_LENGTH);
    if(left)    frameCnt = frameCnt + 1;

    /* Multi-user data block : execpt CRC ... */
    for(i = 0; i < frameCnt; i++)
    {
        if(length <= 0 )    break;

        if(length >= FRAME_LENGTH)  dataLen = FRAME_LENGTH;
        else                        dataLen = length;

        /* read USER data  */
        //if((size = asyncRead( opr, tp, &rxbuf[rxcnt], dataLen+2)) != (dataLen+2))
        if((size = dnpChanRead(scanid, devid, &rxbuf[rxcnt], dataLen+2)) != (dataLen+2))
        {
            if((opr->dnpDebug == devid) || (opr->scanDebug == scanid))
            Debug(console,"rcv%2d> 1. rcv Time out  ... req=%d, rcv=%d \n", devid+1, dataLen+2,size);
            return(0);
        }

        if(B013_ckdnpcrc(&rxbuf[rxcnt], size) != 0)
        {
            crc = B013_mkdnpcrc( &rxbuf[rxcnt], dataLen);
            if((opr->dnpDebug == devid) || (opr->scanDebug == scanid)) 
            Debug(console,"rcv%2d> *user data [%d] - crc error cal=%04x, rcv=%02x-%02x!\n",
                devid+1, i+1, crc, rxbuf[rxcnt + dataLen], rxbuf[rxcnt + dataLen + 1]);
            // 2026-10-21 오전 10:04:26
            DumpDNP_rcv(console, "CRC ",&rxbuf[rxcnt], size);  
                
            rxcnt = rxcnt + dataLen;
            return (0);
        }

        rxcnt  = rxcnt + dataLen;   /* include crc-data */
        length = length - dataLen;
    }

    return (rxcnt);

}

/***************************************************
*   FUNCTION : sendDataLink()
*			  최대 DNP DataLink SIZE 만큼 보냄 ( loop 없이 1회로)
*   send DataLink Layer : make send frame & send
*  return : 보낸 data수
* 2025-08-28 오후 5:59:58  mode=COM_TCP_IP에서 write 실패처리추가
***************************************************/
int sendDataLink(int mode, int scanid, int devid, byte thHeader, byte *src, int length)
{
    int     i,j;
    //int     opcode;
    int     frameCnt;
    int     dataLen,bufPos;
    int     txcnt, rxcnt;
    int     retryCnt;
    int     rxVal;
    byte    *txbuf;
    byte    control;
    word    crc;
    DL_FRAME        *sndData;
    DNP_DATA_LINK   *dnpData;
    DNP_DEVICE  *dnp;     
    

    
    dnp = (DNP_DEVICE *) dnpcfg[devid];
    
    //dnpData  = (DNP_DATA_LINK *) &dataLinkFrame[fid];
    dnpData  = (DNP_DATA_LINK *) &dnp->dataLinkFrame;
    sndData  = (DL_FRAME *) &dnpData->sndFrame;

    control  = (dnp->sndFcbBit + dnp->sndFcvBit) + 0xc0 + FUNC_USER_DATA;
    control  = (dnp->sndFcbBit + dnp->sndFcvBit) + 0xc0 + FUNC_UNCONFIRM; 
    control  = 0xc0 + FUNC_UNCONFIRM;    

    txbuf = (byte *) &sndData->start1;

    /* ------------------------ */
    /*  Make HEADER frame ...   */
    /* ------------------------ */
    txcnt = 0;
    txbuf[txcnt++] = 0x05;
    txbuf[txcnt++] = 0x64;
    txbuf[txcnt++] = length + 5 + 1;        /* include header, TH,AC */
    txbuf[txcnt++] = control;               /* control */
    txbuf[txcnt++] = dnp->ioid & 0xff;
    txbuf[txcnt++] = (dnp->ioid >> 8) & 0xff;
    txbuf[txcnt++] = dnp->hostid & 0xff;
    txbuf[txcnt++] = (dnp->hostid >> 8) & 0xff;
    crc = B013_mkdnpcrc(txbuf, txcnt);

    txbuf[txcnt++] = crc & 0xff;
    txbuf[txcnt++] = (crc >> 8) & 0xff;

    bufPos   = 0;
    frameCnt = (length / FRAME_LENGTH) + 1;
	
	//printf("%s : length %d,  frameCnt %d\r\n",__FUNCTION__,length, frameCnt);
    /* -------------------------------------------- */
    /*  Make USER-DATA Block Multi send Block ....  */
    /* -------------------------------------------- */
    for(i=0; i< frameCnt; i++)
    {
        if(length <= 0) break;
  
        if(i==0)
        {
            if(length >= FRAME_LENGTH)  dataLen = FRAME_LENGTH-1;
            else    dataLen = length;

            txbuf[txcnt+ 0] = thHeader;
            for(j=0; j < dataLen; j++)  txbuf[txcnt+1+j] = src[bufPos++];

            length = length - dataLen;

            crc = B013_mkdnpcrc( &txbuf[txcnt], dataLen+1);
            txcnt = txcnt + dataLen + 1;
            txbuf[txcnt++] = crc & 0xff;
            txbuf[txcnt++] = (crc >> 8) & 0xff;
        }
        else
        {
            if(length >= FRAME_LENGTH)  dataLen = FRAME_LENGTH;
            else    dataLen = length;

            for(j=0; j < dataLen; j++)  txbuf[txcnt+j] = src[bufPos++];

            length = length - dataLen;

            crc = B013_mkdnpcrc( &txbuf[txcnt], dataLen);
            txcnt = txcnt + dataLen;
            txbuf[txcnt++] = crc & 0xff;
            txbuf[txcnt++] = (crc >> 8) & 0xff;
        }
    }

    retryCnt = 5;
    control  = control & 0x0f;

    if((opr->dnpDebug == devid) || (opr->scanDebug == scanid))   
    		DumpDNP_snd(console,"TXD: ", txbuf, txcnt);


    /* ------------------------------------------------ */
    /*  DNP APP Write... DataLink Send ...              */
    /* ------------------------------------------------ */
    for(i=0; i< retryCnt; i++)
    {
        if(mode == COM_TCPIP)
        {
//            if((rxVal = tkWriteTCP( dnp->SocketFd , txbuf, txcnt, 1000)) > 0 )
// 2025-08-28 오후 6:00:29 hkkim
            if((rxVal = tk_net_writen( dnp->SocketFd , txbuf, txcnt, 1000)) ==  txcnt  )
    	    {
		//	   printf("%s :tk_net_writen %d\r\n",__FUNCTION__,rxVal);    	        
    	        /* APP-Data 의 Confirm 여부에 따라 처리.. */
    	        // 위에서 UNCONFRIM으로 고정되어 있다.
    	        if(control == FUNC_UNCONFIRM)
                {
                	// TO-DO  unconfirm에서는  fcb가 변하면 안되는데.. 
                    
                    //if(dnp->sndFcbBit)  dnp->sndFcbBit = 0;
                    //else                dnp->sndFcbBit = FCB_BIT;
                    return (rxVal); // hkkim 정상이면 txCnt와 같다.
                }
                else 
                {
                    if((rxcnt = readDataLink_TCP( scanid, devid, dnpData)) > 0)
                    {
                        dnp->timeOutTick  = 0;
                                    
                        dnpData->rcvCount = rxcnt;
                        if((opr->dnpDebug == devid) || (opr->scanDebug == scanid))  
                        DumpDNP_rcv(console, "RXD#:", &dnpData->rcvFrame, rxcnt);
                    
                        /* check Function Code ... */
                        if(chkRcvFrame(mode, scanid, devid, dnpData) == RCV_ACK)
                        {
                            if(dnp->sndFcbBit)  dnp->sndFcbBit = 0;
                            else                dnp->sndFcbBit = FCB_BIT;
							// hkkim                            
                            return (rxVal);
                        }
                        // LINK LAYER 수신실페
                        else return -1; // 2025-08-28 오후 6:02:31 hkkim
                    }
                    else return -1 ;// 2025-08-28 오후 6:02:43 hkkim
                }
                
                return -1;
		    } 
		    else 	
		    {
		    	printf("sendDataLink : tk_net_writen failed %d\r\n",rxVal);
		    		    return -1 ;     
			}
        }
        else // TODO 읽단 VME는 나중에...
        {      
            if(dnpChanWrite( scanid, devid, txbuf, txcnt) > 0)
            {
                /* APP-Data 의 Confirm 여부에 따라 처리.. */
                if(control == FUNC_UNCONFIRM)
                {
                    if(dnp->sndFcbBit)  dnp->sndFcbBit = 0;
                    else                dnp->sndFcbBit = FCB_BIT;
                    return (txcnt);
                }
                else
                {
                    if((rxcnt = readDataLink( scanid, devid, dnpData)) > 0)
                    {
                        dnp->timeOutTick  = 0;
                                    
                        dnpData->rcvCount = rxcnt;
                        if((opr->dnpDebug == devid) || (opr->scanDebug == scanid))  
                        DumpDNP_rcv(console, "RXD#:", &dnpData->rcvFrame, rxcnt);
                    
                        /* check Function Code ... */
                        if(chkRcvFrame(mode, scanid, devid, dnpData) == RCV_ACK)
                        {
                            if(dnp->sndFcbBit)  dnp->sndFcbBit = 0;
                            else                dnp->sndFcbBit = FCB_BIT;
                            return (txcnt);
                        }
                    }
                }
            }                
        }

    }
	
	printf("%s : never be here\r\n",__FUNCTION__);
    return (NOK);
}


/***************************************************
*   FUNCTION : sendUserFrame()
*   send User Layer : make send frame & send
*   return : 정상적으로 송신한 경우        ret  = byte 수
			        송신 fail 또는 data=0  ret=-1 
*  serial/tcp 공통으로 사용			        
***************************************************/
int sendAppFrame(int mode, int scanid, int devid, DNP_APP_FRAME *sndAppDnp)
{
    int     i;

    int     length;
    //int     opcode;
    int     retVal;
    int     frameCnt, left;
    int     sndPos;
    int     dataLen;
    
    byte    control,thHeader;
    DNP_DEVICE  *dnp;
    
    dnp = (DNP_DEVICE *) dnpcfg[devid];
    
    /* ---------------------------- */
    /*  check Multi frame ...       */
    /* ---------------------------- */
    length   = sndAppDnp->sndCount;           /* App user data length */
    frameCnt = (length / APP_SEND_LEN);
    left     = (length % APP_SEND_LEN);

    if(left) frameCnt = frameCnt + 1;

    sndAppDnp->sndFrame    = frameCnt;
    sndPos = 0;

	//printf("%s : length %d,  frameCnt %d\r\n",__FUNCTION__,length, frameCnt);

    if(length <= 0)
    {
    	// hkkim
        if(opr->msgDebug == devid)
      		  Debug(console,"snd%2d> App send size (%d) error .... !\n", devid+1,length);
        return -1 ;
    }

    control  = 0;
    thHeader = 0;
    sndPos   = 0;

    /* -------------------------------------------------------- */
    /*  APP 당 최대 송신데이터(249byte) 단위로 구분하여 전송... */
    /* -------------------------------------------------------- */
    for(i=0; i< frameCnt; i++)
    {
        if(length <= 0) break;

        if(frameCnt == 1)               dataLen = length;
        else if(length >= APP_SEND_LEN) dataLen = APP_SEND_LEN;
        else                            dataLen = length;

        /* check Frame-Control Bit */
        if(i == 0)              control = TH_FIR_BIT;
        else                    control = 0;
        if(i == (frameCnt - 1)) control = control | TH_FIN_BIT;

        thHeader  = control + dnp->thSndSeqno;
       // printf("sendDataLink Frame[%d]\r\n",i+1) ;
        retVal = sendDataLink( mode, scanid, devid, thHeader, &sndAppDnp->userData[sndPos], dataLen);

		//  OK->dataLen
        //if(retVal == dataLen)
        if ( retVal > 0 )
        {
            dnp->thSndSeqno = (dnp->thSndSeqno + 1) & 0x3f;
            if(opr->msgDebug == devid)
           	 Debug(console,"snd%2d> DL[%d/%d] frame send OK.... !\n", devid+1, i+1, frameCnt  );
            //Debug(console,"snd%2d> DL[%d/%d] frame send OK.... !\n", devid+1, i+1, frameCnt+1  );

        }
        else
        {
            if(opr->msgDebug == devid)
          	  Debug(console,"snd%2d> DL[%d/%d] frame send failed....req[%d] got[%d] !\n", 
            		devid+1, i+1, frameCnt ,dataLen ,retVal );
            //Debug(console,"snd%2d> *** DL[%d] frame send failed ... !\n", devid+1, i);
			//printf("%s : out  %d\r\n", __FUNCTION__, -1);
            return (-1);
        }

        sndPos = sndPos + dataLen;
        length = length - dataLen;
        pause(10);
    }
	//printf("%s : out  %d\r\n",  __FUNCTION__, sndAppDnp->sndCount);
    /* -------------------------- */
    /* APP 송신 데이터 .... 송신. */
    /* -------------------------- */
    dnp->sndFlag    = SET;
    dnp->timeOutTick= 0;
    
    return (sndAppDnp->sndCount );
}


