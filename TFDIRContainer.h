/*
 * TFDIRContainer.h
 *
 *  Created on: Feb 12, 2018
 *      Author: user
 */
#ifndef TFDIRCONTAINER_H_
#define TFDIRCONTAINER_H_

#include <vector>
#include <math.h>
#include <string.h>

#include <time.h>
#include <stdint.h>
#include <stdlib.h>
#include <vector>

#include "data_stream.h"
#include "phasor.h"
#include "Constants.h"
#include "P2PComm.h"
#include "Pathological.hpp"

#ifndef RUNNING_TEST
	#include "dspDebugLog.h"
    #include "TFDIR_MsgHandler.h"

    /* ---------------------------- DSP/BIOS Headers ---------------------------- */
    #include <std.h>
    #include <log.h>
    #include <swi.h>
    #include <sys.h>
    #include <sio.h>
    #include <tsk.h>
    #include <pool.h>
    #include <gbl.h>

    /*  --------------------------- DSP/BIOS LINK Headers ----------------------- */
    #include <failure.h>
    #include <dsplink.h>
    #include <platform.h>

    #include <ringio.h>
    #include <tsk.h>
    #include <notify.h>

    #include "CircularBuffer.h"
    #include "SharedMemoryBuffer.h"
#endif

#define M_PI        3.14159265358979323846264338327950288

#define SAMPLESperCYCLE 32	// num raw samples per cycle in 60Hz
#define CYCLES          10  // number of cycles of phasors

typedef unsigned short FLAG;

// Time
typedef struct {
    long  sec;	// Seconds
    long  usec;	// Micro Seconds
} TIME;

// xy value struct
typedef struct {
    float x;
    float y;
    TIME  t;            // time stamp of the first sample in the origianl cycle of SV
    FLAG  flg;  		// data quality flag: 0/1/2/3 -> Normal/NoData/Fault/Bad
} XY;

typedef struct {
    float r;
    float x;
} RX;

typedef struct {
    float mag;
    float ang;
    TIME  t;    // time stamp at the position of phase angle
    FLAG  flg;  // data quality flag: 0/1/2/3 -> Normal/NoData/Fault/Bad
} PHASOR;

typedef struct {
    char Name[NAME_LEN];
    float val;
} PARAM;

typedef struct { // full cycle of data of one set
    TIME  t;     // time stamp of the first sample
    TIME phase_t[MES_PHASES]; //timestamp of the first sample per phase
    double phase_st[MES_PHASES]; //timestamp iof the first sample
    long long phase_t_ct[MES_PHASES]; //ct of the samples per phase
    PHASOR IRefPhasor[MES_PHASES]; //latested I phasors that references the per phase time
    PHASOR VRefPhasor[MES_PHASES]; //latested V phasors that references the per phase time
    float I[MES_PHASES][SV_CYC_SIZE]; 	// 3-phase currents
    float V[MES_PHASES][SV_CYC_SIZE]; 	// 3-phase votlages
    float Vr[SV_CYC_SIZE]; 	 // phase reference votlage from an ordinary PT
    FLAG  NumI[MES_PHASES];  // Number of valid samples for 3-phase currents
    FLAG  NumV[MES_PHASES];  // Number of valid samples for 3-phase voltages
    FLAG  NumVr;			 // Number of valid samples for single phase reference voltage
    FLAG  Iflg[MES_PHASES];  // I Data Quality flag
    FLAG  Vflg[MES_PHASES];  // V Data Quality flagvv
    FLAG  Frflg;			 // Vr Data Quality flag
} SMP_SET;

typedef struct { // all sets
    SMP_SET SmpSet[MAX_SETS];
} SMP_NODE;

typedef struct { // Configuration: ICS Sensor
    char  Name[NAME_LEN];		  // Sensor name
    FLAG  SeqNum;				  // Sensor Sequence number: Left/bottom: 1/4/7, Central: 2/5/8, Right/Top: 3/6/9
    FLAG  PhaseID;				  // Phase : A:0001, B:0010, C:0100
//	FLAG  UseCT;				  // Use CT: 0/1: no/yes
    FLAG  UseVT;				  // Use VT: 0/1-10: no/yes
    FLAG  RefVT;				  // VT as voltage reference 0/1: no/yes (priority 1->10: Highest->Lowest
    FLAG  CTPolarity;			  // 0/1: +/- (+: facing bus/node, -: facing line
//  RTUSTA StaPnts[#];			  // RTU Status Points list
//  RTUANL AnlPnts[#];			  // RTU Analog Points list
} CFG_SNS;

