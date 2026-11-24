/* 
 *****************************************************************************
 * SYSTEM   : CNA VMECU-V3.0  (철도청 VMECU )
 * FileName : cliDisplay.C  
 * File 내용: 사용자 인터페이스용 운영 메세지들의 다양한 형태의 출력을 지원
 * Designed : SANE-SYSTEM ... by LHS
 *****************************************************************************
*/   

#include	"localLib.h"
#include    "external.h"

extern  int CheckEOT(void);
extern  int getDecWord();

static char modType[20][12]    = { {"NULL      "},{"DIM-32    "}, {"DOM-32    "}, {"AIM-16    "}, {"DIOM-8    "}, 
                                   {"DNP-HIMAP "},{"DNP-GIPAM "}, {"DNP-FRTU  "}, {"DNP-PROPAC"}, {"DNP-VIPAM "},
                                   {"MODBUS(SM)"},{"MODBUS(LG)"}, {"DNP-SIEMEN"}, {"LOCATOR1  "}, {"LOCATOR2  "},{"MPLC-TCPIP"}};
                 
static char devTypeStr[24][8]  = { {"NULL"},{"DIM "}, {"DOM "}, {"AIM "}, {"AOM "}, {"DIOM"},
                                   {"RDI*"},{"RDO*"}, {"RAI*"}, {"RAO*"}, {"ADEV"}, {"DEV*"}, 
                                   {"MCPU"},{"SCPU"}, {"CNTR"}, {"LDI*"}, {"LAI*"}, {"ALOC"}};
                                    
                                    
static char pointTypeStr[16][8]= { {"NULL"},{"COS "}, {"SOE "}, {"ALL "}, {"ACC "}, {"SBO "},
                                   {"CCO "},{"1MA "}, {"20MA"}, {"10V "}, {"ACTF"}, {"LOCK"}, {"DLOC"}};
                                                                
static char mmiPtType[4][8]    = {{"*NULL*"},  {"STATUS "}, {"ANALOG "},{"CONTROL"}};
                                                                
static char portType[4][8]     = {{"NULL"},  {"CAI "}, {"ANA "},{"ACC "}};
//static char protocolType[8][8] = {{"DNP   "},  {"MODBUS"},{"TUNNEL"}, {"BUS   "}, {"*RES#1"}, {"*RES#2"}};
static char protocolType[8][8] = {{"DNP   "}, {"MODBUS"}, {"MPLC  "}, {"BUS   "}, {"LOC1  "}, {"*RES#1"}, {"*RES#2"}, {"*RES#3"}};

static char onoffType[2][8]    = {{"OFF"},   {"ON"}};
static char cpuTypeStr[2][12]  = {{"MASTER-CPU"}, {"SLAVE-CPU"}};
static char setType[2][8]      = {{"RESET"}, {"SET"}};

static char comPortType[20][8] = {{" COM1"}, {" COM2"},{" COM3"}, {" COM4"}, {" COM5"}, {" COM6"}, {" COM7"}, {" COM8"}, {" COM9"},{"COM10"},
                                  {" VME1"}, {" VME2"},{" VME3"}, {" VME4"}, {" VME5"}, {" VME6"}, {" VME7"}, {" VME8"}, {" VME9"},{"VME10"}};
								    
static char modbusType[4][8]   = {{"DIM"}, {"AIM"}, {"DOM"},{"AOM"}};
static char modbusFormat[8][10]= {{"BIT  "}, {"BYTE "}, {"SHORT"},{"WORD "},{"INT  "}, {"UINT "}, {"FLOAT"}};

//static char msg[80]   = "0 0 0 0 0 0 0 0 - 0 0 0 0 0 0 0 0 - 0 0 0 0 0 0 0 0 - 0 0 0 0 0 0 0 0";

static char tcpipMode[4][10]   = {{" NULL "}, {"SERVER"}, {"CLIENT"}};
static char netType[8][8]      = {{"NET#1"}, {"NET#2"},{"NET#3"},{"NET#4"},{"NET#5"},{"NET#6"},{"NET#7"},{"NET#8"}};

static char hostMode[4][10]    = {{" NULL "}, {"SINGLE"}, {" DUAL "}};
static char hostPTType[8][8]   = {{"  NULL "}, {" HARRIS"}, {" LANDIS"}, {"  DNP  "},{" MODBUS"},{"870-101"},{"870-103"}};
static char speedType[9][10]   = {{"1200B"}, {"2400B"}, {"4800B"}, {"9600B"},{"19200"},{"38400"},{"57600"}, {"11.52K"}};
static char comModeType[4][8]  = {{"RS232"},{"MODEM"}, {"RS485"}, {"TCPIP"}};

static char lanType[4][8]      = {{"NULL"},   {"ANA8"},   {"SOE16"},    {"ACC_A16"}};
static char classString[4][10] = {{" NULL "},{"CLASS1"},{"CLASS2"}, {"CLASS3"}};

byte    bit[8] = {0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80};
word    bit16[16] = {0x0001, 0x0002, 0x0004, 0x0008, 0x0010, 0x0020, 0x0040, 0x0080,
                  0x0100, 0x0200, 0x0400, 0x0800, 0x1000, 0x2000, 0x4000, 0x8000};   
                  

