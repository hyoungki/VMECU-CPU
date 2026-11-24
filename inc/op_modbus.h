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

/* ---------------------------------------------------- */
/*  MODBUS 통신 OPCODE                                  */
/* ---------------------------------------------------- */   
#define MODBUS_RD_INPUT_BIT     0x01        // Read Bit-Coil Registers
#define MODBUS_RD_INPUT_DIS     0x02        // Read Bit Discretes Registers
#define MODBUS_RD_HOLD_REG      0x03        // Read Holding Registers
#define MODBUS_RD_INPUT_REG     0x04        // Read Input Registers
#define MODBUS_WR_SINGLE_COIL   0x05        // Write Single Coil
#define MODBUS_WR_SINGLE_REG    0x06        // Write Single Register
#define MODBUS_WR_MULTI_REG     0x10        // Write Single Register


#define MPLC_RD_WORD            0x01     
#define MPLC_RD_RESP            0x81
        
#define MPLC_WR_WORD            0x04     
#define MPLC_WR_RESP            0x84

#define MPLC_X                  1
#define MPLC_M                  2
                     
/* ---------------------------------------------------- */
/*  MODBUS 제어절차에 따른 Base Address                 */
/* ---------------------------------------------------- */   
#define CLOSE_SELECT        	0
#define CLOSE_OPERATE       	1
#define TRIP_SELECT         	2
#define TRIP_OPERATE        	3

/* readBlock : Block 데이타 타입 정의 */
#define	MODBUS_TYPE_DI			0
#define	MODBUS_TYPE_AI			1
#define	MODBUS_TYPE_DO			2
#define	MODBUS_TYPE_AO			3

#define	FORMAT_BIT				0
#define	FORMAT_BYTE				1
#define	FORMAT_SHORT			2
#define	FORMAT_WORD				3
#define	FORMAT_INT32			4
#define	FORMAT_UINT32			5
#define	FORMAT_FLOAT			6


/* ---------------------------------------------------- */
/*  2017.08.04                                          */
/*  MODBUS 제어방식별 Config 정의                       */
/* ---------------------------------------------------- */
#define MODBUS_TRIP_FIRST       0       // TRIP/CLOSE 제어시 : TRIP  선행제어
#define MODBUS_CLOSE_FIRST      1       // TRIP/CLOSE 제어시 : CLOSE 선행제어
#define MODBUS_RESET_CNTR       2       // RESET 제어시
#define MODBUS_ONOFF_CNTR       3       // ON/OFF 제어시

#define MPLC_INTERLOCK         	4       // 미쯔비시 계전기... 인터락 포인트

#define MODBUS_TC_PLUSE         5       // TRIP/CLOSE 제어후 RESET : 동일 Address, TRIP (2), CLOSE (1), RESET(0)
#define MODBUS_TRIP_PLUSE       6       // PLUSE 제어후 RESET : TRIP  First, 제어시(1), RESET(0)      
#define MODBUS_CLOSE_PLUSE      7       // PLUSE 제어후 RESET : CLOSE First, 제어시(1), RESET(0)      
     
#define MODBUS_SBO_LG           8       // LG  계전기 대상 SELECT-OPERATE 제어시 : CLOSE First
#define MODBUS_SBO_ICE          11      // ICE 계전기 대상 SELECT-OPERATE 제어시 : FAULT-RESET 





/* .......... end of "MODBUS.H" ............... */
