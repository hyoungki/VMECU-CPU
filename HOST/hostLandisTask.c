
#include	"localLib.h"
#include    "external.h"

extern  int     hostVMERead(int hostid, int chid, byte *rxbuf);
extern  int     hostChanRead(int hostid, int chid, byte *rxbuf, int reqCount);
extern  int     hostChanWrite(int hostid, int chid, int socketID, byte *sndbuf, int sndCount);
extern  int     vmeHostInitial(int hostid, int chid);
extern  int     vmeHostChanReset(int hostid, int chid);

extern  int     getSTSLandis();
extern  void    gen_ana_force(int hostid, byte *rxbuf);
extern  void    gen_adc_ref(int hostid, byte *rxbuf);
extern  void    gen_ind_cos(int hostid, byte *rxbuf);
extern  void    gen_ind_force(int hostid, byte *rxbuf);
extern  void    gen_soe_log(int hostid, byte *rxbuf);
extern  int     gen_sbo_sel(int hostid, byte *rxbuf);
extern  int     gen_sbo_opr(int hostid, byte *rxbuf);
extern  int     gen_sbo_exe( int hostid, byte *buf);
extern  void    gen_rtu_config(int hostid, byte *rxbuf);
extern  int     gen_time_sync( int hostid, byte *rxbuf);
extern  void    gen_time_bias(int hostid, byte *rxbuf);
extern  void    gen_firmware(int hostid, byte *rxbuf);


extern  TTY_DESC hostPort1;
extern  TTY_DESC hostPort2;
extern  TTY_DESC hostPort3;
extern  TTY_DESC hostPort4;
extern  TTY_DESC hostPort5;

extern  TASK_INFO	    *taskPtr;
extern  int             termExec;

extern  int             wdtHostFlag[MAX_HOST];

/****************************************************************
*   FUNCTION : analogInitLANDIS()
*   - analog value initiaize (landis: 0->2048)
****************************************************************/
void analogInitLANDIS()
{
	int i;
	HOST_ANABUF *hostB;
	
   /* 아날로그 예비포인트 0 clear */
    for(i=0 ; i< MAX_HOST_ANA_PT ; i++)
    {
    	hostB = (HOST_ANABUF *) hostAnaBuf[i];
    	hostB->pointData = 2048;
    	hostB->convData  = 2048;
    }
    	
}

/**
**  receive data with LRC
**/
int rcvWithLANDIS(int hostid, int chid, byte *rxbuf)
{
    short   reqsize,rcvsize;
    //short   index , count;
    short   rcvCount;
    //byte    data;
    word    rcvcrc,calcrc;

    HOST_DCB    *host;
    TTY_DESC    *tp;

    host = (HOST_DCB *) hostDCB[hostid];
    tp   = (TTY_DESC *) host->ttyPort[chid];

    rcvCount = 0;

    /*  check DCD on state */
    do {
        if(hostChanRead(hostid, chid, &rxbuf[0], 1) == 0) return (0);
    } while(rxbuf[0] != 0xff);

//START_MSG:
    /* ----------------------------------- */
    /*  search Header 2 byte (status:addr) */
    /* ----------------------------------- */
    //if(asyncRead( opr, tp, &rxbuf[0], 2) != 2) return(0);
    if(hostChanRead(hostid, chid, &rxbuf[0], 2) != 2) return (0);    
    if((rxbuf[0] & 0x6b) != 0 ) return (0);
    
    /* receive ADDR : 1 byte        */
    landis->broadCast = RESET;
    if(rxbuf[1] != landis->rtuid)
    {
        if(rxbuf[1] == 0)   landis->broadCast = SET;
        else                
        {
            if(opr->hostDebug == hostid)
            Debug(console,"l&g%02d> *RTU Address mismatch : rtu=%d, rcv=%d\n", hostid, landis->rtuid, rxbuf[1]);
            return 0;
        }     
    }
    
    rcvCount = 2;

    pause(20);
    
    /* ---------------------------- */
    /* check SHORT-MSG     : 2 Byte */
    /* ---------------------------- */
    if(rxbuf[0] & BIT_SHR)
    {
        pause(50);
        
        /* receive CRC - 2 byte     */
        //if(asyncRead( opr, tp, &rxbuf[2], 2) != 2)
        if(hostChanRead(hostid, chid, &rxbuf[2], 2) != 2)    
        {
            if(opr->hostDebug == hostid)
            Debug(console,"l&g%02d> *SHR : Tail not received !\n", hostid);
            return(0);
        }
        rcvCount = rcvCount + 2;   
        goto    CHECK_CRC;             
        //return (rcvCount);
    }
	
    /* ------------------------------------ */
    /* receive FC & length : 2 Byte         */
    /* ------------------------------------ */
    //if(asyncRead( opr, tp, &rxbuf[rcvCount], 2) != 2)
    if(hostChanRead(hostid, chid, &rxbuf[rcvCount], 2) != 2)        
    {
        if(opr->hostDebug == hostid)
        Debug(console,"l&g%02d> *rcv (FC & Length) ... time out !\n", hostid);
        return(0);
    }
    
    /* ------------------------------------ */
    /* check L-bit : Master msg all SET     */
    /* ------------------------------------ */
    if((rxbuf[2] & 0x80) == 0)
    {
        if(opr->hostDebug == hostid)
        Debug(console,"l&g%02d> * L bit => 0 ... return !\n", hostid);
        return(0);
    }

    rcvCount = 4;
    reqsize = (rxbuf[3] & 0x7f) + 2;

    pause(reqsize*10);
    
    /* ------------------------------------ */
    /* receive DATA-BLOCK                   */
    /* ------------------------------------ */
    //if((rcvsize = asyncRead( opr, tp, &rxbuf[rcvCount], reqsize)) != reqsize)
    if((rcvsize = hostChanRead( hostid, chid, &rxbuf[rcvCount], reqsize)) != reqsize)    
    {
        rcvCount += rcvsize;
        if(opr->hostDebug == hostid)
        {
            Debug(console,"l&g%02d> *Tail not Rcv [req =%d], received %d\n", hostid, reqsize, rcvsize);
            DumpBuff(console, "*RCV: ", (char *)rxbuf, rcvCount);
        }
        return (0);
    }
  
    rcvCount += rcvsize;

CHECK_CRC:
    /* ------------------------------------ */
    /* check CRC-code                       */
    /* ------------------------------------ */
    calcrc= GenCrc((char *)&rxbuf[0], rcvCount);
    
    if(calcrc != 0)
    {
        rcvcrc = (rxbuf[rcvCount-1]*256) + rxbuf[rcvCount-2];
        if(opr->hostDebug == hostid)
        {
            Debug(console,"l&g%02d> *CRC error (rcv:%4x, cal:%4x)\n", hostid, rcvcrc, calcrc);
            DumpBuff(console, "*CRC: ", (char *)rxbuf, rcvCount);
        }
        
        return (0);
    }
    
    return(rcvCount);
}

