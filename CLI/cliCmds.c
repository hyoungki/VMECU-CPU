//==============================================================================
//
// 파일:    cliCmds.c
//
//==============================================================================

//
// 헤더 파일
//
#include    "cli.h"
#include    "external.h"
//#include    "eventmsg.h"


//
// External Variables / Functions
//
extern int GetNumber(int low, int high, int *value, char *message);
extern int GetToken(void);
extern  int CheckEOT(void);
extern  void PrnLine(char mode);

extern  int dispPort(int port);
extern  void dispHostData(int hostid);
extern  void dispDevStatus(int devid, int point);
extern  void dispDevAnalog(int devid, int point);
extern  int  dispDevice(int devid);
extern  int dispLandisHost();

extern int	termExec;
extern char	token[TOKEN_LENGTH];
extern char	command[CMD_BUF_LEN];
extern char	*cmdPtr;
extern SHM_MEMORY	*shmPtr;

//
// Function Type Definition
//
void	CmdClear();
void	CmdHelp(), CmdProcess(), CmdQuit(), CmdExit();
void    CmdSETRTC();

void    CmdHistory(void);           // 시스템 이벤트 
void    CmdEvent(void);             // 상태변화 이벤트
void    CmdOffDebug(void);
void    CmdSimDebug(void);
void    CmdMmiDebug(void);

void    CmdDispMODBUSCfg(void);
void    CmdDispHOSTConfig(void);
void    CmdDispMMIConfig(void);

void    CmdDispDIConfig(void);
void    CmdDispAIConfig(void);
void    CmdDispDOConfig(void);
void    CmdDispAOConfig(void);
void    CmdDispDEVConfig(void);

void    CmdDispRelayCfg(void);
void    CmdDevDebug(void);
void    CmdMessageDebug(void);
void    CmdTRIPControl(void);
void    CmdCLOSEControl(void);

void    CmdTEST_DOM(void);

void    CmdDispDevice(void);
void    CmdSOEDebug(void);
void    CmdWDTDebug(void);
void    CmdHostDebug(void);
void    CmdLineDebug(void);
void    CmdHexDebug(void);
void    CmdDispPOINTConfig(void);

void    CmdDispHostData(void);
void    CmdLINKDebug(void);
void    CmdSETAnalog(void);
void    CmdSETStatus(void);
void    CmdDispRTU(void);
void    CmdDispPORT(void);
void    CmdDispVERSION(void);
void    CmdScuDebug(void);
void    CmdCHANGEcpu(void);

void    CmdSCUTest(void);
void    CmdCOADebug(void);
void    CmdModbusDebug(void);
void    CmdSCANDebug(void);
void    CmdSLAVEDebug(void);
void    CmdDispINTER_LOCK(void);

void    CmdLOC_POLL(void);

void    CmdLOC_TRIPControl(void);
void    CmdLOC_CLOSEControl(void);
void    CmdDispLOCATOR(void);

void    CmdDispLANDIS(void);
void    CmdDispLandisData(void);


void display_ini(void) ; 
void CmdChange(void);

CMD_ENTRY   cmdTable[MAX_COMMAND] = {
    {"    ",        CmdHelp,	        "   ",	"----------------------------"},
	{"help",        CmdHelp,	        "help  <ENTER>",	    "Command-List Display "},
	{"process",	    CmdProcess,	        "process <ENTER>",	    "PROCESSOR Status Display"},
	{"clear",	    CmdClear,	        "clear <ENTER>",	    "Clear EVENT-Queue."},
	{"setrtc",	    CmdSETRTC,          "setrtc <ENTER>",       "SET CU-Time Sync"},	
	{"history",	    CmdHistory,	        "hiistory <ENTER>",     "CU History-Queue Display"},	
	{"event",	    CmdEvent,	        "event <ENTER>",        "CU Event(SOE/Control) Display"},	
	{"off",	        CmdOffDebug,	    "OFF <ENTER>",          "Monitoring Debug Flag - OFF."},	
	{"version",	    CmdDispVERSION,	    "version <ENTER>",      "DISPLAY CU-Version"},	
	{"quit",	    CmdQuit,	        "quit <ENTER>",	        "CONSOLE-TASK quit"},
	{"exit",	    CmdExit,	        "exit <ENTER>",         "***> VME-CU Exit<***"},
    {"    ",        CmdHelp,	        "   ",	"----------------------------"},
    {"hd",	        CmdHostDebug,	    "hd # <ENTER>",	        "HOST Communication Debug ON"}, 
    {"hex",	        CmdHexDebug,	    "hex # <ENTER>",	    "HOST Message Debug ON"}, 
    {"scan",	    CmdSCANDebug,	    "scan # <ENTER>",	    "SCAN Communication Debug ON"},  
    {"dev",	        CmdDevDebug,	    "dev # <ENTER>",	    "Device Communication Debug ON"},  
	{"msg",	        CmdMessageDebug,	"msg <ENTER>",	        "Device Message Debug ON"},  
	{"mod",	        CmdModbusDebug,	    "mod <ENTER>",	        "MODBUS Check Point Debug ON"},  
    {"ld",	        CmdLineDebug,	    "ld # <ENTER>",	        "SCAN Port LINE-Debug ON"}, 
    {"soe",	        CmdSOEDebug,	    "soe <ENTER>",	        "[SOE] Monitoring Debug ON"}, 
	{"wdt",	        CmdWDTDebug,	    "wdt <ENTER>",	        "[WDT] Monitoring Debug ON"}, 
    {"scu",	        CmdScuDebug,	    "scu <ENTER>",	        "[SCU] Monitoring Debug ON"}, 
    {"sim",	        CmdSimDebug,	    "sim <ENTER>",	        "[SIM] Simulator Monitoring Debug ON"}, 
    {"link",	    CmdLINKDebug,	    "link <ENTER>",	        "[LNK] Dual-CPU Monitoring Debug ON"}, 
    {"slave",	    CmdSLAVEDebug,	    "slave <ENTER>",	    "[LNK] SLAVE-Mode Data Debug ON"}, 
	{"mmi",	        CmdMmiDebug,	    "mmi <ENTER>",	        "[MMI] Monitoring Debug ON"}, 
	{"coa",	        CmdCOADebug,	    "coa <ENTER>",	        "COA : Analog COS Debug ON"}, 
    {"    ",        CmdHelp,	        "   ",	"----------------------------"},
    {"host",	    CmdDispHostData,    "host # <ENTER>",	    "HOST Report-Info Display"},   
	{"relay",	    CmdDispRelayCfg,    "relay <ENTER>",	    "DEVICE Configuration Display"}, 
	{"device",	    CmdDispDevice,	    "device # <ENTER>",     "DEVICE Report-Info Display"},	 
	{"modbus",	    CmdDispMODBUSCfg,   "modbus <ENTER>",	    "DEVICE MODBUS-Config Display"},   
	{"rtu",	        CmdDispRTU,         "rtu <ENTER>",	        "HARRIS RTU Config "},   
	{"port",	    CmdDispPORT,        "port # <ENTER>",	    "HARRIS PORT Config "},
	{"locdata",	    CmdDispLOCATOR,     "locdata <ENTER>",	    "LOCATOR-DATA Display "},    
	{"lancfg",	    CmdDispLANDIS,      "lancfg <ENTER>",	    "LANDIS RTU Config "}, 
	{"landata",	    CmdDispLandisData,  "landata<ENTER>",	    "LANDIS HOST Data "}, 
    {"dicfg",	    CmdDispDIConfig,	"dicfg <ENTER>",        "DI-Point Config Display"},	
    {"aicfg",	    CmdDispAIConfig,	"aicfg <ENTER>",        "AI-Point Config Display"},
    {"docfg",	    CmdDispDOConfig,	"docfg <ENTER>",        "DO-Point Config Display"},	
    //{"aocfg",	    CmdDispAOConfig,	"aocfg <ENTER>",        "AO-Point Config Display"},
    {"devcfg",	    CmdDispDEVConfig,	"devcfg <ENTER>",       "DEV-Point Config Display"},
    {"pointcfg",    CmdDispPOINTConfig,	"pointcfg <ENTER>",     "POINT Configuration Display"},
    {"hostcfg",   	CmdDispHOSTConfig,  "hostcfg <ENTER>",    	"HOST Configuration Display."},	
    {"mmicfg",   	CmdDispMMIConfig,   "mmicfg <ENTER>",    	"MMI-Database Display."},	
    {"    ",        CmdHelp,	        "   ",	"----------------------------"},
    {"trip",	    CmdTRIPControl,	    "trip  <ENTER>",        "[TRIP]   Control TEST."},	
    {"close",	    CmdCLOSEControl,	"close <ENTER>",        "[CLOSE]  Control TEST."},	
    {"analog",	    CmdSETAnalog,	    "analog <ENTER>",       "[ANALOG] Test Data Input"},	
    {"status",	    CmdSETStatus,	    "status <ENTER>",       "[STATUS] Test Data Input"},
    {"change",	    CmdCHANGEcpu,	    "change <ENTER>",       "[CHANGE] Test CPU Change"},
    {"testdo",	    CmdTEST_DOM,	    "testdo <ENTER>",       "[TEST-DO] Control TEST."},		
    {"locpoll",	    CmdLOC_POLL,	    "locpoll <ENTER>",      "[TEST] Locator Poll TEST."},
    {"loctrip",	    CmdLOC_TRIPControl,	"loctrip  <ENTER>",     "[TRIP]   Control TEST."},
    {"locclose",    CmdLOC_CLOSEControl,	"locclose  <ENTER>",     "[TRIP]   Control TEST."},			    
    {"    ",        CmdHelp,	        "   ",	"----------------------------"},
    {"dispini",    display_ini,	"dispini  <ENTER>",     "Display config.ini"},	
    {"change",     CmdChange,  "change <ENTER>",   "Change Master/Slave"} ,		  
    {"",        CmdHelp,	        "   ",	"----------------------------"},
    		
};