typedef struct { // Configuration ICS Set for Smart Tap
    char  Name[NAME_LEN];
    FLAG  SeqNum;				  // Sensor-set Sequence number, e.g., Left: 1; Right: 2; Tap: 3
    FLAG  SrvFlg;				  // The ICS Set installation/service flag 0/1 -> no/yes
    FLAG  VrtFlg;				  // The ICS Set real/virtual flag 0/1 -> real/virtual
    FLAG  SetTyp;				  // The ICS Set for Tapped-off line flag 0/1 -> TransLine/TapLine
    FLAG  CfgZone;                // Configured fault zone based on Tap location in the line: 0/NA,1,2,3,4
    FLAG  UseCfgZone;             // 0/1/2/3: Use FltZone/CfgZone /CfgZone+FltZone/CfgZone+FltZone(when FltZone=2 only)
    FLAG  SwitchEnabled;		  // The corresponding Set Switch is enable for operation 0/1 - > No/Yes
    FLAG  PeerSrvFlg;             // This set's Comm. Peer service flag
    FLAG  PeerSetSeqNum;          // Corresponding Peer's Set sequence number
    float PeerNodeID;             // Corresponding Peer's Node ID, or an IP address number
    RX	  Z0;					  // Zero sequence impedance of the line segment, Ohm/mile
    RX	  Z1;					  // Positive/Negative sequence impedance of the line, Ohm/unit
    RX	  Zs0;					  // Zero sequence impedance of the equivalent source
    RX	  Zs1;					  // Positive/Negative sequence impedance of the equivalent source
    float AmpSclFactor;			  // Amp Scale factor to actual engineering value from what measured
    float AmpAngOffset;			  // Amp angle offset (0/+/-: none/Add/Subtract to/from the angle measured
    float VoltSclFactor;		  // Amp Scale factor to actual engineering value from what measured
    float VoltAngOffset;		  // Amp angle offset (0/+/-: none/Add/Subtract to/from the angle measured
    float CTRatio;				  // CT Ratio
    float CTAngOffset;			  // CT angle offset (0/+/-: none/Add/Subtract to/from the angle measured
    float LineLength;			  // Line length in a unit (mile or km) from the node to the source station
    float TapSegLength;			  // Tap segment length to the next Tap
    float ZoneSetting[2];		  // Zone setting in persentage of segment lentgh(e.g., Z1: 60%, Z2: 130%)
    float Irated;				  // Rated phase current in Amp
    float Inrated;				  // Rated neutral current
    float Ifm;					  // Phase fault setting
    float Ifnm;					  // Neutral fault setting
    float Ifcm;					  // Delta change fault setting
    float Ifdb;					  // Dead band of phase fault current, if I<Ipfm back to normal
    float Ifndb;				  // Dead band of neutral fault current, if I<Infm back to normal
    CFG_SNS CfgSns[MES_PHASES];
} CFG_SET;