/*
*/
void     exceptReport(int hostid, int chid, byte *txbuf)
{
    int     txcnt;
    
    word    crc;
    HOST_DCB    *host;
    TTY_DESC    *tp;

    host = (HOST_DCB *) hostDCB[hostid];
    tp   = (TTY_DESC *) host->ttyPort[chid];
    
    /* Response Message 지정 */
    txbuf[0] = 0xff;
    txbuf[1] = 0;                     /* frame status */
    txbuf[2] = landis->rtuid;         /* rtu addr */  
    txbuf[3] = OP_EXP_REPORT | 0x80;
    txbuf[4] = 0;                     /* data size */
    txcnt    = 5;

    txbuf[txcnt++] = landis->exceptCode;
    txbuf[txcnt++] = landis->exceptPara;
    
    txbuf[1] = getSTSLandis();        /* frame status */
    
    txbuf[4] = txcnt - 4 - 1;

    crc = GenCrc((char *)&txbuf[1], txcnt-1);

    txbuf[txcnt++] = crc & 0xff;
    txbuf[txcnt++] = (crc >> 8) & 0xff;

    hostChanWrite( hostid, chid, 0, txbuf, txcnt);
    if(opr->hostDebug == hostid)    DumpBuff(console, "*EXCEPT: ",(char *) &txbuf[1], txcnt-1);

    /* ExceptionFlag Clear... */     
    landis->exceptFlag = 0;
    landis->exceptCode = 0;
    landis->exceptPara = 0;
}

