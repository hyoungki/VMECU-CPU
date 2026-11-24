
#include	"localLib.h"
#include    "external.h"

#if 0
extern	SHM_MEMORY	    *shmPtr;
extern  RTC             *rtc;
extern  OPR_MSG         *opr;
extern  HISTORY_QUE     *hque;

extern	HOST_DCB        *hostDCB[MAX_HOST];             /* DNP HOST Structure */
extern	DNP_DEVICE      *dnpcfg[MAX_DNP_DEVICE];

extern  int             termExec;

extern  int             wdtHostFlag[MAX_HOST];
#endif


/*
*
*/
IEC_OBJ_TABLE objTable[] = {
	{
		"Single point information without time tag",  	/* description  */
		IEC_TYPE_M_SP_NA,   				  			/* object - 1   */
        1,                                        		/* single data size */
		0                                            	/* default sq   */
	},
	{
	    "Single point information with time tag",     	/* description  */
		IEC_TYPE_M_SP_TA,   				  			/* object - 2   */
        4,                                        		/* single data size */
		0                                            	/* default sq   */
	},
	{
		"Double-point information without time tag",  	/* description  */
		IEC_TYPE_M_DP_NA,   				  			/* object - 3   */
        1,                                        		/* single data size */
		0                                               /* default sq   */
	},
	{
		"Step position information without time tag", 	/* description  */
		IEC_TYPE_M_ST_NA,					  			/* object - 5   */
        2,                                        		/* single data size */
		0                                            	/* default sq   */
	},
	{
		"Bitstring of 32 bits without time tag",	  	/* description  */
		IEC_TYPE_M_BO_NA,					  			/* object - 7   */
        5,                                        		/* single data size */
		0                                            	/* default sq   */
	},
	{
		"Measured value, normalized without time tag",	/* description  */
		IEC_TYPE_M_ME_NA,					  			/* object - 9   */
        3,                                        		/* single data size */
		0                                            	/* default sq   */
	},
	{
		"Measured value, scaled without time tag",	  	/* description  */
		IEC_TYPE_M_ME_NB,					  			/* object - 11  */
        3,                                        		/* single data size */
		0                                            	/* default sq   */
	},
	{
		"Measured value, short floating point number without time tag",	/* description */
		IEC_TYPE_M_ME_NC,					  			/* object - 13  */
        5,                                        		/* single data size */
		0                                            	/* default sq   */
	},
	{
		"Integrated totals without time tag", 			/* description */
		IEC_TYPE_M_IT_NA,					  			/* object - 15  */
        5,                                         		/* single data size */
		0                                            	/* default sq   */
	},
	{
		"Packed single-point with status change detection", /* description */
		IEC_TYPE_M_PS_NA,					  			/* object - 20  */
        5,                                        		/* single data size */
		0                                            	/* default sq   */
	},
	{
		"Measured value, normalized value without quality descriptor", /* description */
		IEC_TYPE_M_ME_ND,					  			/* object - 21  */
        2,                                        		/* single data size */
		0                                            	/* default sq   */
	},
	{
		"Single point information with time tag CP56Time2a", /* description */
		IEC_TYPE_M_SP_TB,					  			/* object - 30  */
        8,                                        		/* single data size */
		0                                            	/* default sq   */
	},
	{
		"Double point information with time tag CP56Time2a", /* description */
		IEC_TYPE_M_DP_TB,					  			/* object - 31  */
        8,                                        		/* single data size */
		0                                            	/* default sq   */
	},
	{
		"Step position information with time tag CP56Time2a", /* description */
		IEC_TYPE_M_ST_TB,					  			/* object - 32  */
        9,                                        		/* single data size */
		0                                            	/* default sq   */
	},
	{
		"Bitstring of 32 bits with time tag CP56Time2a", /* description */
		IEC_TYPE_M_BO_TB,					  			/* object - 33  */
        12,                                       		/* single data size */
		0                                            	/* default sq   */
	},
	{
		"Single Command", 			                  	/* description  */
		IEC_TYPE_C_SC_NA,					  			/* object - 45  */
        1,                                        		/* single data size */
		1                                            	/* default sq   */
	},
	{
		"Double Command",			                  	/* description  */
		IEC_TYPE_C_DC_NA,			          			/* object - 46  */
        1,                                        		/* single data size */
		0                                            	/* default sq   */
	},
	{
		"Regulating step Command",	                  	/* description  */
		IEC_TYPE_C_RC_NA,			          			/* object - 47  */
        1,                                        		/* single data size */
		0                                            	/* default sq   */
	},
	{
		"Set-point command, normalized value",	      	/* description  */
		IEC_TYPE_C_SE_NA,			          			/* object - 48  */
        3,                                        		/* single data size */
		0                                            	/* default sq   */
	},
	{
		"Set-point command, scaled value",	          	/* description  */
		IEC_TYPE_C_SE_NB,			          			/* object - 49  */
        3,                                        		/* single data size */
		0                                            	/* default sq   */
	},
	{
		"Set-point command, short floating point number",	/* description */
		IEC_TYPE_C_SE_NC,			          			/* object - 50  */
        5,                                        		/* single data size */
		0                                            	/* default sq   */
	},
	{
		"Bitstring 32 bit command",	                  	/* description  */
		IEC_TYPE_C_BO_NA,			          			/* object - 51  */
        4,                                        		/* single data size */
		0                                            	/* default sq   */
	},
	{
		"End of initialization",                      	/* description  */
		IEC_TYPE_M_EI_NA,					  			/* object - 70  */
        1,                                        		/* single data size */
		0                                            	/* default sq   */
	},
	{
		"Interrogation command",                      	/* description  */
		IEC_TYPE_C_IC_NA,					  			/* object - 100 */
        1,                                        		/* single data size */
		0                                            	/* default sq   */
	},
	{
		"Counter Interrogation command",              	/* description  */
		IEC_TYPE_C_CI_NA, 					  			/* object - 101 */
        1,                                        		/* single data size */
		0                                            	/* default sq   */
	},
	{
		"Clock synchronization",                      	/* description  */
		IEC_TYPE_C_CS_NA,					  			/* object - 103 */
        7,                                        		/* single data size */
		0                                            	/* default sq   */
	},
	{
		"Delay acqusition command",                   	/* description  */
		IEC_TYPE_C_CD_NA,					  			/* object - 106 */
        2,                                        		/* single data size */
		0                                            	/* default sq   */
	},
	{
		0,                                            	/* description  */
		0,			                   				  	/* object end   */
        0,                                        		/* single data size */
        0                                            	/* default sq   */
	}
};



