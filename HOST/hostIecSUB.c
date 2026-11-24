#include	"localLib.h"
#include    "external.h"

extern  int     hostChanWrite(int hostid, int chid, int socketID, byte *sndbuf, int sndCount);
extern  void	checkAppType_IEC(int hostid, int object, IEC_DATA_LINK *iecData);
extern  int	    funcASDU_M_SP_NA(int hostid, IEC_DATA_LINK *iecData);
extern  int	    funcASDU_M_SP_TA(int hostid, IEC_DATA_LINK *iecData);
extern  int	    funcASDU_normal_measure_NB(int hostid, IEC_DATA_LINK *iecData);

extern  int	funcASDU_single_command(int hostid, IEC_DATA_LINK *iecData, byte *appData);
extern  int	funcASDU_double_command(int hostid, IEC_DATA_LINK *iecData, byte *appData);
extern  int	funcASDU_endof_initial(int hostid, IEC_DATA_LINK *iecData, byte *appData);
extern  int	funcASDU_intrrogation(int hostid, IEC_DATA_LINK *iecData);
extern  int	funcASDU_clock_sync(int hostid, IEC_DATA_LINK *iecData, byte *appData);
extern  int	funcASDU_delay_check(int hostid, IEC_DATA_LINK *iecData, byte *appData);


/*
*   IEC-HOST 응답시 상태 Check...
*/
int checkStatus_IEC(int hostid, int chid)
{
    //byte    status[2];
    byte    front, rear;
    
    HOST_DCB        *host;
    IEC_SOE_QUEUE   *soeQ;  
    
    host = (HOST_DCB *) hostDCB[hostid];

    /* ------------------------------------ */
    /*  SOE Event : Class Assign...         */
    /* ------------------------------------ */    
    soeQ    = (IEC_SOE_QUEUE *) &host->iecSOEQ;
    front   = soeQ->front& 0xff;
    rear    = soeQ->rear & 0xff;
    
    if(front == rear )	return (0);
	else				
	{
	    if(opr->hostDebug == hostid)    printf("=> IEC(%d) SOE-Q front=%d, rear=%d\n", hostid, front, rear);
		return (1);		// SOE가 있음.
	}
	
}



/***************************************************
*   FUNCTION : sendConfirm_IEC()
*   send DataLink Layer : make send frame & send
***************************************************/
void sendConfirm_IEC( int hostid, int chid, IEC_DATA_LINK *iecData, int code)
{
    //int     i,j,k;
    int     txcnt;
    byte	control;
    byte    *txbuf;
    byte	chksum;

    HOST_DCB    *host;
    IEC_FIX_FRAME   *sndFrame;
    //TTY_DESC    *tp;

    host = (HOST_DCB *) hostDCB[hostid];
    
    sndFrame = (IEC_FIX_FRAME *) &iecData->sndFrame1;
    txbuf = (byte *) &sndFrame->start1;

	control = code & 0x0f;
	
	/* ---------------------------------------- */
	/*  SOE 정보 추출...                        */
	/* ---------------------------------------- */
	if( checkStatus_IEC(hostid, chid) > 0)	control = control | LINK_CNTR_ACD_BIT;
	
    /* ------------------------ */
    /*  Make HEADER frame ...   */
    /* ------------------------ */
    txcnt = 0;

    txbuf[txcnt++] = PHYS_FIXED_STX;
    txbuf[txcnt++] = control;                   	/* control */
    //txbuf[txcnt++] = host->hostid & 0xff;           /* des Addr */
    txbuf[txcnt++] = host->rtuAddr & 0xff;           /* des Addr */
    
    chksum = genSumByte( &txbuf[1], txcnt - 1); 
    
    txbuf[txcnt++] = chksum;
    txbuf[txcnt++] = PHYS_ETX;
    
    if(opr->hostDebug == hostid)   prn_snd("ACK:", (char *)txbuf, txcnt);
    
    //PortWrite( (TTY_DESC *) tp, (char *) txbuf, txcnt);
    hostChanWrite( hostid, chid, 0, txbuf, txcnt);
    pause(20);
}