typedef struct { // Configuration ICS Node
    float NodeID;                 // Node ID
    FLAG  PeerCommFDIR;           // Peer Comm. Off/On: 0/1
    //	FLAG  PoleConfig;			  // Pole top configuration: 1/2/3/... Horizontal/Vertical/Triangle/...
    FLAG  RptPermFlt;             // Disable/Enable permenent fault report: 0/1
    FLAG  RptTempFlt;			  // Disable/Enable temporary fault report: 0/1, with breaker trip, reclose succesful
    FLAG  RptMmtEvt;			  // Disable/Enable Momentary Event report: 0/1 for Momentary fault, no breaker trip
//	FLAG  RptOpr;				  // Disable/Enable Operation Report 0/1
    FLAG  AutoSwitchOpen;		  // Automaticly open switch? 0/1: no/yes, open in T-FDIR
    FLAG  MaxRclNum;			  // Maximum reclosing time before locking out, e.g., 3 times
    FLAG  TfdirRclNum;            // Number of reclosing fails for TFDIR to take action (0 or 1)
    float Da;					  // Distance of Phase A sensor to ground in meters
    float Db;					  // Distance of Phase B sensor to ground
    float Dc;					  // Distance of Phase C sensor to ground
    float Dab;					  // Distance of Phase A to Phase B sensor
    float Dbc;					  // Distance of Phase B to Phase C sensor
    float Dca;					  // Distance of Phase C to Phase A sensor
    float daa;					  // Sensor/Conductor diameter in meters
    float PCTfact;				  // Phase coupling tuning factor: 0.0 - 1.0;
    float SysFreq;                // Configured System frequency
    float RclWaitTime;			  // In seconds to wait for Recloser to close again after tripping
    float HotTime;				  // In seconds to confirm line is successfully hot after breaker close/reclose
    float DeadTime;				  // In seconds to confirm line is dead after lossing 3-currents
    float FltTime;				  // In seconds to confirm a fault for too long without breaker tripping
    float RtnTime;				  // In seconds to confirm line return to normal after a temporary fault
//	float RptOprTime;			  // The minimum time interval to report operation data, V,I,P,Q
    float MTA;					  // Max Torque Angle (-90 to +90 degrees leading voltage, usually -15)
    float TAW;					  // Torque Angle Width (0 - 180 degrees, centralized at MTA, usually 150)
    float Vrated;             	  // Rated votlage (L-L)
    float Vmin;          	      // Low limit of voltage
    float Vmax;    	    	      // High limit of voltage
    float Imin;					  // Low limit of current magnitude, I less than this limit is treated as zero
    CFG_SET CfgSet[MAX_SETS];
}CFG_NODE;

typedef struct {				  // Fault history in reclosing cycles
    XY     Ia[MES_PHASES];		  // Prefault current average phasor
    XY     Va[MES_PHASES];        // Prefault voltage average phasor
    RX	   Zs0;					  // Equivalent zero sequence source impedance at Tap
    RX	   Zs2;					  // Equivalent negative sequence source impedance at Tap
    PHASOR If[MES_PHASES];		  // Fault currents in 3 phases
    PHASOR Ifc[MES_PHASES];	 	  // Delta Fault currents in 3 phases
    PHASOR Vf[MES_PHASES];		  // Fault voltages in 3 phases
    PHASOR Vref;				  // Reference voltage
    PHASOR I0; 					  // Zero sequence current phasor
    PHASOR I2; 					  // Negative sequence current phasor
    PHASOR V0; 					  // Zero sequence voltage phasor
    PHASOR V2; 					  // Negative sequence voltage phasor
    float  FltDist;				  // Estimated fault distance based on I0/I2 (double end model)
    FLAG   FltZone;				  // Fault zone: 0/1/2/3: no/Zone1/Zone2/Zone3
    FLAG   FltDir[MES_PHASES];	  // Fault direction 00/01/10/11: no/+/-/unknown
    FLAG   FltDirSum;			  // Summary Fault direction 00/01/10/11: no/+/-/unknown
    FLAG   FltFlg;				  // Set A:001, B:010, C:100 if the phase is faulted: 0/1 no/yes
    FLAG   FltTyp;	   			  // Set 0/1/2/4/6/7: no/ChgFlt/NutrFault/PhsFlt/BothFlt/All
    FLAG   EvtTyp;				  // Fault event type, No/Permernent/Temprorary/Momentary 0/1/2/3
}EVT_LOG;