/**
**/
int respLANDIS(int  hostid, byte *rxbuf, byte *txbuf)
{
    //int     i,j,ioid;
    int     opcode;
    //int     startPoint,stopPoint;
    //int     timeBias;
    //int     retVal;
    int     txCount;
    //short   anaData1,anaData2,anaData3;
    word    crc;
    //byte    hour,min,sec,data;
    //uint    tick;
    HOST_DCB *host;
    
    host = (HOST_DCB *) hostDCB[hostid];
    txCount = 0;
    
    /* ---------------------------------------- */
    /*  RTU 가 응답을 하지 않는 경우    ...     */
    /* ---------------------------------------- */
    if(landis->broadCast == SET)
    {
        if(opr->hexDebug == hostid)    Debug(console,"l&g%02d> rcv BROADCAST ...!\n", hostid);
        return (0);
    }
    else if(rxbuf[0] & BIT_SHR)
    {
        if(opr->hexDebug == hostid)    
        Debug(console,"l&g%02d> No Data Block ...!\n", hostid);
        return (0);
    }    
                     
    opcode  = rxbuf[2] & 0x7f;
    landis->rcvOpcode = opcode;
    landis->dataCount = 0;      /* send msg Count clear ... */
        
    if(opcode < OP_RESTART)
    {
        switch(opcode)
        {
        case OP_ANA_COS:
            landis->sendPos     = 0;
            landis->totalCount  = 0;
            if(opr->hexDebug == hostid) Debug(console,"l&g%02d> ANA COS Report ...!\n", hostid);
            
            /*notAllowFunc(opcode, hostid);   */
            break;  
                             
        case OP_ANA_FORCE:
            landis->sendPos  = 0;
            gen_ana_force(hostid, rxbuf);
            host->hostActive = HOST_ACTIVE_STS;
            break;     
            
        case OP_ANA_GCOS: 
            landis->sendPos     = 0;
            landis->totalCount  = 0;
            if(opr->hexDebug == hostid)  Debug(console,"l&g%02d> ANA GROUP COS ... !\n", hostid);
            /*notAllowFunc(opcode, hostid);   */
            break;                 

        case OP_ANA_GFORCE:      
            landis->sendPos     = 0;
            if(opr->hexDebug == hostid) Debug(console,"l&g%02d> ANA GROUP FORCE ... !\n", hostid);
            /*notAllowFunc(opcode, hostid);   */           
            break;          
                                      
        case OP_ADC_REF:                // 아날로그 카드별 기준정보
            landis->sendPos  = 0;
            gen_adc_ref(hostid, rxbuf);
            break;                   
            
        case OP_IND_COS:         
        case OP_SOE_COS: 
            landis->sendPos  = 0;       
            gen_ind_cos(hostid, rxbuf);
            break;

        case OP_IND_FORCE:       
        case OP_SOE_FORCE:   
            landis->sendPos  = 0;  
            gen_ind_force(hostid, rxbuf);  
            host->hostActive = HOST_ACTIVE_STS;
            break;                  
             
        case OP_DIGITAL_FORCE:   
            landis->sendPos  = 0;
            if(opr->hostDebug == hostid) Debug(console,"l&g> DIGITAL FORCE ... !\n");
            /*notAllowFunc(opcode, hostid);   */
            break;                  

        case OP_ACC_COS: 
            landis->sendPos  = 0;  
            landis->totalCount = 0;      
            if(opr->hostDebug == hostid) Debug(console,"l&g> ACC COS Report ...!\n");
            /*notAllowFunc(opcode, hostid);   */
            break;                   

        case OP_ACC_FORCE:
            landis->sendPos  = 0;
            if(opr->hostDebug == hostid) Debug(console,"l&g> ACC FORCE Report ...!\n");
            /*notAllowFunc(opcode, hostid);   */          
            break;                   

        case OP_SOELOG_COS:
            landis->sendPos  = 0;      
            gen_soe_log(hostid, rxbuf);  
            break;   
                            
        case OP_ANALOG_OUT:
            landis->sendPos  = 0; 
            if(opr->hostDebug == hostid)  Debug(console,"l&g> ANA OUTPUT ... !\n");
            /*gen_AO_Out();*/
            /*notAllowFunc(opcode, hostid);   */
            break;          
                                        
        case OP_SBO_SEL: 
            landis->sendPos  = 0; 
            gen_sbo_sel(hostid, rxbuf);
            break; 
                              
        case OP_SBO_OPR: 
            landis->sendPos  = 0;        
            gen_sbo_opr(hostid, rxbuf);
            break;  
                            
        case OP_DIGITAL_OUT: 
            landis->sendPos  = 0;
            if(opr->hostDebug == hostid)  Debug(console,"l&g> DIGITAL OUTPUT ... !\n");
            /*notAllowFunc(opcode, hostid);   */
            landis->totalCount = landis->dataCount;                 
            break;                   
                                         
        case OP_ACC_FRZ: 
            landis->sendPos  = 0;      
            if(opr->hostDebug == hostid)  Debug(console,"l&g> ACC FREEZE ... !\n");
            /*notAllowFunc(opcode, hostid);   */
                       
            break;                   

        case OP_PULSE_OUT:
            landis->sendPos  = 0;      
            if(opr->hostDebug == hostid)  Debug(console,"l&g> PULSE OUTPUT ... !\n");
           /*notAllowFunc(opcode, hostid);   */
            break;                   
            
        case OP_PULSE_TRAIN:     
            landis->sendPos  = 0;
            if(opr->hostDebug == hostid)  Debug(console,"l&g> PULSE TRAIN ... !\n");
            /*notAllowFunc(opcode, hostid);   */
            break;                   
            
        case OP_SBO_EXE: 
            landis->sendPos  = 0; 
            if(opr->hostDebug == hostid)   Debug(console,"l&g> SBO EXE ... !\n");
            if(gen_sbo_exe(hostid, &rxbuf[4]) != OK)
            {
                /* if missmatch Control point */
                landis->exceptFlag = SET;
                landis->exceptCode = EXCEPT_SEMIS;
                landis->exceptPara = opr->point;
            }            
            break;                   
 
        default:
            if(opr->hostDebug == hostid)  Debug(console,"l&g> 1... Invalid Opcode [%2d] !\n", opcode);
            return (0);
            break;    
        }        
    }
    else
    {
        switch(opcode)
        {
        case OP_RESTART: 
            if(opr->hostDebug == hostid)   Debug(console,"l&g> RTU RESTART ... !\n");
            pause(1000);
            //reboot();            
            break;                   
            
        case OP_CONFIG:  
            landis->sendPos  = 0;     
            gen_rtu_config(hostid, rxbuf);
            break;
                               
        case OP_TIME_SYNC:
            landis->sendPos  = 0;
            gen_time_sync(hostid, rxbuf);

            /* ----------------------- */
            /*  SHORT-MESSAGE response */
            /* ----------------------- */
            landis->shortMsg = SET;   
            break;    
                           
        case OP_TIME_BIAS:
            landis->sendPos  = 0;   
            gen_time_bias(hostid, rxbuf);
            break;                   
 
        case OP_DEADBAND:
            landis->sendPos  = 0;     
            if(opr->hostDebug == hostid) Debug(console,"l&g> ANA DEADBAND ... !\n");

            /* ----------------------- */
            /*  SHORT-MESSAGE response */
            /* ----------------------- */
            landis->shortMsg = SET;
            landis->freeze  = 0;
            break;                   
            
        case OP_ANA_GROUP:
            landis->sendPos  = 0;    
            if(opr->hostDebug == hostid) Debug(console,"l&g> ANA GROUP DEF ... !\n");
            /* ----------------------- */
            /*  SHORT-MESSAGE response */
            /* ----------------------- */
            landis->shortMsg = SET;
            /*notAllowFunc(opcode, hostid);   */ 
            break;                   
            
        case OP_ACC_PRESET:
            landis->sendPos  = 0; 
            if(opr->hostDebug == hostid) Debug(console,"l&g> ACC PRESET ... !\n");
            /* ----------------------- */
            /*  SHORT-MESSAGE response */
            /* ----------------------- */
            landis->shortMsg = SET;
            break;                   
            
        case OP_CONTINUE:
            if(opr->hostDebug == hostid) Debug(console,"l&g> CONTINUATION ... !\n");
            landis->totalCount = landis->totalCount - landis->sendSize;            
            if(landis->preOpcode == OP_ANA_FORCE)   
            {
                landis->sendPos  = 0;
                gen_ana_force(hostid, rxbuf);
                opcode   = OP_ANA_FORCE;
            }                    
            break;                   
            
        case OP_REPEAT:  
            if(opr->hostDebug == hostid)  Debug(console,"l&g> LAST REPEAT ... !\n");
            /*notAllowFunc(opcode, hostid);   */
            landis->totalCount = 0;   
            break;                   
            
        case OP_FIRMWARE:
            landis->sendPos  = 0;     
            if(opr->hostDebug == hostid) Debug(console,"l&g> FIRMWARE ... !\n");
            landis->totalCount = 0;
            gen_firmware(hostid, rxbuf);
            break;                   
            
        case OP_TREAD:   
            landis->sendPos  = 0;
            if(opr->hostDebug == hostid)   Debug(console,"l&g> TABLE READ ... !\n");
            landis->totalCount = 0;
            /*notAllowFunc(opcode, hostid);   */
            break;                   
            
        case OP_TWRITE:  
            landis->sendPos  = 0;     
            if(opr->hostDebug == hostid) Debug(console,"l&g> TABLE WRITE ... !\n");
            landis->totalCount = 0;
            /*notAllowFunc(opcode, hostid);   */
            break;                   
            
        case OP_SELF_INTERVAL:
            if(opr->hostDebug == hostid) Debug(console,"l&g> SELF INTERVAL ... !\n");
            landis->totalCount = 0;
            /*notAllowFunc(opcode, hostid);   */
            break;                   
            
        case OP_SELF_SEQUENCE:
            landis->sendPos  = 0;
            landis->totalCount = 0;
            if(opr->hostDebug == hostid)  Debug(console,"l&g> SELF SEQUENCE ... !\n");
            /*notAllowFunc(opcode, hostid);   */
            break;                   

        case OP_EXP_REPORT:   
            landis->totalCount = 0;
            if(opr->hostDebug == hostid)   Debug(console,"l&g> EXCEPT REPORT ... !\n");
            /*notAllowFunc(opcode, hostid);   */
            break;    
                                       
        default:
            if(opr->hostDebug == hostid)  Debug(console,"l&g> 2... Invalid Opcode [%2d] !\n", opcode);
            /*notAllowFunc(opcode, hostid);   */
            landis->totalCount = landis->dataCount;         
            break;    
        }/*switch */            
    }

    if(landis->shortMsg == SET)
    {
        /* ----------------------- */
        /*  SHORT-MESSAGE response */
        /* ----------------------- */
        txCount = 0;
        txbuf[txCount++] = 0xff;
        txbuf[txCount++] = getSTSLandis();        /* frame status */
        txbuf[txCount++] = landis->rtuid;
        landis->shortMsg = RESET;       
    }
    else 
    {
        if(landis->multiMsg == RESET)
        {
            if(landis->totalCount >= 128)       /* max report - 127 byte */ 
            {
                landis->multiMsg = SET;
                landis->sendSize = 127;
                landis->sendPos  = 0;
            }    
            else
            {
                landis->sendSize = landis->totalCount;
                landis->sendPos  = 0;
            }    
        }
        else 
        {
            if(landis->totalCount >= 128)
            {
                landis->sendSize = 127;
            }
            else
            {
                landis->sendSize = landis->totalCount;
                landis->multiMsg = RESET;
            }
        }
    
        /* send message HEADER-BLOCK */
        txbuf[0] = 0xff;
        txbuf[1] = 0;                                       /* frame status */
        txbuf[2] = landis->rtuid;                           /* rtu addr */  
        txbuf[3] = opcode | 0x80;                           /* opcode */
        
#if 0
        if(landis->multiMsg == SET)  txbuf[3] &= 0x7f;   /* opcode */
#endif
    
        txbuf[4] = 0;                                       /* data size */
        txCount  = 5;
    
        /* send message DATA-BLOCK */
        memcpy( &txbuf[txCount], &landis->dataBuf[landis->sendPos], landis->sendSize);
        txCount    = txCount + landis->sendSize;
        landis->sendPos  = landis->sendPos + landis->sendSize;
        
        txbuf[1] = getSTSLandis();        /* frame status */
        txbuf[4] = txCount - 4 - 1;

    }
    
    /* send message CRC-BLOCK */
    crc = GenCrc((char *)&txbuf[1], txCount-1);

    txbuf[txCount++] = crc & 0xff;
    txbuf[txCount++] = (crc >> 8) & 0xff;

    landis->preOpcode = opcode;
    
    return(txCount);
    
    
} 