//
// 모듈:	CmdHistory()
//
void CmdDispVERSION(void)
{
    //int i, j;
    //int ch;
    //int id, code,para;
    //byte    front, rear;
        
        
	// 명령 형식을 점검한다.
	if (CheckEOT() < 0)	return;

    printf("\n*** Display CU History ***\n");
    printf(" 1. Target Name : [%s] \n", TARGET_NAME);
    printf(" 2. Manufacture : ACE-CONTROL Co.,Ltd\n");
    printf(" 3. Model Name  : %s\n", VERSION_STRING);
    printf(" 4. Simulator   : %s\n", SIMULATOR_NAME);
    printf(" 5. Update Date : %s\n", DATE_STRING);
    printf(" 6. Print  Date : %4d/%2d/%2d %02d:%02d:%02d\n", ((rtc->year%100) + 2000), rtc->month, rtc->day, rtc->hour, rtc->min, rtc->sec);
    printf("[프로그램 Update]------------------------------------\n");
#if 0
    printf("[2013/05/27 - 지멘스 계전기 - 포인트 용량 증설(=> 1024)\n");
    printf("[2013/05/28 - 콘솔에서 계전기 제어시험 - 포인트 증설(Max DO Point)\n");
    printf("[2013/05/29 - GIPAM2000 Analog Point(15~20) Scale .... Delete !\n");
    printf("[2013/06/21 - 이중화 상태/절체 포인트 기능 추가 (MCPU/SCPU/CNTR) !\n");
    printf("[2013/06/25 - ANALOG Point-Value Scaleing (Config) !\n");
    printf("[2013/07/04 - ANALOG 데이터 변환시 => 소수점 보상 !\n");
    printf("[2013/08/27 - 기능모듈부 연계/ History/Event 분리 !\n");
    printf("[2013/09/02 - 이중화CPU 구성시 - 절체회수제한 10회 !\n");
    printf("[2013/09/25 - 이중화CPU 구성시 데이터 동기 !\n");
    printf("[2013/09/28 - 모듈Offline 시 HOST 데이터 표시 !\n");
    printf("[2013/09/30 - 모듈Offline 시 CPU Update 금지 !\n");
    printf("-----------------------------------------------------\n");

    printf("[2013/10/24 - Network 버젼, NET-Simulator 10.01 변경 !\n");
    printf("[2013/11/01 - SOE 생성시 COS 정보 생성 !\n");
    printf("[2013/11/04 - 지멘스 계전기-이중화 Active Flag !\n");
    printf("[2013/11/12 - 지멘스 Time-Sync 1회전송 !\n");
    printf("[2013/11/15 - 지멘스/ICE Time-Sync 데이터변환!\n");
    printf("[2013/11/22 - 포인트 타입(SOE/COS/ALL)별 이벤트 생성\n");
    printf("[2013/11/29 - 비츠로계전기 Ack 시간지연 => 50ms \n");
    printf("[2013/12/06 - MODBUS SOE Debounce Count => HOST Parameter (FUNC1 : 0/1/2/3/4) 지정\n");
    printf("-----------------------------------------------------\n");
    printf("[2014/01/28 - 계전기 DNP-SOE 정보형식 (Qcode=0x17, 0x28) 처리\n");
    printf("[2014/02/12 - MODBUS 아날로그-Float Swep 지정(VIDER-20 = 100)\n");
    printf("[2014/04/14 - MODBUS 통신성공후 Next 메세지 전송(신청주, Miss Opcode Check)\n");
    printf("[2014/06/05 - SOE 이벤트 메세지내 포인트 정보 출력\n");
    printf("[2014/06/13 - 지멘스 계전기 - 제어포인트 Base 변경\n");
 
    printf("-----------------------------------------------------\n");
    printf("[2014/07/02 - VMECU-NET-V4 , 네트워크 HOST 추가\n");
    printf("[2014/07/04 - VMECU-NET-V5 , CPU 이중화 정보연계처리\n");
    printf("[2014/07/11 - CPU 절체모드/상태 이벤트 생성/처리\n");
    printf("[2014/07/12 - Network CONSOLE 기능 추가\n");
    printf("-----------------------------------------------------\n");
    printf("[2014/11/06 - TCPIP 방식, 구로사령 통신시험, SOE연계\n");
    printf("[2014/11/10 - 원흥역사 : DNP계전기 제어방식-Pulse 추가\n");
    printf("[2014/11/19 - 모듈부 Offline 시 : 5초단위 모듈 Reset\n");
    printf("[2014/11/25 - 청라: 이중화 절체이벤트 \n");
    printf("[2014/12/10 - MODBUS 예외처리 발생시- Offline 처리 \n");
    printf("[2014/12/12 - DNP: AI 계측 Update... Max Point Check \n");
    printf("            - DB와 Structure 오류시...DB-Initial \n");
    printf("            - DI SOE Time => System Clock 적용 \n");
    printf("-----------------------------------------------------\n");
    printf("[2015/01/23 - VMECU-NET-V6 Version Update \n");
    printf("            - IEC60870, 지멘스 TCPIP-DNP 기능추가 \n");
    printf("[2015/02/02 - IEC-HOST 통신 Online 이벤트 추가 \n");
    printf("[2015/02/07 - IEC-HOST 통신 Buffer 개별처리 \n");
    printf("            - IEC-HOST CPU LINK Data 처리 \n");
    printf("[2015/02/27 - 슈나이더 계전기 - Control Block 처리 \n");
    printf("[2015/03/11 - VME SIO Access ... 이상시 절체(SIO USE 경우) 처리 \n");
    printf("[2015/03/16 - DNP : Status Dump Check (SOE Debug)... \n");
    printf("[2015/03/19 - MMI : MMI 통신이벤트 제거 (History 참조)... \n");
    printf("[2015/03/20 - CPU 이중화 : SOE 정보생성관련... \n");
    printf("[2015/03/22 - SCAN Debug 기능 추가 (통신 Debug)... \n");
    printf("[2015/03/26 - 예비 HOST 링크시 : SOE 정보 Clear 처리... \n");
    printf("[2015/03/27 - Network Rout-Table Add/Del 처리... \n");
    printf("[2015/03/30 - Analog COS Data 처리... \n");
    printf("[2015/04/02 - CPU 이중화 - HOST Time Sync 처리... \n");
    printf("[2015/04/06 - Slave Mode CPU - 수신데이터 처리 변경(Dummy). \n");
    printf("-----------------------------------------------------\n");
    printf("[2015/05/11 - HOST 관련 Route-Table 처리 (/mnt/routetable-A/B.txt). \n");
    printf("[2015/05/14 - Active-CPU IED통신상태 정보 - 정상처리 \n");
    printf("[2015/05/26 - CPU 이중화-SOE 정보 처리...계전기 SOE 정보연계 \n");
    printf("[2015/08/25 - IIN-Read 시 상태=>0,Slave시 모뎀부 HOST 통신상태 반영 \n");
    printf("[2015/09/08 - SINGLE-CPU 적용시(HOST-Parameter) : LINK, SCU Task 비활성 \n");
    printf("[2015/09/14 - LANDIS HOST 통신 인터페이스 (MFCU와 동일) 이식 \n");
    printf("[2015/09/18 - TCPIP-MODBUS 경우 : 수신 Address Check 하지 않음(LG 계전기)\n");
    printf("[2015/11/04 - TCPIP-MODBUS (신청주) : MODBUS 수신 Opcode 오류시... pause & continue\n");
    printf("[2015/12/22 - LANDIS 통신처리 및 Host Point#1 오정보 처리\n");
     
    printf("-----------------------------------------------------\n");
    printf("[2016/02/11 - 영종 ESS 모듈 : Modbus (0x06) 추가\n");
    printf("[2016/03/03 - 영종-신규 HOST(비츠로) : UNSOLICIT Enb/Disb Response 추가\n");
    printf("[2016/03/05 - 영종-신규 HOST(비츠로) : UNSOLICIT SOE Response 추가\n");
    printf("-----------------------------------------------------\n");
    printf("[2016/05/17 - TCPIP-HOST : Accept() 시도 30회 실패시... Reboot 추가\n");
    printf("[2016/05/29 - 진상역 대상 : SCAN-PORT#6 추가(SCAN#5 Copy) \n");
    printf("[2016/06/22 - 호남선 : HOST 별 PROCESS 로 구조변경,  Server 접속방식 -> NON-BLOCK \n");
    printf("[2016/08/31 - DNP Device : Fault-RESET 제어를 위한 AO Control (config == 8) \n");
    printf("[2016/09/28 - 수도권 FRTU - SOE 수신시 ACK 지연응답 (비츠로와 동일, Port번호 ms) \n");
    printf("[2016/11/26 - MODBUS 제어시-Write Block 참조(포인트-MMI 정보) \n");
    printf("[2016/11/28 - ICE 계전기 - Fault Reset (SBO 제어)  (포인트 Config = 11) \n");

    printf("-----------------------------------------------------\n");
    printf("[2016/12/09 - VMECU/LBS-RTU(6U/4U) 프로그램 통합 \n");
    printf("[2016/12/10 - 수도권 CU - DNP HOST 제어시- 미등록 제어포인트 \n");
    printf("-----------------------------------------------------\n");
    printf("[2016/12/20 - ACE-VMECU-NET-V8 프로그램 Setup \n");
    printf("[2016/12/21 - Event-File LOG = /mnt/vmecu-A/B \n");
    printf("[2016/12/22 - 전체 계전기 통신상태 => DB에 정의된 계전기 대상\n");
    printf("[2016/12/26 - CPU 링크시, 반전 포인트 처리, 장치포인트 상태 (online) \n");
    printf("[2016/12/27 - LBSRTU: Slave 모드시 모듈리셋부 제외 \n");
    printf("-----------------------------------------------------\n");
    printf("[2017/05/25 - 지멘스, LG 계전기-DNP Control-Flag 지정 (Pulse, Latch, Trip/Close) \n");
    printf("[2017/06/08 - MMI 상태포인트 표출 : 패킷당 255 포인트, 포인트 수만큼 분할 전송 \n");
    printf("[2017/06/28 - 전철CU : 계전기 TCPIP 통신시 Slave 전환시... 소켓 Close 추가 \n");
    printf("-----------------------------------------------------\n");
    printf("[2017/08/20 - ACE-VMECU-NET-V10 프로그램 Update (시뮬레이터 V12) \n");
    printf("            - LOCATOR 계전기 Thread 추가 \n");
    printf("            - 미쯔비시 계전기 - (쇄정/해정)제어방식별 제어로직 구현 \n");
    printf("[2017/08/22 - LANDIS HOST Update  \n");
    printf("[2017/08/25 - HARRIS : SOE 포인트 발생시... COS 자동생성...  \n");
    printf("[2017/08/28 - 고장점-Modbus : 동일 Address에대한 가상계전기 처리...  \n");
#endif          
    printf("-----------------------------------------------------\n");
    printf("[2017/09/04 - ACE-VMECU-NET-V11 프로그램 Update (시뮬레이터 V12.0.3) \n");
    printf("            - 미쯔비시 계전기 - 통신 프로토콜 적용 \n");
    printf("            - LANDIS/HARRIS : MMI HOST 통신상태 반영 \n");
    printf("[2017/09/09 - 고장점(쯔다) 데이터 처리 Update \n");
    printf("-----------------------------------------------------\n");
    printf("[2017/09/22 - 미쯔비시 계전기 통신 Update \n");
    printf("            - 계전기 전체/쇄정/해정 Logic \n");
    printf("-----------------------------------------------------\n");
    printf("[2018/03/02 - 지멘스 계전기 - TCPIP-DNP 통신 : Sequence mismatch...\n");
    printf("            - 10회이상 mismatch 발생시...LINK-RESET 수행\n");
    printf("[2018/03/15 - 지멘스 계전기 - UNSOLITE 메세지에 대한 응답처리 추가...\n");
    printf("[2018/03/30 - 모란역: 고장점(쯔다) 데이터 처리 Update... Type#1/Type#2 처리 \n");
    printf("[2018/05/17 - 모란역: 고장점(쯔다) Poll 수행시.. comFailMin 값 Clear (Offline 이벤트 방지)\n");
    printf("[           - 고장점 RESET 상태포인트 추가 (point=1)\n");
    printf("[2018/07/03 - 모란역: 고장점(쯔다) - No Poll, 아날로그 현시시 상태 SET \n");
    printf("[2018/08/21 - 계전기 통신이상-CPU 절체 시간 단축 (10회 => 2회) \n");
    printf("[2018/12/06 - HOST 통신상태 LED - SLAVE 통신정상시 미반영=>반영...\n");
    printf("-----------------------------------------------------\n");
    printf("[2019/01/24 - LANDIS-HOST : 계전기 통신상태 정보 Update 미반영=>반영...\n");
    printf("[2019.03.05 - 비츠로OLD: DNP-Analog Dump (Var==0) 인경우... Specail Function 값 지정\n");
    printf("[2019.09.02 - PNC 계전기 : 16bit Analog Dump -->(SHORT INT) 형 적용\n");
    printf("[2019.11.22 - 광명부하계폐기 : DNP통신, RESET-CLOSE Config(5) 추가\n");
    printf("-----------------------------------------------------\n");
    printf("[2020/04/09 - LINK-SOE 입력시... HOST 상태 Update 후 Reject \n");
    printf("[2020/06/05 - DNP 통신시 IIN-이상 BIT 상태 무시 ...계전기 AO-Point 지정 \n");
    printf("[2020/06/18 - 전체통신이상 이벤트 COS 처리, SOE/COS 발생 => 포인트 타입 ALL 지정 \n");
    printf("[2020/11/24 - CONSOLE: ANALOG TEST 기능 추가 - 포인트 지정 \n");
    printf("-----------------------------------------------------\n");
    printf("[2021/10/20 - 계전기 통신이상 이벤트 ... SOE(기본)/COS(ALLPT) 형식 Report \n");
    printf("[2021/11/26 - HOST 통신지연 => 계전기 초기화 이후, 10초 \n");
    printf("[2021/12/02 - 디바이스 포인트 SOE 상태정보 Update \n");
    printf("[2021/12/06 - 디바이스 포인트 INDEX Update / SOE 확인 \n");
    printf("-----------------------------------------------------\n");
    printf("[2023/01/02 - (지멘스)계전기(MAX 32개) 포인트 구성: DI(1024), DO(1024), AI(256) 용량 \n");
    printf("-----------------------------------------------------\n");
    printf("[2025/01/16 - (TCPIP-Device) OFFLINE 지연: Net-Initial() 3회 이상시 ...Offline \n");
    printf("[2025/02/05 - (HOST-DNP) MODBUS 계전기 제어시, Direct 제어정보 연계...  \n");
    printf("-----------------------------------------------------\n");
    printf("[2026/07/07 - (전체계전기 통신) HOST 별 전체통신상태 처리...ALL Offline Check\n");
    printf("\n");

}