/*
 * get decimal word
 */
int getDecWord()
{
    int ch;
    int ret_val = 0;

    while((ch = getchar()) != 0x0a)
    {
        //printf("%c", ch);
        if(isdigit(ch))
            ret_val = ret_val * 10 + (ch - '0');
    }
    //printf("\n\r");
    return(ret_val);
}


void CmdChange(void)

{
	 opr->cpuChange = SET;
	 printf("CLI> Set cpuChange to %s\r\n", opr->cpuChange);

}

void display_ini(void)
{
	
display_config_info();	
}

//
// 모듈:	CPU Change()
//
void CmdCHANGEcpu(void)
{
    //int     i,j;
    //int     devid, point;
    //int     index;
    //word    chksum;
    //DNP_DEVICE  *dnp; 
    //SCAN_CONFIG     *scan;
    char    buffer[128];
    
	if (CheckEOT() < 0) return;

#if 0
    if(opr->cpuMode == CPU_MASTER)
    {
        if(opr->mmiDebug)   printf("mmi> Change... A=>B\n");
        linkCfg->changeCPU = CHANGE_M_TO_S;
    }
    else if(opr->cpuMode == CPU_SLAVE)
    {
        if(opr->mmiDebug)   printf("mmi> Change... B=>A\n");
        linkCfg->changeCPU = CHANGE_S_TO_M;    
    }
#endif

    if((opr->dualCpuSts == SET) && (scuCfg->remoteMode == AUTO_MODE))
    {    
        opr->cpuChange = SET;    
        
        logEvent1(opr, sque, EVENT_CHANGE_CPU, 30, 0, 0, opr->cpuMode);     
        logHistoryQue1(opr, hque, EVENT_CHANGE_CPU, 30, 0, 0, opr->cpuMode);     
        printf("test> Change...CPU \n");       
        
        /* -------------------------------- */
        /* LOG File 저장                    */
        /* -------------------------------- */
        sprintf(buffer, "<-- USER> CPU CHANGE cmd ... " );
        LogFile_MPU (opr, EVENT_CHANGE_CPU, buffer, strlen(buffer));  
    }         
}



void CmdSETRTC(void)
{

    int  year, month, day, week, hour, min, sec;

    printf("[*] Clock Setting\n");
    printf("year [2000-2099] = "); year  = getDecWord();
    printf("month[1-12] 	 = "); month = getDecWord();
    printf("day  [1-31] 	 = "); day   = getDecWord();
    printf("hour [0-23] 	 = "); hour  = getDecWord();
    printf("min  [0-59] 	 = "); min   = getDecWord();
    printf("sec  [0-59] 	 = "); sec   = getDecWord();
    printf("week [0 -6] 	 = "); week  = getDecWord();
    printf("correct or not (y/n) ? ");
    if(getchar() == 'y')
    {
        opr->year   = year;
        opr->month  = month;
        opr->day    = day;
        opr->week   = week;
        opr->hour   = hour;
        opr->min    = min;
        opr->sec    = sec;
        
        opr->rtcUpdateFlag = SET;
        opr->vmecuTimeSet   = SET;   // LINK 간 Time Set
        
        printf(">> CU Time SET : %04d/%02d/%02d-%d-%02d:%02d:%02d\n", year, month, day, week, hour, min, sec);	        
        printf("*clock modified\n");
        
    }
    else
    {
        printf("*clock cancel\n");
    }

}


//
// 모듈:	제어모듈 TRIP/CLOSE()
//
void CmdLOC_POLL(void)
{
    int     devid;
    DNP_DEVICE  *dnp; 
    
	if (CheckEOT() < 0) return;

    printf("[*] Test LOCATOR ID .... !\n");
    printf("1. Locator Device# [1..%d] : ", MAX_MODULE);

    devid = getDecWord() & 0x3f;
    
    if((devid < 1) || (devid > MAX_MODULE))
    {
        printf("*** Invalid Locator ID :  %2d \n", devid);
        return;
    }
    
    printf("==> LOCATOR Device-%02d ... USER-Poll ...!\n", devid);
    
    devid = devid - 1;
    dnp = (DNP_DEVICE *) dnpcfg[devid];
    
    dnp->initialLoc = SET;
    dnp->locMode = LOC_ENQ_SEARCH;
    dnp->txCount = 0;
    dnp->rxCount = 0;
    
}


