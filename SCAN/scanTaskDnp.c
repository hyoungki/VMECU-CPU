
#include	"localLib.h"
#include    "external.h"

extern  int     dnpChanWrite(int fid, int devid, byte *sndbuf, int sndCount);
extern  void    vmeScanInitial(int scanid, int chid);
extern  int     vmeScanChanReset(int devid);

extern  int     setOffline( int devid);
extern  int     setOnline( int devid);

extern  int     sendAppFrame(int mode, int scanid, int devid, DNP_APP_FRAME *sndAppDnp);

extern  int     dnpChanRead(int fid, int devid, byte *rxbuf, int reqCount);
extern  int     readDataLink( int scanid, int devid, DNP_DATA_LINK *dnpData);
extern  int     chkRcvFrame ( int mode, int scanid, int devid, DNP_DATA_LINK *dnpData);

extern  int     checkDataLink(int mode, int scanid, int devid, DNP_DATA_LINK *dnpData);

extern  TTY_DESC scanChan[MAX_CPUPORT];

extern  int     termExec;
extern  TASK_INFO	*taskPtr;

extern  PDPDTIME_DATE_TIME      dnpRTC;
extern  PDPDTIME_MS_SINCE_70    dnpRTCInfo;

/* ---------------------------------------- */
/*  VMEBUS 관련 변수 초기화                 */
/* ---------------------------------------- */
extern  VME_SIODCB      *vmeSioDCB;
extern  VME_CHAN_DCB    *vmeChanDCB[SIO_CHANNEL_MAX];

/***************************************************
*   FUNCTION : sendConfirm()
*   send DataLink Layer : make send frame & send
***************************************************/
int   sendConfirm( int mode, int scanid, int devid, int code)
{
    //int     i,j,k;
    int     txcnt;
    int     rxVal;
    byte    *txbuf;
    word    crc;

    DL_FRAME    *sndFrame;
    DNP_DEVICE  *dnp;

    dnp = (DNP_DEVICE *) dnpcfg[devid];

    //sndFrame = (DL_FRAME *) &dataLinkFrame[fid].sndFrame;
    sndFrame = (DL_FRAME *) &dnp->dataLinkFrame.sndFrame;
    txbuf = (byte *) &sndFrame->start1;

    /* ------------------------ */
    /*  Make HEADER frame ...   */
    /* ------------------------ */
    txcnt = 0;

    txbuf[txcnt++] = 0x05;
    txbuf[txcnt++] = 0x64;
    txbuf[txcnt++] = 5;                             /* length */
    txbuf[txcnt++] = 0x80 + code;                   /* control */
    txbuf[txcnt++] = dnp->ioid & 0xff;              /* des Addr */
    txbuf[txcnt++] = (dnp->ioid >> 8) & 0xff;
    txbuf[txcnt++] = dnp->hostid & 0xff;            /* src Addr */
    txbuf[txcnt++] = (dnp->hostid >> 8) & 0xff;
    crc = B013_mkdnpcrc(txbuf, txcnt);

    txbuf[txcnt++] = crc & 0xff;
    txbuf[txcnt++] = (crc >> 8) & 0xff;

    if((opr->dnpDebug == devid) || (opr->scanDebug == scanid))  DumpDNP_snd(console,"ACK:", txbuf, txcnt);
    
    if(mode == COM_TCPIP)
    {
        
        if((rxVal = tkWriteTCP( dnp->SocketFd , txbuf, txcnt, 1000)) < 0 )

    	{
        	if((opr->dnpDebug == devid) || (opr->scanDebug == scanid)) 
        	Debug(console, "dev%02d> **** TCP Write failed [%d/%d]...socket[%d] !\n", devid+1, txcnt,  rxVal, dnp->SocketFd);
// 2025-10-24 오후 1:30:52 hkkim		  	
		  	//dnp->connectStatus = RESET; 위에서 처리하게 여기서는 commeetout 
		  	return -1 ;
		}      
    }
    else
    {              
        //PortWrite( dnp->ttyPort, txbuf, txcnt);
        dnpChanWrite( scanid, devid, txbuf, txcnt);
    }
    
    return 0; 
}



/***************************************************
*   FUNCTION : resetDataLink()
***************************************************/
int  resetDataLink( int mode, int scanid, int devid)
{
    //int     i,j,k;
    int     txcnt;
    int     rxVal;
    byte    *txbuf;
    word    crc;

    DNP_DEVICE  *dnp;
    DL_FRAME    *sndFrame;

    dnp      = (DNP_DEVICE *) dnpcfg[devid];
    //sndFrame = (DL_FRAME *) &dataLinkFrame[fid].sndFrame;
    sndFrame = (DL_FRAME *) &dnp->dataLinkFrame.sndFrame;

    txbuf = (byte *) &sndFrame->start1;

    /* ------------------------ */
    /*  Make HEADER frame ...   */
    /* ------------------------ */
    txcnt = 0;
    txbuf[txcnt++] = 0x05;
    txbuf[txcnt++] = 0x64;
    txbuf[txcnt++] = 5;                             /* length */
    txbuf[txcnt++] = 0xc0;                          /* control */
    txbuf[txcnt++] = dnp->ioid & 0xff;              /* des Addr */
    txbuf[txcnt++] = (dnp->ioid >> 8) & 0xff;
    txbuf[txcnt++] = dnp->hostid & 0xff;            /* src Addr */
    txbuf[txcnt++] = (dnp->hostid >> 8) & 0xff;
    crc = B013_mkdnpcrc(txbuf, txcnt);

    txbuf[txcnt++] = crc & 0xff;
    txbuf[txcnt++] = (crc >> 8) & 0xff;

    if((opr->dnpDebug == devid) || (opr->scanDebug == scanid))
    {
        Debug(console,"ch%2d>> devid = %2d\r\n", scanid, devid+1);   
        DumpDNP_snd(console,"CFM:", txbuf, txcnt);
    }
    
    if(mode == COM_TCPIP)
    {
        if((rxVal = tk_net_writen( dnp->SocketFd , txbuf, txcnt, 1000)) < 0 )
    	{
        	if((opr->dnpDebug == devid) || (opr->scanDebug == scanid))
        	Debug(console, "dev%02d> **** net Send Error [%d / %d]...socket = %d !\n", devid+1, txcnt,  rxVal, dnp->SocketFd);
		  	//dnp->connectStatus = RESET;
			return -1 ; 
		}     
		else return txcnt ;    
    }
    else
    {              
        //PortWrite(tp, txbuf, txcnt);
        dnpChanWrite( scanid, devid, txbuf, txcnt);
        return txcnt ; 
    }
}

/*
*/
void  getDnpTime(int scanid, int devid)
{
    int     millisecond;
    
    struct timeval rv; 
    time_t  tm;
    struct  tm  localtm;

    /* -------------------------------------------- */
    /*  CU 시스템 시각을 로드해서 계전기로 Down...  */
    /* -------------------------------------------- */
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
    
    if((opr->dnpDebug == devid) || (opr->scanDebug == scanid))
    Debug(console,"DevTime set... %4d/%2d/%2d %02d:%02d:%02d-%d\n",
        dnpRTC.year, dnpRTC.month,  dnpRTC.day, dnpRTC.hour, dnpRTC.minute, dnpRTC.second, dnpRTC.millisecond);            

    /* -------------------------------- */
    /*  DNP Time 구조체 변환            */
    /* -------------------------------- */
    makeDnpTimeByte( &dnpRTCInfo, &dnpRTC);

}


/*
*   read Binary Input : Object-01, Variation-0, Qcode-6
*/
int dnpBinaryInput(int scanid, int devid)
{
    int         stopPoint;
    DNP_DEVICE      *dnp;
    DNP_APP_FRAME   *sndAppDnp;

    dnp = (DNP_DEVICE *) dnpcfg[devid];
    //sndAppDnp = (DNP_APP_FRAME *) &sndAppFrame[fid];
    sndAppDnp = (DNP_APP_FRAME *) &dnp->sndAppFrame;

    sndAppDnp->sndCount = 0;

    /* Single BIT Binary Input Read */
    sndAppDnp->userData[0] = 0xc0 + dnp->acSndSeqno;      /* AC */
    sndAppDnp->userData[1] = 0x01;      /* FC : read*/
    
    if(dnp->type == TYPE_DNP_SIEMENS)
    {
#if 1        
        sndAppDnp->userData[2] = OBJ_BINARY_INPUT;      /* Object     : 01 : Single-Bit status */
        sndAppDnp->userData[3] = 0x01;                  /* variation  : 1  */
        sndAppDnp->userData[4] = 0x01;                  /* qualifier  : 1  */
        
        stopPoint = dnp->devDiPoint - 1;       
        sndAppDnp->userData[5] = 0x00;                  /* start Point: LSB */
        sndAppDnp->userData[6] = 0x00;                  /* start Point: MSB */
        sndAppDnp->userData[7] = stopPoint & 0xff;              /* stop  : LSB  */
        sndAppDnp->userData[8] = (stopPoint >> 8)  & 0xff;      /* stop  : MSB  */
        sndAppDnp->sndCount = 9;
#endif

#if 0        
        /* for BIT */
        sndAppDnp->userData[2] = OBJ_BINARY_INPUT;      /* Object     : 01 : Single-Bit status */
        sndAppDnp->userData[3] = 0x01;                  /* variation  : 1  */
        sndAppDnp->userData[4] = 0x06;                  /* qualifier  : 6  */
        sndAppDnp->sndCount = 5;
#endif        
    }
    else
    {             
        sndAppDnp->userData[2] = OBJ_BINARY_INPUT;      /* Object     : 01 : Single-Bit status */
        sndAppDnp->userData[3] = 0x01;                  /* variation  : 1  */
        sndAppDnp->userData[4] = 0x06;                  /* qualifier  : 6  */
        sndAppDnp->sndCount = 5;
    }
    
    if((opr->dnpDebug == devid) || (opr->scanDebug == scanid))
    Debug(console,"snd%2d> Binary Single Input ... !\n", devid+1);

    return (sndAppDnp->sndCount);
}

/*
*   read Binary Input : Object-01, Variation-0, Qcode-6
*/
int dnpBinaryChange(int scanid, int devid)
{
    DNP_DEVICE      *dnp;
    DNP_APP_FRAME   *sndAppDnp;

    dnp = (DNP_DEVICE *) dnpcfg[devid];
    //sndAppDnp = (DNP_APP_FRAME *) &sndAppFrame[fid];
    sndAppDnp = (DNP_APP_FRAME *) &dnp->sndAppFrame;

    sndAppDnp->sndCount = 0;

    /* Single BIT Binary Input Read */
    sndAppDnp->userData[0] = 0xc0 + dnp->acSndSeqno;      /* AC */
    sndAppDnp->userData[1] = 0x01;      /* FC : read*/

// 20261124  효성계전기 원복
#if 0
    sndAppDnp->userData[2] = OBJ_BINARY_CHANGE;     /* Object     : 01 : Single-Bit status */
    sndAppDnp->userData[3] = 0x00;                  /* variation  : 1  */
    sndAppDnp->userData[4] = 0x06;                  /* qualifier  : 6  */
    sndAppDnp->sndCount = 5;
        
#else
    /* ------------------------------------------------ */
    /*  계전기 TYPE : ProPAC 의경우...CLASS DUMP        */
    /* ------------------------------------------------ */
    if(dnp->type == TYPE_PROPACK)
    {
     //   sndAppDnp->userData[2] = OBJ_BINARY_CHANGE;     /* Object     : 01 : Single-Bit status */
     //   sndAppDnp->userData[3] = 0x02;                  /* variation  : 1  */
     //   sndAppDnp->userData[4] = 0x06;                  /* qualifier  : 6  */
     //    sndAppDnp->sndCount = 5;   

        /* 최종 확인...OBE Error 가 없음 */
        sndAppDnp->userData[2] = OBJ_CLASS;             /* Object     : 60  */
        sndAppDnp->userData[3] = DNP_CLASS1;            /* variation  : 1,2,3,4 Classid */
        sndAppDnp->userData[4] = 0x06;                  /* qualifier  : 0x06*/
        sndAppDnp->sndCount = 5;
    }
    else
    {
        sndAppDnp->userData[2] = OBJ_BINARY_CHANGE;      /* Object     : 01 : Single-Bit status */
        sndAppDnp->userData[3] = 0x00;      /* variation  : 1  */
        sndAppDnp->userData[4] = 0x06;      /* qualifier  : 6  */
        sndAppDnp->sndCount = 5;
    }
#endif
    
    if((opr->dnpDebug == devid) || (opr->scanDebug == scanid))
    Debug(console,"snd%2d> Binary Change Check ... !\n", devid+1);

    return (sndAppDnp->sndCount);
}