/*
*   수신한 ASDU 의 내용을 출력함...
*/
void	checkAppType_IEC(int hostid, int object, IEC_DATA_LINK *iecData)
{
	//int	i;
	int	index;
	
	for(index=0;objTable[index].object != 0;index++) 
	{
        if(object == objTable[index].object) 
        {
        	if(opr->hostDebug == hostid)
        	printf("   rcv OBJ = %2d... %s \n", object, objTable[index].objDesc);	
            break;
        }
    }

}


/*
* ASDU Function : 100, Interrogation command
* ASDU => IEC_TYPE_C_IC_NA, Interrogation command
*/
int	funcASDU_intrrogation(int hostid, IEC_DATA_LINK *iecData)
{
	int	count=0;
	
	HOST_DCB        *host;
	IEC_VAR_FRAME	*rcvFrame, *sndFrame;
	
    host = (HOST_DCB *) hostDCB[hostid];
    rcvFrame = (IEC_VAR_FRAME *) &iecData->rcvFrame2;
    sndFrame = (IEC_VAR_FRAME *) &iecData->sndFrame2;
    
    /* ---------------------------------------- */
    /*  ASDU : 응용서비스 데이터 구조 정의      */
    /* ---------------------------------------- */
    sndFrame->userData[0] = IEC_TYPE_C_IC_NA;		// type
    sndFrame->userData[1] = 0x01;			        // SQ=0, 개별적인 정보구조, 정보객체 수 ...qualifer

    /* ASDU : 전송원인(COT) 영역 */ 
    if(host->rcvCOT == COT_ACTIVATION)              // COT : 기동 
    {	
    	sndFrame->userData[2] = COT_ACTIVATION_TERMINATION;		// COT : Activation Terminate
    }
    else if(host->rcvCOT == COT_DEACTIVATION)       // COT : 정지 
    {
    	sndFrame->userData[2] = COT_DEACTIVATION_CONFIRMATION;		// COT : Deactivation Confirmation
    }			
    
    /* ASDU : 공통 주소 영역 */       
    sndFrame->userData[3] = host->hostid & 0xff;
    sndFrame->userData[4] = (host->hostid >> 8) & 0xff;
    
    /* 정보객체의 주소 */
    sndFrame->userData[5] = 0;				    // Information Object Address
    sndFrame->userData[6] = 0;	                // Information Object Address
    
    /* 정보객체 내용 : 문의 한정자 */
    sndFrame->userData[7] = 20;			        // QOI : Qualifier of interrogation
    
    count = 8;	
    
    /* ASDU : 전송 FRAME 구조 정의 */
	iecData->sndFrameType = IEC_LINK_VARIABLE_TYPE;
		
	return(count);
}

/*
*/
int	checkClock56Time2a(int hostid, byte *timeInfo)
{
	int	year, month, day, week, hour, min, sec, milisec;
	char    buffer[128];
	
	milisec = timeInfo[0] + (timeInfo[1]*256);
	
	sec = milisec / 1000;
	milisec = milisec % 1000;
	
	min = timeInfo[2] & 0x3f;
	hour = timeInfo[3] & 0x1f;
	day  = timeInfo[4] & 0x1f;
	week = (timeInfo[4] >> 5) & 0x07;
	month = timeInfo[5] & 0x0f;
	
	year = timeInfo[6] & 0x7f;
	
	if((year > 99) || (month > 12) || (day > 31) || (hour > 23) || (min > 59) || (sec > 59))	return (0);	
	
	year = year + 2000;
	
	printf(">> rcv Time Info ...%2d/%2d/%2d-%d %02d:%02d:%02d-%3d\n", year, month, day, week, hour, min, sec, milisec);

    /* -------------------------------------------- */
    /*  IEC-HOST : 시각동기 ....                    */
    /* -------------------------------------------- */			
   	opr->year   = year;
    opr->month  = month;
    opr->day    = day;
    opr->week   = week;		// 1:Monday, 2: Tuesday, 3:Wednesday, 4:Thursday, 5:Friday, 6:Saturday, 7:Sunday 
    opr->hour   = hour;
    opr->min    = min;
    opr->sec    = sec;
        
    opr->rtcUpdateFlag = SET;
    
    /* TIME-SYNC 관련 이벤트 생성 */    	
    //logEvent1( opr, sque, EVENT_HOST_TIMESET, hostid, 0, 0, hostid);
    logHistoryQue1( opr, hque, EVENT_HOST_TIMESET, hostid, 0, 0, hostid);
    
    /* -------------------------------------------- */
    /* LOG File 저장                                */
    /* -------------------------------------------- */	
    sprintf(buffer, "IEC> ID=%d, =====> TIMESET: %4d/%2d/%2d %02d:%02d:%02d ",  hostid+1, opr->year, opr->month, opr->day, opr->hour, opr->min, opr->sec);
    LogFile_MPU (opr, EVENT_HOST_TIMESET, buffer, strlen(buffer));  
        
	return (1);
	
}

