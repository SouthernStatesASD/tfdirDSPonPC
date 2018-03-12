/*
 * TFDIR.h
 *
 *  Created on: Feb 12, 2018
 *      Author: user
 */
#include <vector>
#include "PhasorContainerV2.h"

#ifndef PC_DEBUG
#include "dspDebugLog.h"
    #include "TFDIR_MsgHandler.h"
#endif

using namespace std;

#ifndef TFDIRV2_H_
#define TFDIRV2_H_

class TFDIRV2 {
public:
    TFDIRV2(PhasorContainerV2 *_phasorContainer);
    virtual ~TFDIRV2();

    bool shouldCreateComtrade;

    void run();

#ifdef RUNNING_TEST
    void testSTC();
#endif

private:
    int EVT_LOG_Size;
    int EVT_Total_Size;
    int runCt;

    PHASOR abc[MES_PHASES];

    PhasorContainerV2 *PhasorContainerObj;

#ifndef PC_DEBUG
    TFDIR_MsgHandler tfdirMsgHandler;
#endif

    void SetAvg();
    void SetFault();
    void DetectFault();
    void CalDist(FLAG LchFlg);
    int EvtTrigger ();
    int TakeAction ();

};

#endif /* TFDIRV2_H_ */