/*
*   read Analog Input : Object-30, Variation-0, Qcode-0
*/
int dnpAnalogInput(int scanid, int devid)
{
    int             stopPoint;
    DNP_DEVICE      *dnp;
    DNP_APP_FRAME   *sndAppDnp;

    dnp = (DNP_DEVICE *) dnpcfg[devid];
    //sndAppDnp = (DNP_APP_FRAME *) &sndAppFrame[fid];
    sndAppDnp = (DNP_APP_FRAME *) &dnp->sndAppFrame;

    sndAppDnp->sndCount = 0;


    if(dnp->type == TYPE_FRTU)
    {
        /* Analog Input Read */
        sndAppDnp->userData[0] = 0xc0 + dnp->acSndSeqno;      /* AC */
        sndAppDnp->userData[1] = 0x01;      /* FC : read*/
        
        /* 회로#1 전압/전류 */
        sndAppDnp->userData[2] = OBJ_ANALOG_INPUT;        /* Object     : 30  */
        sndAppDnp->userData[3] = 0;         /* variation  : 0   */
        sndAppDnp->userData[4] = 0;         /* qualifier  : 0   */
        sndAppDnp->userData[5] = 0;         /* start :0         */
        sndAppDnp->userData[6] = 2;         /* stop  :2         */
        
        sndAppDnp->userData[7] = OBJ_ANALOG_INPUT;        /* Object     : 30  */
        sndAppDnp->userData[8] = 0;         /* variation  : 0   */
        sndAppDnp->userData[9] = 0;         /* qualifier  : 0   */
        sndAppDnp->userData[10]= 10;        /* start :10        */
        sndAppDnp->userData[11]= 12;        /* stop  :12        */

        /* 회로#2 전압/전류 */
        sndAppDnp->userData[12]= OBJ_ANALOG_INPUT;        /* Object     : 30  */
        sndAppDnp->userData[13]= 0;         /* variation  : 0   */
        sndAppDnp->userData[14]= 0;         /* qualifier  : 0   */
        sndAppDnp->userData[15]= 23;        /* start :23        */
        sndAppDnp->userData[16]= 25;        /* stop  :25        */
        
        sndAppDnp->userData[17]= OBJ_ANALOG_INPUT;        /* Object     : 30  */
        sndAppDnp->userData[18]= 0;         /* variation  : 0   */
        sndAppDnp->userData[19]= 0;         /* qualifier  : 0   */
        sndAppDnp->userData[20]= 33;        /* start :33        */
        sndAppDnp->userData[21]= 35;        /* stop  :35        */

        /* 회로#3 전압/전류 */
        sndAppDnp->userData[22]= OBJ_ANALOG_INPUT;        /* Object     : 30  */
        sndAppDnp->userData[23]= 0;         /* variation  : 0   */
        sndAppDnp->userData[24]= 0;         /* qualifier  : 0   */
        sndAppDnp->userData[25]= 46;        /* start :46        */
        sndAppDnp->userData[26]= 48;        /* stop  :48        */
        
        sndAppDnp->userData[27]= OBJ_ANALOG_INPUT;        /* Object     : 30  */
        sndAppDnp->userData[28]= 0;         /* variation  : 0   */
        sndAppDnp->userData[29]= 0;         /* qualifier  : 0   */
        sndAppDnp->userData[30]= 56;        /* start :56        */
        sndAppDnp->userData[31]= 58;        /* stop  :58        */                

        /* 회로#3 전압/전류 */
        sndAppDnp->userData[32]= OBJ_ANALOG_INPUT;        /* Object     : 30  */
        sndAppDnp->userData[33]= 0;         /* variation  : 0   */
        sndAppDnp->userData[34]= 0;         /* qualifier  : 0   */
        sndAppDnp->userData[35]= 69;        /* start :69        */
        sndAppDnp->userData[36]= 71;        /* stop  :71        */
        
        sndAppDnp->userData[37]= OBJ_ANALOG_INPUT;        /* Object     : 30  */
        sndAppDnp->userData[38]= 0;         /* variation  : 0   */
        sndAppDnp->userData[39]= 0;         /* qualifier  : 0   */
        sndAppDnp->userData[40]= 79;        /* start :79        */
        sndAppDnp->userData[41]= 81;        /* stop  :81        */         
                
        sndAppDnp->sndCount = 42;

    }    
    else if(dnp->type == TYPE_DNP_SIEMENS)
    {
        /* Analog Input Read OBJ=30, Var = 2 */
        sndAppDnp->userData[0] = 0xc0 + dnp->acSndSeqno;      /* AC */
        sndAppDnp->userData[1] = 0x01;      /* FC : read*/
        sndAppDnp->userData[2] = OBJ_ANALOG_INPUT;        /* Object     : 01 */
        sndAppDnp->userData[3] = 0x02;      /* variation  : 2  */
        sndAppDnp->userData[4] = 0x01;      /* qualifier  : 0  */
        
        stopPoint = dnp->devAiPoint - 1;       
        sndAppDnp->userData[5] = 0x00;                  /* start Point: LSB */
        sndAppDnp->userData[6] = 0x00;                  /* start Point: MSB */
        sndAppDnp->userData[7] = stopPoint & 0xff;              /* stop  : LSB  */
        sndAppDnp->userData[8] = (stopPoint >> 8)  & 0xff;      /* stop  : MSB  */
        sndAppDnp->sndCount = 9;
    }
    else
    {
        /* GIPAM2000 인 경우 ...    */
        /* Analog Input Read */
        sndAppDnp->userData[0] = 0xc0 + dnp->acSndSeqno;    /* AC */
        sndAppDnp->userData[1] = 0x01;                      /* FC : read*/
        sndAppDnp->userData[2] = OBJ_ANALOG_INPUT;          /* Object     : 30 */
        sndAppDnp->userData[3] = 0;                         /* variation  : 0  */
        sndAppDnp->userData[4] = 0;                         /* qualifier  : 0  */

        sndAppDnp->userData[5] = 0;                         /* start :0  */
        sndAppDnp->userData[6] = dnp->devAiPoint - 1;       /* stop  :20  */

        sndAppDnp->sndCount = 7;
    }    

    if((opr->dnpDebug == devid) || (opr->scanDebug == scanid))
    Debug(console,"snd%2d> Analog Input ... !\n", devid+1);

    return (sndAppDnp->sndCount);
}


/*
*   read Binary Counter Input : Object-20, Variation-5, Qcode-0
*/
int dnpCountInput(int scanid, int devid)
{
    DNP_DEVICE      *dnp;
    DNP_APP_FRAME   *sndAppDnp;

    dnp = (DNP_DEVICE *) dnpcfg[devid];
    //sndAppDnp = (DNP_APP_FRAME *) &sndAppFrame[fid];
    sndAppDnp = (DNP_APP_FRAME *) &dnp->sndAppFrame;

    sndAppDnp->sndCount = 0;


    /* Analog Input Read */
    sndAppDnp->userData[0] = 0xc0 + dnp->acSndSeqno;    /* AC */
    sndAppDnp->userData[1] = 0x01;                      /* FC : read*/
    sndAppDnp->userData[2] = OBJ_BINARY_COUNTER;        /* Object     : 20 */
    sndAppDnp->userData[3] = 5;                         /* variation  : 5  */
    sndAppDnp->userData[4] = 0;                         /* qualifier  : 0  */

    sndAppDnp->userData[5] = 0;                         /* start :0  */
    sndAppDnp->userData[6] = dnp->devBCPoint - 1;       /* stop  :20  */

    sndAppDnp->sndCount = 7;

    if((opr->dnpDebug == devid) || (opr->scanDebug == scanid))
    Debug(console,"snd%2d> Binary-Count Input ... !\n", devid+1);

    return (sndAppDnp->sndCount);
}

/*
*   read Analog Input : Object-30, Variation-3, Qcode-0
*/
int dnpAnalogOutput(int scanid, int devid)
{
    DNP_DEVICE      *dnp;
    DNP_APP_FRAME   *sndAppDnp;

    dnp = (DNP_DEVICE *) dnpcfg[devid];
    //sndAppDnp = (DNP_APP_FRAME *) &sndAppFrame[fid];
    sndAppDnp = (DNP_APP_FRAME *) &dnp->sndAppFrame;

    sndAppDnp->sndCount = 0;

    if(dnp->type == TYPE_DNP_SIEMENS)
    {
        /* Analog Output Read OBJ=30, Var = 3 */
        sndAppDnp->userData[0] = 0xc0 + dnp->acSndSeqno;      /* AC */
        sndAppDnp->userData[1] = 0x01;      /* FC : read*/
        sndAppDnp->userData[2] = OBJ_ANALOG_OUTPUT;        /* Object     : 30 */
        sndAppDnp->userData[3] = 0x03;      /* variation  : 3  */
        sndAppDnp->userData[4] = 0x06;      /* qualifier  : 6  */
        sndAppDnp->sndCount = 5;
    }
    else        // TYPE_GIPAM2000, TYPE_FRTU
    {
        /* Analog Input Read */
        sndAppDnp->userData[0] = 0xc0 + dnp->acSndSeqno;      /* AC */
        sndAppDnp->userData[1] = 0x01;      /* FC : read*/
        sndAppDnp->userData[2] = OBJ_ANALOG_OUTPUT;        /* Object     : 40 */
        sndAppDnp->userData[3] = 3;         /* variation  : 3  */
        sndAppDnp->userData[4] = 0;         /* qualifier  : 0  */
        sndAppDnp->userData[5] = 0;         /* start :0  */

        sndAppDnp->userData[6] = dnp->devAoPoint - 1;    /* stop  :5  */

        sndAppDnp->sndCount = 7;
    }


    if((opr->dnpDebug == devid) || (opr->scanDebug == scanid))
    Debug(console,"snd%2d> Analog Output ... !\n", devid+1);

    return (sndAppDnp->sndCount);
}

/*
*   read Binary Input : Object-50, Variation-1, Qcode-0x07, Index-1
*/
int dnpTimeSync(int scanid, int devid)
{
    DNP_DEVICE      *dnp;
    DNP_APP_FRAME   *sndAppDnp;

    dnp = (DNP_DEVICE *) dnpcfg[devid];
    //sndAppDnp = (DNP_APP_FRAME *) &sndAppFrame[fid];
    sndAppDnp = (DNP_APP_FRAME *) &dnp->sndAppFrame;

    sndAppDnp->sndCount = 0;

    getDnpTime(scanid, devid);			/* rtc를 읽어 dnpRTCInfo로 만든다.*/

    /* Binary Input Read */
    sndAppDnp->userData[0] = 0xc0 + dnp->acSndSeqno;      /* AC */
    sndAppDnp->userData[1] = 0x02;          /* FC : write */
    sndAppDnp->userData[2] = OBJ_TIME_AND_DATE;        /* Object     : 50  */
    sndAppDnp->userData[3] = 1;         /* variation  : 1   */
    sndAppDnp->userData[4] = 7;         /* Index:0, qualifier  : 07 */
    
    sndAppDnp->userData[5] = 1;         /* quentity : 1 */
    sndAppDnp->userData[6] = (dnpRTCInfo.leastSignificant) & 0xff;
    sndAppDnp->userData[7] = (dnpRTCInfo.leastSignificant >> 8) & 0xff;
    sndAppDnp->userData[8] = (dnpRTCInfo.mostSignificant) & 0xff;
    sndAppDnp->userData[9] = (dnpRTCInfo.mostSignificant >> 8)  & 0xff;
    sndAppDnp->userData[10]= (dnpRTCInfo.mostSignificant >> 16) & 0xff;
    sndAppDnp->userData[11]= (dnpRTCInfo.mostSignificant >> 24) & 0xff;

    sndAppDnp->sndCount = 12;
    dnp->timeSyncReq = RESET;

    if((opr->dnpDebug == devid) || (opr->scanDebug == scanid))
    Debug(console,"snd%2d> Time Sync ... !\n", devid+1);

    return (sndAppDnp->sndCount);
}

/*
*   read CLASS Input : Object-60, Variation-1,2,3,4, Qcode-0x06, Index-0
*/
int dnpClassRead(int scanid, int devid, int classid)
{
    DNP_DEVICE      *dnp;
    DNP_APP_FRAME   *sndAppDnp;

    dnp = (DNP_DEVICE *) dnpcfg[devid];
    //sndAppDnp = (DNP_APP_FRAME *) &sndAppFrame[fid];
    sndAppDnp = (DNP_APP_FRAME *) &dnp->sndAppFrame;

    sndAppDnp->sndCount = 0;

    /* CLASS  Input Read */
    sndAppDnp->userData[0] = 0xc0 + dnp->acSndSeqno;      /* AC */
    sndAppDnp->userData[1] = 0x01;          /* FC : read*/
    sndAppDnp->userData[2] = OBJ_CLASS;     /* Object     : 60  */
    sndAppDnp->userData[3] = classid;       /* variation  : 1,2,3,4 Classid */
    sndAppDnp->userData[4] = 0x06;          /* qualifier  : 0x06*/
    sndAppDnp->sndCount = 5;

    if((opr->dnpDebug == devid) || (opr->scanDebug == scanid))
    Debug(console,"snd%2d> Class Read = %d\n", devid +1, classid - 1);

    return (sndAppDnp->sndCount);
}

/*
*   read CLASS Input : Object-60, Variation-1,2,3,4, Qcode-0x06, Index-0
*/
int dnpClassAll(int scanid, int devid)
{
    int             count;
    DNP_DEVICE      *dnp;
    DNP_APP_FRAME   *sndAppDnp;

    dnp = (DNP_DEVICE *) dnpcfg[devid];
    //sndAppDnp = (DNP_APP_FRAME *) &sndAppFrame[fid];
    sndAppDnp = (DNP_APP_FRAME *) &dnp->sndAppFrame;

    sndAppDnp->sndCount = 0;

    /* CLASS  Input Read */
    sndAppDnp->userData[0] = 0xc0 + dnp->acSndSeqno;      /* AC */
    sndAppDnp->userData[1] = 0x01;      /* FC : read*/
    count = 2;
    
    if(dnp->class1Req == SET)
    {
        sndAppDnp->userData[count++] = OBJ_CLASS;          /* Object     : 60  */
        sndAppDnp->userData[count++] = DNP_CLASS1;  /* variation  : 1,2,3,4 Classid */
        sndAppDnp->userData[count++] = 0x06;        /* qualifier  : 0x06*/
        if((opr->dnpDebug == devid) || (opr->scanDebug == scanid))
        Debug(console,"snd%2d> Class 1 Read \n", devid +1);        
    }

    if(dnp->class2Req == SET)
    {
        sndAppDnp->userData[count++] = OBJ_CLASS;          /* Object     : 60  */
        sndAppDnp->userData[count++] = DNP_CLASS2;  /* variation  : 1,2,3,4 Classid */
        sndAppDnp->userData[count++] = 0x06;        /* qualifier  : 0x06*/
        if((opr->dnpDebug == devid) || (opr->scanDebug == scanid))
        Debug(console,"snd%2d> Class 2 Read \n", devid +1);         
    }    
            
    if(dnp->class3Req == SET)
    {
        sndAppDnp->userData[count++] = OBJ_CLASS;          /* Object     : 60  */
        sndAppDnp->userData[count++] = DNP_CLASS3;  /* variation  : 1,2,3,4 Classid */
        sndAppDnp->userData[count++] = 0x06;        /* qualifier  : 0x06*/
        if((opr->dnpDebug == devid) || (opr->scanDebug == scanid))
        Debug(console,"snd%2d> Class 3 Read \n", devid +1);         
    }
                
    sndAppDnp->sndCount = count;

    return (sndAppDnp->sndCount);
}