//
// 모듈:	제어모듈 TRIP/CLOSE()
//
void CmdTEST_DOM(void)
{
    int     i,j;
    int     devid, point, tcf;
    char    buffer[128];
    //int     index;
    //word    chksum;
    DNP_DEVICE  *dnp; 
    SCAN_CONFIG     *scan;
    POINT_BUF       *ptBuf;
    
	if (CheckEOT() < 0) return;

    printf("[*] Test DOM Control.... !\n");
    printf("1. Control Device# [1..%d] : ", MAX_MODULE);

    devid = getDecWord() & 0x3f;
    point = 0;    
    tcf   = TRIP_CONTROL;
    
    if((devid < 1) || (devid > MAX_MODULE))
    {
        printf("*** Invalid Test DOM :  %2d \n", devid);
        return;
    }
    
    //printf("==> Control Device-%02d, Point-%02d ... TRIP Control...!\n", devid, point);
    
    devid = devid - 1;

CONTROL_NEXT:        
    /* ---------------------------------------- */
    /* SCAN Task 제어정보 Setting ...           */
    /* ---------------------------------------- */
    printf("\n\n------------------------------------[%4d/%2d/%2d %02d:%02d:%02d]\n", 
       	((rtc->year%100) + 2000), rtc->month, rtc->day, rtc->hour, rtc->min, rtc->sec);
    for(i=0; i< MAX_SCAN_PORT; i++)
    {
        scan = (SCAN_CONFIG *) scanCfg[i];
        if(scan->devNum <= 0)   continue;
        
        for(j=0; j< scan->devNum; j++)
        {
            /* 제어대상 모듈을 search... */
            if(scan->device[j] == devid)
            {
                dnp = (DNP_DEVICE *) dnpcfg[devid];
                ptBuf = (POINT_BUF *) &dnp->doPtBuf[point];
                
                dnp->cntPoint = point;
                dnp->cntTcf   = tcf;   /* TRIP */
                dnp->cntrType = ptBuf->ptConfig;         // 제어속성 : Pulse, Latch 제어
                dnp->selectReq  = SET;
                
                if(ptBuf->dbmax <= 0)   opr->rcvONtime = 500;   
                else                    opr->rcvONtime = ptBuf->dbmax;
                    
                opr->cntrDevId[i]   = devid;
                opr->cntrComTick[i] = 0;
                opr->cntrFlag[i]    = SET;

                if(tcf == TRIP_CONTROL)			
                {
                	logEvent1( opr, sque, EVENT_LOCAL_CNTR, devid, point, TRIP_CONTROL, opr->cpuMode);
                	logHistoryQue1( opr, hque, EVENT_LOCAL_CNTR, devid, point, TRIP_CONTROL, opr->cpuMode);        // 상태 이벤트
                	printf(">> DOM(%d)- TRIP  Control, point=%d / tcf=%2x ...[%4d/%2d/%2d %02d:%02d:%02d] ... %s\n", devid+1, point+1, tcf,
                		((rtc->year%100) + 2000), rtc->month, rtc->day, rtc->hour, rtc->min, rtc->sec, ptBuf->nameStr);
                }
               	else if(tcf == CLOSE_CONTROL)	
               	{
               		logEvent1( opr, sque, EVENT_LOCAL_CNTR, devid, point, CLOSE_CONTROL, opr->cpuMode);
               		logHistoryQue1( opr, hque, EVENT_LOCAL_CNTR, devid, point, CLOSE_CONTROL, opr->cpuMode);        // 상태 이벤트
               		printf(">> DOM(%d)- CLOSE Control, point=%d / tcf=%2x ...[%4d/%2d/%2d %02d:%02d:%02d] ... %s\n", devid+1, point+1, tcf,
                		((rtc->year%100) + 2000), rtc->month, rtc->day, rtc->hour, rtc->min, rtc->sec, ptBuf->nameStr);
               	}
               	
               	/* -------------------------------- */
                /* LOG File 저장                    */
                /* -------------------------------- */
                sprintf(buffer, "<-- USER> Control INFO : Dev=%d, PT=%d, TCF[TRIP=0x80, CLOSE=0x40]= %2x... %s ", devid+1, point+1, tcf, ptBuf->nameStr);
                LogFile_MPU (opr, EVENT_LOCAL_CNTR, buffer, strlen(buffer));    
                	
               	if(tcf == TRIP_CONTROL)
		        {
        			tcf = CLOSE_CONTROL;
		        }
        		else if(tcf == CLOSE_CONTROL)
		        {
        			tcf = TRIP_CONTROL;
        			point = point + 1;
        			
#ifdef  ACE_LBS_4U
        			if(point >= 8)
        			{
        				point = 0;
		        		printf(">> Control DOM(%d) .... Test End...1 !\n", devid+1);
        				//break;
        				//return;
        			}	
#else
        			if(point >= 16)
        			{
        				point = 0;
		        		printf(">> Control DOM(%d) .... Test End...1 !\n", devid+1);
        				//break;
        				//return;
        			}	
#endif        			
        		}
            }                            
        }
      	
    }
    
    pause(1000);
    pause(1000);
    //pause(1000);
        
    goto CONTROL_NEXT;
        
    printf(">> Control DOM(%d) .... Test End...2 !\n", devid+1);
}


/* 
*   전체 쇄정해정 - 해정(TRIP) 제어
*/
void CmdLOC_TRIPControl(void)
{
    int     devid, ptinx;
    
    DNP_DEVICE      *dnp; 
    POINT_BUF       *doPoint;
    
    Debug(console,"**** ALL LOCK Control  : 해정(TRIP) !\n" );   
    
    for(devid =0; devid < MAX_MODULE; devid++)
    {         
        dnp = (DNP_DEVICE *) dnpcfg[devid];

        if(dnp->locControl == 0)    continue;
                                
        for(ptinx=0; ptinx < dnp->devDoPoint; ptinx++)
        {
            doPoint = (POINT_BUF *) &dnp->doPtBuf[ptinx];
            if(doPoint->ptType == DLOCKPT)
            {
                dnp->cntPoint = ptinx;
                dnp->cntTcf   = TRIP_CONTROL;
                                        
                dnp->selectReq  = SET;
                dnp->operateReq = 0;  
                                        
                Debug(console,"==> Device(%2d) Point=%2d... DEV LOCK-Control = %02x... SET\n", devid+1, ptinx+1, TRIP_CONTROL);
                break;
            }          
        }
    }    
}