/*
* ASDU Function : 103, Clock synchronization
* ASDU => IEC_TYPE_C_CS_NA, Clock synchronization
*/
int	funcASDU_clock_sync(int hostid, IEC_DATA_LINK *iecData, byte *appData)
{
	int	count=0;
	int	retVal;
	
	
	HOST_DCB        *host;
	IEC_VAR_FRAME	*rcvFrame, *sndFrame;
	
    host = (HOST_DCB *) hostDCB[hostid];
    rcvFrame = (IEC_VAR_FRAME *) &iecData->rcvFrame2;
    sndFrame = (IEC_VAR_FRAME *) &iecData->sndFrame2;
    
    /* ---------------------------------------- */
    /* 시간정보 분해 및 Time Setting ...        */
    /* ---------------------------------------- */
    retVal = checkClock56Time2a( hostid, (byte *)&appData[2]);
    
    /* ---------------------------------------- */
    /*  ASDU : 응용서비스 데이터 구조 정의      */
    /* ---------------------------------------- */
    sndFrame->userData[0] = IEC_TYPE_C_CS_NA;		// type
    sndFrame->userData[1] = 0x01;			        // SQ=0, 개별적인 정보구조, 정보객체 수 ...qualifer

#if 0    
    /* ASDU : 전송원인(COT) 영역 */ 
    if(host->rcvCOT == COT_ACTIVATION)              // COT : 기동 
    {	
    	sndFrame->userData[2] = COT_ACTIVATION_TERMINATION;		// COT : Activation Terminate
    }
    else
    {
        sndFrame->userData[2] = COT_UNKNOWN_COT_BY_HOST;		// COT : Unknown COT
    }            
        
    /* ASDU : 공통 주소 영역 */       
    sndFrame->userData[3] = host->hostid & 0xff;
    sndFrame->userData[4] = (host->hostid >> 8) & 0xff;
    
    /* 정보객체의 주소 */
    sndFrame->userData[5] = host->rtuAddr & 0xff;				// Information Object Address
    sndFrame->userData[6] = (host->rtuAddr >> 8) & 0xff;	    // Information Object Address
#endif

#if 1    
    /* ---------------------------------------- */
    /*  기존 RTU - 메세지...                    */
    /* ---------------------------------------- */
    sndFrame->userData[2] = COT_SPONTANEOUS;		// COT : Activation Terminate
    
    /* ASDU : 공통 주소 영역 */       
    sndFrame->userData[3] = host->hostid & 0xff;
    sndFrame->userData[4] = (host->hostid >> 8) & 0xff;
    
    /* 정보객체의 주소 */
    sndFrame->userData[5] = 0x65;			// Information Object Address
    sndFrame->userData[6] = 0x00;	        // Information Object Address
#endif
    
    sndFrame->userData[7] = appData[2];				// 56Time2a Clock Info
    sndFrame->userData[8] = appData[3];				// 56Time2a Clock Info
    sndFrame->userData[9] = appData[4];				// 56Time2a Clock Info
    sndFrame->userData[10]= appData[5];				// 56Time2a Clock Info
    sndFrame->userData[11]= appData[6];				// 56Time2a Clock Info
    sndFrame->userData[12]= appData[7];				// 56Time2a Clock Info
    sndFrame->userData[13]= appData[8];				// 56Time2a Clock Info
    
    count = 14;	
    
    /* ASDU : 전송 FRAME 구조 정의 */
	iecData->sndFrameType = IEC_LINK_VARIABLE_TYPE;
		
	return(count);
}

/*
* ASDU Function : 106, Delay Check
* ASDU => IEC_TYPE_C_CD_NA, Clock synchronization
*/
int	funcASDU_delay_check(int hostid, IEC_DATA_LINK *iecData, byte *appData)
{
    int passTime;
	int	count=0;
	//int	retVal;
	
	
	HOST_DCB        *host;
	IEC_VAR_FRAME	*rcvFrame, *sndFrame;
	
    host = (HOST_DCB *) hostDCB[hostid];
    rcvFrame = (IEC_VAR_FRAME *) &iecData->rcvFrame2;
    sndFrame = (IEC_VAR_FRAME *) &iecData->sndFrame2;
    
    /* ---------------------------------------- */
    /*  ASDU : 응용서비스 데이터 구조 정의      */
    /* ---------------------------------------- */
    sndFrame->userData[0] = IEC_TYPE_C_CD_NA;		// type
    sndFrame->userData[1] = 0x01;			        // SQ=0, 개별적인 정보구조, 정보객체 수 ...qualifer
    
    /* ASDU : 전송원인(COT) 영역 */ 
   	sndFrame->userData[2] = COT_ACTIVATION_TERMINATION;		// COT : Activation Terminate
        
    /* ASDU : 공통 주소 영역 */       
    sndFrame->userData[3] = host->hostid & 0xff;
    sndFrame->userData[4] = (host->hostid >> 8) & 0xff;
    
    /* 정보객체의 주소 */
    sndFrame->userData[5] = 0;				    // Information Object Address
    sndFrame->userData[6] = 0;	                // Information Object Address
    
    /* 경과시간.... */
    passTime = 1000;            // 경과시간 : 1ch (1000ms)
    
    sndFrame->userData[7] = passTime & 0xff;	    // CP16Time2a
    sndFrame->userData[8] = (passTime >> 8) & 0xff;	// CP16Time2a
    
    count = 9;	
    
    /* ASDU : 전송 FRAME 구조 정의 */
	iecData->sndFrameType = IEC_LINK_VARIABLE_TYPE;
		
	return(count);
}


/*
* CLASS 2 응답함수
* ASDU Function : 상태정보- No Time
* ASDU => IEC_TYPE_M_SP_NA, Single-point information, without time tag 
*/
int	funcASDU_M_SP_NA(int hostid, IEC_DATA_LINK *iecData)
{
	int	i;
	int	count=0;
	
	HOST_DCB        *host;
	IEC_VAR_FRAME	*sndFrame;
	DIPOINT_INFO	*stsPoint;
	
    host = (HOST_DCB *) hostDCB[hostid];
    
    /* ---------------------------------------- */
    /*  상태포인트 #1 정보 추출                 */
    /* ---------------------------------------- */
    stsPoint = (DIPOINT_INFO *) &host->stateInfo[0];
    
    sndFrame = (IEC_VAR_FRAME *) &iecData->sndFrame2;           // 가변 Frame 구조
    
    /* ---------------------------------------- */
    /*  ASDU : 응용서비스 데이터 구조 정의      */
    /* ---------------------------------------- */
    sndFrame->userData[0] = IEC_TYPE_M_SP_NA;					// type
    sndFrame->userData[1] = (APPL_SQ_MASK | host->diPtNum);		// SQ=1, 연속적인 정보구조, 정보객체 수 ...qualifer

	/* ASDU : 전송원인(COT) 영역 */ 
	if(host->rcvCOT == COT_REQUEST_OR_REQUESTED)        sndFrame->userData[2] = COT_REQUEST_OR_REQUESTED;	    // COT : 요구함 또는 요구됨
	else if(host->rcvCOT == COT_RETURN_INFO_BY_REMOTE)  sndFrame->userData[2] = COT_RETURN_INFO_BY_REMOTE;	    // COT : 원격명령에 의한 복귀
	else                                                sndFrame->userData[2] = COT_REQUEST_BY_STATION;         // COT : interrgated by station, 일반문의
    
    /* ASDU : 공통 주소 영역 */
    sndFrame->userData[3] = host->hostid & 0xff;
    sndFrame->userData[4] = (host->hostid >> 8) & 0xff;
    
    /* ---------------------------------------- */
    /* ASDU : 정보객체 구조 정의                */
    /*   - SQ=1의 경우 : 시작 포인트 주소       */
    /* ---------------------------------------- */
    sndFrame->userData[5] = stsPoint->iecIndex & 0xff;
    sndFrame->userData[6] = (stsPoint->iecIndex >> 8) & 0xff;
    
    count = 7;
    
    /* 전체 포인트에 대한 상태정보 Upload... */
    for(i=0; i < host->diPtNum; i++)
    {
        /* 주제화 Bit ... SET */
    	//sndFrame->userData[count++] = host->iec_singleData[i] | IEC_PT_NT_BIT;       // 정보객체 : 단점정보 Upoad...
    	sndFrame->userData[count++] = host->iec_singleData[i];      
    }
    
    /* ASDU : 전송 FRAME 구조 정의 */
	iecData->sndFrameType = IEC_LINK_VARIABLE_TYPE;
		
	return(count);
}

