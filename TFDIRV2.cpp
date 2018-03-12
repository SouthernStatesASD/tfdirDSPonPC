/*
 * TFDIRV2.cpp
 *
 *  Created on: Feb 12, 2018
 *      Author: user
 */

#include <string>
#include "TFDIRV2.h"

#ifdef RUNNING_TEST
    #include <fstream>
    #include <streambuf>
#endif

TFDIRV2::TFDIRV2(PhasorContainerV2 * _phasorContainer)
{
    //****
    // set up the phasor container
    //**

    PhasorContainerObj = _phasorContainer;

    //****
    // set up the size vars
    //**
    EVT_LOG_Size = sizeof(EVT_LOG);
    EVT_Total_Size = EVT_SIZE*EVT_LOG_Size;

    runCt = 0;
}

TFDIRV2::~TFDIRV2() {
    // TODO Auto-generated destructor stub
}

void TFDIRV2::run() {

    if (PhasorContainerObj->configLoaded) {
//		printf("Run TFDIR");
        shouldCreateComtrade = false;
        PhasorContainerObj->calcPhasors();
        SetAvg();
        SetFault();
        DetectFault();
        TakeAction();

        runCt++;
        if (runCt == 5) {
//			printf("Reset Phasors & TFDIR");
            PhasorContainerObj->Configure(0, NULL);
            runCt=0;
        }
    }
}

// Calculate Power Quantities and Prefault Average
void TFDIRV2::SetAvg()		  // Set the V, I vectors from raw FFT vector buffers
{
    int   i, j, k, m;
    float x, y, z;
    XY 	    *b0, v;
    ICS_SET *s;
    ICS_SNS *sn;
    PHASOR   s2;

// Set Instaneous and Average Values
    for (i = 0; i < MAX_SETS; i++) {
        s = &PhasorContainerObj->IcsNode.IcsSet[i];
        if (!s->Cfg->SrvFlg) continue;  // the ICS Set is not installed, skip the Set
// Shift and Clear the Instaneous flags
        s->FltFlg0 = s->FltFlg;
        s->FltTyp0 = s->FltTyp;
        s->LineFlg0 = s->LineFlg;
        s->ChgFlg = 0;
        s->FltFlg = 0;
        s->FltTyp = 0;
        s->LineFlg = 0;
        for (j = 0; j < MES_PHASES; j++) {
            sn = &s->IcsSns[j];
//			if(!sn->Cfg->UseCT) continue; 		  // Skip this phase for current process
            memset(&sn->Iist, 0, PhasorContainerObj->PHASOR_Size); // clear I Instant
//			sn->PhsFlg0 = sn->PhsFlg;
//			sn->PhsFlg = 0;
            sn->FltTyp0 = sn->FltTyp;
            sn->FltTyp = 0;
            sn->IfRdyFlg = 0;
            b0 = &sn->Ibuf[0];

// Set Fault Current Instaneous Value
            sn->Iist.flg = b0->flg;
            sn->Iist.t = b0->t;
            if (sn->Iist.flg == DATA_NORMAL) { // SV data is good/valid, or a fault current
                sn->Iist.mag = PhasorContainerObj->XY2Mag(b0->x, b0->y);
                sn->Iist.ang = PhasorContainerObj->XY2Ang(b0->x, b0->y);
                if (sn->Iist.mag > PhasorContainerObj->IcsNode.Cfg->Imin) {
                    Setbit(s->LineFlg, sn->Phase); // Phase On
                    if (sn->Iist.mag >= s->Cfg->Ifm - s->Cfg->Ifdb &&
                        (Isbitset(sn->FltTyp0, FAULT_PHASE) || sn->Iist.mag >= s->Cfg->Ifm)) {
                        b0->flg = DATA_FAULT;
                        sn->Iist.flg = DATA_FAULT;
                        sn->FltTyp = FAULT_PHASE;	// Set to phase fault and clean other fault types
                    }
                }
                else { // I is too small
                    b0->flg = DATA_NO;
                    sn->Iist.flg = DATA_NO;
                }
// Phase Fault Average over the most recent 3 cycles
                m = 0;
                memset (&v, 0, PhasorContainerObj->XY_Size);
                v.flg = DATA_NO;
                sn->Ifa.t = sn->Iist.t;
                for (k = 0; k < FLT_BUF_SIZE; k++) {
                    m += sn->Ibuf[k].flg;		// count the fault flags over the last 3 cycles
                    v.x += sn->Ibuf[k].x/float (FLT_BUF_SIZE);
                    v.y += sn->Ibuf[k].y/float (FLT_BUF_SIZE);
                }
                if (m == DATA_NORMAL) {
                    sn->Ifa.mag = PhasorContainerObj->XY2Mag(v.x, v.y);
                    sn->Ifa.ang = PhasorContainerObj->XY2Ang(v.x, v.y);
                    v.flg = sn->Iist.flg;
                    sn->IfRdyFlg = AVERAGE_READY;
                }
                else if (m == FLT_BUF_SIZE*DATA_FAULT) { // discard the first cycle of fault
                    v.x = 0.9*sn->Ibuf[0].x + 0.1*sn->Ibuf[1].x;
                    v.y = 0.9*sn->Ibuf[0].y + 0.1*sn->Ibuf[1].y;
                    sn->Ifa.mag = PhasorContainerObj->XY2Mag(v.x, v.y);
                    sn->Ifa.ang = PhasorContainerObj->XY2Ang(v.x, v.y);
                    v.flg = DATA_FAULT;
                    sn->IfRdyFlg = AVERAGE_READY;
                }
                else if (sn->Iist.flg == DATA_FAULT) { // from normal to fault, use instaneous fault
                    v.x = b0->x;
                    v.y = b0->y;
                    v.flg = DATA_FAULT;
                    sn->IfRdyFlg = AVERAGE_1;
                    if (sn->Ibuf[1].flg == DATA_FAULT) { // 2 cycle average, 0.8 current cycle, 0.2 previous cycle
                        v.x = 0.9*b0->x + 0.1*sn->Ibuf[1].x;
                        v.y = 0.9*b0->y + 0.1*sn->Ibuf[1].y;
                        sn->IfRdyFlg = AVERAGE_2;
                    }
                    sn->Ifa.mag = PhasorContainerObj->XY2Mag(v.x, v.y);
                    sn->Ifa.ang = PhasorContainerObj->XY2Ang(v.x, v.y);
                }
                else {  // from fault to normal, set to instaneous current
                    sn->Ifa = sn->Iist;
                    v.flg = DATA_NO;
                    sn->IfRdyFlg = AVERAGE_NO;
                }
            }
// Prefault Current Average
            b0 = &sn->Ibuf[BUF_SIZE];
            if (b0->flg == DATA_NORMAL && sn->IaCnt > 0) {
                sn->IaCnt--;
                sn->Ia.x -= b0->x;
                sn->Ia.y -= b0->y;
            }
            b0 = &sn->Ibuf[FLT_BUF_SIZE];
            if (b0->flg == DATA_NORMAL) {
                sn->IaCnt++;
                sn->Ia.x += b0->x;
                sn->Ia.y += b0->y;
                sn->Ia.t  = b0->t;
            }
//			else if (sn->IaCnt <= AVG_BUF_SIZE/2) {
            else if (sn->IaCnt <= AVG_BUF_SIZE) {
                sn->IaCnt = 0;
                sn->Ia.x = 0.0;
                sn->Ia.y = 0.0;
                sn->IaRdyFlg = AVERAGE_NO;
                sn->Ia.flg = DATA_NO;
            }

//			if (sn->IaCnt+1 >= AVG_BUF_SIZE/2) {
            if (sn->IaCnt+1 >= AVG_BUF_SIZE) {
                sn->IaRdyFlg = AVERAGE_READY;
                sn->Ia.flg = DATA_NORMAL;
            }
            else {
                sn->IaRdyFlg = AVERAGE_NO;
                sn->Ia.flg = DATA_NO;
            }
            if (sn->IaCnt > 0) {
                x = -sn->Ia.x /float(sn->IaCnt);
                y = -sn->Ia.y /float(sn->IaCnt);
            }
            else {
                x = -b0->x;
                y = -b0->y;
            }
// Delta Change Current
//			if (sn->IaRdyFlg == AVERAGE_READY && sn->IfRdyFlg == AVERAGE_READY && v.flg == DATA_NORMAL) {
//			if (sn->IaRdyFlg == AVERAGE_READY && (v.flg == DATA_NORMAL || v.flg == DATA_FAULT)) {
            if (sn->IaCnt > AVG_BUF_SIZE) {
//				x += v.x;
//				y += v.y;
                x += sn->Ibuf[0].x;
                y += sn->Ibuf[0].y;
                sn->Ifc.mag = PhasorContainerObj->XY2Mag(x, y);
                sn->Ifc.ang = PhasorContainerObj->XY2Ang(x, y);
                sn->Ifc.t   = sn->Iist.t;
                sn->Ifc.flg = sn->Iist.flg;
            }
            else {
                memset(&sn->Ifc, 0, PhasorContainerObj->PHASOR_Size);
                sn->Ifc.flg = DATA_NO;
//				sn->Ifc = sn->Iist;
            }

// Instaneous Voltage Average (up to the most recent 3 cycles)
            if(sn->Cfg->UseVT) { 		// VT in use for this phase
                b0 = &sn->Vbuf[0];
                if (b0->flg == DATA_NORMAL) {
                    x = PhasorContainerObj->XY2Mag(b0->x, b0->y);
                    if (x < PhasorContainerObj->IcsNode.Cfg->Vmin ||  x > PhasorContainerObj->IcsNode.Cfg->Vmax)
                        b0->flg = DATA_NO;
                }
                m = 0;
                memset (&v, 0, PhasorContainerObj->XY_Size);
//				for (k = 0; k < FLT_BUF_SIZE; k++) {
                for (k = 0; k < 1; k++) {
                    b0 = &sn->Vbuf[k];
                    if (b0->flg == DATA_NORMAL) {
                        m++;
                        v.x += b0->x;
                        v.y += b0->y;
                        v.t  = b0->t;
                    }
                    else				// Stop countting when a bad record is encountered
                        break;
                }
                if (m > 0) {
                    sn->Vist.t = v.t;
                    sn->Vist.mag = PhasorContainerObj->XY2Mag(v.x, v.y)/float(m);
                    sn->Vist.ang = PhasorContainerObj->XY2Ang(v.x, v.y);
                    sn->Vist.t = v.t;
                    sn->Vist.flg = DATA_NORMAL;
                }
                else {
                    memset(&sn->Vist, 0, PhasorContainerObj->XY_Size);
                    sn->Vist.flg = DATA_NO;
                }
// Prefault Voltage Average
                b0 = &sn->Vbuf[BUF_SIZE];
                if (b0->flg == DATA_NORMAL && sn->VaCnt > 0) {
                    sn->VaCnt--;
                    sn->Va.x -= b0->x;
                    sn->Va.y -= b0->y;
                }
                b0 = &sn->Vbuf[FLT_BUF_SIZE];
                if (b0->flg == DATA_NORMAL) {
                    x = PhasorContainerObj->XY2Mag(b0->x, b0->y);
                    if (x >= PhasorContainerObj->IcsNode.Cfg->Vmin &&  x <= PhasorContainerObj->IcsNode.Cfg->Vmax) {
                        sn->VaCnt++;
                        sn->Va.x += b0->x;
                        sn->Va.y += b0->y;
                        sn->Va.t  = b0->t;
                    }
                }
                if (sn->VaCnt >= FLT_BUF_SIZE)
                    sn->Va.flg = DATA_NORMAL;
                else
                    sn->Va.flg = DATA_NO;
                if (sn->Va.flg == DATA_NORMAL) {
                    sn->Vref.mag = PhasorContainerObj->XY2Mag(sn->Va.x, sn->Va.y)/(float)sn->VaCnt;
                    sn->Vref.ang = PhasorContainerObj->XY2Ang(sn->Va.x, sn->Va.y);
                    sn->Vref.t = sn->Va.t;
                    sn->Vref.flg = DATA_NORMAL;
                }
                else if (sn->Vist.flg == DATA_NORMAL)
                    sn->Vref = sn->Vist;
                else if (sn->Vref.flg == DATA_NORMAL && sn->Ibuf[0].flg == DATA_NORMAL &&
                         PhasorContainerObj->TimeDiff (&sn->Ibuf[0].t, &sn->Vref.t) >= 0.8) { // no voltage signal for more than 0.8 seconds
                    memset(&sn->Vref, 0, PhasorContainerObj->PHASOR_Size);		 // Set reference voltage to zero
                    sn->Vref.flg = DATA_NO;
                }
            }
            memmove(&sn->Vbuf[1], &sn->Vbuf[0], BUF_SIZE*PhasorContainerObj->XY_Size);
            sn->Vbuf[0].flg = DATA_NO;
            memmove(&sn->Ibuf[1], &sn->Ibuf[0], BUF_SIZE*PhasorContainerObj->XY_Size);
            sn->Ibuf[0].flg = DATA_NO;
        }
// Calculate Neutral Current
//		if (s->LineFlg <= PHASE_ABC) { // all 3-phases are hot
        if (s->LineFlg > PHASE_NO) { // any phase is of current
            PhasorContainerObj->abcxy2s0(&s->I0, &s->IcsSns[0].Ibuf[1], &s->IcsSns[1].Ibuf[1], &s->IcsSns[2].Ibuf[1]);
            s->I0.t = s->IcsSns[0].Ibuf[1].t;
            if (s->I0.mag >= s->Cfg->Ifnm/3.0)
                s->NtrCnt ++;
            else
                s->NtrCnt = 0;
            if(s->I0.mag >= s->Cfg->Ifnm/3.0 && s->NtrCnt >= AVERAGE_2) {
//				if (!s->NtrFlg && Isbitset(s->FltTyp0, FAULT_NEUTRAL)) { // Find out faulted phase
                Setbit(s->FltTyp, FAULT_NEUTRAL);
                j = -1;
                if (!s->NtrFlg) { // Find out faulted phase
                    for (k = 0; k < 3; k++) {
                        abc[k].mag = PhasorContainerObj->XY2Mag(s->IcsSns[k].Ibuf[1].x, s->IcsSns[k].Ibuf[1].y);
                        abc[k].ang = PhasorContainerObj->XY2Ang(s->IcsSns[k].Ibuf[1].x, s->IcsSns[k].Ibuf[1].y);
                    }
                    PhasorContainerObj->abc2s2(&s2, &abc[0], &abc[1], &abc[2]); // Phase-A's S2
                    if (s2.mag > 1.5*s->I0.mag) { // Maybe an upstream high impedance fault
                        x = fabs(abc[0].mag - 0.5*(abc[1].mag + abc[2].mag)); // with Delta connected Taps in radial mode.
                        y = fabs(abc[1].mag - 0.5*(abc[0].mag + abc[2].mag)); // Use current magnitude difference
                        z = fabs(abc[2].mag - 0.5*(abc[0].mag + abc[1].mag)); // to determine the faulted phase
                        if (x > y && x > z) {
                            s->NtrFlg = PHASE_A;
                            j = 0;
                        }
                        else if (y > x && y > z) {
                            s->NtrFlg = PHASE_B;
                            j = 1;
                        }
                        else if (z > x && z > y) {
                            s->NtrFlg = PHASE_C;
                            j = 2;
                        }
                    }
                    else { // use negative sequence current direction to determine the faulted phase
                        x = (s->I0.ang >= 0.0) ? s->I0.ang : s->I0.ang + 360.0;
                        y = x - ((s2.ang >= 0.0) ? s2.ang : s2.ang + 360.0);
                        y = fmod(fabs(y), 360.0);
//					abc2s2(&s2, &abc[1], &abc[2], &abc[0]); // Phase-B's S2
                        z = s2.ang + 120.0;
                        z = x - ((z >= 0.0) ? z : z + 360.0);
                        z = fmod(fabs(z), 360.0);
                        if (y < z) {
                            s->NtrFlg = PHASE_A;
                            j = 0;
                        }
                        else {
                            s->NtrFlg = PHASE_B;
                            y = z;
                            j = 1;
                        }
//					abc2s2(&s2, &abc[2], &abc[0], &abc[1]); // Phase-C's S2
                        z = s2.ang - 120.0;
                        z = x - ((z >= 0.0) ? z : z + 360.0);
                        z = fmod(fabs(z), 360.0);
                        if (z < y) {
                            s->NtrFlg = PHASE_C;
                            j = 2;
                        }
                    }
                }
// Recalculate Ifa and Ifc for the faulted phase
                if (j >= 0) { // faulted phase identified {
                    sn = &s->IcsSns[j];
                    sn->Ifa.mag = abc[j].mag;
                    sn->Ifa.ang = abc[j].ang;
                    if(sn->IaCnt > 0) {
                        x = sn->Ibuf[1].x - sn->Ia.x /float(sn->IaCnt);
                        y = sn->Ibuf[1].y - sn->Ia.y /float(sn->IaCnt);
                    }
                    else {
                        x = sn->Ibuf[1].x;
                        y = sn->Ibuf[1].y;
                    }
                    sn->Ifc.mag = PhasorContainerObj->XY2Mag(x, y);
                    sn->Ifc.ang = PhasorContainerObj->XY2Ang(x, y);
                }
            }
            else if(s->NtrFlg)
                s->NtrFlg = 0;
        }
    }
// Assign Node Vref
    memset(&PhasorContainerObj->IcsNode.Vref, 0 , PhasorContainerObj->PHASOR_Size);
    PhasorContainerObj->IcsNode.Vref.flg = DATA_NO;
    m = 99;
    for (i = 0; i < MAX_SETS; i++) {
        s = &PhasorContainerObj->IcsNode.IcsSet[i];
        if (!s->Cfg->SrvFlg || s->Cfg->VrtFlg || !s->LineFlg)  // the ICS Set is not installed or line dead, skip the set
            continue;
//Set Reference Voltage: only one active reference votlage is needed per node
        for (j = 0; j < MES_PHASES; j++) {
            sn = &s->IcsSns[j];
            if(sn->Cfg->UseVT && sn->Cfg->RefVT && sn->Cfg->RefVT < m && sn->Vref.flg == DATA_NORMAL) {
                m = sn->Cfg->RefVT;				// RefVT is set with its highest priority, 1->10:H->L
                PhasorContainerObj->IcsNode.Vref = sn->Vref;
                if(sn->Phase == PHASE_B)
                    PhasorContainerObj->IcsNode.Vref.ang += 120.0;
                else if (sn->Phase == PHASE_C)
                    PhasorContainerObj->IcsNode.Vref.ang -= 120.0;
                PhasorContainerObj->IcsNode.Vref.flg = DATA_NORMAL;
            }
        }
    }
// Use Vref to set instaneous voltage for those phases without votlage sensors
    if (PhasorContainerObj->IcsNode.Vref.flg == DATA_NORMAL) {
        for (i = 0; i < MAX_SETS; i++) {
            s = &PhasorContainerObj->IcsNode.IcsSet[i];
            if (!s->Cfg->SrvFlg || !s->LineFlg)  // the ICS Set is not installed or line dead, skip the set
                continue;
            s->p = 0.0;
            s->q = 0.0;
            for (j = 0; j < MES_PHASES; j++) {
                sn = &s->IcsSns[j];
                if (!sn->Cfg->UseVT) {
                    sn->Vist = PhasorContainerObj->IcsNode.Vref;
                    if (sn->Phase == PHASE_B)
                        sn->Vist.ang -= 120.0;
                    else if (sn->Phase == PHASE_C)
                        sn->Vist.ang += 120.0;
                    x = PhasorContainerObj->TimeDiff(&sn->Iist.t, &sn->Vist.t)*PhasorContainerObj->IcsNode.Ms2Degrees;
                    sn->Vist = PhasorContainerObj->PhasorShift(&sn->Vist, x);
                    sn->Vist.t = sn->Iist.t;
                }
                PhasorContainerObj->VI2PQ (&x, &y, &sn->Vist, &sn->Iist);
                sn->p = 0.6*sn->p + 0.4*x;
                sn->q = 0.6*sn->q + 0.4*y;
                s->p += sn->p;
                s->q += sn->q;
            }
            s->pf = s->p/PhasorContainerObj->XY2Mag(s->p, s->q);
        }
    }
}