/* 
*   전체 쇄정해정 - 해정(TRIP) 제어
*/
void CmdLOC_CLOSEControl(void)
{
    int     devid, ptinx;
    
    DNP_DEVICE      *dnp; 
    POINT_BUF       *doPoint;
    
    Debug(console,"**** ALL LOCK Control  : 쇄정(CLOSE) !\n" );   
    
    for(devid =0; devid < MAX_MODULE; devid++)
    {         
        dnp = (DNP_DEVICE *) dnpcfg[devid];

        if(dnp->locControl == 0)    continue;
                                
        for(ptinx=0; ptinx < dnp->devDoPoint; ptinx++)
        {
            doPoint = (POINT_BUF *) &dnp->doPtBuf[ptinx];
            if(doPoint->ptType == DLOCKPT)
            {
                dnp->cntPoint = ptinx;
                dnp->cntTcf   = CLOSE_CONTROL;
                                        
                dnp->selectReq  = SET;
                dnp->operateReq = 0;  
                                        
                Debug(console,"==> Device(%2d) Point=%2d... DEV LOCK-Control = %02x... SET\n", devid+1, ptinx+1, CLOSE_CONTROL);
                break;
            }          
        }
    }    
}


/* 
*   TRIP-제어시험
*/
void CmdTRIPControl(void)
{
    int     i,j;
    int     devid, point;
    //int     index;
    //word    chksum;
    char    buffer[256];
    
    DNP_DEVICE      *dnp; 
    SCAN_CONFIG     *scan;
    POINT_BUF       *ptBuf;
    
	if (CheckEOT() < 0) return;

    printf("[*] Test TRIP Control.... !\n");
    printf("1. Control Device# [1..%d] : ", MAX_MODULE);
    devid = getDecWord() & 0x3f;
    printf("2. Control Point # [1..%d] : ", MAX_DEV_DO_POINT);
    point = getDecWord();    
    
    if((devid < 1) || (devid > MAX_MODULE) || (point < 1) || (point > MAX_DEV_DO_POINT))
    {
        printf("*** Invalid Control... :  %2d %2d\n", devid, point);
        return;
    }
    
    printf("==> Control Device-%02d, Point-%02d ... TRIP Control...[%4d/%2d/%2d %02d:%02d:%02d]\n",  devid, point,
       	((rtc->year%100) + 2000), rtc->month, rtc->day, rtc->hour, rtc->min, rtc->sec);
    
    devid = devid - 1;
    point = point - 1;

    /* ------------------------------------ */
    /*  ACTIVE-CPU로 제어정보 연계          */
    /* ------------------------------------ */
    if(opr->runMode == LOCAL_SLAVE)
    {       
        linkCfg->cntDev   = devid;   
        linkCfg->cntPoint = point;
        linkCfg->cntTCF   = TRIP_CONTROL;
        linkCfg->cntHost  = 0;
        linkCfg->cntSndTick = 0;                                
        linkCfg->rcvONtime = 500;
        linkCfg->rcvOFFtime= 500;
                                        
        linkCfg->cntFlag  = SET;
        
        return;
    }
                                        
//TRIP_NEXT:        
    /* ---------------------------------------- */
    /* SCAN Task 제어정보 Setting ...           */
    /* ---------------------------------------- */
    for(i=0; i< MAX_SCAN_PORT; i++)
    {
    
        scan = (SCAN_CONFIG *) scanCfg[i];
        if(scan->devNum <= 0)   continue;
        
        for(j=0; j< scan->devNum; j++)
        {
            /* 제어대상 모듈을 search... */
            if(scan->device[j] == devid)
            {
                dnp = (DNP_DEVICE *) dnpcfg[devid];
                ptBuf = (POINT_BUF *) &dnp->doPtBuf[point];
                
                dnp->cntPoint = point;
                dnp->cntrType = ptBuf->ptConfig;
                
                if((dnp->type == TYPE_MODBUS_SZ) || (dnp->type == TYPE_MODBUS_LG))
                {
                    dnp->cntTcf   = TRIP_CONTROL;               /* TRIP */        
                    dnp->cntrType = ptBuf->ptConfig;            // 제어속성 : TC(0), Pulse(1), Latch(2) 제어
                    dnp->selectReq  = SET;
                }
                else
                {          
                    dnp->cntTcf   = TRIP_CONTROL;               /* TRIP */        
                    dnp->cntrType = ptBuf->ptConfig;            // 제어속성 : TC(0), Pulse(1), Latch(2) 제어
                    dnp->selectReq  = SET;
#if 0                          
                    if (dnp->cntrType == DNP_AO_CONTROL)
                    {
                        dnp->aoVpoint     = point;     /* point */
                        dnp->aoVpointData = 0xaa;   /* data 0 */
    
                        dnp->anaVOutSelect  = SET;
      	                opr->cntrState 		= RESET;		// 제어단계별 상태 (ARM,ARM-RUN, OPR...)
                    }
                    else
                    {     
                        dnp->cntTcf   = TRIP_CONTROL;               /* TRIP */        
                        dnp->cntrType = ptBuf->ptConfig;            // 제어속성 : TC(0), Pulse(1), Latch(2) 제어
                        dnp->selectReq  = SET;
                    }
#endif
                    
                }
                
                if(ptBuf->dbmax <= 0)   opr->rcvONtime = 500;   
                else                    opr->rcvONtime = ptBuf->dbmax;
                    
                opr->cntrDevId[i]   = devid;
                opr->cntrComTick[i] = 0;
                opr->cntrFlag[i]    = SET;

                logEvent1( opr, sque, EVENT_LOCAL_CNTR, devid, point, TRIP_CONTROL, opr->cpuMode);
                logHistoryQue1( opr, hque, EVENT_LOCAL_CNTR, devid, point, TRIP_CONTROL, opr->cpuMode);        // 상태 이벤트
                
                /* -------------------------------- */
                /* LOG File 저장                    */
                /* -------------------------------- */
                sprintf(buffer, "<-- USER> Control INFO : Dev=%d, PT=%d, TCF[TRIP=0x80, CLOSE=0x40]= %2x...%s ", devid+1, point+1, TRIP_CONTROL, ptBuf->nameStr);
                LogFile_MPU (opr, EVENT_LOCAL_CNTR, buffer, strlen(buffer));    
    
                printf("==> Control-OPR SET : dev=%d, point=%d, TCF=%2x...TRIP Control! ... %s \n", devid+1, dnp->cntPoint+1, dnp->cntTcf, ptBuf->nameStr);
                break;
            }                            
        }
    }
    
}