/*
*   read IIN Input : Object-80, Variation-1, Qcode-0x0, Index-0
*/
int dnpIINRead(int scanid, int devid)
{
    DNP_DEVICE      *dnp;
    DNP_APP_FRAME   *sndAppDnp;

    dnp = (DNP_DEVICE *) dnpcfg[devid];
    //sndAppDnp = (DNP_APP_FRAME *) &sndAppFrame[fid];
    sndAppDnp = (DNP_APP_FRAME *) &dnp->sndAppFrame;

    sndAppDnp->sndCount = 0;

    /* CLASS  Input Read */
    sndAppDnp->userData[0] = 0xc0 + dnp->acSndSeqno;      /* AC */
    sndAppDnp->userData[1] = 0x01;      /* FC : read*/
    sndAppDnp->userData[2] = OBJ_DEVICE_IIN;        /* Object     : 80  */
    sndAppDnp->userData[3] = 1;         /* variation  : 1 */
    sndAppDnp->userData[4] = 0x00;      /* qualifier  : 0x0*/
    sndAppDnp->userData[5] = 7;
    sndAppDnp->userData[6] = 7;
    sndAppDnp->userData[7] = 0;
    sndAppDnp->sndCount = 8;

    if((opr->dnpDebug == devid) || (opr->scanDebug == scanid))
    Debug(console,"snd%2d> IIN Read ... !\n", devid+1);

    return (sndAppDnp->sndCount);
}

/*
*   write IIN  : Object-80, Variation-1, Qcode-0x0, Index-0
*/
int dnpIINWrite(int scanid, int devid)
{
    DNP_DEVICE      *dnp;
    DNP_APP_FRAME   *sndAppDnp;

    dnp = (DNP_DEVICE *) dnpcfg[devid];
    //sndAppDnp = (DNP_APP_FRAME *) &sndAppFrame[fid];
    sndAppDnp = (DNP_APP_FRAME *) &dnp->sndAppFrame;

    sndAppDnp->sndCount = 0;

    /* Internal Indication Write ... */
    sndAppDnp->userData[0] = 0xc0 + dnp->acSndSeqno;      /* AC */
    sndAppDnp->userData[1] = 0x02;      /* FC : write*/
    
    sndAppDnp->userData[2] = OBJ_DEVICE_IIN;        /* Object     : 80  */
    sndAppDnp->userData[3] = 1;         /* variation  : 1 */
    sndAppDnp->userData[4] = 0x00;      /* qualifier  : 0x0*/
    sndAppDnp->userData[5] = 7;
    sndAppDnp->userData[6] = 7;
    sndAppDnp->userData[7] = 0;
    sndAppDnp->sndCount = 8;

    if((opr->dnpDebug == devid) || (opr->scanDebug == scanid))
    Debug(console,"snd%2d> IIN-Write ... !\n", devid+1);

    return (sndAppDnp->sndCount);
}


/*
*   write IIN  : Object-80, Variation-1, Qcode-0x0, Index-0
*/
int dnpACConfirm(int scanid, int devid)
{
    DNP_DEVICE      *dnp;
    DNP_APP_FRAME   *sndAppDnp;

    dnp = (DNP_DEVICE *) dnpcfg[devid];
    //sndAppDnp = (DNP_APP_FRAME *) &sndAppFrame[fid];
    sndAppDnp = (DNP_APP_FRAME *) &dnp->sndAppFrame;

    sndAppDnp->sndCount = 0;

    /* CLASS  Input Read */
    sndAppDnp->userData[0] = 0xc0 + dnp->acSndSeqno;      /* AC */
    sndAppDnp->userData[1] = 0x0;       /* FC : confirm*/
    sndAppDnp->sndCount = 2;

    if((opr->dnpDebug == devid) || (opr->scanDebug == scanid))
    Debug(console,"dev%2d> AC Confirm ...!\n", devid+1);
    
    return (sndAppDnp->sndCount);
}