typedef struct {				  // ICS Sensor based measurement vector values
    XY     Ibuf[BUF_SIZE+1];  	  // I buffer (15 cycles, last 10 cycles for pre-fault, first 3 cycles for fault avg)
    XY     Vbuf[BUF_SIZE+1]; 	  // V buffer (???)
// Calculated Phasors from the I/V buffs:
    XY     Ia;					  // Avg Current in x,y, calculated from the 4 - 14 cycles
    XY     Va;					  // Avg Voltage in x,y, calculated from 4 - 14 cycles
    PHASOR Iist;				  // Instaneous Current Vector of the most recent cycle
    PHASOR Ifa; 			      // Flt Avg Current Vector calculated from the first 3 cycles in Ibuf
    PHASOR Ifc;                   // Delta change between instaneous (or flt avg) current to the normal avg.
    PHASOR Vist;			      // Instant Avg Voltage Vector from the current or historial instaneous value of a normal cycle
    PHASOR Vref;				  // V reference from this phase
// Calculated values:
    float  p;                     // Real Power
    float  q;                     // Reactive Power
    float  Vmdcf;				  // Voltage magnitude dynamic correction factor
    float  Vadcf;				  // Voltage angle dynamic correction factor
// Working variables:
    FLAG Phase;		 // Corrected phase ID (001/010/100 -> A/B/C)
    FLAG FltDir;		 // Fault direction 00/01/10/11 (0/1/2/3) -> no/+/-/unknown
//	FLAG PhsFlg0;		 // Previous line phase flag
//	FLAG PhsFlg;         // Line phase dead/alive flag 0/1: dead (no current)/alive (with current)
    FLAG VrefFlg;		 // Reference voltage phase flag: 0/1-> no/yes
    FLAG IaRdyFlg;       // 0/1: not/ready (Avg I buff is ready with 5 - 10 valid values/cycles)
    FLAG IfRdyFlg;	     // 0/1: not/ready (Fault is confirmed with 3 valid values/cycles)
    FLAG FltTyp0;		 // Previous fault flag
    FLAG FltTyp;	     // Set 0/1/2/4/6/7: no/ChgFlt/NutrFault/PhsFlt/BothFlt/All
// Counters:
    FLAG IaCnt;			 // Number of valid values in the first 10 values of Ibuf
    FLAG VaCnt;   	     // Number of valid values in the 10 cycles
    FLAG NumApnt;		 // Number of actual analog points
    FLAG NumSpnt;		 // Number of actual status points
// Configuration parameters
    CFG_SNS *Cfg;		 // pointer to ICS Sensor level configuration parameters
} ICS_SNS;

typedef struct {
    float   IfTimer;			  // Phase Fault detected time counter
    float	IfRtnTimer;			  // Return to normal from fault time counter
    float   IfnTimer;			  // Neutral fault detected time counter
    float	IfnRtnTimer;		  // Neutral fault return to normal timer
    float	IfcTimer;			  // Delta Change fault detected time counter
    float   LineFlgTimer;         // Timer for confirming line dead
    float   p;                    // 3-phase total Real Power
    float   q;                    // 3-phase total Reactive Power
    float   pf;                   // 3-phase Power factor
    PHASOR  I0;            	      // Avg Current Zero Sequence Vector as 3I0
    PHASOR  VmIcs[MES_PHASES];	  // Measured voltage before applying the voltage model matrix for correction.
    FLAG    IistFlg;              // I > Imin indicator at any phase of the set
    FLAG    LineFlg0;        	  // Previous Line flag
    FLAG    LineFlg;        	  // Set A:001, B:010, C:100 if the phase no data or sensor/line dead/alive 0/1
    FLAG    FltFlg0;			  // Previous fault flag
    FLAG    FltFlg;				  // Set A:001, B:010, C:100 if the phase is faulted: 0/1 no/yes
    FLAG	ChgFlg;				  // Set A:001, B:010, C:100 if the phase is identified as the faulted: 0/1 no/yes
    FLAG	NtrFlg;				  // Set A:001, B:010, C:100 if the phase is identified as the faulted: 0/1 no/yes
    FLAG	FltTyp0;			  // Previous fault type
    FLAG    FltTyp;	  			  // Fault type: Set 0/1/2/4/6/7: no/ChgFlt/NutrFault/PhsFlt/BothFlt/All
    FLAG	TypLch;				  // Set 0/1/2/4/6/7: no/ChgFlt/NutrFault/PhsFlt/BothFlt/All
    FLAG	FltLch;				  // Set A:001, B:010, C:100 if the phase is faulted: 0/1 no/yes
    FLAG	ChgLch;				  // Set A:001, B:010, C:100 if the phase is faulted: 0/1 no/yes
    FLAG	NtrLch;				  // Set A:001, B:010, C:100 if the phase is faulted: 0/1 no/yes
    FLAG	FltCnt;				  // Count Ifa average
    FLAG	NtrCnt;				  // Count consecutive times of Neutral changes
    FLAG	ChgCnt;				  // Count consecutive times of Delta changes
    FLAG	OpenSw;				  // Command to open the switch 0/1/2 No/Open/Opened
    FLAG	FltZone;			  // Resultant fault zone
//	FLAG	HisCnt;				  // Counts of history fault events
//	FLAG    NumApnt;			  // Number of active analog points
//	FLAG    NumSpnt;			  // Number of active status points
// Operation Arrays
    ICS_SNS  IcsSns[MES_PHASES];  // 3-phases of I and V vectors, including the calculated neutral current
    EVT_LOG  Evt[EVT_SIZE];		  // Fault event log: 5 records per event
// Configuration parameters
    CFG_SET *Cfg;				  // pointer to ICS Sensor level configuration parameters
} ICS_SET;