// Set Faults
void TFDIRV2::SetFault()		  // Set faults
{
    int      i, j, k;
    float    x;
    ICS_SET *s;
    ICS_SNS *sn;
    PHASOR 	 p, q;

    for (i = 0; i < MAX_SETS; i++) {
        s = &PhasorContainerObj->IcsNode.IcsSet[i];
        if (!s->Cfg->SrvFlg || !s->LineFlg)  // the ICS Set is not installed or line dead, skip the set
            continue;
// Set Instaneous Phase Fault
        for (j = 0; j < MES_PHASES; j++) {
            sn = &s->IcsSns[j];
//			if(!sn->Cfg->UseCT) continue; 		// no CT for this phase, skip current process
            if(Isbitset(sn->FltTyp, FAULT_PHASE)) { // phase fault
                Setbit(s->FltFlg, sn->Phase);
                Setbit(s->FltTyp, FAULT_PHASE);
            }
        }

        if (Isbitset(s->FltTyp, FAULT_PHASE) && s->FltCnt <= AVERAGE_READY) { // Phase fault
            for (j = 0; j < MES_PHASES; j++) {
                sn = &s->IcsSns[j];
//				if(!sn->Cfg->UseCT) continue; 		// Skip this phase for current process
                if(Isbitset(sn->FltTyp, FAULT_PHASE) && s->FltCnt < sn->IfRdyFlg)
                    s->FltCnt = sn->IfRdyFlg;
            }
            if (s->FltCnt == AVERAGE_READY)
                s->FltCnt += 1;						// mark as full
        }

// Check Delta and Set Delta Fault
        x = 0;
        for (j = 0; j < MES_PHASES; j++) {
            sn = &s->IcsSns[j];
//			if(!sn->Cfg->UseCT) continue; 		// Skip this phase for current process
            if (sn->Ifc.mag >= s->Cfg->Ifcm && x < sn->Ifc.mag) {
                x = sn->Ifc.mag;
                k = j;				// pick the biggest Ifc phase as the only faulted phase
            }
        }
        if (x > 1.0) { // check the Delta neutral current
            Setbit(s->ChgFlg, s->IcsSns[k].Phase);
            s->ChgCnt++;
            PhasorContainerObj->abc2s0(&p, &s->IcsSns[0].Ifc, &s->IcsSns[1].Ifc, &s->IcsSns[2].Ifc);
            PhasorContainerObj->abc2s2(&q, &s->IcsSns[0].Ifc, &s->IcsSns[1].Ifc, &s->IcsSns[2].Ifc);
            if (p.mag < 0.05*x && q.mag < 0.05*x) {  // Balanced change, more likely a small load change
                s->ChgCnt = 0;
                s->ChgFlg = 0;
            }
            else if (s->ChgFlg) {
                if(p.mag < 0.15*x && q.mag < 0.15*x) { // more likely a normal load change
                    s->ChgCnt = 0;
                    s->ChgFlg = 0;
                }
                else if(x >= s->Cfg->Ifcm && s->ChgCnt >= AVERAGE_2) {
                    PhasorContainerObj->abc2s0(&p, &s->IcsSns[0].Iist, &s->IcsSns[1].Iist, &s->IcsSns[2].Iist);
                    PhasorContainerObj->abc2s2(&q, &s->IcsSns[0].Iist, &s->IcsSns[1].Iist, &s->IcsSns[2].Iist);
                    if (p.mag >= 0.2*x || q.mag >= 0.2*x)  // unbalanced 3-phase current also
                        Setbit(s->FltTyp, FAULT_CHANGE);  // a candidate high impedance fault
                    else {
                        s->ChgCnt = 0;
                        s->ChgFlg = 0;
                    }
                }
            }
        }
        else {
            s->ChgCnt = 0;
            s->ChgFlg = 0;
            Clearbit(s->FltTyp, FAULT_CHANGE);
        }
    }
}

