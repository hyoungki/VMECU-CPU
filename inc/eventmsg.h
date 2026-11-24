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

//#define NUMBER_OF_EVENT     (0x30 + 2)

byte    errmsg[NUMBER_OF_EVENT][20] = {
        " ------------- ",      /* id = 0x00 */
        "*CU Restart    ",      /* id = 0x01 */
        "*ROMDB Fail    ",      /* id = 0x02 */
        "*Reset by USER ",      /* id = 0x03 */
        "*Reset by WDT  ",      /* id = 0x04 */
        "MASTER-CPU ACT ",      /* id = 0x05 */
        "SLAVE-CPU ACT  ",      /* id = 0x06 */
        "Change SYSTEM-A",      /* id = 0x07 */
        "Change SYSTEM-B",      /* id = 0x08 */
        "Change AUTO    ",      /* id = 0x09 */
        "Change MANUAL  ",      /* id = 0x0a */
        "Local Time-SET ",      /* id = 0x0b */
        "SIM   Time-SET ",      /* id = 0x0c */
        "*Change CPU-SET",      /* id = 0x0d */
        "*Host WDT-Reset",      /* id = 0x0e */
        "*ALL-LOCK Cntr ",      /* id = 0x0f */
        
        "HOST Online    ",      /* id = 0x10 */
        "*HOST Offline  ",      /* id = 0x11 */
        "HOST Time-SET  ",      /* id = 0x12 */
        "Device Online  ",      /* id = 0x13 */
        "*Device Offline",      /* id = 0x14 */
        "MMI Online     ",      /* id = 0x15 */
        "*MMI Offline   ",      /* id = 0x16 */
        "SCU Online     ",      /* id = 0x17 */
        "*SCU Offline   ",      /* id = 0x18 */
        "LINK Online    ",      /* id = 0x19 */
        "*LINK Offline  ",      /* id = 0x1a */
        "*         ",           /* id = 0x1b */
        "*         ",           /* id = 0x1c */
        "*         ",           /* id = 0x1d */
        "*         ",           /* id = 0x1e */
        "*         ",           /* id = 0x1f */
        
        "SOE Event      ",      /* id = 0x20 */
        "HOST Control   ",      /* id = 0x21 */
        "LOCAL Control  ",      /* id = 0x22 */
        "IED-Cnt Select ",      /* id = 0x23 */
        "IED-Cnt Operate",      /* id = 0x24 */
        "*SCAN-DB Change",      /* id = 0x25 */
        "*MOD-DB Change ",      /* id = 0x26 */
        "*HOST-DB Change",      /* id = 0x27 */
        "*         ",           /* id = 0x28 */
        "*         ",           /* id = 0x29 */
        "*         ",           /* id = 0x2a */
        "*         ",           /* id = 0x2b */
        "*         ",           /* id = 0x2c */
        "*         ",           /* id = 0x2d */
        "*         ",           /* id = 0x2e */
        // hkkim , 추가 
        "*         ",             /* id = 0x2f */
        };