typedef struct {
    float  SinV[SV_CYC_SIZE]; // Sin[] values fro 80 sample points, populated at startup
    float  CosV[SV_CYC_SIZE]; // Sin[] values fro 80 sample points, populated at startup
    float  SysFreq;			  // System dynamic frequency
    float  Ms2Degrees;        // 0.36*(SYSFREQ) Milliseconds to degrees
    float  Degrees2Ms;        // 1.0/(MS2DEGREES) Degrees to milliseconds
    float  Cycle2Sec;         // 1.0/(SYSFREQ) Cycle to seconds
    float  DnTimer;			  // All line down/dead timer in seconds
    float  UpTimer;			  // All line up/alive timer in seconds
//	float  RptOprTimer;	      // Report normal operation data, V, I, P, Q
    float  ToqAngLead;		  // Torque Angle leading the phase voltage: e.g., +65 degrees
    float  ToqAngLag;		  // Torque Angle lagging the phase voltage: e.g., -85 degrees
    float  VoltMtrx[MES_PHASES][MES_PHASES]; // Voltage model matrix (V = M*v, V/v: LineV/MeasuredV)
    float  VmPT;			  // Measured phase voltage from single phase PT, default to Vrated/sqrt(3.0)
    PHASOR Vmref[MES_PHASES]; // Voltage reference model for single PT based dynamic calibration
    PHASOR Vref;			  // Reference votlage vector for fult direction, normalized to phase
    FLAG   VoltSetNum;		  // The set number whose voltages are actually used for distance calulation
    FLAG   NumActSets;		  // Number of actual ICS sets
    FLAG   NumMesSets;		  // Number of actual ICS sets with sensors installed
    FLAG   LchFlg;			  // Fault latch ready flag
    FLAG   LchTyp;            // Fault type latched
    FLAG   TrpCnt;			  // Trip counter since a fault first detected
    FLAG   RclCnt;			  // Reclosing counter since a fault trip
    FLAG   FltLck;			  // Permenent fault locked if set
    FLAG   Flt2Long;		  // Fault too long flag
    FLAG   CtrlSusp;		  // Suspend control due to too low current or voltage
    FLAG   EvtTrigger;		  // A fault condition detected to trigger the DFR when it is set
//	FLAG   RptOpr;            // Report Operation if set
//	FLAG   NumApt;			  // Number of active analog points
//	FLAG   NumSpt;			  // Number of active status points
    ICS_SET IcsSet[MAX_SETS]; // a ICS node for STC can have 3 ICS sets (2 reals, 1 virtual)
// Configuration parameters
    CFG_NODE *Cfg; 			  // pointer to ICS Sensor level configuration parameters
} ICS_NODE;

typedef struct {
    float I[MAX_SETS][MES_PHASES][SAMPLESperCYCLE+1]; 	// 3 sensors' 3-phase currents for 2 cycles
    float V[MAX_SETS][MES_PHASES][SAMPLESperCYCLE+1]; 	// 3 sensors' 3-phase votlages for 2 cycles
} CIRC_SAMPLES;