/*
* CLASS 1 응답함수 (SOE)
* ASDU Function : 상태정보- with Time 
* ASDU => IEC_TYPE_M_SP_TA, Single-point information, without time tag 
*/
int	funcASDU_M_SP_TA(int hostid, IEC_DATA_LINK *iecData)
{
	int	i;
	int	count=0;
	byte	front, rear, soeCount;
	
	HOST_DCB        *host;
	IEC_VAR_FRAME	*sndFrame;
	//DIPOINT_INFO	*stsPoint;
	IEC_SOE_QUEUE   *soeQ;  
	IEC_SOEQ_ENTRY  *entryIEC;
	
    host = (HOST_DCB *) hostDCB[hostid];
    
    /* ------------------------------------ */
    /*  SOE Event : Class Assign...         */
    /* ------------------------------------ */    
    soeQ    = (IEC_SOE_QUEUE *) &host->iecSOEQ;
    front   = soeQ->front& 0xff;
    rear    = soeQ->rear & 0xff;
    
    soeCount = (front - rear + 256) % 256;
    if(soeCount == 0)	return (0);
    
    sndFrame = (IEC_VAR_FRAME *) &iecData->sndFrame2;       // 가변 Frame 구조
    
    /* ---------------------------------------- */
    /*  ASDU : 응용서비스 데이터 구조 정의      */
    /* ---------------------------------------- */
    sndFrame->userData[0] = IEC_TYPE_M_SP_TA;	            // type
    sndFrame->userData[1] = soeCount;			            // SQ=0, 개별적인 정보구조, 정보객체 수 ...qualifer

    /* ASDU : 전송원인(COT) 영역 */ 
	if(host->rcvCOT == COT_REQUEST_OR_REQUESTED)        sndFrame->userData[2] = COT_REQUEST_OR_REQUESTED;	    // COT : 요구함 또는 요구됨
	else if(host->rcvCOT == COT_RETURN_INFO_BY_REMOTE)  sndFrame->userData[2] = COT_RETURN_INFO_BY_REMOTE;	    // COT : 원격명령에 의한 복귀
	else                                                sndFrame->userData[2] = COT_REQUEST_BY_STATION;         // COT : interrgated by station, 일반문의
    
    /* ASDU : 공통 주소 영역 */
    sndFrame->userData[3] = host->hostid & 0xff;
    sndFrame->userData[4] = (host->hostid >> 8) & 0xff;
    
    count = 5;
    
    /* ---------------------------------------- */
    /*  SOE 정보 Upload...                      */
    /* ---------------------------------------- */
    for(i=0; i < soeCount; i++)
    {
    	entryIEC = ( IEC_SOEQ_ENTRY *) &soeQ->soeQueue[rear];

    	/* 정보객체의 주소 */
    	sndFrame->userData[count++] = entryIEC->baseAddr & 0xff;
    	sndFrame->userData[count++] = (entryIEC->baseAddr >> 8) & 0xff;
    	
    	//sndFrame->userData[count++] = entryIEC->status | IEC_PT_NT_BIT;     // 정보객체 상태 : 주제화 단점정보 Upoad...
    	sndFrame->userData[count++] = entryIEC->status;     
    	
    	sndFrame->userData[count++] = entryIEC->soeTime[0];                 // 정보객체 시간 : CP24Time2a 구조...
    	sndFrame->userData[count++] = entryIEC->soeTime[1];
    	sndFrame->userData[count++] = entryIEC->soeTime[2];
    	
    	rear = (rear + 1) & 0xff;
    	
    	/* 전송 사이즈 제한... */
    	if(i >= 63) break;
    }
    
    /* ---------------------------------------- */
    /* SOE-Queue 포인트 Update ...              */
    /* ---------------------------------------- */
    soeQ->rear = rear;
    
    /* ASDU : 전송 FRAME 구조 정의 */
	iecData->sndFrameType = IEC_LINK_VARIABLE_TYPE;
		
	return(count);
}

/*
* ASDU Function : 11, Measured value, normalized,  without time tag
* ASDU => IEC_TYPE_M_ME_NB, Measured value, normalized,  without time tag
*/
int	funcASDU_normal_measure_NB(int hostid, IEC_DATA_LINK *iecData)
{
	int	i;
	int	count=0;
	//int aiValue;
	
	HOST_DCB        *host;
	IEC_VAR_FRAME	*sndFrame;
	AIPOINT_INFO	*anaPoint;
	
	DNP_ANA_INPUT   *anaData;
    
    host = (HOST_DCB *) hostDCB[hostid];
    
    /* ---------------------------------------- */
    /*  상태포인트 #1 정보 추출                 */
    /* ---------------------------------------- */
    anaPoint = (AIPOINT_INFO *) &host->analogInfo[0];
    
    sndFrame = (IEC_VAR_FRAME *) &iecData->sndFrame2;           // 가변 Frame 구조
    
    /* ---------------------------------------- */
    /*  ASDU : 응용서비스 데이터 구조 정의      */
    /* ---------------------------------------- */
    //sndFrame->userData[0] = IEC_TYPE_M_ME_NA;					// type 9, 정규화값
    sndFrame->userData[0] = IEC_TYPE_M_ME_NB;					// type 11, 척도화 값
    
    sndFrame->userData[1] = (APPL_SQ_MASK | host->aiPtNum);		// SQ=1, 연속적인 정보구조, 정보객체 수 ...qualifer

	/* ASDU : 전송원인(COT) 영역 */ 
	if(host->rcvCOT == COT_REQUEST_OR_REQUESTED)        sndFrame->userData[2] = COT_REQUEST_OR_REQUESTED;	    // COT : 요구함 또는 요구됨
	else if(host->rcvCOT == COT_RETURN_INFO_BY_REMOTE)  sndFrame->userData[2] = COT_RETURN_INFO_BY_REMOTE;	    // COT : 원격명령에 의한 복귀
	else                                                sndFrame->userData[2] = COT_REQUEST_OR_REQUESTED;       // COT : interrgated by station, 일반문의
    
    /* ASDU : 공통 주소 영역 */
    sndFrame->userData[3] = host->hostid & 0xff;
    sndFrame->userData[4] = (host->hostid >> 8) & 0xff;
    
    /* ---------------------------------------- */
    /* ASDU : 정보객체 구조 정의                */
    /*   - SQ=1의 경우 : 시작 포인트 주소       */
    /* ---------------------------------------- */
    sndFrame->userData[5] = anaPoint->iecIndex & 0xff;
    sndFrame->userData[6] = (anaPoint->iecIndex >> 8) & 0xff;
    
    count = 7;

    /* ---------------------------------------- */    
    /* 전체 포인트에 대한 상태정보 Upload...    */
    /* ---------------------------------------- */  
    for(i=0; i < host->aiPtNum; i++)
    {
        anaData = (DNP_ANA_INPUT *) &host->ana_pointData[i];
        
        /* 주제화 Bit ... SET */
    	sndFrame->userData[count++] = anaData->pointData & 0xff;;           // 정보객체 : 계측정보 Upoad...
    	sndFrame->userData[count++] = (anaData->pointData >> 8) & 0xff;;    // 정보객체 : 계측정보 Upoad...
    	
    	sndFrame->userData[count++] = 0;       // 정보객체 : 포인트 상태
    }
    
    /* ASDU : 전송 FRAME 구조 정의 */
	iecData->sndFrameType = IEC_LINK_VARIABLE_TYPE;

	return(count);
}