void    CmdDispLOCATOR()
{
    int     i, devid;
    DNP_DEVICE  *dnp;
    POINT_BUF *diPoint;
    POINT_BUF *aiPoint;

    printf("\n[*] LOCATOR Device No = ");
    devid = getDecWord();
    
    if(devid > 0)   devid = devid - 1;
    else            
    {
        return;
    }
         
    dnp = (DNP_DEVICE *) dnpcfg[devid];
        
    if((dnp->type != TYPE_LOCATOR1) && (dnp->type != TYPE_LOCATOR2))  return;

    printf(" [%02d] LOCATOR --------------------------------------------[EVENT = %d]\n", devid+1, dnp->locEvent);
        
    printf(" [ANALOG]     NAME            HOST   LOC  CommF  OVR  LIMIT  EVENT \n");
    for(i=0; i< dnp->devAiPoint; i++)
    {
        aiPoint = (POINT_BUF *) &dnp->aiPtBuf[i];
        if(aiPoint->config == 0) continue;
        printf("[%02d] %20s    %4d   %4d   %d     %d    %d    %d\n", i+1, aiPoint->nameStr, aiPoint->pointData,  
            aiPoint->locData, aiPoint->pointComFail, aiPoint->pointOver, aiPoint->limitEvent, aiPoint->locEvent);         
    }
        
    printf("------------------------------------------------------------\n");
        
    printf(" [STATUS]     NAME            Status\n");
    for(i=0; i< dnp->devDiPoint; i++)
    {
        diPoint = (POINT_BUF *) &dnp->diPtBuf[i];
        if(diPoint->config == 0) continue;
        printf("[%02d] %20s    %d\n", i+1, diPoint->nameStr, diPoint->status);       
    }
        
    printf("\n\n");    
    
}

/*----------------------------------------------------------------------------
* Function Name : dispLandisConfig()
* 수행내용: CCU내의 모듈구성정보를 Console로 출력하는 함수
* ArgList :
* Return  :  
---------------------------------------------------------------------------- */  
void    CmdDispLANDIS()
{
    int     i,j,type;
    
    printf("[*] CHASSIS Number ... [%2d] \n", opr->chassisNum);
    
    for(i=0; i< opr->chassisNum; i++)
    {
        for(j=0; j< 16; j++)
        {
            type = rtudb->chassis[i][j];
            printf(" [%2d] Chassis [%2d] Card - [ %s ] \n", i, j, lanType[type & 0x03]);
                    
        }
        printf("... type any key ? \n");
        getchar();
    }
    
    printf("... Configuration end ! \n");


}


/*----------------------------------------------------------------------------
* Function Name : CmdDispRTU()
* 수행내용: RTU 운영버퍼의 내용을 Console로 출력하는 함수
* ArgList :
* Return  :  
---------------------------------------------------------------------------- */  
void CmdDispRTU()
{
    //short   index;
    short   i;
    short   port;
    short	hostid;
    
    RTU     *rtu;
	HOST_DCB	*host;
	
    /* ------------------------------------ */
    /*  HOST MASTER Channel RTU ...         */
    /* ------------------------------------ */
    for(hostid = 0; hostid < MAX_HOST; hostid++)
    {
        host = (HOST_DCB *) hostDCB[hostid];                 /* 주장치 #1 속성정의 */
    
    	printf("\n-----------------------------------\n");    
	    for(i = 0; i < MAX_RTU; i++)
    	{
        	rtu = (RTU *) &host->rtubuf[i];
	        if(rtu->id == 0) continue;

    	    printf(" RTU_%d Feature\n", i + 1);
        	printf("-> rtu-address   : %d  [COS = %d,%d]\n",
            	rtu->id, rtu->cosq.front, rtu->cosq.rear);
	        printf("-> SOE-Queue     : %d\n", rtu->soeq.front);
    	    printf("-> c&i   port    : %d\n", rtu->ciPortCnt);
        	printf("-> data  port    : %d\n", rtu->dataPortCnt);
	        printf("  [port status]  : ");

    	    for(port = 0; port < MAX_PORT; port++)
        	{
            	if(host->portdb[port].rtuid != (i + 1)) continue;
            	printf("%02x ", host->portdb[port].portStatus);
        	}
        	printf("\n");
    	}
    }
    
    printf("\n\n");
    
}

/*----------------------------------------------------------------------------
* Function Name : dispLandisHost()
* Return  :  
---------------------------------------------------------------------------- */  
int dispLandisHost()
{
    int     i, j;
    int     type, point;
    int     ptStart, ptStop;
    char    ch;
    //word    *buf;
    byte    msg[128];
    byte    *bfptr;
    HOST_STSBUF *diPoint;
    HOST_ANABUF *aiPoint;
    
    printf("\nLANDIS HOST Point Configure ...\n");

    printf("\nSelect TYPE [S:Status, A:Analog] = ");
    ch = getchar();
    if((ch == 's' ) || (ch == 'S')) type = 1;       /* Status Display */
    else                            type = 2;       /* Analog Display */
    
    ch = getchar();
    
    printf("\nStart Point = ");
    point = getDecWord();
    
    if(point > 0)   point = point - 1;
    else            point = 0;
    
    if ((point < 0) || (point > MAX_HOST_STS_PT))
    {
        printf("*invalid Point ...%d\n", point);
        return 0;
    }
    
    if(type == 1)   /* Status Display */
    {
        if(point >= (MAX_HOST_STS_PT - 16))     point = MAX_HOST_STS_PT - 16;
    }
    else
    {
        if(point >= (MAX_HOST_ANA_PT - 10))     point = MAX_HOST_ANA_PT - 10;
    }
    
    while(ch != 'q')
    {    
        if(type == 1)
        {
            /* Status Bit Display ... */
            ptStart = point;
            ptStop  = ptStart + 16;
            printf("STS%3d: ", point + 1);
            for(i = ptStart, j=0, bfptr = msg; i < ptStop; i++, j++)
            {
                diPoint = (HOST_STSBUF *) hostStsBuf[i];
                *bfptr++ = ' ';
                *bfptr++ = '0' + diPoint->status;      

                if((j & 0xf) == 0xf)
                {
                    *bfptr = '\0';
                    printf(" %s\n", msg);
                    bfptr = msg;
                }
                else if((j & 7) == 7)
                {
                    *bfptr = ' ';
                    bfptr++;
                    *bfptr = '-';
                    bfptr++;
                }
            }                
        } 
        else
        {
            /* Status Bit Display ... */
            ptStart = point;
            ptStop  = ptStart + 5;
    
            printf("ANA%3d: ", point+ 1);
            for(i = ptStart,j=1; i < ptStop; i++, j++)
            {
                aiPoint = (HOST_ANABUF *) hostAnaBuf[i];
                printf("%04d[%3x] ", aiPoint->pointData, aiPoint->convData);
            }            
            printf("\n");        
        }
        
        //printf("\n");        
        
        ch = getchar();
    }

    printf("\n");   
    
    return 0; 
}


