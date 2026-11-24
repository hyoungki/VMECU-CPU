/*
 * ==============================================================
 * System TARGET : ACE Control VME-CU Ver 2.0   
 * Target CPU    : MPC8248, VME6U
 * Main Factors  : 호남고속철용 VMECU
 *          - CPU 이중화 구성
 *          - VMEBUS Function : DI/DO/AI (프로세서 타입)    
 *          - 통합 시뮬레이터 : 2013/07/08 V8.2 적용 
 * --------------------------------------------------------------
 * System DESIGN : SANE-SYSTEM   .... by  Lee Ho-Sang
 * Initial-DATA  : 2012,12,10
 * Last Updated  : 2013,07,10
 * ==============================================================
 */

#ifndef	EXTERNAL_HEADER_INCLUDED

#define	EXTERNAL_HEADER_INCLUDED

//
// External Function(s)
//
//extern void 		Debug(TASK_INFO *prcPtr, ...);
//extern void 		DumpBuffer(TASK_INFO *prcPtr, ...);
extern void 		ExecCommand(char *format, ...);
extern void			FileClose(FILE_DESC *fp);
extern int			FileOpen(FILE_DESC *fp);
extern int			FileRead(FILE_DESC *fp, char *buffer, int length);
extern int			FileReadField(FILE_DESC *fp, char *ptr, int ofst, int len);
extern int			FileSave(FILE_DESC *fp, char *buffer, int length);
extern int			FileSaveDirect(FILE_DESC *fp, char *buffer, int length);
extern int			FileWrite(FILE_DESC *fp, char *buffer, int length);
extern int			FileWriteField(FILE_DESC *fp, char *ptr, int ofst, int len);
extern int			GetClock(struct timeval *prev);

extern WORD			GenCrc(char *buffer, int length);
//extern BYTE         GenLrc(char *buffer, int length);
extern word         gensum(byte *buf, int bfcnt);
extern byte         genlrc(byte *buf, int bfcnt);
extern byte	        genSumByte(byte *buf, int bfcnt);

extern word         genCrc_modbus (byte *ptr, int len);

extern int			GetFileStat(char *fileName, struct stat *state);
extern int			GetMsecDelta(struct timeval *prev, struct timeval *cur);
extern struct tm	*GetTod(void);
extern WORD			GetWord(char *ptr, int mode);
extern int			IsProcessActive(int pid);
extern void			KillProcess(pid_t pid, int wait);
extern int			PioAccess(int status);
extern int			PioInit(int mode);
extern short		PioReadImage(short *ptr, int length);
extern short		PioReadStatus(void);
extern void			PioReset(void);
extern void			PioStartGrabbing(int mode);
extern int			PortAttribute(TTY_DESC *tp);
extern void			PortClose(TTY_DESC *tp);
extern int			PortOpen(TTY_DESC *tp);
extern int          PortRead(TTY_DESC *tp, byte *bfptr, int count);

extern int			PortRxReady(int fd, struct timeval *timeOut);
extern int			PortWrite(TTY_DESC *tp, byte *buffer, int length);
extern int          PortInitial(TTY_DESC *ttyDesc);

extern int			QueueCheck(MSG_DESC *qp);
extern void			QueueClear(MSG_DESC *qp);
extern int			QueueCreate(MSG_DESC *qp);
//extern bool			QueueCreateList(MSG_DESC *qp, int count);
extern void			QueueDelete(MSG_DESC *qp);
extern void			QueueDeleteList(MSG_DESC *qp, int count);
extern int			QueueRcv(MSG_DESC *qp, char *buffer, int length);
extern int			QueueSnd(MSG_DESC *qp, char *buffer, int length);
extern int			SendLoopCmd(TASK_INFO *prcPtr, ...);
extern void			SetClock(struct timeval *tv);
extern void			SetEtime(TASK_INFO *tp, int eTime);
extern void			SetPortInfo(TTY_DESC *tp, PORT_INFO *ptr);
extern void			SetWord(char *ptr, int value, int mode);
extern int			ShmCheck(SHM_DESC *sp);
extern int			ShmCreate(SHM_DESC *sp);
extern void			ShmDelete(SHM_DESC *sp);
extern void			ShmDetach(SHM_DESC *sp);
extern void			StartProcess(char *name, char *argument);
extern void			SysLog(int logging, ...);

extern void			UpdateProcessInfo(TASK_INFO *prcPtr, int state, pid_t pid, int arg);