typedef struct {
    XY I[MAX_SETS][MES_PHASES][CYCLES];
    XY V[MAX_SETS][MES_PHASES][CYCLES];
} PHASORS;

typedef struct
{
    float			currentNow;
    float			current0_5msAgo;
    float			voltageNow;
    float			voltage0_5msAgo;
} InterpolatedData;

typedef struct
{
    int set;
    int phase;
} SensorIdentifier;

typedef struct
{
    SensorIdentifier sensor;
    long long timeCt;
} ExecutionStamp;

using namespace std;

#ifndef RUNNING_TEST
using namespace MityDSP;
#endif

class TFDIRContainer {
public:
    TIME  Time;

    int XY_Size, PHASOR_Size;
    float Ir, Vr, Ir_inv, Vr_inv;
    bool configLoaded;
    bool inSimulationMode;
    int executionCt;

    ICS_NODE IcsNode;
    CFG_NODE CfgNode;
    SMP_NODE SmpNode;
    P2P_NODE P2PNode;

    Pathological *pathological;
    OPERATINGMODE operatingMode;
//    CONTROLMODE controlMode;

    CIRC_SAMPLES CircSamples;
    PHASORS PhasorsObj;

#ifndef RUNNING_TEST
    TFDIRContainer(CircularBuffer *circularBuf);
#else
    TFDIRContainer();
#endif

    virtual ~TFDIRContainer();

    PHASOR XY2PhasorI(XY *xy, int i);
    PHASOR XY2PhasorV(XY *xy, int i, int j);
    float XY2Mag(float x, float y);
    float XY2Ang(float x, float y);
    float MagAng2X(float mag, float ang);
    float MagAng2Y(float mag, float ang);
    void MagAng2XY(float *x, float *y, float *mag, float *ang);
    RX VectAdd(RX* Z1, RX* Z2);
    RX VectSub(RX* Z1, RX* Z2);
    RX VectX2(RX* Z1, RX* Z2);
    RX VectX3(RX* Z1, RX* Z2, RX* Z3);
    RX VectDiv(RX* Z1, RX* Z2);
    void abcxy2s0(PHASOR *S, XY *A, XY *B, XY *C);
    void abc2s0(PHASOR *S, PHASOR *A, PHASOR *B, PHASOR *C);
    void abc2s1(PHASOR *S, PHASOR *A, PHASOR *B, PHASOR *C);
    void abc2s2(PHASOR *S, PHASOR *A, PHASOR *B, PHASOR *C);
    void abc2s012(PHASOR *Seq012, PHASOR *ABC);
    void PhasorAdd(PHASOR *S, PHASOR *A, PHASOR *B);
    void PhasorDiff(PHASOR *D, PHASOR *A, PHASOR *B);
    PHASOR PhasorShift(PHASOR *A, float Ang);
    void VI2PQ(float *P, float *Q, PHASOR *V, PHASOR *I);
    float TimeDiff (TIME *t1, TIME *t2);
    void fft(float *X, float *Y, float *C, float *S, float *SV, int K);
    void fft_ReversedSmp(float *X, float *Y, float *C, float *S, float *SV, int K); //sample prep puts the sample in reverse order for history
    int MatInv(float *b, float *a, int n);

#ifndef RUNNING_TEST
    void addToSampleBuffer(DataStream& setDataStream, int set);
    void addNewSample(DataStream& setDataStream, int set, int phase);
    void addNewSampleSinglePhase(DataStream& setDataStream, int set, int phase);
    void addNewSampleSinglePhaseTimeClamped(DataStream& setDataStream, int set, int phase);
#endif
    int Configure(FLAG SetOption, char *configContents);
    void calcPhasors(int set, int phase);

    void CalibVolt();

    //test samples
    void genRawSamples(); //testing only
    void genRawSamplesHardCoded();

    int loadSimulation(char *simulationData);

    void SetAvg();
    void SetFault();
    void DetectFault();
    int TakeAction ();
    void resetAllBuffers();

