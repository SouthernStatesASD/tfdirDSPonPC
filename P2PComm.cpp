/*
 * P2PComm.cpp
 * This is for TFDIR
 *  Created on: May 13. 24, 2019
 *      Author: Jiyuan Fan
 */
#include <stdio.h>
#include "TFDIRContainer.h"
//#include "P2PComm.h"
#include <algorithm>
#include <assert.h>

#ifdef RUNNING_TEST
    #include "Comtrade.h"
#endif

int TFDIRContainer::GetPeersMsgs() {
    int error = 0;

    if (!P2PNode.PeerCommFDIR)
        return 0;

    if (pathological->peersMsgs.msgWriteIdx != pathological->peersMsgs.msgProcessedIdx) {
        //there're unprocessed msgs
        int numMsgs = pathological->peersMsgs.msgWriteIdx - pathological->peersMsgs.msgProcessedIdx;
        if (numMsgs < 0)
            numMsgs += MAX_PEER_MSGS;

//        printf("Has new peer msgs: %d, %d/%d\n", numMsgs, pathological->peersMsgs.msgWriteIdx, pathological->peersMsgs.msgProcessedIdx);

        PEER_MSG *curPeerMsg;
        for (int i=0; i<numMsgs; i++) {
            curPeerMsg = &pathological->peersMsgs.msgs[pathological->peersMsgs.msgProcessedIdx];

//            curPeerMsg->cyclesWaited++;
//            if (curPeerMsg->cyclesWaited > 30) {
            //don't process the msgs until 30 cycles later

            P2P_MSGPACK recvMsgPack;
            memset(&recvMsgPack, 0, P2P_MSGPACK_Size);
            memcpy(&recvMsgPack, curPeerMsg->msgBuffer, P2P_MSGPACK_Size);

            recvMsgPack.ExecCnt = executionCt + 2*i;  // For Simulation Test use

            if (recvMsgPack.MsgHead.ReceiverIP.IP == P2PNode.NodeID) {
// Received an urgent message
                if (recvMsgPack.MsgHead.CntType == CNT_FAULT)
                    printf("Received Urgent MsgType:%d, from Node:%d to ThisNode%d, Time:%d/%d\n",
                       recvMsgPack.MsgHead.CntType, recvMsgPack.MsgHead.SenderIP.IP,
                       recvMsgPack.MsgHead.ReceiverIP.IP, Time.tv_sec, Time.tv_usec);
                else
                    printf("Received Normal MsgType:%d, from Node:%d to ThisNode%d, Time:%d/%d\n",
                           recvMsgPack.MsgHead.CntType, recvMsgPack.MsgHead.SenderIP.IP,
                           recvMsgPack.MsgHead.ReceiverIP.IP, Time.tv_sec, Time.tv_usec);

//                printf("Number: %d, \n", P2PNode.UgtInPnt[1]);

                P2PNode.InPk[P2PNode.InPnt[1]] = recvMsgPack;
                P2PNode.InPnt[1]++; //increment the urgent write idx
                if (P2PNode.InPnt[1] >= P2P_PBUF_SIZE)
                    P2PNode.InPnt[1] = 0;
            }

            curPeerMsg->needToProcess = false;

            pathological->peersMsgs.msgProcessedIdx++;
            if (pathological->peersMsgs.msgProcessedIdx > MAX_PEER_MSGS) {
                pathological->peersMsgs.msgProcessedIdx = 0;
            }
        }

        FromPeer();
    }

    return error;
}