/*
*   Select Write : Object-12, Variation-1, Qcode-0x07, Index-1
*/
int dnpSelectWrite(int scanid, int devid)
{
    int ontime, offtime;
    int cntPoint=0, cntTcf=0;

    DNP_DEVICE      *dnp;
    DNP_APP_FRAME   *sndAppDnp;
    POINT_BUF   *ptBuf;

    dnp = (DNP_DEVICE *) dnpcfg[devid];
    //sndAppDnp = (DNP_APP_FRAME *) &sndAppFrame[fid];
    sndAppDnp = (DNP_APP_FRAME *) &dnp->sndAppFrame;

    sndAppDnp->sndCount = 0;
    
    /* 상위 제어시간 정보 적용 */
    if((opr->rcvONtime <= 500 ) || (opr->rcvONtime > 10000))    ontime = 1000;          // 수신한 제어시간 정보
    else                                                        ontime = opr->rcvONtime;

    if((opr->rcvOFFtime <= 500 ) || (opr->rcvOFFtime > 10000))  offtime = 1000;          // 수신한 제어시간 정보
    else                                                        offtime = opr->rcvOFFtime;    

    /* ---------------------------------------------------- */
    /*  계전기 TYPE별 제어속성 정보 지정                    */
    /* ---------------------------------------------------- */
    if(dnp->type == TYPE_PROPACK)
    {
        /* CLASS  Input Read */
        sndAppDnp->userData[0] = 0xc0 + dnp->acSndSeqno;    /* AC */
        sndAppDnp->userData[1] = 0x03;                      /* FC : select*/
        sndAppDnp->userData[2] = OBJ_CONTROL_RELAY;         /* Object     : 12  */
        sndAppDnp->userData[3] = 1;                         /* variation  : 1 */
        sndAppDnp->userData[4] = 0x17;                      /* qualifier  : 0x07, Index = 1*/

        if(dnp->cntTcf == TRIP_CONTROL )        
        {
            cntPoint = dnp->cntPoint*2 + 1; 
            cntTcf   = 0x41;
            if(opr->soeDebug)
            Debug(console,"Propack%2d> select TRIP Control : point=%d, cntPoint = %d, time=%d/%d \n", devid+1, dnp->cntPoint+1, cntPoint, ontime, offtime);
        }
        else if(dnp->cntTcf == CLOSE_CONTROL )  
        {
            cntPoint = dnp->cntPoint*2;
            cntTcf   = 0x41;
            if(opr->soeDebug)
            Debug(console,"Propack%2d> select CLOSE Control : point=%d, cntPoint = %d, time=%d/%d\n", devid+1, dnp->cntPoint+1, cntPoint, ontime, offtime);
        }
        else
        {
            Debug(console,"Propack> *** Control Failed....\n");
            return(0); 
        } 

        sndAppDnp->userData[5] = 1;                         /* quantity : 1 */
        sndAppDnp->userData[6] = cntPoint;              /* point */
        sndAppDnp->userData[7] = cntTcf;                /* control: trip, Pulse on */
        //sndAppDnp->userData[7] = dnp->cntTcf + 0x01;        /* control: trip, Pulse on */
        sndAppDnp->userData[8] = 1;                         /* count   */
        sndAppDnp->userData[9] = ontime & 0xff;             /* on Time */
        sndAppDnp->userData[10]= (ontime >> 8) & 0xff;      /* on Time */
        sndAppDnp->userData[11]= (ontime >> 16) & 0xff;     /* on Time */
        sndAppDnp->userData[12]= (ontime >> 24) & 0xff;     /* on Time */
        sndAppDnp->userData[13]= offtime & 0xff;            /* on Time */
        sndAppDnp->userData[14]= (offtime >> 8) & 0xff;     /* on Time */
        sndAppDnp->userData[15]= (offtime >> 16) & 0xff;    /* on Time */
        sndAppDnp->userData[16]= (offtime >> 24) & 0xff;    /* on Time */
        sndAppDnp->userData[17]= 0;                         /* status  */

        sndAppDnp->sndCount = 18;
    }
    /* ---------------------------------------------------- */
    /*  2017년 5월 24일 - 지멘스/LG 계전기 DNP 제어         */
    /*      - 제어방식 : Point Latch 방식 제어              */
    /*      - Point Config에 따라서 TRIP(0)/CLOSE(1) 선행   */
    /*      - 제어 Index : dbMAX 항목값 지정                */
    /* ---------------------------------------------------- */
    /*  - 0 : PULSE 제어 (TRIP / CLOSE제어)                 */
	/*  - 1 : PULSE-ON 제어 = CLOSE 우선                    */
	/*  - 2 : PULSE-ON 제어 = TRIP 우선                     */
	/*  - 3 : LATCH-ON 제어 = CLOSE 우선                    */     
	/*  - 4 : LATCH-ON 제어 = TRIP 우선                     */
	/*  - 5 : 예약                                          */
	/*  - 6 : 예약                                          */
    /*  - 7 : AO 포인트 제어, Fault Reset 시                */
    /* ---------------------------------------------------- */        
    else if(dnp->type == TYPE_DNP_SIEMENS)
    {
        ptBuf = (POINT_BUF *) &dnp->doPtBuf[dnp->cntPoint];
        
        /* CLASS  Input Read */
        sndAppDnp->userData[0] = 0xc0 + dnp->acSndSeqno;                      /* AC */
        sndAppDnp->userData[1] = 0x03;                      /* FC : select*/
        sndAppDnp->userData[2] = OBJ_CONTROL_RELAY;                        /* Object     : 12  */
        sndAppDnp->userData[3] = 1;                         /* variation  : 1 */
        sndAppDnp->userData[4] = 0x28;                      /* qualifier  : 0x08, Index = 2*/

        sndAppDnp->userData[5] = 1;                         /* quantity : 1 */
        sndAppDnp->userData[6] = 0;                         /* quantity : 1 */

#if 0        
        /* ------------------------------------- */
        /* 포인트 Config에 따른 제어 포인트 지정 */
        /* ------------------------------------- */
        if(ptBuf->ptConfig == 0)            /* 지멘스제어기 TRIP/CLOSE  제어시 : TRIP 선행 */
        {
            if(dnp->cntTcf == TRIP_CONTROL)         cntPoint = ptBuf->dbmax;
            else if(dnp->cntTcf == CLOSE_CONTROL)   cntPoint = ptBuf->dbmax + 1;    
        }
        else                                /* 지멘스제어기 TRIP/CLOSE  제어시 : CLOSE 선행 */
        {
            if(dnp->cntTcf == TRIP_CONTROL)         cntPoint = ptBuf->dbmax + 1;
            else if(dnp->cntTcf == CLOSE_CONTROL)   cntPoint = ptBuf->dbmax;    
        }
                
        sndAppDnp->userData[7] = cntPoint & 0xff;             /* point */
        sndAppDnp->userData[8] = (cntPoint >> 8) & 0xff;

        /* ------------------------------------- */
        /*  제어방식 : LATCH-ON 제어 ...         */
        /* ------------------------------------- */
        sndAppDnp->userData[9] = 0x03;        /* control: close, Latch ON */
#endif
        /* ---------------------------------------- */
        /* 제어방식(TC, PULSE, LATCH) 별 제어데이터 */
        /* ---------------------------------------- */
        if(dnp->cntrType == DNP_PULSE_CLOSE)                    /* ptConfig == 1, CLOSE-POINT 우선 */  
        {
            if(dnp->cntTcf == CLOSE_CONTROL)                     
            {
                cntPoint = ptBuf->dbmax;                        
                sndAppDnp->userData[7] = cntPoint & 0xff;             
                sndAppDnp->userData[8] = (cntPoint >> 8) & 0xff;
                sndAppDnp->userData[9] = 0x01;                  /* control: close, Pulse ON */
                
                if(opr->soeDebug)
                Debug(console,"snd%2d> select SIEMENS : Config ... PULSE-CLOSE, CLOSE-Point= %d\n", devid+1, cntPoint);  
            }
            else if(dnp->cntTcf == TRIP_CONTROL)    
            {
                cntPoint = ptBuf->dbmax + 1;                        
                sndAppDnp->userData[7] = cntPoint & 0xff;             
                sndAppDnp->userData[8] = (cntPoint >> 8) & 0xff;
                sndAppDnp->userData[9] = 0x01;                  /* control: close, Pulse ON */
                
                if(opr->soeDebug)
                Debug(console,"snd%2d> select SIEMENS : Config ... PULSE-CLOSE, TRIP-Point= %d\n", devid+1, cntPoint);  
            }
            else
            {                                         
                cntPoint = ptBuf->dbmax;                        
                sndAppDnp->userData[7] = cntPoint & 0xff;             
                sndAppDnp->userData[8] = (cntPoint >> 8) & 0xff;
                sndAppDnp->userData[9] = 0x00;                  /* control: close, NULL- Op */
            }
        }
        else if(dnp->cntrType == DNP_PULSE_TRIP)                /* ptConfig == 2, TRIP-POINT 우선 */  
        {
            if(dnp->cntTcf == TRIP_CONTROL)                     
            {
                cntPoint = ptBuf->dbmax;                        
                sndAppDnp->userData[7] = cntPoint & 0xff;             
                sndAppDnp->userData[8] = (cntPoint >> 8) & 0xff;
                sndAppDnp->userData[9] = 0x01;                  /* control: close, Pulse ON */
                
                if(opr->soeDebug)
                Debug(console,"snd%2d> select SIEMENS : Config ... PULSE-TRIP, TRIP-Point= %d\n", devid+1, cntPoint);  
            }
            else if(dnp->cntTcf == CLOSE_CONTROL)    
            {
                cntPoint = ptBuf->dbmax + 1;                        
                sndAppDnp->userData[7] = cntPoint & 0xff;             
                sndAppDnp->userData[8] = (cntPoint >> 8) & 0xff;
                sndAppDnp->userData[9] = 0x01;                  /* control: close, Pulse ON */
                
                if(opr->soeDebug)
                Debug(console,"snd%2d> select SIEMENS : Config ... PULSE-TRIP, CLOSE-Point= %d\n", devid+1, cntPoint);  
            }
            else
            {                                         
                cntPoint = ptBuf->dbmax;                        
                sndAppDnp->userData[7] = cntPoint & 0xff;             
                sndAppDnp->userData[8] = (cntPoint >> 8) & 0xff;
                sndAppDnp->userData[9] = 0x00;                  /* control: close, NULL- Op */
            }
        }
        else if(dnp->cntrType == DNP_LATCH_CLOSE)               /* ptConfig == 3, LATCH-ON CLOSE 우선 */ 
        {
            if(dnp->cntTcf == CLOSE_CONTROL)                     
            {
                cntPoint = ptBuf->dbmax;                        
                sndAppDnp->userData[7] = cntPoint & 0xff;             
                sndAppDnp->userData[8] = (cntPoint >> 8) & 0xff;
                sndAppDnp->userData[9] = 0x03;                  /* control: close, Pulse ON */
                
                if(opr->soeDebug)
                Debug(console,"snd%2d> select SIEMENS : Config ... LATCH-CLOSE, CLOSE-Point= %d\n", devid+1, cntPoint);  
            }
            else if(dnp->cntTcf == TRIP_CONTROL)    
            {
                cntPoint = ptBuf->dbmax + 1;                        
                sndAppDnp->userData[7] = cntPoint & 0xff;             
                sndAppDnp->userData[8] = (cntPoint >> 8) & 0xff;
                sndAppDnp->userData[9] = 0x03;                  /* control: close, Pulse ON */
                
                if(opr->soeDebug)
                Debug(console,"snd%2d> select SIEMENS : Config ... LATCH-CLOSE, TRIP-Point= %d\n", devid+1, cntPoint);  
            }
            else
            {                                         
                cntPoint = ptBuf->dbmax;                        
                sndAppDnp->userData[7] = cntPoint & 0xff;             
                sndAppDnp->userData[8] = (cntPoint >> 8) & 0xff;
                sndAppDnp->userData[9] = 0x00;                  /* control: close, NULL- Op */
            }
        }
        else if(dnp->cntrType == DNP_LATCH_TRIP)               /* ptConfig == 4, LATCH-ON TRIP 우선 */ 
        {
            
            if(dnp->cntTcf == TRIP_CONTROL)                     
            {
                cntPoint = ptBuf->dbmax;                        
                sndAppDnp->userData[7] = cntPoint & 0xff;             
                sndAppDnp->userData[8] = (cntPoint >> 8) & 0xff;
                sndAppDnp->userData[9] = 0x03;                  /* control: close, Pulse ON */
                
                if(opr->soeDebug)
                Debug(console,"snd%2d> select SIEMENS : Config ... LATCH-TRIP, TRIP-Point= %d\n", devid+1, cntPoint);      
            
            }
            else if(dnp->cntTcf == CLOSE_CONTROL)    
            {
                cntPoint = ptBuf->dbmax + 1;                        
                sndAppDnp->userData[7] = cntPoint & 0xff;             
                sndAppDnp->userData[8] = (cntPoint >> 8) & 0xff;
                sndAppDnp->userData[9] = 0x03;                  /* control: close, Pulse ON */
                
                if(opr->soeDebug)
                Debug(console,"snd%2d> select SIEMENS : Config ... LATCH-TRIP, CLOSE-Point= %d\n", devid+1, cntPoint);     
            }
            else
            {                                         
                cntPoint = ptBuf->dbmax;                        
                sndAppDnp->userData[7] = cntPoint & 0xff;             
                sndAppDnp->userData[8] = (cntPoint >> 8) & 0xff;
                sndAppDnp->userData[9] = 0x00;                  /* control: close, NULL- Op */
            }
        }
        else                                                    /* ptConfig == 0, TRIP/CLOSE */ 
        {   
            cntPoint = ptBuf->dbmax;                        
            sndAppDnp->userData[7] = cntPoint & 0xff;             
            sndAppDnp->userData[8] = (cntPoint >> 8) & 0xff;
                        
            sndAppDnp->userData[9] = dnp->cntTcf + 0x01;        /* control: TRIP/CLOSE, Pulse on */
            
            if(opr->soeDebug)
            Debug(console,"snd%2d> select SIEMENS : Config ... TRIP/CLOSE, Point= %d\n", devid+1, cntPoint);  
        }
        
        sndAppDnp->userData[10] = 1;                         /* count   */
        sndAppDnp->userData[11] = ontime & 0xff;             /* on Time */
        sndAppDnp->userData[12]= (ontime >> 8) & 0xff;      /* on Time */
        sndAppDnp->userData[13]= (ontime >> 16) & 0xff;     /* on Time */
        sndAppDnp->userData[14]= (ontime >> 24) & 0xff;     /* on Time */
        sndAppDnp->userData[15]= offtime & 0xff;            /* on Time */
        sndAppDnp->userData[16]= (offtime >> 8) & 0xff;     /* on Time */
        sndAppDnp->userData[17]= (offtime >> 16) & 0xff;    /* on Time */
        sndAppDnp->userData[18]= (offtime >> 24) & 0xff;    /* on Time */
        sndAppDnp->userData[19]= 0;                         /* status  */

        sndAppDnp->sndCount = 20;

        if(opr->soeDebug)
        Debug(console,"snd%2d> select SIEMENS : Point= %d, TCF = %2x (base=%d )\n", devid+1, dnp->cntPoint+1, dnp->cntTcf, cntPoint);        
    }
    else
    {
    	/* GIPAM2000, FRTU 등등 */	
        sndAppDnp->userData[0] = 0xc0 + dnp->acSndSeqno;    /* AC */
        sndAppDnp->userData[1] = 0x03;                      /* FC : select*/
        sndAppDnp->userData[2] = OBJ_CONTROL_RELAY;         /* Object     : 12  */
        sndAppDnp->userData[3] = 1;                         /* variation  : 1 */
        sndAppDnp->userData[4] = 0x17;                      /* qualifier  : 0x07, Index = 1*/

        sndAppDnp->userData[5] = 1;                         /* quantity : 1 */
        sndAppDnp->userData[6] = dnp->cntPoint;             /* point */

        /* ---------------------------------------- */
        /* 제어방식(TC, PULSE, LATCH) 별 제어데이터 */
        /* ---------------------------------------- */
        if(dnp->cntrType == DNP_PULSE_CLOSE)                    /* ptConfig == 1, CLOSE-POINT 우선 */  
        {
            if(dnp->cntTcf == CLOSE_CONTROL)                     
            {
                sndAppDnp->userData[6] = dnp->cntPoint;         /* point */
                //sndAppDnp->userData[7] = 0x01 + 0x40;         /* POINT PULSE ON */
                sndAppDnp->userData[7] = 0x01;                  /* POINT PULSE ON */
                
                if(opr->soeDebug)
                Debug(console,"snd%2d> select : Config ... PULSE-CLOSE, CLOSE-Point= %d\n", devid+1, sndAppDnp->userData[6]);  
            }
            else if(dnp->cntTcf == TRIP_CONTROL)    
            {
                sndAppDnp->userData[6] = dnp->cntPoint + 1;     /* point = CLOSE-Point + 1*/
                //sndAppDnp->userData[7] = 0x01 + 0x40;         /* POINT PULSE ON */    
                sndAppDnp->userData[7] = 0x01;                  /* POINT PULSE ON */      
                
                if(opr->soeDebug)
                Debug(console,"snd%2d> select : Config ... PULSE-CLOSE, TRIP-Point= %d\n", devid+1, sndAppDnp->userData[6]);   
            }
            else                                    
                sndAppDnp->userData[7] = 0x00;       
        }
        else if(dnp->cntrType == DNP_PULSE_TRIP)                /* ptConfig == 2, TRIP-POINT 우선 */  
        {
            if(dnp->cntTcf == TRIP_CONTROL)        
            {
                sndAppDnp->userData[6] = dnp->cntPoint;         /* point */
                //sndAppDnp->userData[7] = 0x01 + 0x40;         /* POINT PULSE ON */
                sndAppDnp->userData[7] = 0x01;                  /* POINT PULSE ON */
                
                if(opr->soeDebug)
                Debug(console,"snd%2d> select : Config ... PULSE-TRIP, TRIP-Point= %d\n", devid+1, sndAppDnp->userData[6]); 
            }
            else if(dnp->cntTcf == CLOSE_CONTROL)    
            {
                sndAppDnp->userData[6] = dnp->cntPoint + 1;     /* point = CLOSE-Point + 1*/
                //sndAppDnp->userData[7] = 0x01 + 0x40;         /* POINT PULSE ON */    
                sndAppDnp->userData[7] = 0x01;                  /* POINT PULSE ON */        
                
                if(opr->soeDebug)
                Debug(console,"snd%2d> select : Config ... PULSE-TRIP, CLOSE-Point= %d\n", devid+1, sndAppDnp->userData[6]); 
            }
            else                                    
                sndAppDnp->userData[7] = 0x00;       
        }
        /* -------------------------------------------- */
        /*  원흥역사의 경우....외부 DO 사용시           */
        /* -------------------------------------------- */
        else if(dnp->cntrType == DNP_LATCH_CLOSE)               /* ptConfig == 3, LATCH-ON CLOSE 우선 */ 
        {
            if(dnp->cntTcf == CLOSE_CONTROL)        
            {
                sndAppDnp->userData[6] = dnp->cntPoint;         /* point */
                sndAppDnp->userData[7] = 0x03;                  /* POINT LATCH ON */
                
                if(opr->soeDebug)
                Debug(console,"snd%2d> select : Config ... LATCH-CLOSE, CLOSE-Point= %d\n", devid+1, sndAppDnp->userData[6]); 
            }
            else if(dnp->cntTcf == TRIP_CONTROL)    
            {
                sndAppDnp->userData[6] = dnp->cntPoint + 1;     /* point = CLOSE-Point + 1*/
                sndAppDnp->userData[7] = 0x04;                  /* POINT LATH OFF */          
                
                if(opr->soeDebug)
                Debug(console,"snd%2d> select : Config ... LATCH-CLOSE, TRIP-Point= %d\n", devid+1, sndAppDnp->userData[6]); 
            }
            else                                    
                sndAppDnp->userData[7] = 0x00;     
        }
        else if(dnp->cntrType == DNP_LATCH_TRIP)               /* ptConfig == 4, LATCH-ON TRIP 우선 */ 
        {
            if(dnp->cntTcf == TRIP_CONTROL)        
            {
                sndAppDnp->userData[6] = dnp->cntPoint;         /* point */
                sndAppDnp->userData[7] = 0x03;                  /* POINT LATCH ON */
                
                if(opr->soeDebug)
                Debug(console,"snd%2d> select : Config ... LATCH-TRIP, TRIP-Point= %d\n", devid+1, sndAppDnp->userData[6]); 
            }
            else if(dnp->cntTcf == CLOSE_CONTROL)    
            {
                sndAppDnp->userData[6] = dnp->cntPoint + 1;     /* point = CLOSE-Point + 1*/
                sndAppDnp->userData[7] = 0x04;                  /* POINT LATH OFF */          
                
                if(opr->soeDebug)
                Debug(console,"snd%2d> select : Config ... LATCH-TRIP, CLOSE-Point= %d\n", devid+1, sndAppDnp->userData[6]); 
            }
            else                                    
                sndAppDnp->userData[7] = 0x00;     
        }
        else if(dnp->cntrType == DNP_RESET_CLOSE)               /* ptConfig == 5, 광명부하개폐기, RESET 제어 - CLOSE 전용 */ 
        {
        	/* 제어상태 임의지정 => CLOSE 제어 */
        	dnp->cntTcf = CLOSE_CONTROL;
       	    sndAppDnp->userData[7] = dnp->cntTcf + 0x01;        /* control: TRIP/CLOSE, Pulse on */
            
            if(opr->soeDebug)
            Debug(console,"snd%2d> select : Config ... RESET-CLOSE, Point= %d\n", devid+1, sndAppDnp->userData[6]); 
        }
        else if(dnp->cntrType == DNP_RESET_TRIP)               	/* ptConfig == 6, 광명부하개폐기, RESET 제어 - TRIP 전용 */ 
        {
        	/* 제어상태 임의지정 => TRIP 제어 */
        	dnp->cntTcf = TRIP_CONTROL;
       	    sndAppDnp->userData[7] = dnp->cntTcf + 0x01;        /* control: TRIP/CLOSE, Pulse on */
            
            if(opr->soeDebug)
            Debug(console,"snd%2d> select : Config ... RESET-TRIP, Point= %d\n", devid+1, sndAppDnp->userData[6]); 
        }
        else                                                    /* ptConfig == 0, TRIP/CLOSE */ 
        {           
            sndAppDnp->userData[7] = dnp->cntTcf + 0x01;        /* control: TRIP/CLOSE, Pulse on */
            
            if(opr->soeDebug)
            Debug(console,"snd%2d> select : Config ... TRIP/CLOSE, Point= %d\n", devid+1, sndAppDnp->userData[6]); 
        }
                
        sndAppDnp->userData[8] = 1;                         /* count   */
        sndAppDnp->userData[9] = ontime & 0xff;             /* on Time */
        sndAppDnp->userData[10]= (ontime >> 8) & 0xff;      /* on Time */
        sndAppDnp->userData[11]= (ontime >> 16) & 0xff;     /* on Time */
        sndAppDnp->userData[12]= (ontime >> 24) & 0xff;     /* on Time */
        sndAppDnp->userData[13]= offtime & 0xff;            /* on Time */
        sndAppDnp->userData[14]= (offtime >> 8) & 0xff;     /* on Time */
        sndAppDnp->userData[15]= (offtime >> 16) & 0xff;    /* on Time */
        sndAppDnp->userData[16]= (offtime >> 24) & 0xff;    /* on Time */
        sndAppDnp->userData[17]= 0;                         /* status  */

        sndAppDnp->sndCount = 18;
        
        if(opr->soeDebug)
        {    
            Debug(console,"snd%2d> select : Point= %d, TCF = %2x, time=%d/%d \n", devid+1, dnp->cntPoint+1, dnp->cntTcf, ontime, offtime);
            prndat(">> ", (char *) &sndAppDnp->userData[2], 16);
        }
    }

    return (sndAppDnp->sndCount);
}

