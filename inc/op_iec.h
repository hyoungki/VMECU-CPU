
//-------------------------------------------------------------------------------
//	for IECDEFS.H
//-------------------------------------------------------------------------------
#define IEC_LINK_NULL_TYPE  			0x00	/* fixed length type          */
#define IEC_LINK_FIXED_TYPE  			0x10	/* fixed length type          */
#define IEC_LINK_VARIABLE_TYPE			0x68	/* variable length type       */

#define IEC_LINK_FIXED_HEADER_SIZE  	5   /* number of bytes in header  */
#define IEC_LINK_VARIABLE_HEADER_SIZE  	6
#define IEC_MAX_ASDU_DATA_SIZE    		245
#define IEC_MAX_ASDU_DATA_NUM     		127

#define IEC_CLASS1_TYPE           		0
#define IEC_CLASS2_TYPE           		1

#define IEC_TIME_UPDATE_ONLY      		1
#define IEC_TIMER_UPDATE          		2


/* ASDUs for process information in monitor direction */
#define IEC_TYPE_NO_DEFINE  	0   	/* No object type assigned  */
#define IEC_TYPE_M_SP_NA  		1     	/* Single-point information, without time tag	*/
#define IEC_TYPE_M_SP_TA  		2     	/* Single-point information, with time tag CP24Time2a  	  */
#define IEC_TYPE_M_DP_NA  		3     	/* Double-point information, without time tag   */
#define IEC_TYPE_M_ST_NA  		5     	/* Step position information, without time tag	 */
#define IEC_TYPE_M_BO_NA  		7     	/* Bitstring of 32 bits without time tag 		 */
#define IEC_TYPE_M_ME_NA  		9     	/* Measured value, normalized,  without time tag  */
#define IEC_TYPE_M_ME_NB  		11    	/* Measured value, scaled without, time tag 	  */
#define IEC_TYPE_M_ME_NC  		13    	/* Measured value, short floating point number without time tag  */
#define IEC_TYPE_M_IT_NA  		15    	/* Integrated totals without time tag */
#define IEC_TYPE_M_PS_NA  		20    	/* Packed single-point with  status change detection  	  */
#define IEC_TYPE_M_ME_ND  		21    	/* Measured value, normalized value without quality  */

#define IEC_TYPE_M_SP_TB  		30		/* Single point information with time tag CP56Time2a   */
#define IEC_TYPE_M_DP_TB  		31		/* Double point information with time tag CP56Time2a   */
#define IEC_TYPE_M_ST_TB  		32		/* Step position information with  time tag CP56Time2a  */
#define IEC_TYPE_M_BO_TB  		33		/* Bitstring of 32 bits with time  tag CP56Time2a  */

/* ASDUs for process information in control direction */
#define IEC_TYPE_C_SC_NA  		45    	/* Single Command 				  */
#define IEC_TYPE_C_DC_NA  		46    	/* Double Command 				  */
#define IEC_TYPE_C_RC_NA  		47		/* Regulating step command 		  */
#define IEC_TYPE_C_SE_NA  		48    	/* Set-point command, normalized  value */
#define IEC_TYPE_C_SE_NB  		49		/* Set-point command, scaled value */
#define IEC_TYPE_C_SE_NC  		50		/* Set-point command, short floating point number */
#define IEC_TYPE_C_BO_NA  		51		/* Bitstring 32 bit 			  */

/* ASDUs for system information in monitor direction */
#define IEC_TYPE_M_EI_NA  		70    	/* End of initialization 		  */
#define IEC_TYPE_C_IC_NA  		100   	/* Interrogation command 		  */
#define IEC_TYPE_C_CI_NA  		101   	/* Counter interrogation command  */
#define IEC_TYPE_C_CS_NA  		103		/* Clock synchronization 		  */
#define IEC_TYPE_C_RP_NA  		105		/* 재설정 		  */
#define IEC_TYPE_C_CD_NA  		106		/* Delay acquisition command      */


/* Cause of transmission code */
#define COT_NO_DEFINE                               0
#define COT_PERIODIC_CYCLIC                         1
#define COT_BACKGROUND_SCAN                         2
#define COT_SPONTANEOUS                             3
#define COT_INITIALIZED                             4
#define COT_REQUEST_OR_REQUESTED                    5
#define COT_ACTIVATION                              6