int TFDIRContainer::SendPeersMsgs() {
    int error = 0;

    if (!P2PNode.PeerCommFDIR)
        return 0;

    ToPeer();
//    if (ExecCnt%(P2PCOMM_LOST/2) == 0)
//        ToPeerNom();

    //check for urgent msg to be sent
    if (P2PNode.OutPnt[0] != P2PNode.OutPnt[1]) {
        //there are urgent msgs we need to send out
        int numMsgs = P2PNode.OutPnt[1] - P2PNode.OutPnt[0];
        if (numMsgs < 0)
            numMsgs += P2P_PBUF_SIZE;

        for (int i=0; i<numMsgs; i++) {
            if (P2PNode.OutPk[P2PNode.OutPnt[0]].MsgState == 1) {
                //msg is actually ready to be sent
                memcpy(msgCharBuffer, &P2PNode.OutPk[P2PNode.OutPnt[0]], P2P_MSGPACK_Size);
                pathological->pub_send_to_peers("peer", msgCharBuffer, P2P_MSGPACK_Size);
                P2PNode.OutPk[P2PNode.OutPnt[0]].MsgState = 2; //set it as taken
// Send an urgent message
                if (P2PNode.OutPk[P2PNode.OutPnt[0]].MsgHead.CntType == CNT_FAULT)
                    printf("Send Urgent MsgType: %d from ThisNode:%d to Node:%d, Time:%d/%d)\n",
                           P2PNode.OutPk[P2PNode.OutPnt[0]].MsgHead.CntType,
                           P2PNode.OutPk[P2PNode.OutPnt[0]].MsgHead.SenderIP.IP,
                           P2PNode.OutPk[P2PNode.OutPnt[0]].MsgHead.ReceiverIP.IP, Time.tv_sec, Time.tv_usec);
                else
                    printf("Send Normal MsgType: %d from ThisNode:%d to Node:%d, Time:%d/%d)\n",
                           P2PNode.OutPk[P2PNode.OutPnt[0]].MsgHead.CntType,
                           P2PNode.OutPk[P2PNode.OutPnt[0]].MsgHead.SenderIP.IP,
                           P2PNode.OutPk[P2PNode.OutPnt[0]].MsgHead.ReceiverIP.IP, Time.tv_sec, Time.tv_usec);
//                printf("Number: %d, \n", P2PNode.UgtOutPnt[0]);
            }

            //move to the next msg
            P2PNode.OutPnt[0]++;
            if (P2PNode.OutPnt[0] >= P2P_PBUF_SIZE)
                P2PNode.OutPnt[0] = 0;
        }
    }

    return error;
}

//
// Get Operation Data
//
void TFDIRContainer::GetOprData() { // Set operation data in P2P
    int i, j;
    ICS_NODE *icsNode;
    ICS_SET  *icsSet;
    P2P_SET  *Set;

    if (!P2PNode.PeerCommFDIR)
        return;

    icsNode = &IcsNode;
    for (i = 0; i < MAX_SETS; i++) {
        Set = &P2PNode.Set[i];
        if (!Set->SrvFlg)
            continue;
        icsSet = &icsNode->IcsSet[i];

        if (icsNode->LchFlg && (!Set->FltDir || Set->FltDir == FAULT_UNKNOWN)) {
            j = icsNode->TrpCnt;
            Set->FltDir = icsSet->Evt[j].FltDirSum;
//            Set->Peer.FltSentMark = 0;
            Set->FltZoneNo = 0;
            Set->FltZoneUn = 0;
        }

        if (Set->FltReset) { // clean all P2P Flt Infor
            Set->FltDir = 0;
            Set->FltZone = 0;
            Set->FltZoneNo = 0;
            Set->FltExecCnt = 0;
            Set->Peer.FltGot = 0;
            Set->Peer.FltDir = 0;
            Set->Peer.FltSentMark = 0;
            Set->FltReset = 0; // Reset the FltReset

//            printf("FltReset to 0 !!! \n");
        }
    }
}
//
// Determine P2P Fault Zone
//
void TFDIRContainer::P2PFaultScheme() { // Check all switches to determine peer fault zone

    int i, j, k, l, m, n;
    int i1, j1, k1;
    FLAG fD, zN = 0;
    float w, w1;
    ICS_SET *icsSET;
    P2P_SET *Set;
    P2P_PEER *Pr;

    if (!P2PNode.PeerCommFDIR)
        return;

    for (i = 0; i < MAX_SETS; i++) {
        Set = &P2PNode.Set[i];
        if (!Set->SrvFlg || !Set->FltDir || Set->FltZone || Set->FltZoneNo) {
            continue;
        }
        else if (Set->Peer.FltGot) {
            if (Set->FltDir == FAULT_BACKWARD &&
                    (Set->Peer.FltDir == FAULT_BACKWARD || Set->Peer.FltDir == FAULT_NO)) {
                Set->FltZone = 1;  // Fault in the side of this site
                printf("Set FltZone = 1, for Set:%d ! \n", i);
            }
            else if (Set->FltDir == FAULT_FORWARD ||
                        (Set->FltDir == FAULT_BACKWARD && Set->Peer.FltDir == FAULT_FORWARD) ||
                        (Set->FltDir == FAULT_UNKNOWN && Set->Peer.FltDir == FAULT_FORWARD)) {
                Set->FltZoneNo = 1;  // Fault not in the side of this site
                printf("Set FltZone No, for Set:%d ! \n", i);
            }
            else if ((Set->FltDir == FAULT_UNKNOWN && Set->Peer.FltDir == FAULT_BACKWARD) ||
                       (Set->FltDir == FAULT_BACKWARD && Set->Peer.FltDir == FAULT_UNKNOWN)) {
                Set->FltZoneUn = 1;  // Fault unknown in the side of this site
                printf("Set FltZone Unknown, for Set:%d ! \n", i);
            }
        }
        else if (!Set->Peer.P2PState && !Set->FltZoneUn || executionCt - Set->FltExecCnt >= 60) { // 1.0s, Lost comm. or not feeder back
            Set->FltZoneUn = 2;
            Set->FltExecCnt = executionCt;
            printf("Comm. bad/delayed for 1.0s, FltZone Cannot be set for Set:%d ! \n", i+1);
        }
    }
}