/*
*   Select Write : Object-12, Variation-1, Qcode-0x07, Index-1
*/
int dnpOperateWrite(int scanid, int devid)
{
    int     ontime, offtime;
    int     cntPoint=0, cntTcf=0;

    DNP_DEVICE      *dnp;
    DNP_APP_FRAME   *sndAppDnp;
    POINT_BUF   *ptBuf;

    dnp = (DNP_DEVICE *) dnpcfg[devid];
    sndAppDnp = (DNP_APP_FRAME *) &dnp->sndAppFrame;

    sndAppDnp->sndCount = 0;

    /* 상위 제어시간 정보 적용 */
    if((opr->rcvONtime <= 500 ) || (opr->rcvONtime > 10000))    ontime = 1000;          // 수신한 제어시간 정보
    else                                                        ontime = opr->rcvONtime;

    if((opr->rcvOFFtime <= 500 ) || (opr->rcvOFFtime > 10000))  offtime = 1000;          // 수신한 제어시간 정보
    else                                                        offtime = opr->rcvOFFtime;

    /* ---------------------------------------------------- */
    /*  계전기 TYPE별 제어속성 정보 지정                    */
    /* ---------------------------------------------------- */
    if(dnp->type == TYPE_PROPACK)
    {
        /* CLASS  Input Read */
        sndAppDnp->userData[0] = 0xc0 + dnp->acSndSeqno;                      /* AC */
        sndAppDnp->userData[1] = 0x04;                      /* FC : operate*/
        sndAppDnp->userData[2] = OBJ_CONTROL_RELAY;                        /* Object     : 12  */
        sndAppDnp->userData[3] = 1;                         /* variation  : 1 */
        sndAppDnp->userData[4] = 0x17;                      /* qualifier  : 0x07, Index = 1*/

        if(dnp->cntTcf == TRIP_CONTROL )        
        {
            cntPoint = dnp->cntPoint*2 + 1; 
            cntTcf   = 0x41;
            if(opr->soeDebug)
            Debug(console,"Propack%2d> operate TRIP Control : point=%d, cntPoint = %d\n", devid+1, dnp->cntPoint+1, cntPoint);
        }
        else if(dnp->cntTcf == CLOSE_CONTROL )  
        {
            cntPoint = dnp->cntPoint*2;
            cntTcf   = 0x41;
            if(opr->soeDebug)
            Debug(console,"Propack%2d> operate CLOSE Control : point=%d, cntPoint = %d\n", devid+1, dnp->cntPoint+1, cntPoint);
        }
        else
        {
            Debug(console,"Propack> *** Control Failed....\n");
            return(0); 
        } 

        sndAppDnp->userData[5] = 1;                         /* quantity : 1 */
        sndAppDnp->userData[6] = cntPoint;              /* point */
        sndAppDnp->userData[7] = cntTcf;                /* control: trip, Pulse on */
        //sndAppDnp->userData[7] = dnp->cntTcf + 0x01;        /* control: trip, Pulse on */
        sndAppDnp->userData[8] = 1;                         /* count   */
                
        sndAppDnp->userData[9] = ontime & 0xff;             /* on Time */
        sndAppDnp->userData[10]= (ontime >> 8) & 0xff;      /* on Time */
        sndAppDnp->userData[11]= (ontime >> 16) & 0xff;     /* on Time */
        sndAppDnp->userData[12]= (ontime >> 24) & 0xff;     /* on Time */
        sndAppDnp->userData[13]= offtime & 0xff;            /* on Time */
        sndAppDnp->userData[14]= (offtime >> 8) & 0xff;     /* on Time */
        sndAppDnp->userData[15]= (offtime >> 16) & 0xff;    /* on Time */
        sndAppDnp->userData[16]= (offtime >> 24) & 0xff;    /* on Time */
        sndAppDnp->userData[17]= 0;                         /* status  */

        sndAppDnp->sndCount = 18;
    }    
    
    /* ---------------------------------------------------- */
    /*  2017년 5월 24일 - 지멘스/LG 계전기 DNP 제어         */
    /*      - 제어방식 : Point Latch 방식 제어              */
    /*      - Point Config에 따라서 TRIP(0)/CLOSE(1) 선행   */
    /*      - 제어 Index : dbMAX 항목값 지정                */
    /* ---------------------------------------------------- */
    /*  - 0 : PULSE 제어 (TRIP / CLOSE제어)                 */
	/*  - 1 : PULSE-ON 제어 = CLOSE 우선                    */
	/*  - 2 : PULSE-ON 제어 = TRIP 우선                     */
	/*  - 3 : LATCH-ON 제어 = CLOSE 우선                    */     
	/*  - 4 : LATCH-ON 제어 = TRIP 우선                     */
	/*  - 5 : 예약                                          */
	/*  - 6 : 예약                                          */
    /*  - 7 : AO 포인트 제어, Fault Reset 시                */
    /* ---------------------------------------------------- */
    else if(dnp->type == TYPE_DNP_SIEMENS)
    {
        ptBuf = (POINT_BUF *) &dnp->doPtBuf[dnp->cntPoint];
         
        /* CLASS  Input Read */
        sndAppDnp->userData[0] = 0xc0 + dnp->acSndSeqno;                      /* AC */
        sndAppDnp->userData[1] = 0x04;                      /* FC : select*/
        sndAppDnp->userData[2] = OBJ_CONTROL_RELAY;                        /* Object     : 12  */
        sndAppDnp->userData[3] = 1;                         /* variation  : 1 */
        sndAppDnp->userData[4] = 0x28;                      /* qualifier  : 0x07, Index = 1*/

        sndAppDnp->userData[5] = 1;                         /* quantity : 1 */
        sndAppDnp->userData[6] = 0;                         /* quantity : 1 */

#if 0
        /* ------------------------------------- */
        /* 포인트 Config에 따른 제어 포인트 지정 */
        /* ------------------------------------- */
        if(ptBuf->ptConfig == 0)            /* 지멘스제어기 TRIP/CLOSE  제어시 : TRIP 선행 */
        {
            if(dnp->cntTcf == TRIP_CONTROL)         cntPoint = ptBuf->dbmax;
            else if(dnp->cntTcf == CLOSE_CONTROL)   cntPoint = ptBuf->dbmax + 1;    
        }
        else                                /* 지멘스제어기 TRIP/CLOSE  제어시 : CLOSE 선행 */
        {
            if(dnp->cntTcf == TRIP_CONTROL)         cntPoint = ptBuf->dbmax + 1;
            else if(dnp->cntTcf == CLOSE_CONTROL)   cntPoint = ptBuf->dbmax;    
        }
                
        sndAppDnp->userData[7] = cntPoint & 0xff;             /* point */
        sndAppDnp->userData[8] = (cntPoint >> 8) & 0xff;

        /* ------------------------------------- */
        /*  제어방식 : LATCH-ON 제어 ...         */
        /* ------------------------------------- */
        sndAppDnp->userData[9] = 0x03;        /* control: close, Latch ON */
#endif

        /* ---------------------------------------- */
        /* 제어방식(TC, PULSE, LATCH) 별 제어데이터 */
        /* ---------------------------------------- */
        
        if(dnp->cntrType == DNP_PULSE_CLOSE)                    /* ptConfig == 1, CLOSE-POINT 우선 */  
        {
            if(dnp->cntTcf == CLOSE_CONTROL)                     
            {
                cntPoint = ptBuf->dbmax;                        
                sndAppDnp->userData[7] = cntPoint & 0xff;             
                sndAppDnp->userData[8] = (cntPoint >> 8) & 0xff;
                sndAppDnp->userData[9] = 0x01;                  /* control: close, Pulse ON */
                
                if(opr->soeDebug)
                Debug(console,"snd%2d> operate SIEMENS : Config ... PULSE-CLOSE, CLOSE-Point= %d\n", devid+1, cntPoint);  
            }
            else if(dnp->cntTcf == TRIP_CONTROL)    
            {
                cntPoint = ptBuf->dbmax + 1;                        
                sndAppDnp->userData[7] = cntPoint & 0xff;             
                sndAppDnp->userData[8] = (cntPoint >> 8) & 0xff;
                sndAppDnp->userData[9] = 0x01;                  /* control: close, Pulse ON */
                
                if(opr->soeDebug)
                Debug(console,"snd%2d> operate SIEMENS : Config ... PULSE-CLOSE, TRIP-Point= %d\n", devid+1, cntPoint);  
            }
            else
            {                                         
                cntPoint = ptBuf->dbmax;                        
                sndAppDnp->userData[7] = cntPoint & 0xff;             
                sndAppDnp->userData[8] = (cntPoint >> 8) & 0xff;
                sndAppDnp->userData[9] = 0x00;                  /* control: close, NULL- Op */
            }
        }
        else if(dnp->cntrType == DNP_PULSE_TRIP)                /* ptConfig == 2, TRIP-POINT 우선 */  
        {
            if(dnp->cntTcf == TRIP_CONTROL)                     
            {
                cntPoint = ptBuf->dbmax;                        
                sndAppDnp->userData[7] = cntPoint & 0xff;             
                sndAppDnp->userData[8] = (cntPoint >> 8) & 0xff;
                sndAppDnp->userData[9] = 0x01;                  /* control: close, Pulse ON */
                
                if(opr->soeDebug)
                Debug(console,"snd%2d> operate SIEMENS : Config ... PULSE-TRIP, TRIP-Point= %d\n", devid+1, cntPoint);  
            }
            else if(dnp->cntTcf == CLOSE_CONTROL)    
            {
                cntPoint = ptBuf->dbmax + 1;                        
                sndAppDnp->userData[7] = cntPoint & 0xff;             
                sndAppDnp->userData[8] = (cntPoint >> 8) & 0xff;
                sndAppDnp->userData[9] = 0x01;                  /* control: close, Pulse ON */
                
                if(opr->soeDebug)
                Debug(console,"snd%2d> operate SIEMENS : Config ... PULSE-TRIP, CLOSE-Point= %d\n", devid+1, cntPoint);  
            }
            else
            {                                         
                cntPoint = ptBuf->dbmax;                        
                sndAppDnp->userData[7] = cntPoint & 0xff;             
                sndAppDnp->userData[8] = (cntPoint >> 8) & 0xff;
                sndAppDnp->userData[9] = 0x00;                  /* control: close, NULL- Op */
            }
        }
        else if(dnp->cntrType == DNP_LATCH_CLOSE)               /* ptConfig == 3, LATCH-ON CLOSE 우선 */ 
        {
            if(dnp->cntTcf == CLOSE_CONTROL)                     
            {
                cntPoint = ptBuf->dbmax;                        
                sndAppDnp->userData[7] = cntPoint & 0xff;             
                sndAppDnp->userData[8] = (cntPoint >> 8) & 0xff;
                sndAppDnp->userData[9] = 0x03;                  /* control: close, Pulse ON */
                
                if(opr->soeDebug)
                Debug(console,"snd%2d> operate SIEMENS : Config ... LATCH-CLOSE, CLOSE-Point= %d\n", devid+1, cntPoint);  
            }
            else if(dnp->cntTcf == TRIP_CONTROL)    
            {
                cntPoint = ptBuf->dbmax + 1;                        
                sndAppDnp->userData[7] = cntPoint & 0xff;             
                sndAppDnp->userData[8] = (cntPoint >> 8) & 0xff;
                sndAppDnp->userData[9] = 0x03;                  /* control: close, Pulse ON */
                
                if(opr->soeDebug)
                Debug(console,"snd%2d> operate SIEMENS : Config ... LATCH-CLOSE, TRIP-Point= %d\n", devid+1, cntPoint);  
            }
            else
            {                                         
                cntPoint = ptBuf->dbmax;                        
                sndAppDnp->userData[7] = cntPoint & 0xff;             
                sndAppDnp->userData[8] = (cntPoint >> 8) & 0xff;
                sndAppDnp->userData[9] = 0x00;                  /* control: close, NULL- Op */
            }
        }
        else if(dnp->cntrType == DNP_LATCH_TRIP)               /* ptConfig == 4, LATCH-ON TRIP 우선 */ 
        {
            if(dnp->cntTcf == TRIP_CONTROL)                     
            {
                cntPoint = ptBuf->dbmax;                        
                sndAppDnp->userData[7] = cntPoint & 0xff;             
                sndAppDnp->userData[8] = (cntPoint >> 8) & 0xff;
                sndAppDnp->userData[9] = 0x03;                  /* control: close, Pulse ON */
                
                if(opr->soeDebug)
                Debug(console,"snd%2d> operate SIEMENS : Config ... LATCH-TRIP, TRIP-Point= %d\n", devid+1, cntPoint);  
            }
            else if(dnp->cntTcf == CLOSE_CONTROL)    
            {
                cntPoint = ptBuf->dbmax + 1;                        
                sndAppDnp->userData[7] = cntPoint & 0xff;             
                sndAppDnp->userData[8] = (cntPoint >> 8) & 0xff;
                sndAppDnp->userData[9] = 0x03;                  /* control: close, Pulse ON */
                
                if(opr->soeDebug)
                Debug(console,"snd%2d> operate SIEMENS : Config ... LATCH-TRIP, CLOSE-Point= %d\n", devid+1, cntPoint);  
            }
            else
            {                                         
                cntPoint = ptBuf->dbmax;                        
                sndAppDnp->userData[7] = cntPoint & 0xff;             
                sndAppDnp->userData[8] = (cntPoint >> 8) & 0xff;
                sndAppDnp->userData[9] = 0x00;                  /* control: close, NULL- Op */
            }
        }
        else                                                    /* ptConfig == 0, TRIP/CLOSE */ 
        {   
            cntPoint = ptBuf->dbmax;                        
            sndAppDnp->userData[7] = cntPoint & 0xff;             
            sndAppDnp->userData[8] = (cntPoint >> 8) & 0xff;
                        
            sndAppDnp->userData[9] = dnp->cntTcf + 0x01;        /* control: TRIP/CLOSE, Pulse on */
            
            if(opr->soeDebug)
            Debug(console,"snd%2d> operate SIEMENS : Config ... TRIP/CLOSE, Point= %d\n", devid+1, cntPoint);  
        }
        
                
        sndAppDnp->userData[10] = 1;                         /* count   */
        sndAppDnp->userData[11] = ontime & 0xff;             /* on Time */
        sndAppDnp->userData[12]= (ontime >> 8) & 0xff;      /* on Time */
        sndAppDnp->userData[13]= (ontime >> 16) & 0xff;     /* on Time */
        sndAppDnp->userData[14]= (ontime >> 24) & 0xff;     /* on Time */
        sndAppDnp->userData[15]= offtime & 0xff;            /* on Time */
        sndAppDnp->userData[16]= (offtime >> 8) & 0xff;     /* on Time */
        sndAppDnp->userData[17]= (offtime >> 16) & 0xff;    /* on Time */
        sndAppDnp->userData[18]= (offtime >> 24) & 0xff;    /* on Time */
        sndAppDnp->userData[19]= 0;                         /* status  */

        sndAppDnp->sndCount = 20;

        if(opr->soeDebug)
        Debug(console,"snd%2d> operate : Point= %d, TCF = %2x (base=%d )\n", devid+1, dnp->cntPoint+1, dnp->cntTcf, cntPoint);               
    }
    else
    {
    	/* GIPAM2000, FRTU 등등 */	
        /* CLASS  Input Read */
        sndAppDnp->userData[0] = 0xc0 + dnp->acSndSeqno;                      /* AC */
        sndAppDnp->userData[1] = 0x04;                      /* FC : operate*/
        sndAppDnp->userData[2] = OBJ_CONTROL_RELAY;                        /* Object     : 12  */
        sndAppDnp->userData[3] = 1;                         /* variation  : 1 */
        sndAppDnp->userData[4] = 0x17;                      /* qualifier  : 0x07, Index = 1*/

        sndAppDnp->userData[5] = 1;                         /* quantity : 1 */
        sndAppDnp->userData[6] = dnp->cntPoint;             /* point */
        
        /* ---------------------------------------- */
        /* 제어방식(TC, PULSE, LATCH) 별 제어데이터 */
        /* ---------------------------------------- */
        if(dnp->cntrType == DNP_PULSE_CLOSE)                    /* ptConfig == 1, CLOSE-POINT 우선 */  
        {
            if(dnp->cntTcf == CLOSE_CONTROL)        
            {
                sndAppDnp->userData[6] = dnp->cntPoint;         
                sndAppDnp->userData[7] = 0x01;                  /* POINT PULSE ON */
                
                if(opr->soeDebug)
                Debug(console,"snd%2d> operate : Config ... PULSE-CLOSE, CLOSE-Point= %d\n", devid+1, sndAppDnp->userData[6]);  
            }
            else if(dnp->cntTcf == TRIP_CONTROL)    
            {
                sndAppDnp->userData[6] = dnp->cntPoint + 1;     /* point = CLOSE-Point + 1*/
                sndAppDnp->userData[7] = 0x01;                  /* POINT PULSE ON */
                
                if(opr->soeDebug)
                Debug(console,"snd%2d> operate : Config ... PULSE-CLOSE, TRIP-Point= %d\n", devid+1, sndAppDnp->userData[6]);  
            }
            else                                    
                sndAppDnp->userData[7] = 0x00;       
        }
        else if(dnp->cntrType == DNP_PULSE_TRIP)                /* ptConfig == 2, TRIP-POINT 우선 */  
        {
            if(dnp->cntTcf == TRIP_CONTROL)        
            {
                sndAppDnp->userData[6] = dnp->cntPoint;         
                sndAppDnp->userData[7] = 0x01;                  /* POINT PULSE ON */
                
                if(opr->soeDebug)
                Debug(console,"snd%2d> operate : Config ... PULSE-TRIP, TRIP-Point= %d\n", devid+1, sndAppDnp->userData[6]);  
            }
            else if(dnp->cntTcf == CLOSE_CONTROL)    
            {
                sndAppDnp->userData[6] = dnp->cntPoint + 1;     /* point = CLOSE-Point + 1*/
                sndAppDnp->userData[7] = 0x01;                  /* POINT PULSE ON */
                
                if(opr->soeDebug)
                Debug(console,"snd%2d> operate : Config ... PULSE-TRIP, CLOSE-Point= %d\n", devid+1, sndAppDnp->userData[6]);  
            }
            else                                    
                sndAppDnp->userData[7] = 0x00;       
        }
        /* -------------------------------------------- */
        /*  원흥역사의 경우....외부 DO 사용시           */
        /* -------------------------------------------- */
        else if(dnp->cntrType == DNP_LATCH_CLOSE)               /* ptConfig == 3, LATCH-ON, CLOSE POINT 우선 */  
        {
            if(dnp->cntTcf == CLOSE_CONTROL)        
            {
                sndAppDnp->userData[6] = dnp->cntPoint;         /* point */
                sndAppDnp->userData[7] = 0x03;                  /* POINT LATCH ON */
                
                if(opr->soeDebug)
                Debug(console,"snd%2d> operate : Config ... LATCH-CLOSE, CLOSE-Point= %d\n", devid+1, sndAppDnp->userData[6]);  
            }
            else if(dnp->cntTcf == TRIP_CONTROL)    
            {
                sndAppDnp->userData[6] = dnp->cntPoint + 1;     /* point = CLOSE-Point + 1*/
                sndAppDnp->userData[7] = 0x04;                  /* POINT LATCH OFF */       
                
                if(opr->soeDebug)
                Debug(console,"snd%2d> operate : Config ... LATCH-CLOSE, TRIP-Point= %d\n", devid+1, sndAppDnp->userData[6]); 
            }
            else                                    
                sndAppDnp->userData[7] = 0x00;     
        }
        else if(dnp->cntrType == DNP_LATCH_TRIP)               /* ptConfig == 4, LATCH-ON, TRIP POINT 우선 */  
        {
            if(dnp->cntTcf == TRIP_CONTROL)        
            {
                sndAppDnp->userData[6] = dnp->cntPoint;         /* point */
                sndAppDnp->userData[7] = 0x03;                  /* POINT LATCH ON */
                
                if(opr->soeDebug)
                Debug(console,"snd%2d> operate : Config ... LATCH-TRIP, TRIP-Point= %d\n", devid+1, sndAppDnp->userData[6]);  
            }
            else if(dnp->cntTcf == CLOSE_CONTROL)    
            {
                sndAppDnp->userData[6] = dnp->cntPoint + 1;     /* point = CLOSE-Point + 1*/
                sndAppDnp->userData[7] = 0x04;                  /* POINT LATCH OFF */    
                
                if(opr->soeDebug)
                Debug(console,"snd%2d> operate : Config ... LATCH-TRIP, CLOSE-Point= %d\n", devid+1, sndAppDnp->userData[6]);     
            }
            else                                    
                sndAppDnp->userData[7] = 0x00;     
        }        
        else if(dnp->cntrType == DNP_RESET_CLOSE)               /* ptConfig == 5, 광명부하개폐기, RESET 제어 - CLOSE 전용 */ 
        {
        	/* 제어상태 임의지정 => CLOSE 제어 */
        	dnp->cntTcf = CLOSE_CONTROL;
       	    sndAppDnp->userData[7] = dnp->cntTcf + 0x01;        /* control: TRIP/CLOSE, Pulse on */
            
            if(opr->soeDebug)
            Debug(console,"snd%2d> operate : Config ... RESET-CLOSE, Point= %d\n", devid+1, sndAppDnp->userData[6]); 
        }
        else if(dnp->cntrType == DNP_RESET_TRIP)               	/* ptConfig == 6, 광명부하개폐기, RESET 제어 - TRIP 전용 */ 
        {
        	/* 제어상태 임의지정 => TRIP 제어 */
        	dnp->cntTcf = TRIP_CONTROL;
       	    sndAppDnp->userData[7] = dnp->cntTcf + 0x01;        /* control: TRIP/CLOSE, Pulse on */
            
            if(opr->soeDebug)
            Debug(console,"snd%2d> operate : Config ... RESET-TRIP, Point= %d\n", devid+1, sndAppDnp->userData[6]); 
        }        
        else
        {           
            sndAppDnp->userData[7] = dnp->cntTcf + 0x01;        /* control: TRIP/CLOSE, Pulse on */
            
            if(opr->soeDebug)
            Debug(console,"snd%2d> operate : Config ... TRIP/CLOSE, Point= %d\n", devid+1, sndAppDnp->userData[6]);    
        }
        
        sndAppDnp->userData[8] = 1;                         /* count   */
        sndAppDnp->userData[9] = ontime & 0xff;             /* on Time */
        sndAppDnp->userData[10]= (ontime >> 8) & 0xff;      /* on Time */
        sndAppDnp->userData[11]= (ontime >> 16) & 0xff;     /* on Time */
        sndAppDnp->userData[12]= (ontime >> 24) & 0xff;     /* on Time */
        sndAppDnp->userData[13]= offtime & 0xff;            /* on Time */
        sndAppDnp->userData[14]= (offtime >> 8) & 0xff;     /* on Time */
        sndAppDnp->userData[15]= (offtime >> 16) & 0xff;    /* on Time */
        sndAppDnp->userData[16]= (offtime >> 24) & 0xff;    /* on Time */
        sndAppDnp->userData[17]= 0;                         /* status  */

        sndAppDnp->sndCount = 18;

        if(opr->soeDebug)
        {    
            Debug(console,"snd%2d> operate : Point= %d, TCF = %2x \n", devid+1, dnp->cntPoint+1, dnp->cntTcf);        
            prndat(">> ", (char *) &sndAppDnp->userData[2], 16);
        }
    }
    

    /* 제어 출력시 soe에서의 상태 dump off */

    return (sndAppDnp->sndCount);
        
}