/* 
*   CLOSE-제어시험
*/
void CmdCLOSEControl(void)
{
    int     i,j;
    int     devid, point;
    //int     index;
    //word    chksum;
    char    buffer[256];
    
    DNP_DEVICE  *dnp; 
    SCAN_CONFIG     *scan;
    POINT_BUF       *ptBuf;
    
	if (CheckEOT() < 0) return;

    printf("[*] Test CLOSE Control.... !\n");
    printf("1. Control Device# [1..%d] : ", MAX_MODULE);
    devid = getDecWord() & 0x3f;
    printf("2. Control Point # [1..%d] : ", MAX_DEV_DO_POINT);
    point = getDecWord();    
    
    if((devid < 1) || (devid > MAX_MODULE) || (point < 1) || (point > MAX_DEV_DO_POINT))
    {
        printf("*** Invalid Control... :  %2d %2d\n", devid, point);
        return;
    }
    
    printf("==> Control Device-%02d, Point-%02d ... CLOSE Control...[%4d/%2d/%2d %02d:%02d:%02d]\n",  devid, point,
       	((rtc->year%100) + 2000), rtc->month, rtc->day, rtc->hour, rtc->min, rtc->sec);
    
    devid = devid - 1;
    point = point - 1;

    /* ------------------------------------ */
    /*  ACTIVE-CPU로 제어정보 연계          */
    /* ------------------------------------ */
    if(opr->runMode == LOCAL_SLAVE)
    {       
        linkCfg->cntDev   = devid;   
        linkCfg->cntPoint = point;
        linkCfg->cntTCF   = CLOSE_CONTROL;
        linkCfg->cntHost  = 0;
        linkCfg->cntSndTick = 0;                                                      
        linkCfg->rcvONtime = 500;
        linkCfg->rcvOFFtime= 500;
                                        
        linkCfg->cntFlag  = SET;
        
        return;
    }
    
//CLOSE_NEXT:        
    /* ---------------------------------------- */
    /* SCAN Task 제어정보 Setting ...           */
    /* ---------------------------------------- */
    for(i=0; i< MAX_SCAN_PORT; i++)
    {
        scan = (SCAN_CONFIG *) scanCfg[i];
        if(scan->devNum <= 0)   continue;
        
        for(j=0; j< scan->devNum; j++)
        {
            /* 제어대상 모듈을 search... */
            if(scan->device[j] == devid)
            {
                dnp = (DNP_DEVICE *) dnpcfg[devid];
                ptBuf = (POINT_BUF *) &dnp->doPtBuf[point];
                
                dnp->cntPoint = point;
                dnp->cntrType = ptBuf->ptConfig;
                
                if((dnp->type == TYPE_MODBUS_SZ) || (dnp->type == TYPE_MODBUS_LG))
                {
                    dnp->cntTcf   = CLOSE_CONTROL;               /* TRIP */        
                    dnp->cntrType = ptBuf->ptConfig;            // 제어속성 : TC(0), Pulse(1), Latch(2) 제어
                    dnp->selectReq  = SET;
                }
                else
                {
                    dnp->cntTcf   = CLOSE_CONTROL;               /* TRIP */        
                    dnp->cntrType = ptBuf->ptConfig;            // 제어속성 : TC(0), Pulse(1), Latch(2) 제어
                    dnp->selectReq  = SET;
                                
#if 0
                    if (dnp->cntrType == DNP_AO_CONTROL)
                    {
                        dnp->aoVpoint     = point;     /* point */
                        dnp->aoVpointData = 0xaa;   /* data 0 */
    
                        dnp->anaVOutSelect  = SET;
      	                opr->cntrState 		= RESET;		// 제어단계별 상태 (ARM,ARM-RUN, OPR...)
                    }
                    else
                    {     
                        dnp->cntTcf   = CLOSE_CONTROL;               /* TRIP */        
                        dnp->cntrType = ptBuf->ptConfig;            // 제어속성 : TC(0), Pulse(1), Latch(2) 제어
                        dnp->selectReq  = SET;
                    }
#endif
                    
                }
                       
                if(ptBuf->dbmax <= 0)   opr->rcvONtime = 500;   
                else                    opr->rcvONtime = ptBuf->dbmax;
                    
                opr->cntrDevId[i]   = devid;
                opr->cntrComTick[i] = 0;
                opr->cntrFlag[i]    = SET;

                logEvent1( opr, sque, EVENT_LOCAL_CNTR, devid, point, CLOSE_CONTROL, opr->cpuMode);
                logHistoryQue1( opr, hque, EVENT_LOCAL_CNTR, devid, point, CLOSE_CONTROL, opr->cpuMode);        // 상태 이벤트
                
                /* -------------------------------- */
                /* LOG File 저장                    */
                /* -------------------------------- */
                sprintf(buffer, "<-- USER> Control INFO : Dev=%d, PT=%d, TCF[TRIP=0x80, CLOSE=0x40]= %2x...%s ", devid+1, point+1, CLOSE_CONTROL, ptBuf->nameStr);
                LogFile_MPU (opr, EVENT_LOCAL_CNTR, buffer, strlen(buffer));    
                
                printf("==> Control-OPR SET : dev=%d, point=%d, TCF=%2x...CLOSE Control! ... %s\n", devid+1, dnp->cntPoint+1, dnp->cntTcf, ptBuf->nameStr);
                break;
            }                            
        }
    }

}

//
// 모듈:	CmdClear()
//
void CmdClear(void)
{
    word    chksum;
    char    buffer[128];
    
	if (CheckEOT() < 0) return;
	
    /* -------------------------------------------- */
    /* 내부 History & Event Queue 내용 Clear...     */
    /* -------------------------------------------- */
    printf("[*] EVENT Queue Cleared.... !\n");
    bzero8248((byte *)sque, sizeof(SYSTEM_QUE));
    chksum = gensum((byte *) sque, sizeof(SYSTEM_QUE) - 2);

    /* CPU 이중화시... 동기 */
    linkCfg->eventRear = sque->front;
    
    sque->chksum = chksum;
    sque->clear  = 255;  
    
    pause(100);
    
    printf("[*] HISTORY Queue Cleared.... !\n");
    bzero8248((byte *) hque, sizeof(HISTORY_QUE));
    chksum = gensum((byte *) hque, sizeof(HISTORY_QUE) - 2);

    /* CPU 이중화시... 동기 */
    linkCfg->historyRear = hque->front;
    
    hque->chksum = chksum;
    hque->clear  = 255;  
    
    /* -------------------------------- */
    /* LOG File 저장                    */
    /* -------------------------------- */
    sprintf(buffer, "<-- USER> *History/Event Clear... ");
    LogFile_MPU (opr, EVENT_LOCAL_CNTR, buffer, strlen(buffer)); 
    
}

//
// 모듈:	CmdQuit()
//
void CmdOffDebug(void)
{
	if (CheckEOT() < 0) return;

    printf("[*] Debug OFF ...!\n");

    opr->hostDebug  = 0xff;
    opr->dnpDebug   = 0xff;
    opr->scanDebug  = 0xff;
    
    //opr->chanDebug  = 0; 
    opr->soeDebug   = 0; 
    opr->mmiDebug   = 0; 
    opr->slaveDebug = 0;
    opr->linkDebug  = 0; 
    opr->wdtDebug   = 0;
    opr->msgDebug   = 0xff; 
    opr->modDebug   = 0xff; 
    opr->hexDebug   = 0xff; 
    opr->simDebug   = 0;   
    opr->coaDebug   = 0;
    opr->testAnalogFLAG = 0;
    
    opr->lineDebug = 0xff;
    opr->scuDebug = 0;
    
}

#if 0
void CmdSETAnalog(void)
{
	//int		vmsid, value;

	// 명령 형식을 점검한다.
	if (CheckEOT() < 0) return;

    opr->testAnalogFLAG = SET;
    
}
#endif