/***************************************************
*   FUNCTION : sendDataLink_IEC()
***************************************************/
int sendDataLink_IEC(int hostid, int chid,IEC_DATA_LINK *iecData, int count)
{
    //int     i,j,k;
    //int     opcode;
    //int     frameCnt;
    //int     dataLen,bufPos;
    //int     sndCount;
    int     txcnt;
    //int     retryCnt;
    byte	chksum;
    byte    *txbuf;
    byte    control;
    //word    crc;
    
    //DL_FRAME        *sndData;
    //DNP_DATA_LINK   *dnpData;
    HOST_DCB        *host;
    //TTY_DESC         *tp;

    host = (HOST_DCB *) hostDCB[hostid];  

	txcnt = 0;	
	control = LINK_FUNC_USER_DATA;
	
	/* ---------------------------------------- */
	/*  SOE 정보 추출...                        */
	/* ---------------------------------------- */
	if( checkStatus_IEC(hostid, chid) > 0)	control = control | LINK_CNTR_ACD_BIT;
	
	if(iecData->sndFrameType == IEC_LINK_VARIABLE_TYPE)		
	{
		txbuf = (byte *) &iecData->sndFrame2;		// 가변정보 구조체
		txbuf[0] = PHYS_VARIABLE_STX;
		txbuf[1] = count + 2;                       // USER Data + Control + Address
		txbuf[2] = count + 2;
		txbuf[3] = PHYS_VARIABLE_STX;
		
		txbuf[4] = control;
		txbuf[5] = host->rtuAddr;
		
		txcnt = count + 6;                          // USER Data + Header Count
		
		chksum = genSumByte( &txbuf[4], txcnt - 4); 
		txbuf[txcnt++] = chksum;
		txbuf[txcnt++] = PHYS_ETX;
		
	}	
	else
	{
		txbuf = (byte *) &iecData->sndFrame1;		// 고정정보 구조체
	}

	/* ---------------------------------------- */
	/*  HOST 데이터 전송 ...                    */
	/* ---------------------------------------- */
    if(hostChanWrite(hostid, chid, 0, txbuf, txcnt) <= 0)
    {
    	if(opr->hostDebug == hostid)
    	printf("*** host [%2d] Channel LinkFail ...txcnt = %d !\n", hostid, txcnt);
    	
    	host->comFailTick[chid] = 120;
    	
    	return (OK);
    }
    
    if(opr->hostDebug == hostid)       
   	{
       	if(chid == MASTER_PORT)         prn_snd("TXM:", (char *)txbuf, txcnt);
       	else if(chid == SLAVE_PORT)     prn_snd("TXS:", (char *)txbuf, txcnt);
   	}

    return (OK);


}