/* ------------------------------------------------------- */
/*  HOST MAIN 프로그램 Start Routine ....             */
/* ------------------------------------------------------- */
int    hostLandisThread(int hostid)
{
    //int     i;
	byte	runtick;
	int     opcode, rcvACK;    
	int		oldsec=0;
	//int     retVal, timeout;
	//int     passSec, passTime;
	int     txcnt, rxcnt;
	int     activeFID=0;
	//byte    front, rear;
	byte    *rxbuf,*txbuf;
	//word    crc;
	char    buffer[128];
	
	HOST_DCB        *host;
	
	// HOST별 Index ...
    host = (HOST_DCB *) hostDCB[hostid];                 /* 주장치 #1 속성정의 */
    
    host->hostActive = HOST_NOT_READY;          /* HOST 통신모드 : 미지정 */   
    
    host->online[MASTER_PORT] = 0;    // default OFFLINE
    host->online[SLAVE_PORT]  = 0;    // default OFFLINE
    host->comFailTick[MASTER_PORT] = 0;
    host->comFailTick[SLAVE_PORT]  = 0;

    runtick = 0;

    /* -------------------------------------------- */
    /*  VMESIO Channel Comm Parameter 초기화        */
    /* -------------------------------------------- */
    host->vmeChan[MASTER_PORT]->protocolType = SCAN_ASYNC;
    host->vmeChan[MASTER_PORT]->cfgType      = host->hostComType;
    host->vmeChan[MASTER_PORT]->cfgSpeed     = host->hostComSpeed;
    
    host->vmeChan[SLAVE_PORT]->protocolType  = SCAN_ASYNC;
    host->vmeChan[SLAVE_PORT]->cfgType       = host->hostComType;
    host->vmeChan[SLAVE_PORT]->cfgSpeed      = host->hostComSpeed;
    
    vmeHostInitial(hostid, host->masterChan);  
    vmeHostInitial(hostid, host->slaveChan);  
    
    /* ---------------------------------------------------- */
    /* ACE-RTU : 상위 HOST 통신포트 초기화                  */
    /* ---------------------------------------------------- */        
    if(host->hostDualMode == HOST_DUAL)             // 이중화 구조인 경우 
    {
        printf(" %s HOST[%d] LANDIS-THREAD *DUAL [%d/%d]*  Activated ...RTUID=%d, HOSTID=%d !\n", TARGET_NAME, hostid+1, host->masterChan, host->slaveChan, host->rtuAddr, host->hostid);
    }
    else if(host->hostDualMode == HOST_SINGLE)      // 개별 구조인 경우 
    {
        printf(" %s HOST[%d] LANDIS-THREAD *SINGLE [%d/%d]*  Activated ...RTUID=%d, HOSTID=%d  !\n", TARGET_NAME, hostid+1, host->masterChan, host->slaveChan, host->rtuAddr, host->hostid);
    }        
    else         // 사용하지 않는 경우 
    {
        printf(" %s HOST[%d] LANDIS-THREAD *NOT Define [%d/%d]*  Activated ... !\n", TARGET_NAME, hostid+1, host->masterChan, host->slaveChan);
        
        while(termExec)
	    {
	        wdtHostFlag[hostid] = 0;
		    pause(1000);        
		    
		    opr->mmiHostSts[hostid] = 3;    // 사용않함.    
		    
		    if(opr->wdtDebug)
		    Debug(console,">> *HOST[%d] LANDIS-THREAD *NOT Define ... !\n", hostid+1);
        }		    
    }
    
    txbuf = (byte *) &host->sndBuffer[0];  
    rxbuf = (byte *) &host->rcvBuffer[0];  
             
    host->noCommand  = 0;
    host->rxcnt= 0;
    host->txcnt= 0;  
    
    /* LANDIS Analog-Data init... */
    analogInitLANDIS();

    /* ------------------------------------ */
    /* LANDIS RTU Address : 1 RTU           */
    /* ------------------------------------ */
    landis->rtuid    = host->rtuAddr;
    landis->hostAddr = host->hostid;
    opr->hostLandis  = SET;
    
    pause(200);

    /* ------------------------------------ */
    /* Restart Exception Call               */
    /* ------------------------------------ */
    landis->exceptFlag = SET;
    landis->exceptCode = EXCEPT_COLDS;
    landis->exceptPara = 0;

    landis->version  = 0x010A;    /* version 1.0A */
    landis->multiMsg = RESET; 
    landis->shortMsg = RESET; 

    opcode   = 0x80;
    rcvACK   = 0;
    landis->rcvACK = 0x80;

    landis->anaDumpIndex  = 1;
    landis->sendSize      = 0;
    landis->sendPos       = 0;
    landis->totalCount    = 0;

    opr->hostRestart[hostid] = RESET;
                            
	while(termExec)
	{
	    taskPtr->wdtCount = 0;
	    wdtHostFlag[hostid] = 0;
		pause(20);

		/* ---------------------------------------------------- */
		/*	CPU 이중화 : LOCAL-SLAVE ... 대기모드 				*/
		/* ---------------------------------------------------- */
		if(opr->runMode == LOCAL_SLAVE)	
		{
		    /* 2015.08.25 Update...SLAVE모드시 HOST Offline 처리 */
		    host->online[MASTER_PORT] = 0;
		    host->online[SLAVE_PORT]  = 0;
		    pause(1000);
		    
		    /* ------------------------------------ */
            /* HOST-DB 변경에 따른 재기동 ...       */
            /* ------------------------------------ */
            if(opr->hostRestart[hostid] == SET)
            {
                Debug(console, "host%02d> *** HOST TASK Restart... DB Chg ...\n", hostid+1);
                pause(1000);
                termExec = RESET;
            }
            
		    continue;
		}
		
        if(opr->rtuinit > 0)
        {
            if(opr->hostDebug == hostid)    Debug(console,"LANDIS-host%d> *** Module Not Ready (rtuinit= %d).... !\n", hostid, opr->rtuinit);
            pause(100);
            continue;
        }
        
        landis->rcvPACKET   = RESET;
        txbuf = (byte *) &host->sndBuffer[0];  
        rxbuf = (byte *) &host->rcvBuffer[0];  

        /* ---------------------------------------------------- */
        /*  MASTER 통신포트 : 상위 데이터 수신 처리 ...         */
        /* ---------------------------------------------------- */
        if((rxcnt = rcvWithLANDIS( hostid, MASTER_PORT, &rxbuf[0])) > 0)
        {
            host->comFailTick[MASTER_PORT]= 0;
            host->comFailTick[SLAVE_PORT] = 0;
            
            opr->hostComSts[hostid] = 1;
            landis->rcvPACKET = SET;
            activeFID = MASTER_PORT;

            if(host->online[MASTER_PORT] != 1)  
            {
                logEvent1(opr, sque, EVENT_HOST_ONLINE, hostid, MASTER_PORT, 0, hostid);  
                logHistoryQue1(opr, hque, EVENT_HOST_ONLINE, hostid, MASTER_PORT, 0, hostid);  
                
                /* -------------------------------------------- */
                /* LOG File 저장                                */
                /* -------------------------------------------- */
                sprintf(buffer, "LANDIS> ID=%2d, => MASTER-PORT Online ", hostid+1);
                LogFile_MPU ( opr, EVENT_HOST_ONLINE, buffer, strlen(buffer));
            }
                                        
            host->online[MASTER_PORT] = 1;
        }
        /* ---------------------------------------------------- */
        /*  SLAVE  통신포트 : 상위 데이터 수신 처리 ...         */
        /* ---------------------------------------------------- */        
        else if(host->hostDualMode == HOST_DUAL)
        {
            if((rxcnt = rcvWithLANDIS( hostid, SLAVE_PORT, &rxbuf[0])) > 0)
            {
                host->comFailTick[MASTER_PORT]= 0;
                host->comFailTick[SLAVE_PORT] = 0;
            
                opr->hostComSts[hostid] = 1;
                landis->rcvPACKET = SET;
                activeFID = SLAVE_PORT;
                        
                if(host->online[SLAVE_PORT] != 1)  
                {
                    logEvent1(opr, sque, EVENT_HOST_ONLINE, hostid, SLAVE_PORT, 0, hostid);  
                    logHistoryQue1(opr, hque, EVENT_HOST_ONLINE, hostid, SLAVE_PORT, 0, hostid);  
                    
                    /* -------------------------------------------- */
                    /* LOG File 저장                                */
                    /* -------------------------------------------- */
                    sprintf(buffer, "LANDIS> ID=%2d, => SLAVE-PORT Online ", hostid+1);
                    LogFile_MPU ( opr, EVENT_HOST_ONLINE, buffer, strlen(buffer));
                }
                
                host->online[SLAVE_PORT] = 1;
            }            
        }

        /* ---------------------------- */
        /*  response HOST message       */
        /* ---------------------------- */        
        if(landis->rcvPACKET == SET)
        {
            if(opr->hostDebug == hostid)    printf("------------------------->\n");
                
            if(opr->hostDebug == hostid)
            {
                if(activeFID == MASTER_PORT)        DumpBuff(console, "RXM:", (char *)rxbuf, rxcnt);
                else if(activeFID == SLAVE_PORT)    DumpBuff(console, "RXS:", (char *)rxbuf, rxcnt);
            }
            
            /* --------------------------------- */
            /* check Retry Command & send Buffer */
            /* --------------------------------- */
            opcode = rxbuf[2] & 0x7f;
            rcvACK = rxbuf[0] & BIT_ACK;

            if((opcode == OP_REPEAT) || (rcvACK == landis->rcvACK))
            {
                if(activeFID == MASTER_PORT)    hostChanWrite( hostid, MASTER_PORT, 0, landis->sndBuffer, landis->sndCount);
                if(activeFID == SLAVE_PORT)     hostChanWrite( hostid, SLAVE_PORT, 0, landis->sndBuffer, landis->sndCount);
                
                if(opr->hostDebug == hostid) DumpBuff(console, "*REP: ", (char *)&landis->sndBuffer[1], landis->sndCount-1);
            }
            else
            { 
                /* -------------------------------- */
                /* check rcv ACK & Response Message */
                /* -------------------------------- */
                landis->rcvACK = rcvACK;
                txcnt = respLANDIS(hostid, &rxbuf[0], txbuf);
                
                if(txcnt > 0)   
                {
                    if(activeFID == MASTER_PORT)        hostChanWrite( hostid, MASTER_PORT, 0, txbuf, txcnt);
                    else if(activeFID == SLAVE_PORT)    hostChanWrite( hostid, SLAVE_PORT, 0, txbuf, txcnt);
                
                    /* Store snd buffer */
                    memcpy(landis->sndBuffer, txbuf, txcnt);
                    landis->sndCount = txcnt;
                
                    if(opr->hostDebug == hostid)    
                    {
                        if(activeFID == MASTER_PORT)        DumpBuff(console,"TXM:", (char *)&txbuf[1], txcnt-1);
                        else if(activeFID == SLAVE_PORT)    DumpBuff(console,"TXS:", (char *)&txbuf[1], txcnt-1);
                    }

                    if(landis->exceptFlag)
                    {
                        if(activeFID == MASTER_PORT)        exceptReport(hostid, MASTER_PORT, txbuf);
                        else if(activeFID == SLAVE_PORT)    exceptReport(hostid, SLAVE_PORT, txbuf);
                    }                        
                }       
            }      
                        
        } 
        
        if(oldsec == rtc->sec)  continue;
        oldsec = rtc->sec;
        
        /* ------------------------------------ */
        /* HOST-DB 변경에 따른 재기동 ...       */
        /* ------------------------------------ */
        if(opr->hostRestart[hostid] == SET)
        {
            Debug(console, "host%02d> *** HOST TASK Restart... DB Chg ...\n", hostid+1);
            pause(1000);
            termExec = RESET;
        }
        
        /* -------------------------------- */
        /* 상위 HOST별 통신상태 Check...    */
        /* -------------------------------- */
        if(++host->comFailTick[MASTER_PORT] > 30)   
        {
            opr->hostComSts[hostid] = 0;
            
            host->comFailTick[MASTER_PORT] = 0;
            if(host->online[MASTER_PORT] == 1)
            {
                logEvent1(opr, sque, EVENT_HOST_OFFLINE, hostid, MASTER_PORT, 0, hostid);  
                logHistoryQue1(opr, hque, EVENT_HOST_OFFLINE, hostid, MASTER_PORT, 0, hostid);  
                
                /* -------------------------------------------- */
                /* LOG File 저장                                */
                /* -------------------------------------------- */
                sprintf(buffer, "LANDIS> ID=%2d, => MASTER-PORT *Offline ", hostid+1);
                LogFile_MPU ( opr, EVENT_HOST_OFFLINE, buffer, strlen(buffer));
            }
            
            host->online[MASTER_PORT] = 0;
            host->hostActive = HOST_NOT_READY;          /* HOST 통신모드 : 미지정 */  
            
            /* -------------------------------- */                        
            /* VME Channel Reinitial...         */
            /* VMECHAN 프로토콜 타입 지정       */
            /* -------------------------------- */  
            vmeHostChanReset(hostid, MASTER_PORT);
            host->vmeChan[MASTER_PORT]->protocolType  = SCAN_ASYNC;
            host->vmeChan[MASTER_PORT]->cfgType       = host->hostComType;
            host->vmeChan[MASTER_PORT]->cfgSpeed      = host->hostComSpeed;

            vmeHostInitial(hostid, host->masterChan);  
            
            if(opr->hostDebug == hostid)
            Debug(console,"host> *LANDIS-HOST(%2d) : Master Comm Point initial...!\n", hostid);            
        }
        /* DUAL 모드인 경우 : Master Port 가 OFFLINE 인 경우 */
        else if(host->hostDualMode == HOST_DUAL)
        {
            if(++host->comFailTick[SLAVE_PORT] > 30)   
            {
                opr->hostComSts[hostid] = 0;
                
                host->comFailTick[SLAVE_PORT] = 0;
                
                if(host->online[SLAVE_PORT] == 1)
                {
                    logEvent1(opr, sque, EVENT_HOST_OFFLINE, hostid, SLAVE_PORT, 0, hostid);  
                    logHistoryQue1(opr, hque, EVENT_HOST_OFFLINE, hostid, SLAVE_PORT, 0, hostid);  
                    
                    /* -------------------------------------------- */
                    /* LOG File 저장                                */
                    /* -------------------------------------------- */
                    sprintf(buffer, "LANDIS> ID=%2d, => SLAVE-PORT *Offline ", hostid+1);
                    LogFile_MPU ( opr, EVENT_HOST_OFFLINE, buffer, strlen(buffer));
                }
            
                host->online[SLAVE_PORT] = 0;
                host->hostActive = HOST_NOT_READY;          /* HOST 통신모드 : 미지정 */  
                
                /* -------------------------------- */                        
                /* VME Channel Reinitial...         */
                /* VMECHAN 프로토콜 타입 지정       */
                /* -------------------------------- */  
                vmeHostChanReset(hostid, SLAVE_PORT);
                
                host->vmeChan[SLAVE_PORT]->protocolType  = SCAN_ASYNC;
                host->vmeChan[SLAVE_PORT]->cfgType       = host->hostComType;
                host->vmeChan[SLAVE_PORT]->cfgSpeed      = host->hostComSpeed;

                vmeHostInitial(hostid, host->slaveChan);  
 
            
                if(opr->hostDebug == hostid)
                Debug(console,"host> *LANDIS-HOST(%2d) : Slave Comm Point initial...!\n", hostid);     
                            
            }
        }
                
	}/* while */

    return (0);
}