void CmdSETAnalog(void)
{
    //int     i,j;
    int     devid, point, state;
    DNP_DEVICE  *dnp; 
    
	if (CheckEOT() < 0) return;

    printf("[*] Test ANALOG Point Update.... !\n");
    printf("1. Input Device# [1..32] : ");
    devid = getDecWord() & 0x1f;;
    printf("2. Input Point # [1..80] : ");
    point = getDecWord();    
    printf("3. Input Analog# [xxxx] : ");
    state = getDecWord();    
    
    if((devid < 1) || (devid > 32) || (point < 1) || (point > 80))
    {
        printf("*** Invalid ANALOG Information ... :  %2d %2d\n", devid, point);
        return;
    }

    printf("\n----------------------------------------------------->>\n");
    printf("==> Status Info Device-%02d, Point-%02d ... analog = %d !\n", devid, point, state);
    printf("----------------------------------------------------->>\n");
    
    opr->testDevid = devid - 1;
    opr->testPoint = point - 1;
    opr->testState = state;

    /* 시험용 FLAG Clear... */
    dnp = (DNP_DEVICE *) dnpcfg[opr->testDevid];
    opr->testAnalogFLAG = SET;
                        
}

void CmdSETStatus(void)
{
    //int     i,j;
    int     devid, point, state;
    DNP_DEVICE  *dnp; 
    
	if (CheckEOT() < 0) return;

    printf("[*] Test STATUS Point Update.... !\n");
    printf("1. Input Device# [1..%d] : ", MAX_MODULE);
    devid = getDecWord() & 0x3f;
    printf("2. Input Point # [1..80] : ");
    point = getDecWord();    
    printf("3. Input Status# [0...1] : ");
    state = getDecWord() & 0x01;    
    
    if((devid < 1) || (devid > MAX_MODULE) || (point < 1) || (point > MAX_DEV_DI_POINT))
    {
        //printf("*** Invalid Status Information ... :  %2d %2d\n", devid, point);
        return;
    }

    printf("\n----------------------------------------------------->>\n");
    printf("==> Status Info Device-%02d, Point-%02d ... state = %d !\n", devid, point, state);
    printf("----------------------------------------------------->>\n");
    
    opr->testDevid = devid - 1;
    opr->testPoint = point - 1;
    opr->testState = state;

    /* 시험용 FLAG Clear... */
    dnp = (DNP_DEVICE *) dnpcfg[opr->testDevid];
    dnp->initial = STATUS_DUMP_TICK;
    opr->testStatusFLAG = SET;
                        
}

void CmdSCUTest(void)
{
    //int     i,j;
    int     devid;
    //DNP_DEVICE  *dnp; 
    
	if (CheckEOT() < 0) return;

    printf("[*] Test STATUS Point Update.... !\n");
    printf("1. Active Channel [1/2] : ");
    devid = getDecWord() & 0x3f;

    if(devid == 1)
    {
        printf("Change system-A ...!\n");
        opr->testState = 1;
        opr->testFlag  = SET;
    }
    else if(devid == 2)
    {
        printf("Change system-B ...!\n");
        opr->testState = 2;
        opr->testFlag  = SET;
    }
        
}

void CmdDevDebug(void)
{
	int		vmsid;

	// 명령 형식을 점검한다.
	if (GetNumber(0, MAX_MODULE, &vmsid, "DNP Device Comm Debug...?  ") < 0)	return;
	if (CheckEOT() < 0) return;

    if(vmsid == 0)  opr->dnpDebug = MAX_MODULE;
    else            opr->dnpDebug = vmsid - 1;
    
    printf("[*] DNP Device (%2d) Comm Debug... ...!\n", vmsid);
    
}

void CmdSCANDebug(void)
{
	int		vmsid;

	// 명령 형식을 점검한다.
	if (GetNumber(0, MAX_SCAN_PORT, &vmsid, "SCAN Port Comm Debug...?  ") < 0)	return;
	if (CheckEOT() < 0) return;

    opr->scanDebug = vmsid - 1;
    
    printf("[*] SCAN Port (%2d) Comm Debug... ...!\n", vmsid);
    
}

void CmdDispPORT(void)
{
	int		vmsid;

	// 명령 형식을 점검한다.
	if (GetNumber(0, MAX_PORT, &vmsid, "HARRIS PORT Config...?  ") < 0)	vmsid = 0;
	if (CheckEOT() < 0) vmsid = 0;

    dispPort(vmsid);
    
}

void CmdDispLandisData(void)
{
	// 명령 형식을 점검한다.
    dispLandisHost(); 
}

void CmdDispHostData(void)
{
	int		vmsid;
	HOST_DCB        *host;

	// 명령 형식을 점검한다.
	if (GetNumber(1, 5, &vmsid, "HOST DNP Data Display...?  ") < 0)	return;
	if (CheckEOT() < 0) return;

    vmsid = vmsid - 1;
    host = (HOST_DCB *) hostDCB[vmsid];
    
    dispHostData(vmsid);
    
}

void CmdHostDebug(void)
{
	int		vmsid;

	// 명령 형식을 점검한다.
	if (GetNumber(1, MAX_MODULE, &vmsid, "HOST Comm Debug...?  ") < 0)	return;
	if (CheckEOT() < 0) return;

    opr->hostDebug = vmsid - 1;
    printf("[*] HOST (%2d) Comm Debug... ...!\n", vmsid);
}

void CmdLineDebug(void)
{
	int		    vmsid;
	//int         fid;
    //TTY_DESC    *ttyPort;
    
	// 명령 형식을 점검한다.
	if (GetNumber(1, MAX_MODULE, &vmsid, "LINE Comm Debug...?  ") < 0)	return;
	if (CheckEOT() < 0) return;

    opr->lineDebug = vmsid - 1;
    printf("[*] COM-PORT(%2d) Line Debug... ...!\n", vmsid);
        
}

void CmdScuDebug(void)
{
	//int		    vmsid;
	//int         fid;
    //TTY_DESC    *ttyPort;
    
	// 명령 형식을 점검한다.
	//if (GetNumber(1, 32, &vmsid, "LINE Comm Debug...?  ") < 0)	return;
	//if (CheckEOT() < 0) return;

    opr->scuDebug = SET;
    printf("[*] SCU Comm Debug ...!\n");
        
}

void CmdCOADebug(void)
{
	//int		    vmsid;
	//int         fid;
    //TTY_DESC    *ttyPort;
    
	// 명령 형식을 점검한다.
	//if (GetNumber(1, 32, &vmsid, "LINE Comm Debug...?  ") < 0)	return;
	//if (CheckEOT() < 0) return;

    opr->coaDebug = SET;
    printf("[*] COA Comm Debug ...!\n");
        
}

void CmdHexDebug(void)
{
	int		vmsid;

	// 명령 형식을 점검한다.
	if (GetNumber(1, MAX_HOST, &vmsid, "HOST Message Debug...?  ") < 0)	return;
	if (CheckEOT() < 0) return;

    opr->hexDebug = vmsid - 1;
    printf("[*] HOST (%2d) COM-Message Debug... ...!\n", vmsid);
}


/*
*
*/
void CmdSOEDebug(void)
{
	//int		vmsid, value;

	// 명령 형식을 점검한다.
	//if (GetNumber(1, 32, &vmsid, "Display RTU Information...?  ") < 0)	return;
	if (CheckEOT() < 0) return;

    opr->soeDebug = SET;
    printf("[*] SOE Queue ... Message Debug ON...!\n");    
}