extern  void    sysEvent1(OPR_MSG *opr, SYSTEM_QUE *shmSque, int logid, int ioid, int point, int state, int hostid);
extern  void    logEvent1(OPR_MSG *opr, SYSTEM_QUE *shmSque, int logid, int ioid, int point, int state, int hostid);
extern  void    logHistoryQue1(OPR_MSG *opr, HISTORY_QUE *shmHque, int logid, int ioid, int point, int state, int hostid);

extern  void    soeEvent1(OPR_MSG *opr, SYSTEM_QUE *shmSque, int logid, int ioid, int point, int state, int hostid);
extern  void    soeHistoryQue1(OPR_MSG *opr, HISTORY_QUE *shmHque, int logid, int ioid, int point, int state, int hostid);

extern  void    bzero8248(byte *buf, int size);
extern  void    memcpy8248(volatile byte *des, volatile byte *src, int size);

extern  void    Debug(CONSOLE_INFO *debug, ...);
extern  void    DumpBuff(CONSOLE_INFO *debug, ...);

extern  int     dbFileRead(RTU_DATABASE *localDB, int mode);
extern  int     dbFileWrite(RTU_DATABASE *localDB, int mode);

extern  word    B013_mkdnpcrc( byte *buff,  int count);
extern  word    B013_ckdnpcrc(byte *buff, int count);

extern  void    DumpDNP_snd(CONSOLE_INFO *debug, ...);
extern  void    DumpDNP_rcv(CONSOLE_INFO *debug, ...);

extern  void    prndat(const char *msg, char *str, int  len);
extern  void    prn_snd(char *msg, char *str, int len);
extern  void    prn_rcv(char *msg, char *str, int len);

extern  int	    asyncRead( OPR_MSG *opr, TTY_DESC	*tp,  byte *bfptr, int count);

extern  int     TKreadn (int fd, char *buf, int size , int timeout);
extern  int     tkWriteTCP(int sockfd, byte *txBuffer, int size,int timeout);

extern  int     vmeChanWrite(int chid, VME_SIODCB *vmeSio, VME_CHAN_DCB *vmeChan, byte *txbuf, int count);
extern  int     vmeChanRead(int chid, VME_SIODCB *vmeSio, VME_CHAN_DCB *vmeChan, byte *rxbuf, int reqCount);

extern  int     calclock (struct timespec inclock, struct timespec outclock);
extern  void    makeDnpTimeByte(PDPDTIME_MS_SINCE_70 *pMsSince70, PDPDTIME_DATE_TIME *pDateTime);
extern  void    makeDnpTimeRTC(PDPDTIME_DATE_TIME   *pDateTime, PDPDTIME_MS_SINCE_70 *pMsSince70);

extern  int     LogFile_MPU (OPR_MSG *opr, int code, char *outstr, int osize);                                                  

extern  RTC             *rtc;
extern  OPR_MSG         *opr;
extern	LINK_MSG		*linkCfg;
extern  SCU_MSG         *scuCfg;

extern  RTU_DATABASE    *rtudb;
extern  MMI_DATABASE    *mmidb;

extern  SYSTEM_QUE      *sque;              // MMI 용 이벤트
extern  HISTORY_QUE     *hque;              // CONSOLE 용 이벤트
//extern  MMI_COA_QUE     *mmiCOA;            // MMI Analog-Event


/* DEVICE(계전기) 관련 공유메모리 */
extern  SCAN_CONFIG     *scanCfg[MAX_SCAN_PORT];         // 하위계전기 SCAN Config
extern  DNP_DEVICE      *dnpcfg[MAX_MODULE];
extern  POINT_BUF       *devPtBuf[MAX_DEV_POINT];

/* HOST 관련 공유메모리 */
extern  HOST_DCB        *hostDCB[MAX_HOST];             /* HARRIS, LANDIS, SICS HOST Structure */

extern  LANDIS_CFG      *landis;                        /* LANDIS HOST Structure */
extern  HOST_STSBUF     *hostStsBuf[MAX_HOST_STS_PT];       /* LANDIS-HOST defined Status Point */
extern  HOST_ANABUF     *hostAnaBuf[MAX_HOST_ANA_PT];       /* LANDIS-HOST defined Analog Point */

extern  CONSOLE_INFO	*console;

extern  byte    errmsg[NUMBER_OF_EVENT][20];

#endif	// EXTERNAL_HEADER_INCLUDED

//
// End of external.h
//