/*
* ASDU Function : 45, Single Command 
* ASDU => IEC_TYPE_C_SC_NA, Single Command
*/
int	funcASDU_single_command(int hostid, IEC_DATA_LINK *iecData, byte *appData)
{
	int	i,j;
	int	count=0;
	int	chkMode;
	word	infoAddr;
	byte	cntrData;
	byte	state, qoc, cntrMode;
	char    buffer[128];
	
	HOST_DCB        *host;
	IEC_VAR_FRAME	*rcvFrame, *sndFrame;
	DNP_DEVICE  *dnp; 
	POINT_BUF		*doPoint;
	
    host = (HOST_DCB *) hostDCB[hostid];
    
    /* -------------------------------------------- */
    /* 2014.10.01 제어종료 명령 전송을 위하여...    */
    /* -------------------------------------------- */
    if(opr->iecControl[hostid] == CONTROL_OPERATE)  rcvFrame = (IEC_VAR_FRAME *) &iecData->rcvControl;      // 이전 제어명령 수신정보
    else                                            rcvFrame = (IEC_VAR_FRAME *) &iecData->rcvFrame2;       // 현재 수신정보
    
    /* -------------------------------------------- */
    /*  정보객체 분석 : 제어정보 추출               */
    /* -------------------------------------------- */
    infoAddr = appData[0] + (appData[1] * 256);	            // Address of INFO
    cntrData = appData[2];						            // Control Info

	state = cntrData & 0x03;                                // 제어상태 : 0 - OFF, 1 - ON
	qoc   = (cntrData >> 3) & 0x1f;                         // 제어동작시간
	cntrMode  = (cntrData & IEC_CNTR_SE_BIT);               // 0x00: 실행, 0x80 : 선택

	/* -------------------------------------------- */
	/*  IEC-HOST 제어 실행시...	                    */
	/*   - Select 수신 : 제어준비 & Confirm         */
	/*   - Operate수신 : 제어실행 & Confirm         */
	/*   - Operatet실행: Termination 전송           */
	/* -------------------------------------------- */
	if(opr->iecControl[hostid] != CONTROL_OPERATE)  
    {	
        if(opr->soeDebug)
        {      
            printf("\n-------------------------------------------------------[%02d:%02d:%02d]\n", rtc->hour, rtc->min, rtc->sec);	
    	    printf(">> host(%2d) IEC Control Info(single).... Base = %d, state=%d, QOC=%d, mode=%02x\n", hostid+1,infoAddr, state, qoc, cntrMode);
	    }    
	    
	    /* -------------------------------------------- */
	    /*  제어실행명령 수신시... 모듈부 제어 수행     */
	    /* -------------------------------------------- */
    	if(cntrMode == 0x00)	
	    {	
    		chkMode = 0;
        	for(i=0; i< MAX_MODULE; i++)
	        {
        		dnp = (DNP_DEVICE *) dnpcfg[i];
    	
	        	if((dnp->type != TYPE_DOM) && (dnp->type != TYPE_DIOM))	continue;
    	
	        	for(j=0; j < MAX_DEV_DO_POINT; j++)
        		{
        			doPoint = (POINT_BUF *) &dnp->doPtBuf[j];
    	    		if(doPoint->config == RESET)	continue;
    		
	    	    	if(doPoint->modBase == infoAddr)
        			{
        			    if(doPoint->devType == CNTR_POINT)
        			    {
        			        if(linkCfg->offline == RESET)
                            {     
                                if(state == 0)                 // TRIP 제어 , CPU-A 동작
                                {
                                    if((opr->cpuMode == CPU_SLAVE) && (opr->runMode == LOCAL_MASTER))
                                    {
                                        //opr->cpuChange = SET;        
                                        opr->IEC_cpuChange = SET;
                                        opr->IEC_ChangeTick= 0;
                                        
                                        logEvent1(opr, sque, EVENT_CHANGE_CPU, 20, 0, 0, hostid);    
                                        logHistoryQue1(opr, hque, EVENT_CHANGE_CPU, 20, 0, 0, hostid);    
		                                Debug(console,"hrcv%2d> **** Change CPU  : CPU-B => [CPU-A] !\n", hostid+1);   
		                                
		                                /* -------------------------------- */
                                        /* LOG File 저장                    */
                                        /* -------------------------------- */
                                        sprintf(buffer, "<-- IEC> OPERATE...Host=%2d, CPU Change Control : CPU-A SET ", hostid+1);
                                        LogFile_MPU (opr, EVENT_HOST_CNTR, buffer, strlen(buffer));   
                                    }       
                                }    
                                else if(state == 1)            // CLOSE제어 , CPU-B 동작    
                                {
                                    if((opr->cpuMode == CPU_MASTER) && (opr->runMode == LOCAL_MASTER))
                                    {
                                        //opr->cpuChange = SET;        
                                        opr->IEC_cpuChange = SET;
                                        opr->IEC_ChangeTick= 0;
                                    
                                        logEvent1(opr, sque, EVENT_CHANGE_CPU, 10, 0, 0, hostid);  
                                        logHistoryQue1(opr, hque, EVENT_CHANGE_CPU, 10, 0, 0, hostid);          
	    	                            Debug(console,"hrcv%2d> **** Change CPU  : CPU-A => [CPU-B] !\n", hostid+1);   
	    	                            
	    	                            /* -------------------------------- */
                                        /* LOG File 저장                    */
                                        /* -------------------------------- */
                                        sprintf(buffer, "<-- IEC> OPERATE...Host=%2d, CPU Change Control : CPU-B SET ", hostid+1);
                                        LogFile_MPU (opr, EVENT_HOST_CNTR, buffer, strlen(buffer));   
                                    }  
                                }
                            }
                            
                            if(opr->soeDebug)
                            Debug(console,"hrcv%2d> SYSTEM-OPERATE : CPU Change ... state =%2x\n", hostid+1, state);
                            
                            /* IEC-HOST 제어명령 실행후... 종료전송... */
        	                opr->iecControl[hostid] = CONTROL_OPERATE;
        	            
        			        break;
        			    }
        			    else
        			    {           
            				dnp->cntPoint = j;
                    	    if(state == 0)			dnp->cntTcf   = TRIP_CONTROL;   	/* TRIP */
                        	else if(state == 1)		dnp->cntTcf   = CLOSE_CONTROL;   	/* CLOSE */	
                	    
    	                    dnp->selectReq  = SET;
	                
	                        chkMode = SET;
	                    
	                        if(opr->soeDebug)
        	                printf(">> host(%2d) Control-OPERATE(single)....dev=%d, point=%d... Base = %d, state=%d, QOC=%d\n", hostid+1, i+1, j+1, infoAddr, state, qoc);
        	            
        	                /* -------------------------------- */
                            /* LOG File 저장                    */
                            /* -------------------------------- */
                            sprintf(buffer, "<-- IEC> SINGLE-OPERATE...Host=%2d, Device=%d, Point=%d, TCF=%02x ", hostid+1, i+1, j+1, dnp->cntTcf);
                            LogFile_MPU (opr, EVENT_HOST_CNTR, buffer, strlen(buffer));   
                                        
            	            /* IEC-HOST 제어명령 실행후... 종료전송... */
            	            opr->iecControl[hostid] = CONTROL_OPERATE;
        	            
            	            break;
	                    }
	                }
            	}
    		}
    	}
        else
        {
            opr->iecControl[hostid] = CONTROL_SELECT;
            if(opr->soeDebug)
        	printf(">> host(%2d) Control-SELECT(single)....Base = %d, state=%d, QOC=%d\n", hostid+1, infoAddr, state, qoc);
        }
        
      	/* ---------------------------------------- */
        /*	응답 메세지 구성 ...					*/
        /* ---------------------------------------- */	
        sndFrame = (IEC_VAR_FRAME *) &iecData->sndFrame2;

        sndFrame->userData[0] = IEC_TYPE_C_SC_NA;		// type
        sndFrame->userData[1] = 0x01;					// qualifer
    
        /* ASDU : 전송원인(COT) 영역 */ 
        if(host->rcvCOT == COT_ACTIVATION)              // COT : 기동 
        {	
            /* ------------------------------  */
            /* 제어명령 수신 ...기동확인 전송  */
            /* ------------------------------  */    	
            sndFrame->userData[2] = COT_ACTIVATION_CONFIRMATION;		// COT : Activation Confirm
        }
        else if(host->rcvCOT == COT_DEACTIVATION)       // COT : 정지 
        {
    	    sndFrame->userData[2] = COT_DEACTIVATION_CONFIRMATION;		// COT : Deactivation Confirmation
        }			
    
        /* ASDU : 공통 주소 영역 */
        sndFrame->userData[3] = host->hostid & 0xff;				// Common Addres of ASDU
        sndFrame->userData[4] = (host->hostid >> 8) & 0xff;			// Common Addres of ASDU
    
        /* 정보객체의 주소 */
        sndFrame->userData[5] = infoAddr & 0xff;					// Information Object Address
        sndFrame->userData[6] = (infoAddr >> 8) & 0xff;			// Information Object Address
    
        sndFrame->userData[7] = cntrData;							// SOC
    
        count = 8;	
    
        /* ---------------------------- */
        /* ASDU : 전송 FRAME 구조 정의  */
        /* ---------------------------- */
	    iecData->sndFrameType = IEC_LINK_VARIABLE_TYPE;
		
		return(count);
    }
    else
    {
   	    printf(">> host(%2d) IEC Control Terminate(single).... Base = %d, state=%d, QOC=%d, mode=%02x\n", hostid+1, infoAddr, state, qoc, cntrMode);

     	/* ---------------------------------------- */
        /*	응답 메세지 구성 ...					*/
        /* ---------------------------------------- */	
        sndFrame = (IEC_VAR_FRAME *) &iecData->sndFrame2;

        sndFrame->userData[0] = IEC_TYPE_C_SC_NA;		// type
        sndFrame->userData[1] = 0x01;					// qualifer
    
        /* ------------------------------  */
        /* 제어명령 수행후 ...기동종료 전송*/
        /* ------------------------------  */  
        sndFrame->userData[2] = COT_ACTIVATION_TERMINATION;		// COT : Activation Terminate
    
        /* ASDU : 공통 주소 영역 */
        sndFrame->userData[3] = host->hostid & 0xff;				// Common Addres of ASDU
        sndFrame->userData[4] = (host->hostid >> 8) & 0xff;			// Common Addres of ASDU
    
        /* 정보객체의 주소 */
        sndFrame->userData[5] = infoAddr & 0xff;					// Information Object Address
        sndFrame->userData[6] = (infoAddr >> 8) & 0xff;			// Information Object Address
    
        sndFrame->userData[7] = cntrData;							// SOC
    
        count = 8;	
    
        /* ---------------------------- */
        /* ASDU : 전송 FRAME 구조 정의  */
        /* ---------------------------- */
	    iecData->sndFrameType = IEC_LINK_VARIABLE_TYPE;
		
	    return(count);
		      
    }
    
}


