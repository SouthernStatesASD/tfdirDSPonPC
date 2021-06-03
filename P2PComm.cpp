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
                       recvMsgPack.MsgHead.ReceiverIP.IP, Time.sec, Time.usec);
                else
                    printf("Received Normal MsgType:%d, from Node:%d to ThisNode%d, Time:%d/%d\n",
                           recvMsgPack.MsgHead.CntType, recvMsgPack.MsgHead.SenderIP.IP,
                           recvMsgPack.MsgHead.ReceiverIP.IP, Time.sec, Time.usec);

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
                           P2PNode.OutPk[P2PNode.OutPnt[0]].MsgHead.ReceiverIP.IP, Time.sec, Time.usec);
                else
                    printf("Send Normal MsgType: %d from ThisNode:%d to Node:%d, Time:%d/%d)\n",
                           P2PNode.OutPk[P2PNode.OutPnt[0]].MsgHead.CntType,
                           P2PNode.OutPk[P2PNode.OutPnt[0]].MsgHead.SenderIP.IP,
                           P2PNode.OutPk[P2PNode.OutPnt[0]].MsgHead.ReceiverIP.IP, Time.sec, Time.usec);
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

    for (i = 0; i < MAX_SETS; i++) {
        Set = &P2PNode.Set[i];
        if (!Set->SrvFlg)
            continue;
        icsSet = &IcsNode.IcsSet[i];

        if (icsNode->LchFlg && !Set->FltDir) {
            j = icsNode->TrpCnt - 1;
            Set->FltDir = icsSet->Evt[j].FltDirSum;
        }

        if (Set->FltReset) { // clean all P2P Flt Infor
            Set->FltDir = 0;
            Set->FltZone = 0;
            Set->FltZoneNo = 0;
            Set->FltExecCnt = 0;
            Set->Peer.FltGot = 0;
            Set->Peer.FltDir = 0;
            Set->FltReset = 0; // Reset the FltReset
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
        if (!Set->SrvFlg || !Set->FltDir || Set->FltZone || Set->FltZoneNo || Set->FltZoneUn) {
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
        else if (!Set->Peer.P2PState || executionCt - Set->FltExecCnt >= 90) { // 1.5s, Lost comm. or not feeder back
            Set->FltZoneUn = 2;
            printf("Comm. bad or delayed for 2 Sec., FltZone Cannot be set for Set:%d ! \n", i);
        }
    }
}

void TFDIRContainer::ToPeer() { // Send fault/Hart-bit information to peers
    int i, j, l, k;
    int i1, j1, l1, k1;
    ICS_SET *icsSet;
    P2P_SET *Set;
    P2P_HEADER *Mh;
    P2P_MSGPACK *Mp;
    P2P_PEER *Pr;

    if (!P2PNode.PeerCommFDIR)
        return;

    for (i = 0; i < MAX_SETS; i++) { // check and Prepare locally detected fault info
        Set = &P2PNode.Set[i];
        Pr = &Set->Peer;
        if (Set->SrvFlg && Pr->Cfg.PeerSrvFlg) {
            if (Set->FltDir && !Set->FltZone && !Set->Peer.FltGot) {
                k = executionCt - Set->Peer.FltSentMark;
                if (Set->Peer.FltSentMark < 1 || k == 60) { // Send FltDir no more than twice}
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
                Mp->MsgState = 1; // Message ready for transmitting
                P2PNode.OutPnt[1]++;
                Set->Peer.LastExec = executionCt;
            }
        }
    }
}

void TFDIRContainer::FromPeer() { // Get and process P2P information, and send cascading information right away
    int i,  j,  k,  l,  m,  n;
    int i1, j1, k1, l1, m1, n1;
    int sPnt, ePnt, nPnt;
    FLAG fD;
    SDC_SWITCH *sdcSw, *nsdcSw;
    P2P_SWITCH *Sw, *nSw;
    P2P_PEER *Pr;
    P2P_NEXTPEER *nPr;
    P2P_HEADER   *Mh, *nMh;
    P2P_MSGPACK  *Mp, *nMp;
    bool valid;

    sPnt = P2PNode.UgtInPnt[0]; // last read pnt
    ePnt = P2PNode.UgtInPnt[1]; // last write pnt

    if (ePnt < sPnt)
        ePnt += P2P_PBUF_SIZE;
    for (i = sPnt; i < ePnt; i++) {
        valid = false;
        j = (i < P2P_PBUF_SIZE) ? i : i - P2P_PBUF_SIZE;
        Mp = &P2PNode.UgtInPk[j];

// ??? For Simulation Test use only
        if (operatingMode == SIMULATION) {
            if (ExecCnt - Mp->ExecCnt < 4*CHKS_PERCYC) { // Dely for 5 cycles
                P2PNode.UgtInPnt[0] = j;
                return;
            }
        }

        Mp->MsgState = 2;
        Mh = &Mp->MsgHead;
        if (Mh->ReceiverIP.IP == P2PNode.NodeID) { // Message is for this node
            k = Mh->ReceiverIP.SwSeqNum;
            l = Mh->ReceiverIP.TermSeqNum;
            m = Mh->ReceiverIP.PeerSeqNum;
            sdcSw = &SdcNode.SdcSw[k];
            Sw = &P2PNode.Switch[k];
            Pr = &Sw->Term[l].Peer[m];
            if (!Mh->ToNextPeer) { // Message from immediate peer
                if (Mh->SenderIP.IP == Pr->Cfg.PeerNodeID) {
                    if (!Pr->P2PState) {
                        SetSeqEvt(SEQEVT_P2PPEERRTN, P2PNode.NodeID, k, l, m, 1);
                        Pr->P2PState = 1;
                        if (!Pr->PeerCfg) { // This peer's config. is not capcured yet
                            l1 = (l == 0) ? 1 : 0;
                            if (!Sw->Term[l1].HiddFlg) { // Single Switch
                                ReqP2PCfg(k, l, m, k, l1);
                            }
                            else { // Multiple Switch
                                for (i1 = 0; i1 < MAX_SWITCHES; i1++) {
                                    nSw = &P2PNode.Switch[l];
                                    if (nSw == Sw || !nSw->SrvFlg)
                                        continue;
                                    k1 = (nSw->Term[0].HiddFlg) ? 1 : 0;   // index to the other switch's terminal
                                    ReqP2PCfg(k, l, m, nSw->SeqNum, k1);
                                }
                            }
                        }
                    }
                    Pr->LastExec = ExecCnt;
                    valid = true;
                }
                else {
                    for (m1 = 0; m1 < MAX_PEERS; m1++) {
                        Pr = &Sw->Term[l].Peer[m];
                        if (Pr->Cfg.SrvFlg && Mh->SenderIP.IP == Pr->Cfg.PeerNodeID) {
                            if (!Pr->P2PState) {
                                SetSeqEvt(SEQEVT_P2PPEERRTN, P2PNode.NodeID, k, l, m, 1);
                                Pr->P2PState = 1;
                                if (!Pr->PeerCfg) { // This peer's config. is not capcured yet
                                    l1 = (l == 0) ? 1 : 0;
                                    if (!Sw->Term[l1].HiddFlg) { // Single Switch
                                        ReqP2PCfg(k, l, m1, k, l1);
                                    } else { // Multiple Switch
                                        for (i1 = 0; i1 < MAX_SWITCHES; i1++) {
                                            nSw = &P2PNode.Switch[l];
                                            if (nSw == Sw || !nSw->SrvFlg)
                                                continue;
                                            k1 = (nSw->Term[0].HiddFlg) ? 1
                                                                        : 0;   // index to the other switch's terminal
                                            ReqP2PCfg(k, l, m1, nSw->SeqNum, k1);
                                        }
                                    }
                                }
                            }
                            m = m1;
                            Pr->LastExec = ExecCnt;
                            valid = true;
                            break;
                        }
                    }
                }
                if (!valid) {
                    SetSeqEvt(SEQEVT_PEERMISMCH,Mh->SenderIP.IP, Mh->SenderIP.SwSeqNum, Mh->SenderIP.TermSeqNum,
                            Mh->SenderIP.PeerSeqNum, 1);
                }
            }
            else if (!P2PNode.NonCommFDIR) { // Message from next peer
                n1 = Mh->SenderIP.SwSeqNum;
                m1 = Mh->SenderIP.PeerSeqNum;
                for (n = 0; n < MAX_PEERS; n++) {
                    nPr = &Sw->Term[l].NextPeer[n][n1];
                    if (!nPr->SwSrvFlg)
                        continue;
                    Pr = &nPr->Peer[m1];
                    if (Mh->SenderIP.IP == Pr->Cfg.PeerNodeID) {
                        if (!Pr->P2PState) {
                            SetSeqEvt(SEQEVT_P2PNEXTPEERRTN, P2PNode.NodeID, k, l, m, 1);
                            nPr->Peer[m1].P2PState = 1;
                        }
                        Pr->LastExec = ExecCnt;
                        valid = true;
                        break;
                    }
                }
                if (!valid) { // Search all peers
                    for (n = 0; n < MAX_PEERS; n++) {
                        nPr = &Sw->Term[l].NextPeer[n][n1];
                        if (!nPr->SwSrvFlg)
                            continue;
                        for (m1 = 0; m1 < MAX_PEERS; m1++) {
                            Pr = &nPr->Peer[m1];
                            if (Mh->SenderIP.IP == Pr->Cfg.PeerNodeID) {
                                if (!Pr->P2PState) {
                                    SetSeqEvt(SEQEVT_P2PNEXTPEERRTN, P2PNode.NodeID, k, l, m, 1);
                                    nPr->Peer[m1].P2PState = 1;
                                }
                                Pr->LastExec = ExecCnt;
                                valid = true;
                                break;
                            }
                        }
                        if (valid)
                            break;
                    }
                }
                if (!valid) {
                    SetSeqEvt(SEQEVT_NEXTPEERMISMCH,Mh->SenderIP.IP, Mh->SenderIP.SwSeqNum, Mh->SenderIP.TermSeqNum,
                              Mh->SenderIP.PeerSeqNum, 1);
                }
            }
            if (!valid) {
                printf ("UrgentType:%d, Peer:%d from SenderID:%d, Switch:%d, Term:%d does not match this receiver's corresponding Term: %d, Peer:%d \n",
                        Mh->CntType, Mh->SenderIP.PeerSeqNum, Mh->SenderIP.IP, Mh->SenderIP.SwSeqNum,
                        Mh->SenderIP.TermSeqNum, Mh->ReceiverIP.TermSeqNum, Mh->ReceiverIP.PeerSeqNum);
                continue; // No peer match is found
            }
            // Process message contents
            switch (Mh->MsgType) { // not for cascading
                case TO_HOST:
                    // For each TO_HOST message, the P2P server should take it to DNP protocol if this node is a HOST
                    // and then cascade to the other nodes if this node is not a Host node
                    if (P2PNode.HostComm) {
                        // This is a Comm. HostNode, process the data to report to the Master

                    }
                    else { // This node has no HostComm, Cascade to the peer nodes
                        l1 = (l == 0) ? 1 : 0;
                        if (!Sw->Term[l1].HiddFlg) { // Only one switch in the node
                            for (m1 = 0; m1 < MAX_PEERS; m1++) {
                                Pr = &Sw->Term[j1].Peer[k1];
                                if (!Pr->Cfg.SrvFlg)
                                    continue;
                                if (Pr->P2PState) {
                                    if (P2PNode.UgtOutPnt[1] >= P2P_PBUF_SIZE)
                                        P2PNode.UgtOutPnt[1] = 0;
                                    nPnt = P2PNode.UgtOutPnt[1];
                                    nMp = &P2PNode.UgtOutPk[nPnt];
                                    *nMp = *Mp;
                                    nMh = &nMp->MsgHead;
                                    nMh->ToNextPeer = 0;
                                    nMh->SenderIP.IP = P2PNode.NodeID;
                                    nMh->SenderIP.SwSeqNum = k;
                                    nMh->SenderIP.TermSeqNum = l1;
                                    nMh->SenderIP.PeerSeqNum = m1;
                                    nMh->ReceiverIP.IP = Pr->Cfg.PeerNodeID;
                                    nMh->ReceiverIP.SwSeqNum = Pr->Cfg.PeerSwSeqNum;
                                    nMh->ReceiverIP.TermSeqNum = Pr->Cfg.PeerTermSeqNum;
                                    nMh->ReceiverIP.PeerSeqNum = Pr->Cfg.PeerSeqNum;
                                    nMp->MsgState = 1;
                                    P2PNode.UgtOutPnt[1]++;
                                }
                                else if (!P2PNode.NonCommFDIR) { // Go to next level peers
                                    for (k1 = 0; k1 < MAX_SWITCHES - 1; k1++) {
                                        nPr = &Sw->Term[l1].NextPeer[m1][k1];
                                        if (!nPr->SwSrvFlg)
                                            continue;
                                        for (n1 = 0; n1 < MAX_PEERS; n1++) {
                                            if (!nPr->Peer[n1].Cfg.SrvFlg)
                                                continue;
                                            if (P2PNode.UgtOutPnt[1] >= P2P_PBUF_SIZE)
                                                P2PNode.UgtOutPnt[1] = 0;
                                            nPnt = P2PNode.UgtOutPnt[1];
                                            nMp = &P2PNode.UgtOutPk[nPnt];
                                            *nMp = *Mp;
                                            nMh = &nMp->MsgHead;
                                            nMh->ToNextPeer = 1;
                                            nMh->SenderIP.IP = P2PNode.NodeID;
                                            nMh->SenderIP.SwSeqNum = k;
                                            nMh->SenderIP.TermSeqNum = l1;
                                            nMh->SenderIP.PeerSeqNum = m1;
                                            nMh->ReceiverIP.IP = nPr->Peer[n1].Cfg.PeerNodeID;
                                            nMh->ReceiverIP.SwSeqNum = nPr->Peer[n1].Cfg.PeerSwSeqNum;
                                            nMh->ReceiverIP.TermSeqNum = nPr->Peer[n1].Cfg.PeerTermSeqNum;
                                            nMh->ReceiverIP.PeerSeqNum = nPr->Peer[n1].Cfg.PeerSeqNum;
                                            nMp->MsgState = 1;
                                            P2PNode.UgtOutPnt[1]++;
                                        }
                                    }
                                }
                            }
                        }
                        else { // Multiple switches in this node
                            for (k1 = 0; k1 < MAX_SWITCHES; k1++) {
                                nSw = &P2PNode.Switch[k1];
                                if (nSw == Sw || !nSw->SrvFlg)
                                    continue;
                                l1 = (nSw->Term[0].HiddFlg) ? 1 : 0;
                                for (m1 = 0; m1 < MAX_PEERS; m1++) {
                                    Pr = &nSw->Term[l1].Peer[m1];
                                    if (!Pr->Cfg.SrvFlg)
                                        continue;
                                    if (Pr->P2PState) {
                                        if (P2PNode.UgtOutPnt[1] >= P2P_PBUF_SIZE)
                                            P2PNode.UgtOutPnt[1] = 0;
                                        nPnt = P2PNode.UgtOutPnt[1];
                                        nMp = &P2PNode.UgtOutPk[nPnt];
                                        *nMp = *Mp;
                                        nMh = &nMp->MsgHead;
                                        nMh->ToNextPeer = 0;
                                        nMh->SenderIP.IP = P2PNode.NodeID;
                                        nMh->SenderIP.SwSeqNum = nSw->SeqNum;
                                        nMh->SenderIP.TermSeqNum = l1;
                                        nMh->SenderIP.PeerSeqNum = m1;
                                        nMh->ReceiverIP.IP = Pr->Cfg.PeerNodeID;
                                        nMh->ReceiverIP.SwSeqNum = Pr->Cfg.PeerSwSeqNum;
                                        nMh->ReceiverIP.TermSeqNum = Pr->Cfg.PeerTermSeqNum;
                                        nMh->ReceiverIP.PeerSeqNum = Pr->Cfg.PeerSeqNum;
                                        nMp->MsgState = 1;
                                        P2PNode.UgtOutPnt[1]++;
                                    }
                                    else if (!P2PNode.NonCommFDIR) { // Go to next level peers
                                        for (j1 = 0; j1 < MAX_SWITCHES - 1; j1++) {
                                            nPr = &nSw->Term[l1].NextPeer[m1][j1];
                                            if (!nPr->SwSrvFlg)
                                                continue;
                                            for (n1 = 0; n1 < MAX_PEERS; n1++) {
                                                if (!nPr->Peer[n1].Cfg.SrvFlg)
                                                    continue;
                                                if (P2PNode.UgtOutPnt[1] >= P2P_PBUF_SIZE)
                                                    P2PNode.UgtOutPnt[1] = 0;
                                                nPnt = P2PNode.UgtOutPnt[1];
                                                nMp = &P2PNode.UgtOutPk[nPnt];
                                                *nMp = *Mp;
                                                nMh = &nMp->MsgHead;
                                                nMh->ToNextPeer = 1;
                                                nMh->SenderIP.IP = P2PNode.NodeID;
                                                nMh->SenderIP.SwSeqNum = Sw->SeqNum;
                                                nMh->SenderIP.TermSeqNum = l1;
                                                nMh->SenderIP.PeerSeqNum = m1;
                                                nMh->ReceiverIP.IP = nPr->Peer[n1].Cfg.PeerNodeID;
                                                nMh->ReceiverIP.SwSeqNum = nPr->Peer[n1].Cfg.PeerSwSeqNum;
                                                nMh->ReceiverIP.TermSeqNum = nPr->Peer[n1].Cfg.PeerTermSeqNum;
                                                nMh->ReceiverIP.PeerSeqNum = nPr->Peer[n1].Cfg.PeerSeqNum;
                                                nMp->MsgState = 1;
                                                P2PNode.UgtOutPnt[1]++;
                                            }
                                        }
                                    }
                                }

                            }
                        }
                    }
                    break;
                case TO_PEER:
                    switch (Mh->CntType) {
                        case CNT_FLTZONE1:
                            if (Sw->SwStatus && (Sw->SwLocked || Sw->SwLocal)) {
                                Sw->FltCase = FLT_UP;
                                Sw->TripPhs = Mp->MsgCnt.Ctrl.SwPhs;
                                Sw->FltSide = l;
                            }
                            else {
                                l1 = (l == 0)? 1:0;
                                if (!Sw->Term[l1].NoTie) { // The terminal can lead to a Tie-switch, open it
                                    if (!Sw->SwStatus && !Sw->SwLocked && sdcSw->DevCmd->Lock[0].type != CMDTYPE_ON) {
                                        // Switch is the open tie, lock it for NonCommFDIR case
                                        if (CfgNode.NonCommFDIR) {
                                            sdcSw->DevCmd->Lock[0].type = CMDTYPE_ON;
                                            sdcSw->DevCmd->Lock[0].val = PHASE_ABC;
                                        }
                                    }
                                    else {
                                        Sw->FltZone = 1;
                                        Sw->TripPhs = Mp->MsgCnt.Ctrl.SwPhs;
                                        Sw->FltZoneExecCnt = ExecCnt; // Set ExecCnt as a time mark when FltZone is
                                        if (!Sw->FltState) {
                                            Sw->Term[l].PeerFltNum = 1; // Mark the terminal to have the other one start TieClsCmd
                                        }
                                    }
                                }
                            }
                            break;
                        case CNT_FLTZONE2:
                            l1 = (l == 0)? 1:0;
                            if (!Sw->Term[l1].NoTie) { // The terminal can lead to a Tie-switch, open it
                                if (!Sw->SwStatus && !Sw->SwLocked && sdcSw->DevCmd->Lock[0].type != CMDTYPE_ON) {
                                    // Switch is the open tie, lock it for NonCommFDIR case
                                    if (CfgNode.NonCommFDIR) {
                                        sdcSw->DevCmd->Lock[0].type = CMDTYPE_ON;
                                        sdcSw->DevCmd->Lock[0].val = PHASE_ABC;
                                    }
                                }
                                else {
                                    Sw->FltZone = 1;
                                    Sw->TripPhs = Mp->MsgCnt.Ctrl.SwPhs;
                                    Sw->FltZoneExecCnt = ExecCnt; // Set ExecCnt as a time mark when FltZone is
                                    if (!Sw->FltState) {
                                        Sw->Term[l].PeerFltNum = 1; // Mark the terminal to have the other one start TieClsCmd
                                    }
                                }
                            }
                            break;
                        case CNT_FAULT:
                            Pr->FltDir = Mp->MsgCnt.Fault.FltDir;
                            Pr->FltPhs = Mp->MsgCnt.Fault.FltPhs;
                            if (Sw->Term[l].FltDir)
                                Pr->FltGot = 1;  // Set this flag only when the local switch sees a fault
                            if (Pr->FltDir) { // The peer saw a fault, reply with this terminal's fault info
                                Sw->Term[l].PeerFltNum += 1; // Count the number of faults received from peers
                                Sw->Term[l].PeerFltDir = Pr->FltDir;
                                if (!Sw->Term[l].FltDir) { // This terminal does not see the fault, send no-fault flag
//                                    for (m1 = 0; m1 < MAX_PEERS; m1++) { // to all pears of this terminal
//                                        Pr = &Sw->Term[l].Peer[m1];
//                                        if (!Pr->Cfg.SrvFlg)
//                                            continue;
//                                        if (Pr->P2PState) {
//                                            if (P2PNode.UgtOutPnt[1] >= P2P_PBUF_SIZE)
//                                                P2PNode.UgtOutPnt[1] = 0;
//                                            nPnt = P2PNode.UgtOutPnt[1];
//                                            nMp = &P2PNode.UgtOutPk[nPnt];
//                                            nMh = &nMp->MsgHead;
//                                            *nMh = *Mh;
//                                            nMh->SenderIP.IP = P2PNode.NodeID;
//                                            nMh->SenderIP.SwSeqNum = Sw->SeqNum;
//                                            nMh->SenderIP.TermSeqNum = l;
//                                            nMh->SenderIP.PeerSeqNum = m1;
//                                            nMh->ReceiverIP.IP = Pr->Cfg.PeerNodeID;
//                                            nMh->ReceiverIP.SwSeqNum = Pr->Cfg.PeerSwSeqNum;
//                                            nMh->ReceiverIP.TermSeqNum = Pr->Cfg.PeerTermSeqNum;
//                                            nMh->ReceiverIP.PeerSeqNum = Pr->Cfg.PeerSeqNum;
//                                            nMp->MsgCnt.Fault.FltDir = 0;
//                                            nMp->MsgState = 1;
//                                            P2PNode.UgtOutPnt[1]++;
//                                        }
//                                    }
                                    if (P2PNode.UgtOutPnt[1] >= P2P_PBUF_SIZE) // Send back to the sending node
                                        P2PNode.UgtOutPnt[1] = 0;
                                    nPnt = P2PNode.UgtOutPnt[1];
                                    nMp = &P2PNode.UgtOutPk[nPnt];
                                    nMh = &nMp->MsgHead;
                                    *nMh = *Mh;
                                    nMh->SenderIP = Mh->ReceiverIP;
                                    nMh->ReceiverIP = Mh->SenderIP;
                                    nMp->MsgCnt.Fault.FltDir = 0;
                                    nMp->MsgCnt.Fault.FltPhs = 0;
                                    nMp->MsgState = 1;
                                    P2PNode.UgtOutPnt[1]++;
                                }
                            }
                            break;
                        case CNT_INCFAULT:
                            k1 = 0;
                            if (Mp->MsgCnt.IncFault.IncFlag) { // Sender switch sees fault
                                if (Sw->Term[l].IincFlag) { // A replied IncFault Flag to this terminal
                                    Pr->IncGot = 2; // IncFault is not between this switch and the downstream replier
                                }
                                else { // A IncFault from upstream switch to request for replying
                                    k1 = 1; // for reply to the sender switch with IncFault Infor
                                }
                            }
                            else if (Sw->Term[l].IincFlag) { // Replied IncFault Infor from a dowsntream switch
                                Pr->IncGot = 1; // Fault may be between this switch and the upstream switch
                                for (l1 = 0; l1 < 3; l1++ )
                                    Sw->Term[l].Iinc.Iseg[l1] -= Mp->MsgCnt.IncFault.Imag[l1];
                            }

                            if (k1 > 0) { // The peer saw a fault, reply with this terminal's IncFault infor regardless
                                l1 = (l == 0) ? 1 : 0;
                                if (P2PNode.UgtOutPnt[1] >= P2P_PBUF_SIZE) // Send back to the sending node
                                    P2PNode.UgtOutPnt[1] = 0;
                                nPnt = P2PNode.UgtOutPnt[1];
                                nMp = &P2PNode.UgtOutPk[nPnt];
                                nMh = &nMp->MsgHead;
                                *nMh = *Mh;
                                nMh->SenderIP = Mh->ReceiverIP;
                                nMh->ReceiverIP = Mh->SenderIP;
                                nMp->MsgCnt.IncFault.IncFlag = Sw->Term[l1].IincFlag;
                                nMp->MsgCnt.IncFault.IncDir  = Sw->Term[l1].Iinc.IncDir;
                                nMp->MsgCnt.IncFault.IncPhs  = Sw->Term[l1].Iinc.IncPhs;
                                memcpy(nMp->MsgCnt.IncFault.Imag, Sw->OprData[0].I, 3*sizeof(float));
                                nMp->MsgState = 1;
                                P2PNode.UgtOutPnt[1]++;
                            }
                            break;
                        case CNT_STATUS:
                            Pr->SwStatus = Mp->MsgCnt.Status.SwStatus;
                            Pr->SwLock = Mp->MsgCnt.Status.SwLock;
                            break;
                        case CNT_CTRL_STATUS:
                            fD = Mp->MsgCnt.Ctrl.SwStatus;
                            if (fD && !Sw->SwStatus && sdcSw->DevCmd->Stat[0].type != CMDTYPE_ON) {
                                sdcSw->DevCmd->Stat[0].type = CMDTYPE_ON;
                                sdcSw->DevCmd->Stat[0].val = PHASE_ABC;
                            }
                            else if (!fD && Sw->SwStatus && sdcSw->DevCmd->Stat[0].type != CMDTYPE_OFF) {
                                sdcSw->DevCmd->Stat[0].type = CMDTYPE_OFF;
                                sdcSw->DevCmd->Stat[0].val = PHASE_ABC;
                            }
                            break;
                        case CNT_CTRL_LOCK:
                            fD = P2PNode.UgtInPk[j].MsgCnt.Ctrl.SwStatus;
                            if (fD && !Sw->SwLocked && sdcSw->DevCmd->Lock[0].type != CMDTYPE_ON) {
                                sdcSw->DevCmd->Lock[0].type = CMDTYPE_ON;
                                sdcSw->DevCmd->Lock[0].val = PHASE_ABC;
                            }
                            else if (!fD && Sw->SwLocked && sdcSw->DevCmd->Lock[0].type != CMDTYPE_OFF) {
                                sdcSw->DevCmd->Lock[0].type = CMDTYPE_OFF;
                                sdcSw->DevCmd->Lock[0].val = PHASE_ABC;
                            }
                            break;
                        case CNT_CTRL_LOCAL:
                            fD = P2PNode.UgtInPk[j].MsgCnt.Ctrl.SwStatus;
                            if (fD && !Sw->SwLocked && sdcSw->DevCmd->Local[0].type != CMDTYPE_ON) {
                                sdcSw->DevCmd->Local[0].type = CMDTYPE_ON;
                                sdcSw->DevCmd->Local[0].val = PHASE_ABC;
                            }
                            else if (!fD && Sw->SwLocked && sdcSw->DevCmd->Local[0].type != CMDTYPE_OFF) {
                                sdcSw->DevCmd->Local[0].type = CMDTYPE_OFF;
                                sdcSw->DevCmd->Local[0].val = PHASE_ABC;
                            }
                            break;
                    }
                    break;
                case TO_CASCADE:
                    if (Mh->CntType == CNT_CTRL_STATUS || Mh->CntType == CNT_CTRL_TIE) {
                        if (Mh->SrcTarIP.IP == P2PNode.NodeID) { // This is the target node
                            k = Mh->SrcTarIP.SwSeqNum;
                            l = Mh->SrcTarIP.TermSeqNum;
                            sdcSw = &SdcNode.SdcSw[k];
                            Sw = sdcSw->P2PSw;
                            fD = Mp->MsgCnt.Ctrl.SwStatus;
                            if (Mh->CntType == CNT_CTRL_STATUS) {
                                if (fD && !Sw->SwStatus && sdcSw->DevCmd->Stat[0].type != CMDTYPE_ON) {
                                    sdcSw->DevCmd->Stat[0].type = CMDTYPE_ON;
                                    sdcSw->DevCmd->Stat[0].exec = CMDEXEC_NO;
                                    sdcSw->DevCmd->Stat[0].val = PHASE_ABC;
                                }
                                else if (!fD && Sw->SwStatus && sdcSw->DevCmd->Stat[0].type != CMDTYPE_OFF) {
                                    sdcSw->DevCmd->Stat[0].type = CMDTYPE_OFF;
                                    sdcSw->DevCmd->Stat[0].exec = CMDEXEC_NO;
                                    sdcSw->DevCmd->Stat[0].val = PHASE_ABC;
                                }
                            }
                            else if (!Sw->SwStatus) { // For TIE Switch to close
                                l1 = (l == 0)? 1 : 0;
                                if (sdcSw->DevCmd->Stat[0].type != CMDTYPE_ON &&
                                    Sw->Term[l1].EngzFlg&&(!Sw->Term[l].EngzFlg || Mp->MsgCnt.Ctrl.SwPhs < PHASE_ABC)) {
                                    sdcSw->DevCmd->Stat[0].type = CMDTYPE_ON;
                                    sdcSw->DevCmd->Stat[0].exec = CMDEXEC_NO;
                                    sdcSw->DevCmd->Stat[0].val = PHASE_ABC;
                                    if (Mp->MsgCnt.Ctrl.SwPhs == PHASE_ABC)
                                        sdcSw->DevCmd->Stat[0].chk = 1; // Close only if one side is hot
                                    else
                                        sdcSw->DevCmd->Stat[0].chk = 0;
                                }
                            }
                            else if (!Sw->Term[0].EngzFlg){ // Switch is closed and dead, wrong SrcTarIP, continue trace
                                // continue to trace ???
                            }
                        }
                        else if (Mh->CntType == CNT_CTRL_STATUS ||
                                 (Sw->SwStatus && Mh->CntType == CNT_CTRL_TIE && (!Sw->Term[0].EngzFlg ||
                                  Mp->MsgCnt.Ctrl.SwPhs > 0 && Mp->MsgCnt.Ctrl.SwPhs < PHASE_ABC))) {
                            // Cascade to the peers of the other side terminal of the switch
                            l1 = (l == 0) ? 1 : 0;
                            if (!Sw->Term[l1].HiddFlg) { // only one switch in the node
                                for (m1 = 0; m1 < MAX_PEERS; m1++) {
                                    Pr = &Sw->Term[l1].Peer[m1];
                                    if (!Pr->Cfg.SrvFlg)
                                        continue;
                                    if (Pr->P2PState) {
                                        if (P2PNode.UgtOutPnt[1] >= P2P_PBUF_SIZE)
                                            P2PNode.UgtOutPnt[1] = 0;
                                        nPnt = P2PNode.UgtOutPnt[1];
                                        nMp = &P2PNode.UgtOutPk[nPnt];
                                        *nMp = *Mp;
                                        nMh = &nMp->MsgHead;
                                        nMh->ToNextPeer = 0;
                                        nMh->SenderIP.IP = P2PNode.NodeID;
                                        nMh->SenderIP.SwSeqNum = k;
                                        nMh->SenderIP.TermSeqNum = l1;
                                        nMh->SenderIP.PeerSeqNum = m1;
                                        nMh->ReceiverIP.IP = Pr->Cfg.PeerNodeID;
                                        nMh->ReceiverIP.SwSeqNum = Pr->Cfg.PeerSwSeqNum;
                                        nMh->ReceiverIP.TermSeqNum = Pr->Cfg.PeerTermSeqNum;
                                        nMh->ReceiverIP.PeerSeqNum = Pr->Cfg.PeerSeqNum;
                                        nMp->MsgState = 1;
                                        P2PNode.UgtOutPnt[1]++;
                                    }
                                    else if (!P2PNode.NonCommFDIR) { // Got to next layer peers
                                        for (k1 = 0; k1 < MAX_SWITCHES; k1++) {
                                            nPr = &Sw->Term[l1].NextPeer[m1][k1];
                                            if (!nPr->SwSrvFlg)
                                                continue;
                                            for (n1 = 0; n1 < MAX_PEERS; n1++) {
                                                Pr = &nPr->Peer[n1];
                                                if (!Pr->Cfg.SrvFlg || !Pr->P2PState)
                                                    continue;
                                                if (P2PNode.UgtOutPnt[1] >= P2P_PBUF_SIZE)
                                                    P2PNode.UgtOutPnt[1] = 0;
                                                nPnt = P2PNode.UgtOutPnt[1];
                                                nMp = &P2PNode.UgtOutPk[nPnt];
                                                *nMp = *Mp;
                                                nMh = &nMp->MsgHead;
                                                nMh->ToNextPeer = 1;
                                                nMh->SenderIP.IP = P2PNode.NodeID;
                                                nMh->SenderIP.SwSeqNum = k;
                                                nMh->SenderIP.TermSeqNum = l1;
                                                nMh->SenderIP.PeerSeqNum = m1;
                                                nMh->ReceiverIP.IP = Pr->Cfg.PeerNodeID;
                                                nMh->ReceiverIP.SwSeqNum = Pr->Cfg.PeerSwSeqNum;
                                                nMh->ReceiverIP.TermSeqNum = Pr->Cfg.PeerTermSeqNum;
                                                nMh->ReceiverIP.PeerSeqNum = Pr->Cfg.PeerSeqNum;
                                                nMp->MsgState = 1;
                                                P2PNode.UgtOutPnt[1]++;
                                            }
                                        }
                                    }
                                }
                            }
                            else { // Multiple switches in the node
                                for (k1 = 0; k1 < MAX_SWITCHES; k1++) {
                                    nSw = &P2PNode.Switch[k1];
                                    if (nSw == Sw || !nSw->SrvFlg || !nSw->SwStatus)
                                        continue;
                                    l1 = (nSw->Term[0].HiddFlg) ? 1 : 0;
                                    for (m1 = 0; m1 < MAX_PEERS; m1++) {
                                        Pr = &nSw->Term[l1].Peer[m1];
                                        if (!Pr->Cfg.SrvFlg)
                                            continue;
                                        if (Pr->P2PState) {
                                            if (P2PNode.UgtOutPnt[1] >= P2P_PBUF_SIZE)
                                                P2PNode.UgtOutPnt[1] = 0;
                                            nPnt = P2PNode.UgtOutPnt[1];
                                            nMp = &P2PNode.UgtOutPk[nPnt];
                                            *nMp = *Mp;
                                            nMh = &nMp->MsgHead;
                                            nMh->ToNextPeer = 0;
                                            nMh->SenderIP.IP = P2PNode.NodeID;
                                            nMh->SenderIP.SwSeqNum = k1;
                                            nMh->SenderIP.TermSeqNum = l1;
                                            nMh->SenderIP.PeerSeqNum = m1;
                                            nMh->ReceiverIP.IP = Pr->Cfg.PeerNodeID;
                                            nMh->ReceiverIP.SwSeqNum = Pr->Cfg.PeerSwSeqNum;
                                            nMh->ReceiverIP.TermSeqNum = Pr->Cfg.PeerTermSeqNum;
                                            nMh->ReceiverIP.PeerSeqNum = Pr->Cfg.PeerSeqNum;
                                            nMp->MsgState = 1;
                                            P2PNode.UgtOutPnt[1]++;
                                        }
                                        else if (!P2PNode.NonCommFDIR) { // Got to next layer peers
                                            for (i1 = 0; i1 < MAX_SWITCHES; i1++) {
                                                nPr = &nSw->Term[l1].NextPeer[m1][i1];
                                                if (!nPr->SwSrvFlg)
                                                    continue;
                                                for (n1 = 0; n1 < MAX_PEERS; n1++) {
                                                    Pr = &nPr->Peer[n1];
                                                    if (!Pr->Cfg.SrvFlg || !Pr->P2PState)
                                                        continue;
                                                    if (P2PNode.UgtOutPnt[1] >= P2P_PBUF_SIZE)
                                                        P2PNode.UgtOutPnt[1] = 0;
                                                    nPnt = P2PNode.UgtOutPnt[1];
                                                    nMp = &P2PNode.UgtOutPk[nPnt];
                                                    *nMp = *Mp;
                                                    nMh = &nMp->MsgHead;
                                                    nMh->ToNextPeer = 1;
                                                    nMh->SenderIP.IP = P2PNode.NodeID;
                                                    nMh->SenderIP.SwSeqNum = k1;
                                                    nMh->SenderIP.TermSeqNum = l1;
                                                    nMh->SenderIP.PeerSeqNum = m1;
                                                    nMh->ReceiverIP.IP = nPr->Peer[n1].Cfg.PeerNodeID;
                                                    nMh->ReceiverIP.SwSeqNum = nPr->Peer[n1].Cfg.PeerSwSeqNum;
                                                    nMh->ReceiverIP.TermSeqNum = nPr->Peer[n1].Cfg.PeerTermSeqNum;
                                                    nMh->ReceiverIP.PeerSeqNum = nPr->Peer[n1].Cfg.PeerSeqNum;
                                                    nMp->MsgState = 1;
                                                    P2PNode.UgtOutPnt[1]++;
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                    else if (Mh->CntType == CNT_TRCE_TIE) { // Trace to an open tie
                        l1 = (l == 0)? 1 : 0;
                        if (!Sw->SwStatus && !Sw->SwLocked) {
                            if (sdcSw->DevCmd->Stat[0].type != CMDTYPE_ON && Sw->Term[l1].EngzFlg) { // Other side hot
                                sdcSw->DevCmd->Stat[0].type = CMDTYPE_ON;
                                sdcSw->DevCmd->Stat[0].exec = CMDEXEC_NO;
                                sdcSw->DevCmd->Stat[0].val = PHASE_ABC;
                                sdcSw->DevCmd->Stat[0].dur = 0.0;
                                sdcSw->DevCmd->Stat[0].chk = 1; // Close only if one side is hot after the timer up
                                sdcSw->DevCmd->ExecCntDn = (int) (Mp->MsgCnt.SrcCap.TimeOff / SdcNode.ExecIntv);
                                if(Sw->Term[l].EngzFlg) { // From-terminal hot too, wait for it to be dead
                                    sdcSw->DevCmd->ExecCntDn += 50;
                                }
                            }
                        }
//                      else if (Sw->SwStatus && !Sw->Term[0].EngzFlg) { // switch is closed and dead, continue cascade
                        else if (Sw->SwStatus) { // switch is closed, continue cascade
                            if (!Sw->Term[l1].HiddFlg) { // Single switch node
                                for (m1 = 0; m1 < MAX_PEERS; m1++) {
                                    Pr = &Sw->Term[l1].Peer[m1];
                                    if (!Pr->Cfg.SrvFlg)
                                        continue;
                                    if (P2PNode.UgtOutPnt[1] >= P2P_PBUF_SIZE)
                                        P2PNode.UgtOutPnt[1] = 0;
                                    nPnt = P2PNode.UgtOutPnt[1];
                                    nMp = &P2PNode.UgtOutPk[nPnt];
                                    *nMp = *Mp;
                                    nMh = &nMp->MsgHead;
                                    nMh->ToNextPeer = 0;
                                    nMh->SenderIP.IP = P2PNode.NodeID;
                                    nMh->SenderIP.SwSeqNum = k;
                                    nMh->SenderIP.TermSeqNum = l1;
                                    nMh->SenderIP.PeerSeqNum = m1;
                                    nMh->ReceiverIP.IP = Pr->Cfg.PeerNodeID;
                                    nMh->ReceiverIP.SwSeqNum = Pr->Cfg.PeerSwSeqNum;
                                    nMh->ReceiverIP.TermSeqNum = Pr->Cfg.PeerTermSeqNum;
                                    nMh->ReceiverIP.PeerSeqNum = Pr->Cfg.PeerSeqNum;
                                    nMp->MsgCnt.SrcCap.TimeOff += 0.2 * (float)m1;
                                    nMp->MsgState = 1;
                                    P2PNode.UgtOutPnt[1]++;
                                    if (!Pr->P2PState && !P2PNode.NonCommFDIR) { // Send to the Next level peers too
                                        for (k1 = 0; k1 < MAX_SWITCHES; k1++) {
                                            nPr = &Sw->Term[l1].NextPeer[m1][k1];
                                            if (!nPr->SwSrvFlg)
                                                continue;
                                            for (n1 = 0; n1 < MAX_PEERS; n1++) {
                                                Pr = &nPr->Peer[n1];
                                                if (!Pr->Cfg.SrvFlg || !Pr->P2PState)
                                                    continue;
                                                if (P2PNode.UgtOutPnt[1] >= P2P_PBUF_SIZE)
                                                    P2PNode.UgtOutPnt[1] = 0;
                                                nPnt = P2PNode.UgtOutPnt[1];
                                                nMp = &P2PNode.UgtOutPk[nPnt];
                                                *nMp = *Mp;
                                                nMh = &nMp->MsgHead;
                                                nMh->ToNextPeer = 1;
                                                nMh->SenderIP.IP = P2PNode.NodeID;
                                                nMh->SenderIP.SwSeqNum = k;
                                                nMh->SenderIP.TermSeqNum = l1;
                                                nMh->SenderIP.PeerSeqNum = m1;
                                                nMh->ReceiverIP.IP = Pr->Cfg.PeerNodeID;
                                                nMh->ReceiverIP.SwSeqNum = Pr->Cfg.PeerSwSeqNum;
                                                nMh->ReceiverIP.TermSeqNum = Pr->Cfg.PeerTermSeqNum;
                                                nMh->ReceiverIP.PeerSeqNum = Pr->Cfg.PeerSeqNum;
                                                nMp->MsgCnt.SrcCap.TimeOff += 0.2*(float)(m1 + k1 + n1);
                                                nMp->MsgState = 1;
                                                P2PNode.UgtOutPnt[1]++;
                                            }
                                        }
                                    }
                                }
                            }
                            else { // multiple switches
                                for (k1 = 0; k1 < MAX_SWITCHES; k1++) {
                                    nSw = &P2PNode.Switch[k1];
                                    l1 = (nSw->Term[0].HiddFlg) ? 1 : 0;
                                    if (nSw == Sw || !nSw->SrvFlg)
                                        continue;
                                    if (!nSw->SwStatus && !nSw->SwLocked) {
                                        nsdcSw = &SdcNode.SdcSw[k1];
                                        if (nsdcSw->DevCmd->Stat[0].type != CMDTYPE_ON && nSw->Term[l1].EngzFlg) { // Otehr side hot
                                            nsdcSw->DevCmd->Stat[0].type = CMDTYPE_ON;
                                            nsdcSw->DevCmd->Stat[0].exec = CMDEXEC_NO;
                                            nsdcSw->DevCmd->Stat[0].val = PHASE_ABC;
                                            nsdcSw->DevCmd->Stat[0].dur = 0.0;
                                            nsdcSw->DevCmd->Stat[0].chk = 1; // Close only if one side is hot after the timer up
                                            nsdcSw->DevCmd->ExecCntDn = (int) (Mp->MsgCnt.SrcCap.TimeOff/SdcNode.ExecIntv);
                                            if (nSw->Term[l].EngzFlg) { // From-terminal hot too, wait for it to be dead
                                                nsdcSw->DevCmd->Stat[0].chk = 2; // Close only if one side is hot within the timer
                                                nsdcSw->DevCmd->ExecCntDn = (int) (0.5 / SdcNode.ExecIntv);
                                            }
                                            break;
                                        }
                                    }
//                                    else if (nSw->SwStatus && !nSw->Term[l1].EngzFlg) { // Switch closed, continue
                                    else if (nSw->SwStatus) { // Switch closed, continue
                                        for (m1 = 0; m1 < MAX_PEERS; m1++) {
                                            Pr = &nSw->Term[l1].Peer[m1];
                                            if (!Pr->Cfg.SrvFlg)
                                                continue;
                                            if (P2PNode.UgtOutPnt[1] >= P2P_PBUF_SIZE)
                                                P2PNode.UgtOutPnt[1] = 0;
                                            nPnt = P2PNode.UgtOutPnt[1];
                                            nMp = &P2PNode.UgtOutPk[nPnt];
                                            *nMp = *Mp;
                                            nMh = &nMp->MsgHead;
                                            nMh->ToNextPeer = 0;
                                            nMh->SenderIP.IP = P2PNode.NodeID;
                                            nMh->SenderIP.SwSeqNum = k1;
                                            nMh->SenderIP.TermSeqNum = l1;
                                            nMh->SenderIP.PeerSeqNum = m1;
                                            nMh->ReceiverIP.IP = Pr->Cfg.PeerNodeID;
                                            nMh->ReceiverIP.SwSeqNum = Pr->Cfg.PeerSwSeqNum;
                                            nMh->ReceiverIP.TermSeqNum = Pr->Cfg.PeerTermSeqNum;
                                            nMh->ReceiverIP.PeerSeqNum = Pr->Cfg.PeerSeqNum;
                                            nMp->MsgCnt.SrcCap.TimeOff += 0.2 * (float) (k1 + m1);
                                            nMp->MsgState = 1;
                                            P2PNode.UgtOutPnt[1]++;
                                            if (!Pr->P2PState && !P2PNode.NonCommFDIR) { // Send to the Next level peers too
                                                for (i1 = 0; i1 < MAX_SWITCHES; i1++) {
                                                    nPr = &nSw->Term[l1].NextPeer[m1][i1];
                                                    if (!nPr->SwSrvFlg)
                                                        continue;
                                                    for (n1 = 0; n1 < MAX_PEERS; n1++) {
                                                        Pr = &nPr->Peer[n1];
                                                        if (!Pr->Cfg.SrvFlg || !Pr->P2PState)
                                                            continue;
                                                        if (P2PNode.UgtOutPnt[1] >= P2P_PBUF_SIZE)
                                                            P2PNode.UgtOutPnt[1] = 0;
                                                        nPnt = P2PNode.UgtOutPnt[1];
                                                        nMp = &P2PNode.UgtOutPk[nPnt];
                                                        *nMp = *Mp;
                                                        nMh = &nMp->MsgHead;
                                                        nMh->ToNextPeer = 1;
                                                        nMh->SenderIP.IP = P2PNode.NodeID;
                                                        nMh->SenderIP.SwSeqNum = k1;
                                                        nMh->SenderIP.TermSeqNum = l1;
                                                        nMh->SenderIP.PeerSeqNum = m1;
                                                        nMh->ReceiverIP.IP = nPr->Peer[n1].Cfg.PeerNodeID;
                                                        nMh->ReceiverIP.SwSeqNum = nPr->Peer[n1].Cfg.PeerSwSeqNum;
                                                        nMh->ReceiverIP.TermSeqNum = nPr->Peer[n1].Cfg.PeerTermSeqNum;
                                                        nMh->ReceiverIP.PeerSeqNum = nPr->Peer[n1].Cfg.PeerSeqNum;
                                                        nMp->MsgCnt.SrcCap.TimeOff += 0.2 * (float) (k1 + m1 + i1);
                                                        nMp->MsgState = 1;
                                                        P2PNode.UgtOutPnt[1]++;
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                    else if (Mh->CntType == CNT_CLEN_TIE) { // Clean Tie Infor and trace up to the root
                        if (Sw->SrvFlg && Sw->SwStatus && Sw->Term[l].EngzFlg && Sw->Term[l].TieIP.IP) {
                            Sw->Term[0].TieIP.IP = 0;
                            Sw->Term[1].TieIP.IP = 0;
                            j1 = (l == 0) ? 1 : 0;
                            if (!Sw->Term[j1].HiddFlg) { // Single switch node
                                for (l1 = 0; l1 < MAX_PEERS; l1++) {
                                    Pr = &Sw->Term[j1].Peer[l1];
                                    if (!Pr->Cfg.SrvFlg || Pr->TpDir != 1)
                                        continue;
                                    if (P2PNode.UgtOutPnt[1] >= P2P_PBUF_SIZE)
                                        P2PNode.UgtOutPnt[1] = 0;
                                    n = P2PNode.UgtOutPnt[1];
                                    Mp = &P2PNode.UgtOutPk[n];
                                    memset(Mp, 0, sizeof(P2P_MSGPACK));
                                    Mh = &Mp->MsgHead;
                                    Mh->ToNextPeer = 0;
                                    Mh->SenderIP.IP = P2PNode.NodeID;
                                    Mh->SenderIP.SwSeqNum = k;
                                    Mh->SenderIP.TermSeqNum = j1;
                                    Mh->SenderIP.PeerSeqNum = l1;
                                    Mh->ReceiverIP.IP = Pr->Cfg.PeerNodeID;
                                    Mh->ReceiverIP.SwSeqNum = Pr->Cfg.PeerSwSeqNum;
                                    Mh->ReceiverIP.TermSeqNum = Pr->Cfg.PeerTermSeqNum;
                                    Mh->ReceiverIP.PeerSeqNum = Pr->Cfg.PeerSeqNum;
                                    Mp->MsgState = 1; // Set Message ready for transmitting
                                    P2PNode.UgtOutPnt[1]++;
                                }
                            }
                            else { // Multiple switch node
                                for (i1 = 0; i1 < MAX_SWITCHES; i1++) {
                                    nSw = &P2PNode.Switch[i1];
                                    j1 = (nSw->Term[0].HiddFlg) ? 1 : 0;
                                    if (nSw == Sw || !nSw->SrvFlg || !nSw->SwStatus || !nSw->Term[j1].TieIP.IP)
                                        continue;
                                    nSw->Term[0].TieIP.IP = 0;
                                    nSw->Term[1].TieIP.IP = 0;
                                    if (!nSw->Term[j1].EngzFlg)
                                        continue;
                                    for (n1 = 0; n1 < MAX_PEERS; n1++) {
                                        Pr = &nSw->Term[j1].Peer[n1];
                                        if (!Pr->Cfg.SrvFlg || Pr->TpDir != 1)
                                            continue;
                                        if (P2PNode.UgtOutPnt[1] >= P2P_PBUF_SIZE)
                                            P2PNode.UgtOutPnt[1] = 0;
                                        n = P2PNode.UgtOutPnt[1];
                                        Mp = &P2PNode.UgtOutPk[n];
                                        memset(Mp, 0, sizeof(P2P_MSGPACK));
                                        Mh = &Mp->MsgHead;
                                        Mh->ToNextPeer = 0;
                                        Mh->SenderIP.IP = P2PNode.NodeID;
                                        Mh->SenderIP.SwSeqNum = i1;
                                        Mh->SenderIP.TermSeqNum = j1;
                                        Mh->SenderIP.PeerSeqNum = n1;
                                        Mh->ReceiverIP.IP = Pr->Cfg.PeerNodeID;
                                        Mh->ReceiverIP.SwSeqNum = Pr->Cfg.PeerSwSeqNum;
                                        Mh->ReceiverIP.TermSeqNum = Pr->Cfg.PeerTermSeqNum;
                                        Mh->ReceiverIP.PeerSeqNum = Pr->Cfg.PeerSeqNum;
                                        Mp->MsgState = 1; // Set Message ready for transmitting
                                        P2PNode.UgtOutPnt[1]++;
                                    }
                                }
                            }
                        }
                    }
                    break;
            }
        }
    }
    P2PNode.UgtInPnt[0] = (ePnt < P2P_PBUF_SIZE) ? ePnt : ePnt - P2P_PBUF_SIZE; // Set the read pnt to the most recent
}