#define COT_ACTIVATION_CONFIRMATION                 7
#define COT_DEACTIVATION                            8
#define COT_DEACTIVATION_CONFIRMATION               9
#define COT_ACTIVATION_TERMINATION                  10

#define COT_RETURN_INFO_BY_REMOTE                   11
#define COT_RETRUN_INFO_BY_LOCAL                    12
#define COT_FILE_TRANSFER                           13
#define COT_REQUEST_BY_STATION                      20
#define COT_REQUEST_BY_GROUP_1                      21
#define COT_REQUEST_BY_GROUP_2                      22
#define COT_REQUEST_BY_GROUP_3                      23
#define COT_REQUEST_BY_GROUP_4                      24
#define COT_REQUEST_BY_GROUP_5                      25
#define COT_REQUEST_BY_GROUP_6                      26
#define COT_REQUEST_BY_GROUP_7                      27
#define COT_REQUEST_BY_GROUP_8                      28
#define COT_REQUEST_BY_GROUP_9                      29
#define COT_REQUEST_BY_GROUP_10                     30
#define COT_REQUEST_BY_GROUP_11                     31
#define COT_REQUEST_BY_GROUP_12                     32
#define COT_REQUEST_BY_GROUP_13                     33
#define COT_REQUEST_BY_GROUP_14                     34
#define COT_REQUEST_BY_GROUP_15                     35
#define COT_REQUEST_BY_GROUP_16                     36
#define COT_REQUEST_BY_GROUP_GENERAL_COUNTER        37
#define COT_REQUEST_BY_GROUP_1_COUNTER              38
#define COT_REQUEST_BY_GROUP_2_COUNTER              39
#define COT_REQUEST_BY_GROUP_3_COUNTER              40
#define COT_REQUEST_BY_GROUP_4_COUNTER              41
#define COT_UNKNOWN_TYPE_IDENTIFICATION             44
#define COT_UNKNOWN_COT_BY_HOST                     45
#define COT_UNKNOWN_COMMON_ADDRESS                  46
#define COT_UNKNOWN_INFO_OBJECT_ADDRESS             47


#if 0

/* Cause of transmission code */
#define IECDEFS_COT_NO_DEFINE                                0
#define IECDEFS_COT_PERIODIC_CYCLIC                          1
#define IECDEFS_COT_BACKGROUND_SCAN                          2
#define IECDEFS_COT_SPONTANEOUS                              3
#define IECDEFS_COT_INITIALIZED                              4
#define IECDEFS_COT_REQUEST_OR_REQUESTED                     5
#define IECDEFS_COT_ACTIVATION                               6
#define IECDEFS_COT_ACTIVATION_CONFIRMATION                  7
#define IECDEFS_COT_DEACTIVATION                             8
#define IECDEFS_COT_DEACTIVATION_CONFIRMATION                9
#define IECDEFS_COT_ACTIVATION_TERMINATION                   10
#define IECDEFS_COT_RET_INFO_CAUSED_BY_REMOTE_COMMAND        11
#define IECDEFS_COT_RETRUN_INFO_CAUSED_BY_LOCAL_COMMAND      12
#define IECDEFS_COT_FILE_TRANSFER                            13
#define IECDEFS_COT_INTERROGATED_BY_STATION                  20
#define IECDEFS_COT_INTERROGATED_BY_GROUP_1                  21
#define IECDEFS_COT_INTERROGATED_BY_GROUP_2                  22
#define IECDEFS_COT_INTERROGATED_BY_GROUP_3                  23
#define IECDEFS_COT_INTERROGATED_BY_GROUP_4                  24
#define IECDEFS_COT_INTERROGATED_BY_GROUP_5                  25
#define IECDEFS_COT_INTERROGATED_BY_GROUP_6                  26
#define IECDEFS_COT_INTERROGATED_BY_GROUP_7                  27
#define IECDEFS_COT_INTERROGATED_BY_GROUP_8                  28
#define IECDEFS_COT_INTERROGATED_BY_GROUP_9                  29
#define IECDEFS_COT_INTERROGATED_BY_GROUP_10                 30
#define IECDEFS_COT_INTERROGATED_BY_GROUP_11                 31
#define IECDEFS_COT_INTERROGATED_BY_GROUP_12                 32
#define IECDEFS_COT_INTERROGATED_BY_GROUP_13                 33
#define IECDEFS_COT_INTERROGATED_BY_GROUP_14                 34
#define IECDEFS_COT_INTERROGATED_BY_GROUP_15                 35
#define IECDEFS_COT_INTERROGATED_BY_GROUP_16                 36
#define IECDEFS_COT_REQUESTED_BY_GROUP_GENERAL_COUNTER       37
#define IECDEFS_COT_REQUESTED_BY_GROUP_1_COUNTER             38
#define IECDEFS_COT_REQUESTED_BY_GROUP_2_COUNTER             39
#define IECDEFS_COT_REQUESTED_BY_GROUP_3_COUNTER             40
#define IECDEFS_COT_REQUESTED_BY_GROUP_4_COUNTER             41
#define IECDEFS_COT_UNKNOWN_TYPE_IDENTIFICATION              44
#define IECDEFS_COT_UNKNOWN_CAUSE_OF_TRANSMISSION            45
#define IECDEFS_COT_UNKNOWN_COMMON_ADDRESS_OF_ASDU           46
#define IECDEFS_COT_UNKNOWN_INFO_OBJECT_ADDRESS              47