// Detect the Faults
void TFDIRV2::DetectFault()		// Fault detection
{
    int		 i, j, k, m;
    float    x;
    PHASOR   vr;
    ICS_SET *s;
    ICS_SNS *sn;
    EVT_LOG *evt;
    FLAG	 stat;
    FLAG	 hot = 0, firsthot=0, dead = 0, firstdead=0; // flag to mark the node just got hot or dead

    for (i = 0; i < MAX_SETS; i++) {
        s = &PhasorContainerObj->IcsNode.IcsSet[i];
        if (!s->Cfg->SrvFlg) { // the ICS Set is not installed, skip the set
            dead++;
//			firstdead++;
            continue;
        }
// Check energization status and its change
        if (!s->LineFlg) {   // line is dead now
            dead++;
            if (s->LineFlg0) { 				// line is from hot to dead
                firstdead++;
/*
				if (!s->FltLch && Isbitset(s->FltTyp0, FAULT_PHASE)) { // Instaneous phase fault not latched yet
					s->FltLch = s->FltFlg0;
					s->TypLch = FAULT_PHASE;
					IcsNode.LchFlg = 1;
					s->FltCnt = 0;
				}
				else if (!IcsNode.LchFlg && !s->NtrLch && Isbitset(s->FltTyp0, FAULT_NEUTRAL)) {// Neutral fault not latched yet
					s->NtrLch = s->FltFlg0;
					s->TypLch = FAULT_NEUTRAL;
					IcsNode.LchFlg = 1;
				}
				else if (!IcsNode.LchFlg && !s->ChgLch && Isbitset(s->FltTyp0, FAULT_CHANGE)) {// Delat Change fault not latched yet
					s->ChgLch = s->FltFlg0;
					s->TypLch = FAULT_CHANGE;
					IcsNode.LchFlg = 1;
					s->ChgCnt = 0;
				}
*/
            }
        }
        else {	// line is hot
            hot++;
            if(!s->LineFlg0)  // line is from dead to hot
                firsthot++;
// Latch Instaneous Phase Fault
            if (!s->FltLch) { // No latched yet
//				if ((Isbitset(s->FltTyp0, FAULT_PHASE) && !Isbitset(s->FltTyp, FAULT_PHASE)) || // fault disappeared
//					s->FltCnt > AVERAGE_READY) { // 3 cycles of fault captured
                if(s->FltCnt > AVERAGE_READY) { // 3 cycles of fault captured
                    s->FltLch = s->FltFlg;
                    s->TypLch = FAULT_PHASE;
                    if (!PhasorContainerObj->IcsNode.LchFlg)
                        PhasorContainerObj->IcsNode.LchFlg = 1;
                    if (s->FltCnt > AVERAGE_READY)
                        s->IfTimer = 3.0*PhasorContainerObj->IcsNode.Cycle2Sec;
                    else
                        s->IfTimer = 0;
                    s->FltCnt = 0;
                    s->IfRtnTimer = 0.0;
                }
            }
            else { // Fault latched
                if (Isbitset(s->FltTyp, FAULT_PHASE)) {
                    s->IfRtnTimer = 0.0;
                    s->IfTimer += PhasorContainerObj->IcsNode.Cycle2Sec;
                    if (s->IfTimer >= PhasorContainerObj->IcsNode.Cfg->FltTime)  // fault too long
                        Setbit(PhasorContainerObj->IcsNode.Flt2Long, FAULT_PHASE);
                }
                else {
                    s->IfTimer = 0.0;
                    s->IfRtnTimer += PhasorContainerObj->IcsNode.Cycle2Sec;
                    if (s->IfRtnTimer >= PhasorContainerObj->IcsNode.Cfg->RtnTime) {
                        Setbit(PhasorContainerObj->IcsNode.FltLck, EVENT_MOMENTARY);
                        s->FltLch = 0;
                    }
                }
            }

// Latch Other faults
//			if (!s->FltLch && !Isbitset(s->FltTyp, FAULT_PHASE)) { // no Instaneous phase fault occurs
            if (!s->FltLch) {
// Latch Neutral Fault
                if (!s->NtrLch) { // No Neutral fault latched yet
                    if (Isbitset(s->FltTyp, FAULT_NEUTRAL)) {
                        if (!Isbitset(s->FltTyp0, FAULT_NEUTRAL)) { // initialize fault time counter
                            s->IfnTimer = PhasorContainerObj->IcsNode.Cycle2Sec;
                            s->IfnRtnTimer = 0.0;
                        }
                        else if (Isbitset(s->FltTyp0, FAULT_NEUTRAL)) { // Neutral fault lasted for two cycles
                            s->NtrLch = s->NtrFlg;
                            Setbit(s->TypLch, FAULT_NEUTRAL);
                            if (!PhasorContainerObj->IcsNode.LchFlg)
                                PhasorContainerObj->IcsNode.LchFlg = 1;
                        }
                    }
                }
                else { // Neutral fault latched
                    if (Isbitset(s->FltTyp, FAULT_NEUTRAL))
                        s->IfnTimer += PhasorContainerObj->IcsNode.Cycle2Sec;
                    else if (!Isbitset(s->FltTyp, FAULT_NEUTRAL) && Isbitset(s->FltTyp0, FAULT_NEUTRAL)) {
                        s->IfnRtnTimer = PhasorContainerObj->IcsNode.Cycle2Sec;
                        s->IfnTimer = 0.0;
                    }
                    else
                        s->IfnRtnTimer += PhasorContainerObj->IcsNode.Cycle2Sec;
                    if (s->IfnTimer > PhasorContainerObj->IcsNode.Cfg->FltTime) { // fault too long
                        Setbit(PhasorContainerObj->IcsNode.Flt2Long, FAULT_NEUTRAL);
                        s->IfnTimer = 0.0;
                    }
                    else if (s->IfnRtnTimer > PhasorContainerObj->IcsNode.Cfg->RtnTime) {
                        Setbit(PhasorContainerObj->IcsNode.FltLck, EVENT_MOMENTARY);
                        s->IfnRtnTimer = 0.0;
                        s->NtrLch = 0;
                    }
                }
// Latch Delta Fault
                if (!s->NtrLch) {
                    if (!s->ChgLch) { // No Change and Neutral Fault latched yet
                        if (Isbitset(s->FltTyp, FAULT_CHANGE)) { // Delta fault detected
                            if (!Isbitset(s->FltTyp0, FAULT_CHANGE)) // initialize fault time counter
                                s->IfcTimer = 0;
                            else if (Isbitset(s->FltTyp0, FAULT_CHANGE)) { // Delta change fault lasted for two cycles
                                s->ChgLch = s->ChgFlg;
                                Setbit(s->TypLch, FAULT_CHANGE);
                                if (!PhasorContainerObj->IcsNode.LchFlg)
                                    PhasorContainerObj->IcsNode.LchFlg = 1;
                            }
                            s->IfcTimer = PhasorContainerObj->IcsNode.Cycle2Sec;
                        }
                    }
                    else {
                        s->IfcTimer += PhasorContainerObj->IcsNode.Cycle2Sec;  // Keep counting Delta change timer
                        if (s->IfcTimer > PhasorContainerObj->IcsNode.Cfg->FltTime) {
                            Setbit(PhasorContainerObj->IcsNode.FltLck, EVENT_MOMENTARY);
                            s->ChgLch = 0;
                            s->IfcTimer = 0.0;
                        }
                    }
                }
// Latch for Delta Report
//				if (!IcsNode.LchFlg && s->ChgCnt > 0) // Report the Phasor for fault detection at the Master Station
//					IcsNode.RptChg = 1;
            }
        }
    }
// Latch fault current for all sets if a fault is detected in any set, any phase with any type
    if (PhasorContainerObj->IcsNode.LchFlg == 1) { // fault is detected in at least one ICS Set, set all ILch.
        k = (PhasorContainerObj->IcsNode.TrpCnt < EVT_SIZE) ? PhasorContainerObj->IcsNode.TrpCnt : EVT_SIZE-1;
        for (i = 0; i < MAX_SETS; i++) {
            s = &PhasorContainerObj->IcsNode.IcsSet[i];
            if (!s->Cfg->SrvFlg)  // the ICS Set is not installed or line dead, skip the set
                continue;
            evt = &s->Evt[k];
            memset(evt, 0, EVT_LOG_Size);
            evt->FltTyp = s->TypLch;
            evt->Vref = PhasorContainerObj->IcsNode.Vref;
            m = -1;
            for (j = 0; j < MES_PHASES; j++) {
//				if (s->IcsSns[j].Ifa.mag >= IcsNode.Cfg->Imin) {
                if (s->IcsSns[j].Iist.mag >= PhasorContainerObj->IcsNode.Cfg->Imin) {
                    m = j;
                    break;
                }
            }
            if (m < 0)
                continue;

//			x = TimeDiff(&evt->Vref.t, &s->IcsSns[m].Ifa.t); // + is Vref leading Ifa
            x = PhasorContainerObj->TimeDiff(&evt->Vref.t, &s->IcsSns[m].Iist.t); // + is Vref leading Ifa
            x *= PhasorContainerObj->IcsNode.Ms2Degrees;
            evt->Vref = PhasorContainerObj->PhasorShift(&evt->Vref, -x); // Set Vref angle vs. phase Ia
//			evt->Vref.t = s->IcsSns[m].Ifa.t;  // Align Vref timestamp to Ifa
            evt->Vref.t = s->IcsSns[m].Iist.t;  // Align Vref timestamp to Ifa

            for (j = 0; j < MES_PHASES; j++) {
                sn = &s->IcsSns[j];
//			if(!sn->Cfg->UseCT) continue; 		// Skip this phase for current process
//				x = 1000.0*IcsNode.Degrees2Ms*s->IcsSns[j].Ifa.ang;
//				evt->If[j] = sn->Ifa;
                evt->If[j] = sn->Iist;
//				evt->If[j].t.usec -= x;
                evt->Ifc[j] = sn->Ifc;			// Ifc is of the same time stamp as Ifa
//				evt->Ifc[j].t.usec -= x;
                evt->Ia[j] = sn->Ia;            // prefault I average in XY, would be zero for start up case
                evt->Va[j] = sn->Va;            // prefault V average in XY, would be zero for start up case
                evt->Vf[j] = sn->Vist;			// Fault voltage in phasor
            }
            if(PhasorContainerObj->IcsNode.VoltSetNum < 4)	{	// Use Fault voltage Set's voltage, it is selected
                for (j = 0; j < MES_PHASES; j++) {
                    evt->Vf[j] = PhasorContainerObj->IcsNode.IcsSet[PhasorContainerObj->IcsNode.VoltSetNum].IcsSns[j].Vist;
                }
            }
// Calculate fault I0, V0 and I2, V2 and Zs0, Zs2.
            PhasorContainerObj->abc2s0(&evt->I0, &evt->If[0], &evt->If[1], &evt->If[2]);
            PhasorContainerObj->abc2s2(&evt->I2, &evt->If[0], &evt->If[1], &evt->If[2]);
//			abc2s0(&evt->V0, &evt->Vf[0], &evt->Vf[1], &evt->Vf[2]);
//			abc2s2(&evt->V2, &evt->Vf[0], &evt->Vf[1], &evt->Vf[2]);
//			evt->Zs0.r = evt->Zs0.x = 0.0;
//			evt->Zs2.r = evt->Zs2.x = 0.0;
/**
			if (evt->FltFlg != PHASE_ABC) {
//				x = (evt->I0.mag + evt->I2.mag)/3.0;
//				if (evt->I0.mag >= x) {
//					evt->Zs0.r = 1000.0*fabs(cos(DEGREES2PI*(evt->V0.ang-evt->I0.ang))*evt->V0.mag/evt->I0.mag);
//					evt->Zs0.x = 1000.0*fabs(sin(DEGREES2PI*(evt->V0.ang-evt->I0.ang))*evt->V0.mag/evt->I0.mag);
//				}
				evt->Zs0.r = 1000.0*fabs(cos(DEGREES2PI*(evt->V0.ang-evt->I0.ang))*evt->V0.mag/evt->I0.mag);
				evt->Zs0.x = 1000.0*fabs(sin(DEGREES2PI*(evt->V0.ang-evt->I0.ang))*evt->V0.mag/evt->I0.mag);
				evt->Zs2.r = 1000.0*fabs(cos(DEGREES2PI*(evt->V2.ang-evt->I2.ang))*evt->V2.mag/evt->I2.mag);
				evt->Zs2.x = 1000.0*fabs(sin(DEGREES2PI*(evt->V2.ang-evt->I2.ang))*evt->V2.mag/evt->I2.mag);
			}
**/
// Check and process non-latched fault
            if(s->FltTyp && !s->TypLch) { // The set has non-latched fault, set fault latch flag
                if (Isbitset(s->FltTyp, FAULT_PHASE))
                    Setbit(s->TypLch, FAULT_PHASE);
                else if (Isbitset(s->FltTyp, FAULT_NEUTRAL)) {
                    Setbit(s->TypLch, FAULT_NEUTRAL);
                    s->NtrLch = s->NtrFlg;
                }
                else
                    Setbit(s->TypLch, FAULT_CHANGE);
                evt->FltTyp = s->TypLch;
            }

// Phase Instaneous Fault
            if (Isbitset(s->TypLch, FAULT_PHASE)) {  // Find out Fault Direction and current phasor
                evt->FltFlg = s->FltLch;
                for (j = 0; j < MES_PHASES; j++) { // set fault direction for the faulted phase
//				if(!s->IcsSns[j].Cfg->UseCT) continue; 		// Skip this phase for current process
                    sn = &s->IcsSns[j];
                    x = 100001.0;
                    if(sn->Phase == PHASE_A && Isbitset(evt->FltFlg, PHASE_A)) {
                        x = evt->If[j].ang - evt->Vref.ang;
                    }
                    else if(sn->Phase == PHASE_B && Isbitset(evt->FltFlg, PHASE_B)) {
                        x = evt->If[j].ang - evt->Vref.ang + 120.0;
                    }
                    else if(sn->Phase == PHASE_C && Isbitset(evt->FltFlg, PHASE_C)) {
                        x = evt->If[j].ang - evt->Vref.ang - 120.0;
                    }

                    if (x <= 100000.0) {
                        if (evt->Vref.flg != DATA_NORMAL) {
                            evt->FltDir[j] = FAULT_UNKNOWN;
                            continue;
                        }
                        vr.ang = 0.0;
                        vr = PhasorContainerObj->PhasorShift(&vr, x);
                        x = vr.ang;
                        if (x <= PhasorContainerObj->IcsNode.ToqAngLead && x >= PhasorContainerObj->IcsNode.ToqAngLag)
                            evt->FltDir[j] = FAULT_FORWARD;
                        else if (x >= 180.0+PhasorContainerObj->IcsNode.ToqAngLag || x <= PhasorContainerObj->IcsNode.ToqAngLead-180.0)
                            evt->FltDir[j] = FAULT_BACKWARD;
                        else
                            evt->FltDir[j] = FAULT_UNKNOWN;
                    }
                }
            }
// Process Neutral Fault
            else if (Isbitset(s->TypLch, FAULT_NEUTRAL)) { // Find out Fault Direction
                evt->FltFlg = s->NtrLch;
                x = evt->I0.ang - evt->Vref.ang;	// use I0 to determine the fault direction
                j = 0;
                if (Isbitset(evt->FltFlg, PHASE_B))
                    j = 1;
                else if(Isbitset(evt->FltFlg, PHASE_C))
                    j = 2;

                if (evt->I2.mag > 1.5*evt->I0.mag) { // Assume delta taps and upstream high impedance fault
                    if (Isbitset(s->FltTyp, FAULT_CHANGE))
                        x = evt->Ifc[j].ang - evt->Vref.ang; // use Delta If to determine the fault direction
                    else {
                        x = evt->If[j].ang - evt->Vref.ang + 180.0; // Set fault direction reverse to If
                    }
                }
                x += (float)j*120;
                vr.ang = 0.0;
                vr = PhasorContainerObj->PhasorShift(&vr, x);
                x = vr.ang;
                if (x <= PhasorContainerObj->IcsNode.ToqAngLead && x >= PhasorContainerObj->IcsNode.ToqAngLag)
                    evt->FltDir[j] = FAULT_FORWARD;
                else if (x >= 180.0+PhasorContainerObj->IcsNode.ToqAngLag || x <= PhasorContainerObj->IcsNode.ToqAngLead-180.0)
                    evt->FltDir[j] = FAULT_BACKWARD;
                else
                    evt->FltDir[j] = FAULT_UNKNOWN;

                if(evt->Vref.flg != DATA_NORMAL)
                    evt->FltDir[j] = FAULT_UNKNOWN;
            }
// Process Delta Change Fault
            else if (Isbitset(s->TypLch, FAULT_CHANGE)) { // Find out Fault Direction and phasor
                evt->FltFlg = s->ChgLch;
                for (j = 0; j < MES_PHASES; j++) { // set fault direction for the faulted phase
//				if(!s->IcsSns[j].Cfg->UseCT) continue; 		// Skip this phase for current process
                    sn = &s->IcsSns[j];
                    if(sn->Phase == PHASE_A && Isbitset(evt->FltFlg, PHASE_A))
                        x = evt->Ifc[j].ang - evt->Vref.ang;
                    else if(sn->Phase == PHASE_B && Isbitset(evt->FltFlg, PHASE_B)) {
                        x = evt->Ifc[j].ang - evt->Vref.ang + 120.0;
                    }
                    else if(sn->Phase == PHASE_C && Isbitset(evt->FltFlg, PHASE_C)) {
                        x = evt->Ifc[j].ang - evt->Vref.ang - 120.0;
                    }
                    else
                        x = 100001.0;
                    if (x <= 100000.0) {
                        if (evt->Vref.flg != DATA_NORMAL) {
                            evt->FltDir[j] = FAULT_UNKNOWN;
                            continue;
                        }
                        vr.ang = 0.0;
                        vr = PhasorContainerObj->PhasorShift(&vr, x);
                        x = vr.ang;
                        if (x <= PhasorContainerObj->IcsNode.ToqAngLead && x >= PhasorContainerObj->IcsNode.ToqAngLag)
                            evt->FltDir[j] = FAULT_FORWARD;
                        else if (x >= 180.0+PhasorContainerObj->IcsNode.ToqAngLag || x <= PhasorContainerObj->IcsNode.ToqAngLead-180.0)
                            evt->FltDir[j] = FAULT_BACKWARD;
                        else
                            evt->FltDir[j] = FAULT_UNKNOWN;
                    }
                }
            }
            else
                evt->FltFlg = FAULT_NO;
// Get the resultant fault direction
// For 2-phase fault, use the Direction of phase A if AB fault (A+, B-), B if BC fault and C if AC fault
            if (evt->FltFlg) {
                if (evt->FltDir[0] && !evt->FltDir[1] && !evt->FltDir[2]) // A
                    evt->FltDirSum = evt->FltDir[0];
                else if (!evt->FltDir[0] && evt->FltDir[1] && !evt->FltDir[2]) // B
                    evt->FltDirSum = evt->FltDir[1];
                else if (!evt->FltDir[0] && !evt->FltDir[1] && evt->FltDir[2]) // C
                    evt->FltDirSum = evt->FltDir[2];
                else if (evt->FltDir[0] && evt->FltDir[1] && evt->FltDir[2]) { // ABC
                    evt->FltDirSum = evt->FltDir[0];
                    if (evt->FltDir[0] == FAULT_UNKNOWN && evt->FltDir[1] < FAULT_UNKNOWN)
                        evt->FltDirSum = evt->FltDir[1];
                    else if (evt->FltDir[0] == FAULT_UNKNOWN && evt->FltDir[2] < FAULT_UNKNOWN)
                        evt->FltDirSum = evt->FltDir[2];
                }
                else if(evt->FltDir[0] && evt->FltDir[1]) { // AB
                    evt->FltDirSum = evt->FltDir[0];
                    if (evt->FltDir[0] == FAULT_UNKNOWN && evt->FltDir[1] < FAULT_UNKNOWN)
                        evt->FltDirSum = (evt->FltDir[1] == FAULT_FORWARD) ? FAULT_BACKWARD : FAULT_FORWARD;
                }
                else if(evt->FltDir[1] && evt->FltDir[2]) { // BC
                    evt->FltDirSum = evt->FltDir[1];
                    if (evt->FltDir[1] == FAULT_UNKNOWN && evt->FltDir[2] < FAULT_UNKNOWN)
                        evt->FltDirSum = (evt->FltDir[2] == FAULT_FORWARD) ? FAULT_BACKWARD : FAULT_FORWARD;
                }
                else if(evt->FltDir[2] && evt->FltDir[0]) { // CA
                    evt->FltDirSum = evt->FltDir[2];
                    if (evt->FltDir[2] == FAULT_UNKNOWN && evt->FltDir[0] < FAULT_UNKNOWN)
                        evt->FltDirSum = (evt->FltDir[0] == FAULT_FORWARD) ? FAULT_BACKWARD : FAULT_FORWARD;
                }
            }
        }

        CalDist(PhasorContainerObj->IcsNode.LchFlg); // Calculate fault distance if fault is in the sensor's side (reversed direction)

        PhasorContainerObj->IcsNode.LchFlg = 2;	// Prepare the trip type to wait for a fault trip
    }
    else if (PhasorContainerObj->IcsNode.LchFlg > 1)
        CalDist(PhasorContainerObj->IcsNode.LchFlg);

// Report Delta Change for fault detection at Master
/***
	if (!IcsNode.LchFlg && IcsNode.RptChg) {
		if (IcsNode.RptChgTimer <= IcsNode.Cycle2Sec) {
			for (i = 0; i < MAX_SETS; i++) {
				s = &PhasorContainerObj->IcsNode.IcsSet[i];
				if (!s->Cfg->SrvFlg)  // the ICS Set is not installed or line dead, skip the set
					continue;
				evt = &s->Evt[EVT_SIZE-1]; // use the last record of Evt
				for (j = 0; j < MES_PHASES; j++) {
					sn = &s->IcsSns[j];
//			if(!sn->Cfg->UseCT) continue; 		// Skip this phase for current process
//					evt->If[j] = sn->Ifa;
					evt->If[j] = sn->Iist;
					evt->If[j].ang = 0.0;       // the angle should be zero to represent the zero crossing
//					evt->If[j].t.usec -= 1000.0*sn->Ifa.ang*IcsNode.Degrees2Ms; // timestamp is adjusred to zero-crossing
					evt->If[j].t.usec -= 1000.0*sn->Iist.ang*IcsNode.Degrees2Ms; // timestamp is adjusred to zero-crossing
				}
			}
		}
		else {
			IcsNode.RptChgTimer = (IcsNode.RptChgTimer < IcsNode.Cfg->RptChgTime)?
									IcsNode.RptChgTimer + IcsNode.Cycle2Sec : 0.0; 		// Do not report too frequently
			IcsNode.RptChg = 0;
		}
	}
***/
// Proccess hot/dead scenarios
    if (hot) { // at least one line is hot
        if (firsthot == hot) {
            PhasorContainerObj->IcsNode.DnTimer = 0.0;
            if (PhasorContainerObj->IcsNode.TrpCnt > 0)
                PhasorContainerObj->IcsNode.RclCnt += (PhasorContainerObj->IcsNode.RclCnt < PhasorContainerObj->IcsNode.Cfg->MaxRclNum) ? 1 : 0;
        }
        PhasorContainerObj->IcsNode.UpTimer += PhasorContainerObj->IcsNode.Cycle2Sec;
        if (PhasorContainerObj->IcsNode.UpTimer >= PhasorContainerObj->IcsNode.Cfg->HotTime)  // Line is successfully energized no trip again
            if (PhasorContainerObj->IcsNode.TrpCnt > 0)  // Reclose successful
                PhasorContainerObj->IcsNode.FltLck = EVENT_TEMPORARY;
    }
    else if (dead == MAX_SETS) { // all lines dead
        if (firstdead > 0) {
//			if(IcsNode.LchFlg == 2)
            PhasorContainerObj->IcsNode.TrpCnt++;
            if(PhasorContainerObj->IcsNode.Cfg->AutoSwitchOpen) { // Auto open switch for the faulted zone
                for (i = 0; i < MAX_SETS; i++) {
                    s = &PhasorContainerObj->IcsNode.IcsSet[i];
                    if (!s->Cfg->SrvFlg)
                        continue;
                    if (s->Evt[0].FltDirSum == FAULT_BACKWARD || (PhasorContainerObj->IcsNode.TrpCnt == 2 && s->Evt[1].FltDirSum == FAULT_BACKWARD)) {
                        if ((PhasorContainerObj->IcsNode.TrpCnt == 2 && (s->Evt[0].FltZone == 1 || s->Evt[1].FltZone == 1 || s->Cfg->SetTyp == SET_TYPE_TAP)) ||
                            (PhasorContainerObj->IcsNode.TrpCnt == 3 && (s->Evt[0].FltZone <= 2 || s->Evt[1].FltZone <= 2 || s->Evt[2].FltZone <= 2)) ||
                            (PhasorContainerObj->IcsNode.TrpCnt == 1 && (s->Evt[0].FltZone == 1 || s->Cfg->SetTyp == SET_TYPE_TAP) &&
                             PhasorContainerObj->IcsNode.DnTimer >= PhasorContainerObj->IcsNode.Cfg->RclWaitTime)) {
                            s->FltZone = (s->FltZone > s->Evt[0].FltZone || s->FltZone <= 0)? s->Evt[0].FltZone : s->FltZone;
                            if (PhasorContainerObj->IcsNode.TrpCnt == 2 && (s->FltZone > s->Evt[1].FltZone || s->FltZone <= 0))
                                s->FltZone = s->Evt[1].FltZone;
                            else if (PhasorContainerObj->IcsNode.TrpCnt == 3 && (s->FltZone > s->Evt[2].FltZone || s->FltZone <= 0))
                                s->FltZone = s->Evt[2].FltZone;
                            s->OpenSw = (s->OpenSw == 0)? 1 : s->OpenSw;
                            if (s->Cfg->SetTyp == SET_TYPE_TAP && !s->Cfg->SwitchEnabled) { // Tap switch is not available to open
                                stat = s->FltZone;
                                for (j = 0; j < MAX_SETS; j++) {
                                    s = &PhasorContainerObj->IcsNode.IcsSet[j];
                                    if (!s->Cfg->SrvFlg)
                                        continue;
                                    if (s->Cfg->SetTyp == SET_TYPE_LINE) {
                                        s->FltZone = (s->OpenSw == 0)? stat : s->FltZone;
                                        s->OpenSw = (s->OpenSw == 0)? 1 : s->OpenSw;
                                    }
                                }
                            }
                        }
                    }
                }
            }
// Clean Latch flags for each set
            PhasorContainerObj->IcsNode.UpTimer = 0.0;
            for (i = 0; i < MAX_SETS; i++) {
                s = &PhasorContainerObj->IcsNode.IcsSet[i];
                s->ChgLch = 0;
                s->FltLch = 0;
                s->NtrLch = 0;
                s->TypLch = 0;
                s->FltCnt = 0;
                s->NtrCnt = 0;
                s->ChgCnt = 0;
            }
            PhasorContainerObj->IcsNode.LchFlg = 0;
        }

        PhasorContainerObj->IcsNode.DnTimer += PhasorContainerObj->IcsNode.Cycle2Sec;    // count dead time in seconds, add one cycle time
        if (PhasorContainerObj->IcsNode.TrpCnt > 0) {
            if (PhasorContainerObj->IcsNode.RclCnt >= PhasorContainerObj->IcsNode.Cfg->MaxRclNum || // Max rcls number reached
                PhasorContainerObj->IcsNode.DnTimer > PhasorContainerObj->IcsNode.Cfg->DeadTime ||
                PhasorContainerObj->IcsNode.DnTimer > PhasorContainerObj->IcsNode.Cfg->RclWaitTime) // Line is permernently dead and locked
                PhasorContainerObj->IcsNode.FltLck = EVENT_PERMANENT;
        }
// Clear SV buffers and Prefault averages
        if (firstdead) {
            for (i = 0; i < MAX_SETS; i++) {
                s = &PhasorContainerObj->IcsNode.IcsSet[i];
                for (j = 0; j < MES_PHASES; j++) {
                    sn = &s->IcsSns[j];
                    memset(&sn->Ia, 0, PhasorContainerObj->XY_Size);
                    sn->Ia.flg = DATA_NO;
                    sn->IaCnt = 0;
                    memset (sn->Ibuf, 0, (BUF_SIZE+1)*PhasorContainerObj->XY_Size);
                    memset (sn->Vbuf, 0, (BUF_SIZE+1)*PhasorContainerObj->XY_Size);
                    for (k = 0; k <= BUF_SIZE; k++) {
                        sn->Ibuf[k].flg = DATA_NO;
                        sn->Vbuf[k].flg = DATA_NO;
                    }
                }
            }
        }
    }
}