/*
*   Select Write : Object-12, Variation-1, Qcode-0x07, Index-1
*/
int dnpAoV2SelectWrite(int scanid, int devid)
{
    //int             ontime, offtime;

    DNP_DEVICE      *dnp;
    DNP_APP_FRAME   *sndAppDnp;

    dnp = (DNP_DEVICE *) dnpcfg[devid];
    sndAppDnp = (DNP_APP_FRAME *) &dnp->sndAppFrame;

    sndAppDnp->sndCount = 0;

    /* CLASS  Input Read */
    sndAppDnp->userData[0] = 0xc0 + dnp->acSndSeqno;              /* AC */
    sndAppDnp->userData[1] = 0x03;              /* FC : select*/
    sndAppDnp->userData[2] = OBJ_AOUT_BLOCK;    /* Object     : 41 */
    sndAppDnp->userData[3] = 02;                /* variation  : 2, 16 Bit Analog output  */
    sndAppDnp->userData[4] = 0x17;              /* qualifier  : 0x07, Index = 1*/
    sndAppDnp->userData[5] = 1;                 /* quantity : 1 */
    sndAppDnp->userData[6] = dnp->aoVpoint;     /* point */
    sndAppDnp->userData[7] = dnp->aoVpointData; /* data 0 */
    sndAppDnp->userData[8] = 0x00;              /* data 1 */
    sndAppDnp->userData[9]= 0;                  /*      0 */
    sndAppDnp->sndCount = 10;
    
    if(opr->soeDebug)
    Debug(console,"snd%2d> select AO-V2: Point= %d, data = %2x \n", devid+1, dnp->aoVpoint, dnp->aoVpointData);

    return (sndAppDnp->sndCount);
}

/*
*   Select Write : Object-12, Variation-1, Qcode-0x07, Index-1
*/
int dnpAoV2OperateWrite(int scanid, int devid)
{
    DNP_DEVICE      *dnp;
    DNP_APP_FRAME   *sndAppDnp;

    dnp = (DNP_DEVICE *) dnpcfg[devid];
    //sndAppDnp = (DNP_APP_FRAME *) &sndAppFrame[fid];
    sndAppDnp = (DNP_APP_FRAME *) &dnp->sndAppFrame;

    sndAppDnp->sndCount = 0;

    /* CLASS  Input Read */
    sndAppDnp->userData[0] = 0xc0 + dnp->acSndSeqno;              /* AC */
    sndAppDnp->userData[1] = 0x04;              /* FC : operate*/
    sndAppDnp->userData[2] = OBJ_AOUT_BLOCK;    /* Object     : 41  */
    sndAppDnp->userData[3] = 02;                /* variation  : 2 */
    sndAppDnp->userData[4] = 0x17;              /* qualifier  : 0x07, Index = 1*/
    sndAppDnp->userData[5] = 1;                 /* quantity : 1 */
    sndAppDnp->userData[6] = dnp->aoVpoint;     /* point */
    sndAppDnp->userData[7] = dnp->aoVpointData; /* control: trip, Pulse on */
    sndAppDnp->userData[8] = 0x00;              /* count   */
    sndAppDnp->userData[9]= 0;                  /* on Time */
    sndAppDnp->sndCount = 10;

    if(opr->soeDebug)
    Debug(console,"snd%2d> operate AO-V2: Point= %d, data = %2x \n", devid+1, dnp->aoVpoint, dnp->aoVpointData);

    /* 제어 출력시 soe에서의 상태 dump off */

    return (sndAppDnp->sndCount);
}


