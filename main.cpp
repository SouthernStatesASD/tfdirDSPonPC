#include <iostream>
#include <fstream>
#include <thread>
#include "Pathological.hpp"
#include "TFDIRContainer.h"

using namespace std;

int main(int argc, char *argv []) {

    cout << "Starting SAM using ";
    if(argc > 1) {
        for(int i=1; i < argc; i++) {
            cout << argv[i] << " ";
        }
        cout << endl;
    }

    if (argc >= 3) {
        // get configuration information from the configuration.json file in the /opt/sam directory.

        OPERATINGMODE operatingMode = LIVE;  //
        CONTROLMODE controlMode = NOCONTROL; // Report Recommended Switch Operation

        string jsonConfigFile = argv[1];;
        string serverAddress;

        if (argc > 3) {
            if (strcmp(argv[3], "simulation") == 0) {
                operatingMode = SIMULATION;
                if (argc >= 5) {
                    serverAddress = argv[4];
                    serverAddress = "tcp://" + serverAddress;

                    if (argc >= 6) {
                        if (strcmp(argv[5], "relay") == 0)
                            controlMode = RELAY;
                        else if (strcmp(argv[5], "aux") == 0)
                            controlMode = AUXCONTROL;
                    }
                }
            } else if (strcmp(argv[3], "live") == 0) {
                if (argc >= 5) {
                    if (strcmp(argv[4], "relay") == 0)
                        controlMode = RELAY;
                    else if (strcmp(argv[4], "aux") == 0)
                        controlMode = AUXCONTROL;
                }
            } else {
                printf("Invalid parameters!!! \n");
                return 0;
            }
        }

        //Pathological for zeroMQ p2p comms
        Pathological *pathological = new Pathological(jsonConfigFile, operatingMode, serverAddress);
        this_thread::sleep_for(chrono::seconds(1)); //wait for pathological to spin up it's internal threads


        std::cout << "Test TFDIR" << std::endl;

        TFDIRContainer *testTFDIR = new TFDIRContainer(pathological, operatingMode, controlMode);

        PHASOR pv[3], sv[3], pi[3], si[3];
        float Iang[3], Imag[3], Vang[3], Vmag[3];
        float x, y;
        int i, j, k, l, m;
        int error, Error = 0;

//        std::ifstream in(CFG_FILE_PATH);
        ifstream in(argv[2]);
        std::string contents((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());

        char *configContents = new char[contents.size() + 1];
        std::copy(contents.begin(), contents.end(), configContents);
        configContents[contents.size()] = '\0';
        gettimeofday(&testTFDIR->Time, NULL);
        testTFDIR->Configure(0, configContents);

#ifdef PC_DEBUG
        printf("Cycle:  Ia/Ang           Ib/Ang          Ic/Ang       |   Va/Ang          Vb/Ang          Vc/Ang\n");
#endif
        m = 0;
        k = 0;
        while (Error < 1) {

            gettimeofday(&testTFDIR->Time, NULL);
            this_thread::sleep_for(chrono::milliseconds(10));

            testTFDIR->executionCt++;

            error = testTFDIR->GetSamplesReversed(m);
            m++;
            if (error == 1) {
                Error++;
                m = 0;
            }

            testTFDIR->calcPhasors(0, 0);

#ifndef PC_DEBUG
#else
            if (error)
                testTFDIR->IcsNode.DnTimer += 100.0;
#endif
            testTFDIR->SetAvg();

#ifdef PC_DEBUG_STEP
            for (i = 0; i < MAX_SETS; i++) {
                if (!testTFDIR->IcsNode.IcsSet[i].Cfg->SrvFlg)
                    continue;

                printf ("%3d: %8.1f/%6.2f %8.1f/%6.2f %8.1f/%6.2f  |  %6.2f/%6.2f   %6.2f/%6.2f   %6.2f/%6.2f\n", m,
                        testTFDIR->IcsNode.IcsSet[i].IcsSns[0].Iist.mag, testTFDIR->IcsNode.IcsSet[i].IcsSns[0].Iist.ang,
                        testTFDIR->IcsNode.IcsSet[i].IcsSns[1].Iist.mag, testTFDIR->IcsNode.IcsSet[i].IcsSns[1].Iist.ang,
                        testTFDIR->IcsNode.IcsSet[i].IcsSns[2].Iist.mag, testTFDIR->IcsNode.IcsSet[i].IcsSns[2].Iist.ang,
                        testTFDIR->IcsNode.IcsSet[i].IcsSns[0].Vist.mag, testTFDIR->IcsNode.IcsSet[i].IcsSns[0].Vist.ang,
                        testTFDIR->IcsNode.IcsSet[i].IcsSns[1].Vist.mag, testTFDIR->IcsNode.IcsSet[i].IcsSns[1].Vist.ang,
                        testTFDIR->IcsNode.IcsSet[i].IcsSns[2].Vist.mag, testTFDIR->IcsNode.IcsSet[i].IcsSns[2].Vist.ang);
                for (j = 0; j < 3; j++) {
                    l = (j < 2)? j+1 : 0;
                    Iang[j] = testTFDIR->IcsNode.IcsSet[i].IcsSns[j].Iist.ang - testTFDIR->IcsNode.IcsSet[i].IcsSns[l].Iist.ang;
                    if(Iang[j] > 180.0) Iang[j] = 360.0 - Iang[j];
                    else if (Iang[j] < -180.0) Iang[j] += 360.0;
                    Vang[j] = testTFDIR->IcsNode.IcsSet[i].IcsSns[j].Vist.ang - testTFDIR->IcsNode.IcsSet[i].IcsSns[l].Vist.ang;
                    if(Vang[j] > 180.0) Vang[j] = 360.0 - Vang[j];
                    else if (Vang[j] < -180.0) Vang[j] += 360.0;
                    x = testTFDIR->MagAng2X(testTFDIR->IcsNode.IcsSet[i].IcsSns[j].Vist.mag,testTFDIR->IcsNode.IcsSet[i].IcsSns[j].Vist.ang)-
                        testTFDIR->MagAng2X(testTFDIR->IcsNode.IcsSet[i].IcsSns[l].Vist.mag,testTFDIR->IcsNode.IcsSet[i].IcsSns[l].Vist.ang);
                    y = testTFDIR->MagAng2Y(testTFDIR->IcsNode.IcsSet[i].IcsSns[j].Vist.mag,testTFDIR->IcsNode.IcsSet[i].IcsSns[j].Vist.ang)-
                        testTFDIR->MagAng2Y(testTFDIR->IcsNode.IcsSet[i].IcsSns[l].Vist.mag,testTFDIR->IcsNode.IcsSet[i].IcsSns[l].Vist.ang);
                    Vmag[j] = testTFDIR->XY2Mag(x, y);
                    Imag[j] = testTFDIR->IcsNode.IcsSet[i].IcsSns[j].Vist.ang - testTFDIR->IcsNode.IcsSet[i].IcsSns[j].Iist.ang;
                    if (Imag[j] < -180.0) Imag[j] += 360.0;

                    pi[j] = testTFDIR->IcsNode.IcsSet[i].IcsSns[j].Iist;
                    pv[j] = testTFDIR->IcsNode.IcsSet[i].IcsSns[j].Vist;
                }
                testTFDIR->abc2s012(si, pi);
                testTFDIR->abc2s012(sv, pv);
                printf ("              %6.2f           %6.2f          %6.2f |  %6.2f/%6.2f    %6.2f/%6.2f   %6.2f/%6.2f\n",
                        Iang[0], Iang[1], Iang[2], Vmag[0],Vang[0], Vmag[1],Vang[1], Vmag[2],Vang[2]);
                printf (" VIang:       %6.2f           %6.2f          %6.2f\n", Imag[0], Imag[1], Imag[2]);
                printf (" IV012:%6.1f/%6.2f %8.1f/%6.2f %8.1f/%6.2f |  %6.2f/%6.2f   %6.2f/%6.2f   %6.2f/%6.2f\n",
                        si[0].mag, si[0].ang, si[1].mag, si[1].ang, si[2].mag, si[2].ang,
                        sv[0].mag, sv[0].ang, sv[1].mag, sv[1].ang, sv[2].mag, sv[2].ang);
    /*
            printf ("%d: %8.1f/%7.2f %8.1f/%7.2f %8.1f/%7.2f | %8.1f/%7.2f %8.1f/%7.2f %8.1f/%7.2f\n", m,
                    testTFDIR->IcsNode.IcsSet[i].IcsSns[0].Ifa.mag, testTFDIR->IcsNode.IcsSet[i].IcsSns[0].Ifa.ang,
                    testTFDIR->IcsNode.IcsSet[i].IcsSns[1].Ifa.mag, testTFDIR->IcsNode.IcsSet[i].IcsSns[1].Ifa.ang,
                    testTFDIR->IcsNode.IcsSet[i].IcsSns[2].Ifa.mag, testTFDIR->IcsNode.IcsSet[i].IcsSns[2].Ifa.ang,
                    testTFDIR->IcsNode.IcsSet[i].IcsSns[0].Vref.mag, testTFDIR->IcsNode.IcsSet[i].IcsSns[0].Vref.ang,
                    testTFDIR->IcsNode.IcsSet[i].IcsSns[1].Vref.mag, testTFDIR->IcsNode.IcsSet[i].IcsSns[1].Vref.ang,
                    testTFDIR->IcsNode.IcsSet[i].IcsSns[2].Vref.mag, testTFDIR->IcsNode.IcsSet[i].IcsSns[2].Vref.ang);
    */
                printf (" Delta:%6.1f/%6.2f %8.1f/%6.2f %8.1f/%6.2f\n",
                        testTFDIR->IcsNode.IcsSet[i].IcsSns[0].Ifc.mag, testTFDIR->IcsNode.IcsSet[i].IcsSns[0].Ifc.ang,
                        testTFDIR->IcsNode.IcsSet[i].IcsSns[1].Ifc.mag, testTFDIR->IcsNode.IcsSet[i].IcsSns[1].Ifc.ang,
                        testTFDIR->IcsNode.IcsSet[i].IcsSns[2].Ifc.mag, testTFDIR->IcsNode.IcsSet[i].IcsSns[2].Ifc.ang);
                printf ("-----\n");

    //            if (m == 10 || m == 30 || m == 50)
    //                testTFDIR->CfgNode.CfgSet[i].AmpSclFactor = 10.0;
    //            else if(m == 16 || m == 36 || m == 56)
    //                testTFDIR->CfgNode.CfgSet[i].AmpSclFactor = 0.0;
            }
            printf ("***********\n");
#endif
            testTFDIR->SetFault();

            testTFDIR->DetectFault();

// P2P Procesee
            testTFDIR->GetPeersMsgs();
            testTFDIR->GetOprData();
            testTFDIR->P2PFaultScheme();
            testTFDIR->SendPeersMsgs();

// dynamically adjust the voltage correction factor
            if (m % 30 == 3)
                testTFDIR->CalibVolt();    // dynamically calibrate voltage

            testTFDIR->TakeAction();

            m = m % 60000;
#ifdef PC_DEBUG
            if (testTFDIR->IcsNode.RclCnt > k) {
                k = testTFDIR->IcsNode.RclCnt;
                printf("Reclose #:%d Following  the Faulted Trip\n", k);
            }
#endif
        }
    }
    else {
        cout << "Not enough arguments!!!" << endl;
    }
    return 0;
}