#define DCO_PERISSION 				0     	/* Double command DCS permission  */
											/* option				 		  */
#define RCO_PERISSION 				0     	/* Regulating step command RCS    */
											/* permission option  			  */
#define IECDEFS_MAX_FRAME_SIZE    1024



//-------------------------------------------------------------------------------
//	for CONFIG.H
//-------------------------------------------------------------------------------
#define CONFG_MAX_NUM_RX	            256
#define CONFG_MAX_NUM_EVENT	            256
#define CONFG_MAX_RESP_FRAGMENT_SIZE    249
#define CONFG_REQ_FRAGMENT_SIZE         255
#define CONFG_PROC_TIMEOUT_MSEC         5000
#define CONFG_CHANNEL_TIMEOUT_MSEC      10000
#define CONFG_MASTER_NUM_RETRY          3
#define CONFG_RX_SEL_TIMEOUT_SEC        10 
#define CONFG_RX_SEL_TIMEOUT_USEC       0
#define CONFG_TX_SEL_TIMEOUT_SEC        10
#define CONFG_TX_SEL_TIMEOUT_USEC       0
#define CONFG_MAX_NUM_CLASS1_INFO       32
#define CONFG_MAX_NUM_CLASS2_INFO       32
#define CONFG_MAX_NUM_MASTER_CMD_QUE    32
#define CONFG_MAX_CLASS_DATA_BYTE       249
#define CONFG_ARM_SELECT_TIME_MSEC      5000
#define CONFG_MMENA_CYCLIC_TIME_MSEC    2500
#define CONFG_MSP_CYCLIC_TIME_MSEC      60000
#define CONFG_COMMAND_PORT				1001
#define CONFG_DEBUG_PORT				1002


/****************************************************************************/
//	for IECOBJ.H
/****************************************************************************/
#define OBJ_DATA_SIZE_CP56TIMEA   			7

#define OBJDEFS_MSP_SIQ_INVALID_MASK      	0x80
#define OBJDEFS_MSP_SIQ_NOT_TOPICAL_MASK  	0x40
#define OBJDEFS_MSP_SIQ_SUBSTITUTED_MASK  	0x20
#define OBJDEFS_MSP_SIQ_BLOCKED_MASK      	0x10
#define OBJDEFS_MSP_SIQ_VALUE_MASK        	0x01
                                          	
#define OBJDEFS_MSP_DIQ_INVALID_MASK      	0x80
#define OBJDEFS_MSP_DIQ_NOT_TOPICAL_MASK  	0x40
#define OBJDEFS_MSP_DIQ_SUBSTITUTED_MASK  	0x20
#define OBJDEFS_MSP_DIQ_BLOCKED_MASK      	0x10
#define OBJDEFS_MSP_DIQ_VALUE_MASK        	0x03

#define OBJDEFS_MMENA_QDS_VALUE_MASK        0x7F
#define OBJDEFS_MMENA_QDS_INVALID_MASK      0x80
#define OBJDEFS_MMENA_QDS_NOT_TOPICAL_MASK  0x40
#define OBJDEFS_MMENA_QDS_SUBSTITUTED_MASK  0x20
#define OBJDEFS_MMENA_QDS_BLOCKED_MASK      0x10
#define OBJDEFS_MMENA_QDS_OVERFLOW_MASK     0x01