void TFDIRV2::CalDist(FLAG LchFlg)
{
/*
  Calculate fault distance, if fault direction is in the sensor's side (reversed direction),
  and if trip count k = 0 (start the first trip) and single phase fault, use both methods (double-ends and single end),
  otherwise use the single-end impedance method only.
*/
    int		i, j, k, m, n;
    float	a, r, x;
    RX		Zs0, Zs2;
    RX		Zls0, Zls2;
    RX		I0, Z2;
//	RX		Zr0, Zr2;
//	RX		Zlr0, Zlr2;
//	RX		Zl0, Zl2;
//	RX		Z0, I2;
    RX		W1, W2, W3;
    PHASOR  VI0;
    ICS_SET *s;
    EVT_LOG *evt = NULL;
    static EVT_LOG localEvt;
    static int count = 0;

    k = -1;
    j = -1;
    m = 0;
    for (i = 0; i < MAX_SETS; i++) {
        s = &PhasorContainerObj->IcsNode.IcsSet[i];
        if (!s->Cfg->SrvFlg)  // the ICS Set is not in service, skip the set
            continue;
        evt = &s->Evt[PhasorContainerObj->IcsNode.TrpCnt];
        if(evt->FltDirSum == FAULT_BACKWARD && k < 0) {
            k = i;
            Zs0    = s->Cfg->Zs0;
            Zls0.r = s->Cfg->Z0.r*s->Cfg->LineLength;
            Zls0.x = s->Cfg->Z0.x*s->Cfg->LineLength;
            Zs2    = s->Cfg->Zs1;
            Zls2.r = s->Cfg->Z1.r*s->Cfg->LineLength;
            Zls2.x = s->Cfg->Z1.x*s->Cfg->LineLength;
            if (s->Cfg->SetTyp == SET_TYPE_LINE)
                m++;
        }
/***
		else if (s->Cfg->SetTyp == SET_TYPE_LINE && evt->FltDirSum == FAULT_FORWARD && IcsNode.TrpCnt == 0 && j < 0) {
			j = i;
			Zr0    = s->Cfg->Zs0;
			Zlr0.r = s->Cfg->Z0.r*s->Cfg->LineLength;
			Zlr0.x = s->Cfg->Z0.x*s->Cfg->LineLength;
			Zr2    = s->Cfg->Zs1;
			Zlr2.r = s->Cfg->Z1.r*s->Cfg->LineLength;
			Zlr2.x = s->Cfg->Z1.x*s->Cfg->LineLength;
			m++;
		}
***/
    }

    s = (k >= 0)? & PhasorContainerObj->IcsNode.IcsSet[k] : NULL;
    if(s) {
        if(LchFlg == 1) {
            evt = &s->Evt[PhasorContainerObj->IcsNode.TrpCnt];
            localEvt = s->Evt[PhasorContainerObj->IcsNode.TrpCnt];
        }
        else if (LchFlg > 1) {
            k = 0;
            evt = &localEvt;
            if ((Isbitset(evt->FltFlg, PHASE_A) && (evt->If[0].mag + s->Cfg->Ifdb/2.0 <= s->IcsSns[0].Iist.mag)) ||
                (Isbitset(evt->FltFlg, PHASE_B) && (evt->If[1].mag + s->Cfg->Ifdb/2.0 <= s->IcsSns[1].Iist.mag)) ||
                (Isbitset(evt->FltFlg, PHASE_C) && (evt->If[2].mag + s->Cfg->Ifdb/2.0 <= s->IcsSns[2].Iist.mag)))
                k++;
            PhasorContainerObj->abc2s0(&VI0, &s->IcsSns[0].Iist, &s->IcsSns[1].Iist, &s->IcsSns[2].Iist); // Calculate fault I0
            if (evt->I0.mag + s->Cfg->Ifndb/2.0 <= VI0.mag)
                k++;
            if (k >= 1)
                count++;
            if (count >= 2) {
                for (j = 0; j < MES_PHASES; j++) {
                    evt->If[j] = s->IcsSns[j].Iist;
                    evt->Vf[j] = s->IcsSns[j].Vist;
                }
                if(PhasorContainerObj->IcsNode.VoltSetNum < 4)	{	// Use Fault voltage Set's voltage, it is selected
                    for (j = 0; j < MES_PHASES; j++) {
                        evt->Vf[j] = PhasorContainerObj->IcsNode.IcsSet[PhasorContainerObj->IcsNode.VoltSetNum].IcsSns[j].Vist;
                    }
                }
                evt->I0 = VI0;
                count = 0;		// clear counter
            }
            else		// no need to do further calculation
                return;
        }
/***
// use loop method for evt[0] and set radial method results to evt[1]
		if (m == 2 && IcsNode.TrpCnt == 0 && 	// single phase fault only
			(evt->FltFlg == PHASE_A || evt->FltFlg == PHASE_B || evt->FltFlg == PHASE_C)) {
			s->Evt[1].FltDist = 0.0;
			evt = &s->Evt[0];
			I0.r = MagAng2X(evt->I0.mag, evt->I0.ang);
			I0.x = MagAng2Y(evt->I0.mag, evt->I0.ang);
			I2.r = MagAng2X(evt->I2.mag, evt->I2.ang);
			I2.x = MagAng2Y(evt->I2.mag, evt->I2.ang);

			Zl0 = VectAdd(&Zls0, &Zlr0);
			Zl2 = VectAdd(&Zls2, &Zlr2);

			W1 = VectAdd(&Zs0, &Zr0);
			W2 = VectAdd(&Zs2, &Zr2);
			Z0 = VectAdd(&W1, &Zl0);
			Z2 = VectAdd(&W2, &Zl2);

			W1 = VectX3(&Z2, &Zl0, &I2);
			W2 = VectX3(&Z0, &Zl2, &I0);
			W1 = VectSub(&W1, &W2);

			W2 = VectX3(&Z0, &Zs2, &I0);
			W3 = VectX3(&Z2, &Zs0, &I2);
			W2 = VectSub(&W2, &W3);

			a = W2.r/W1.r;
			evt->FltDist = (1.0 - a)*s->Cfg->LineLength - IcsNode.IcsSet[j].Cfg->LineLength;
#ifdef PC_DEBUG
float a2 = W2.x/W1.x;
printf ("Loop: a =%6.4f, a2 =%6.4f\nDist =%6.4f \n", a, a2, evt->FltDist);
#endif
			if (evt->FltDist <= 0.0 || evt->FltDist > s->Cfg->LineLength)
				j = -999;
			else
				j = 999;
		}
***/

// use radial method
        if (evt->FltFlg == PHASE_A || evt->FltFlg == PHASE_B || evt->FltFlg == PHASE_C) { // single phase fault only
            switch (evt->FltFlg) {
                case PHASE_A:
                    m = 0;
                    break;
                case PHASE_B:
                    m = 1;
                    break;
                case PHASE_C:
                    m = 2;
            }
/***
#ifdef PC_DEBUG
// My newly developed Fault Zone Method
			VI0.mag = evt->I0.mag/evt->If[m].mag;
			VI0.ang = evt->I0.ang - evt->If[m].ang;
			W1.r = MagAng2X(VI0.mag, VI0.ang);
			W1.x = MagAng2Y(VI0.mag, VI0.ang);
			W2.r = s->Cfg->Z0.r - s->Cfg->Z1.r;
			W2.x = s->Cfg->Z0.x - s->Cfg->Z1.x;
			W3 = VectX2(&W2, &W1);
			W1.x = (s->Cfg->Z1.x + W3.x)*s->Cfg->TapSegLength; // Zzone.x
			VI0.mag = 1000.0*evt->Vf[m].mag/evt->If[m].mag; // Va/Ia
			VI0.ang = evt->Vf[m].ang - evt->If[m].ang - 180.0;
			q = MagAng2Y(VI0.mag, VI0.ang);
printf ("New Fault Zone Method: \n");
printf ("V/I.x = ?:%6.4f \n", q);
//			a = q - W1.x*s->Cfg->ZoneSetting[0];
//			r = q - W1.x*s->Cfg->ZoneSetting[1];
			W1.x = (s->Cfg->Z1.x + W3.x);
			q /= W1.x;
//printf ("\nZone-1 < 0 ?:%6.4f \n", a);
//printf ("\nZone-2 < 0 ?:%6.4f \n", r);
printf ("Distance:%6.4f \n\n", q);
#endif
***/
// Conventional T-Method Algorithm
            a = Zls2.r*Zls2.r + Zls2.x*Zls2.x;	// Zl1^2 (Zl2= Zl1)
            W1 = PhasorContainerObj->VectSub(&Zls0, &Zls2);
            r =  W1.r*Zls2.r + W1.x*Zls2.x;
            x = -W1.r*Zls2.x + W1.x*Zls2.r;
            W1.r = r/a;						// = k/3
            W1.x = x/a;
            I0.r = PhasorContainerObj->MagAng2X(evt->I0.mag, evt->I0.ang);
            I0.x = PhasorContainerObj->MagAng2Y(evt->I0.mag, evt->I0.ang);
            W2 = PhasorContainerObj->VectX2(&W1, &I0);			// = k*3I0

            W1.r = PhasorContainerObj->MagAng2X(evt->Vf[m].mag, evt->Vf[m].ang);
            W1.x = PhasorContainerObj->MagAng2Y(evt->Vf[m].mag, evt->Vf[m].ang);
            W2.r += PhasorContainerObj->MagAng2X(evt->If[m].mag, evt->If[m].ang);	// = Is + k*3I0
            W2.x += PhasorContainerObj->MagAng2Y(evt->If[m].mag, evt->If[m].ang);
            W2.r /= 1000.0;
            W2.x /= 1000.0;
/***
// Simple T-Method
#ifdef PC_DEBUG
			x = W2.r*W2.r + W2.x*W2.x;
			W3 = W2;
			W3.x = -W3.x;
			Z2 = VectX2(&W1, &W3);
			a = -Z2.x/(x*Zls2.x);
		r = a*s->Cfg->LineLength;
printf ("Simple T-Method: a =%6.4f \nDist =%6.4f \n\n", a, r);
#endif
***/
// Modified T-Method
            x = W1.x*I0.r - W1.r*I0.x;
            W3 = I0;
            W3.x = -I0.x;
            Z2 = PhasorContainerObj->VectX3(&Zls2, &W2, &W3);
            a = -x/Z2.x;
#ifdef PC_DEBUG
            r = a*s->Cfg->LineLength;
printf ("Modified T-Method: a =%6.4f \nDist =%6.4f \n", a, r);
#endif
/***
// My new method
			W1.r = MagAng2X(evt->If[m].mag, evt->If[m].ang);
			W1.x = MagAng2Y(evt->If[m].mag, evt->If[m].ang);
			W2 = VectX2(&s->Cfg->Z1, &W1);	// Z1*If
			W1 = VectSub(&s->Cfg->Z0, &s->Cfg->Z1);
			W3.r = MagAng2X(evt->I0.mag, evt->I0.ang);
			W3.x = MagAng2Y(evt->I0.mag, evt->I0.ang);
			W3 = VectX2(&W1, &W3);  // (Z0-Z1)*I0
			W1 = VectAdd(&W2, &W3); // [Z1*If + (Z0-Z1)*I0]
			W3.r = 1000.0*MagAng2X(evt->Vf[m].mag, evt->Vf[m].ang);
			W3.x = 1000.0*MagAng2Y(evt->Vf[m].mag, evt->Vf[m].ang);
			W2.r = W3.r - a*W1.r*s->Cfg->LineLength;
			W2.x = W3.x - a*W1.x*s->Cfg->LineLength; // Vf-[Z1*If +(Z0-Z1)*I0]
			W2.x *=-1.0;
			W1 = VectX2(&W1, &W2);
			W3 = VectX2(&W3, &W2);
			q = W3.x/W1.x;
			a = q/s->Cfg->LineLength;
#ifdef PC_DEBUG
		r = a*s->Cfg->LineLength;
printf ("\nInvented New Method: a =%6.4f \nDist =%6.4f \n\n", a, r);
#endif
***/
        }
        else if (evt->FltFlg == PHASE_AB || evt->FltFlg == PHASE_BC || evt->FltFlg == PHASE_CA || evt->FltFlg == PHASE_ABC) {

            if (evt->FltFlg == PHASE_AB || evt->FltFlg == PHASE_ABC )
                m = 0;
            else if (evt->FltFlg == PHASE_BC)
                m = 1;
            else if (evt->FltFlg == PHASE_CA)
                m = 2;
            n = (m < 2) ? m + 1 : 0;

            W1.r = 1000.0*(PhasorContainerObj->MagAng2X(evt->Vf[m].mag, evt->Vf[m].ang) - PhasorContainerObj->MagAng2X(evt->Vf[n].mag, evt->Vf[n].ang));
            W1.x = 1000.0*(PhasorContainerObj->MagAng2Y(evt->Vf[m].mag, evt->Vf[m].ang) - PhasorContainerObj->MagAng2Y(evt->Vf[n].mag, evt->Vf[n].ang));
            W2.r = PhasorContainerObj->MagAng2X(evt->If[m].mag, evt->If[m].ang) - PhasorContainerObj->MagAng2X(evt->If[n].mag, evt->If[n].ang);
            W2.x = -PhasorContainerObj->MagAng2Y(evt->If[m].mag, evt->If[m].ang) + PhasorContainerObj->MagAng2Y(evt->If[n].mag, evt->If[n].ang);
            x = W2.r*W2.r + W2.x*W2.x;
            W2 = PhasorContainerObj->VectX2(&W1, &W2);
            a = -W2.x/(x*Zls2.x);

#ifdef PC_DEBUG
            r = a*s->Cfg->LineLength;
printf ("Radial(2-3Phase Fault): a =%6.4f \nDist =%6.4f \n", a, r);
#endif
        }

        if (a < 0.0 && a >= -0.05)
            a = 0.0;
        evt->FltDist = a*s->Cfg->LineLength;
        if (a>= 0.0 && a <= 1.05) {
            r = evt->FltDist/s->Cfg->TapSegLength;
            if (r <= s->Cfg->ZoneSetting[0])
                evt->FltZone = 1;
            else if (r <= s->Cfg->ZoneSetting[1])
                evt->FltZone = 2;
            else
                evt->FltZone = 3;
        }
        else
            evt->FltZone = 4;   // Unknown Zone
#ifdef PC_DEBUG
        printf ("Fault Zone: %d \n\n", evt->FltZone);
#endif
        if (a >= 0.0 && a <= 1.0) {
            if(evt == &localEvt) { // choice the best distance results between the tripEvt log and localEvt log
                if(evt->FltZone < s->Evt[PhasorContainerObj->IcsNode.TrpCnt].FltZone) {
                    s->Evt[PhasorContainerObj->IcsNode.TrpCnt].FltDist = localEvt.FltDist;
                    s->Evt[PhasorContainerObj->IcsNode.TrpCnt].FltZone = localEvt.FltZone;
                }
            }

            for (i = 0; i < MAX_SETS; i++) { // Set the fault distance to the Forward Direction also
                if (!PhasorContainerObj->IcsNode.IcsSet[i].Cfg->SrvFlg || s == &PhasorContainerObj->IcsNode.IcsSet[i])  // the ICS Set is not in service, skip the set
                    continue;
                evt = &PhasorContainerObj->IcsNode.IcsSet[i].Evt[PhasorContainerObj->IcsNode.TrpCnt];
                if (PhasorContainerObj->IcsNode.IcsSet[i].Cfg->SetTyp == SET_TYPE_LINE) {
                    evt->FltDist = s->Evt[PhasorContainerObj->IcsNode.TrpCnt].FltDist;
                    evt->FltZone = s->Evt[PhasorContainerObj->IcsNode.TrpCnt].FltZone;
                }
            }
        }
    }
}