/*
* ASDU Function : 46, Double Command 
* ASDU => IEC_TYPE_C_DC_NA, Double Command
*/
int	funcASDU_double_command(int hostid, IEC_DATA_LINK *iecData, byte *appData)
{
	int	i,j;
	int	count=0;
	int	chkMode;
	word	infoAddr;
	byte	cntrData;
	byte	state, qoc, cntrMode;
	char    buffer[128];
	
	HOST_DCB        *host;
	IEC_VAR_FRAME	*rcvFrame, *sndFrame;
	DNP_DEVICE  *dnp; 
	POINT_BUF		*doPoint;
	
    host = (HOST_DCB *) hostDCB[hostid];

    /* -------------------------------------------- */
    /* 2014.10.01 제어종료 명령 전송을 위하여...    */
    /* -------------------------------------------- */
    if(opr->iecControl[hostid] == CONTROL_OPERATE)  rcvFrame = (IEC_VAR_FRAME *) &iecData->rcvControl;      // 이전 제어명령 수신정보
    else                                            rcvFrame = (IEC_VAR_FRAME *) &iecData->rcvFrame2;       // 현재 수신정보
    
    /* -------------------------------------------- */
    /*  정보객체 분석 : 제어정보 추출               */
    /* -------------------------------------------- */
    infoAddr = appData[0] + (appData[1] * 256);	            // Address of INFO
    cntrData = appData[2];						            // Control Info

	state = cntrData & 0x0f;                                // 제어상태 : 0 - OFF, 1 - ON
	qoc   = (cntrData >> 2) & 0x1f;                         // 제어동작시간
	cntrMode  = (cntrData & IEC_CNTR_SE_BIT);               // 0x00: 실행, 0x80 : 선택

	/* -------------------------------------------- */
	/*  IEC-HOST 제어 실행시...	                    */
	/*   - Select 수신 : 제어준비 & Confirm         */
	/*   - Operate수신 : 제어실행 & Confirm         */
	/*   - Operatet실행: Termination 전송           */
	/* -------------------------------------------- */
	if(opr->iecControl[hostid] != CONTROL_OPERATE)  
    {	
        if(opr->soeDebug)
        {      
            printf("\n-------------------------------------------------------[%02d:%02d:%02d]\n", rtc->hour, rtc->min, rtc->sec);	
    	    printf(">> host(%2d) IEC Control Info(double).... Base = %d, state=%d, QOC=%d, mode=%02x\n", hostid+1, infoAddr, state, qoc, cntrMode);
	    }    
	    
	    /* -------------------------------------------- */
	    /*  제어실행명령 수신시... 모듈부 제어 수행     */
	    /* -------------------------------------------- */
    	if(cntrMode == 0x00)	
	    {	
    		chkMode = 0;
        	for(i=0; i< MAX_MODULE; i++)
	        {
        		dnp = (DNP_DEVICE *) dnpcfg[i];
    	
	        	if((dnp->type != TYPE_DOM) && (dnp->type != TYPE_DIOM))	continue;
    	
	        	for(j=0; j < MAX_DEV_DO_POINT; j++)
        		{
        			doPoint = (POINT_BUF *) &dnp->doPtBuf[j];
    	    		if(doPoint->config == RESET)	continue;
    		
	    	    	if(doPoint->modBase == infoAddr)
        			{
                        if(doPoint->devType == CNTR_POINT)
        			    {
        			        if(linkCfg->offline == RESET)
                            {     
                                if(state == 0)                 // TRIP 제어 , CPU-A 동작
                                {
                                    if((opr->cpuMode == CPU_SLAVE) && (opr->runMode == LOCAL_MASTER))
                                    {
                                        //opr->cpuChange = SET;        
                                        opr->IEC_cpuChange = SET; 
                                        opr->IEC_ChangeTick= 0;       
                                    
                                        logEvent1(opr, sque, EVENT_CHANGE_CPU, 20, 0, 0, hostid);    
                                        logHistoryQue1(opr, hque, EVENT_CHANGE_CPU, 20, 0, 0, hostid);    
		                                Debug(console,"hrcv%2d> **** Change CPU  : CPU-B => [CPU-A] !\n", hostid+1);   
		                                
		                                /* -------------------------------- */
                                        /* LOG File 저장                    */
                                        /* -------------------------------- */
                                        sprintf(buffer, "<-- IEC> OPERATE...Host=%2d, CPU Change Control : CPU-A SET ", hostid+1);
                                        LogFile_MPU (opr, EVENT_HOST_CNTR, buffer, strlen(buffer));   
                                    }       
                                }    
                                else if(state == 1)            // CLOSE제어 , CPU-B 동작    
                                {
                                    if((opr->cpuMode == CPU_MASTER) && (opr->runMode == LOCAL_MASTER))
                                    {
                                        //opr->cpuChange = SET;        
                                        opr->IEC_cpuChange = SET;
                                        opr->IEC_ChangeTick= 0;
                                    
                                        logEvent1(opr, sque, EVENT_CHANGE_CPU, 10, 0, 0, hostid);  
                                        logHistoryQue1(opr, hque, EVENT_CHANGE_CPU, 10, 0, 0, hostid);          
	    	                            Debug(console,"hrcv%2d> **** Change CPU  : CPU-A => [CPU-B] !\n", hostid+1);   
	    	                            
	    	                            /* -------------------------------- */
                                        /* LOG File 저장                    */
                                        /* -------------------------------- */
                                        sprintf(buffer, "<-- IEC> OPERATE...Host=%2d, CPU Change Control : CPU-B SET ", hostid+1);
                                        LogFile_MPU (opr, EVENT_HOST_CNTR, buffer, strlen(buffer));   
                                    }  
                                }
                            }
                            
                            if(opr->soeDebug)
                            Debug(console,"hrcv%2d> SYSTEM-OPERATE : CPU Change ... state =%2x\n", hostid+1, state);
                            
                            /* IEC-HOST 제어명령 실행후... 종료전송... */
        	                opr->iecControl[hostid] = CONTROL_OPERATE;
        	            
        			        break;
        			    }
        			    else
        			    {           
            				dnp->cntPoint = j;
                    	    if(state == 0)			dnp->cntTcf   = TRIP_CONTROL;   	/* TRIP */
                        	else if(state == 1)		dnp->cntTcf   = CLOSE_CONTROL;   	/* CLOSE */	
                	    
    	                    dnp->selectReq  = SET;
	                
	                        chkMode = SET;
	                    
	                        if(opr->soeDebug)
        	                printf(">> host(%2d) Control-OPERATE(double)....dev=%d, point=%d... Base = %d, state=%d, QOC=%d\n", hostid+1, i+1, j+1, infoAddr, state, qoc);
        	            
        	                /* -------------------------------- */
                            /* LOG File 저장                    */
                            /* -------------------------------- */
                            sprintf(buffer, "<-- IEC> DOUBLE-OPERATE...Host=%2d, Device=%d, Point=%d, TCF=%02x ", hostid+1, i+1, j+1, dnp->cntTcf);
                            LogFile_MPU (opr, EVENT_HOST_CNTR, buffer, strlen(buffer));   
                            
            	            /* IEC-HOST 제어명령 실행후... 종료전송... */
            	            opr->iecControl[hostid] = CONTROL_OPERATE;
        	            
            	            break;
	                    }
	                }
            	}
    		}
    	}
        else
        {
            opr->iecControl[hostid] = CONTROL_SELECT;
            if(opr->soeDebug)
        	printf(">> host(%2d) Control-SELECT(double)....Base = %d, state=%d, QOC=%d\n", hostid+1, infoAddr, state, qoc);
        }
        
      	/* ---------------------------------------- */
        /*	응답 메세지 구성 ...					*/
        /* ---------------------------------------- */	
        sndFrame = (IEC_VAR_FRAME *) &iecData->sndFrame2;

        sndFrame->userData[0] = IEC_TYPE_C_DC_NA;		// type
        sndFrame->userData[1] = 0x01;					// qualifer
    
        /* ASDU : 전송원인(COT) 영역 */ 
        if(host->rcvCOT == COT_ACTIVATION)              // COT : 기동 
        {	
            /* ------------------------------  */
            /* 제어명령 수신 ...기동확인 전송  */
            /* ------------------------------  */    	
            sndFrame->userData[2] = COT_ACTIVATION_CONFIRMATION;		// COT : Activation Confirm
        }
        else if(host->rcvCOT == COT_DEACTIVATION)       // COT : 정지 
        {
    	    sndFrame->userData[2] = COT_DEACTIVATION_CONFIRMATION;		// COT : Deactivation Confirmation
        }			
    
        /* ASDU : 공통 주소 영역 */
        sndFrame->userData[3] = host->hostid & 0xff;				// Common Addres of ASDU
        sndFrame->userData[4] = (host->hostid >> 8) & 0xff;			// Common Addres of ASDU
    
        /* 정보객체의 주소 */
        sndFrame->userData[5] = infoAddr & 0xff;					// Information Object Address
        sndFrame->userData[6] = (infoAddr >> 8) & 0xff;			// Information Object Address
    
        sndFrame->userData[7] = cntrData;							// SOC
    
        count = 8;	
    
        /* ---------------------------- */
        /* ASDU : 전송 FRAME 구조 정의  */
        /* ---------------------------- */
	    iecData->sndFrameType = IEC_LINK_VARIABLE_TYPE;

        return(count);		
    }
    else
    {
   	    printf(">> host(%2d) IEC Control Terminate(double).... Base = %d, state=%d, QOC=%d, mode=%02x\n", hostid+1, infoAddr, state, qoc, cntrMode);

     	/* ---------------------------------------- */
        /*	응답 메세지 구성 ...					*/
        /* ---------------------------------------- */	
        sndFrame = (IEC_VAR_FRAME *) &iecData->sndFrame2;

        sndFrame->userData[0] = IEC_TYPE_C_DC_NA;		// type
        sndFrame->userData[1] = 0x01;					// qualifer
    
        sndFrame->userData[2] = COT_ACTIVATION_TERMINATION;		// COT : Activation Terminate
    
        /* ASDU : 공통 주소 영역 */
        sndFrame->userData[3] = host->hostid & 0xff;				// Common Addres of ASDU
        sndFrame->userData[4] = (host->hostid >> 8) & 0xff;			// Common Addres of ASDU
    
        /* 정보객체의 주소 */
        sndFrame->userData[5] = infoAddr & 0xff;					// Information Object Address
        sndFrame->userData[6] = (infoAddr >> 8) & 0xff;			// Information Object Address
    
        sndFrame->userData[7] = cntrData;							// SOC
    
        count = 8;	
    
        /* ---------------------------- */
        /* ASDU : 전송 FRAME 구조 정의  */
        /* ---------------------------- */
	    iecData->sndFrameType = IEC_LINK_VARIABLE_TYPE;
		
	    return(count);
		      
    }
    
}