/*----------------------------------------------------------------------------
* Function Name : dispRTU()
* 수행내용: PORT 구성 및 PORTDB 운영버퍼의 내용을 Console로 출력하는 함수
* ArgList :
*   1. port - 출력하고자 하는 Port 번호
* Return  :  
---------------------------------------------------------------------------- */  
void dispPort(int port)
{
    short   i;
    //short   index, point;
    short   type;
    char    ch=0;
    word    *buf;
    byte    msg[128];
    byte    *bfptr, *bfptr1;
	
	PORT_DB	*portdb;
	
    /* -------------------------------- */
    /* PORT Configuration 정보          */
    /* -------------------------------- */
    if(port == 0)
    {
        printf("\n Port Status DISPLAY\n");
        printf("         [MASTER]    [SLAVE] \n");
        printf("  No.   RTU  TYPE | RTU  TYPE \n");
        for(i = 0; i < MAX_PORT; i++)
        {
            bfptr  = (byte *) &rtudb->portdb1[i*2];
            bfptr1 = (byte *) &rtudb->portdb2[i*2];
            
            printf(" [%02d]   %02d   %02d  || %02d   %02d  \n",
                i + 1,  bfptr[0],  bfptr[1], bfptr1[0],  bfptr1[1]);
        }
        return;
    }
    else if(port > MAX_PORT)
    {
        printf("\n ... invalid Port Number[16] = %d \n", port);    
        return;
    }
    
#if 1
    /* -------------------------------- */
    /* display HOST-HARRIS PORT Data... */
    /* -------------------------------- */
    port--;
    portdb = (PORT_DB *) &hostDCB[0]->portdb[port];
    
    type = portdb->type;

    printf(" MASTER Port [%d] Status\n", port+1);
    printf("-> type        : %s\n", portType[type]);
    printf("-> rtuid       : %d\n", portdb->rtuid);
    printf("-> harris_Port : %d\n", portdb->harrisPort);
    printf("-> status      : %d\n", portdb->portStatus);    

    while(ch != 'q')
    {
        printf("  [point data]\n");
        buf = (word *) &portdb->pointData[0];
        if((type == ANA) || (type == ACC))
        {
            buf = (word *) &portdb->pointData[0];
            for(i = 0; i < 64; i++)
            {
                printf("%03x ", buf[i]);
                if((i & 15) == 15) printf("\n");
            }
        }
        else if(type == CAI)
        {
            for(i = 0, bfptr = msg; i < 64; i++)
            {
                *bfptr++ = ' ';
                *bfptr++ = '0' + buf[i];      /* update 95.7.24 */

                if((i & 0x1f) == 0x1f)
                {
                    *bfptr = '\0';
                    printf(" %s\n", msg);
                    bfptr = msg;
                }
                else if((i & 7) == 7)
                {
                    *bfptr = ' ';
                    bfptr++;
                    *bfptr = '-';
                    bfptr++;
                }
            } /* for */
        }
        
        ch = getchar();
    }
#endif

}

/*----------------------------------------------------------------------------
* Function Name : dispDevice()
* 수행내용: 각 계전기 별 수집된 DI/AI/AO 정보를 표출함
* ArgList :
*   1. port - 출력하고자 하는 계전기 번호
* Return  :  
---------------------------------------------------------------------------- */  
int dispDevice(int devid)
{
    int     i, j;
    int     point;
    int     ptStart, ptStop;
    char    ch;
    byte    msg[128];
    byte    *bfptr;
    
    DNP_DEVICE  *dnp;
    POINT_BUF *diPoint;
    POINT_BUF *aiPoint;
    
    /*  check Invalid DEVICE # */
    if((devid < 0) || (devid >= MAX_MODULE))
    {
        printf("*Invalid devid (%d) ...!\n", devid);
        return (0);
    }
    
    ch = 0;
    while(ch != 'q')
    {
        dnp = (DNP_DEVICE *) dnpcfg[devid];

        printf("\n[DEVICE Information] ------------------------[%4d/%2d/%2d %02d:%02d:%02d]\n", 
        	((rtc->year%100) + 2000), rtc->month, rtc->day, rtc->hour, rtc->min, rtc->sec);
        printf(" Device Address   : %2d\n", dnp->ioid);
        printf(" [*] Device TYPE  : [%s]\n", modType[dnp->type]);
        printf(" [*] Point Config : [DI/DO/AI/Count] - %2d %2d %2d %2d \n", dnp->devDiPoint, dnp->devDoPoint,dnp->devAiPoint, dnp->devAoPoint);
 
 		/* ------------------------ */ 
        /* Status Bit Display ... 	*/
        /* ------------------------ */ 
        ptStart = 0;
        ptStop  = dnp->devDiPoint;
        point   = (ptStop - ptStart) % 32;
        printf("=> DI Data : [%d - %d] \n", ptStart, ptStop);
    
        for(i = ptStart, j=0, bfptr = msg; i < ptStop; i++, j++)
        {
            diPoint = (POINT_BUF *) &dnp->diPtBuf[i];

            if(j > 9)  
            {
                j = 0;
                printf("\n");
            }         
            
            if(dnp->online) printf("[%3d] %2x  ", i+1, diPoint->status);
            else            printf("[%3d] %2x* ", i+1, diPoint->status);
                
        } /* for */
    
        printf("\n");
        
        /* ------------------------ */               
        /* ANALOG Input Display ... */
        /* ------------------------ */ 
        ptStart = 0;
        ptStop  = dnp->devAiPoint;
        printf("=> AI Data : [%d - %d] \n", ptStart, ptStop);
        for(i = ptStart,j=1; i < ptStop; i++, j++)
        {
            aiPoint = (POINT_BUF *) &dnp->aiPtBuf[i];
            printf("[%3d] %4.1f ", i+1, aiPoint->floatData);
                
            if(j== 10) 
            {
                j = 0;
                printf("\n");
            }            
        }
        printf("\n");        
        
        ch = getchar();
    }

    printf("\n"); 
    return (0);   
}


/*----------------------------------------------------------------------------
* Function Name : dispDevStatus()
* 수행내용: 각 계전기 별 수집된 DI 정보를 표출함
* ArgList :
*   1. port - 출력하고자 하는 계전기 번호
* Return  :  
---------------------------------------------------------------------------- */  
void dispDevStatus(int devid, int point)
{
    int     i, j;
    int     ptStart, ptStop;
    char    ch=0;
    byte    msg[128];
    byte    *bfptr;
    
    DNP_DEVICE  *dnp;
    POINT_BUF *diPoint;
    //POINT_BUF *aiPoint;
    
    printf("[DEVICE-%02d / Status POINT-%02d]\n", devid+1, point + 1);

    dnp = (DNP_DEVICE *) dnpcfg[devid];
    printf("\n[DEVICE Information] ---------------------------[%4d/%2d/%2d %02d:%02d:%02d]\n", 
        	((rtc->year%100) + 2000), rtc->month, rtc->day, rtc->hour, rtc->min, rtc->sec);   
    printf(" Device Address   : %2d\n", dnp->ioid);
    printf(" [*] Device TYPE  : [%s]\n", modType[dnp->type]);
    printf(" [*] Point Config : [DI/DO/AI/Count] - %2d %2d %2d %2d \n", dnp->devDiPoint, dnp->devDoPoint,dnp->devAiPoint, dnp->devAoPoint);
    printf("--------------------------------------------------------------------\n");    

    while(ch != 'q')
    {
        /* ------------------------ */ 
        /* Status Bit Display ... 	*/
        /* ------------------------ */ 
        ptStart = point;
        ptStop  = point + 8;
        printf("=> STATUS Data [%3d - %3d] : ", ptStart+1, ptStop);
    
        for(i = ptStart, j=0, bfptr = msg; i < ptStop; i++, j++)
        {
            diPoint = (POINT_BUF *) &dnp->diPtBuf[i];

            if(j > 9)  
            {
                j = 0;
                printf("\n");
            }         
            
            if(dnp->online) printf("[%3d] %2x  ", i+1, diPoint->status);
            else            printf("[%3d] %2x* ", i+1, diPoint->status);
                            
        } /* for */
        
        printf("\n");
        
        ch = getchar();
    }
 
    printf("\n\n");
}