int TFDIRV2::EvtTrigger ()
{
    int i, k = 0;

    for (i = 0; i < MAX_SETS; i++) {
        if (!PhasorContainerObj->IcsNode.IcsSet[i].Cfg->SrvFlg || !PhasorContainerObj->IcsNode.IcsSet[i].LineFlg)
            continue;
        k += PhasorContainerObj->IcsNode.IcsSet[i].ChgLch + PhasorContainerObj->IcsNode.IcsSet[i].FltLch + PhasorContainerObj->IcsNode.IcsSet[i].NtrLch;
        if (k > 0)
            break;
    }
    return k;
}

int TFDIRV2::TakeAction ()  // Report fault and normal V, I, P, Q
{
    int i, j;
    int Error = 0;
    static FLAG trpcnt;

    if (!PhasorContainerObj->IcsNode.EvtTrigger) {
        PhasorContainerObj->IcsNode.EvtTrigger = EvtTrigger();
        if (PhasorContainerObj->IcsNode.EvtTrigger) {
            trpcnt = PhasorContainerObj->IcsNode.TrpCnt;
#ifdef PC_DEBUG
            printf ("###############################\n");
	printf ("DFR Trigger: ON (Prior to Trip:%d) \n", PhasorContainerObj->IcsNode.TrpCnt+1);
	printf ("###############################\n\n");
#else
//??? Call DFR function which will be activated to record 10 - 15 sycles before the trigger time
//	and 50 - 45 cycles following the trigger, total no less than 60 cycles.
//	Once it is activated, it will not check the trigger status untill it completes the recording.
//	This call is just to trigger another function to start working, not allowed to hold up this caling process
#endif
        }
    }
    else if (PhasorContainerObj->IcsNode.TrpCnt > trpcnt) {
        trpcnt = 0;
        PhasorContainerObj->IcsNode.EvtTrigger = 0;
    }

#ifndef PC_DEBUG
    if (PhasorContainerObj->IcsNode.TrpCnt > 0 ) { // Issue control to the selected switch(es) to open
        for (i = 0; i < MAX_SETS; i++) {
            if (!PhasorContainerObj->IcsNode.IcsSet[i].Cfg->SrvFlg || PhasorContainerObj->IcsNode.IcsSet[i].LineFlg)
                continue;
            if (PhasorContainerObj->IcsNode.IcsSet[i].OpenSw == 1 && PhasorContainerObj->IcsNode.IcsSet[i].Cfg->SwitchEnabled) {
                if (!PhasorContainerObj->IcsNode.CtrlSusp)
                    tfdirMsgHandler.SendValues(i, 0);
//???? Call command function to issue opening control and fault zone
                else
//??? Call alarm for suspended switch control due to too low curret or voltage
                    PhasorContainerObj->IcsNode.IcsSet[i].OpenSw = 2;
            }
        }
    }
#else
    if (PhasorContainerObj->IcsNode.TrpCnt > 0 ) { // Issue control to the selected switch(es) to open
		for (i = 0; i < MAX_SETS; i++) {
			if (!PhasorContainerObj->IcsNode.IcsSet[i].Cfg->SrvFlg || PhasorContainerObj->IcsNode.IcsSet[i].LineFlg)
				continue;
			if (PhasorContainerObj->IcsNode.IcsSet[i].OpenSw == 1 && PhasorContainerObj->IcsNode.IcsSet[i].Cfg->SwitchEnabled) {
				printf ("-----------------------------------------------------------------\n");
				printf ("1st Trip's Fault Zone: %d \n", PhasorContainerObj->IcsNode.IcsSet[i].Evt[0].FltZone);
				if (PhasorContainerObj->IcsNode.TrpCnt >= 2)
					printf ("2nd Trip's Fault Zone: %d \n", PhasorContainerObj->IcsNode.IcsSet[i].Evt[1].FltZone);

				printf ("Resultant Fault Zone: %d \n", PhasorContainerObj->IcsNode.IcsSet[i].FltZone);
				printf ("Command: Open Switch of Set-%1d \n", i+1);
				printf ("-----------------------------------------------------------------\n\n");
				PhasorContainerObj->IcsNode.IcsSet[i].OpenSw++; // mark as proceesed status
			}
			else if (PhasorContainerObj->IcsNode.IcsSet[i].OpenSw == 2 && PhasorContainerObj->IcsNode.TrpCnt == 3) {
				printf ("3rd Trip's Fault Zone: %d \n\n", PhasorContainerObj->IcsNode.IcsSet[i].Evt[2].FltZone);
				PhasorContainerObj->IcsNode.IcsSet[i].OpenSw++; // mark as proceesed status

			}
		}
	}
#endif

    if (PhasorContainerObj->IcsNode.FltLck) {// Process and report locked fault

#ifdef PC_DEBUG
        printf ("Fault Typ:  1->PHASE,   2->Neutral,  4->DeltaChange \n");
		printf ("Fault Pha:  1->A, 2->B, 3->AB, 4->C, 5->CA, 6->BC, 7->ABC \n");
		printf ("Fault D(x): 0->No, 1->Forward, 2->Backward, 3->Unknown \n");
#endif

        if (Isbitset(PhasorContainerObj->IcsNode.FltLck, EVENT_PERMANENT)) { // Permenent fault, line dead
            if (PhasorContainerObj->IcsNode.Cfg->RptPermFlt) { // Report fault with breaker lock
#ifndef PC_DEBUG
                for (j = 0; j < PhasorContainerObj->IcsNode.TrpCnt; j++) {
                    for (i = 0; i < MAX_SETS; i++) {
                        if (!PhasorContainerObj->IcsNode.IcsSet[i].Cfg->SrvFlg)
                            continue;
                        PhasorContainerObj->IcsNode.IcsSet[i].Evt[j].EvtTyp = EVENT_PERMANENT;

//???					  ReportFault(i+1, j, IcsNode.IcsSet[i].Evt[j]);
                    }
                }
#else
                for (j = 0; j < PhasorContainerObj->IcsNode.TrpCnt; j++) {
				  printf ("Trip: %d\n", j+1);
				  for (i = 0; i < MAX_SETS; i++) {
					if (!PhasorContainerObj->IcsNode.IcsSet[i].Cfg->SrvFlg)
						continue;
					PhasorContainerObj->IcsNode.IcsSet[i].Evt[j].EvtTyp = EVENT_PERMANENT;
//					ReportFault(i+1, j, PhasorContainerObj->IcsNode.IcsSet[i].Evt[j]);

					printf ("ICS Set: %d \n", i+1);

					printf ("PermFault: Typ:%1d, Pha:%1d, D(A):%1d  %8.1f/%7.2f D(B):%1d: %8.1f/%7.2f D(C):%1d %8.1f/%7.2f ## %8.1f/%7.2f\n",
					PhasorContainerObj->IcsNode.IcsSet[i].Evt[j].FltTyp,
					PhasorContainerObj->IcsNode.IcsSet[i].Evt[j].FltFlg, PhasorContainerObj->IcsNode.IcsSet[i].Evt[j].FltDir[0],
					PhasorContainerObj->IcsNode.IcsSet[i].Evt[j].If[0].mag, PhasorContainerObj->IcsNode.IcsSet[i].Evt[j].If[0].ang,
					PhasorContainerObj->IcsNode.IcsSet[i].Evt[j].FltDir[1],
					PhasorContainerObj->IcsNode.IcsSet[i].Evt[j].If[1].mag, PhasorContainerObj->IcsNode.IcsSet[i].Evt[j].If[1].ang,
					PhasorContainerObj->IcsNode.IcsSet[i].Evt[j].FltDir[2],
					PhasorContainerObj->IcsNode.IcsSet[i].Evt[j].If[2].mag, PhasorContainerObj->IcsNode.IcsSet[i].Evt[j].If[2].ang,
					PhasorContainerObj->IcsNode.IcsSet[i].Evt[j].Vref.mag, PhasorContainerObj->IcsNode.IcsSet[i].Evt[j].Vref.ang);
					printf ("I0:%8.3f/%6.3f \n", PhasorContainerObj->IcsNode.IcsSet[i].Evt[j].I0.mag,
							PhasorContainerObj->IcsNode.IcsSet[i].Evt[j].I0.ang);
					printf ("I2:%8.3f/%6.3f \n", PhasorContainerObj->IcsNode.IcsSet[i].Evt[j].I2.mag,
							PhasorContainerObj->IcsNode.IcsSet[i].Evt[j].I2.ang);
//					printf ("I0:%8.3f, V0:%6.3f \n", IcsNode.IcsSet[i].Evt[j].I0.mag,
//							IcsNode.IcsSet[i].Evt[j].V0.mag);
//					printf ("I2:%8.3f, V2:%6.3f \n", IcsNode.IcsSet[i].Evt[j].I2.mag,
//							IcsNode.IcsSet[i].Evt[j].V2.mag);
//					printf ("Zs0.r:%6.3f, Zs0.x:%6.3f\n", IcsNode.IcsSet[i].Evt[j].Zs0.r,
//							IcsNode.IcsSet[i].Evt[j].Zs0.x);
//					printf ("Zs2.r:%6.3f, Zs2.x:%6.3f\n", IcsNode.IcsSet[i].Evt[j].Zs2.r,
//							IcsNode.IcsSet[i].Evt[j].Zs2.x);
					printf ("Fault Dirrection:%1d \n", PhasorContainerObj->IcsNode.IcsSet[i].Evt[j].FltDirSum);
					printf ("Fault Distance:%8.3f \n", PhasorContainerObj->IcsNode.IcsSet[i].Evt[j].FltDist);
					printf ("Fault Zone: %1d \n\n", PhasorContainerObj->IcsNode.IcsSet[i].Evt[j].FltZone);
				 }
			  }
#endif
            }
        }
        else if (Isbitset(PhasorContainerObj->IcsNode.FltLck, EVENT_TEMPORARY)) { // Temporary fault, reclose successful
            if (PhasorContainerObj->IcsNode.Cfg->RptTempFlt) { // Report fault with a breaker trip
#ifndef PC_DEBUG
                for (j = 0; j < PhasorContainerObj->IcsNode.TrpCnt; j++) {
                    for (i = 0; i < MAX_SETS; i++) {
                        if (!PhasorContainerObj->IcsNode.IcsSet[i].Cfg->SrvFlg)
                            continue;
                        PhasorContainerObj->IcsNode.IcsSet[i].Evt[j].EvtTyp = EVENT_TEMPORARY;

//???					  ReportFault(i+1, j, PhasorContainerObj->IcsNode.IcsSet[i].Evt[j]);
                    }
                }
#else
                for (j = 0; j < PhasorContainerObj->IcsNode.TrpCnt; j++) {
				printf ("Trip: %d\n", j+1);
				for (i = 0; i < MAX_SETS; i++) {
					if (!PhasorContainerObj->IcsNode.IcsSet[i].Cfg->SrvFlg)
						continue;
					PhasorContainerObj->IcsNode.IcsSet[i].Evt[j].EvtTyp = EVENT_TEMPORARY;
//					ReportFault(i+1, j, PhasorContainerObj->IcsNode.IcsSet[i].Evt[j]);

					printf ("ICS Set: %d \n", i+1);
					printf ("TempFault: Typ:%1d, Pha:%1d, D(A):%1d  %8.1f/%7.2f D(B):%1d %8.1f/%7.2f D(C):%1d %8.1f/%7.2f ## %8.1f/%7.2f\n",
					PhasorContainerObj->IcsNode.IcsSet[i].Evt[j].FltTyp,
					PhasorContainerObj->IcsNode.IcsSet[i].Evt[j].FltFlg, PhasorContainerObj->IcsNode.IcsSet[i].Evt[j].FltDir[0],
					PhasorContainerObj->IcsNode.IcsSet[i].Evt[j].If[0].mag, PhasorContainerObj->IcsNode.IcsSet[i].Evt[j].If[0].ang,
					PhasorContainerObj->IcsNode.IcsSet[i].Evt[j].FltDir[1],
					PhasorContainerObj->IcsNode.IcsSet[i].Evt[j].If[1].mag, PhasorContainerObj->IcsNode.IcsSet[i].Evt[j].If[1].ang,
					PhasorContainerObj->IcsNode.IcsSet[i].Evt[j].FltDir[2],
					PhasorContainerObj->IcsNode.IcsSet[i].Evt[j].If[2].mag, PhasorContainerObj->IcsNode.IcsSet[i].Evt[j].If[2].ang,
					PhasorContainerObj->IcsNode.IcsSet[i].Evt[j].Vref.mag, PhasorContainerObj->IcsNode.IcsSet[i].Evt[j].Vref.ang);
					printf ("I0:%8.3f/%6.3f \n", PhasorContainerObj->IcsNode.IcsSet[i].Evt[j].I0.mag,
							PhasorContainerObj->IcsNode.IcsSet[i].Evt[j].I0.ang);
					printf ("I2:%8.3f/%6.3f \n", PhasorContainerObj->IcsNode.IcsSet[i].Evt[j].I2.mag,
							PhasorContainerObj->IcsNode.IcsSet[i].Evt[j].I2.ang);
//					printf ("I0:%8.3f, V0:%6.3f \n", PhasorContainerObj->IcsNode.IcsSet[i].Evt[j].I0.mag,
//							PhasorContainerObj->IcsNode.IcsSet[i].Evt[j].V0.mag);
//					printf ("I2:%8.3f, V2:%6.3f \n\n", PhasorContainerObj->IcsNode.IcsSet[i].Evt[j].I2.mag,
//							PhasorContainerObj->IcsNode.IcsSet[i].Evt[j].V2.mag);
					printf ("Fault Direction:%1d \n", PhasorContainerObj->IcsNode.IcsSet[i].Evt[j].FltDirSum);
					printf ("Fault Distance:%8.3f \n", PhasorContainerObj->IcsNode.IcsSet[i].Evt[j].FltDist);
					printf ("Fault Zone: %1d \n\n", PhasorContainerObj->IcsNode.IcsSet[i].Evt[j].FltZone);
				}
			  }
#endif
            }
        }
        else if (Isbitset(PhasorContainerObj->IcsNode.FltLck, EVENT_MOMENTARY)) { // Momentary fault, return to normal
            if (PhasorContainerObj->IcsNode.Cfg->RptMmtEvt) { // Report fault without causing a trip
#ifndef PC_DEBUG
                for (j = 0; j < 1; j++) {
                    for (i = 0; i < MAX_SETS; i++) {
                        if (!PhasorContainerObj->IcsNode.IcsSet[i].Cfg->SrvFlg)
                            continue;

//???					ReportFault(i+1, j, PhasorContainerObj->IcsNode.IcsSet[i].Evt[j]);
                    }
                }

#else
                for (j = 0; j < 1; j++) {
				printf ("Trip: %d\n", j+1);
				for (i = 0; i < MAX_SETS; i++) {
					if (!PhasorContainerObj->IcsNode.IcsSet[i].Cfg->SrvFlg)
						continue;
					PhasorContainerObj->IcsNode.IcsSet[i].Evt[j].EvtTyp = EVENT_MOMENTARY;
//					ReportFault(i+1, j, PhasorContainerObj->IcsNode.IcsSet[i].Evt[j]);

					printf ("ICS Set: %d \n", i+1);
					printf ("MomentFault: Type:%1d, Pha:%1d, D(A):%1d  %8.1f/%7.2f D(B):%1d: %8.1f/%7.2f D(C):%1d: %8.1f/%7.2f ## %8.1f/%7.2f\n",
					PhasorContainerObj->IcsNode.IcsSet[i].Evt[j].FltTyp,
					PhasorContainerObj->IcsNode.IcsSet[i].Evt[j].FltFlg, PhasorContainerObj->IcsNode.IcsSet[i].Evt[j].FltDir[0],
					PhasorContainerObj->IcsNode.IcsSet[i].Evt[j].If[0].mag, PhasorContainerObj->IcsNode.IcsSet[i].Evt[j].If[0].ang,
					PhasorContainerObj->IcsNode.IcsSet[i].Evt[j].FltDir[1],
					PhasorContainerObj->IcsNode.IcsSet[i].Evt[j].If[1].mag, PhasorContainerObj->IcsNode.IcsSet[i].Evt[j].If[1].ang,
					PhasorContainerObj->IcsNode.IcsSet[i].Evt[j].FltDir[2],
					PhasorContainerObj->IcsNode.IcsSet[i].Evt[j].If[2].mag, PhasorContainerObj->IcsNode.IcsSet[i].Evt[j].If[2].ang,
					PhasorContainerObj->IcsNode.IcsSet[i].Evt[j].Vref.mag, PhasorContainerObj->IcsNode.IcsSet[i].Evt[j].Vref.ang);
					printf ("I0:%8.3f/%6.3f \n", PhasorContainerObj->IcsNode.IcsSet[i].Evt[j].I0.mag,
							PhasorContainerObj->IcsNode.IcsSet[i].Evt[j].I0.ang);
					printf ("I2:%8.3f/%6.3f \n", PhasorContainerObj->IcsNode.IcsSet[i].Evt[j].I2.mag,
							PhasorContainerObj->IcsNode.IcsSet[i].Evt[j].I2.ang);
//					printf ("I0:%8.3f, V0:%6.3f \n", PhasorContainerObj->IcsNode.IcsSet[i].Evt[j].I0.mag,
//							PhasorContainerObj->IcsNode.IcsSet[i].Evt[j].V0.mag);
//					printf ("I2:%8.3f, V2:%6.3f \n\n", PhasorContainerObj->IcsNode.IcsSet[i].Evt[j].I2.mag,
//							PhasorContainerObj->IcsNode.IcsSet[i].Evt[j].V2.mag);
					printf ("Fault Direction:%1d \n", PhasorContainerObj->IcsNode.IcsSet[i].Evt[j].FltDirSum);
					printf ("Fault Distance:%8.3f \n", PhasorContainerObj->IcsNode.IcsSet[i].Evt[j].FltDist);
					printf ("Fault Zone: %1d \n\n", PhasorContainerObj->IcsNode.IcsSet[i].Evt[j].FltZone);
				}
			  }
#endif
            }
        }
// Clear Set Latches
        for (i = 0; i < MAX_SETS; i++) {
            PhasorContainerObj->IcsNode.IcsSet[i].ChgLch = 0;
            PhasorContainerObj->IcsNode.IcsSet[i].FltLch = 0;
            PhasorContainerObj->IcsNode.IcsSet[i].NtrLch = 0;
            PhasorContainerObj->IcsNode.IcsSet[i].TypLch = 0;
            PhasorContainerObj->IcsNode.IcsSet[i].TypLch = 0;
            PhasorContainerObj->IcsNode.IcsSet[i].FltCnt = 0;
            PhasorContainerObj->IcsNode.IcsSet[i].FltZone = 0;
            PhasorContainerObj->IcsNode.IcsSet[i].OpenSw = 0;

            memset(PhasorContainerObj->IcsNode.IcsSet[i].Evt, 0, EVT_Total_Size);

            //***
            //should clear out the samples and the samples?
            //***
            memset(&PhasorContainerObj->SmpNode.SmpSet[i].I, 0, MES_PHASES*SV_CYC_SIZE*PhasorContainerObj->XY_Size);
            memset(&PhasorContainerObj->SmpNode.SmpSet[i].V, 0, MES_PHASES*SV_CYC_SIZE*PhasorContainerObj->XY_Size);
            for (j = 0; j < MES_PHASES; j++) {
                memset(&PhasorContainerObj->IcsNode.IcsSet[i].IcsSns[j].Ibuf, 0, (BUF_SIZE+1)*PhasorContainerObj->XY_Size);
                memset(&PhasorContainerObj->IcsNode.IcsSet[i].IcsSns[j].Vbuf, 0, (BUF_SIZE+1)*PhasorContainerObj->XY_Size);
            }
        }
// Clear fault lock information
        PhasorContainerObj->IcsNode.RclCnt = 0;
        PhasorContainerObj->IcsNode.TrpCnt = 0;
        PhasorContainerObj->IcsNode.LchFlg = 0;
        PhasorContainerObj->IcsNode.FltLck = 0;   // Clear fault lock
        PhasorContainerObj->IcsNode.EvtTrigger = 0;
    }
    else if (PhasorContainerObj->IcsNode.Flt2Long) { // Fault for  too long, Abnormal fault
        // Report a fault for too long
        j = 0;
#ifndef PC_DEBUG
        for (i = 0; i < MAX_SETS; i++) {
            if (!PhasorContainerObj->IcsNode.IcsSet[i].Cfg->SrvFlg)
                continue;
            PhasorContainerObj->IcsNode.IcsSet[i].Evt[j].EvtTyp = EVENT_FAULT2LONG;

//???			ReportAbnormalFault(i+1, j, PhasorContainerObj->IcsNode.IcsSet[i].Evt[j]);
        }
#else
        printf ("Fault-2-Long ---\n");
		for (i = 0; i < MAX_SETS; i++) {
			if (!PhasorContainerObj->IcsNode.IcsSet[i].Cfg->SrvFlg)
				continue;
			PhasorContainerObj->IcsNode.IcsSet[i].Evt[j].EvtTyp = EVENT_FAULT2LONG;
//			ReportAbnormalFault(i+1, j, PhasorContainerObj->IcsNode.IcsSet[i].Evt[j]);

			printf ("ICS Set: %d \n", i+1);
			printf ("Fault2Long: Typ:%2d, Pha:%2d, Dir:%2d  %8.1f/%7.2f %2d: %8.1f/%7.2f %2d: %8.1f/%7.2f ## %8.1f/%7.2f\n",
			PhasorContainerObj->IcsNode.IcsSet[i].Evt[j].FltTyp,
			PhasorContainerObj->IcsNode.IcsSet[i].Evt[j].FltFlg, PhasorContainerObj->IcsNode.IcsSet[i].Evt[j].FltDir[0],
			PhasorContainerObj->IcsNode.IcsSet[i].Evt[j].If[0].mag, PhasorContainerObj->IcsNode.IcsSet[i].Evt[j].If[0].ang,
			PhasorContainerObj->IcsNode.IcsSet[i].Evt[j].FltDir[1],
			PhasorContainerObj->IcsNode.IcsSet[i].Evt[j].If[1].mag, PhasorContainerObj->IcsNode.IcsSet[i].Evt[j].If[1].ang,
			PhasorContainerObj->IcsNode.IcsSet[i].Evt[j].FltDir[2],
			PhasorContainerObj->IcsNode.IcsSet[i].Evt[j].If[2].mag, PhasorContainerObj->IcsNode.IcsSet[i].Evt[j].If[2].ang,
			PhasorContainerObj->IcsNode.IcsSet[i].Evt[j].Vref.mag, PhasorContainerObj->IcsNode.IcsSet[i].Evt[j].Vref.ang);
			printf ("I0:%8.3f/%6.3f \n", PhasorContainerObj->IcsNode.IcsSet[i].Evt[j].I0.mag,
					PhasorContainerObj->IcsNode.IcsSet[i].Evt[j].I0.ang);
			printf ("I2:%8.3f/%6.3f \n", PhasorContainerObj->IcsNode.IcsSet[i].Evt[j].I2.mag,
					PhasorContainerObj->IcsNode.IcsSet[i].Evt[j].I2.ang);
//			printf ("I0:%8.3f, V0:%6.3f \n", PhasorContainerObj->IcsNode.IcsSet[i].Evt[j].I0.mag,
//					PhasorContainerObj->IcsNode.IcsSet[i].Evt[j].V0.mag);
//			printf ("I2:%8.3f, V2:%6.3f \n\n", PhasorContainerObj->IcsNode.IcsSet[i].Evt[j].I2.mag,
//					PhasorContainerObj->IcsNode.IcsSet[i].Evt[j].V2.mag);
			printf ("Fault Direction:%1d \n", PhasorContainerObj->IcsNode.IcsSet[i].Evt[j].FltDirSum);
			printf ("Fault Distance:%8.3f \n", PhasorContainerObj->IcsNode.IcsSet[i].Evt[j].FltDist);
			printf ("Fault Zone: %1d \n\n", PhasorContainerObj->IcsNode.IcsSet[i].Evt[j].FltZone);
		}
#endif
// Clear Set Latches
        for (i = 0; i < MAX_SETS; i++) {
            PhasorContainerObj->IcsNode.IcsSet[i].ChgLch = 0;
            PhasorContainerObj->IcsNode.IcsSet[i].FltLch = 0;
            PhasorContainerObj->IcsNode.IcsSet[i].NtrLch = 0;
            PhasorContainerObj->IcsNode.IcsSet[i].TypLch = 0;
            PhasorContainerObj->IcsNode.IcsSet[i].FltCnt = 0;
            PhasorContainerObj->IcsNode.IcsSet[i].NtrCnt = 0;
            PhasorContainerObj->IcsNode.IcsSet[i].ChgCnt = 0;
            PhasorContainerObj->IcsNode.IcsSet[i].FltZone = 0;
            PhasorContainerObj->IcsNode.IcsSet[i].OpenSw = 0;
            memset(PhasorContainerObj->IcsNode.IcsSet[i].Evt, 0, EVT_Total_Size);

            //***
            //should clear out the samples and the samples?
            //***
            memset(&PhasorContainerObj->SmpNode.SmpSet[i].I, 0, MES_PHASES*SV_CYC_SIZE*PhasorContainerObj->XY_Size);
            memset(&PhasorContainerObj->SmpNode.SmpSet[i].V, 0, MES_PHASES*SV_CYC_SIZE*PhasorContainerObj->XY_Size);
            for (j = 0; j < MES_PHASES; j++) {
                memset(&PhasorContainerObj->IcsNode.IcsSet[i].IcsSns[j].Ibuf, 0, (BUF_SIZE+1)*PhasorContainerObj->XY_Size);
                memset(&PhasorContainerObj->IcsNode.IcsSet[i].IcsSns[j].Vbuf, 0, (BUF_SIZE+1)*PhasorContainerObj->XY_Size);
            }
        }
// Clear fault lock information
        PhasorContainerObj->IcsNode.Flt2Long = 0;
        PhasorContainerObj->IcsNode.LchFlg = 0;
        PhasorContainerObj->IcsNode.RclCnt = 0;
        PhasorContainerObj->IcsNode.TrpCnt = 0;
        PhasorContainerObj->IcsNode.EvtTrigger = 0;
    }
// Report Delta Change and Normal operation
/*
	if (PhasorContainerObj->IcsNode.RptChg && PhasorContainerObj->IcsNode.Cfg->RptChg) {  // Report Delta change to trigger the Master fault detection
#ifndef PC_DEBUG


#else
		printf ("Delta Report *** \n");
		j = EVT_SIZE - 1;
		for (i = 0; i < MAX_SETS; i++) {
			if (!PhasorContainerObj->IcsNode.IcsSet[i].Cfg->SrvFlg)
				continue;
			printf ("ICS Set: %d \n", i+1);
			printf ("DeltaReport: %8.1f/%8.2f %2d: %8.1f/%8.2f %2d: %8.1f/%8.2f ## %8.1f/%8.2f\n",
			PhasorContainerObj->IcsNode.IcsSet[i].Evt[j].If[0].mag, PhasorContainerObj->IcsNode.IcsSet[i].Evt[j].If[0].ang,
			PhasorContainerObj->IcsNode.IcsSet[i].Evt[j].FltDir[1],
			PhasorContainerObj->IcsNode.IcsSet[i].Evt[j].If[1].mag, PhasorContainerObj->IcsNode.IcsSet[i].Evt[j].If[1].ang,
			PhasorContainerObj->IcsNode.IcsSet[i].Evt[j].FltDir[2],
			PhasorContainerObj->IcsNode.IcsSet[i].Evt[j].If[2].mag, PhasorContainerObj->IcsNode.IcsSet[i].Evt[j].If[2].ang,
			PhasorContainerObj->IcsNode.IcsSet[i].Evt[j].Vref.mag, PhasorContainerObj->IcsNode.IcsSet[i].Evt[j].Vref.ang);
			printf ("I0:%8.3f, V0:%6.3f \n", PhasorContainerObj->IcsNode.IcsSet[i].Evt[j].I0.mag,
					PhasorContainerObj->IcsNode.IcsSet[i].Evt[j].V0.mag);
			printf ("I2:%8.3f, V2:%6.3f \n", PhasorContainerObj->IcsNode.IcsSet[i].Evt[j].I2.mag,
					PhasorContainerObj->IcsNode.IcsSet[i].Evt[j].V2.mag);
			printf ("ResultFaultDir:%1d \n", PhasorContainerObj->IcsNode.IcsSet[i].Evt[j].FltDirSum);
			printf ("FaultDistance:%8.3f \n", PhasorContainerObj->IcsNode.IcsSet[i].Evt[j].FltDist);
		}
#endif
		PhasorContainerObj->IcsNode.RptChg = 0;
	}
*/
    return Error;
}