/* single point control */
#define OBJDEFS_SCO_SE_MASK   				0x80
#define OBJDEFS_SCO_QU_MASK   				0x7C
#define OBJDEFS_SCO_SCS_MASK  				0x01
#define OBJDEFS_SCO_DCS_MASK  				0x03

#define OBJDEFS_COI_LOCAL_POWER_SW_ON     	0x01
#define OBJDEFS_COI_LOCAL_MANUAL_RESET    	0x02
#define OBJDEFS_COI_REMOTE_RESET          	0x04
#define OBJDEFS_COI_INIT_CHANGE_PARA      	0x80
                                          	
#define OBJDEFS_QOI_INTERROGATED_BY_STATION   20
#define OBJDEFS_QOI_INTERROGATED_BY_GROUP_1   21
#define OBJDEFS_QOI_INTERROGATED_BY_GROUP_2   22
#define OBJDEFS_QOI_INTERROGATED_BY_GROUP_3   23
#define OBJDEFS_QOI_INTERROGATED_BY_GROUP_4   24
#define OBJDEFS_QOI_INTERROGATED_BY_GROUP_5   25
#define OBJDEFS_QOI_INTERROGATED_BY_GROUP_6   26
#define OBJDEFS_QOI_INTERROGATED_BY_GROUP_7   27
#define OBJDEFS_QOI_INTERROGATED_BY_GROUP_8   28
#define OBJDEFS_QOI_INTERROGATED_BY_GROUP_9   29
#define OBJDEFS_QOI_INTERROGATED_BY_GROUP_10  30
#define OBJDEFS_QOI_INTERROGATED_BY_GROUP_11  31
#define OBJDEFS_QOI_INTERROGATED_BY_GROUP_12  32
#define OBJDEFS_QOI_INTERROGATED_BY_GROUP_13  33
#define OBJDEFS_QOI_INTERROGATED_BY_GROUP_14  34
#define OBJDEFS_QOI_INTERROGATED_BY_GROUP_15  35
#define OBJDEFS_QOI_INTERROGATED_BY_GROUP_16  36

#define OBJDEFS_QCC_REQUESTED_BY_GROUP_1_COUNTER        1
#define OBJDEFS_QCC_REQUESTED_BY_GROUP_2_COUNTER        2
#define OBJDEFS_QCC_REQUESTED_BY_GROUP_3_COUNTER        3
#define OBJDEFS_QCC_REQUESTED_BY_GROUP_4_COUNTER        4
#define OBJDEFS_QCC_REQUESTED_BY_GROUP_GENERAL_COUNTER  5

#endif


//-------------------------------------------------------------------------------
//	for IECPHYS.H
//-------------------------------------------------------------------------------
#define PHYS_FIXED_STX    		0x10	/* fixed length first byte		  */
#define PHYS_VARIABLE_STX 		0x68	/* variable length first byte	  */

#define PHYS_ETX 		    	0x16    /* common etx					  */
#define PHYS_FIXED_LENGTH		5		/* fixed type length except STX   */

#define PHYS_BALANCED    	    0 		/* 0:UNBALANCED 1:BALANCED 		  */
#define PHYS_LENGTH_LEN  		2 		/* 2 octet lengh 				  */
#define PHYS_ADDR_LEN    		0   	/* use address len 1 octet 		  */
#define PHYS_COM_ADDR_ASDU 		2   	/* Common address of ASDU length  */
#define PHYS_INFO_OBJ_ADDR 		2   	/* Information Object Address     */
										/* size in byte   				  */

#define LINK_FIXED_LENGTH    		0
#define LINK_VARIABLE_LENGTH		1
#define LINK_HEADER_LENGTH			7
#define LINK_HEADER_POSITION		4

/* primary application control bits */
#define LINK_FCB_RESET_VALUE      	0xF0
#define LINK_CNTR_RES_BIT			0x80
#define LINK_CNTR_PRM_BIT			0x40
#define LINK_CNTR_FCB_BIT     		0x20
#define LINK_CNTR_FCV_BIT     		0x10
#define LINK_CNTR_FUNC_MASK    		0x0F
#define LINK_CNTR_NO_NEXT_FC   		0xFF

