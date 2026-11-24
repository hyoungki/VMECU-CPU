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

#define CCU_STS_PFR         0x01        /* Status of POWER-FAIL */
#define CCU_STS_OFL         0x02        /* Status of OFFLINE-DEVICE */
    
/* ------------------------------------ */
/* RTU SIMULATOR Protocol Constant      */
/* ------------------------------------ */
#define SIM_GPOLL           0x70    
#define SIM_CHANGE_CPU      0x71        // MASTER-CPU SET
#define SIM_CHANGE_ACK      0x72        // SLAVE_CPU SET

#define	SIM_CHKSUM			0x01

#define SIM_EVENT_DOWN      0x03        // 이중화 CPU 간 이벤트 전송
#define SIM_EVENT_UP        0x04        // 이중화 CPU 간 이벤트 전송
#define SIM_EVENT_ACK       0x05        // 이중화 CPU 간 이벤트 ACK

#define SIM_HISTORY_DOWN    0x06        // 이중화 CPU 간 이벤트 전송
#define SIM_HISTORY_UP      0x07        // 이중화 CPU 간 이벤트 전송
#define SIM_HISTORY_ACK     0x08        // 이중화 CPU 간 이벤트 ACK


#define SIM_DEV_STATUS      0x00        /* DEVICE Status Read */
#define SIM_POINT_SCAN      0x20        /* 계전기 POINT 정보 Scan */
#define SIM_POINT_CNTR      0x22        /* 계전기 POINT 제어 */

#define SIM_POINT_STATUS    0x30        /* 계전기 상태포인트 POINT 정보 Scan */
#define SIM_POINT_ANALOG    0x32        /* 계전기 계측포인트 POINT 정보 Scan */

#define SIM_SOE_DUMP        0x34        /* 계전기 SOE 정보 Update */
#define SIM_SOE_ACK         0x42        /* 계전기 SOE 정보 Update */

#define SIM_CONTROL_INFO    0x36        /* 계전기 제어정보 연계 */
#define SIM_CONTROL_ACK     0x41        /* 계전기 제어정보 연계 */

#define SIM_DEVICE_INFO     0x38        /* 계전기 제어정보 연계 */

#define SIM_TIME_DOWN       0x10        /* Time Sync Down */
#define SIM_TIME_UP         0x12        /* Time Sync Up */
#define SIM_TIME_ACK        0x43        /* Time Sync Down */

#define SIM_HOST_DOWN       0x50        /* HOST Config Database Down */
#define SIM_HOST_UP         0x51        /* HOST Config Database Up */
#define SIM_SCAN_DOWN       0x52        /* SCAN Config Database Down */
#define SIM_SCAN_UP         0x53        /* SCAN Config Database Up */
#define SIM_PORT_DOWN       0x54        /* Port Config Database Down */
#define SIM_PORT_UP         0x55        /* Port Config Database Up */
#define SIM_CHASSIS_DOWN    0x56        /* Port Config Database Down */
#define SIM_CHASSIS_UP      0x57        /* Port Config Database Up */
#define SIM_POINT_DOWN      0x58        /* Point Config Database Down */
#define SIM_POINT_UP        0x59        /* Point Config Database Up */

#define SIM_MODBUS_DOWN     0x60        /* MODBUS Profile Block Database Down */
#define SIM_MODBUS_UP       0x61        /* MODBUS Profile Block Database Up */




/* ------------------------------------ */
/* MMI Protocol Constant      */
/* ------------------------------------ */
#define MMI_TIME_SYNC		0x00    
#define MMI_COM_STATUS		0x10    
#define	MMI_PTSTS_DUMP		0x20
#define	MMI_PTANA_DUMP		0x30
#define	MMI_PT_CONTROL		0x32
#define	MMI_CU_CONTROL		0x34

#define	MMI_INIT_EVENT		0x40
#define	MMI_EVENT_DUMP		0x50
#define	MMI_EVENT_ACK 		0x52

#define MMI_COA_DUMP        0x56        // 아날로그 COA 덤프
#define MMI_COA_ACK         0x58        // 아날로그 COA - ACK

#define	MMI_CHKSUM_LOAD		0x60

#define	MMI_HOSTDB_DUMP		0x70
#define	MMI_DEVICEDB_DUMP	0x80
#define	MMI_POINTDB_DUMP	0x90



/*----------------------------------------- */