/***************************************************
    FUNCTION : checkAPPFrame_IEC()
*   include TH,AC
***************************************************/
int     checkAPPFrame_IEC(int hostid, int chid, IEC_DATA_LINK *iecData)
{
    int     sndCount;
    //int     dataPos;
    //byte    acHeader, requestCON;
    int     qualifier;
    //int     indexSize, qcode;
    int		sector, cotCode, infoAddr,objCount;
    int		quantity, sqCode;
    byte	*ptrInfoObj;
        
    //DL_FRAME        *rcvData;
    //DNP_APP_FRAME   *rcvAppDnp;
    //DNP_APP_FRAME   *sndAppDnp;
    //DNP_DEVICE      *dnp;
    
    //DNP_SOE_QUEUE   *soeQ;
    //DNP_SOE_QUEUE   *dnpClass1;
    //DNP_COS_QUEUE   *cosQ;
    //DNP_COS_QUEUE   *dnpClass2;
    //DNP_COA_QUEUE   *coaQ;
    HOST_DCB        *host;
    IEC_VAR_FRAME	*rcvFrame;


    host = (HOST_DCB *) hostDCB[hostid];
    
    if(iecData->frameType != IEC_LINK_VARIABLE_TYPE)    return (0);
   
    /* 가변 Frame 수신구조체 지정 */ 
    rcvFrame = (IEC_VAR_FRAME *) &iecData->rcvFrame2;
    
    if(opr->hostDebug == hostid)
    {
        prndat("RCVAPP: ", (char *)&rcvFrame->userData, iecData->rcvCount - 8);
    }
    
    /* ---------------------------------------------------- */
    /*  응용서비스 데이터 Parsing ...                       */
    /*  - ASDU 공통 어드레스 : 2 Byte                       */
    /*  - ASDU 정보객체 주소 : 2 Byte                       */
    /* ---------------------------------------------------- */
    host->rcvACfunc = rcvFrame->userData[0];	        // TYPE code
    qualifier   	= rcvFrame->userData[1];	        // qualifier	
    quantity		= rcvFrame->userData[1] & 0x7f;
    sqCode   		= rcvFrame->userData[1] & 0x80;
    
    cotCode			= rcvFrame->userData[2] & 0x3f;	    // couse of Transmit
    sector			= rcvFrame->userData[3] + (rcvFrame->userData[4] * 256);	// Address of ASDU
    
    infoAddr		= rcvFrame->userData[5] + (rcvFrame->userData[6] * 256);	// Address of INFO
	ptrInfoObj      = (byte *) &rcvFrame->userData[7];							// 정보요소 집합
	objCount    	= iecData->rcvCount - 8 - 7; 								// 정보요소의 크기

    /* ---------------------------------------------------- */
	/* 수신 COT ... */
	/* ---------------------------------------------------- */
	host->rcvCOT    = cotCode;
	sndCount  = 0;
	
	if(opr->hostDebug == hostid)
    {
    	printf("hrcv%2d> Sector(%3d) TYPE=%2d, Qcode=%02x (Sq=%2x, Qcount=%2d), COT= %2d, info Addr = %d \n", 
    		hostid, sector, host->rcvACfunc, qualifier, sqCode, quantity, cotCode, infoAddr);
    	prndat("        info OBJ : ", (char *)ptrInfoObj, objCount);
    }
	
	/* ---------------------------------------------------- */
    /*  응용서비스 데이터 표시 ...                          */
    /* ---------------------------------------------------- */
	checkAppType_IEC(hostid, host->rcvACfunc, iecData);
	
	/* ---------------------------------------------------- */
	/* 	ASDU : TYPE 해석 및 처리 						    */
	/* ---------------------------------------------------- */
	switch(host->rcvACfunc)
	{
	/* ASDUs for process information in monitor direction */  	
	case IEC_TYPE_NO_DEFINE:   		/* No object type assigned  */
	    break;
	    
	case IEC_TYPE_M_SP_NA:   		/* Type 1, Single-point information, without time tag	*/
	    sndCount = funcASDU_M_SP_NA(hostid, iecData);	
	    break;
	    
	case IEC_TYPE_M_SP_TA:   		/* Type 2, Single-point information, with time tag CP24Time2a  	  */
	    sndCount = funcASDU_M_SP_TA(hostid, iecData);	
	    break;
	    
	case IEC_TYPE_M_DP_NA:   		/* Double-point information, without time tag   */
	case IEC_TYPE_M_ST_NA:   		/* Step position information, without time tag	 */
	case IEC_TYPE_M_BO_NA:   		/* Bitstring of 32 bits without time tag 		 */
		
	/* ASDUs for process information in monitor direction */
	case IEC_TYPE_M_ME_NA:      	/* Type 9, Measured value, normalized,  without time tag  */
	case IEC_TYPE_M_ME_NB:      	/* Measured value, scaled without, time tag 	  */    
	    sndCount = funcASDU_normal_measure_NB(hostid, iecData);	
	    break;
	
	case IEC_TYPE_M_ME_NC:      	/* Measured value, short floating point number without time tag  */
	case IEC_TYPE_M_IT_NA:      	/* Integrated totals without time tag */
	case IEC_TYPE_M_PS_NA:      	/* Packed single-point with  status change detection  	  */
	case IEC_TYPE_M_ME_ND:      	/* Measured value, normalized value without quality  */
	case IEC_TYPE_M_SP_TB:  		/* Single point information with time tag CP56Time2a   */
	case IEC_TYPE_M_DP_TB:  		/* Double point information with time tag CP56Time2a   */
	case IEC_TYPE_M_ST_TB:  		/* Step position information with  time tag CP56Time2a  */
	case IEC_TYPE_M_BO_TB:  		/* Bitstring of 32 bits with time  tag CP56Time2a  */
		break;
		
	/* ASDUs for process information in control direction */
	case IEC_TYPE_C_SC_NA:  		/* Type 45, Single Command 				  */
        sndCount = funcASDU_single_command(hostid, iecData, &rcvFrame->userData[5]);		

        /* --------------------------------------------------- */
        /* 2014.10.01 제어실행후.... 제어종료 전송을 위하여... */
        /* --------------------------------------------------- */
        if(opr->iecControl[hostid] == CONTROL_OPERATE)
        {
            /* 수신한 제어정보를 저장 */
            memcpy((byte *)&iecData->rcvControl, (byte *)&iecData->rcvFrame2, sizeof(IEC_VAR_FRAME));   
            if(opr->soeDebug)   
            printf(">> host(%2d) Control Information(single)... : Copy OK...\n", hostid+1); 
        }   
		break;
		
	case IEC_TYPE_C_DC_NA:  		/* Type 46, Double Command 				  */
	    sndCount = funcASDU_double_command(hostid, iecData, &rcvFrame->userData[5]);	

	    /* --------------------------------------------------- */
        /* 2014.10.01 제어실행후.... 제어종료 전송을 위하여... */
        /* --------------------------------------------------- */
        if(opr->iecControl[hostid] == CONTROL_OPERATE)
        {
            /* 수신한 제어정보를 저장 */
            memcpy((byte *)&iecData->rcvControl, (byte *)&iecData->rcvFrame2, sizeof(IEC_VAR_FRAME));   
            if(opr->soeDebug)   
            printf(">> host(%2d) Control Information(double)... : Copy OK...\n", hostid+1); 
        }   
	    break;
	    
	case IEC_TYPE_C_RC_NA:  		/* Regulating step command 		  */
	case IEC_TYPE_C_SE_NA:  		/* Set-point command, normalized  value */
	case IEC_TYPE_C_SE_NB:  		/* Set-point command, scaled value */
	case IEC_TYPE_C_SE_NC:  		/* Set-point command, short floating point number */
	case IEC_TYPE_C_BO_NA:  		/* Bitstring 32 bit 			  */
	 	break;
	 	
	/* ASDUs for system information in monitor direction */
	case IEC_TYPE_M_EI_NA:  		/* Type 70, End of initialization 		  */
	    sndCount = funcASDU_endof_initial(hostid, iecData, &rcvFrame->userData[5]);	
		break;
		
	case IEC_TYPE_C_IC_NA:  		/* Type 100, Interrogation command 		  */
		sndCount = funcASDU_intrrogation(hostid, iecData);		
		break;
		
	case IEC_TYPE_C_CI_NA:  		/* Counter interrogation command  */
		break;
		
	case IEC_TYPE_C_CS_NA:  		/* Type 103, Clock synchronization 		  */
		sndCount = funcASDU_clock_sync(hostid, iecData, &rcvFrame->userData[5]);		
		break;
		
	case IEC_TYPE_C_CD_NA:  		/* Type 106, Delay acquisition command      */
	    sndCount = funcASDU_delay_check(hostid, iecData, &rcvFrame->userData[5]);
		break;

	default:
		break;
	}
	
	return (sndCount);
	
}