/*
*
*/
void CmdLINKDebug(void)
{
	//int		vmsid, value;

	// 명령 형식을 점검한다.
	//if (GetNumber(1, 32, &vmsid, "Display RTU Information...?  ") < 0)	return;
	if (CheckEOT() < 0) return;

    opr->linkDebug = SET;
    printf("[*] CPU Link Status Debug ON...!\n");        
}

/*
*
*/
void CmdWDTDebug(void)
{
	//int		vmsid, value;

	// 명령 형식을 점검한다.
	//if (GetNumber(1, 32, &vmsid, "Display RTU Information...?  ") < 0)	return;
	if (CheckEOT() < 0) return;

    opr->wdtDebug = SET;
    printf("[*] WDT Message Debug ON...!\n");    
}


/*
*   GIPAM2000 - DEVICE 
*/
void CmdDispDevice(void)
{
    char    type;
    int     point;
	int		vmsid;
	int devid;

	// 명령 형식을 점검한다.
	if (GetNumber(1, 32, &vmsid, "DNP Device Data Display...?  ") < 0)	return;
	if (CheckEOT() < 0) return;

    devid = vmsid - 1;
    
    printf("=>Select DEVICE = %d\n", devid +1);
    printf("=>Select TYPE  [0: ALL, 1: STATUS, 2: ANALOG] = ");
    type   = getDecWord();
    //printf(" type = %d\n", type);
    
    printf("=>Select Device (%2d)/Point Number (Max 1024) = ", devid + 1); 
    point = getDecWord() - 1;
    //printf(" point = %d\n", point+1);    
    
    if(point < 0)   type = 0;
        
    /* -------------------------------------------- */
    /*  DISPLAY Type에 따른 계전기 데이터 출력 ...  */
    /* -------------------------------------------- */
    if(type == 1)       dispDevStatus(devid, point);
    else if(type == 2)  dispDevAnalog(devid, point);     
    else                dispDevice(devid);       
            
}

//
// 모듈:	CmdSimDebug()
//
void CmdSimDebug(void)
{
	if (CheckEOT() < 0) return;
    printf("[*] Simulator Debug ON ...!\n");
    opr->simDebug = 1;
    
}

//
// 모듈:	CmdSimDebug()
//
void CmdMmiDebug(void)
{
	if (CheckEOT() < 0) return;
    printf("[*] MMI Debug ON ...!\n");
    opr->mmiDebug = 1;
    
}

//
// 모듈:	CmdSimDebug()
//
void CmdSLAVEDebug(void)
{
	if (CheckEOT() < 0) return;
    printf("[*] SLAVE-Mode Data Debug ON ...!\n");
    opr->slaveDebug = 1;
    
}


//
// 모듈:	CmdMessageDebug()
//
void CmdMessageDebug(void)
{
	int		vmsid;

	// 명령 형식을 점검한다.
	if (GetNumber(1, MAX_MODULE, &vmsid, "DNP Device Comm Debug...?  ") < 0)	return;
	if (CheckEOT() < 0) return;
	
    opr->msgDebug = vmsid - 1;
    printf("[*] DNP Device (%2d) Message Debug... ...!\n", vmsid);
    
}

//
// 모듈:	CmdMessageDebug()
//
void CmdModbusDebug(void)
{
	int		vmsid;

	// 명령 형식을 점검한다.
	if (GetNumber(1, MAX_MODULE, &vmsid, "MODBUS Check Point Debug ON..?  ") < 0)	return;
	if (CheckEOT() < 0) return;
	
    opr->modDebug = vmsid - 1;
    printf("[*] MODBUS Device (%2d) Check Point Debug... ...!\n", vmsid);
    
}

//
// 모듈:	CmdQuit()
//
void CmdQuit(void)
{
	if (CheckEOT() < 0)
		return;

	termExec = 0;
	
	CmdOffDebug();
	
}

//
// 모듈:	CmdExit()
//
void CmdExit(void)
{
    char    buffer[128];
    TASK_INFO	*wdtPtr = NULL;
    
	if (CheckEOT() < 0)
		return;
		
    printf(">> Program Terminate.... Are You Sure...? ");
    if(getchar() == 'y')
    {
        /* 운영자 종료명령 */
        //logMsg(hque, rtc, RESET_BY_USER, 0);  
        logEvent1( opr, sque, EVENT_RESET_BY_USER, 0, 0, 0, opr->cpuMode);
        logHistoryQue1( opr, hque, EVENT_RESET_BY_USER, 0, 0, 0, opr->cpuMode);
        
        opr->userEXIT = SET;
         
        sprintf(buffer, "%s", "----------------------------------------------------");
        LogFile_MPU(opr, EVENT_RESET_BY_USER, buffer, strlen(buffer));
        
        pause(1000);
//        pause(1000);
    
        wdtPtr = (TASK_INFO *) &shmPtr->taskInfo[WDT_PROCESS];
        printf("[*] WDT Process ... pid= %d\n", wdtPtr->pid);
        ExecCommand("kill %d \n", wdtPtr->pid);
			  
			  usleep(100000);
//        pause(1000);
    
	      termExec = 0;
    }
    	    
}

//
// 모듈:	CmdHelp()
//
void CmdHelp(void)
{
	int		index, fIndex, length;
	CMD_ENTRY	*cp;

	if (GetToken() > 0)
	{
		cp = cmdTable;
		for (index = 0, fIndex = 0; index < MAX_COMMAND; index++, cp++)
		{
			if (strlen(cp->name) == 0)
				continue;
			length = strlen(cp->name) < strlen(token) ? strlen(cp->name) :
				strlen(token);
			if (strncmp(cp->name, token, length) != 0)
				continue;
			printf("Command %s:\n", cp->name);
			printf("\tDescription: %s\n", cp->help);
			printf("\tUsage: %s\n", cp->usage);
			fIndex++;
		}
		if (fIndex == 0)
			printf("%%%%WRN_Invalid Command %s\n", token);
		return;
    }

	for (index = 0, cp = cmdTable; index < MAX_COMMAND; index++, cp++)
	{
		if (strlen(cp->name) == 0)
		{
			break;
        }			
		printf("%-10s %s\n", cp->name, cp->help);
    }
}


//
// 모듈:	CmdProcess()
//
void CmdProcess(void)
{
	int		idx;
	TASK_INFO	*ptr;

    //word    millisecond;
    //struct timeval rv; 
    //time_t  tm;
    //struct  tm  *localtm;
    
	// 전체 프로세스의 상태를 표시한다.
	PrnLine('=');
	printf(" Process DEF ACT HST DBG CHK %-11s RST %-8s Last %-6s %-6s\n", 	"WDT-Count", "PID", "MaxET", "PrevET");
	PrnLine('-');
	for (idx = 0, ptr = shmPtr->taskInfo; idx < MAX_PROCESS; idx++, ptr++)
	{
		if (strlen(ptr->name) == 0)
			continue;
		printf(" %-7s %-3d %-3d %-3d %-3d %-3d %4d/%-6d %-3d %-8d %-4d %-6d %-6d\n",
			ptr->name, ptr->define, ptr->active, ptr->hostid, ptr->debug,
			ptr->check, ptr->wdtCount, ptr->wdtEnable, ptr->restart,
			ptr->pid, ptr->lastWdt, ptr->maxEtime / 1000,
			ptr->prevEtime / 1000);
	}
	PrnLine('=');
                    
}

