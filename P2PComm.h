//
// Created by jfan on 11/15/2018.
//

#ifndef TFDIR_P2PCOMM_H
#define TFDIR_P2PCOMM_H

#include <vector>
#include <math.h>
#include <string.h>
#include <time.h>
#include <stdint.h>
#include <stdlib.h>
#include <vector>

#include "data_stream.h"
#include "Constants.h"

enum CNT_TYPE {  // Content Categories
    CNT_FAULT = 1,   // 1: Urgent, Switch device's fault information
//    CNT_FLTZONE1,    // 2: Urgent, Notify the switches FltZone by its peer switch, cascade if the target switch is locked
//    CNT_FLTZONE2,    // 3: Urgent, Notify the switches FltZone by its peer switch, do not do cascade
//    CNT_INCFAULT,    // 4: Urgent, Notify neighbor peers for possible HiZ fault
//    CNT_STATUS,      // 5: Urgent, Switch device's open/close and lock/unlock status and availability
//    CNT_CTRL_STATUS, // 6: Urgent, Command to control a switch Open/Open (0/1)
//    CNT_CTRL_TIE,    // 7: Urgent, Command to close a tie switch
//    CNT_TRCE_TIE,    // 8: Urgent, Trace to an available tie to close (for the case of Tie Cap Information not ready)
//    CNT_CTRL_LOCK,   // 9: Urgent, Command to Unlock/Lock(0/1) a switch
//    CNT_CTRL_LOCAL,  //10: Urgent, Command to set a switch to Remote/Local (0/1)
//    CNT_CLEN_TIE,    //11: Normal, Clean Tie Cap Infor for upstream switches after isolating a fault
//    CNT_RQST_PCFG,   //12: Normal, Request Peer config
//    CNT_RPLY_PCFG,   //13: Normal, Reply Peer config
//    CNT_OPR_DATA,    //14: Normal, Switch device's operation data
//    CNT_SRC_CAP,     //15: Normal, Root/source switch's available capacity, forward from root to open-tie
//    CNT_TIE_CAP,     //16: Normal, Tie-switch's available capacity, backward from open-tie to root
//    CNT_TIE_NO,      //17: Normal, No Tie-switch is found from the terminal/peer to its downstream
    CNT_HEART_BEAT = 18  //18: Heart beating message,
};

typedef struct {
    int  IP;         // Switch node IP address
    FLAG SetSeqNum;   // Switch sequence number in the node
} P2P_IP;

typedef struct {  // Message Header
    CNT_TYPE CntType;  // Message content type
    P2P_IP   SenderIP;   // Sending switch IP (immediate sending switch)
    P2P_IP   ReceiverIP; // Immediate Receiving switch device IP
} P2P_HEADER;

typedef struct { // fault information
    FLAG  FltDir;   // 00/01/10/11
//    FLAG  FltPhs;   // Fault phase code
//    float If[3];
} P2P_FAULT;

typedef struct { // Peer config
    int  PeerNodeID;     // Corresponding Peer Switch's Node ID, or an IP address number
    FLAG PeerSrvFlg;     // This Peer's service flag
    FLAG PeerSetSeqNum;  // Corresponding Peer Sensor Set's sequence number
} P2P_CFGPEER;

typedef struct { // Peer switch info
    FLAG  P2PState; // P2P Comm. OK? 0/1 -> no/yes.
    FLAG  FltDir;   // Fault direction: 00/01/10/11 no/FORWARD/BACKWARD/UNKNOWN received from this peer
//    FLAG  FltPhs;   // Fault phase code: 000/001/010/100/011/101/110/111
    FLAG  FltGot;   // Fault information received from this peer -> 0/1/2 No/Yes/TimeOut
    int   LastExec; // ExecCt Mark when the P2P updating received last time for the peer
    int   FltSentMark; // ExecCt Mark when Status/Fault infor was sent
    P2P_CFGPEER Cfg;
} P2P_PEER;

typedef struct { // package for a terminal
//    char Name[NAME_LEN];
    FLAG SeqNum;
    FLAG SrvFlg;
    FLAG FltDir;      // Set's Fault direction
    FLAG FltZone;     // A fault between this switch and its peers (0/1/3/4 -> NoInfo/ConfirmedYes/ConfirmedNo/Cannot Confirm)
    FLAG FltZoneNo;   // Fault is not at the side of this set
    FLAG FltZoneUn;   // Fault is unknown at the side of this set
    FLAG FltReset;    // Reset all P2P Fault infor
    int  FltExecCnt;
    P2P_PEER Peer;
} P2P_SET;

// Message Object
typedef struct {
    FLAG MsgState;      // Message State: 0/1/2 -> No/ReadyForTaking/Taken
    int  ExecCnt;       // For Simulation Test use to delay a message
    P2P_HEADER MsgHead; // Message header
    P2P_FAULT  MsgFlt;  // Message content
} P2P_MSGPACK;

typedef struct {
    FLAG PeerCommFDIR;   // PeerComm available: 0/1 -> no/Yes
    int  NodeID;         // This Node ID or IP
    int  InPnt[2];       // Pointing to the current cell of the urgent incoming buffer for [0]/[1] -> Read/Write
    int  OutPnt[2];      // Pointing to the current cell of the urgent outgoing buffer for [0]/[1] -> Read/Write
    P2P_MSGPACK InPk[P2P_PBUF_SIZE+1];  // Incoming packs
    P2P_MSGPACK OutPk[P2P_PBUF_SIZE+1]; // Outgoing packs
    P2P_SET  Set[MAX_SETS];
} P2P_NODE;

#endif //TFDIRPC_P2PCOMM_H