/*
*   IEC-LINK MESSAGE 
*   - check DATA-LINK Layer  : 1차국 메세지 해석...    
*/
int     chkRcvLink_IEC( int hostid, int chid, IEC_DATA_LINK *iecData)
{
    int     rcvFCB, rcvFCV, rcvPRM;
    //int     chkFcv;
    byte    control, function;

    HOST_DCB *host;
    IEC_FIX_FRAME	*rcvFixLink;
    IEC_VAR_FRAME	*rcvVarLink;

    host = (HOST_DCB *) hostDCB[hostid];

    /* -------------------------------------------- */
    /*  수신부 Frame-Type 에 다른 처리              */
    /* -------------------------------------------- */
	if(iecData->frameType == IEC_LINK_FIXED_TYPE)		// 고정 Frame의 경우
	{
		rcvFixLink = (IEC_FIX_FRAME *) &iecData->rcvFrame1;
		control = rcvFixLink->control;
	}
	else                                                // 가변 Frame의 경우
	{
		rcvVarLink = (IEC_VAR_FRAME *) &iecData->rcvFrame2;
		control = rcvVarLink->control;	
	}

	/* -------------------------------------------- */
	/* 	check LINK Control & Function code 		    */
	/* -------------------------------------------- */
    function = control & LINK_CNTR_FUNC_MASK;
    
    rcvPRM   = control & LINK_CNTR_PRM_BIT;
    rcvFCB   = control & LINK_CNTR_FCB_BIT;
    rcvFCV   = control & LINK_CNTR_FCV_BIT;

    if(opr->hostDebug == hostid)
    {
        printf("hrcv%2d> LINK: control=%2x [PRM=%2x, FCB=%2x, FCV=%2x, func=%d ]\n",  hostid, control, rcvPRM, rcvFCB, rcvFCV, function);
    }
    
    /* 1차국 메세지가 아닌경우... REturn */
    if(rcvPRM == 0)
    {
        if(opr->hostDebug == hostid)
        printf("hrcv%2d> *** LINK: Invalid PRM (control=%2x, func=%2d)...\n",  hostid, control, function);
        return (-1);
    }
    
    
#if 0    
    /* ------------------------------------------------ */        
    /*  Check DATA-LINK : FCV/FCB 가 유효한 경우...     */
    /*  - 센터 Retry 인 경우... 동일한 FVV 가 내려옴... */
    /*  - Fail 처리를 하면 센터 통신이상으로 처리됨...  */
    /*  - 따라서... FCV Check를 하지 않음...            */
    /* ------------------------------------------------ */   
    if(host->rcvLinkSts == SET)
    {
        /* Frame Count-Bit 가 유효한 경우... */      
        if(rcvFCV)
        {
            if(rcvFCB != host->rcvNextFCB)      
            {
                if(opr->hostDebug == hostid)  
                printf("check-frame %2d> *** Invalid FCV Bit ... rcv=%2x, next=%2x !\n", hostid, rcvFCB, host->rcvNextFCB);

                sendConfirm_IEC( hostid, chid, iecData, LINK_FUNC_NACK);
                return (100);
            }
        }
    }
#endif


	/* ------------------------------------------------ */
	/*	센터로부터 수신된 LINK-메세지 해석...			*/
	/* ------------------------------------------------ */
    switch(function)
    {
    /* Primary station Control Byte function codes  */
    case LINK_FUNC_LINK_RESET:		/* code 0: 송신/확인, Reset remote link  			  */
		if(opr->hostDebug == hostid)
        {  
            printf("============================================\n");
            printf("hrcv%2d> >>>>> IEC...RESET LINK rcv ... !\n", hostid);
            printf("============================================\n");
        }                
            
        host->rcvLinkSts = SET;             		// LINK RESET 상태 
        host->rcvEndOk   = SET;
        host->rcvNextFCV = LINK_CNTR_FCV_BIT;       // 다음명령의 FCB 예상
        host->rcvNextFCB = LINK_CNTR_FCB_BIT;       // 다음명령의 FCB 예상
            
        host->cuRestart  = RESET;
            
        host->sndTHseqno = 0;
        host->resetCount++;
            
        sendConfirm_IEC( hostid, chid, iecData, LINK_FUNC_ACK);	
	    break;
			
	case LINK_FUNC_RESET_USER:		/* code 1: 송신/확인, Reset user process 			  */
		if(opr->hostDebug == hostid)  printf("hrcv%2d> Request.... *RESET USER ... !\n", hostid);
				
		sendConfirm_IEC( hostid, chid, iecData, LINK_FUNC_ACK);	
        //host->rcvNextFCB = (~rcvFCB) & LINK_CNTR_FCB_BIT;         // 다음명령의 FCB 예상
	    break;
			
	case LINK_FUNC_CNFRM_DATA:		/* code 3: 송신/확인, Send confirm data  			  */
		if(opr->hostDebug == hostid)  printf("hrcv%2d> Request.... USER CONFIRM DATA ... !\n", hostid);

		if(host->rcvLinkSts == RESET)  return (100);                // RESET 상태시 무응답 
        sendConfirm_IEC( hostid, chid, iecData, LINK_FUNC_ACK); 
        
        host->rcvNextFCB = (~rcvFCB) & LINK_CNTR_FCB_BIT;           // 다음명령의 FCB 예상				
	    break;
			
	case LINK_FUNC_UNCONFIRM:		/*  code 4: 송신/무응답, Send un-confirm data					  */
		if(opr->hostDebug == hostid)  printf("hrcv%2d> Request.... USER UN-CONFIRM DATA ... !\n", hostid);
		//host->rcvNextFCB = (~rcvFCB) & LINK_CNTR_FCB_BIT;         // 다음명령의 FCB 예상	
		break;
			
	case LINK_FUNC_REQ_ACC_DEM:		/* code 8: 접근요구를 위한 요구, Request -- 					  */
		if(opr->hostDebug == hostid)  printf("hrcv%2d> Request.... ACCESS REQ ... !\n", hostid);
		/* ---------------------------------------- */
        /*  2014.09.29 ACK 명령을 전송하지 않음     */
        /* ---------------------------------------- */		    
		//sendConfirm_IEC( hostid, chid, iecData, LINK_FUNC_ACK);    
		break;
			
	case LINK_FUNC_STATUS_OF_LINK:	/* code 9: 송신/응답, Request link status  		  */
		if(opr->hostDebug == hostid)  printf("hrcv%2d> Request.... LINK STATUS ... !\n", hostid);
		sendConfirm_IEC( hostid, chid, iecData, LINK_FUNC_STS_OF_LINK);
		break;

    /* ---------------------------------------------------- */
    /*  중요데이터 CLASS#1,2....                            */
    /* ---------------------------------------------------- */			
	case LINK_FUNC_REQ_CLASS1:	    /* code 10: 송신/응답, Request class1 data  		  */
		if(opr->hostDebug == hostid)  printf("hrcv%2d> Request.... CLASS#1 DATA ... !\n", hostid);
				
		if(host->rcvLinkSts == RESET)  return (100);    // RESET 상태시 무응답 	
        /* ---------------------------------------- */
        /*  2014.09.29 ACK 명령을 전송하지 않음     */
        /* ---------------------------------------- */		    
		//sendConfirm_IEC( hostid, chid, iecData, LINK_FUNC_ACK);
		host->rcvNextFCB = (~rcvFCB) & LINK_CNTR_FCB_BIT;         // 다음명령의 FCB 예상		
		break;
			
	case LINK_FUNC_REQ_CLASS2:	    /* code 11: 송신/응답, Request class2 data  		  */	
		if(opr->hostDebug == hostid)  printf("hrcv%2d> Request.... CLASS#2 DATA ... !\n", hostid);
				
		if(host->rcvLinkSts == RESET)  return (100);    // RESET 상태시 무응답 	
		/* ---------------------------------------- */
        /*  2014.09.29 ACK 명령을 전송하지 않음     */
        /* ---------------------------------------- */		    
		//sendConfirm_IEC( hostid, chid, iecData, LINK_FUNC_ACK);
		host->rcvNextFCB = (~rcvFCB) & LINK_CNTR_FCB_BIT;         // 다음명령의 FCB 예상		
		break;

	default:
        if(opr->hostDebug == hostid)  printf("hrcv%2d> *default func ... %d!\n", hostid, function);
        function =100;
        break;
    }

	return (function);
	    
}