/*----------------------------------------------------------------------------
* Function Name : dispDevStatus()
* 수행내용: 각 계전기 별 수집된 DI 정보를 표출함
* ArgList :
*   1. port - 출력하고자 하는 계전기 번호
* Return  :  
---------------------------------------------------------------------------- */  
void dispDevAnalog(int devid, int point)
{
    int     i, j;
    int     ptStart, ptStop;
    char    ch=0;
    //byte    msg[128];
    //byte    *bfptr;
    
    DNP_DEVICE  *dnp;
    //POINT_BUF *diPoint;
    POINT_BUF *aiPoint;
    
    printf("[DEVICE-%02d / Analog POINT-%02d]\n", devid+1, point + 1);

    dnp = (DNP_DEVICE *) dnpcfg[devid];
    printf("\n[DEVICE Information] ---------------------------[%4d/%2d/%2d %02d:%02d:%02d]\n", 
        	((rtc->year%100) + 2000), rtc->month, rtc->day, rtc->hour, rtc->min, rtc->sec);    
    printf(" Device Address   : %2d\n", dnp->ioid);
    printf(" [*] Device TYPE  : [%s]\n", modType[dnp->type]);
    printf(" [*] Point Config : [DI/DO/AI/Count] - %2d %2d %2d %2d \n", dnp->devDiPoint, dnp->devDoPoint,dnp->devAiPoint, dnp->devAoPoint);
    printf("--------------------------------------------------------------------\n");    

    while(ch != 'q')
    {
        
        /* ------------------------ */               
        /* ANALOG Input Display ... */
        /* ------------------------ */ 
        ptStart = point;
        ptStop  = point + 8;
        printf("=> ANALOG Data [%3d - %3d] : ", ptStart+1, ptStop);
        
        for(i = ptStart,j=0; i < ptStop; i++, j++)
        {
            aiPoint = (POINT_BUF *) &dnp->aiPtBuf[i];
            printf("[%3d] %04.1f ", i+1, aiPoint->floatData);
                
            if(j== 10) 
            {
                j = 0;
                printf("\n");
            }
            
        }
        
        ch = getchar();
    }
 
    printf("\n\n");
}

/*----------------------------------------------------------------------------
* Function Name : dispDevice()
* 수행내용: 각 계전기 별 수집된 DI/AI/AO 정보를 표출함
* ArgList :
*   1. port - 출력하고자 하는 계전기 번호
* Return  :  
---------------------------------------------------------------------------- */  
void dispHostData(int hostid)
{
    int     i, j;
    int     point, online;
    int     ptStart, ptStop;
    byte    state;
    char    ch;
    byte    msg[128];
    byte    *bfptr, temp[4];
    float   *fptr;
    
    HOST_DCB        *host;
    DNP_ANA_INPUT   *anaPoint;
    //DIPOINT_INFO    *hostSts;
    DNP_SOE_QUEUE   *dnpSOEQ;   
    
    ch = 0;
    while(ch != 'q')
    {
        host = (HOST_DCB *) hostDCB[hostid];
        
        dnpSOEQ = (DNP_SOE_QUEUE *) &host->dnpSOEQ;
        
        printf("\n[HOST%2d Information] ------------------------[%4d/%2d/%2d %02d:%02d:%02d]\n", 
        	hostid +1, ((rtc->year%100) + 2000), rtc->month, rtc->day, rtc->hour, rtc->min, rtc->sec);
        printf("[*] HOST RUN Mode = [%s]\n", hostMode[host->hostDualMode]);            /* 0:이중화모드, 1:개별모드 */
        printf("[*] HOST Protocol = [%s]\n", hostPTType[host->hostProtocol]);          /* 0:미정의, 1:HARRIS, 2: LANDIS, 3: DNP */
        printf("[*] HOST Address  = HOST  [%d]/CU   [%d]\n", host->hostid, host->rtuAddr);        /* RTU Address */
        printf("[*] HOST Word Rpt = [DI=%d, AI=%d]\n", host->diIndexWord, host->aiIndexWord);
        printf("[*] HOST Status   = %2x\n", host->hostStatus);  
        printf("[*] HOST SOE-Queue= front=%2d/rear=%2d \n", dnpSOEQ->front, dnpSOEQ->rear);
        printf("[*] PT [DI/DO/AI/AO]= %2d/%2d/%2d/%2d\n\n", host->diPtNum,host->doPtNum,host->aiPtNum, host->aoPtNum);
    
        /* Status Bit Display ... */
        ptStart = 0;
        ptStop  = host->diPtNum;
        point   = (ptStop - ptStart) % 32;
        
        printf("=> DI Data : [%d - %d] \n", ptStart, ptStop);
    
        for(i = ptStart, j=0, bfptr = msg; i < ptStop; i++, j++)
        {
            /* -------------------------------- */
            /* HOST Type에 따른 데이터 처리     */
            /* -------------------------------- */   
            state = host->sts_pointData[i] & PT_FLAG_STS_ON;
            online = host->sts_pointData[i] & PT_FLAG_ONLINE;
            
            if(j > 9)  
            {
                j = 0;
                printf("\n");
            }         
            
            if(online)  printf("[%3d] %2x  ", i+1, host->sts_pointData[i]);
            else        printf("[%3d] %2x* ", i+1, host->sts_pointData[i]);
            
        } /* for */

        printf("\n");   
        
        /* ------------------------ */               
        /* ANALOG Input Display ... */
        /* ------------------------ */ 
        ptStart = 0;
        ptStop  = host->aiPtNum;
        printf("\n\n=> AI Data : [%d - %d] \n", ptStart, ptStop);
        for(i = ptStart,j=1; i < ptStop; i++, j++)
        {
            anaPoint = (DNP_ANA_INPUT *) &hostDCB[hostid]->ana_pointData[i];
            
            temp[0] = anaPoint->lowdata[3];
            temp[1] = anaPoint->lowdata[2];
            temp[2] = anaPoint->lowdata[1];
            temp[3] = anaPoint->lowdata[0];
            
            fptr = (float *) temp;
            //printf("[%3d] %5.2f(%d) ", i, anaPoint->floatData, anaPoint->flag);   
            printf("[%3d] %05.2f(%d) ", i, *fptr, anaPoint->flag);   
                
            if(j == 8) 
            {
                j = 0;
                printf("\n");
                
            }            
        }
        printf("\n");        
        
        ch = getchar();
    }

    printf("\n");    
}