/*
* ASDU Function : 70, End of initialization 
* ASDU => IEC_TYPE_M_EI_NA, End of initialization 
*/
int	funcASDU_endof_initial(int hostid, IEC_DATA_LINK *iecData, byte *appData)
{
	//int	i,j;
	int	count=0;
	//int	chkMode;
	//word	infoAddr;
	//byte	cntrData;
	//byte	state, qoc, cntrMode;
	
	HOST_DCB        *host;
	IEC_VAR_FRAME	*rcvFrame, *sndFrame;
	//DNP_DEVICE  *dnp; 
	//POINT_BUF		*doPoint;
	
    host = (HOST_DCB *) hostDCB[hostid];
    
    rcvFrame = (IEC_VAR_FRAME *) &iecData->rcvFrame2;
    
 	/* ---------------------------------------- */
    /*	응답 메세지 구성 ...					*/
    /* ---------------------------------------- */	
    sndFrame = (IEC_VAR_FRAME *) &iecData->sndFrame2;

    sndFrame->userData[0] = IEC_TYPE_M_EI_NA;		// type
    sndFrame->userData[1] = 0x01;					// qualifer
    
    /* ASDU : 전송원인(COT) 영역 */ 
    sndFrame->userData[2] = COT_INITIALIZED;		// COT : 초기화
    
    /* ASDU : 공통 주소 영역 */
    sndFrame->userData[3] = host->hostid & 0xff;				// Common Addres of ASDU
    sndFrame->userData[4] = (host->hostid >> 8) & 0xff;			// Common Addres of ASDU
    
    /* 정보객체의 주소 */
    sndFrame->userData[5] = host->rtuAddr & 0xff;				// Information Object Address
    sndFrame->userData[6] = (host->rtuAddr  >> 8 ) & 0xff;	    // Information Object Address
    
    /* 정보객체 내용 : 초기화 원인 */
    sndFrame->userData[7] = 0;					    // 초기화 원인 : 전원 스위치 ON
    
    count = 8;	
    
    /* ASDU : 전송 FRAME 구조 정의 */
	iecData->sndFrameType = IEC_LINK_VARIABLE_TYPE;
		
	return(count);
}