void TFDIRContainer::ToPeer() { // Send fault/Hart-bit information to peers
    int i, j, k;
    P2P_SET *Set;
    P2P_HEADER  *Mh;
    P2P_MSGPACK *Mp;
    P2P_PEER    *Pr;

    if (!P2PNode.PeerCommFDIR)
        return;

    for (i = 0; i < MAX_SETS; i++) { // check and Prepare locally detected fault info
        Set = &P2PNode.Set[i];
        Pr = &Set->Peer;
        if (Set->SrvFlg && Pr->Cfg.PeerSrvFlg) {
            if (Set->FltDir && !Set->FltZone) {
                k = executionCt - Set->Peer.FltSentMark;
                if (Set->Peer.FltSentMark < 1 || k == 120) { // Send FltDir no more than twice, two cycles late}
                    if (P2PNode.OutPnt[1] >= P2P_PBUF_SIZE)
                        P2PNode.OutPnt[1] = 0;
                    j = P2PNode.OutPnt[1];
                    Mp = &P2PNode.OutPk[j];
                    memset(Mp, 0, sizeof(P2P_MSGPACK));
                    Mh = &Mp->MsgHead;
                    Mh->CntType = CNT_FAULT;
                    Mh->SenderIP.IP = P2PNode.NodeID;  // NodeID is the truncated IP address
                    Mh->SenderIP.SetSeqNum = i;
                    Mh->ReceiverIP.IP = Pr->Cfg.PeerNodeID;
                    Mh->ReceiverIP.SetSeqNum = Pr->Cfg.PeerSetSeqNum;
                    Mp->MsgFlt.FltDir = Set->FltDir;
                    Mp->MsgState = 1; // Message ready for transmitting
                    P2PNode.OutPnt[1]++;
                    Set->Peer.LastExec = executionCt;
                    if (Set->Peer.FltSentMark < 1)
                        Set->Peer.FltSentMark = executionCt;
                }
            }
            else if ((executionCt - Set->Peer.LastExec) % 300 == 1) { // Heartbit per 5 seconds
                if (P2PNode.OutPnt[1] >= P2P_PBUF_SIZE)
                    P2PNode.OutPnt[1] = 0;
                j = P2PNode.OutPnt[1];
                Mp = &P2PNode.OutPk[j];
                memset(Mp, 0, sizeof(P2P_MSGPACK));
                Mh = &Mp->MsgHead;
                Mh->CntType = CNT_HEART_BEAT;
                Mh->SenderIP.IP = P2PNode.NodeID;  // NodeID is the truncated IP address
                Mh->SenderIP.SetSeqNum = i;
                Mh->ReceiverIP.IP = Pr->Cfg.PeerNodeID;
                Mh->ReceiverIP.SetSeqNum = Pr->Cfg.PeerSetSeqNum;
                Mp->MsgFlt.FltDir = Set->FltDir;
                Mp->MsgState = 1; // Message ready for transmitting
                P2PNode.OutPnt[1]++;
                Set->Peer.LastExec = executionCt-1;
            }
        }
    }
}