/*----------------------------------------------------------------------------
* Function Name : CmdDispHOSTConfig()
* 수행내용: CCU 내부의 운영정보를 Console로 출력하는 함수
* ArgList :
* Return  :  
---------------------------------------------------------------------------- */   
void CmdDispHOSTConfig()
{
    //int             i,j;
    int             hostid;
    //int             scanid;
    HOST_DCB        *host;
    //SCAN_CONFIG     *scan;

    printf("\n--------[ HOST ] Parameter setting.---------\n");

	printf(" No. Mode   Protoc  Speed  comMod  Mport  Sport  Host   Rtu   UNSOLICT\n");

	for(hostid = 0; hostid < MAX_HOST; hostid++)
    {
        host = (HOST_DCB *) hostDCB[hostid];                /* 주장치 #1 속성정의 */
        printf(" %d  %s %s  %s  %s  %s  %s %5d  %5d       %d[%d] \n", hostid+1, 
        	hostMode[host->hostDualMode], 
        	hostPTType[host->hostProtocol],
        	speedType[host->hostComSpeed], 
        	comModeType[host->hostComType],
        	comPortType[host->masterChan], 
        	comPortType[host->slaveChan],
        	host->hostid, 
        	host->rtuAddr, 
        	host->unsolMode, 
        	host->unsolEvent);
    }
    printf("==================================================\n");  
    printf(" No. Mode   PORT  NET#(M1)   IP-ADDR      NET#(M2)   IP-ADDR      NET#(S1)  IP-ADDR       NET#(S2)   IP-ADDR   \n");

	for(hostid = 0; hostid < MAX_HOST; hostid++)
    {
        host = (HOST_DCB *) hostDCB[hostid];                /* 주장치 #1 속성정의 */
        printf(" %d  %s %5d  %s  %s  %s  %s  %s  %s  %s  %s\n", hostid+1, 
        	tcpipMode[host->tcpMode], host->tcpPort,
        	netType[(int) host->mst_Net1], host->mst_IPAddr1,
        	netType[(int) host->mst_Net2], host->mst_IPAddr2,
        	netType[(int) host->slv_Net1], host->slv_IPAddr1,
        	netType[(int) host->slv_Net2], host->slv_IPAddr2);
    }

	printf("==================================================\n");    
	
	printf(" No. SOE-C  COS-C  COA-C  Unsol Time Reset [maxDI maxAI] dipt dopt aipt aopt \n");
	for(hostid = 0; hostid < MAX_HOST; hostid++)
    {
        host = (HOST_DCB *) hostDCB[hostid];                /* 주장치 #1 속성정의 */
        printf(" %d  %s %s %s %s %3d  %3d    %4d  %4d    %3d  %3d  %3d  %3d\n", hostid+1, 
        	classString[host->soeClass],
        	classString[host->cosClass],
        	classString[host->coaClass],
        	setType[host->unsolEvent],
        	host->timeOutTick,
        	host->resetCount,
        	host->diIndexWord, 
        	host->aiIndexWord,
        	host->diPtNum, host->doPtNum, host->aiPtNum, host->aoPtNum);
        
        //printf("[*] HOST#%d Word Rpt = [DI=%d, AI=%d]\n", hostid+1,host->diIndexWord, host->aiIndexWord);
        //printf("[*] PT [DI/DO/AI/AO]= %2d/%2d/%2d/%2d\n", host->diPtNum,host->doPtNum,host->aiPtNum, host->aoPtNum);
        //printf("[*] DATA-LINK Reset = [%2d]\n\n", host->resetCount);        
    }

    printf("==================================================\n");  
    printf("[*] DUAL-CPU Run Mode  = %s , CPU = %s\n", onoffType[opr->dualCpuMode], cpuTypeStr[opr->cpuMode]);
    printf("[*] DUAL-RTU Run Mode  = %s \n", onoffType[opr->dualRtuMode]);
    printf("[*] USE MMI Module     = %s \n", onoffType[opr->mmiUseFlag]);
    printf("[*] USE SCU Module     = %s \n", onoffType[opr->scuUseFlag]);
    printf("[*] SOE Debounce       = %d \n", opr->soeDebounce);
    printf("[*] Status Dump Period = %2d [sec]\n", opr->stsDumpPeriod);
    printf("[*] Analog Dump Period = %2d [sec]\n", opr->anaDumpPeriod);
    printf("==================================================\n"); 
    printf("[*] Master Network#1 Info(IP/GW/SUB): use = %d, %s/%s/%s\n",   opr->net_mst_Flag[0], opr->net_mst_IP[0], opr->net_mst_GW[0], opr->net_mst_SUB[0]);
    printf("           Network#2 Info(IP/GW/SUB): use = %d, %s/%s/%s\n",   opr->net_mst_Flag[1], opr->net_mst_IP[1], opr->net_mst_GW[1], opr->net_mst_SUB[1]);
    printf("           Network#3 Info(IP/GW/SUB): use = %d, %s/%s/%s\n\n", opr->net_mst_Flag[2], opr->net_mst_IP[2], opr->net_mst_GW[2], opr->net_mst_SUB[2]);
    printf("[*] Slave  Network#1 Info(IP/GW/SUB): use = %d, %s/%s/%s\n",   opr->net_slv_Flag[0], opr->net_slv_IP[0], opr->net_slv_GW[0], opr->net_slv_SUB[0]);
    printf("           Network#2 Info(IP/GW/SUB): use = %d, %s/%s/%s\n",   opr->net_slv_Flag[1], opr->net_slv_IP[1], opr->net_slv_GW[1], opr->net_slv_SUB[1]);
    printf("           Network#3 Info(IP/GW/SUB): use = %d, %s/%s/%s\n",   opr->net_slv_Flag[2], opr->net_slv_IP[2], opr->net_slv_GW[2], opr->net_slv_SUB[2]);
    printf("==================================================\n");
    
}

/*----------------------------------------------------------------------------
* Function Name : CmdDispMMIConfig()
* ArgList :
* Return  :  
---------------------------------------------------------------------------- */   
void CmdDispMMIConfig()
{
    int             i;
    int             ch, count=0;
    
    //MMI_HOST_CFG        *mmiHost;
    MMI_POINT_CFG       *mmiPT;

    printf("\n--------[ MMI ] DATABASE Display.----[CHKSUM = %04x]\n", mmidb->chksum);

    printf("[HOST] Config \n");
    printf(" HOST#1   : config=%d, port= %d, %s\n", mmidb->mmiHost[0].scan, mmidb->mmiHost[0].portNo,  mmidb->mmiHost[0].nameStr);
    printf(" HOST#2   : config=%d, port= %d, %s\n", mmidb->mmiHost[1].scan, mmidb->mmiHost[1].portNo,  mmidb->mmiHost[1].nameStr);
    printf(" HOST#3   : config=%d, port= %d, %s\n", mmidb->mmiHost[2].scan, mmidb->mmiHost[2].portNo,  mmidb->mmiHost[2].nameStr);
    printf(" HOST#4   : config=%d, port= %d, %s\n", mmidb->mmiHost[3].scan, mmidb->mmiHost[3].portNo,  mmidb->mmiHost[3].nameStr);
    printf(" HOST#5   : config=%d, port= %d, %s\n", mmidb->mmiHost[4].scan, mmidb->mmiHost[4].portNo,  mmidb->mmiHost[4].nameStr);
    printf(" CU Name  : %s\n", mmidb->cuName);
    printf(" MAX Point: %d\n", mmidb->maxPoint);
    
    printf("----------------------------------------------------------------------------------\n");
    for(i=0; i < MAX_MODULE; i++)
    {
        printf(" DEVICE#%02d : config=%d, port=%d, %s\n", i+1, mmidb->mmiModule[i].scan, mmidb->mmiModule[i].portNo, mmidb->mmiModule[i].nameStr);
    }
    
    ch= getchar();
    
    printf("----------------------------------------------------------------------------------\n");
    printf(" No. [ dNo dPt]   TYPE       ON-STR      OFF-STR    NAME \n");
    printf("----------------------------------------------------------------------------------\n");
    
    for(i = 0; i < MAX_MMI_POINT; i++)
    {
        mmiPT = (MMI_POINT_CFG *) &mmidb->mmiPoint[i];
        
        printf(" %03d   [%2d %02d]  %7s  %10s   %10s   %s \n",
            i+1, mmiPT->devNo,  mmiPT->pointNo, mmiPtType[mmiPT->ptType],  mmiPT->onStr, mmiPT->offStr, mmiPT->nameStr);
                    
        if(++count == 32)
        {
            count = 0;
            printf("[q]... quit!\n");
            ch=getchar();
            if(ch == 'q')  break;
    
    printf("----------------------------------------------------------------------------------\n");
    printf(" No. [ dNo dPt]   TYPE       ON-STR      OFF-STR    NAME \n");
    printf("----------------------------------------------------------------------------------\n");            
        }
    }
}