/*
*   Select Write : Object-12, Variation-1, Qcode-0x07, Index-1
*   Fault reset Select.......!!
*/
int dnpAoFaultSelectWrite(int scanid, int devid)
{
    //int             ontime, offtime;

    DNP_DEVICE      *dnp;
    DNP_APP_FRAME   *sndAppDnp;

    dnp = (DNP_DEVICE *) dnpcfg[devid];
    //sndAppDnp = (DNP_APP_FRAME *) &sndAppFrame[fid];
    sndAppDnp = (DNP_APP_FRAME *) &dnp->sndAppFrame;

    sndAppDnp->sndCount = 0;

    /* CLASS  Input Read */
    sndAppDnp->userData[0] = 0xc0 + dnp->acSndSeqno;              /* AC */
    sndAppDnp->userData[1] = 0x03;              /* FC : select*/
    sndAppDnp->userData[2] = OBJ_AOUT_BLOCK;                /* Object     : 41  */
    sndAppDnp->userData[3] = 02;                /* variation  : 1 */
    sndAppDnp->userData[4] = 0x17;              /* qualifier  : 0x07, Index = 1*/       
    sndAppDnp->userData[5] = 1;                 /* quantity : 1 */
    sndAppDnp->userData[6] = 129;               /* point */
    sndAppDnp->userData[7] = 0xaa;              /* data 0 */
    sndAppDnp->userData[8] = 0x00;              /* data 1 */
    sndAppDnp->userData[9] = 0;                 /* on Time */
    sndAppDnp->sndCount = 10;
    
    return (sndAppDnp->sndCount);
}

/*
*   Select Write : Object-12, Variation-1, Qcode-0x07, Index-1
*   Fault Reset Operation.....!!
*/
int dnpAoFaultOperateWrite(int scanid, int devid)
{
    int             ontime, offtime;

    DNP_DEVICE      *dnp;
    DNP_APP_FRAME   *sndAppDnp;

    dnp = (DNP_DEVICE *) dnpcfg[devid];
    //sndAppDnp = (DNP_APP_FRAME *) &sndAppFrame[fid];
    sndAppDnp = (DNP_APP_FRAME *) &dnp->sndAppFrame;

    sndAppDnp->sndCount = 0;

    /* 상위 제어시간 정보 적용 */
    if((opr->rcvONtime <= 500 ) || (opr->rcvONtime > 10000))    ontime = 1000;          // 수신한 제어시간 정보
    else                                                        ontime = opr->rcvONtime;

    if((opr->rcvOFFtime <= 500 ) || (opr->rcvOFFtime > 10000))  offtime = 1000;          // 수신한 제어시간 정보
    else                                                        offtime = opr->rcvOFFtime;    

    /* CLASS  Input Read */
    sndAppDnp->userData[0] = 0xc0 + dnp->acSndSeqno;              /* AC */
    sndAppDnp->userData[1] = 0x04;              /* FC : operate */
    sndAppDnp->userData[2] = OBJ_AOUT_BLOCK;                /* Object     : 41  */
    sndAppDnp->userData[3] = 02;                /* variation  : 2 */
    sndAppDnp->userData[4] = 0x17;              /* qualifier  : 0x07, Index = 1*/       
    sndAppDnp->userData[5] = 1;                 /* quantity : 1 */
    sndAppDnp->userData[6] = 129;               /* point */
    sndAppDnp->userData[7] = 0xaa;              /* control: trip, Pulse on */
    sndAppDnp->userData[8] = 0x00;              /* count   */
    sndAppDnp->userData[9] = 0;                 /* on Time */
    sndAppDnp->sndCount = 10;

    return (sndAppDnp->sndCount);
}


/*
*   DNP 송신메세지 구성
*
*/
int makeCmdRELAY(int mode, int scanid, int devid)
{
    int     retVal;

    DNP_DEVICE      *dnp;
    DNP_APP_FRAME   *sndAppDnp;
    SCAN_CONFIG     *scan;

    dnp = (DNP_DEVICE *) dnpcfg[devid];
    
    //printf("mk-relay... %d\n", devid);
    
    /* SCAN-TASK 별 모듈구성 정보 */ 
    scan = (SCAN_CONFIG *) scanCfg[scanid];
    sndAppDnp = (DNP_APP_FRAME *) &dnp->sndAppFrame;

            
    /* ---------------------------------- */
    /* Offline 시 : check LINK-Status ... */
    /* ---------------------------------- */
    if(dnp->sndLinkSts == RESET)
    {
        if(++dnp->offlineTick > dnp->offCount)     
        {
            dnp->offlineTick = 0;
            if(dnp->online != 0)    
            {
                setOffline(devid);
            }
        }
        
        //dnp->comSndCount++;
        resetDataLink( mode, scanid, devid);
        return (1);
    }

    /* check Online */
    if(dnp->online != SET)    
    {
        setOnline(devid);
        opr->cpuChangeTick = 0;
    }
    
    if(dnp->online == SET)         dnp->offlineTick = 0;

    dnp->sndFlag = RESET;

    if((opr->dnpDebug == devid) || (opr->scanDebug == scanid))
    {
        Debug(console,"\n-----------------------------------------[%02d/%02d-%02d:%02d:%02d]\n", 
        	rtc->month, rtc->day, rtc->hour, rtc->min, rtc->sec);
    }
    
    /* ---------------------------------------- */
    /* DNP APP-Command Handler                  */
    /* ---------------------------------------- */
    if(dnp->sndFlag == RESET)
    {
        if(dnp->devRestart == SET)          dnpIINWrite(scanid, devid);
        else if(dnp->timeSyncReq == SET)    dnpTimeSync(scanid, devid);
        else if(dnp->selectReq == SET)      dnpSelectWrite(scanid, devid);
        else if(dnp->operateReq == SET)     dnpOperateWrite(scanid, devid);
        else if(dnp->anaVOutSelect == SET)  dnpAoV2SelectWrite(scanid, devid);          /* V0    Reset Select */
        else if(dnp->anaVOutOpr == SET)     dnpAoV2OperateWrite(scanid, devid);         /* V0    Reset Operate */ 
        else if(dnp->class1Req == SET)      dnpClassRead(scanid, devid, DNP_CLASS1);
        else if(dnp->class2Req == SET)      dnpClassRead(scanid, devid, DNP_CLASS2);
        else if(dnp->class3Req == SET)      dnpClassRead(scanid, devid, DNP_CLASS3);
        else if(dnp->statusDump == SET)     dnpBinaryInput(scanid, devid);
        else if(dnp->analogDump == SET)     dnpAnalogInput(scanid, devid);
        //else if(dnp->countDump == SET)      dnpCountInput(scanid, devid);        
        else
        {
            /* -------------------------------------------------------- */
    		/*	2024.12.10 DNP 통신시 Change Check 명령 처리					*/
		    /*	=> Change Check 명령을 하지 않음 ... modProfile 을 4로 지정함...	*/
		    /* 	modbusFileNo => PROFILE 4 지정							*/
    		/* -------------------------------------------------------- */
    		if(dnp->modbusFileNo == 3)	dnpBinaryInput(scanid, devid);
    		else						dnpBinaryChange(scanid, devid);	
        }            
        
        /* ------------------------------------ */
        /*  APPLICATION Command Request...      */
        /* ------------------------------------ */
        if(sndAppDnp->sndCount)
        {
            /* 장치별 통신상태 Check : Device Com rate ... */
            if(dnp->comSndCount >= 1000)
            {
                dnp->oldRcvCount = dnp->comRcvCount;
                dnp->comSndCount = 0;
                dnp->comRcvCount = 0;
            }

            /* 통신모드(TCPIP/RS232) 에따라서 전송 ... */
            retVal = sendAppFrame( mode, scanid, devid, sndAppDnp);
            
            dnp->comSndCount++;
            dnp->appFailTick = 0;
        }
    }

    /* ---------------------------------------------- */
    /* APP 송신 / 응답 없는 경우 실패... Retry 5회    */
    /* ---------------------------------------------- */
    if(++dnp->appFailTick > 10)
    {
        dnp->appFailTick = 0;
        dnp->sndFlag    = RESET;    // 송신상태 Flag Clear...
        Debug(console,"*[%2d] APP send Error...!\n", devid);
    }

    return (1);
}

/*
*   FUNCTION : dnpDevInitial()
*   - 계전기 장치 구조체 ... 초기화
*/
int dnpFlagClear( int devid)
{
    //int i;
    DNP_DEVICE  *dnp;

    /* --------------------------- */
    /*    pointer init             */
    /* --------------------------- */
    dnp = (DNP_DEVICE *) dnpcfg[devid];

    /* DNP initialize ... */
    dnp->sndFlag    = RESET;
    dnp->sndLinkSts = RESET;
    dnp->rcvLinkSts = RESET;
    
    dnp->sndLinkSts = RESET;
    dnp->sndFcvBit = FCV_BIT;
    dnp->sndFcbBit = FCB_BIT;
                
    dnp->thSndSeqno = 0;                
    dnp->acSndSeqno = 0;
    dnp->ACseqFail  = 0;
    dnp->timeOutTick= 0;

    dnp->timeSyncReq    = RESET;
    dnp->selectReq      = RESET;
    dnp->operateReq     = RESET;
    dnp->statusDump     = RESET;
    dnp->analogDump     = RESET;
    dnp->anaOutDump     = RESET;
    dnp->anaVOutSelect  = RESET;
    dnp->anaVOutOpr     = RESET;
    dnp->classReq       = RESET;
    dnp->class1Req      = RESET;
    dnp->class2Req      = RESET;
    dnp->class3Req      = RESET;
    
    return (0);
            
}

