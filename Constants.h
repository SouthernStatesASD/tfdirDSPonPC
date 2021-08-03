//
//  Data: 11-01-2014
//  Author: Jiyuan Fan
//
#ifndef CONSTANTS_H_
#define CONSTANTS_H_

#include <vector>
#include <math.h>
#include <string.h>
#include <sys/time.h>
#include <stdint.h>
#include <stdlib.h>
#include <vector>

#define PC_DEBUG		// PC Version Debugging
//#define PC_DEBUG_STEP
#define RUNNING_TEST

#define NAME_LEN        20      // Name length
#define EVT_SIZE  	     7		// Fault history event size
#define MAX_SETS         3      // Plus the calculated 3rd set for T-Tap
#define MES_PHASES       3      // Measured Number of phases
#define CAL_PHASES       4      // 3 + Calculated neutral phase
#define MES_TYPES        2      // V and I
#define SV_CYC_SIZE    167      // ATP Entergy SVs/Cycle
//#define SV_CYC_SIZE     32      // ICS SVs/Cycle
#define BUF_SIZE		 8      // 7 Cycles of vectors
#define AVG_BUF_SIZE	 4      // Average of 5 cycles for normal operation
#define FLT_BUF_SIZE	 3      // Average of first 3 cycles from fault to pick the best fault cycle

#define PHASOR_TYPES     3      // 3 types of phasor values: [0]/[1]/[2]: Origianl/Normalized/Zero-crossing

#define PHASOR_DS_SAMPLES	6;  //6 samples

#define PI2DEGREES	   180.0/3.141592654
#define DEGREES2PI	   3.141592654/180.0

#define LINE_READ_BUFFER 100

#define P2P_PBUF_SIZE  100 // P2P package buff size
#define P2PCOMM_LOST   900 // Set P2P Comm State as LOST (15 seconds without receiving a message from a peer)

//#define CFG_FILE_PATH "C:/Users/jfan/Workspace_v6.0/TFDIR/Cfg/xGTCsim.x"
//#define CFG_FILE_PATH "C:/Users/jfan/Workspace_v6.0/TFDIR/Cfg/xAEPhuff.x"
//#define CFG_FILE_PATH "C:/Users/jfan/Workspace_v6.0/TFDIR/Cfg/xAEPhan.x"
//#define CFG_FILE_PATH "C:/Users/jfan/Workspace_v6.0/TFDIR/Cfg/xFirstEnergy.x"
//#define CFG_FILE_PATH   "C:/Users/jfan/Workspace_v6.0/TFDIR/Cfg/xDom_F.x"
#define CFG_FILE_PATH   "/home/jiyuan/Documents/Tconfigs/Line27LongCreak.txt"

typedef unsigned short FLAG;
typedef timeval TIME;

// Enumerations
enum PHASES {
    PHASE_NO = 0,
    PHASE_A,
    PHASE_B,
    PHASE_AB,
    PHASE_C = 4,
    PHASE_CA,
    PHASE_BC,
    PHASE_ABC = 7,
    PHASE_N
};
enum DATAQUALITY {
    DATA_NORMAL = 0,
    DATA_NO,          // no data, either real zero or line dead or sensor dead
    DATA_BAD,         // data bad or invalid, but line is hot
    DATA_FAULT		  // fault detected
};
enum AVERAGEREADY {
    AVERAGE_NO = 0,
    AVERAGE_1,
    AVERAGE_2,
    AVERAGE_READY
};
enum FAULTTYPES {	  // 0/1/2/3/4/7: no/PhsFlt/NutrFault/BothFault/ChgFlt/AllFlt
    FAULT_NO = 0,	  // 000
    FAULT_PHASE,	  // 001
    FAULT_NEUTRAL,    // 010
    FAULT_CHANGE = 4, // 100
    FAULT_ALL	      // 111
};
enum FAULTDIRECTION { // 0/1/2/3 (00/01/10/11) -> no/+/-/unknown
    FAULT_FORWARD = 1,// 01 +
    FAULT_BACKWARD,   // 10
    FAULT_UNKNOWN     // 11
};
enum EVENTTYPES {	  // 0/1/2/3/4 no/Permenent/Temorary/Both/Momentary fault
    EVENT_NO = 0,	  // 000
    EVENT_PERMANENT,  // 001
    EVENT_TEMPORARY,  // 010
    EVENT_ALL,		  // 011
    EVENT_MOMENTARY,  // 100
    EVENT_FAULT2LONG  // 101
};
enum SETTYPES {	  // 0/1/2/3/4 TransLine/TapLine/PTSet_Mag+Ang/ExtVoltMagOnly/...
    SET_TYPE_LINE = 0,	  // 0
    SET_TYPE_TAP,		  // 1
    SET_TYPE_VMA,		  // 2
    SET_TYPE_VM			  // 3
};

// Single Bit assigns
#define Isbitset(c,l)          (((c)&(l))!=0)          // is a bit set ?
#define Setbit(c,l)            ((c)|=(l)) 			   // set bit
#define Clearbit(c,l)          ((c)&=~(1 << (l-1)))    // clear a bit

// Bit mask checks
// #define ISmask(c,l)         (((c)&(l))==(l))            // all bit(s) set
// #define ISinmask(c,l,m)     (((c)&(l))==(m))            // some bit(s) set
// #define ISclear(c,l)        (((c)&(l))==0)              // bit(s) cleared
// #define INrange(c,l,r)      (((c)>=(l))&&((c)<=(r)))    // value within range
// #define ISodd(c)/            isset((c),B0)              // bit 0 set
// #define ISeven(c)           isclear((c),B0)             // bit 0 cleared
// #define ISnext(c,l)         ((c)==(l+1))                // sucessor
// #define ISprevious(c,l)     ((c)==(l-1))                // antecessor

// Bit mask assigns
// #define Setbits(c,l)        ((c)|=(l))                  // bit(s) set
// #define Clearbits(c,l)      ((c)&=~(l))                 // bit(s) cleared
// #define Toggle(c,l)         ((c)^=(l))                  // bit(s) changed

#endif

/****************** end of file **************************/