#define LINK_CNTR_ACD_BIT			0x20
#define LINK_CNTR_DFC_BIT			0x10


#define	APPL_SQ_MASK             	0x80

/* secondary control bits */
#define LINK_CONTROL_ACD_MASK		0x20
#define LINK_CONTROL_DFC_MASK		0x10

/* 수신 : Primary station Control Byte function codes  */
#define LINK_FUNC_LINK_RESET      	0		/* Reset remote link  			  */
#define LINK_FUNC_RESET_USER      	1   	/* Reset user process 			  */
#define LINK_FUNC_CNFRM_DATA      	3   	/* Send confirm data  			  */
#define LINK_FUNC_UNCONFIRM    		4   	/* Send un-confirm data			  */
#define LINK_FUNC_REQ_ACC_DEM  		8   	/* Request -- 					  */
#define LINK_FUNC_STATUS_OF_LINK  	9   	/* Request link status  		  */
#define LINK_FUNC_REQ_CLASS1 		10  	/* Request class1 data  		  */
#define LINK_FUNC_REQ_CLASS2 		11  	/* Request class2 data  		  */

/* 송신 : Secondary station Control Byte Function Codes  */
#define LINK_FUNC_ACK  				0   	/* Confirm ack 					  */
#define LINK_FUNC_NACK  			1   	/* Confirm no ack 				  */
#define LINK_FUNC_USER_DATA 		8   	/* User data avaliable 			  */
#define LINK_FUNC_USR_DAT_NAV  		9   	/* User data not avaliable 		  */
#define LINK_FUNC_STS_OF_LINK  		11  	/* Status of link 				  */


// for IEC
#define	MAX_NAME_LEN  	20
#define	MAX_EVENT    	128


#define IEC_PT_IV_BIT       0x80        // 단일정보 객체, 유효 비트 
#define IEC_PT_NT_BIT       0x40        // 단일정보 객체, 주제화 비트 
#define IEC_PT_SB_BIT       0x20        // 단일정보 객체, 치환 비트 
#define IEC_PT_BL_BIT       0x10        // 단일정보 객체, 차단 비트 

#define IEC_CNTR_SE_BIT     0x80        // 단일제어 명령, 선택/실행BIT



typedef struct
    {
        byte    start1;
        byte    control;
        byte    address;
        byte    chksum;
        byte    userData[256];

    } __attribute__ ((packed)) IEC_FIX_FRAME;

typedef struct
    {
        byte    start1;
        byte	length1;
        byte	length2;
        byte	start2;
        byte    control;
        byte    address;
        byte    userData[256];

    } __attribute__ ((packed)) IEC_VAR_FRAME;
    
typedef struct
    {
        word    rtuid;
        word    hostid;
        word    rcvCount;
        
        short	frameType;
        short	sndFrameType;
        
        short	reqCount;
        short	rcvFrameOK;
        
        byte	rxcnt;
        byte	rxbuf[256];

        /* ---------------------- */
        IEC_FIX_FRAME   sndFrame1;          // 고정Frame 송신버퍼
        IEC_FIX_FRAME   rcvFrame1;          // 고정Frame 수신버퍼
        
        IEC_VAR_FRAME   sndFrame2;          // 가변Frame 송신버퍼
        IEC_VAR_FRAME   rcvFrame2;          // 가변Frame 수신버퍼
        
        IEC_VAR_FRAME   sndControl;         // 가변Frame 수신버퍼 (제어정보)
        IEC_VAR_FRAME   rcvControl;         // 가변Frame 수신버퍼 (제어정보)
        
    } __attribute__ ((packed)) IEC_DATA_LINK;
    
typedef struct
    {
        short   rcvPos;
        short   rcvSize;

        short   sndPos;
        short   sndSeqno;

        word    desAddr;
        word    srcAddr;

        short   sndCount;
        short   sndFrame;

        byte    userData[MAX_APP_BUFFER];

    } __attribute__ ((packed)) IEC_APP_FRAME;
    

/* structure defining entry in Object Information Table */
typedef struct {
    char	*objDesc;       /* text string describing object variation */
    byte	object;			/* OBJ 번호 구분자 */
    byte	size;          	/* singl data size */
    byte	default_sq;    	/* default SQ */
} IEC_OBJ_TABLE;