#ifdef RUNNING_TEST
//this is just to test the STC scenario
void TFDIRV2::testSTC() {
    PHASOR pv[3], sv[3], pi[3], si[3];
    float Iang[3], Imag[3], Vang[3], Vmag[3];
    float x, y;
    int i, j, k, l, m;
    int error = 0;

    std::ifstream in(CFG_FILE_PATH);
    std::string contents((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());

    char * configContents = new char[contents.size() + 1];
    std::copy(contents.begin(), contents.end(), configContents);
    configContents[contents.size()] = '\0';

    PhasorContainerObj->Configure(0, configContents);

#ifdef PC_DEBUG
    printf ("Cycle:  Ia/Ang           Ib/Ang          Ic/Ang       |   Va/Ang          Vb/Ang          Vc/Ang\n");
#endif
    m = 0;
    k = 0;
    while (!error) {
        error = PhasorContainerObj->GetSamples(m);

        PhasorContainerObj->calcPhasors();

#ifndef PC_DEBUG
#else
        if (error)
			PhasorContainerObj->IcsNode.DnTimer += 100.0;
#endif
        m++;
        SetAvg();

#ifdef PC_DEBUG_STEP
        for (i = 0; i < MAX_SETS; i++) {
			if (!PhasorContainerObj->IcsNode.IcsSet[i].Cfg->SrvFlg)
				continue;

		printf ("%3d: %8.1f/%6.2f %8.1f/%6.2f %8.1f/%6.2f  |  %6.2f/%6.2f   %6.2f/%6.2f   %6.2f/%6.2f\n", m,
				PhasorContainerObj->IcsNode.IcsSet[i].IcsSns[0].Iist.mag, PhasorContainerObj->IcsNode.IcsSet[i].IcsSns[0].Iist.ang,
				PhasorContainerObj->IcsNode.IcsSet[i].IcsSns[1].Iist.mag, PhasorContainerObj->IcsNode.IcsSet[i].IcsSns[1].Iist.ang,
				PhasorContainerObj->IcsNode.IcsSet[i].IcsSns[2].Iist.mag, PhasorContainerObj->IcsNode.IcsSet[i].IcsSns[2].Iist.ang,
				PhasorContainerObj->IcsNode.IcsSet[i].IcsSns[0].Vist.mag, PhasorContainerObj->IcsNode.IcsSet[i].IcsSns[0].Vist.ang,
				PhasorContainerObj->IcsNode.IcsSet[i].IcsSns[1].Vist.mag, PhasorContainerObj->IcsNode.IcsSet[i].IcsSns[1].Vist.ang,
				PhasorContainerObj->IcsNode.IcsSet[i].IcsSns[2].Vist.mag, PhasorContainerObj->IcsNode.IcsSet[i].IcsSns[2].Vist.ang);
		for (j = 0; j < 3; j++) {
			l = (j < 2)? j+1 : 0;
			Iang[j] = PhasorContainerObj->IcsNode.IcsSet[i].IcsSns[j].Iist.ang - PhasorContainerObj->IcsNode.IcsSet[i].IcsSns[l].Iist.ang;
			if(Iang[j] > 180.0) Iang[j] = 360.0 - Iang[j];
			else if (Iang[j] < -180.0) Iang[j] += 360.0;
			Vang[j] = PhasorContainerObj->IcsNode.IcsSet[i].IcsSns[j].Vist.ang - PhasorContainerObj->IcsNode.IcsSet[i].IcsSns[l].Vist.ang;
			if(Vang[j] > 180.0) Vang[j] = 360.0 - Vang[j];
			else if (Vang[j] < -180.0) Vang[j] += 360.0;
			x = PhasorContainerObj->MagAng2X(PhasorContainerObj->IcsNode.IcsSet[i].IcsSns[j].Vist.mag,PhasorContainerObj->IcsNode.IcsSet[i].IcsSns[j].Vist.ang)-
					PhasorContainerObj->MagAng2X(PhasorContainerObj->IcsNode.IcsSet[i].IcsSns[l].Vist.mag,PhasorContainerObj->IcsNode.IcsSet[i].IcsSns[l].Vist.ang);
			y = PhasorContainerObj->MagAng2Y(PhasorContainerObj->IcsNode.IcsSet[i].IcsSns[j].Vist.mag,PhasorContainerObj->IcsNode.IcsSet[i].IcsSns[j].Vist.ang)-
					PhasorContainerObj->MagAng2Y(PhasorContainerObj->IcsNode.IcsSet[i].IcsSns[l].Vist.mag,PhasorContainerObj->IcsNode.IcsSet[i].IcsSns[l].Vist.ang);
			Vmag[j] = PhasorContainerObj->XY2Mag(x, y);
			Imag[j] = PhasorContainerObj->IcsNode.IcsSet[i].IcsSns[j].Vist.ang - PhasorContainerObj->IcsNode.IcsSet[i].IcsSns[j].Iist.ang;
			if (Imag[j] < -180.0) Imag[j] += 360.0;

			pi[j] = PhasorContainerObj->IcsNode.IcsSet[i].IcsSns[j].Iist;
			pv[j] = PhasorContainerObj->IcsNode.IcsSet[i].IcsSns[j].Vist;
		}
		PhasorContainerObj->abc2s012(si, pi);
		PhasorContainerObj->abc2s012(sv, pv);
		printf ("              %6.2f           %6.2f          %6.2f |  %6.2f/%6.2f    %6.2f/%6.2f   %6.2f/%6.2f\n",
			Iang[0], Iang[1], Iang[2], Vmag[0],Vang[0], Vmag[1],Vang[1], Vmag[2],Vang[2]);
		printf (" VIang:       %6.2f           %6.2f          %6.2f\n", Imag[0], Imag[1], Imag[2]);
		printf (" IV012:%6.1f/%6.2f %8.1f/%6.2f %8.1f/%6.2f |  %6.2f/%6.2f   %6.2f/%6.2f   %6.2f/%6.2f\n",
		  si[0].mag, si[0].ang, si[1].mag, si[1].ang, si[2].mag, si[2].ang,
		  sv[0].mag, sv[0].ang, sv[1].mag, sv[1].ang, sv[2].mag, sv[2].ang);
/*
		printf ("%d: %8.1f/%7.2f %8.1f/%7.2f %8.1f/%7.2f | %8.1f/%7.2f %8.1f/%7.2f %8.1f/%7.2f\n", m,
				PhasorContainerObj->IcsNode.IcsSet[i].IcsSns[0].Ifa.mag, PhasorContainerObj->IcsNode.IcsSet[i].IcsSns[0].Ifa.ang,
				PhasorContainerObj->IcsNode.IcsSet[i].IcsSns[1].Ifa.mag, PhasorContainerObj->IcsNode.IcsSet[i].IcsSns[1].Ifa.ang,
				PhasorContainerObj->IcsNode.IcsSet[i].IcsSns[2].Ifa.mag, PhasorContainerObj->IcsNode.IcsSet[i].IcsSns[2].Ifa.ang,
				PhasorContainerObj->IcsNode.IcsSet[i].IcsSns[0].Vref.mag, PhasorContainerObj->IcsNode.IcsSet[i].IcsSns[0].Vref.ang,
				PhasorContainerObj->IcsNode.IcsSet[i].IcsSns[1].Vref.mag, PhasorContainerObj->IcsNode.IcsSet[i].IcsSns[1].Vref.ang,
				PhasorContainerObj->IcsNode.IcsSet[i].IcsSns[2].Vref.mag, PhasorContainerObj->IcsNode.IcsSet[i].IcsSns[2].Vref.ang);
*/
		printf (" Delta:%6.1f/%6.2f %8.1f/%6.2f %8.1f/%6.2f\n",
				PhasorContainerObj->IcsNode.IcsSet[i].IcsSns[0].Ifc.mag, PhasorContainerObj->IcsNode.IcsSet[i].IcsSns[0].Ifc.ang,
				PhasorContainerObj->IcsNode.IcsSet[i].IcsSns[1].Ifc.mag, PhasorContainerObj->IcsNode.IcsSet[i].IcsSns[1].Ifc.ang,
				PhasorContainerObj->IcsNode.IcsSet[i].IcsSns[2].Ifc.mag, PhasorContainerObj->IcsNode.IcsSet[i].IcsSns[2].Ifc.ang);
		printf ("-----\n");

		}
		printf ("***********\n");
#endif
        SetFault();

        DetectFault();

// dynamically adjust the voltage correction factor
        if (m % 30 == 3)
            PhasorContainerObj->CalibVolt();	// dynamically calibrate voltage

        TakeAction();

        m = m % 60000;
#ifdef PC_DEBUG
        if (PhasorContainerObj->IcsNode.RclCnt > k) {
			k = PhasorContainerObj->IcsNode.RclCnt;
			printf("Reclose #: %d\n", k);
		}
#endif
    }
}
#endif