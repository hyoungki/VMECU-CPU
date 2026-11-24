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
 
#if 0

/* ANSI C compatible include files */
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "signal.h"
#include "time.h"
#include "errno.h"
#include "locale.h"

/* machine dependent include files */
#ifdef __SVR
#include "netdb.h"
#endif

#ifdef __SUN
#include "sys/param.h"
#endif

#include "unistd.h"
#include "ctype.h"
#include "sys/utsname.h"
#include "sys/types.h"
#include "sys/socket.h"
#include "netinet/in.h"
#include "arpa/inet.h"

#ifndef VXWORKS
#include <memory.h>
#include <sys/ipc.h>
#define KERNEL
#include <sys/msg.h>
#undef KERNEL
#endif

#define OPCODE_LOC  5
#define PASS_OPCODE 0x50

/* define for net port */
#define UNIX_NET_PORT 		7654
#define VXWORKS_NET_PORT 	7653
#define PC_NET_PORT 		7652

#define HOST_NAME 		"ultra"
#define CCU_NAME 		"vx4"
#define PC_NAME 		"pc1"
#define HOST_ADDR 		"192.9.200.6"	/* by TRATEK ... ultra host */
#define NUM_RETRY 		5

#if 0
#define HOST_NAME 		"vms"
#define CCU_NAME 		"vx1"
#define PC_NAME 		"pc1"
#define HOST_ADDR 		"128.10.10.20"	/* by LG-HOST ... vms host */
#define NUM_RETRY 		5
#endif

#endif


/* SERVER 환경에 따른 파라메터  */
//#define SIMULATOR_NET_PORT 	  1234        /* TCPIP - HOST-WISDOM  Port */
#define SIMULATOR_NET_PORT 	  9600        /* TCPIP - HOST-WISDOM  Port */
//#define MONITOR_NET_PORT    2004        /* TCPIP - HOST-Monitor Port */
//#define UDP_SERVER_PORT     2008        /* UDP   - LOCAL Port */
//#define IMAGE_SERVER_PORT   19377       /* TCPIP - IMAGE LOCAL Port */