/*----------------------------------------------------------------------------
* Function Name : CmdDispMODBUSCfg()
---------------------------------------------------------------------------- */  
void CmdDispMODBUSCfg()
{
    short i, j;
    word	baseAddr,baseOffset,  tripData, closeData;
    
    DB_MODBUS_BLOCK  *dbBlock;
	DB_MODBUS_READ	*readBlock, *timeBlock;
	DB_MODBUS_WRITE	*writeBlock;

    	
    printf("\n %s : MODBUS  Configuration ...\n", TARGET_NAME);
    printf("BLK  No. USE  OPC  Addr  Offset Data  Type  Format ... Trip Close   [Swep:Func]\n");
    for(i = 0; i < 8; i++)
    {
    	printf("-----------------------------------------------------------\n");
        dbBlock = (DB_MODBUS_BLOCK *) &rtudb->modbusBlock[i];
        
        for(j=0; j < 8; j++)
        {
        	readBlock = (DB_MODBUS_READ *) &dbBlock->readBlock[j];
        	baseAddr = readBlock->baseAddr[0]*256 + readBlock->baseAddr[1];
        	baseOffset = readBlock->baseOffset[0]*256 + readBlock->baseOffset[1];
        	if(readBlock->useFlag == 0) continue;
        		
        	printf("%02d.  R%d   %d   %2x   %06d %06d  %03d    %s   %s ... %04x %04x   [%2d/%2d]\n", 
            	i + 1, j+1, 
            	readBlock->useFlag, readBlock->opcode, baseAddr, baseOffset, readBlock->dataNum, 
            	modbusType[readBlock->dataType], modbusFormat[readBlock->dataFormat], 0, 0, readBlock->swepEnb, readBlock->function);
        }
        
        for(j=0; j < 4; j++)
        {
        	writeBlock = (DB_MODBUS_WRITE *) &dbBlock->writeBlock[j];
        	baseAddr = writeBlock->baseAddr[0]*256 + writeBlock->baseAddr[1];
        	tripData = writeBlock->tripData[0]*256 + writeBlock->tripData[1];
        	closeData= writeBlock->closeData[0]*256 + writeBlock->closeData[1];
        	
        	if(writeBlock->useFlag == 0) continue;
        		
        	printf("%02d.  W%d   %d   %2x   %06d %06d  %03d    %s   %s ... %04x %04x\n", 
            	i + 1, j+1, 
            	writeBlock->useFlag, writeBlock->opcode, baseAddr, baseOffset, writeBlock->dataNum, 
            	modbusType[writeBlock->dataType], modbusFormat[writeBlock->dataFormat], tripData, closeData);
        }
        
        timeBlock = (DB_MODBUS_READ *) &dbBlock->timeBlock;
       	baseAddr = timeBlock->baseAddr[0]*256 + timeBlock->baseAddr[1];
        	
       	if(timeBlock->useFlag == 0) continue;
        		
       	printf("%02d.  T%d   %d   %2x   %06d %06d  %03d    %s   %s ... %04x %04x\n", 
           	i + 1, 1, 
            timeBlock->useFlag, timeBlock->opcode, baseAddr, baseOffset, timeBlock->dataNum, 
            modbusType[timeBlock->dataType], modbusFormat[timeBlock->dataFormat], 0, 0);
            	
    }

    printf("-----------------------------------------------------------\n");
    printf("... Configuration End \n");
}


/*----------------------------------------------------------------------------
* Function Name : CmdDispRelayCfg()
* 수행내용: CCU내의 모듈구성정보를 Console로 출력하는 함수
* ArgList :
* Return  :  
---------------------------------------------------------------------------- */  
void CmdDispRelayCfg(void)
{
    short i, j;
    
    DNP_DEVICE  *dnp;
    SCAN_CONFIG *scan;

	printf("\n============================================================================\n");
    printf(" %s : SCAN Configuration ...\n", TARGET_NAME);
    printf("  No.  ACT  PROTO  MODE    PORT  SPEED [DELAY OFFT] TCP[Port Mode IP ]\n");
    printf("============================================================================\n");
    
    for(i = 0; i < MAX_SCAN_PORT; i++)
    {
    	scan = (SCAN_CONFIG *) scanCfg[i];
        
        printf("  %02d.   %d  %s  %s   %5s  %s    %3d    %3d      %4d   %d  %s\n", 
            i + 1,
            scan->active, protocolType[scan->protocol], comModeType[scan->comMode], 
            comPortType[scan->comPort], speedType[scan->comSpeed],
            scan->comDelay, scan->offCount,
            scan->tcpPort, scan->tcpMode, scan->ipAddr);
    }

    printf("----------------------------------------------------------------------------\n");
    printf(" %s : RELAY Configuration ...(Max CU Point = %d\n", TARGET_NAME, opr->maxCuPoint);
    printf("   No.    DEVICE-Name          Port     Type     Addr Scan  [ DI/ DO/ AI/ AO] MOD Dual ONL [IIN] [ SND / RCV ] PRE  [IP_ADDR  Port]\n");
    printf("----------------------------------------------------------------------------\n");
    for(i = 0,j=0; i < MAX_MODULE; i++)
    {
        dnp = (DNP_DEVICE *) dnpcfg[i];
        
        scan = (SCAN_CONFIG *) scanCfg[dnp->scanPort];
        
        if((dnp->type == 0) && (dnp->scan == 0))    continue;
            
        printf("   %02d. %20s %8s %12s %2d  %02d  [%3d/%3d/%3d/%3d]  %2d  %2d   %02x  %02x-%02x [%05d-%05d]  %05d [%-16s,%4d]\n", 
            i + 1,
            dnp->nameStr,
            comPortType[scan->comPort],
            modType[dnp->type],
            dnp->ioid, 
            dnp->scan, 
            dnp->devDiPoint, dnp->devDoPoint, dnp->devAiPoint,dnp->devAoPoint,
            dnp->modbusFileNo, dnp->dualIED, 
            dnp->online,
            dnp->rcvIIN[0], 
            dnp->rcvIIN[1],
            (int)dnp->comSndCount,
            (int)dnp->comRcvCount,
            (int)dnp->oldRcvCount,
            dnp->ipString,
            dnp->netPort);
    }

}


/*----------------------------------------------------------------------------
* Function Name : CmdDispDIConfig()
* 수행내용: CCU내의 모듈에 대한 포인트 구성에 대한 정보를 Console로 출력하는 함수
* ArgList :
*   1. ioid - 출력하고자 하는 모듈 번호
* Return  :  
---------------------------------------------------------------------------- */   
int CmdDispDIConfig(void)
{
    int     hostid,point;
    int     index;
    char            ch;
    short   devNo, devPt;
    
    //char            *buf;
    POINT_BUF       *ptBuf;
    HOST_DCB        *host;
    DIPOINT_INFO    *hostSts;
    DNP_DEVICE      *dnp;

    printf("\n==> 상위 HOST 상태포인트 정보... \n");    
    printf("======> HOST Number [1/2/3/4/5] : ");
    hostid = getDecWord();
    
    if(hostid <= MAX_HOST)  hostid = hostid - 1;
    else                    return (0);
    
    index = 0;
    host = (HOST_DCB *) hostDCB[hostid];

    printf("\n[HOST %02d] State Info-------------------------------------------------\n", hostid+1);
    printf("[*] HOST Status = %02x\n", host->hostStatus);  
    printf(" No. [ dNo dPt ] config Multi   Ststus  Name\n");
    printf("-----------------------------------------------------------------------\n");
    
    // MAX_DNP_DI_POINT : 1152    
    for(point = 0; point < MAX_DNP_DI_POINT; point++)
    {
        hostSts = (DIPOINT_INFO *) &host->stateInfo[point];
        if(hostSts->config == RESET)   continue;
    
        devNo = hostSts->devNo;
        devPt = hostSts->devPt;
        
        dnp = (DNP_DEVICE *) dnpcfg[devNo];
        ptBuf = (POINT_BUF *) &dnp->diPtBuf[devPt];
                    
        printf(" %03d   [%2d  %3d]   %2d     %2d       %02x    %s \n", point + 1, hostSts->devNo+1, hostSts->devPt+1, hostSts->config, hostSts->multiPoint, host->sts_pointData[point], ptBuf->nameStr);
    
        if(++index == 32)
        {
            index = 0;
            ch= getchar();
            if(ch == 'q')   break;      
        }        
    }            
    return (0);
} 

/*----------------------------------------------------------------------------
* Function Name : CmdDispDIConfig()
* 수행내용: CCU내의 모듈에 대한 포인트 구성에 대한 정보를 Console로 출력하는 함수
* ArgList :
*   1. ioid - 출력하고자 하는 모듈 번호
* Return  :  
---------------------------------------------------------------------------- */   
int    CmdDispAIConfig()
{
    int     hostid,point;
    int     index;
    int     devNo, devPt;
    char            ch;
    POINT_BUF       *ptBuf;
    HOST_DCB        *host;
    AIPOINT_INFO    *hostAna;
    DNP_ANA_INPUT   *anaPoint;
    DNP_DEVICE      *dnp;

    printf("\n==> 상위 HOST 계측포인트 정보... \n");    
    printf("======> HOST Number [1/2/3/4/5] : ");
    hostid = getDecWord();
    
    if(hostid <= MAX_HOST)  hostid = hostid - 1;
    else                    return (0);
    
    index =0;
    host = (HOST_DCB *) hostDCB[hostid];

    printf("\n[HOST %02d] Analog Info-------------------------------------------------\n", hostid+1);
    printf("[*] HOST Status = %02x\n", host->hostStatus);  
    printf(" No. [ dNo dPt] cfg  Data   Name\n");
    printf("-----------------------------------------------------------------------\n");
        
    for(point = 0; point < MAX_DNP_AI_POINT; point++)
    {
        hostAna = (AIPOINT_INFO *) &host->analogInfo[point];
        if(hostAna->config == RESET)   continue;
        
        devNo = hostAna->devNo;
        devPt = hostAna->devPt;
        
        dnp = (DNP_DEVICE *) dnpcfg[devNo];
        ptBuf = (POINT_BUF *) &dnp->aiPtBuf[devPt];
        anaPoint = (DNP_ANA_INPUT *) &host->ana_pointData[point];
        printf(" %03d  [%2d  %3d]  %2d  %4.1f \t%s \n", point+1, hostAna->devNo+1, hostAna->devPt+1, hostAna->config, anaPoint->floatData, ptBuf->nameStr);
        
        if(++index == 32)
        {
            index = 0;
            ch= getchar();
            if(ch == 'q')   break;      
        }
    }      
    return (0);    
} 