void TFDIRContainer::FromPeer() { // Get and process P2P information, and send cascading information right away
    int i, j, k, l;
    int sPnt, ePnt;
    P2P_SET *Set;
    P2P_PEER *Pr;
    P2P_HEADER *Mh, *nMh;
    P2P_MSGPACK *Mp, *nMp;
    bool valid;

    sPnt = P2PNode.InPnt[0]; // last read pnt
    ePnt = P2PNode.InPnt[1]; // last write pnt

    if (ePnt < sPnt)
        ePnt += P2P_PBUF_SIZE;
    for (i = sPnt; i < ePnt; i++) {
        valid = false;
        j = (i < P2P_PBUF_SIZE) ? i : i - P2P_PBUF_SIZE;
        Mp = &P2PNode.InPk[j];
        Mp->MsgState = 2;
        Mh = &Mp->MsgHead;
        if (Mh->ReceiverIP.IP == P2PNode.NodeID) { // Message is for this node
            k = Mh->ReceiverIP.SetSeqNum;
            Set = &P2PNode.Set[k];
            Pr = &Set->Peer;

            if (Mh->SenderIP.IP == Pr->Cfg.PeerNodeID) {
                if (!Pr->P2PState) {
//                    SetSeqEvt(SEQEVT_P2PPEERRTN, P2PNode.NodeID, k, 0, 0, 0);
                    printf("P2P Comm. of Set:%d from SenderID:%d returns NORMAL! \n",
                           Mh->SenderIP.SetSeqNum, Mh->SenderIP.IP);
                    Pr->P2PState = 1;
                }
                Pr->LastExec = executionCt;
                valid = true;
            }
            if (!valid) {
                printf("P2P Comm. of Set:%d from SenderID:%d does not Match this NodeID! \n",
                       Mh->SenderIP.SetSeqNum, Mh->SenderIP.IP);
                continue; // No peer match is found
            }
            // Process message contents
            switch (Mh->CntType) { // not for cascading
                case CNT_FAULT:
                    Pr->FltDir = Mp->MsgFlt.FltDir;
                    if (Set->FltDir) {
                        Pr->FltGot = 1;  // Set this flag only when the local switch sees a fault
                    }
                    else if (Pr->FltDir) { // The set did not see a fault, reply with no fault info
                        if (P2PNode.OutPnt[1] >= P2P_PBUF_SIZE)
                            P2PNode.OutPnt[1] = 0;
                        l = P2PNode.OutPnt[1];
                        nMp = &P2PNode.OutPk[l];
                        memset(nMp, 0, sizeof(P2P_MSGPACK));
                        nMh = &Mp->MsgHead;
                        nMh->CntType = CNT_FAULT;
                        nMh->SenderIP.IP = P2PNode.NodeID;  // NodeID is the truncated IP address
                        nMh->SenderIP.SetSeqNum = i;
                        nMh->ReceiverIP = Mh->SenderIP;
                        nMp->MsgFlt.FltDir = Set->FltDir;
                        nMp->MsgState = 1; // Message ready for transmitting
                        P2PNode.OutPnt[1]++;
                        Set->Peer.LastExec = executionCt;
                        Set->FltReset = 1; // Clear fault Infor for this set
                    }
                    break;
                case CNT_HEART_BEAT:
                    break;
                default:
                    break;
            }
        }
    }
    P2PNode.InPnt[0] = (ePnt < P2P_PBUF_SIZE) ? ePnt : ePnt - P2P_PBUF_SIZE; // Set the read pnt to the most recent
}