//
// 모듈:	CmdHistory()
//
void CmdEvent(void)
{
    int j;
    int ch;
    int id, ioid, point, state, hostid;
    word    front;
    SYSLOG_FORM   *log;
    DNP_DEVICE  *dnp;
    POINT_BUF   *ptBuf;
        
        
	// 명령 형식을 점검한다.
	if (CheckEOT() < 0)	return;

    printf("\n*** CU-EVENT Data Display ***\n");
    printf(">> CU Controller : %s \n", TARGET_NAME);
// 2025-10-16 오후 3:28:09  흔적을 지우자..
//    printf(">> Manufacture   : ACE-CONTROL System Co.,Ltd\n");
    printf(">> Model Name    : %s\n", VERSION_STRING);
    printf(">> Update Date   : %s\n", DATE_STRING);
    printf(">> Event Number  : %3d ... over =%d, link = %d \n", sque->front, sque->overlab, linkCfg->eventRear );
    printf("\n");

    front = (sque->front - 1) & SYSTEM_QUE_MASK;
    for(j = 0; ;)
    {
        // ??? 뭐냐...
        if((sque->overlab == 0) && (front == SYSTEM_QUE_MASK))    break;
        
        log = (SYSLOG_FORM *) &sque->queue[front];
        
        id      = log->logid;
        ioid    = log->ioid + 1;
        point   = log->point + 1;
        state   = log->state;
        hostid  = log->hostid + 1;

		/* ------------------------------------ */
		/*	이벤트 정보 표출시... 이상정보 Check...		*/
		/* ------------------------------------ */
		if((ioid < 1) || (ioid > MAX_MODULE))			continue;
		if((point < 1) || (point > MAX_DEV_DI_POINT))	continue;	
		if( id > EVENT_HDB_UPDATE)						continue;
				
        if(id == EVENT_SOE)
        {
            dnp     = (DNP_DEVICE *) dnpcfg[ioid-1];
            if(dnp->devType[point-1] == LDI_POINT)          ptBuf   = (POINT_BUF *) &dnp->diPtBuf[point-1];
            else if(dnp->devType[point-1] >= ADEV_POINT)    ptBuf   = (POINT_BUF *) devPtBuf[hostid - 1];
            else                                            ptBuf   = (POINT_BUF *) &dnp->diPtBuf[point-1];
            printf("%3d. %04d/%02d/%02d-%02d:%02d:%02d-%03d [%s : %2d/%3d/%02x/%2d] : %s\n", front + 1,
                log->year + 2000, log->month, log->day, log->hour, log->min, log->sec, log->milisec, errmsg[id], ioid, point, state, hostid, ptBuf->nameStr);
        }
        else if((id == EVENT_MOD_ONLINE) || (id == EVENT_MOD_OFFLINE))
        {
            //ptBuf   = (POINT_BUF *) devPtBuf[ioid];
            dnp     = (DNP_DEVICE *) dnpcfg[ioid-1];
            printf("%3d. %04d/%02d/%02d-%02d:%02d:%02d-%03d [%s : %2d/%3d/%02x/%2d] : %s\n", front + 1,
                log->year + 2000, log->month, log->day, log->hour, log->min, log->sec, log->milisec, errmsg[id], ioid, point, state, hostid, dnp->nameStr);
        }
        else
        {             
            printf("%3d. %04d/%02d/%02d-%02d:%02d:%02d-%03d [%s : %2d/%3d/%02x/%2d] \n", front + 1,
                log->year + 2000, log->month, log->day, log->hour, log->min, log->sec, log->milisec, errmsg[id], ioid, point, state, hostid);
        }
                  
        if(((++j)%32) == 0) 
        {
            ch = getchar();
            if(ch == 'q')   break;     
        }            
        
        /* end 지정 */
        front = (front - 1) & SYSTEM_QUE_MASK;
        
    }
}


//
// 모듈:	CmdEvent()
//
void CmdHistory(void)
{
    int j;
    int ch;
    int id, ioid, point, state, hostid;
    word    front;
    SYSLOG_FORM   *log;
        
        
	// 명령 형식을 점검한다.
	if (CheckEOT() < 0)	return;

    printf("\n*** CU-HISTORY Data Display ***\n");
    printf(">> CU Controller : %s \n", TARGET_NAME);
// 2025-10-16 오후 3:42:34 
//    printf(">> Manufacture   : ACE-CONTROL System Co.,Ltd\n");
    printf(">> Model Name    : %s\n", VERSION_STRING);
    printf(">> Update Date   : %s\n", DATE_STRING);
    printf(">> Event Number  : %3d ... over =%d... link = %d\n", hque->front, hque->overlab, linkCfg->historyRear );
    printf("\n");

    front = (hque->front - 1) & HISTORY_QUE_MASK;
    for(j = 0; ;)
    {
        if((hque->overlab == 0) && (front == HISTORY_QUE_MASK))    break;
        
        log = (SYSLOG_FORM *) &hque->queue[front];
        
        id      = log->logid;
        ioid    = log->ioid + 1;
        point   = log->point + 1;
        state   = log->state;
        hostid  = log->hostid + 1;
// for testing
//        printf("%3d. %04d/%02d/%02d-%02d:%02d:%02d-%03d [%s : %2d/%2d/%02x/%2d] \n", front + 1,
  //              log->year + 2000, log->month, log->day, log->hour, log->min, log->sec, log->milisec, errmsg[id], ioid, point, state, hostid);


        // hkkim
		/* ------------------------------------ */
		/*	이벤트 정보 표출시... 이상정보 Check...		*/
		/* ------------------------------------ */
		if((ioid < 1) || (ioid > MAX_MODULE))			continue;
		if((point < 1) || (point > MAX_DEV_DI_POINT))	continue;	
		
		if( id > EVENT_HDB_UPDATE)						
		{
        printf("%3d. %04d/%02d/%02d-%02d:%02d:%02d-%03d [%s *%d* : %2d/%2d/%02x/%2d] \n", front + 1,
                log->year + 2000, log->month, log->day, log->hour, log->min, log->sec, log->milisec, "logid Error",id , errmsg[id], ioid, point, state, hostid);		  
		}  
        else 

        printf("%3d. %04d/%02d/%02d-%02d:%02d:%02d-%03d [%s : %2d/%2d/%02x/%2d] \n", front + 1,
                log->year + 2000, log->month, log->day, log->hour, log->min, log->sec, log->milisec, errmsg[id], ioid, point, state, hostid);


                
        if(((++j)%32) == 0) 
        {
            ch = getchar();
            if(ch == 'q')   break;     
        }            
        
        /* end 지정 */
        front = (front - 1) & HISTORY_QUE_MASK;
        
    }
}