    int  GetPeersMsgs();
    int  SendPeersMsgs();
    void InitP2PCfg();
    void P2PFaultScheme();
    void GetOprData();
    void FromPeer();
    void ToPeer();

#ifdef RUNNING_TEST
    int GetSamples(int CallFlag);
    int GetSamplesReversed(int CallFlag);
#endif

private:
    int EVT_LOG_Size;
    int EVT_Total_Size;

    PHASOR abc[MES_PHASES];

#ifndef PC_DEBUG
    TFDIR_MsgHandler tfdirMsgHandler;
#endif

    DataStream prevSample[2][MAX_SETS][MES_PHASES];
    InterpolatedData interpolatedData;
    PHASOR iPhasor[3], vPhasor[3];
    double iPhasor_refTime[3], vPhasor_refTime[3];
    DataType filledData;
    DataStream filledSample;
    bool isInitialSample[MAX_SETS][MES_PHASES];
    int prevSmpAway;
    int DataStreamSize;
    int floatSize;
    int ILimit, VLimit;
    double smpTimeResolution;
    double cycleDuration;
    double degreesPerSec;
    double radiansPerSec;
    double prevSmpTimePerPhase[MAX_SETS][MES_PHASES];
    int setsPhasesTotalSampleCt[MAX_SETS][MES_PHASES];
    int neededSetsPhasesTotalSampleCt;

    DataStream sampleBuffer[MAX_SETS][MES_PHASES][4];
    int sampleBufferCounter[MAX_SETS][MES_PHASES];

    ExecutionStamp prevExecution;
    SensorIdentifier initialRefSensor;

#ifndef RUNNING_TEST
    CircularBuffer *gCircularBuf;
    vector<SharedMemoryBuffer*> sharedMemList;
    vector<SharedMemoryBuffer*>::iterator sharedMemIt;
#endif

//	SharedMemoryBuffer* sharedMemory;
    //ConfigVar
    int NUM_NODE_PARAMS;
    int NUM_SET_PARAMS;
    int NUM_SNS_PARAMS;

    enum PARSE_MODES{
        NODE=0,
        SET,
        SENSOR
    };

    FLAG **nodeFlagMap;
    float **nodeFloatMap;
    char **nodeFlagParams;
    char **nodeFloatParams;
    char **setFlagParams;
    char **setFloatParams;
    char **sensorFlagParams;

    int *nodeFound;
    int *setFound;
    int *snsFound;

    char **configContentsArray;

    char msgCharBuffer[MSG_SIZE];
    int P2P_MSGPACK_Size;

    char simline[LINE_READ_BUFFER];
    DataStream simDataStream_a;
    DataStream simDataStream_b;
    DataStream simDataStream_c;

    int sampleSeq;

    void Initialize();

    int parseConfig(char *configContents);

    void populateParam(char *line, PARAM *param);
    int  populateNode(PARAM param);
    void populateSet(PARAM param, CFG_SET *Set);
    void populateSns(PARAM param, CFG_SNS *Sensor);
    int checkNode();
    int checkSet();
    int checkSns();
    int VoltModel();

    static bool sortBufferByTime(DataStream &data1, DataStream &data2);
    int numSamplesAway(double time, double timeRef, double timeRange);
    void setSmpSetValues(int set, int phase, int idx, float i, float v, bool countSample);
    bool validateData(DataStream dataStream);
    void checkCycleComplete(int numSamplesAdded, int set, int phase);

    void processSmpSetData(DataStream& setDataStream, int set, int idx);
    void interpolateData(DataType& curData, DataType& prevData, double curTime, double interTime, double prevTime, InterpolatedData& interpolatedData);
    void interpolateData(DataType& curData, DataType& prevData, DataStream& prevPrevSample, double curTime, double interTime, double prevTime, InterpolatedData& interpolatedData);

    void setSimDataStream(DataStream & dataStream, int id, uint32_t secs, uint32_t nsecs, int i, int i_05ms, int v, int v_05ms);

    void sendBufferedDataToArm();

    void executeTFDIR(int set, int phase);

    void CalDist(FLAG LchFlg);

    int EvtTrigger ();

};

#endif /* TFDIRCONTAINER_H_ */