/*----------------------------------------------------------------------------
* Function Name : CmdDispDOConfig()
* 수행내용: CCU내의 모듈에 대한 포인트 구성에 대한 정보를 Console로 출력하는 함수
* ArgList :
*   1. ioid - 출력하고자 하는 모듈 번호
* Return  :  
---------------------------------------------------------------------------- */   
int CmdDispDOConfig()
{
    int     hostid,point;
    int     index;
    int     devNo, devPt;
    char            ch;
    POINT_BUF       *ptBuf;
    HOST_DCB        *host;
    CONTROL_INFO    *hostCntr;
    DNP_DEVICE      *dnp;

    printf("\n==> 상위 HOST 제어포인트 정보... \n");    
    printf("======> HOST Number [1/2/3/4/5] : ");
    hostid = getDecWord();
    
    if(hostid <= MAX_HOST)  hostid = hostid - 1;
    else                    return (0);
        
    host = (HOST_DCB *) hostDCB[hostid];

    printf("\n[HOST %02d] Control Info-------------------------------------------------\n", hostid+1);
    printf("[*] HOST Status = %02x\n", host->hostStatus);  
    printf(" No. [ dNo dPt] config  dbmax   Name\n");
    printf("-----------------------------------------------------------------------\n");

    index = 0;        
    for(point = 0; point < MAX_DNP_DO_POINT; point++)
    {
        hostCntr = (CONTROL_INFO *) &host->controlInfo[point];
        if(hostCntr->config == RESET)   continue;
        
        devNo = hostCntr->devNo;
        devPt = hostCntr->devPt;
        
        dnp = (DNP_DEVICE *) dnpcfg[devNo];
        ptBuf = (POINT_BUF *) &dnp->doPtBuf[devPt];
        
        printf(" %03d   [%2d  %3d]   %2d  %04d   %s \n", point+1, hostCntr->devNo+1, hostCntr->devPt+1, hostCntr->config, hostCntr->dbmax, ptBuf->nameStr);

        if(++index == 32)            
        {                            
            index = 0;               
            ch= getchar();           
            if(ch == 'q')   break;   
        }                            
    }      
    return (0);    
} 




/*----------------------------------------------------------------------------
* Function Name : CmdDispDEVConfig()
* 수행내용: CCU내의 모듈에 대한 포인트 구성에 대한 정보를 Console로 출력하는 함수
* ArgList :
*   1. ioid - 출력하고자 하는 모듈 번호
* Return  :  
---------------------------------------------------------------------------- */   
int CmdDispDEVConfig()
{
    int         index;
    int             point;
    char            ch;
    POINT_BUF       *devPt;

    printf("\n[SDP-DEVICE Point List] -----------------------------------------------\n" );
    printf(" D.No [ dNo dPt] cfg  HST1 HST2 HST3 HST4 HST5 [STS] \n");
    printf("-----------------------------------------------------------------------\n");
    index = 0;           
    for(point = 0; point < MAX_DEV_POINT; point++)
    {
        devPt = (POINT_BUF *) devPtBuf[point];
        
        if(devPt->config == 0)  continue;    
        printf(" %02d    [%2d  %3d]   %2d  %3d  %3d  %3d  %3d  %3d  [ %d ]  %s\n", point +1, devPt->devNo, devPt->devPt, devPt->config, 
            devPt->hostIndex[0], devPt->hostIndex[1], devPt->hostIndex[2], devPt->hostIndex[3], devPt->hostIndex[4], devPt->status, devPt->nameStr);

        if(++index == 32)            
        {                            
            index = 0;               
            ch= getchar();           
            if(ch == 'q')   break;   
        }                      
    }    
    printf("-----------------------------------------------------------------------\n\n");
    return (0);
} 


/*----------------------------------------------------------------------------
* Function Name : CmdDispPOINTConfig()
* 수행내용: CCU내의 모듈에 대한 포인트 구성에 대한 정보를 Console로 출력하는 함수
* ArgList :
*   1. ioid - 출력하고자 하는 모듈 번호
* Return  :  
---------------------------------------------------------------------------- */   
void CmdDispPOINTConfig()
{
    int     i,ch, count=0 ;
    int     devNo, devPt;
    int     ptType, devType, ptConfig;
    int		port1, point1, port2, point2;
    int     dnp_index[MAX_HOST];
    int     dbmax, modBase, modIndex;
    int     dbCFG, dbMWB;
    byte    ptname[32];
    //POINT_BUF *ptBuf;
    DEVICE_PTCFG    *devPoint;
    
    printf("\n%s : RTUDB Point Configure ...\n", TARGET_NAME);

    printf("---------------------------------------------------------------------------------------------------------------\n");
    printf(" No.  [ dNo  dPt]   TYPE  PT-TYP  M-base M-inx CFG DMAX PO1 PT1 PO2 PT2 [CIN MWB] HST1 HST2 HST3 HST4 HST5 \n");
    printf("---------------------------------------------------------------------------------------------------------------\n");
    
    for(i = 0; i < MAX_DBASE_POINT; i++)
    {
        devPoint = (DEVICE_PTCFG *) &rtudb->devPtConfig[i];
        devNo = devPoint->devNo;
        devPt = (devPoint->devPt[0]*256) + devPoint->devPt[1];
        
        //if((devNo < 1) || (devNo > MAX_MODULE))     	continue;
        //if((devPt < 1) || (devPt > MAX_DEV_DI_POINT))   continue;
        //if(devPoint->devType > DEV_POINT)   continue;

        bzero8248(ptname, 32);
        
        /* DNP-HOST Information */
        devType    = devPoint->devType;
        ptType     = devPoint->ptType;
        ptConfig   = devPoint->ptConfig;
        modBase    = (devPoint->modBase[0]*256) + devPoint->modBase[1];
        modIndex   = (devPoint->modIndex[0]*256) + devPoint->modIndex[1];
        dbmax      = (devPoint->maxValue[0]*256) + devPoint->maxValue[1];
        port1      = devPoint->port1;
        point1     = devPoint->point1;
        port2      = devPoint->port2;
        point2     = devPoint->point2;
        
        dbCFG      = devPoint->cntrFlag;
        dbMWB      = devPoint->writeBlock;
        //dbhostpt   = (devPoint->landisIndex[0]*256) + devPoint->landisIndex[1];
        
        dnp_index[0]= (devPoint->hostIndex1[0]*256) + devPoint->hostIndex1[1];
        dnp_index[1]= (devPoint->hostIndex2[0]*256) + devPoint->hostIndex2[1];
        dnp_index[2]= (devPoint->hostIndex3[0]*256) + devPoint->hostIndex3[1];
        dnp_index[3]= (devPoint->hostIndex4[0]*256) + devPoint->hostIndex4[1];
        dnp_index[4]= (devPoint->hostIndex5[0]*256) + devPoint->hostIndex5[1];
        
        memcpy(ptname, devPoint->pointName, 20);
        ptname[19] = '\0';
        
        printf(" %03d   [%2d  %4d]   %4s  %4s  %6d  %3d    %2d  %4d  %2d  %2d  %2d  %2d  [%2d  %2d] %4d %4d %4d %4d %4d  %s \n",
            i+1, devNo,  devPt,     
            devTypeStr[devType], pointTypeStr[ptType],modBase, modIndex, ptConfig, dbmax, 
            port1, point1, port2, point2,
            dbCFG, dbMWB, dnp_index[0], dnp_index[1], dnp_index[2],dnp_index[3],dnp_index[4], ptname);
                    
        if(++count == 32)
        {
            count = 0;
            printf("[q]... quit!\n");
            ch=getchar();
            if(ch == 'q')  break;

    printf("---------------------------------------------------------------------------------------------------------------\n");
    printf(" No.  [ dNo  dPt]   TYPE  PT-TYP  M-base M-inx CFG DMAX PO1 PT1 PO2 PT2 [CIN MWB] HST1 HST2 HST3 HST4 HST5 \n");
    printf("---------------------------------------------------------------------------------------------------------------\n");
                    
        }
                    
    }
    printf("\n");

} 