/* ------------------------------------------------------- */
/*  SCAN THREAD Start Routine ....              */
/*  - ASYNC 용 타스크                           */
/* ------------------------------------------------------- */
void    *scan_dnp_thread(int  *arg)
{
    int     i,scanid;
    int		comPort;
    int     devid, devIndex;
    int     chanComFail;
    int     chanReset;
    int     rxcnt, retVal;
    int     oldsec=0;
    int     reqTimeTick;
    int		taskDelay;
    int     startDev, stopDev;
    int     timeout;
    int     vmeChanActive;
    //int    	timedelay;
    byte    rxbuf[32];
    char    buffer[128];
    
    DNP_DEVICE      *dnp;
	TTY_DESC        *scanPort=NULL;
	SCAN_CONFIG     *scan;
	//struct timeval         tv;
	
	DNP_DATA_LINK   dataLinkFrame;
	DNP_DATA_LINK   *dnpData;
	
	/* -------------------------------------------- */
	/* THREAD Argment Assign ...                    */
	/* -------------------------------------------- */
	scanid = (int ) *arg;

    dnpData   = (DNP_DATA_LINK *) &dataLinkFrame;
    
    /* SCAN-TASK 별 모듈구성 정보 */ 
    scan = (SCAN_CONFIG *) scanCfg[scanid];
    comPort = scan->comPort;
    vmeChanActive = RESET;
    
    /* -------------------------------------------- */
    /* SCAN THREAD - 통신 Delay 설정                */
    /* -------------------------------------------- */
    taskDelay = scan->comDelay * 10;
    if(taskDelay < 5)		taskDelay = 50;
    if(taskDelay > 2000)	taskDelay = 2000;
   
    pause(1000);
     	
    /* -------------------------------------------- */
    /* SCAN THREAD - MASTER : 통신관련 변수 초기화  */
    /* -------------------------------------------- */
    if(comPort < MAX_CPUPORT)
    {
        scanPort = (TTY_DESC *) &scanChan[comPort];
        
        /* COM1 통신속도 반영 */
        if(scan->comSpeed == SPEED_9600)        scanPort->baudRate = 9600;
        else if(scan->comSpeed == SPEED_19200)  scanPort->baudRate = 19200;
        else if(scan->comSpeed == SPEED_38400)  scanPort->baudRate = 38400;
        else if(scan->comSpeed == SPEED_57600)  scanPort->baudRate = 57600;
        else                                    scanPort->baudRate = 38400;
        
        retVal = PortInitial((TTY_DESC *) scanPort);
        if(retVal == -1)
        {
            termExec = 0;
            printf("\n===========================================\n");
            printf("scan> *Serial Com Port Open...ERROR...return = %d \n", retVal);
            printf("===========================================\n");
            pause(1000);
            pause(1000);
        }
        
        startDev = 0;
        stopDev  = scan->devNum;
        
        for(i = startDev; i < stopDev; i++)
        {
            devid = scan->device[i] & MAX_MODULE_MASK;
            dnp = (DNP_DEVICE *) dnpcfg[devid];
            
            dnp->ttyPort = (TTY_DESC *) scanPort;
            dnp->offCount = scan->offCount;
            dnpFlagClear(devid);
        }                
    }
    else if((comPort >= VME_COM1) && (comPort <= VME_COM8))           // VME-SIO 통신포트 #1
    {
        startDev = 0;
        stopDev  = scan->devNum;
        
        /* ------------------------ */
        /* SIO Board 를 사용함      */
        /* ------------------------ */
        opr->useSIOBoard = SET;
    
        for(i = startDev; i < stopDev; i++)
        {
            devid = scan->device[i] & MAX_MODULE_MASK;
            dnp = (DNP_DEVICE *) dnpcfg[devid];
            
            dnp->vmeChan = (VME_CHAN_DCB *) vmeChanDCB[comPort - VME_COM1];
            dnp->ttyPort = NULL;
            dnpFlagClear(devid);
            
            /* VMECHAN 프로토콜 타입 지정 */
            vmeChanActive = SET;
            dnp->vmeChan->protocolType = SCAN_DNP;
            dnp->vmeChan->cfgSpeed     = scan->comSpeed;
            dnp->vmeChan->cfgType      = COM_RS232;
            
            dnp->vmeChan->rxFront   = 0;
            dnp->vmeChan->rxRear    = 0;
            
            dnp->offCount = scan->offCount;
        }                
        
        vmeScanInitial(scanid, comPort - VME_COM1);
    }        
    else
    {
 	    while(termExec)
	    {
	        if(opr->wdtDebug)
	        Debug(console,">>> *Invalid SCAN[%d] COM-PORT  ... !\n", scanid+1);
	        
	        opr->scanWDT[scanid] = 0;
	        pause(1000);
	    }        
    }

    printf(" %s SCAN[%02d] DNP-PROCESS Activated ......(COM-PORT=%02d, Delay = %d ms, Offline Count=%d) !\n", TARGET_NAME, scanid+1, scan->comPort, taskDelay, scan->offCount);
    
    sprintf(buffer, "%s SCAN[%02d] DNP-PROCESS Activated ......(COM-PORT=%02d, Delay = %d ms, Offline Count=%d) !", TARGET_NAME, scanid+1, scan->comPort, taskDelay, scan->offCount);
    LogFile_MPU ( opr, NOT_DEFINED, buffer, strlen(buffer));

#if 1
    printf("DNP-SCAN[%02d] device Num = %02d ... ", scanid, scan->devNum );
    for(i=0; i< scan->devNum; i++)  
    {
        printf(" %2d", scan->device[i]);
    }
    printf("\n");
#endif

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
	            
    devIndex    = 0;
    chanComFail = 0;
    chanReset   = 0;
    reqTimeTick = 150;


	while(termExec)
	{
	    /* -------------------------------------------- */
	    /*  계전기별 통신 지연                          */
	    /* -------------------------------------------- */
	    opr->scanWDT[scanid] = 0;
	    pause(taskDelay);
	    
		/* -------------------------------------------- */
		/*	CPU 이중화 : LOCAL-SLAVE ... 대기모드       */      
		/* -------------------------------------------- */
		if(opr->runMode == LOCAL_SLAVE)		
		{
			/* ---------------------------------------- */
			/*	SLAVE 모드에서 수신된 데이터를 처리함   */
			/* ---------------------------------------- */
			chanComFail = 0;
            chanReset   = 0;
            
            devid = scan->device[0];
            for(i=0; i< 32; i++)
            {
                if(dnpChanRead(scanid, devid, &rxbuf[0], 16) == 0) break;
            } 
            
            continue;
        }     

        /* ---------------------------------------- */
        /* 계전기 DEVICE Comm Handling ...          */
        /* ---------------------------------------- */
        for(;;)
        {
            devid = scan->device[devIndex] & MAX_MODULE_MASK;
            dnp = (DNP_DEVICE *) dnpcfg[devid];
            
            //if(dnp->sndFlag == RESET)
            {
                if(++devIndex >= scan->devNum)
                {
                    devIndex = 0;
                    devid = scan->device[devIndex] & MAX_MODULE_MASK;
                    
                    /* 계전기 초기데이터 수집 */
                    if(scan->scaninit)
                    {
                        if(--scan->scaninit == 0)  
                        {
                            Debug(console,"scan%2d> dnp module scan end ...!\n", scanid);
                        }       
                    }
                    
                    if(opr->rtuinit)
                    {
                        if(--opr->rtuinit == 0)  
                        {
                            Debug(console,"scan%2d> Module-Init scan end ...!\n", scanid);
                        }       
                    }
                    
                    pause(50);
                }
            }
            
            /* ---------------------------------------- */
            /* 현재 선택된 계전기 : 송수신...           */
            /* ---------------------------------------- */
            devid = scan->device[devIndex] & MAX_MODULE_MASK;
            dnp = (DNP_DEVICE *) dnpcfg[devid];
            
            /* 선택된 계전기별 통신 Buffer 할당 */
            dnpData   = (DNP_DATA_LINK *) &dnp->dataLinkFrame;
    
            if((dnp->type != 0) && (dnp->scan == SET))   
            {
                opr->devScanCount[scanid]++;
                break;
            }
            else
            {
                dnp->sndFlag = RESET;               
            }
        }
        
        /* ---------------------------------------- */
        /* 제어명령시 우선적으로 Device 고정        */
        /* ---------------------------------------- */
        if(opr->cntrFlag[scanid] == SET)
        {
            devid = opr->cntrDevId[scanid] & MAX_MODULE_MASK;
            dnp   = (DNP_DEVICE *) dnpcfg[devid];
            
            /* 선택된 계전기별 통신 Buffer 할당 */
            dnpData   = (DNP_DATA_LINK *) &dnp->dataLinkFrame;
            
            if(++opr->cntrComTick[scanid] > 20)
            {
                opr->cntrFlag[scanid]   = 0;
                opr->cntrComTick[scanid]= 0;
                
                dnp->selectReq      = RESET;
                dnp->operateReq     = RESET;
                dnp->anaVOutSelect  = RESET;
                dnp->anaVOutOpr     = RESET;
            }
        }

        /* ---------------------------------------- */
        /* make Command: 보호계전기 송수신 루틴...  */
        /* ---------------------------------------- */
        if(vmeChanActive == SET)
        {    
            dnp->vmeChan->rxFront   = 0;
            dnp->vmeChan->rxRear    = 0;
        }
        
        /* ------------------------------------------ */
        /* Offline 계전기에 대한 주기적 통신 허용 ... */
        /* ------------------------------------------ */            
        if(scan->changeMode == 0)
        {
            if(dnp->online == 0)
            {
                if(dnp->timeOutTick++ > 10)     dnp->timeOutTick = 0;
                else                            continue;
            }
        }
                    
        /* ---------------------------------------- */
        /* make Command: 보호계전기 송수신 루틴...  */
        /* ---------------------------------------- */
        if(makeCmdRELAY( COM_RS485, scanid, devid) > 0)
        {
            /* 2015.04.06 : 계전기당 수신대기시간 조정 (20 => 50) */
            reqTimeTick = 50;           // 수신지연 : 1000ms 정도 
                
            /* ------------------------ */
            /* 모듈별 Time out 처리     */
            /* ------------------------ */
            for(timeout = 0; timeout < reqTimeTick; timeout++)
            {
                usleep(2000);
                if((rxcnt = readDataLink( scanid, devid, dnpData)) > 0)
                {
                	timeout = 0;
                	
                	chanReset   = 0;
                    chanComFail = 0;      
                    dnp->comFailTick  = 0;
                    //opr->cpuChangeTick = 0;          
                    opr->scanComSts[scanid] = 1;            // SCAN-PORT 통신상태
                    
                    dnpData->rcvCount = rxcnt;
                    dnp->timeOutTick  = 0;
                    
                    if((opr->dnpDebug == devid) || (opr->scanDebug == scanid))
                        DumpDNP_rcv(console, "RXD: ", &dnpData->rcvFrame, rxcnt);

                    /* ---------------------------- */
                    /* check Function Code ...      */
                    /* ---------------------------- */
                    retVal = chkRcvFrame(COM_RS485, scanid, devid, dnpData);
                    if((retVal == FUNC_USER_DATA) || (retVal == FUNC_UNCONFIRM))
                    {
                    	dnp->comRcvCount++;
                        retVal = checkDataLink( COM_RS485, scanid, devid, dnpData);
                        
                        if(retVal)	break;
                    }

#if 0                    
                    /* ---------------------------- */
                    /* 2015.04.06 : 계전기당 처리   */
                    /* - Unsolite 메세지 skip...    */
                    /* ---------------------------- */
                    break;
#endif                    
                }
                
            }/*timeout */ 
            
            
            /* ---------------------------------------- */
            /*  계전기별 통신이상시...  Offline         */    
            /* ---------------------------------------- */        
            if(++dnp->comFailTick > scan->offCount)
            {
                if((opr->dnpDebug == devid) || (opr->scanDebug == scanid))
                Debug(console,"scan%02d> *DNP-Device(%d) Comm Fail..OffCount= %d : %d !\n", scanid+1, devid+1, scan->offCount, dnp->comFailTick );   
                
                dnp->comFailTick = 0;
                dnp->offlineTick = scan->offCount;

                dnpFlagClear(devid);

                if(vmeChanActive == SET)    
                {
                    vmeScanChanReset(devid);
                    dnp->vmeChan->protocolType = SCAN_DNP;
                    dnp->vmeChan->cfgSpeed     = scan->comSpeed;
                    dnp->vmeChan->cfgType      = COM_RS232;
                
                    vmeScanInitial(scanid, scan->comPort - VME_COM1);
                    
                    if((opr->dnpDebug == devid) || (opr->scanDebug == scanid))
                    Debug(console,"scan%02d> *Device(%d) Comm Fail..VME RESET \n", scanid+1, devid+1);   
                }
            
                /* ---------------------------------------- */ 
                /* 개별 장치별 Offline 상태 : CPU 절체      */
                /* ---------------------------------------- */ 
                if((opr->runMode == LOCAL_MASTER) && (scan->changeMode == CHANGE_EACH_DEVICE))
                {
                    if((opr->dualCpuSts == SET) && (scuCfg->remoteMode == AUTO_MODE) && (opr->cpuChangeTick < CHANGE_CPU_TICK))   
                    {
                        pause(1000);
                        opr->cpuChange = SET;
                        logEvent1(opr, sque, EVENT_CHANGE_CPU, CHANGE_EACH_DEVICE, 0, 0, opr->cpuMode);   
                        logHistoryQue1(opr, hque, EVENT_CHANGE_CPU, CHANGE_EACH_DEVICE, 0, 0, opr->cpuMode);   
                        
                        /* -------------------------------- */
                        /* LOG File 저장                    */
                        /* -------------------------------- */
                        sprintf(buffer, "[DNP] 개별장치 통신절체...Device=%d, changeTick=%d ", devid+1, opr->cpuChangeTick);
                        LogFile_MPU (opr, EVENT_CHANGE_CPU, buffer, strlen(buffer));   

                        pause(1000);
                        
                        opr->cpuChangeTick++;
                        
                        if(opr->wdtDebug)
                        {
                            Debug(console,"\n--------------------------------------------\n");
                            Debug(console,"scan%02d> %02d:%02d:%02d *** DNP scan Com-Fail... CPU Change (%d) ...!\n", scanid+1, rtc->hour, rtc->min, rtc->sec, opr->cpuChangeTick);
                            Debug(console,"--------------------------------------------\n");
                        }
                        
                        /* OFFLINE 이벤트 발생... */
                        dnp->online = 0;
                    }
                        
                }    
            }  
        }

        /* ------------------------------------ */
        /* RS485 통신라인에 대한 재 초기화 ...  */ 
        /* ------------------------------------ */       
        if(oldsec == rtc->sec)  continue;
        oldsec = rtc->sec;
        
        if(++chanReset > CHANNEL_TIMEOUT_TICK)
        {
//COMPORT_INITIAL:            
            chanReset = 0;
            opr->scanComSts[scanid] = 0;

            /* ------------------------------------- */
            /* VME-CHAN 프로토콜 타입 지정시         */
            /* ------------------------------------- */
            if(vmeChanActive == SET)    
            {
                vmeScanChanReset(devid);
                dnp->vmeChan->protocolType = SCAN_DNP;
                dnp->vmeChan->cfgSpeed     = scan->comSpeed;
                dnp->vmeChan->cfgType      = COM_RS232;
                
                vmeScanInitial(scanid, scan->comPort - VME_COM1);
            }
            else
            {
                PortClose((TTY_DESC *) scanPort);  pause(100);
                PortInitial((TTY_DESC *) scanPort); pause(100);
                
                dnp->ttyPort = (TTY_DESC *) scanPort;
            }
            
            if((opr->dnpDebug == devid) || (opr->scanDebug == scanid))
            {   
                Debug(console,"\n===================================================================\n"); 
                Debug(console,"scan02%d> *ALL Device(%d) Comm Fail...Reinitial...offCount=%d !\n", scanid+1, devid+1, scan->offCount);  
                Debug(console,"===================================================================\n"); 
            }
                 
            /* ------------------------------------ */
            /* 해당 채널 : 전체 통신라인 이상시...  */
            /* ------------------------------------ */
            if((opr->dualCpuSts == SET) && (scuCfg->remoteMode == AUTO_MODE))
            {    
                if((scan->changeMode == CHANGE_ALL_DEVICE) && (++chanComFail > 5)&& (opr->cpuChangeTick < CHANGE_CPU_TICK))
                {
                    chanComFail    = 0;
                    
                    opr->cpuChange = SET;
                    logEvent1(opr, sque, EVENT_CHANGE_CPU, CHANGE_ALL_DEVICE, 0, 0, opr->cpuMode); 
                    logHistoryQue1(opr, hque, EVENT_CHANGE_CPU, CHANGE_ALL_DEVICE, 0, 0, opr->cpuMode);   
                    
                    /* -------------------------------- */
                    /* LOG File 저장                    */
                    /* -------------------------------- */
                    sprintf(buffer, "[DNP] 전체장치 통신절체...Device=%d, changeTick=%d ", devid+1, opr->cpuChangeTick);
                    LogFile_MPU (opr, EVENT_CHANGE_CPU, buffer, strlen(buffer));   
                        
                    pause(1000);
                    
                    opr->cpuChangeTick++;

                    if(opr->wdtDebug)
                    {      
                        Debug(console,"\n--------------------------------------------\n");
                        Debug(console,"scan %2d> %02d:%02d:%02d *** DNP Channel Com-Fail... CPU Change (%d) ...!\n", scanid+1, rtc->hour, rtc->min, rtc->sec, opr->cpuChangeTick);
                        Debug(console,"--------------------------------------------\n");
                    }        
                    
                    for(i=0; i< scan->devNum; i++)    
                    {
                        devid = scan->device[i] & MAX_MODULE_MASK;
                        dnp   = (DNP_DEVICE *) dnpcfg[devid];
                        dnpFlagClear(devid);
                        
                        /* OFFLINE 이벤트 발생... */
                        dnp->online = 0;
                    }
                }
            }
        }
        
	}/* while */

    return (0);
}
