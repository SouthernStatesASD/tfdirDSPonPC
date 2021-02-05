/*
 * TFDIRContainer.cpp
 *
 *  Created on: Feb 12, 2018
 *      Author: user
 */

#include <stdio.h>
#include "TFDIRContainer.h"
#include <algorithm>

#ifdef RUNNING_TEST
    #include "Comtrade.h"
#else
    #include "dspDebugLog.h"
#endif

#ifndef RUNNING_TEST
    TFDIRContainer::TFDIRContainer(CircularBuffer *circularBuf) {
        // TODO Auto-generated constructor stub
        gCircularBuf = circularBuf;

        Initialize();
    }
#else
    TFDIRContainer::TFDIRContainer() {
        // TODO Auto-generated constructor stub

        Initialize();
}
#endif

TFDIRContainer::~TFDIRContainer() {
    // TODO Auto-generated destructor stubDEBU
}

void TFDIRContainer::Initialize() {
    executionCt = 0;
    configLoaded = false;
    inSimulationMode = false;
    XY_Size = sizeof(XY);
    PHASOR_Size = sizeof(PHASOR);
    DataStreamSize = sizeof(DataStream);
    floatSize = sizeof(float);
    EVT_LOG_Size = sizeof(EVT_LOG);
    EVT_Total_Size = EVT_SIZE * EVT_LOG_Size;
    memset(&isInitialSample, 1, MAX_SETS*MES_PHASES*sizeof(bool));
    memset(&setsPhasesTotalSampleCt, 0, MAX_SETS*MES_PHASES*sizeof(int));
    memset(&prevSample, 0, MAX_SETS*MES_PHASES*sizeof(DataStream));
    memset(&sampleBufferCounter, 0, MAX_SETS*MES_PHASES*sizeof(int));
    memset(&prevExecution, 0, sizeof(ExecutionStamp));
    memset(&initialRefSensor, 0, sizeof(SensorIdentifier));

    for(int i=0; i<MAX_SETS-1; i++) {
        memset(&SmpNode.SmpSet[i].phase_st, 0, MES_PHASES*sizeof(double));
        memset(&SmpNode.SmpSet[i].phase_t, 0, MES_PHASES*sizeof(TIME));
        memset(&SmpNode.SmpSet[i].phase_t_ct, 0, MES_PHASES*sizeof(long long));
        memset(&SmpNode.SmpSet[i].I, 0, MES_PHASES*SV_CYC_SIZE*sizeof(float));
        memset(&SmpNode.SmpSet[i].V, 0, MES_PHASES*SV_CYC_SIZE*sizeof(float));
    }

    configContentsArray = new char*[300];
    for(int i=0; i<300; i++) {
        configContentsArray[i] = new char[LINE_READ_BUFFER];
    }

#ifndef RUNNING_TEST
    //****
    // Initialize shared memory buffers and set the buffer list
    //**
    for (int i = 0; i < 3; i++)
    {
        const char* buf_name = MsgBase::DSP_BINARY_RECORDING_NAMES[i];
        SharedMemoryBuffer* sharedMemory = new SharedMemoryBuffer(buf_name, MsgBase::DSP_BINARY_RECORDING_BUF_SIZE);
        if (sharedMemory->Initialize())
        {
            sharedMemList.push_back(sharedMemory);
        }
        else
        {
            printf("ERROR: DSP: initializing shared memory, %S", buf_name);
        }
    }
    sharedMemIt = sharedMemList.begin();  // the iterator for the shared memory buffer list
#endif

    sampleSeq = 0;
}

int TFDIRContainer::Configure(FLAG SetOption, char *configContents) {
    int i, j, k;
    int Error = 0;
    float x = 0.0;
    float y = 6.2831853/float(SV_CYC_SIZE); // Gradia/SV = 2Pi/Ncycle
    // Gradian/SV = 2Pi*Tsv/Tcycle - Tcycle = 1000/f (ms)
// Cold Start
    if (!SetOption) {
        memset(&IcsNode, 0, sizeof(ICS_NODE));
        memset(&CfgNode, 0, sizeof(CFG_NODE));

// Set pointer link to Confiuration structure
        IcsNode.Cfg = &CfgNode;
        for (i = 0; i < MAX_SETS; i++) {
            IcsNode.IcsSet[i].Cfg = &CfgNode.CfgSet[i];
            for (j = 0; j < MES_PHASES; j++) {
                IcsNode.IcsSet[i].IcsSns[j].Cfg = &CfgNode.CfgSet[i].CfgSns[j];
                IcsNode.IcsSet[i].IcsSns[j].Vmdcf = 1.0;
                IcsNode.IcsSet[i].IcsSns[j].Vadcf = 0.0;
            }
        }
        IcsNode.VoltSetNum = 0; // no valid voltage set defined yet

// Populate Configuration Structures from Configuration file
        Error = parseConfig(configContents);

        for (i = 0; i < MAX_SETS; i++) {
            for (j = 0; j < MES_PHASES; j++) {
                for (k = 0; k <= BUF_SIZE; k++) {
                    IcsNode.IcsSet[i].IcsSns[j].Ibuf[k].flg = 1;
                    IcsNode.IcsSet[i].IcsSns[j].Vbuf[k].flg = 1;
                }
                IcsNode.IcsSet[i].Cfg->ZoneSetting[j] /= 100.0;
            }
            IcsNode.IcsSet[i].IcsSns[0].Phase = PHASE_A;
            IcsNode.IcsSet[i].IcsSns[1].Phase = PHASE_B;
            IcsNode.IcsSet[i].IcsSns[2].Phase = PHASE_C;
            memset(IcsNode.IcsSet[i].Evt, 0, EVT_SIZE*sizeof(EVT_LOG));
        }

// Set Voltage Model Matrix

        IcsNode.VoltMtrx[1][0] = IcsNode.VoltMtrx[0][1] = 0.0;
        IcsNode.VoltMtrx[2][0] = IcsNode.VoltMtrx[0][2] = 0.0;
        IcsNode.VoltMtrx[2][1] = IcsNode.VoltMtrx[1][2] = 0.0;
        IcsNode.VoltMtrx[0][0] = IcsNode.VoltMtrx[1][1] = IcsNode.VoltMtrx[2][2] = 1.0;

        Error = VoltModel();  // Calculate voltage matrix based on pole layout parameters

        if (Error != 0)
            printf("Error: Voltage Model Matrix is Singular !!!");
        IcsNode.VmPT = IcsNode.Cfg->Vrated/1.732051; // Default to the rated phase voltage

// initialize to configured frequency
        IcsNode.SysFreq = IcsNode.Cfg->SysFreq;        // initialize to configured freq, to be updated dynamicly
        IcsNode.Ms2Degrees = 0.36*(IcsNode.SysFreq);   // Milliseconds to degrees
        IcsNode.Degrees2Ms = 1.0/(IcsNode.Ms2Degrees); // Degrees to milliseconds
        IcsNode.Cycle2Sec  = 1.0/(IcsNode.SysFreq);	   // Cycle to seconds

// Validate the parameters
        IcsNode.ToqAngLead = IcsNode.Cfg->MTA + 0.5*IcsNode.Cfg->TAW;
        IcsNode.ToqAngLag  = IcsNode.Cfg->MTA - 0.5*IcsNode.Cfg->TAW;

        if (IcsNode.Cfg->DeadTime < 1.1*IcsNode.Cfg->RclWaitTime)
            IcsNode.Cfg->DeadTime = 1.1*IcsNode.Cfg->RclWaitTime;
// ?? Verify CT Polarities, CT/VT Rotations (ABC, not ACB)
// ?? Verify the assignments of the phases at the right arrays (A,B,C to [0],[1],[2])

// Initalize the SinV and CosV Arrays for FFT
        for (i = 0; i < SV_CYC_SIZE; i++) {
            IcsNode.SinV[i] = sin(x);
            IcsNode.CosV[i] = cos(x);
            x += y;
        }
// ICS Sets with real Sensors installed
        IcsNode.NumActSets = 0;
        IcsNode.NumMesSets = 0;
        for (i = 0; i < MAX_SETS; i++) {
            if (IcsNode.IcsSet[i].Cfg->SrvFlg) {
                IcsNode.NumActSets += 1;
                if (!IcsNode.IcsSet[i].Cfg->VrtFlg)
                    IcsNode.NumMesSets += 1;
            }
        }
        if ((IcsNode.NumActSets - IcsNode.NumMesSets) > 1) {
            printf("Error (2) \n ActiveICSSets(%d) and MeasICSSets(%d): %d-%d> 1 \n\n", IcsNode.NumActSets, IcsNode.NumMesSets,
                       IcsNode.NumActSets, IcsNode.NumMesSets);
            Error =  2;
        }
    }
    else if (SetOption < 2) {
// Full reset the operation variables, e.g., some counters, timers, flags

    }
    else {
// Reset Partial Operation Variables

    }

//    smpTimeResolution = round(1000000 * 1.0/(CfgNode.SysFreq*SV_CYC_SIZE/2.0))/1000000;
    smpTimeResolution = 1.0/(CfgNode.SysFreq*SV_CYC_SIZE/2.0);
    cycleDuration = 1.0/CfgNode.SysFreq - smpTimeResolution/2.0;

    degreesPerSec = 360.0*CfgNode.SysFreq;
    radiansPerSec = 2*M_PI*CfgNode.SysFreq;

    Ir = 2.0*CfgNode.CfgSet[0].Irated;
    Vr = CfgNode.Vrated;

    Ir_inv = 0.5/CfgNode.CfgSet[0].Irated;
    Vr_inv = 1.0/CfgNode.Vrated;

//    printf("Ratings: %f, %f, %f, %f", Ir, Vr, Ir_inv, Vr_inv);

    //set the valid range for I and V
    ILimit = 100*CfgNode.CfgSet[0].Irated;
    if (ILimit > 32767)
        ILimit = 32767;

    VLimit = 10*CfgNode.Vrated;
    if (VLimit > 32767)
        VLimit = 32767;

    //test with single set
//	IcsNode.NumMesSets = 1;

    //test with single phase
    neededSetsPhasesTotalSampleCt = SV_CYC_SIZE;

    //set the needed number of samples for a full complete cycle of all sets and phases
//	neededSetsPhasesTotalSampleCt = IcsNode.NumMesSets * MES_PHASES * SV_CYC_SIZE;

    return Error;
}

int TFDIRContainer::parseConfig(char *configContents) {

    //variable init
    char *parameter, name[NAME_LEN], line[LINE_READ_BUFFER];
    int parseMode, i=0, j=0, k=0, numNodeParams=0, numSetParams=0, numSnsParams=0;
    int Error = 0;
    PARAM param;

    FLAG *_nodeFlagMap[] = {
            //		&CfgNode.PoleConfig,
            &CfgNode.RptPermFlt,
            &CfgNode.RptTempFlt,
            &CfgNode.RptMmtEvt,
            //		&CfgNode.RptOpr,
            &CfgNode.AutoSwitchOpen,
            &CfgNode.MaxRclNum
    };

    float *_nodeFloatMap[] = {
            &CfgNode.Da,
            &CfgNode.Db,
            &CfgNode.Dc,
            &CfgNode.Dab,
            &CfgNode.Dbc,
            &CfgNode.Dca,
            &CfgNode.daa,
            &CfgNode.PCTfact,
            &CfgNode.SysFreq,
            &CfgNode.RclWaitTime,
            &CfgNode.HotTime,
            &CfgNode.DeadTime,
            &CfgNode.FltTime,
            &CfgNode.RtnTime,
            //		&CfgNode.RptOprTime,
            &CfgNode.MTA,
            &CfgNode.TAW,
            &CfgNode.Vrated,
            &CfgNode.Vmin,
            &CfgNode.Vmax,
            &CfgNode.Imin
    };

    char *_nodeFlagParams[] = {
            //		"PoleConfig",
            "ReportPermFault",
            "ReportTempFault",
            "ReportMomentEvent",
            //		"ReportOprValues",
            "AutoSwitchOpen",
            "NumOfRclsTimes",
            NULL
    };

    char *_nodeFloatParams[] = {
            "Da",
            "Db",
            "Dc",
            "Dab",
            "Dbc",
            "Dca",
            "daa",
            "PCTfact",
            "SysFrequency",
            "RclsWaitTime",
            "HotTime",
            "DeadTime",
            "FaultTime",
            "ReturnTime",
            //		"RptOprInterval",
            "MTA",
            "TAW",
            "RatedVolt",
            "LowVoltLimit",
            "HighVoltLimit",
            "LowAmpLimit",
            NULL
    };


    char *_setFlagParams[] = {
            "SequenceNum",
            "ServiceFlag",
            "VirtualFlag",
            "SetTypeFlag",
            "CfgZone",
            "UseCfgZone",
            "SwitchEnabled",
            NULL
    };

    char *_setFloatParams[] = {
            "R0",
            "X0",
            "R1",
            "X1",
            "Rs0",
            "Xs0",
            "Rs1",
            "Xs1",
            "AmpSclFactor",	 // Amp Scale factor to actual engineering value from what measured
            "AmpAngOffset",	 // Amp angle offset (0/+/-: none/Add/Subtract to/from the angle measured
            "VoltSclFactor", // Amp Scale factor to actual engineering value from what measured
            "VoltAngOffset", // Amp angle offset (0/+/-: none/Add/Subtract to/from the angle measured
            "LineLength",
            "TapSegLength",
            "Zone1Setting",
            "Zone2Setting",
            "RatedPhaseAmp",
            "RatedNeutralAmp",
            "PhaseFaultSetting",
            "NeutralFaultSetting",
            "DeltaFaultSetting",
            "PhaseSettingDB",
            "NeutralSettingDB",
            NULL
    };

    char *_sensorFlagParams[] = {
            "SequenceNum",
            "UseVT",
            "VTasRef",
            "CTPolarity",
            "PhaseID",
            NULL
    };

    nodeFlagMap = _nodeFlagMap;
    nodeFloatMap = _nodeFloatMap;
    nodeFlagParams = _nodeFlagParams;
    nodeFloatParams = _nodeFloatParams;
    setFlagParams = _setFlagParams;
    setFloatParams = _setFloatParams;
    sensorFlagParams = _sensorFlagParams;

    NUM_NODE_PARAMS = ((((int)sizeof(_nodeFlagParams)+(int)sizeof(_nodeFloatParams))/(int)sizeof(char *))-2);
    NUM_SET_PARAMS = ((((int)sizeof(_setFlagParams)+(int)sizeof(_setFloatParams))/(int)sizeof(char *))-2);
    NUM_SNS_PARAMS = (((int)sizeof(_sensorFlagParams)/(int)sizeof(char *))-1);

    nodeFound = new int[NUM_NODE_PARAMS];
    setFound = new int[NUM_SET_PARAMS];
    snsFound = new int[NUM_SNS_PARAMS];

    for (int i=0; i<NUM_NODE_PARAMS; i++) {
        nodeFound[i] = 0;
    }
    for (int i=0; i<NUM_SET_PARAMS; i++) {
        setFound[i] = 0;
    }
    for (int i=0; i<NUM_SNS_PARAMS; i++) {
        snsFound[i] = 0;
    }

    int numLines = 0;
    if (configContents != NULL) {
        char *readline = strtok(configContents, "\n");

        while(readline != NULL) {
            strncpy(configContentsArray[numLines], readline, LINE_READ_BUFFER);
            readline = strtok(NULL, "\n");
            numLines++;
        }
    }

    i = 0;
    for(int lineCt=0; lineCt<numLines; lineCt++){
        strncpy(line, configContentsArray[lineCt], LINE_READ_BUFFER);

        if(*line == '/' || *line=='\n' || *line == 0x0d || *line == ' '|| *line == '\0' || *line == '\t')
            continue; //ignore comment lines

        parameter = strtok(line,"//"); //remove comment from line with parameter

        //Determine parameter type to parse (node,set, or sensor) & set set or sensor name
        if(parameter[0]=='&'){
            parseMode = NODE;
            continue;
        }
        else if(parameter[0]=='@'){
            //ready to parse a new SET
            parseMode = SET;
            //check if last sensor set is filled
            if(numSnsParams > 0) {
                Error = checkSns();
                if (numSnsParams != NUM_SNS_PARAMS) {
                    printf("Some parameters missing in %s\n",CfgNode.CfgSet[i-1].CfgSns[j-1].Name);
                    Error++;
                }
            }
            //reset check array
            for(k=0;k<NUM_SNS_PARAMS;k++){
                snsFound[k]=0;
            }
            //check if last set filled

            if(numSetParams >0 ) {
                Error = checkSet();
                if(numSetParams !=  NUM_SET_PARAMS) {
                    printf("Some parameter missing in %s\n",CfgNode.CfgSet[i-1].Name);
                    Error++;
                }
            }
            //reset check array
            for(k=0;k<NUM_SET_PARAMS;k++){
                setFound[k]=0;
            }

            //Warning for missing sensors
            if(i>0 && j<MES_PHASES){
                printf("Error: only %d sensors defined in %s\n", j, CfgNode.CfgSet[i-1].Name);
                Error++;
            }
            if(i == MAX_SETS)
                break;
            //extract & set set name
//			sscanf(parameter,"%*s %s",name);
            sscanf(parameter,"%s",name);
            strncpy(CfgNode.CfgSet[i].Name, name, NAME_LEN);
            //update indices
            i++;
            j=0;
            numSetParams=0;
            continue;
        }
        else if(parameter[0]=='#'){
            //check if all sensor parameters specified
            if(j>0 && numSnsParams>0) {
                Error = checkSns();
                if(numSnsParams != NUM_SNS_PARAMS) {
                    Error++;
                }
            }
            //reset check array
            for(k=0;k<NUM_SNS_PARAMS;k++){
                snsFound[k]=0;
            }
            //reset sensor count for next set
            if(j==MES_PHASES){
                j=0;
            }
            numSnsParams=0;
            parseMode = SENSOR;
            //extract name & update index
//			sscanf(parameter,"%*s %s",name);
            sscanf(parameter,"%s", name);
//			printf("DSP Sns name: %s, %s", parameter, name);
            strncpy(CfgNode.CfgSet[i-1].CfgSns[j].Name, name, NAME_LEN);
            j++;
            continue;
        }

        //Populate parameter structures & initialize structs
        if(parseMode==NODE){
            populateParam(line, &param);
            populateNode(param);
            numNodeParams++;
        }
        else if(parseMode==SET){
            populateParam(line, &param);
            populateSet(param, &(CfgNode.CfgSet[i-1]));
            numSetParams++;
        }
        else if(parseMode==SENSOR){
            populateParam(line, &param);
            populateSns(param, &CfgNode.CfgSet[i-1].CfgSns[j-1]);
            numSnsParams++;
        }

    }

    //Final check
    checkNode();

    if(numSetParams !=  NUM_SET_PARAMS){
        checkSet();
        printf("Some SET parameters missing in %s\n",CfgNode.CfgSet[i-1].Name);
        Error++;
    }
    if(numSnsParams != NUM_SNS_PARAMS){
        checkSns();
        printf("Some SNS parameters missing in %s\n",CfgNode.CfgSet[i-1].CfgSns[j-1].Name);
        Error++;
    }
    if(j<MES_PHASES){
        printf("Error: Only %d sensors defined in %s\n", j, CfgNode.CfgSet[i-1].Name);
        Error++;
    }
    if(i != MAX_SETS){
        printf("Warning: Only %d sets defined \n", i);
    }

    memset(configContentsArray, 0, sizeof(configContentsArray));

//	if (Error == 0) {
    configLoaded = true;
//	}

    return Error;
}

void TFDIRContainer::populateParam(char *line, PARAM *param){

    char *paramName;
    float val,min,max;

    if(line[0]=='A'){
        param->val = PHASE_A;
        strncpy(param->Name,"PhaseID", NAME_LEN);
    }
    else if(line[0]=='B'){
        param->val = PHASE_B;
        strncpy(param->Name,"PhaseID", NAME_LEN);
    }
    else if(line[0]=='C'){
        param->val = PHASE_C;
        strncpy(param->Name,"PhaseID", NAME_LEN);
    }
    else{
        sscanf(strtok(line,",["),"%f",&val); //extract value
        sscanf(strtok(NULL,"[ ,"),"%f",&min); //extract minimum
        sscanf(strtok(NULL,", ]"),"%f",&max); //extract maximum
        paramName = strtok(NULL,", [ ]"); //extract parameter name

        //Range validation

        strncpy(param->Name, paramName, NAME_LEN);
        param->val = val;
        if(val < min || val > max) {
//            printf("Parameter %s (%f) is out of range. Must be between %f and %f.\n",paramName,val,min,max);
            if (val < min) {
                param->val = min;
                printf("Set to the lower limit: (%f).\n", min);
            }
            else {
                param->val = max;
                printf("Set to the upper limit: (%f).\n", max);
            }
            //exit(EXIT_FAILURE);
        }
    }
}

int TFDIRContainer::populateNode(PARAM param){

    int Error = 0, i=0,j=0,found=0;

    //Assign and typecast to correct type matching map
    while(nodeFlagParams[i]){
        if(!strncmp(param.Name,nodeFlagParams[i], NAME_LEN)) {
            if(!nodeFound[j]) {
                *nodeFlagMap[i]= (FLAG)param.val;
                found = 1;
                nodeFound[j]=1;
                break;
            }
            else {
                nodeFound[j]++;
//                printf("Flag inc: %s", param.Name);
            }
        }
//        printf("Flag nodeFound: %d, %d", j, nodeFound[j]);
        i++;
        j++;
    }
    i=0;
    if(!found) {
        while(nodeFloatParams[i]){
            if(!strncmp(param.Name,nodeFloatParams[i], NAME_LEN)) {
//            	printf("Found Float: %s, %d, %d", param.Name, j, nodeFound[j]);
                if (!nodeFound[j]) {
//                	printf("nodeFound: %s", param.Name);
                    *nodeFloatMap[i]= param.val;
                    found = 1;
                    nodeFound[j]=1;
                    break;
                }
                else {
                    nodeFound[j]++;
                }
            }
            i++;
            j++;
        }
    }
    //Error checking if invalid parameter is entered (typos)
    if(!found) {
        printf("Invalid Node Parameter: %s\n",param.Name);
        Error++;
    }
    return Error;
}

void TFDIRContainer::populateSet(PARAM param, CFG_SET *Set){

    FLAG *setFlagMap[] = {
            &Set->SeqNum,
            &Set->SrvFlg,
            &Set->VrtFlg,
            &Set->SetTyp,
            &Set->CfgZone,
            &Set->UseCfgZone,
            &Set->SwitchEnabled,
            NULL
    };

    float *setFloatMap[] = {
            &(Set->Z0).r,
            &(Set->Z0).x,
            &(Set->Z1).r,
            &(Set->Z1).x,
            &(Set->Zs0).r,
            &(Set->Zs0).x,
            &(Set->Zs1).r,
            &(Set->Zs1).x,
            &Set->AmpSclFactor,
            &Set->AmpAngOffset,
            &Set->VoltSclFactor,
            &Set->VoltAngOffset,
            &Set->LineLength,
            &Set->TapSegLength,
            &Set->ZoneSetting[0],
            &Set->ZoneSetting[1],
            &Set->Irated,
            &Set->Inrated,
            &Set->Ifm,
            &Set->Ifnm,
            &Set->Ifcm,
            &Set->Ifdb,
            &Set->Ifndb,
            NULL
    };

    int i=0,j=0,found=0;

    while(setFloatParams[i]){
        if(!strncmp(param.Name,setFloatParams[i], NAME_LEN)) {
            if(!setFound[j]) {
                *setFloatMap[i]= param.val;
                found=1;
                setFound[j]=1;
                break;
            }
            else {
                setFound[j]++;
            }
        }
        i++;
        j++;
    }
    i=0;
    if(!found) {
        while(setFlagParams[i]) {
            if(!strncmp(param.Name,setFlagParams[i], NAME_LEN)) {
                if (!setFound[j]) {
                    *setFlagMap[i]= (FLAG) param.val;
                    found=1;
                    setFound[j]=1;
                    break;
                }
                else {
                    setFound[j]++;
                }
            }
            i++;
            j++;
        }
    }

    if(!found){
        printf("Invalid Parameter in %s: %s\n",Set->Name, param.Name);
    }
}

void TFDIRContainer::populateSns(PARAM param, CFG_SNS *Sensor){

    FLAG *sensorFlagMap[] = {
            &Sensor->SeqNum,
//			&Sensor->UseCT,
            &Sensor->UseVT,
            &Sensor->RefVT,
            &Sensor->CTPolarity,
            &Sensor->PhaseID,
            NULL
    };

    int i=0,found=0;
    while(sensorFlagParams[i]){
        if(!strncmp(param.Name,sensorFlagParams[i], NAME_LEN)){
            if (!snsFound[i]) {
                *sensorFlagMap[i]= (FLAG) param.val;
                found=1;
                snsFound[i]=1;
                break;
            }
            else {
                snsFound[i]++;
            }
        }
        i++;
    }

    if(!found){
        printf("Invalid Parameter in %s: %s\n",Sensor->Name, param.Name);
    }
}

int TFDIRContainer::checkNode(){
    int Error = 0, i=0,j=0;
    while(nodeFlagParams[i]){
        if(nodeFound[j] == 0){
            printf("Missing node parameter: %s, \n",nodeFlagParams[i]);
            Error++;
        }
        else if(nodeFound[j] > 1) {
            printf("Node parameter: %s, has %d name conflicts within the length of %d characters \n",
                       nodeFlagParams[i], nodeFound[j], NAME_LEN);
            Error++;
        }
        i++;
        j++;
    }
    i=0;
    while(nodeFloatParams[i]){
        if(nodeFound[j]==0){
            printf("Missing node parameter: %s, \n",nodeFloatParams[i]);
            Error++;
        }
        else if(nodeFound[j] > 1) {
            printf("Node parameter: %s, has %d name conflicts within the length of %d characters \n",
                       nodeFloatParams[i], nodeFound[j], NAME_LEN);
            Error++;
        }
        i++;
        j++;
    }
    return Error;
}

int TFDIRContainer::checkSet(){
    int Error = 0, i=0,j=0;
    while(setFloatParams[i]){
        if(setFound[j]==0){
            printf("Missing set parameter: %s, \n",setFloatParams[i]);
            Error++;
        }
        else if(setFound[j] > 1) {
            printf("Set parameter: %s, has %d name conflicts within the length of %d characters \n",
                       setFloatParams[i], setFound[j], NAME_LEN);
            Error++;
        }
        i++;
        j++;
    }
    i=0;
    while(setFlagParams[i]){
        if(setFound[j] == 0){
            printf("Missing set parameter: %s, \n",setFlagParams[i]);
            Error++;
        }
        else if(setFound[j] > 1) {
            printf("Set parameter: %s, has %d name conflicts within the length of %d characters \n",
                       setFlagParams[i], setFound[j], NAME_LEN);
            Error++;
        }
        i++;
        j++;
    }
    return Error;
}

int TFDIRContainer::checkSns(){
    int Error = 0, i=0;
    while(sensorFlagParams[i]){
        if(snsFound[i]==0){
            printf("Missing sensor parameter: %s, \n",sensorFlagParams[i]);
            Error++;
        }
        else if(snsFound[i] > 1) {
            printf("Sensor parameter: %s, has %d name conflicts within the length of %d characters \n",
                       sensorFlagParams[i], snsFound[i], NAME_LEN);
            Error++;
        }
        i++;
    }
    return Error;
}

void TFDIRContainer::sendBufferedDataToArm() {
#ifndef RUNNING_TEST
    // Send the binary data
    char* shared_buf = NULL;
    int   shared_buf_size(0);
    SharedMemoryBuffer* sharedBuf = *sharedMemIt;
    if (sharedBuf->AcquireBuffer(&shared_buf, shared_buf_size))
    {
        if (shared_buf != NULL)
        {
            // copy the binary data into the shared memory buffer
            gCircularBuf->Read(shared_buf, shared_buf_size);

            // Release the shared buffer and notify the reader that data is available
            if (sharedBuf->ReleaseAndNotify(gCircularBuf->BytesInBuffer()))
            {
                printf("DSP: Sending data...");
                gCircularBuf->Reset(0);       // reset the buffer after sending (set to all 0)
            }
        }
    }

    // Increment to the next shared memory buffer
    sharedMemIt++;
    if (sharedMemIt == sharedMemList.end())
    {
        sharedMemIt = sharedMemList.begin();
    }
#endif
}

int TFDIRContainer::loadSimulation(char *simulationData) {

    int Error = 0;

    if (simulationData != NULL) {
        int lineCt = 0;
        long sec, nsec;
        int ia, ib, ic, va, vb, vc, ia_05ms, ib_05ms, ic_05ms, va_05ms, vb_05ms, vc_05ms;

        char *readline = strtok(simulationData, "\n");
        while(readline != NULL) {
//			strncpy(simulationContentsArray[numLines], readline, LINE_READ_BUFFER);

            //check if this is the end of simulation
            if(strcmp(readline,"Simulation Complete") == 0) {
                resetAllBuffers();
                inSimulationMode = false;
                printf("Fault simulation complete");
            } else {
                if (!inSimulationMode) {
                    inSimulationMode = true;
                    memset(&isInitialSample, 1, MAX_SETS*MES_PHASES*sizeof(bool));

                    //reset existing buffers
                    resetAllBuffers();

                    printf("Fault simulation started");
                }

//        		printf("DSP simulation line: %s", readline);

                sscanf(readline,"%ld,%ld,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d",
                       &sec, &nsec, &ia, &ib, &ic, &va, &vb, &vc, &ia_05ms, &ib_05ms, &ic_05ms, &va_05ms, &vb_05ms, &vc_05ms);

//        		printf("Decoded: %d,%ld,%ld,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d",
//        		        				lineCt, sec, nsec, ia, ib, ic, va, vb, vc, ia_05ms, ib_05ms, ic_05ms, va_05ms, vb_05ms, vc_05ms);

                setSimDataStream(simDataStream_a, 0+lineCt*6, sec, nsec, ia, ia_05ms, va, va_05ms);
                setSimDataStream(simDataStream_b, 2+lineCt*6, sec, nsec, ib, ib_05ms, vb, vb_05ms);
                setSimDataStream(simDataStream_c, 4+lineCt*6, sec, nsec, ic, ic_05ms, vc, vc_05ms);

#ifndef RUNNING_TEST
                addToSampleBuffer(simDataStream_a, lineCt);
                addToSampleBuffer(simDataStream_b, lineCt);
                addToSampleBuffer(simDataStream_c, lineCt);
#endif
            }

            readline = strtok(NULL, "\n");
            lineCt++;
        }
    }

    return Error;
}

void TFDIRContainer::resetAllBuffers() {
    //***
    //should clear out the samples and the ics buffers
    //***
    memset(&prevSample, 0, MAX_SETS*MES_PHASES*sizeof(DataStream));

    for(int i=0; i<MAX_SETS-1; i++) {
        SmpNode.SmpSet[i].t.sec = 0;
        SmpNode.SmpSet[i].t.usec = 0;
        memset(&SmpNode.SmpSet[i].I, 0, MES_PHASES*SV_CYC_SIZE*sizeof(float));
        memset(&SmpNode.SmpSet[i].V, 0, MES_PHASES*SV_CYC_SIZE*sizeof(float));
    }

    for (int i = 0; i < MAX_SETS; i++) {
        IcsNode.IcsSet[i].ChgLch = 0;
        IcsNode.IcsSet[i].FltLch = 0;
        IcsNode.IcsSet[i].NtrLch = 0;
        IcsNode.IcsSet[i].TypLch = 0;
        IcsNode.IcsSet[i].TypLch = 0;
        IcsNode.IcsSet[i].FltCnt = 0;
        IcsNode.IcsSet[i].FltZone = 0;
        IcsNode.IcsSet[i].OpenSw = 0;
        memset(&IcsNode.IcsSet[i].Evt, 0, EVT_SIZE*sizeof(EVT_LOG));

        memset(&SmpNode.SmpSet[i].I, 0, MES_PHASES*SV_CYC_SIZE*XY_Size);
        memset(&SmpNode.SmpSet[i].V, 0, MES_PHASES*SV_CYC_SIZE*XY_Size);
        for (int j = 0; j < MES_PHASES; j++) {
            memset(&IcsNode.IcsSet[i].IcsSns[j].Ibuf, 0, (BUF_SIZE+1)*XY_Size);
            memset(&IcsNode.IcsSet[i].IcsSns[j].Vbuf, 0, (BUF_SIZE+1)*XY_Size);
            memset(&IcsNode.IcsSet[i].IcsSns[j].Ia, 0, XY_Size);
            memset(&IcsNode.IcsSet[i].IcsSns[j].Va, 0, XY_Size);
        }

    }

    IcsNode.RclCnt = 0;
    IcsNode.TrpCnt = 0;
    IcsNode.LchFlg = 0;
    IcsNode.FltLck = 0;   // Clear fault lock
    IcsNode.EvtTrigger = 0;

    //reset data streams
    int datastream_size = sizeof(DataStream);
    memset(&simDataStream_a, 0, datastream_size);
    memset(&simDataStream_b, 0, datastream_size);
    memset(&simDataStream_c, 0, datastream_size);
}

void TFDIRContainer::setSimDataStream(DataStream & dataStream, int id, uint32_t secs, uint32_t nsecs, int i, int i_05ms, int v, int v_05ms){
    dataStream.data.id = id;
    dataStream.data.time.secs = secs;
    dataStream.data.time.nsecs = nsecs;
    dataStream.data.sample.currentNow = i;
    dataStream.data.sample.current0_5msAgo = i_05ms;
    dataStream.data.sample.voltageNow = v;
    dataStream.isCRCgood = true;
}


int TFDIRContainer::VoltModel()
{
/*
  Calculate voltage matrix based on pole layout parameters
  Calculate the line mutual to self-capacitance ratio
*/
    int   i, j;
    int   error;
    float d, x, y, w, s;
    float a[MES_PHASES][MES_PHASES];

    s = IcsNode.Cfg->PCTfact;
    x = fabs(IcsNode.Cfg->Da - IcsNode.Cfg->Db);
    y = IcsNode.Cfg->Dab*IcsNode.Cfg->Dab - x*x;
    w = IcsNode.Cfg->Da + IcsNode.Cfg->Db;
    d = sqrt(y + w*w);
    a[0][1] = a[1][0] = s*log(d)/log(IcsNode.Cfg->Dab);

    x = fabs(IcsNode.Cfg->Db - IcsNode.Cfg->Dc);
    y = IcsNode.Cfg->Dbc*IcsNode.Cfg->Dbc - x*x;
    w = IcsNode.Cfg->Db + IcsNode.Cfg->Dc;
    d = sqrt(y + w*w);
    a[1][2] = a[2][1] = s*log(d)/log(IcsNode.Cfg->Dbc);

    x = fabs(IcsNode.Cfg->Dc - IcsNode.Cfg->Da);
    y = IcsNode.Cfg->Dca*IcsNode.Cfg->Dca - x*x;
    w = IcsNode.Cfg->Dc + IcsNode.Cfg->Da;
    d = sqrt(y + w*w);
    a[2][0] = a[0][2] = s*log(d)/log(IcsNode.Cfg->Dca);

    d = 2.0*IcsNode.Cfg->Da/IcsNode.Cfg->daa;
    a[0][0] = log(d);
    d = 2.0*IcsNode.Cfg->Db/IcsNode.Cfg->daa;
    a[1][1] = log(d);
    d = 2.0*IcsNode.Cfg->Dc/IcsNode.Cfg->daa;
    a[2][2] = log(d);

    x = a[0][0] - 0.5*(a[0][1] + a[0][2]);
    y = 0.8660254*(a[0][2] - a[0][1]);
    IcsNode.Vmref[0].mag = XY2Mag(x, y);
    IcsNode.Vmref[0].ang = XY2Ang(x, y);

    x = a[1][1] - 0.5*(a[1][0] + a[1][2]);
    y = 0.8660254*(a[1][0] - a[1][2]);
    IcsNode.Vmref[1].mag = XY2Mag(x, y);
    IcsNode.Vmref[1].ang = XY2Ang(x, y);

    x = a[2][2] - 0.5*(a[2][0] + a[2][1]);
    y = 0.8660254*(a[2][1] - a[2][0]);
    IcsNode.Vmref[2].mag = XY2Mag(x, y);
    IcsNode.Vmref[2].ang = XY2Ang(x, y);

    s = (IcsNode.Vmref[0].mag + IcsNode.Vmref[1].mag + IcsNode.Vmref[2].mag)/3.0;
    for (i = 0; i < 3; i++) {
        IcsNode.Vmref[i].mag /= s;
        for (j = 0; j < 3; j++)
            a[i][j] /= s;
    }
/*
	printf ("Vmref: %8.2f/%8.2f  %8.2f/%8.2f  %8.2f/%8.2f\n\n",
			IcsNode.Vmref[0].mag, IcsNode.Vmref[0].ang,
			IcsNode.Vmref[1].mag, IcsNode.Vmref[1].ang,
			IcsNode.Vmref[2].mag, IcsNode.Vmref[2].ang);

    printf ("Source Matrix A: \n");
    printf ("  |%8.6f %8.6f %8.6f| \n",  a[0][0], a[0][1], a[0][2]);
    printf ("A=|%8.6f %8.6f %8.6f| \n",  a[1][0], a[1][1], a[1][2]);
    printf ("  |%8.6f %8.6f %8.6f| \n\n",a[2][0], a[2][1], a[2][2]);
*/
    memset(IcsNode.VoltMtrx, 0, 9*sizeof(float));
    IcsNode.VoltMtrx[0][0] = IcsNode.VoltMtrx[1][1] = IcsNode.VoltMtrx[2][2] = 1.0;

    error = MatInv((float *)IcsNode.VoltMtrx, (float *)a, MES_PHASES);
/*
    printf ("Matrix B -Inversed from A: \n");
    printf ("  |%8.6f %8.6f %8.6f| \n",  IcsNode.VoltMtrx[0][0], IcsNode.VoltMtrx[0][1], IcsNode.VoltMtrx[0][2]);
    printf ("B=|%8.6f %8.6f %8.6f| \n",  IcsNode.VoltMtrx[1][0], IcsNode.VoltMtrx[1][1], IcsNode.VoltMtrx[1][2]);
    printf ("  |%8.6f %8.6f %8.6f| \n\n",IcsNode.VoltMtrx[2][0], IcsNode.VoltMtrx[2][1], IcsNode.VoltMtrx[2][2]);
*/
    return error;
}

void TFDIRContainer::CalibVolt ()
{
    int i, j, k;
    float x, w, yi, yv, im, vm;

    k = IcsNode.Flt2Long + IcsNode.RclCnt + IcsNode.TrpCnt + IcsNode.LchFlg;
    for (i = 0; i < MAX_SETS; i++) {
        if (!IcsNode.IcsSet[i].Cfg->SrvFlg || IcsNode.IcsSet[i].Cfg->VrtFlg ||
            IcsNode.IcsSet[i].LineFlg != PHASE_ABC || IcsNode.IcsSet[i].LineFlg0 != PHASE_ABC)
            continue;
        k += IcsNode.IcsSet[i].ChgLch + IcsNode.IcsSet[i].FltLch +
             IcsNode.IcsSet[i].NtrLch + IcsNode.IcsSet[i].TypLch +
             IcsNode.IcsSet[i].FltCnt + IcsNode.IcsSet[i].NtrCnt + IcsNode.IcsSet[i].ChgCnt;
        for (j = 0; j < MES_PHASES; j++)
            k += IcsNode.IcsSet[i].IcsSns[j].FltTyp + IcsNode.IcsSet[i].IcsSns[j].FltTyp0;
        if (k > 0)
            break;
    }
    if (k > 0) {// during fault, do not do voltage calibration
//        printf("Can't CalibVolt - %d \n", k);
    	return;
    }

#ifndef PC_DEBUG
    //??? Call external function to get PT voltage
#endif

    if (1.732*IcsNode.VmPT > 1.15*IcsNode.Cfg->Vrated || IcsNode.VmPT < 0.85*IcsNode.Cfg->Vrated)
        IcsNode.VmPT = IcsNode.Cfg->Vrated/1.732;   // More likely bad PT voltage, use the rated volatge

    k = -1;
    vm = im = -1.0;
    for (i = 0; i < MAX_SETS; i++) {
        if (!IcsNode.IcsSet[i].Cfg->SrvFlg || IcsNode.IcsSet[i].Cfg->VrtFlg ||
            IcsNode.IcsSet[i].LineFlg != PHASE_ABC || IcsNode.IcsSet[i].LineFlg0 != PHASE_ABC)
            continue;

        w = (fmod(IcsNode.IcsSet[i].VmIcs[0].ang, 360.0) +
             fmod(IcsNode.IcsSet[i].VmIcs[1].ang + 120.0, 360.0) +
             fmod(IcsNode.IcsSet[i].VmIcs[2].ang+240.0, 360))/3.0;

        yi = yv = 1.0e6;
        for (j = 0; j < 3; j++) {
            IcsNode.IcsSet[i].IcsSns[j].Vmdcf = IcsNode.Vmref[j].mag/IcsNode.IcsSet[i].VmIcs[j].mag;
            IcsNode.IcsSet[i].IcsSns[j].Vmdcf *= IcsNode.VmPT;
            x = w + IcsNode.Vmref[j].ang - IcsNode.IcsSet[i].VmIcs[j].ang - (float)j*120.0;
            IcsNode.IcsSet[i].IcsSns[j].Vadcf = fmod(x, 360);

            yi = (yi < IcsNode.IcsSet[i].IcsSns[j].Iist.mag) ? yi : IcsNode.IcsSet[i].IcsSns[j].Iist.mag;
            yv = (yv < IcsNode.IcsSet[i].VmIcs[j].mag) ? yv : IcsNode.IcsSet[i].VmIcs[j].mag;
        }

        if ((k < 0 && yi < 1.0e5) || (im < 1.2*IcsNode.Cfg->Imin && yi > im) || (im >= 1.2*IcsNode.Cfg->Imin && yv > vm))
            k = i;

        im = (im > yi)? im : yi; // select the biggest
        vm = (vm < yv)? vm : yv; // select the smallest;

//#ifdef PC_DEBUG
        printf ("DynClib Set(%2d): %6.3f/%6.3f  %6.3f/%6.3f  %6.3f/%6.3f \n", i+1,
                    IcsNode.IcsSet[i].IcsSns[0].Vmdcf, IcsNode.IcsSet[i].IcsSns[0].Vadcf,
                    IcsNode.IcsSet[i].IcsSns[1].Vmdcf, IcsNode.IcsSet[i].IcsSns[1].Vadcf,
                    IcsNode.IcsSet[i].IcsSns[2].Vmdcf, IcsNode.IcsSet[i].IcsSns[2].Vadcf);
//#endif
    }

    IcsNode.VoltSetNum = (k >= 0) ? k : IcsNode.VoltSetNum;

    if (im < 1.2*IcsNode.Cfg->Imin || vm < 1.2*IcsNode.Cfg->Vmin)
        IcsNode.CtrlSusp = 1; // Suspend switch control due to too low current or voltage
    else
        IcsNode.CtrlSusp = 0;
}

void TFDIRContainer::genRawSamples() {//test only
    double i_angle = 0;
    double v_angle = M_PI*18.0/180.0;
    double p2p_angle = M_PI*120.0/180.0;
    double rate_angle = 2*M_PI/SV_CYC_SIZE;
    Ir = 0.5/CfgNode.CfgSet[0].Irated;
    Vr = 1.0/CfgNode.Vrated;

    for(int i=0; i<MAX_SETS-1; i++) {

        SmpNode.SmpSet[i].t.sec = 0;
        SmpNode.SmpSet[i].t.usec = 0;

        for (int j=0; j<MES_PHASES; j++) {

            SmpNode.SmpSet[i].NumI[j] = SV_CYC_SIZE;
            SmpNode.SmpSet[i].NumV[j] = SV_CYC_SIZE;
            SmpNode.SmpSet[i].Iflg[j] = 0;
            SmpNode.SmpSet[i].Vflg[j] = 0;

            for (int k=0; k<SV_CYC_SIZE; k++) {
                //set the new values
                SmpNode.SmpSet[i].I[j][k] = Ir*30000*cos(j*p2p_angle+i_angle+k*rate_angle); //per unit;
                SmpNode.SmpSet[i].V[j][k] = Vr*0.1*cos(j*p2p_angle+v_angle+k*rate_angle); //per unit
            }
        }
    }
}

void TFDIRContainer::genRawSamplesHardCoded() {//test only

    float testI[] = {-2.04766655,
                     -6.14366674,
                     -6.14366674,
                     -9.55700016,
                     0.682999969,
                     -10.9223328,
                     -0.68233335,
                     5.46166658,
                     9.55766677,
                     -8.1916666,
                     -4.09566641,
                     4.0963335,
                     -1.36500001,
                     -8.87433338,
                     5.46166658,
                     -4.09566641,
                     4.0963335,
                     10.2403336,
                     8.19233322,
                     -2.04766655,
                     1.36566663,
                     -8.87433338,
                     2.73099995,
                     -3.41299987,
                     -7.50899982,
                     10.2403336,
                     6.14433336,
                     -2.04766655,
                     3.41366673,
                     8.875,
                     -5.46099997,
                     6.14433336};

    float testV[] = {-0.000817391265,
                     -0.000747826066,
                     -0.000660869526,
                     -0.000504347787,
                     -0.000313043449,
                     -0.000104347819,
                     0.000104347819,
                     0.00032173912,
                     0.000504347787,
                     0.000660869526,
                     0.000782608695,
                     0.000869565178,
                     0.00091304339,
                     0.000939130433,
                     0.000921739091,
                     0.000895652105,
                     0.00083478255,
                     0.000756521709,
                     0.000652173883,
                     0.000547826057,
                     0.000373913033,
                     0.000121739125,
                     -0.0000869565192,
                     -0.000304347806,
                     -0.000486956502,
                     -0.000652173883,
                     -0.000756521709,
                     -0.000852173835,
                     -0.000886956462,
                     -0.00091304339,
                     -0.000895652105,
                     -0.000869565178};

    for(int i=0; i<MAX_SETS-1; i++) {

        SmpNode.SmpSet[i].t.sec = 0;
        SmpNode.SmpSet[i].t.usec = 0;

        for (int j=0; j<MES_PHASES; j++) {

            SmpNode.SmpSet[i].NumI[j] = SV_CYC_SIZE;
            SmpNode.SmpSet[i].NumV[j] = SV_CYC_SIZE;
            SmpNode.SmpSet[i].Iflg[j] = 0;
            SmpNode.SmpSet[i].Vflg[j] = 0;

            for (int k=0; k<SV_CYC_SIZE; k++) {

                //set the new values
                SmpNode.SmpSet[i].I[j][k] = testI[k]; //per unit
                SmpNode.SmpSet[i].V[j][k] = testV[k]; //per unit
            }
        }
    }
}

#ifdef RUNNING_TEST
int TFDIRContainer::GetSamples(int CallFlag)
{
    int i, j, k, m, n;
    int error = 0;
    char path[200];
    PKT pkt;
    unsigned char q[PKTperCYCLE];
    static FILE* cfile[3];
    static FILE* dfile[3];
    static SIGNAL_INFO sigList[3][NUM_SIGNALS];
    float Ir = 0.5/CfgNode.CfgSet[0].Irated;
    float Vr = 1.0/CfgNode.Vrated;
    char *SV_CFG_FILE[] = {
            "tap1-set1.cfg",
            "tap1-set2.cfg",
            "tap1-set3.cfg",
            NULL
    };
    char *SV_DAT_FILE[] = {
            "tap1-set1.dat",
            "tap1-set2.dat",
            "tap1-set3.dat",
            NULL
    };
    if (CallFlag == 0) {
        for (i = 0; i < MAX_SETS; i++) {
            if (!IcsNode.IcsSet[i].Cfg->SrvFlg || IcsNode.IcsSet[i].Cfg->VrtFlg)
                continue;
            strcpy(path, (char *)SV_CFG_PATH);
            strcat(path, SV_CFG_FILE[i]);
            cfile[i] = openCOMTRADE(path);
            strcpy(path, (char *)SV_CFG_PATH);
            strcat(path, SV_DAT_FILE[i]);
            dfile[i] = openCOMTRADE(path);
            if (!cfile[i] || !dfile[i]) {
                error = 1;
                return error;
            }
            signalList(cfile[i], sigList[i]);
        }
    }

// Get the Raw Sampled Values
    memset (&SmpNode, 0, sizeof(SMP_NODE));
    for (i = 0; i < MAX_SETS; i++) {
        if (!IcsNode.IcsSet[i].Cfg->SrvFlg || IcsNode.IcsSet[i].Cfg->VrtFlg)
            continue;
        memset (q, 0, sizeof(char)*PKTperCYCLE);
        m = 0;
        n = 0;
        for (j = 0; j < PKTperCYCLE; j++) {
            error = getPKT(dfile[i], sigList[i], &pkt);
            if (error) {
                fclose(dfile[i]);
                return error;
            }
            q[j] = pkt.dataQuality;
            if (j == 0) {
                SmpNode.SmpSet[i].t.sec  = pkt.timestamp_sec;
                SmpNode.SmpSet[i].t.usec = pkt.timestamp_usec;
            }
            if (pkt.dataQuality) continue;
            n++;
            for (k = 0; k < SAMPLESperPKT; k++) {
                SmpNode.SmpSet[i].I[0][m] = Ir*pkt.iA[k];
                SmpNode.SmpSet[i].I[1][m] = Ir*pkt.iB[k];
                SmpNode.SmpSet[i].I[2][m] = Ir*pkt.iC[k];
                SmpNode.SmpSet[i].V[0][m] = Vr*pkt.vA[k];
                SmpNode.SmpSet[i].V[1][m] = Vr*pkt.vB[k];
                SmpNode.SmpSet[i].V[2][m] = Vr*pkt.vC[k];
                m++;
            }
        }
        m = n;
// Check bad packages
        if (n < PKTperCYCLE) { // there are bad packages
            if(n < PKTperCYCLE/2) {// 1/2 or more packages are bad
                for (j = 0; j < 3; j++) {
                    SmpNode.SmpSet[i].Iflg[j] = DATA_BAD;
                    SmpNode.SmpSet[i].Vflg[j] = DATA_BAD;
                }
                m = 0;
            }
            else { // keep only 1/2 cycle continuous packages
                m = 0;
                for (j = PKTperCYCLE-1; j >= 0 ; j--) {
                    m++;
                    if (q[j] && m <= PKTperCYCLE/2) {
                        m = 0;
                        for (k = PKTperCYCLE*SAMPLESperPKT-1; k >= j*SAMPLESperPKT; k--) {
                            SmpNode.SmpSet[i].I[0][k] = 0.0;
                            SmpNode.SmpSet[i].I[1][k] = 0.0;
                            SmpNode.SmpSet[i].I[2][k] = 0.0;
                            SmpNode.SmpSet[i].V[0][k] = 0.0;
                            SmpNode.SmpSet[i].V[1][k] = 0.0;
                            SmpNode.SmpSet[i].V[2][k] = 0.0;
                        }
                    }
                    else if (m >= PKTperCYCLE/2) {
                        for (k = j*SAMPLESperPKT-1; k >= 0; k--) {
                            SmpNode.SmpSet[i].I[0][k] = 0.0;
                            SmpNode.SmpSet[i].I[1][k] = 0.0;
                            SmpNode.SmpSet[i].I[2][k] = 0.0;
                            SmpNode.SmpSet[i].V[0][k] = 0.0;
                            SmpNode.SmpSet[i].V[1][k] = 0.0;
                            SmpNode.SmpSet[i].V[2][k] = 0.0;
                        }
                        break;
                    }
                }
            }
        }
        m *= SAMPLESperPKT;
        for (k = 0; k < 3; k++) {
            SmpNode.SmpSet[i].NumI[k] = m;
            SmpNode.SmpSet[i].NumV[k] = m;
        }
    }
    return error;
}

int TFDIRContainer::GetSamplesReversed(int CallFlag)
{
    int i, j, k, m, n;
    int error = 0;
    char path[200];
    PKT pkt;
    unsigned char q[PKTperCYCLE];
    static FILE* cfile[3];
    static FILE* dfile[3];
    static SIGNAL_INFO sigList[3][NUM_SIGNALS];
    float Ir = 0.5/CfgNode.CfgSet[0].Irated;
    float Vr = 1.0/CfgNode.Vrated;
    char *SV_CFG_FILE[] = {
            "tap1-set1.cfg",
            "tap1-set2.cfg",
            "tap1-set3.cfg",
            NULL
    };
    char *SV_DAT_FILE[] = {
            "tap1-set1.dat",
            "tap1-set2.dat",
            "tap1-set3.dat",
            NULL
    };
    if (CallFlag == 0) {
        for (i = 0; i < MAX_SETS; i++) {
            if (!IcsNode.IcsSet[i].Cfg->SrvFlg || IcsNode.IcsSet[i].Cfg->VrtFlg)
                continue;
            strcpy(path, (char *)SV_CFG_PATH);
            strcat(path, SV_CFG_FILE[i]);
            cfile[i] = openCOMTRADE(path);
            strcpy(path, (char *)SV_CFG_PATH);
            strcat(path, SV_DAT_FILE[i]);
            dfile[i] = openCOMTRADE(path);
            if (!cfile[i] || !dfile[i]) {
                error = 1;
                return error;
            }
            signalList(cfile[i], sigList[i]);
        }
    }

// Get the Raw Sampled Values
    memset (&SmpNode, 0, sizeof(SMP_NODE));
    for (i = 0; i < MAX_SETS; i++) {
        if (!IcsNode.IcsSet[i].Cfg->SrvFlg || IcsNode.IcsSet[i].Cfg->VrtFlg)
            continue;
        memset (q, 0, sizeof(char)*PKTperCYCLE);
        m = 0;
        n = 0;
        for (j = 0; j < PKTperCYCLE; j++) {
            error = getPKT(dfile[i], sigList[i], &pkt);
            if (error) {
                fclose(dfile[i]);
                return error;
            }
            q[j] = pkt.dataQuality;
            if (j == 0) {
                SmpNode.SmpSet[i].t.sec  = pkt.timestamp_sec;
                SmpNode.SmpSet[i].t.usec = pkt.timestamp_usec;
            }
            if (pkt.dataQuality) continue;
            n++;
            for (k = SAMPLESperPKT-1; k >= 0 ; k--) {
                SmpNode.SmpSet[i].I[0][m] = Ir*pkt.iA[k];
                SmpNode.SmpSet[i].I[1][m] = Ir*pkt.iB[k];
                SmpNode.SmpSet[i].I[2][m] = Ir*pkt.iC[k];
                SmpNode.SmpSet[i].V[0][m] = Vr*pkt.vA[k];
                SmpNode.SmpSet[i].V[1][m] = Vr*pkt.vB[k];
                SmpNode.SmpSet[i].V[2][m] = Vr*pkt.vC[k];

//                printf("%f\n", SmpNode.SmpSet[i].I[0][m]);
                m++;
            }
        }
        m = n;
// Check bad packages
        if (n < PKTperCYCLE) { // there are bad packages
            if(n < PKTperCYCLE/2) {// 1/2 or more packages are bad
                for (j = 0; j < 3; j++) {
                    SmpNode.SmpSet[i].Iflg[j] = DATA_BAD;
                    SmpNode.SmpSet[i].Vflg[j] = DATA_BAD;
                }
                m = 0;
            }
            else { // keep only 1/2 cycle continuous packages
                m = 0;
                for (j = PKTperCYCLE-1; j >= 0 ; j--) {
                    m++;
                    if (q[j] && m <= PKTperCYCLE/2) {
                        m = 0;
                        for (k = PKTperCYCLE*SAMPLESperPKT-1; k >= j*SAMPLESperPKT; k--) {
                            SmpNode.SmpSet[i].I[0][k] = 0.0;
                            SmpNode.SmpSet[i].I[1][k] = 0.0;
                            SmpNode.SmpSet[i].I[2][k] = 0.0;
                            SmpNode.SmpSet[i].V[0][k] = 0.0;
                            SmpNode.SmpSet[i].V[1][k] = 0.0;
                            SmpNode.SmpSet[i].V[2][k] = 0.0;
                        }
                    }
                    else if (m >= PKTperCYCLE/2) {
                        for (k = j*SAMPLESperPKT-1; k >= 0; k--) {
                            SmpNode.SmpSet[i].I[0][k] = 0.0;
                            SmpNode.SmpSet[i].I[1][k] = 0.0;
                            SmpNode.SmpSet[i].I[2][k] = 0.0;
                            SmpNode.SmpSet[i].V[0][k] = 0.0;
                            SmpNode.SmpSet[i].V[1][k] = 0.0;
                            SmpNode.SmpSet[i].V[2][k] = 0.0;
                        }
                        break;
                    }
                }
            }
        }
        m *= SAMPLESperPKT;
        for (k = 0; k < 3; k++) {
            SmpNode.SmpSet[i].NumI[k] = m;
            SmpNode.SmpSet[i].NumV[k] = m;
        }
    }
    return error;
}
#endif

#ifndef RUNNING_TEST
void TFDIRContainer::addNewSampleSinglePhaseTimeClamped(DataStream& setDataStream, int set, int phase) {
    if (configLoaded) {
        //only perform sampling if configuration is actually loaded

        //validate the values are actually within possible range
//	        if (!validateData(setDataStream)) {
////	            printf("%d invalid data - %d,%d,%d,%d,%d", set, ILimit, VLimit, setDataStream.data.sample.currentNow, setDataStream.data.sample.current0_5msAgo, setDataStream.data.sample.voltageNow);
//	        	return;
//	        }

        bool isMissing = false;

        //get the previous time
//		double prevSmpSetTime = SmpNode.SmpSet[set].phase_t[phase].sec + (double)SmpNode.SmpSet[set].phase_t[phase].usec/1000000.0;
        double prevSmpSetTime = SmpNode.SmpSet[set].phase_st[phase] + (double)SmpNode.SmpSet[set].phase_t_ct[phase] * smpTimeResolution;
        double prevSampleSetTime = prevSample[0][set][phase].data.time.secs + (double)prevSample[0][set][phase].data.time.nsecs/1000000000.0;
        double curSampleTime = setDataStream.data.time.secs + (double)setDataStream.data.time.nsecs/1000000000.0;
        double curSmpTime = prevSmpSetTime;

        int smpAway = numSamplesAway(curSampleTime, prevSmpSetTime, smpTimeResolution);
//        int smpPhaseAway = numSamplesAway(curSampleTime, prevSmpTimePerPhase[set][phase], smpTimeResolution);


//	        if (!isInitialSample && (smpAway < -100 || smpAway > 100)) {
//	        	//timestamp is corrupt, ignore
//	        	return;
//	        }

        if (isInitialSample[set][phase]) {
            //very first time, just set the idx to 0
            smpAway = 0;
//			SmpNode.SmpSet[set].phase_t[phase].sec = (long) curSampleTime;
//			SmpNode.SmpSet[set].phase_t[phase].usec = (long) ((curSampleTime - SmpNode.SmpSet[set].phase_t[phase].sec) * 1000000);
            SmpNode.SmpSet[set].phase_st[phase] = curSampleTime;
//            smpPhaseAway = 0;
            curSmpTime = curSampleTime;
        }

//	        if (executionCt<1 && set == 0 && phase == 0)
//	        	printf("Add new sample, %lf, %d, %lf, %d, %d, %d, %d", curSampleTime, smpAway, curSmpTime, setsPhasesTotalSampleCt, setDataStream.data.sample.current0_5msAgo,
//	        		setDataStream.data.sample.currentNow, setDataStream.data.sample.voltageNow);

        if(smpAway >= 0) {

            //this means it's a newer sample, which will require shifting of the existing values down.
            curSmpTime += smpAway * smpTimeResolution;

//			SmpNode.SmpSet[set].phase_t[phase].sec = (long) curSmpTime;
//			SmpNode.SmpSet[set].phase_t[phase].usec = (long) ((curSmpTime - SmpNode.SmpSet[set].phase_t[phase].sec) * 1000000);
            if (smpAway == 1)
                SmpNode.SmpSet[set].phase_t_ct[phase]++;

            if (smpAway > 6) {
                //missed too many samples, reset to 0s
//				printf("missed too many samples!!!, %d, %d, %d, %lf, %lf, %lf, %d", set, phase, executionCt, curSampleTime, prevSmpSetTime, curSmpTime, smpAway);

//				memset(&SmpNode.SmpSet[set].I[phase], 0, SV_CYC_SIZE*floatSize);
//				memset(&SmpNode.SmpSet[set].V[phase], 0, SV_CYC_SIZE*floatSize);
//				for (int i = 0; i < MES_PHASES; i++) {
//					SmpNode.SmpSet[set].Iflg[phase] = DATA_BAD;
//					SmpNode.SmpSet[set].Vflg[phase] = DATA_BAD;
//					SmpNode.SmpSet[set].NumI[phase] = 0;
//					SmpNode.SmpSet[set].NumV[phase] = 0;

//					setsPhasesTotalSampleCt[set][phase] = 0;
//				}

                //set the interpolated data
//				interpolatedData.currentNow = setDataStream.data.sample.currentNow;
//				interpolatedData.current0_5msAgo = setDataStream.data.sample.current0_5msAgo;
//				interpolatedData.voltageNow = setDataStream.data.sample.voltageNow;
//				interpolatedData.voltage0_5msAgo = 0;

                //fill in the missing samples wih 0s
                for (int j=0; j< smpAway; j++) {
                    setSmpSetValues(set, phase, 0, 0.0, 0.0, true);
                    setSmpSetValues(set, phase, 1, 0.0, 0.0, true);
                    checkCycleComplete(2, set, phase);
                }

                isMissing = true;

            } else {
                if (smpAway >= 1) {
//					if(smpAway > 1 && executionCt>=2 && executionCt<=10 && set==0 && phase==0)
//						printf("Has missing!!!");

                    //shift the existing samples down and prefill values only when the actual SmpNode samples are at least 1 sample away
                    for (int j = smpAway-1; j >= 0; j--) {
                        memmove(&SmpNode.SmpSet[set].I[phase][2], &SmpNode.SmpSet[set].I[phase][0],
                                (SV_CYC_SIZE - 2) * floatSize);
                        memmove(&SmpNode.SmpSet[set].V[phase][2], &SmpNode.SmpSet[set].V[phase][0],
                                (SV_CYC_SIZE - 2) * floatSize);

                        if (smpAway > 1) {
                            //definitely has missing samples
                            //prefill with interpolated values so they are always valid incase the new sample in the future doesn't arrive

                            double tempSmpTime = curSmpTime - smpTimeResolution * j;
//							SmpNode.SmpSet[set].phase_t[phase].sec = (int) tempSmpTime;
//							SmpNode.SmpSet[set].phase_t[phase].usec = (int) ((tempSmpTime - SmpNode.SmpSet[set].phase_t[phase].sec) * 1000000);
                            SmpNode.SmpSet[set].phase_t_ct[phase]++;

                            if (j>0) {
                                //fill in the actually missing values

                                if (IcsNode.IcsSet[set].IcsSns[phase].Ibuf[0].flg == DATA_NORMAL && IcsNode.IcsSet[set].IcsSns[phase].Vbuf[0].flg == DATA_NORMAL) {
                                    // need to fill in the missed samples using the last know phasors
                                    if (j==smpAway-1) {
                                        //only need to do this the first time per set or when phasor is recalculated
                                        //								iPhasor[phase] = XY2PhasorI(&SmpNode.SmpSet[set].IRefPhasor[phase], set);
                                        //								vPhasor[phase] = XY2PhasorV(&SmpNode.SmpSet[set].VRefPhasor[phase], set, phase);
                                        iPhasor[phase] = SmpNode.SmpSet[set].IRefPhasor[phase];
                                        vPhasor[phase] = SmpNode.SmpSet[set].VRefPhasor[phase];
                                        iPhasor[phase].ang *= DEGREES2PI;
                                        vPhasor[phase].ang *= DEGREES2PI;
                                        iPhasor_refTime[phase] = iPhasor[phase].t.sec + (double) iPhasor[phase].t.usec / 1000000.0;
                                        vPhasor_refTime[phase] = vPhasor[phase].t.sec + (double) vPhasor[phase].t.usec / 1000000.0;
                                    }

                                    double missedI, missedV;

                                    //sample with phasor calc, I/V=Mag*(2*pi*f*t + Ang)
                                    missedI = iPhasor[phase].mag * sin(2.0 * M_PI * CfgNode.SysFreq * (tempSmpTime - iPhasor_refTime[phase]) + iPhasor[phase].ang);
                                    missedV = vPhasor[phase].mag * sin(2.0 * M_PI * CfgNode.SysFreq * (tempSmpTime - vPhasor_refTime[phase]) + vPhasor[phase].ang);
                                    setSmpSetValues(set, phase, 0, missedI, missedV, true);

                                    missedI = iPhasor[phase].mag * sin(2.0 * M_PI * CfgNode.SysFreq * (tempSmpTime - smpTimeResolution/2 - iPhasor_refTime[phase]) + iPhasor[phase].ang);
                                    missedV = vPhasor[phase].mag * sin(2.0 * M_PI * CfgNode.SysFreq * (tempSmpTime - smpTimeResolution/2 - vPhasor_refTime[phase]) + vPhasor[phase].ang);
                                    setSmpSetValues(set, phase, 1, missedI, missedV, true);

                                } else {
                                    //if no valid phasor exists then use linear interpolation to fill in the missing values
                                    interpolateData(setDataStream.data.sample, prevSample[0][set][phase].data.sample,
                                                    prevSample[1][set][phase], curSampleTime, tempSmpTime, prevSampleSetTime,
                                                    interpolatedData);
                                    setSmpSetValues(set, phase, 0, interpolatedData.currentNow, interpolatedData.voltageNow, true);
                                    setSmpSetValues(set, phase, 1, interpolatedData.current0_5msAgo, interpolatedData.voltage0_5msAgo, true);
                                }

//								if (executionCt>=2 && executionCt<=10 && set==0 && phase==0) {
//	//	                                	printf("I phasor, %f, %f, %lf", iPhasor[i].mag, iPhasor[i].ang, iPhasor_refTime[i]);
//
//									printf("DSP: missed sample, %d, %d-%d, %d, origTime=%lf, time=%lf, %f,%f", sampleSeq, set, phase, setsPhasesTotalSampleCt[set][phase],
//										   curSampleTime, tempSmpTime, SmpNode.SmpSet[set].I[phase][1],
//										   SmpNode.SmpSet[set].V[phase][1]);
//									printf("DSP: missed sample, %d, %d-%d, %d, origTime=%lf, time=%lf, %f,%f", sampleSeq, set, phase, setsPhasesTotalSampleCt[set][phase],
//										   curSampleTime, tempSmpTime, SmpNode.SmpSet[set].I[phase][0],
//										   SmpNode.SmpSet[set].V[phase][0]);
//								}
                            } else if (j==0) {
                                //set the prev samples only when it's missing samples since we set them here
                                prevSample[1][set][phase].data.time.secs = (int) (curSmpTime - smpTimeResolution*3);
                                prevSample[1][set][phase].data.time.nsecs =  (int) ((curSmpTime - smpTimeResolution*3 - prevSample[1][set][phase].data.time.secs)*1000000000);
                                prevSample[1][set][phase].data.sample.currentNow = (int) (Ir*SmpNode.SmpSet[set].I[phase][4]);
                                prevSample[1][set][phase].data.sample.current0_5msAgo = (int) (Ir*SmpNode.SmpSet[set].I[phase][5]);
                                prevSample[1][set][phase].data.sample.voltageNow = (int) (Vr*SmpNode.SmpSet[set].V[phase][4]);

                                prevSample[0][set][phase].data.time.secs = (int) (curSmpTime - smpTimeResolution);
                                prevSample[0][set][phase].data.time.nsecs =  (int) ((curSmpTime - smpTimeResolution - prevSample[0][set][phase].data.time.secs)*1000000000);
                                prevSample[0][set][phase].data.sample.currentNow = (int) (Ir*SmpNode.SmpSet[set].I[phase][2]);
                                prevSample[0][set][phase].data.sample.current0_5msAgo = (int) (Ir*SmpNode.SmpSet[set].I[phase][3]);
                                prevSample[0][set][phase].data.sample.voltageNow = (int) (Vr*SmpNode.SmpSet[set].V[phase][2]);

                                //interpolate the cur new data against the most recent filled in data for actually missing data
                                prevSmpSetTime = curSmpTime - smpTimeResolution;

                                interpolateData(setDataStream.data.sample, prevSample[0][set][phase].data.sample,
                                                prevSample[1][set][phase], curSampleTime, curSmpTime, prevSmpSetTime,
                                                interpolatedData);

                                setSmpSetValues(set, phase, 0, interpolatedData.currentNow, interpolatedData.voltageNow, true);
                                setSmpSetValues(set, phase, 1, interpolatedData.current0_5msAgo, interpolatedData.voltage0_5msAgo, true);

//								if (executionCt>=2 && executionCt<=10 && set==0 && phase==0) {
////	                                    	printf("On last missed, %d, %d, %d, %lf, %lf, %lf", setDataStream.data.sample.voltageNow, prevSample[0][set][phase].data.sample.voltageNow,
////	                                    	                                                prevSample[1][set][phase].data.sample.voltageNow, curSampleTime, curSmpTime, prevSmpSetTime);
//
//									printf("DSP: missed last Sample, %d, %d-%d, %d, origTime=%lf, time=%lf, %f,%f", sampleSeq, set, phase, setsPhasesTotalSampleCt[set][phase],
//										   curSampleTime, curSmpTime, SmpNode.SmpSet[set].I[phase][1],
//										   SmpNode.SmpSet[set].V[phase][1]);
//									printf("DSP: missed last Sample, %d, %d-%d, %d, origTime=%lf, time=%lf, %f,%f", sampleSeq, set, phase, setsPhasesTotalSampleCt[set][phase],
//										   curSampleTime, curSmpTime, SmpNode.SmpSet[set].I[phase][0],
//										   SmpNode.SmpSet[set].V[phase][0]);
//								}

                                isMissing = true;
                            }

                            //check the missing samples
                            checkCycleComplete(2, set, phase);
                        }
                    }
                }
            }

            if (!isMissing && smpAway <= 1) {
                //interpolate between the prev and the newest sample
                if (prevSample[0][set][phase].isCRCgood)
                    interpolateData(setDataStream.data.sample, prevSample[0][set][phase].data.sample, prevSample[1][set][phase], curSampleTime, curSmpTime, prevSampleSetTime, interpolatedData);
                else {//this should only happen on the first sample per set per phase since the prevSample is none existent
                    //set the interpolated data
                    interpolatedData.currentNow = setDataStream.data.sample.currentNow;
                    interpolatedData.current0_5msAgo = setDataStream.data.sample.current0_5msAgo;
                    interpolatedData.voltageNow = setDataStream.data.sample.voltageNow;
                    interpolatedData.voltage0_5msAgo = setDataStream.data.sample.voltageNow;

                    isInitialSample[set][phase] = true;
                }

            }

            //set the cur samples to the prevSample
            memmove(&prevSample[1][set][phase], &prevSample[0][set][phase], DataStreamSize);
            memcpy(&prevSample[0][set][phase], &setDataStream, DataStreamSize);

            if (!isMissing && (prevSampleSetTime < curSmpTime || isInitialSample[set][phase])) {
                isInitialSample[set][phase] = false;

                //only set the resampled when the last sample was before the desired time and obviously the new sample is after the desired sample time.  Also set it if it's the first sample
                setSmpSetValues(set, phase, 0, interpolatedData.currentNow, interpolatedData.voltageNow, true);
                setSmpSetValues(set, phase, 1, interpolatedData.current0_5msAgo, interpolatedData.voltage0_5msAgo, true);

//				if (executionCt>=2 && executionCt<=10 && set==0 && phase == 0) {
//					printf("DSP: new Sample, %d, %d-%d, %d, origTime=%lf, time=%lf, %f,%f", sampleSeq, set, phase, setsPhasesTotalSampleCt[set][phase], curSampleTime,
//						   curSmpTime,
//						   SmpNode.SmpSet[set].I[phase][1], SmpNode.SmpSet[set].V[phase][1]);
//					printf("DSP: new Sample, %d, %d-%d, %d, origTime=%lf, time=%lf, %f,%f", sampleSeq, set, phase, setsPhasesTotalSampleCt[set][phase], curSampleTime,
//						   curSmpTime,
//						   SmpNode.SmpSet[set].I[phase][0], SmpNode.SmpSet[set].V[phase][0]);
//				}
                //this only added 2 samples
                checkCycleComplete(2, set, phase);
            }
        }
    }
}


void TFDIRContainer::addNewSampleSinglePhase(DataStream& setDataStream, int set, int phase) {
    if (configLoaded) {
        //only perform sampling if configuration is actually loaded

        //validate the values are actually within possible range
//	        if (!validateData(setDataStream)) {
////	            printf("%d invalid data - %d,%d,%d,%d,%d", set, ILimit, VLimit, setDataStream.data.sample.currentNow, setDataStream.data.sample.current0_5msAgo, setDataStream.data.sample.voltageNow);
//	        	return;
//	        }

        bool isMissing = false;

        //get the previous time
        double prevSmpSetTime = SmpNode.SmpSet[set].phase_t[phase].sec + (double)SmpNode.SmpSet[set].phase_t[phase].usec/1000000.0;
        double prevSampleSetTime = prevSample[0][set][phase].data.time.secs + (double)prevSample[0][set][phase].data.time.nsecs/1000000000.0;
        double curSampleTime = setDataStream.data.time.secs + (double)setDataStream.data.time.nsecs/1000000000.0;
        double curSmpTime = prevSmpSetTime;

        int smpAway = numSamplesAway(curSampleTime, prevSmpSetTime, smpTimeResolution);
//        int smpPhaseAway = numSamplesAway(curSampleTime, prevSmpTimePerPhase[set][phase], smpTimeResolution);


//	        if (!isInitialSample && (smpAway < -100 || smpAway > 100)) {
//	        	//timestamp is corrupt, ignore
//	        	return;
//	        }

        if (isInitialSample[set][phase]) {
            //very first time, just set the idx to 0
            smpAway = 0;
//            smpPhaseAway = 0;
            curSmpTime = curSampleTime;
        }

//	        if (executionCt<1 && set == 0 && phase == 0)
//	        	printf("Add new sample, %lf, %d, %lf, %d, %d, %d, %d", curSampleTime, smpAway, curSmpTime, setsPhasesTotalSampleCt, setDataStream.data.sample.current0_5msAgo,
//	        		setDataStream.data.sample.currentNow, setDataStream.data.sample.voltageNow);

        if(smpAway >= 0) {

            //this means it's a newer sample, which will require shifting of the existing values down.
            curSmpTime += smpAway * smpTimeResolution;

            SmpNode.SmpSet[set].phase_t[phase].sec = (long) curSmpTime;
            SmpNode.SmpSet[set].phase_t[phase].usec = (long) ((curSmpTime - SmpNode.SmpSet[set].phase_t[phase].sec) * 1000000);

            if (smpAway > 6) {
                //missed too many samples, reset to 0s
                printf("missed too many samples!!!, %d, %d, %d, %lf, %lf, %lf, %d", set, phase, executionCt, curSampleTime, prevSmpSetTime, curSmpTime, smpAway);

                memset(&SmpNode.SmpSet[set].I[phase], 0, SV_CYC_SIZE*floatSize);
                memset(&SmpNode.SmpSet[set].V[phase], 0, SV_CYC_SIZE*floatSize);
//				for (int i = 0; i < MES_PHASES; i++) {
//					SmpNode.SmpSet[set].Iflg[phase] = DATA_BAD;
//					SmpNode.SmpSet[set].Vflg[phase] = DATA_BAD;
                SmpNode.SmpSet[set].NumI[phase] = 0;
                SmpNode.SmpSet[set].NumV[phase] = 0;

                setsPhasesTotalSampleCt[set][phase] = 0;
//				}

                //set the interpolated data
                interpolatedData.currentNow = setDataStream.data.sample.currentNow;
                interpolatedData.current0_5msAgo = setDataStream.data.sample.current0_5msAgo;
                interpolatedData.voltageNow = setDataStream.data.sample.voltageNow;
                interpolatedData.voltage0_5msAgo = 0;

            } else {
                if (smpAway >= 1) {
                    //shift the existing samples down and prefill values only when the actual SmpNode samples are at least 1 sample away
                    for (int j = smpAway-1; j >= 0; j--) {
                        memmove(&SmpNode.SmpSet[set].I[phase][2], &SmpNode.SmpSet[set].I[phase][0],
                                (SV_CYC_SIZE - 2) * floatSize);
                        memmove(&SmpNode.SmpSet[set].V[phase][2], &SmpNode.SmpSet[set].V[phase][0],
                                (SV_CYC_SIZE - 2) * floatSize);

                        if (smpAway > 1) {
                            //definitely has missing samples
                            //prefill with interpolated values so they are always valid incase the new sample in the future doesn't arrive

                            double tempSmpTime = curSmpTime - smpTimeResolution * j;
                            SmpNode.SmpSet[set].phase_t[phase].sec = (int) tempSmpTime;
                            SmpNode.SmpSet[set].phase_t[phase].usec = (int) ((tempSmpTime - SmpNode.SmpSet[set].phase_t[phase].sec) * 1000000);

                            if (IcsNode.IcsSet[set].IcsSns[phase].Ibuf[0].flg == DATA_NORMAL && IcsNode.IcsSet[set].IcsSns[phase].Vbuf[0].flg == DATA_NORMAL) {
                                // need to fill in the missed samples using the last know phasors
                                if (j==smpAway-1) {
                                    //only need to do this the first time per set or when phasor is recalculated
                                    //								iPhasor[phase] = XY2PhasorI(&SmpNode.SmpSet[set].IRefPhasor[phase], set);
                                    //								vPhasor[phase] = XY2PhasorV(&SmpNode.SmpSet[set].VRefPhasor[phase], set, phase);
                                    iPhasor[phase] = SmpNode.SmpSet[set].IRefPhasor[phase];
                                    vPhasor[phase] = SmpNode.SmpSet[set].VRefPhasor[phase];
                                    iPhasor[phase].ang *= DEGREES2PI;
                                    vPhasor[phase].ang *= DEGREES2PI;
                                    iPhasor_refTime[phase] = iPhasor[phase].t.sec + (double) iPhasor[phase].t.usec / 1000000.0;
                                    vPhasor_refTime[phase] = vPhasor[phase].t.sec + (double) vPhasor[phase].t.usec / 1000000.0;
                                }

                                double missedI, missedV;

                                //sample with phasor calc, I/V=Mag*(2*pi*f*t + Ang)
                                missedI = iPhasor[phase].mag * sin(2.0 * M_PI * CfgNode.SysFreq * (tempSmpTime - iPhasor_refTime[phase]) + iPhasor[phase].ang);
                                missedV = vPhasor[phase].mag * sin(2.0 * M_PI * CfgNode.SysFreq * (tempSmpTime - vPhasor_refTime[phase]) + vPhasor[phase].ang);
                                setSmpSetValues(set, phase, 0, missedI, missedV, true);

                                missedI = iPhasor[phase].mag * sin(2.0 * M_PI * CfgNode.SysFreq * (tempSmpTime - smpTimeResolution/2 - iPhasor_refTime[phase]) + iPhasor[phase].ang);
                                missedV = vPhasor[phase].mag * sin(2.0 * M_PI * CfgNode.SysFreq * (tempSmpTime - smpTimeResolution/2 - vPhasor_refTime[phase]) + vPhasor[phase].ang);
                                setSmpSetValues(set, phase, 1, missedI, missedV, true);

                            } else {
                                //if no valid phasor exists then use linear interpolation to fill in the missing values
                                interpolateData(setDataStream.data.sample, prevSample[0][set][phase].data.sample,
                                                prevSample[1][set][phase], curSampleTime, tempSmpTime, prevSampleSetTime,
                                                interpolatedData);
                                setSmpSetValues(set, phase, 0, interpolatedData.currentNow, interpolatedData.voltageNow, true);
                                setSmpSetValues(set, phase, 1, interpolatedData.current0_5msAgo, interpolatedData.voltage0_5msAgo, true);
                            }

//							if (smpAway > 1 && j>0) {// && temp_set==0 && i==0) {
////	                                	printf("I phasor, %f, %f, %lf", iPhasor[i].mag, iPhasor[i].ang, iPhasor_refTime[i]);
//
//								printf("DSP: missed sample, %d, %d-%d, %d, origTime=%lf, time=%lf, %f,%f", sampleSeq, set, phase, setsPhasesTotalSampleCt[set][phase],
//									   curSampleTime, tempSmpTime, SmpNode.SmpSet[set].I[phase][1],
//									   SmpNode.SmpSet[set].V[phase][1]);
//								printf("DSP: missed sample, %d, %d-%d, %d, origTime=%lf, time=%lf, %f,%f", sampleSeq, set, phase, setsPhasesTotalSampleCt[set][phase],
//									   curSampleTime, tempSmpTime, SmpNode.SmpSet[set].I[phase][0],
//									   SmpNode.SmpSet[set].V[phase][0]);
//							}

                            if (j==0) {
                                //set the prev samples only when it's missing samples since we set them here
                                prevSample[1][set][phase].data.time.secs = (int) (curSmpTime - smpTimeResolution*3);
                                prevSample[1][set][phase].data.time.nsecs =  (int) ((curSmpTime - smpTimeResolution*3 - prevSample[1][set][phase].data.time.secs)*1000000000);
                                prevSample[1][set][phase].data.sample.currentNow = (int) (Ir*SmpNode.SmpSet[set].I[phase][4]);
                                prevSample[1][set][phase].data.sample.current0_5msAgo = (int) (Ir*SmpNode.SmpSet[set].I[phase][5]);
                                prevSample[1][set][phase].data.sample.voltageNow = (int) (Vr*SmpNode.SmpSet[set].V[phase][4]);

                                prevSample[0][set][phase].data.time.secs = (int) (curSmpTime - smpTimeResolution);
                                prevSample[0][set][phase].data.time.nsecs =  (int) ((curSmpTime - smpTimeResolution - prevSample[0][set][phase].data.time.secs)*1000000000);
                                prevSample[0][set][phase].data.sample.currentNow = (int) (Ir*SmpNode.SmpSet[set].I[phase][2]);
                                prevSample[0][set][phase].data.sample.current0_5msAgo = (int) (Ir*SmpNode.SmpSet[set].I[phase][3]);
                                prevSample[0][set][phase].data.sample.voltageNow = (int) (Vr*SmpNode.SmpSet[set].V[phase][2]);

                                //interpolate the cur new data against the most recent filled in data for actually missing data
                                prevSmpSetTime = curSmpTime - smpTimeResolution;

                                interpolateData(setDataStream.data.sample, prevSample[0][set][phase].data.sample,
                                                prevSample[1][set][phase], curSampleTime, curSmpTime, prevSmpSetTime,
                                                interpolatedData);

                                setSmpSetValues(set, phase, 0, interpolatedData.currentNow, interpolatedData.voltageNow, false);
                                setSmpSetValues(set, phase, 1, interpolatedData.current0_5msAgo, interpolatedData.voltage0_5msAgo, false);

                                //	                                    if (executionCt>=2 && executionCt<=6){// && set==0 && phase==0) {
                                ////	                                    	printf("On last missed, %d, %d, %d, %lf, %lf, %lf", setDataStream.data.sample.voltageNow, prevSample[0][set][phase].data.sample.voltageNow,
                                ////	                                    	                                                prevSample[1][set][phase].data.sample.voltageNow, curSampleTime, curSmpTime, prevSmpSetTime);
                                //
                                //											printf("DSP: missed last Sample, %d, %d-%d, %d, origTime=%lf, time=%lf, %f,%f", sampleSeq, set, phase, setsPhasesTotalSampleCt[set][phase],
                                //												   curSampleTime, curSmpTime, SmpNode.SmpSet[set].I[phase][1],
                                //												   SmpNode.SmpSet[set].V[phase][1]);
                                //											printf("DSP: missed last Sample, %d, %d-%d, %d, origTime=%lf, time=%lf, %f,%f", sampleSeq, set, phase, setsPhasesTotalSampleCt[set][phase],
                                //												   curSampleTime, curSmpTime, SmpNode.SmpSet[set].I[phase][0],
                                //												   SmpNode.SmpSet[set].V[phase][0]);
                                //	                                    }

                                isMissing = true;
                            }

                            //check the missing samples
                            checkCycleComplete(2, set, phase);
                        }
                    }
                }
            }

            if (!isMissing && smpAway <= 1) {
                //interpolate between the prev and the newest sample
                if (prevSample[0][set][phase].isCRCgood)
                    interpolateData(setDataStream.data.sample, prevSample[0][set][phase].data.sample, prevSample[1][set][phase], curSampleTime, curSmpTime, prevSampleSetTime, interpolatedData);
                else {//this should only happen on the first sample per set per phase since the prevSample is none existent
                    //set the interpolated data
                    interpolatedData.currentNow = setDataStream.data.sample.currentNow;
                    interpolatedData.current0_5msAgo = setDataStream.data.sample.current0_5msAgo;
                    interpolatedData.voltageNow = setDataStream.data.sample.voltageNow;
                    interpolatedData.voltage0_5msAgo = setDataStream.data.sample.voltageNow;

                    isInitialSample[set][phase] = true;
                }

            }

            //set the cur samples to the prevSample
            memmove(&prevSample[1][set][phase], &prevSample[0][set][phase], DataStreamSize);
            memcpy(&prevSample[0][set][phase], &setDataStream, DataStreamSize);

            if (!isMissing && (prevSampleSetTime < curSmpTime || isInitialSample[set][phase])) {
                isInitialSample[set][phase] = false;

                //only set the resampled when the last sample was before the desired time and obviously the new sample is after the desired sample time.  Also set it if it's the first sample
                setSmpSetValues(set, phase, 0, interpolatedData.currentNow, interpolatedData.voltageNow, true);
                setSmpSetValues(set, phase, 1, interpolatedData.current0_5msAgo, interpolatedData.voltage0_5msAgo, true);

//	                if (executionCt>=2 && executionCt<=6) { //&& set==0 && phase == 0) {
//						printf("DSP: new Sample, %d, %d-%d, %d, origTime=%lf, time=%lf, %f,%f", sampleSeq, set, phase, setsPhasesTotalSampleCt[set][phase], curSampleTime,
//							   curSmpTime,
//							   SmpNode.SmpSet[set].I[phase][1], SmpNode.SmpSet[set].V[phase][1]);
//						printf("DSP: new Sample, %d, %d-%d, %d, origTime=%lf, time=%lf, %f,%f", sampleSeq, set, phase, setsPhasesTotalSampleCt[set][phase], curSampleTime,
//							   curSmpTime,
//							   SmpNode.SmpSet[set].I[phase][0], SmpNode.SmpSet[set].V[phase][0]);
//	                }
                //this only added 2 samples
                checkCycleComplete(2, set, phase);
            }
        }
    }
}

//void TFDIRContainer::addNewSample(DataStream& setDataStream, int set, int phase) {
//	//testing single phase
//	if (set!=0 || phase!=0)
//		return;
//
//	if (configLoaded) {
//		//only perform sampling if configuration is actually loaded
//
//		//validate the values are actually within possible range
////	        if (!validateData(setDataStream)) {
//////	            printf("%d invalid data - %d,%d,%d,%d,%d", set, ILimit, VLimit, setDataStream.data.sample.currentNow, setDataStream.data.sample.current0_5msAgo, setDataStream.data.sample.voltageNow);
////	        	return;
////	        }
//
////	        //covert id to phase designation
////	        int phase = setDataStream.data.id/2;
////	        if (phase > MES_PHASES-1)
////	            phase = phase-MES_PHASES;
//
//		bool isMissing = false;
//
//		//get the previous time
//		double prevSmpSetTime = SmpNode.SmpSet[set].t.sec + (double)SmpNode.SmpSet[set].t.usec/1000000.0;
//		double prevSampleSetTime = prevSample[0][set][phase].data.time.secs + (double)prevSample[0][set][phase].data.time.nsecs/1000000000.0;
//		double curSampleTime = setDataStream.data.time.secs + (double)setDataStream.data.time.nsecs/1000000000.0;
//		double curSmpTime = prevSmpSetTime;
//
//		int smpAway = numSamplesAway(curSampleTime, prevSmpSetTime, smpTimeResolution);
////        int smpPhaseAway = numSamplesAway(curSampleTime, prevSmpTimePerPhase[set][phase], smpTimeResolution);
//
//
////	        if (!isInitialSample && (smpAway < -100 || smpAway > 100)) {
////	        	//timestamp is corrupt, ignore
////	        	return;
////	        }
//
//		if (isInitialSample) {
//			//very first time, just set the idx to 0
//			smpAway = 0;
////            smpPhaseAway = 0;
//			curSmpTime = curSampleTime;
//		}
//
////	        if (smpAway > 0 && prevSmpAway > 0) {
////				//2 Consecutive samples where the time keeps moving, which means the other phases were skipped for some reason.
////				//this means we have to force a cycle check based on the previously filled in data
////				//previous it only added to 1 phase of 1 set, so now must add the other remaining smps for all the sets and phases
////
////	        	printf("Time Moving!!!");
////				checkCycleComplete(2*(IcsNode.NumMesSets*MES_PHASES-1));
////			}
//
//		prevSmpAway = smpAway;
//
////	        if (executionCt<1 && set == 0 && phase == 0)
////	        	printf("Add new sample, %lf, %d, %lf, %d, %d, %d, %d", curSampleTime, smpAway, curSmpTime, setsPhasesTotalSampleCt, setDataStream.data.sample.current0_5msAgo,
////	        		setDataStream.data.sample.currentNow, setDataStream.data.sample.voltageNow);
//
//		if(smpAway >= 0) {
//
//			//this means it's a newer sample, which will require shifting of the existing values down.
//			curSmpTime += smpAway * smpTimeResolution;
//
//			//set the time for all sets
//			for (int temp_set=0; temp_set<IcsNode.NumMesSets; temp_set++) {
//				SmpNode.SmpSet[temp_set].t.sec = (long) curSmpTime;
//				SmpNode.SmpSet[temp_set].t.usec = (long) ((curSmpTime - SmpNode.SmpSet[temp_set].t.sec) * 1000000);
//
//				if (smpAway > 6) {
//					//missed too many samples, reset to 0s
//					printf("missed too many samples!!!, %d, %d, %lf, %lf, %lf, %d", set, phase, curSampleTime, prevSmpSetTime, curSmpTime, smpAway);
//
//					memset(&SmpNode.SmpSet[temp_set].I, 0, MES_PHASES*SV_CYC_SIZE*XY_Size);
//					memset(&SmpNode.SmpSet[temp_set].V, 0, MES_PHASES*SV_CYC_SIZE*XY_Size);
//					for (int i = 0; i < MES_PHASES; i++) {
//						SmpNode.SmpSet[temp_set].Iflg[i] = DATA_BAD;
//						SmpNode.SmpSet[temp_set].Vflg[i] = DATA_BAD;
//						SmpNode.SmpSet[temp_set].NumI[i] = 0;
//						SmpNode.SmpSet[temp_set].NumV[i] = 0;
//					}
//
//					//set the interpolated data
//					interpolatedData.currentNow = setDataStream.data.sample.currentNow;
//					interpolatedData.current0_5msAgo = 0;
//					interpolatedData.voltageNow = setDataStream.data.sample.voltageNow;
//					interpolatedData.voltage0_5msAgo = 0;
//
//				} else {
//					if (smpAway >= 1) {
//						//shift the existing samples down and prefill values only when the actual SmpNode samples are at least 1 sample away
//						for (int j = smpAway-1; j >= 0; j--) {
//							double tempSmpTime = curSmpTime - smpTimeResolution * j;
//							SmpNode.SmpSet[temp_set].t.sec = (int) tempSmpTime;
//							SmpNode.SmpSet[temp_set].t.usec = (int) ((tempSmpTime - SmpNode.SmpSet[temp_set].t.sec) * 1000000);
//
//							for (int i=0; i<MES_PHASES; i++) {
//
//								memmove(&SmpNode.SmpSet[temp_set].I[i][2], &SmpNode.SmpSet[temp_set].I[i][0],
//										(SV_CYC_SIZE - 2) * floatSize);
//								memmove(&SmpNode.SmpSet[temp_set].V[i][2], &SmpNode.SmpSet[temp_set].V[i][0],
//										(SV_CYC_SIZE - 2) * floatSize);
//
//								//prefill with interpolated values so they are always valid incase the new sample in the future doesn't arrive
//								if (IcsNode.IcsSet[temp_set].IcsSns[i].Ibuf[0].flg == DATA_NORMAL && IcsNode.IcsSet[temp_set].IcsSns[i].Vbuf[0].flg == DATA_NORMAL) {
//									// need to fill in the missed samples using the last know phasors
//									if (j==smpAway-1 || setsPhasesTotalSampleCt==0) {
//										//only need to do this the first time per set or when phasor is recalculated
//										iPhasor[i] = XY2PhasorI(&IcsNode.IcsSet[temp_set].IcsSns[i].Ibuf[0], temp_set);
//										vPhasor[i] = XY2PhasorV(&IcsNode.IcsSet[temp_set].IcsSns[i].Vbuf[0], temp_set, i);
//										iPhasor[i].ang *= DEGREES2PI;
//										vPhasor[i].ang *= DEGREES2PI;
//										iPhasor_refTime[i] = iPhasor[i].t.sec + (double) iPhasor[i].t.usec / 1000000.0;
//										vPhasor_refTime[i] = vPhasor[i].t.sec + (double) vPhasor[i].t.usec / 1000000.0;
//
//	//	                                    if (temp_set==0 && i==0)
//	//	                                    	printf("I phasor, %f, %f, %lf", iPhasor[i].mag, iPhasor[i].ang, iPhasor_refTime[i]);
//									}
//
//									double missedI, missedV;
//
//									//sample with phasor calc, I/V=Mag*(2*pi*f*t + Ang)
//									missedI = iPhasor[i].mag * sin(2.0 * M_PI * CfgNode.SysFreq * (tempSmpTime - iPhasor_refTime[i]) + iPhasor[i].ang);
//									missedV = vPhasor[i].mag * sin(2.0 * M_PI * CfgNode.SysFreq * (tempSmpTime - vPhasor_refTime[i]) + vPhasor[i].ang);
//									setSmpSetValues(temp_set, i, 0, missedI, missedV, true);
//
//									missedI = iPhasor[i].mag * sin(2.0 * M_PI * CfgNode.SysFreq * (tempSmpTime - smpTimeResolution/2 - iPhasor_refTime[i]) + iPhasor[i].ang);
//									missedV = vPhasor[i].mag * sin(2.0 * M_PI * CfgNode.SysFreq * (tempSmpTime - smpTimeResolution/2 - vPhasor_refTime[i]) + vPhasor[i].ang);
//									setSmpSetValues(temp_set, i, 1, missedI, missedV, true);
//
////										if(executionCt>=4 && executionCt<=8 && smpAway > 1 && j==0 && i==0)
////											printf("using phasor for missed");
//
//								} else {
//									//if no valid phasor exists then use linear interpolation to fill in the missing values
//									interpolateData(setDataStream.data.sample, prevSample[0][temp_set][i].data.sample,
//													prevSample[1][temp_set][i], curSampleTime, tempSmpTime, prevSampleSetTime,
//													interpolatedData);
//									setSmpSetValues(temp_set, i, 0, interpolatedData.currentNow, interpolatedData.voltageNow, true);
//									setSmpSetValues(temp_set, i, 1, interpolatedData.current0_5msAgo, interpolatedData.voltage0_5msAgo, true);
//
////										if(executionCt>=4 && executionCt<=8 && smpAway > 1 && j==0 && i==0)
////											printf("using interpolate for missed");
//								}
//
//								//set the prev samples only when it's missing samples since we set them here
//								if (smpAway > 1 && j==0) {
//
//									prevSample[1][temp_set][i].data.time.secs = (int) (curSmpTime - smpTimeResolution*3);
//									prevSample[1][temp_set][i].data.time.nsecs =  (int) ((curSmpTime - smpTimeResolution*3 - prevSample[1][temp_set][i].data.time.secs)*1000000000);
//									prevSample[1][temp_set][i].data.sample.currentNow = (int) (Ir*SmpNode.SmpSet[temp_set].I[i][4]);
//									prevSample[1][temp_set][i].data.sample.current0_5msAgo = (int) (Ir*SmpNode.SmpSet[temp_set].I[i][5]);
//									prevSample[1][temp_set][i].data.sample.voltageNow = (int) (Vr*SmpNode.SmpSet[temp_set].V[i][4]);
//
//									prevSample[0][temp_set][i].data.time.secs = (int) (curSmpTime - smpTimeResolution);
//									prevSample[0][temp_set][i].data.time.nsecs =  (int) ((curSmpTime - smpTimeResolution - prevSample[0][temp_set][i].data.time.secs)*1000000000);
//									prevSample[0][temp_set][i].data.sample.currentNow = (int) (Ir*SmpNode.SmpSet[temp_set].I[i][2]);
//									prevSample[0][temp_set][i].data.sample.current0_5msAgo = (int) (Ir*SmpNode.SmpSet[temp_set].I[i][3]);
//									prevSample[0][temp_set][i].data.sample.voltageNow = (int) (Vr*SmpNode.SmpSet[temp_set].V[i][2]);
//
////	                                    if (temp_set == 0 && i == 0)
////	                                    	printf("Prev Samples, %d, %d", prevSample[0][set][phase].data.sample.voltageNow, prevSample[1][set][phase].data.sample.voltageNow);
//								}
//
////	                                if (executionCt>=2 && executionCt<=6 && smpAway > 1 && j>0) {// && temp_set==0 && i==0) {
//////	                                	printf("I phasor, %f, %f, %lf", iPhasor[i].mag, iPhasor[i].ang, iPhasor_refTime[i]);
////
////	                                	printf("DSP: missed Sample, %d, %d-%d, %d, origTime=%lf, time=%lf, %f,%f", sampleSeq, temp_set, i, setsPhasesTotalSampleCt,
////	                                           curSampleTime, tempSmpTime, SmpNode.SmpSet[temp_set].I[i][1],
////	                                           SmpNode.SmpSet[temp_set].V[i][1]);
////	                                	printf("DSP: missed Sample, %d, %d-%d, %d, origTime=%lf, time=%lf, %f,%f", sampleSeq, temp_set, i, setsPhasesTotalSampleCt,
////	                                           curSampleTime, tempSmpTime, SmpNode.SmpSet[temp_set].I[i][0],
////	                                           SmpNode.SmpSet[temp_set].V[i][0]);
////	                                }
//							}
//
//							//interpolate the cur new data against the most recent filled in data for actually missing data
//							if (smpAway > 1) {
//								if (set==temp_set && j == 0) {
//									prevSmpSetTime = curSmpTime - smpTimeResolution;
//
//									interpolateData(setDataStream.data.sample, prevSample[0][set][phase].data.sample,
//													prevSample[1][set][phase], curSampleTime, curSmpTime, prevSmpSetTime,
//													interpolatedData);
//
//									setSmpSetValues(set, phase, 0, interpolatedData.currentNow, interpolatedData.voltageNow, false);
//									setSmpSetValues(set, phase, 1, interpolatedData.current0_5msAgo, interpolatedData.voltage0_5msAgo, false);
//
////	                                    if (executionCt>=2 && executionCt<=6){// && set==0 && phase==0) {
//////	                                    	printf("On last missed, %d, %d, %d, %lf, %lf, %lf", setDataStream.data.sample.voltageNow, prevSample[0][set][phase].data.sample.voltageNow,
//////	                                    	                                                prevSample[1][set][phase].data.sample.voltageNow, curSampleTime, curSmpTime, prevSmpSetTime);
////
////											printf("DSP: missed last Sample, %d, %d-%d, %d, origTime=%lf, time=%lf, %f,%f", sampleSeq, set, phase, setsPhasesTotalSampleCt,
////												   curSampleTime, curSmpTime, SmpNode.SmpSet[set].I[phase][1],
////												   SmpNode.SmpSet[set].V[phase][1]);
////											printf("DSP: missed last Sample, %d, %d-%d, %d, origTime=%lf, time=%lf, %f,%f", sampleSeq, set, phase, setsPhasesTotalSampleCt,
////												   curSampleTime, curSmpTime, SmpNode.SmpSet[set].I[phase][0],
////												   SmpNode.SmpSet[set].V[phase][0]);
////	                                    }
//
//									//this only added the latest 2 samples
//									checkCycleComplete(2);
//
//									isMissing = true;
//								}
//
//								if (temp_set == IcsNode.NumMesSets-1 && j > 0) {
//									//this added missing samples for all sets and all phases
////	                                    checkCycleComplete(2*IcsNode.NumMesSets*MES_PHASES);
//									checkCycleComplete(2);
//								}
//							}
//						}
//					}
//				}
//			}
//
//			if (!isMissing && smpAway <= 1) {
//				//interpolate between the prev and the newest sample
//				if (prevSample[0][set][phase].isCRCgood)
//					interpolateData(setDataStream.data.sample, prevSample[0][set][phase].data.sample, prevSample[1][set][phase], curSampleTime, curSmpTime, prevSampleSetTime, interpolatedData);
//				else {//this should only happen on the first sample per set per phase since the prevSample is none existent
//					//set the interpolated data
//					interpolatedData.currentNow = setDataStream.data.sample.currentNow;
//					interpolatedData.current0_5msAgo = setDataStream.data.sample.current0_5msAgo;
//					interpolatedData.voltageNow = setDataStream.data.sample.voltageNow;
//					interpolatedData.voltage0_5msAgo = setDataStream.data.sample.voltageNow;
//
//					isInitialSample = true;
//				}
//
//			}
//
//			//set the cur samples to the prevSample
//			memmove(&prevSample[1][set][phase], &prevSample[0][set][phase], DataStreamSize);
//			memcpy(&prevSample[0][set][phase], &setDataStream, DataStreamSize);
//
//			if (!isMissing && (prevSampleSetTime < curSmpTime || isInitialSample)) {
//				isInitialSample = false;
//
//				//only set the resampled when the last sample was before the desired time and obviously the new sample is after the desired sample time.  Also set it if it's the first sample
//				setSmpSetValues(set, phase, 0, interpolatedData.currentNow, interpolatedData.voltageNow, true);
//				setSmpSetValues(set, phase, 1, interpolatedData.current0_5msAgo, interpolatedData.voltage0_5msAgo, true);
//
////	                if (executionCt>=2 && executionCt<=6) { //&& set==0 && phase == 0) {
////						printf("DSP: new Sample, %d, %d-%d, %d, origTime=%lf, time=%lf, %f,%f", sampleSeq, set, phase, setsPhasesTotalSampleCt, curSampleTime,
////							   curSmpTime,
////							   SmpNode.SmpSet[set].I[phase][1], SmpNode.SmpSet[set].V[phase][1]);
////						printf("DSP: new Sample, %d, %d-%d, %d, origTime=%lf, time=%lf, %f,%f", sampleSeq, set, phase, setsPhasesTotalSampleCt, curSampleTime,
////							   curSmpTime,
////							   SmpNode.SmpSet[set].I[phase][0], SmpNode.SmpSet[set].V[phase][0]);
////	                }
//				//this only added 2 samples
//				checkCycleComplete(2);
//			}
//		}
//	}
//}

//void TFDIRContainer::processSmpSetData(DataStream& setDataStream, int set, int idx) {
//    int phase = setDataStream.data.id/2;
//    if (phase > MES_PHASES-1)
//        phase = phase-MES_PHASES;
//
//    if(idx >= 0) {
//        //this means it's a newer sample, which will require shifting of the existing values down.
//        //Also requires it to set the time
//        SmpNode.SmpSet[set].t.sec = setDataStream.data.time.secs;
//        SmpNode.SmpSet[set].t.usec = setDataStream.data.time.nsecs/1000;
//
//        if (idx > 6) {
//            //missed too many samples, reset to 0s
//            memset(&SmpNode.SmpSet[set].I, 0, MES_PHASES*SV_CYC_SIZE*XY_Size);
//            memset(&SmpNode.SmpSet[set].V, 0, MES_PHASES*SV_CYC_SIZE*XY_Size);
//
//        } else if (idx > 0) {
//            //shift the existing samples down by the idx
//            memmove(&SmpNode.SmpSet[set].I[phase][idx], &SmpNode.SmpSet[set].I[phase][0], (SV_CYC_SIZE-idx)*XY_Size);
//            memmove(&SmpNode.SmpSet[set].V[phase][idx], &SmpNode.SmpSet[set].V[phase][0], (SV_CYC_SIZE-idx)*XY_Size);
//
//            if (idx > 2) {
//                //need to fill in the missed samples using the last know phasors
//                double curSampleTime = setDataStream.data.time.secs + (double)setDataStream.data.time.nsecs/1000000000.0;
//                XY *iPhasor = &IcsNode.IcsSet[set].IcsSns[phase].Ibuf[0];
//                XY *vPhasor = &IcsNode.IcsSet[set].IcsSns[phase].Vbuf[0];
//                double missedI, missedV;
//
//                for (int i=3; i<=idx; i++) {
//                    //sample calc, I/V=Mag*(2*pi*f*t + Ang)
//                    missedI = iPhasor->x*cos(2*M_PI*CfgNode.SysFreq*(curSampleTime-0.0005*i) + iPhasor->y)/Ir;
//                    missedV = vPhasor->x*cos(2*M_PI*CfgNode.SysFreq*(curSampleTime-0.0005*i) + vPhasor->y)/Ir;
//
//                    setSmpSetValues(set, phase, (i-1), missedI, missedV, true);
//
//                    printf("DSP: Missed Sample time=%f, idx=%d/%d, I=%f, V=%f", curSampleTime, i, idx, missedI, missedV);
//                }
//            }
//        }
//
//        //always sets the newest sample to 0th obj
//        idx = 0;
//    }
//
//    if (idx >= -31 && idx <= 0) {
//        //set the newest sample that should always include the data from 500us ago too
//        setSmpSetValues(set, phase, idx, setDataStream.data.sample.currentNow, setDataStream.data.sample.voltageNow, true);
//        setSmpSetValues(set, phase, idx+1, setDataStream.data.sample.current0_5msAgo, setDataStream.data.sample.voltageNow, true); //needs to be voltage0_5msAgo
//
////      double curSampleTime = setDataStream.data.time.secs + (double)setDataStream.data.time.nsecs/1000000000.0;
////		printf("DSP: new Sample time=%f, idx=%d, I=%f, V=%f", curSampleTime, idx, SmpNode.SmpSet[set].I[phase][idx], SmpNode.SmpSet[set].V[phase][idx]);
////		printf("DSP: new Sample2 time=%f, idx=%d, I=%f, V=%f", curSampleTime, idx+1, SmpNode.SmpSet[set].I[phase][idx+1], SmpNode.SmpSet[set].I[phase][idx+1]);
//    }
//}

void TFDIRContainer::addToSampleBuffer(DataStream& setDataStream, int set) {

    //covert id to phase designation
    int phase = setDataStream.data.id/2;
    if (phase > MES_PHASES-1)
        phase = phase-MES_PHASES;

    sampleSeq++;

    if (sampleBufferCounter[set][phase] >= 4) {
        //call addNewSample to process a full 2 sets of samples
        for (int i=0; i<IcsNode.NumMesSets; i++) {
            for (int j=0; j<MES_PHASES; j++) {
                if (sampleBufferCounter[i][j] > 0) {
//                    addNewSampleSinglePhase(sampleBuffer[i][j][0], i, j);
                    addNewSampleSinglePhaseTimeClamped(sampleBuffer[i][j][0], i, j);
                    sampleBufferCounter[i][j]--;
                }
            }
        }
    }

    //add this sample to the buffer and sort the buffer according to time
//        int curIdx = sampleBufferCounter[set][phase];
    memcpy(&sampleBuffer[set][phase][0], &setDataStream, DataStreamSize);
    sampleBufferCounter[set][phase]++;

    std::sort(&sampleBuffer[set][phase][0], &sampleBuffer[set][phase][4], sortBufferByTime);
}
#endif

bool TFDIRContainer::sortBufferByTime(DataStream &stream1, DataStream &stream2) {
    double time1 = stream1.data.time.secs + (double)stream1.data.time.nsecs/1000000000.0;
    double time2 = stream2.data.time.secs + (double)stream2.data.time.nsecs/1000000000.0;

    return time1 < time2;
}

void TFDIRContainer::checkCycleComplete(int numSamplesAdded, int set, int phase) {

    setsPhasesTotalSampleCt[set][phase]+=numSamplesAdded;

    if (setsPhasesTotalSampleCt[set][phase] >= SV_CYC_SIZE) {
        setsPhasesTotalSampleCt[set][phase] = 0;

//		if (set==0 && phase==0) //should only execute this when it's set 0 and phase 0 since it's the master clock
        executeTFDIR(set, phase);
    }
}

bool TFDIRContainer::validateData(DataStream setDataStream) {
    bool isValid = false;

    if (setDataStream.data.sample.currentNow >= -ILimit && setDataStream.data.sample.currentNow <= ILimit &&
        setDataStream.data.sample.current0_5msAgo >= -ILimit && setDataStream.data.sample.current0_5msAgo <= ILimit &&
        setDataStream.data.sample.voltageNow >= -VLimit && setDataStream.data.sample.voltageNow <= VLimit)
        isValid = true;

    return isValid;
}

void TFDIRContainer::setSmpSetValues(int set, int phase, int idx, float i, float v, bool countSample) {

    SmpNode.SmpSet[set].I[phase][idx] = Ir_inv*i;
    SmpNode.SmpSet[set].V[phase][idx] = Vr_inv*v;

    if (countSample) {
        //set all sets and phases to be normal
        SmpNode.SmpSet[set].Iflg[phase] = DATA_NORMAL;
        SmpNode.SmpSet[set].Vflg[phase] = DATA_NORMAL;

        if (SmpNode.SmpSet[set].NumI[phase] < SV_CYC_SIZE) {
            SmpNode.SmpSet[set].NumI[phase] += 2;
            SmpNode.SmpSet[set].NumV[phase] += 2;
        }
    }
}

int TFDIRContainer::numSamplesAway(double time, double timeRef, double timeRange) {
//    double timeRange = 0.000375;//  375us, 75% of 1 fpga sampling period

//	double diff = round(1000000 * (time-timeRef))/1000000;
    double diff = time-timeRef;

    return (int)(diff/timeRange);
}

void TFDIRContainer::interpolateData(DataType& curData, DataType& prevData, double curTime, double interTime, double prevTime, InterpolatedData& interpolatedData) {
    interpolatedData.currentNow = prevData.currentNow + (curData.currentNow - prevData.currentNow) * (interTime - prevTime)/(curTime - prevTime);
    interpolatedData.current0_5msAgo = prevData.current0_5msAgo + (curData.current0_5msAgo - prevData.current0_5msAgo) * (interTime - smpTimeResolution/2 - (prevTime - 0.0005))/(curTime - prevTime);

    interpolatedData.voltageNow = prevData.voltageNow + (curData.voltageNow - prevData.voltageNow) * (interTime - prevTime)/(curTime - prevTime);
    interpolatedData.voltage0_5msAgo = prevData.voltageNow + (curData.voltageNow - prevData.voltageNow) * (interTime - smpTimeResolution/2 - prevTime)/(curTime - prevTime);
}

void TFDIRContainer::interpolateData(DataType& curData, DataType& prevData, DataStream& prevPrevSample, double curTime, double interTime, double prevTime, InterpolatedData& interpolatedData) {
    interpolatedData.currentNow = prevData.currentNow + (curData.currentNow - prevData.currentNow) * (interTime - prevTime)/(curTime - prevTime);
    interpolatedData.current0_5msAgo = prevData.current0_5msAgo + (curData.current0_5msAgo - prevData.current0_5msAgo) * (interTime - smpTimeResolution/2 - (prevTime - 0.0005))/(curTime - prevTime);

    interpolatedData.voltageNow = prevData.voltageNow + (curData.voltageNow - prevData.voltageNow) * (interTime - prevTime)/(curTime - prevTime);

    //the 0.5ms sample time could be before the prev sample time
    if (prevTime > (interTime - smpTimeResolution/2)) {
        curTime = prevTime;
        prevTime = prevPrevSample.data.time.secs + (double)prevPrevSample.data.time.nsecs/1000000000.0;

        interpolatedData.voltage0_5msAgo = prevPrevSample.data.sample.voltageNow + (prevData.voltageNow - prevPrevSample.data.sample.voltageNow) *
                                                                                   (interTime - smpTimeResolution / 2 - prevTime) /
                                                                                   (curTime - prevTime);
    } else {
        interpolatedData.voltage0_5msAgo = prevData.voltageNow + (curData.voltageNow - prevData.voltageNow) *
                                                                 (interTime - smpTimeResolution / 2 - prevTime) /
                                                                 (curTime - prevTime);
    }
}

void TFDIRContainer::calcPhasors(int set, int phase) {
    int i, j, k, l;
    float x, y, w, z;
    double timeDiff = 0;
    bool zeroPhasor = false;
//    float Ir = 2.0*IcsNode.Cfg->CfgSet[0].Irated;
//    float Vr = IcsNode.Cfg->Vrated;
    FLAG  Vbad;

    float F = 60.0;
    Phasor VP, IP;

    phasor_init(&VP, F);
    phasor_init(&IP, F);

    //use the given set and phase time as the master time
//    double masterSmpTime = SmpNode.SmpSet[0].phase_st[0] + SmpNode.SmpSet[0].phase_t_ct[0]*smpTimeResolution - cycleDuration;
//    double masterSmpTime = SmpNode.SmpSet[set].phase_st[phase] + SmpNode.SmpSet[set].phase_t_ct[phase]*smpTimeResolution - cycleDuration;
    double masterSmpTime = executionCt * cycleDuration;
    long masterSec = (long) masterSmpTime;
    long masterUsec = (long) ((masterSmpTime - masterSec) * 1000000);

    double refTimeDiff = 0.0;
    if (set != initialRefSensor.set || phase != initialRefSensor.phase) {
        refTimeDiff = SmpNode.SmpSet[set].phase_st[phase] - SmpNode.SmpSet[initialRefSensor.set].phase_st[initialRefSensor.phase];

//    	printf("not initial sensor: %d-%d, %lf", set, phase, refTimeDiff);
    }

//    printf("smpTime, %lf, %ld, %ld", smpTime, sec, usec);

    for (i = 0; i < MAX_SETS; i++) {
        if (!IcsNode.IcsSet[i].Cfg->SrvFlg || IcsNode.IcsSet[i].Cfg->VrtFlg)
            continue;
        Vbad = 0;
        for (j = 0; j < MES_PHASES; j++) {
            k = (int)IcsNode.IcsSet[i].IcsSns[j].Cfg->SeqNum - 1; // Use the SeqNum to correct the Phase lable
            l = (int)IcsNode.IcsSet[i].IcsSns[j].Cfg->PhaseID - 1;
            if (l == 3) l = 2;
            IcsNode.IcsSet[i].IcsSns[l].Ibuf[0].flg = SmpNode.SmpSet[i].Iflg[k];
//            IcsNode.IcsSet[i].IcsSns[l].Vbuf[0].t   = SmpNode.SmpSet[i].t;
            IcsNode.IcsSet[i].IcsSns[l].Ibuf[0].t.sec = masterSec;
            IcsNode.IcsSet[i].IcsSns[l].Ibuf[0].t.usec = masterUsec;
            IcsNode.IcsSet[i].IcsSns[l].Vbuf[0].t.sec = masterSec;
            IcsNode.IcsSet[i].IcsSns[l].Vbuf[0].t.usec = masterUsec;
            IcsNode.IcsSet[i].IcsSns[l].Vbuf[0].flg = SmpNode.SmpSet[i].Vflg[k];

//            double phaseSmpTime = (double)SmpNode.SmpSet[i].phase_t[l].sec + (double)SmpNode.SmpSet[i].phase_t[l].usec/1000000.0 - cycleDuration;
//            double phaseSmpTime = SmpNode.SmpSet[i].phase_st[l] + SmpNode.SmpSet[i].phase_t_ct[l]*smpTimeResolution - cycleDuration;
            double phaseSmpTime = executionCt * cycleDuration;
            long phaseSec = (long) phaseSmpTime;
            long phaseUsec = (long) ((phaseSmpTime - phaseSec) * 1000000);

            timeDiff = masterSmpTime - phaseSmpTime - refTimeDiff;

            SmpNode.SmpSet[i].IRefPhasor[l].t.sec = phaseSec;
            SmpNode.SmpSet[i].IRefPhasor[l].t.usec = phaseUsec;
            SmpNode.SmpSet[i].VRefPhasor[l].t.sec  = phaseSec;
            SmpNode.SmpSet[i].VRefPhasor[l].t.usec = phaseUsec;

            //check to see if there's too long of a gap (>8 samples) to the master clock.
            //if there is a gap then the phasor should be 0
            zeroPhasor = false;
            if (SmpNode.SmpSet[set].phase_t_ct[phase] - SmpNode.SmpSet[i].phase_t_ct[l] > 4) {
                IcsNode.IcsSet[i].IcsSns[l].Ibuf[0].x = 0.0;
                IcsNode.IcsSet[i].IcsSns[l].Ibuf[0].y = 0.0;
                IcsNode.IcsSet[i].IcsSns[l].Vbuf[0].x = 0.0;
                IcsNode.IcsSet[i].IcsSns[l].Vbuf[0].y = 0.0;
                zeroPhasor = true;
                printf("Detected zero for %d-%d", i, l);
            }

            if (!zeroPhasor) {
                if (!SmpNode.SmpSet[i].Iflg[k] && SmpNode.SmpSet[i].NumV[k] > 0) {
                    x = y = 0.0;

                    //				phasor_calc(&IP, SmpNode.SmpSet[i].I[k]);
                    //				x = IP.ax;
                    //				y = IP.ay;

                    fft_ReversedSmp(&x, &y, IcsNode.CosV, IcsNode.SinV, SmpNode.SmpSet[i].I[k], SV_CYC_SIZE);

                    //                if (inSimulationMode && i==0 && j==0) {
                    //                	printf("DSP xy %d, %d, %d, %d, %ld, %ld, %d: %f, %f", i, j, k, l, sec, usec, SmpNode.SmpSet[i].NumI[k], x, y);
                    //                }

                    w = XY2Mag(x, y);
                    //                w *= 1.4142*Ir*CfgNode.CfgSet[i].AmpSclFactor/(float) SmpNode.SmpSet[i].NumI[k];
                    w *= 1.4142*Ir*CfgNode.CfgSet[i].AmpSclFactor/(float) SmpNode.SmpSet[i].NumI[k];
                    z = XY2Ang(x, y) + CfgNode.CfgSet[i].AmpAngOffset;

                    //this is the ref phasor for the given phase
                    SmpNode.SmpSet[i].IRefPhasor[k].mag = w;
                    SmpNode.SmpSet[i].IRefPhasor[k].ang = z;

                    //sycronize the phasor to the master time for the given phase
                    //                if(i==0 && j==1)
                    //                	printf("Time diff: %lf", timeDiff);
                    z += timeDiff * degreesPerSec;
                    MagAng2XY(&x, &y, &w, &z);

                    // For yemporary use
                    if (w < 2.0)
                        IcsNode.IcsSet[i].IcsSns[l].Ibuf[0].flg - DATA_NO;
                    else
                        IcsNode.IcsSet[i].IcsSns[l].Ibuf[0].flg - DATA_NORMAL;
                    //this is the sync'd x,y for the given phase
                    IcsNode.IcsSet[i].IcsSns[l].Ibuf[0].x = x;
                    IcsNode.IcsSet[i].IcsSns[l].Ibuf[0].y = y;

                    //                if (inSimulationMode && i==0 && j==0) {
                    //                if (executionCt >=0 && executionCt <=100) {
                    ////                if (i==0 && j==0) {
                    //					printf("DSP I Phasor %d, %d, %d, %d, %ld, %ld, %lf, %d: %f, %f", i, j, k, l, masterSec, masterUsec, timeDiff, SmpNode.SmpSet[i].NumI[k], w, z);
                    ////					for(int tt=SV_CYC_SIZE-1; tt>=0; tt--) {
                    ////						printf("DSP smp, %d, %f", tt, SmpNode.SmpSet[i].I[k][tt]);
                    ////					}
                    //				}

                    if (w < IcsNode.Cfg->Imin)
                        IcsNode.IcsSet[i].IcsSns[j].Ibuf[0].flg = DATA_NO;
                }
                else { // bad data or no data, set to the previous cycle value
                    IcsNode.IcsSet[i].IcsSns[j].Ibuf[0].x = IcsNode.IcsSet[i].IcsSns[j].Ibuf[1].x;
                    IcsNode.IcsSet[i].IcsSns[j].Ibuf[0].y = IcsNode.IcsSet[i].IcsSns[j].Ibuf[1].y;
                }

                if (!SmpNode.SmpSet[i].Vflg[k] && SmpNode.SmpSet[i].NumV[k] > 0) {
                    x = y = 0.0;

                    //				phasor_calc(&VP, SmpNode.SmpSet[i].V[k]);
                    //				x = VP.ax;
                    //				y = VP.ay;

                    fft_ReversedSmp(&x, &y, IcsNode.CosV, IcsNode.SinV, SmpNode.SmpSet[i].V[k], SV_CYC_SIZE);

                    w = XY2Mag(x, y);
                    //                w *= 1.4142*Vr*CfgNode.CfgSet[i].VoltSclFactor/(float) SmpNode.SmpSet[i].NumV[k];
                    w *= 1.4142*Vr*CfgNode.CfgSet[i].VoltSclFactor/(float) SmpNode.SmpSet[i].NumV[k];
                    z = XY2Ang(x, y) + CfgNode.CfgSet[i].VoltAngOffset;

                    //this is the ref phasor for the given phase
                    SmpNode.SmpSet[i].VRefPhasor[k].mag = w;
                    SmpNode.SmpSet[i].VRefPhasor[k].ang = z;

                    //sycronize the phasor to the master time for the given phase
                    z += timeDiff * degreesPerSec;

                    IcsNode.IcsSet[i].VmIcs[l].mag = w;	// Volt Mag before dynamic calibration
                    IcsNode.IcsSet[i].VmIcs[l].ang = z;	// Volt Ang before dynamic calibration

                    w *= IcsNode.IcsSet[i].IcsSns[l].Vmdcf; // Apply dinamic calibration on Volt Mag
                    z += IcsNode.IcsSet[i].IcsSns[l].Vadcf; // Apply dinamic calibration on Volt Ang

                    //                printf("calc phasor: %f,%f", IcsNode.IcsSet[i].IcsSns[l].Vmdcf, IcsNode.IcsSet[i].IcsSns[l].Vadcf);

                    if (w < IcsNode.Cfg->Vmin) {
                        IcsNode.IcsSet[i].IcsSns[l].Vbuf[0].flg = DATA_NO;
                        Vbad ++;
                    }
                    MagAng2XY(&x, &y, &w, &z);
                    IcsNode.IcsSet[i].IcsSns[l].Vbuf[0].x = x;
                    IcsNode.IcsSet[i].IcsSns[l].Vbuf[0].y = y;

                    if (executionCt >=0 && executionCt <=100) {
                        //                if (i==0 && j==0) {
                        //					printf("DSP V Phasor %d, %d, %d, %d, %ld, %ld, %lf, %d: %f, %f", i, j, k, l, masterSec, masterUsec, timeDiff, SmpNode.SmpSet[i].NumV[k], w, z);
                        //					for(int tt=SV_CYC_SIZE-1; tt>=0; tt--) {
                        //						printf("DSP smp, %d, %f", tt, SmpNode.SmpSet[i].I[k][tt]);
                        //					}
                    }

                    //                printf("DSP Phasor %d, %d, %d, %d: %f, %f, %f", i, j, k, l, w, z, IcsNode.Cfg->Vmin);
                }
                else { // bad data or no data, set to the previous cycle value
                    //				IcsNode.IcsSet[i].IcsSns[j].Vbuf[0].x = IcsNode.IcsSet[i].IcsSns[j].Vbuf[1].x ;
                    //				IcsNode.IcsSet[i].IcsSns[j].Vbuf[0].y = IcsNode.IcsSet[i].IcsSns[j].Vbuf[1].y ;
                    Vbad ++;
                }

                //            if (inSimulationMode)
                //            	printf("DSP Phasor %d, %d, %d, %d: %.2f, %.2f\n", i, j, k, l, IcsNode.IcsSet[i].IcsSns[l].Ibuf[0].x, IcsNode.IcsSet[i].IcsSns[l].Ibuf[0].y);

                if(IcsNode.IcsSet[i].IcsSns[j].Cfg->CTPolarity) {
                    IcsNode.IcsSet[i].IcsSns[l].Ibuf[0].x *= -1.0;
                    IcsNode.IcsSet[i].IcsSns[l].Ibuf[0].y *= -1.0;
                }
            }
        }

        if (Vbad) {
            for (j = 0; j < MES_PHASES; j++) {
                IcsNode.IcsSet[i].IcsSns[j].Vbuf[0].x = IcsNode.IcsSet[i].IcsSns[j].Vbuf[1].x ;
                IcsNode.IcsSet[i].IcsSns[j].Vbuf[0].y = IcsNode.IcsSet[i].IcsSns[j].Vbuf[1].y ;
            }
        }
        else if (IcsNode.IcsSet[i].IcsSns[0].Cfg->UseVT && IcsNode.IcsSet[i].IcsSns[1].Cfg->UseVT &&
                 IcsNode.IcsSet[i].IcsSns[2].Cfg->UseVT) { // 3-phase voltages for use
            for (j = 0; j < MES_PHASES; j++) {
                x = y = 0.0;
                for (k = 0; k < MES_PHASES; k++) {
                    x += IcsNode.VoltMtrx[j][k]*IcsNode.IcsSet[i].IcsSns[k].Vbuf[0].x;
                    y += IcsNode.VoltMtrx[j][k]*IcsNode.IcsSet[i].IcsSns[k].Vbuf[0].y;
                }
                IcsNode.IcsSet[i].IcsSns[j].Vbuf[0].x = x;
                IcsNode.IcsSet[i].IcsSns[j].Vbuf[0].y = y;
            }
        }
    }

    if (IcsNode.NumMesSets == IcsNode.NumActSets - 1) { // There is a virtual set
        k = -1;
        l = -1;
        x = 0;
        for (i = 0; i < MAX_SETS; i++) {
            if (IcsNode.IcsSet[i].Cfg->SrvFlg && IcsNode.IcsSet[i].Cfg->VrtFlg) {
                k = i;
                continue;
            }
            y = 1.e6;
            for (j = 0; j < MES_PHASES; j++) {
                w = XY2Mag(IcsNode.IcsSet[i].IcsSns[j].Ibuf[0].x, IcsNode.IcsSet[i].IcsSns[j].Ibuf[0].y);
                if (w > 2*IcsNode.Cfg->Imin && w < y)
                    y = w;
            }

            if (y < 1.e5 && y > x) {
                x = y;
                l = i;
            }
        }

        if (k >= 0) {
            for (j = 0; j < MES_PHASES; j++) {
                memset(&IcsNode.IcsSet[k].IcsSns[j].Ibuf[0], 0, sizeof(XY));
                memset(&IcsNode.IcsSet[k].IcsSns[j].Vbuf[0], 0, sizeof(XY));
                IcsNode.IcsSet[k].IcsSns[j].Ibuf[0].flg = DATA_NO;
            }

            for (i = 0; i < MAX_SETS; i++) {
                if (!IcsNode.IcsSet[i].Cfg->SrvFlg || IcsNode.IcsSet[i].Cfg->VrtFlg)
                    continue;
                for (j = 0; j < MES_PHASES; j++) {
                    IcsNode.IcsSet[k].IcsSns[j].Ibuf[0].flg = (IcsNode.IcsSet[i].IcsSns[j].Ibuf[0].flg == DATA_NORMAL)?
                            DATA_NORMAL : IcsNode.IcsSet[k].IcsSns[j].Ibuf[0].flg;
                    IcsNode.IcsSet[k].IcsSns[j].Ibuf[0].t = IcsNode.IcsSet[i].IcsSns[j].Ibuf[0].t;
                    IcsNode.IcsSet[k].IcsSns[j].Ibuf[0].x -= IcsNode.IcsSet[i].IcsSns[j].Ibuf[0].x;
                    IcsNode.IcsSet[k].IcsSns[j].Ibuf[0].y -= IcsNode.IcsSet[i].IcsSns[j].Ibuf[0].y;
                }
            }
            if (l >= 0) {
                for (j = 0; j < MES_PHASES; j++) {
//                        IcsNode.IcsSet[k].IcsSns[j].Vbuf[0].flg = IcsNode.IcsSet[l].IcsSns[j].Vbuf[0].flg;
                    IcsNode.IcsSet[k].IcsSns[j].Vbuf[0].t = IcsNode.IcsSet[l].IcsSns[j].Vbuf[0].t;
                    IcsNode.IcsSet[k].IcsSns[j].Vbuf[0].x = IcsNode.IcsSet[l].IcsSns[j].Vbuf[0].x;
                    IcsNode.IcsSet[k].IcsSns[j].Vbuf[0].y = IcsNode.IcsSet[l].IcsSns[j].Vbuf[0].y;
                }
            }
        }
    }
}

// calculate original I phasor from Buf x, y
PHASOR TFDIRContainer::XY2PhasorI(XY *xy, int i) {
    PHASOR p;
    p.mag  = sqrt((xy->x*xy->x) + (xy->y*xy->y));
    p.mag *= SV_CYC_SIZE / (1.4142*Ir*CfgNode.CfgSet[i].AmpSclFactor);
    p.ang  = (float)(atan2(xy->y, xy->x)*PI2DEGREES);
    p.ang -= CfgNode.CfgSet[i].AmpAngOffset;
    p.t    = xy->t;
    return p;
}

// calculate original V phasor from Buf x, y
PHASOR TFDIRContainer::XY2PhasorV(XY *xy, int i, int j) { // Set i, Sensor j
    int l;
    PHASOR p;
    l = (int)IcsNode.IcsSet[i].IcsSns[j].Cfg->PhaseID - 1;
    p.mag  = sqrt((xy->x*xy->x) + (xy->y*xy->y));
    p.mag *= SV_CYC_SIZE / (1.4142*Vr*CfgNode.CfgSet[i].VoltSclFactor*IcsNode.IcsSet[i].IcsSns[l].Vmdcf);
    p.ang  = (float)(atan2(xy->y, xy->x)*PI2DEGREES);
    p.ang -= CfgNode.CfgSet[i].VoltAngOffset + IcsNode.IcsSet[i].IcsSns[l].Vadcf;
    p.t    = xy->t;
    return p;
}

// calculate the magnitude from x & y values.
float TFDIRContainer::XY2Mag(float x, float y) {
    return (float)(sqrt((x*x) + (y*y)));
}

// calculate the angle from x & y values.
float TFDIRContainer::XY2Ang(float x, float y) {
//	if (y==0)
//		return 0.0;
//	else
    return (float)(atan2(y, x)*PI2DEGREES);
}

// calculate the x (real) component from magnitude & angle values.
float TFDIRContainer::MagAng2X(float mag, float ang) {
    return (float)(mag * cos(ang * DEGREES2PI));
}

// calculate the y (reactive) component from magnitude & angle values.
float TFDIRContainer::MagAng2Y(float mag, float ang) {
    return (float)(mag * sin(ang * DEGREES2PI));
}

// calculate the x (real) and y (reactive) components from magnitude & angle values.
void TFDIRContainer::MagAng2XY(float *x, float *y, float *mag, float *ang) {
    float w = *ang * DEGREES2PI;
    *x = *mag * cos(w);
    *y = *mag * sin(w);
}

RX TFDIRContainer::VectAdd(RX* Z1, RX* Z2) {
    RX w;
    w.r = Z1->r + Z2->r;
    w.x = Z1->x + Z2->x;
    return w;
}

RX TFDIRContainer::VectSub(RX* Z1, RX* Z2) {
    RX w;
    w.r = Z1->r - Z2->r;
    w.x = Z1->x - Z2->x;
    return w;
}

RX TFDIRContainer::VectX2(RX* Z1, RX* Z2) {
    RX w;
    w.r = Z1->r*Z2->r - Z1->x*Z2->x;
    w.x = Z1->r*Z2->x + Z1->x*Z2->r;
    return w;
}

RX TFDIRContainer::VectX3(RX* Z1, RX* Z2, RX* Z3) {
    float r, x;
    RX w;
    r = Z1->r*Z2->r - Z1->x*Z2->x;
    x = Z1->r*Z2->x + Z1->x*Z2->r;
    w.r = r*Z3->r - x*Z3->x;
    w.x = r*Z3->x + x*Z3->r;
    return w;
}

RX TFDIRContainer::VectDiv(RX* Z1, RX* Z2) {
    float r, x;
    RX w;
    r = Z2->r*Z2->r + Z2->x*Z2->x;
    if (r >= 1e-10)
        r = 1.0/r;
    else {
        r = 1.0e10;
    }
    w.r = r*(Z1->r*Z2->r + Z1->x*Z2->x);
    w.x = r*(Z2->r*Z1->x - Z2->x*Z1->r);
    return w;
}

void TFDIRContainer::abcxy2s0(PHASOR *S, XY *A, XY *B, XY *C)  // Calculate 3.0*I0 from ABC vectors
{
    float x, y;
    x = A->x + B->x + C->x;
    y = A->y + B->y + C->y;
    S->mag = XY2Mag(x, y)/3.0;
    S->ang = XY2Ang(x, y);
}

void TFDIRContainer::abc2s0(PHASOR *S, PHASOR *A, PHASOR *B, PHASOR *C)  // Calculate 3.0*I0 from ABC vectors
{
    float x, y;
    x = MagAng2X(A->mag, A->ang) + MagAng2X(B->mag, B->ang) + MagAng2X(C->mag, C->ang);
    y = MagAng2Y(A->mag, A->ang) + MagAng2Y(B->mag, B->ang) + MagAng2Y(C->mag, C->ang);
    S->mag = XY2Mag(x, y)/3.0;
    S->ang = XY2Ang(x, y);
}

void TFDIRContainer::abc2s1(PHASOR *S, PHASOR *A, PHASOR *B, PHASOR *C)  // Calculate I1 from ABC vectors
{
    float x, y;
    x = MagAng2X(A->mag, A->ang) + MagAng2X(B->mag, B->ang+120.0) + MagAng2X(C->mag, C->ang-120.0);
    y = MagAng2Y(A->mag, A->ang) + MagAng2Y(B->mag, B->ang+120.0) + MagAng2Y(C->mag, C->ang-120.0);
    S->mag = XY2Mag(x, y)/3.0;
    S->ang = XY2Ang(x, y);
}

void TFDIRContainer::abc2s2(PHASOR *S, PHASOR *A, PHASOR *B, PHASOR *C)  // Calculate I2 from ABC vectors
{
    float x, y;
    x = MagAng2X(A->mag, A->ang) + MagAng2X(B->mag, B->ang-120.0) + MagAng2X(C->mag, C->ang+120.0);
    y = MagAng2Y(A->mag, A->ang) + MagAng2Y(B->mag, B->ang-120.0) + MagAng2Y(C->mag, C->ang+120.0);
    S->mag = XY2Mag(x, y)/3.0;
    S->ang = XY2Ang(x, y);
}

void TFDIRContainer::abc2s012(PHASOR *Seq012, PHASOR *ABC)  // Convert phase domain ABC vector into Seqeunce Domain 012 vector
{
    int i;
    float x, y, z;
    x = y = 0.0;
    for (i = 0; i < 3; i++) {                        //Zero sequence
        x += MagAng2X(ABC[i].mag, ABC[i].ang);
        y += MagAng2Y(ABC[i].mag, ABC[i].ang);
    }
    Seq012[0].mag = XY2Mag(x, y)/3.0;
    Seq012[0].ang = XY2Ang(x, y);
    x = y = 0.0;
    for (i = 0; i < 3; i++) {                        //Positive Sequence
        z = float(i)*120;
        x += MagAng2X(ABC[i].mag, ABC[i].ang + z);
        y += MagAng2Y(ABC[i].mag, ABC[i].ang + z);
    }
    Seq012[1].mag = XY2Mag(x, y)/3.0;
    Seq012[1].ang = XY2Ang(x, y);
    x = y = 0.0;
    for (i = 0; i < 3; i++) {                        //Negativetive Sequence
        z = float(i)*120;
        x += MagAng2X(ABC[i].mag, ABC[i].ang - z);
        y += MagAng2Y(ABC[i].mag, ABC[i].ang - z);
    }
    Seq012[2].mag = XY2Mag(x, y)/3.0;
    Seq012[2].ang = XY2Ang(x, y);
}

void TFDIRContainer::PhasorAdd(PHASOR *S, PHASOR *A, PHASOR *B)  // Phasor S = A + B
{
    float x, y;
    x = MagAng2X(A->mag, A->ang) + MagAng2X(B->mag, B->ang);
    y = MagAng2Y(A->mag, A->ang) + MagAng2Y(B->mag, B->ang);
    S->mag = XY2Mag(x, y);
    S->ang = XY2Ang(x, y);
}

void TFDIRContainer::PhasorDiff(PHASOR *D, PHASOR *A, PHASOR *B)  // Phasor D = A - B
{
    float x, y;
    x = MagAng2X(A->mag, A->ang) - MagAng2X(B->mag, B->ang);
    y = MagAng2Y(A->mag, A->ang) - MagAng2Y(B->mag, B->ang);
    D->mag = XY2Mag(x, y);
    D->ang = XY2Ang(x, y);
}

PHASOR TFDIRContainer::PhasorShift(PHASOR *A, float Ang)  // Phasor A->ang += Ang
{
    PHASOR p;
    int    k;
    p = *A;
    p.ang += Ang; // shifting the phase angle shifts the reference only, does not change the original time t
    if(fabs(p.ang) >= 360.0) {
        k = ((int)p.ang)/360;
        p.ang -= (float) (360*k);
    }
    if(p.ang > 180.0)
        p.ang -= 360.0;
    else if (p.ang <= -180.0)
        p.ang += 360;
    return p;
}

void TFDIRContainer::VI2PQ(float *P, float *Q, PHASOR *V, PHASOR *I)
{
    float vx, vy, ix, iy;
    vx = MagAng2X(V->mag, V->ang);
    vy = MagAng2Y(V->mag, V->ang);
    ix = MagAng2X(I->mag, I->ang);
    iy = MagAng2Y(I->mag, I->ang);
    *P = sqrt(vx*ix + vy*iy);
    *Q = sqrt(vy*ix - vx*iy);
}

float TFDIRContainer::TimeDiff (TIME *t1, TIME *t2)
{
    float t = 1000.0*(float)(t1->sec - t2->sec) + (float)(t1->usec - t2->usec)/1000.0;
    return t; // Milli Seconds
}

void TFDIRContainer::fft(float *X, float *Y, float *C, float *S, float *SV, int K)
{
    int i, m;

    m = (K > 0) ? K : 1;
    m = (m > SV_CYC_SIZE) ? SV_CYC_SIZE : m;
    for (i = 0; i < m; i++) {
        *X += *SV * *S++;
        *Y += *SV++ * *C++;
    }
    if (K > m) {
        S -= m;
        C -= m;
        m = K - m;
        for (i = 0; i < m; i++) {
            *X += *SV * *S++;
            *Y += *SV++ * *C++;
        }
    }
}

void TFDIRContainer::fft_ReversedSmp(float *X, float *Y, float *C, float *S, float *SV, int K)
{
    int i, m;

    m = (K > 0) ? K : 1;
    m = (m > SV_CYC_SIZE) ? SV_CYC_SIZE : m;
    for (i = 0; i < m; i++) {
        *X += SV[m-1-i] * *S++;
        *Y += SV[m-1-i] * *C++;
    }
    if (K > m) {
        S -= m;
        C -= m;
        m = K - m;
        for (i = 0; i < m; i++) {
            *X += SV[m-1-i] * *S++;
            *Y += SV[m-1-i] * *C++;
        }
    }
}

int TFDIRContainer::MatInv(float *b, float *a, int n) // Matrix Inverse:
{// Input:  a[n][n] with b[n][n] being unity diagnal
    // Output: b[n][n] with a[n][n] beining unity diagnal
    float x;
    int i, j, k;
    int err = 0;
    for (i = 0; i < n; i++) {
        x = *((a+i*n)+i);
        if (fabs(x) <= 1.0e-7) {
            err = -999;
            break;
        }
        for (j = i; j < n; j++)
            *((a+i*n)+j) /= x;
        for (j = 0; j < 3; j++)
            *((b+i*n)+j) /= x;

        for (k = 0; k < 3; k++) {
            if (k == i) continue;
            x = *((a+k*n)+i);
            for (j = i; j < 3; j++)
                *((a+k*n)+j) -= *((a+i*n)+j)*x;
            for (j = 0; j < 3; j++)
                *((b+k*n)+j) -= *((b+i*n)+j)*x;
        }
    }
    return err;
}

void TFDIRContainer::executeTFDIR(int set, int phase)
{
    //need to check to see if this should be executed.
    //This should only run once per cycle (32 samples) of any given phase
    //check that it's at least 30 samples (15 time resolutions) more than the previous execution
    if (SmpNode.SmpSet[set].phase_t_ct[phase] - prevExecution.timeCt < 15) {
//		printf("shouldn't run tfdir with %d-%d-%lld, %d-%d-%lld", set, phase, SmpNode.SmpSet[set].phase_t_ct[phase], prevExecution.set, prevExecution.phase, prevExecution.timeCt);
        return;
    }

//	printf("running tfdir with %d-%d-%lld, %d-%d-%lld", set, phase, SmpNode.SmpSet[set].phase_t_ct[phase], prevExecution.sensor.set, prevExecution.sensor.phase, prevExecution.timeCt);

    prevExecution.sensor.set = set;
    prevExecution.sensor.phase = phase;
    prevExecution.timeCt = SmpNode.SmpSet[set].phase_t_ct[phase];

    if (executionCt == 0) {//first execution ever, set the initial ref sensor
        initialRefSensor.set = set;
        initialRefSensor.phase = phase;
    }

    calcPhasors(set, phase);

    float iw0,iz0,iw1,iz1,iw2,iz2;
    float vw0,vz0,vw1,vz1,vw2,vz2;
    double t_ref, ang_ref;

    if (executionCt>=0 && executionCt <= 40) {
        for(int set = 0; set < 1; set++) {
            int i = 0;
            //		for(int i=0; i<BUF_SIZE+1; i++) {
            //			t_ref = IcsNode.IcsSet[set].IcsSns[0].Ibuf[i].t.sec + (double) IcsNode.IcsSet[set].IcsSns[0].Ibuf[i].t.usec/1000000.0;
            //			ang_ref = t_ref / degreesPerSec;
            //			ang_ref -= (int)ang_ref/360;

            iw0 = XY2Mag(IcsNode.IcsSet[set].IcsSns[0].Ibuf[i].x, IcsNode.IcsSet[set].IcsSns[0].Ibuf[i].y);
            iz0 = XY2Ang(IcsNode.IcsSet[set].IcsSns[0].Ibuf[i].x, IcsNode.IcsSet[set].IcsSns[0].Ibuf[i].y);
            iw1 = XY2Mag(IcsNode.IcsSet[set].IcsSns[1].Ibuf[i].x, IcsNode.IcsSet[set].IcsSns[1].Ibuf[i].y);
            iz1 = XY2Ang(IcsNode.IcsSet[set].IcsSns[1].Ibuf[i].x, IcsNode.IcsSet[set].IcsSns[1].Ibuf[i].y);
            iw2 = XY2Mag(IcsNode.IcsSet[set].IcsSns[2].Ibuf[i].x, IcsNode.IcsSet[set].IcsSns[2].Ibuf[i].y);
            iz2 = XY2Ang(IcsNode.IcsSet[set].IcsSns[2].Ibuf[i].x, IcsNode.IcsSet[set].IcsSns[2].Ibuf[i].y);

            vw0 = XY2Mag(IcsNode.IcsSet[set].IcsSns[0].Vbuf[i].x, IcsNode.IcsSet[set].IcsSns[0].Vbuf[i].y);
            vz0 = XY2Ang(IcsNode.IcsSet[set].IcsSns[0].Vbuf[i].x, IcsNode.IcsSet[set].IcsSns[0].Vbuf[i].y);
            vw1 = XY2Mag(IcsNode.IcsSet[set].IcsSns[1].Vbuf[i].x, IcsNode.IcsSet[set].IcsSns[1].Vbuf[i].y);
            vz1 = XY2Ang(IcsNode.IcsSet[set].IcsSns[1].Vbuf[i].x, IcsNode.IcsSet[set].IcsSns[1].Vbuf[i].y);
            vw2 = XY2Mag(IcsNode.IcsSet[set].IcsSns[2].Vbuf[i].x, IcsNode.IcsSet[set].IcsSns[2].Vbuf[i].y);
            vz2 = XY2Ang(IcsNode.IcsSet[set].IcsSns[2].Vbuf[i].x, IcsNode.IcsSet[set].IcsSns[2].Vbuf[i].y);

//				if (executionCt==0) {
//					for(int tt=SV_CYC_SIZE-1; tt>=0; tt--) {
//						printf("DSP smp %d-%d, %d, %f", set, 1, tt, SmpNode.SmpSet[set].I[1][tt]);
//					}
//				}
//				printf("Set %d, Phasors I - %f/%f, %f/%f, %f/%f   V - %f/%f, %f/%f, %f/%f", set, iw0,iz0,iw1,iz1,iw2,iz2,vw0,vz0,vw1,vz1,vw2,vz2);
        }
    }

    if (executionCt > 2) {
        //don't do this until 3 cycles have been processed, as some of the phasors haven't been fully filled out
        SetAvg();
        SetFault();
        DetectFault();

        if (executionCt == 6)
            CalibVolt();
        //
        TakeAction();
    }

    executionCt++;
}

// Calculate Power Quantities and Prefault Average
void TFDIRContainer::SetAvg()		  // Set the V, I vectors from raw FFT vector buffers
{
    int   i, j, k, m, nref;
    float x, y, z;
    XY 	    *b0, v;
    ICS_SET *s;
    ICS_SNS *sn;
    PHASOR   s2;

// Set Instaneous and Average Values
    nref = 0;
    for (i = 0; i < MAX_SETS; i++) {
        s = &IcsNode.IcsSet[i];
        if (!s->Cfg->SrvFlg) continue;  // the ICS Set is not installed, skip the Set
// Shift and Clear the Instantaneous flags
        s->FltFlg0 = s->FltFlg;
        s->FltTyp0 = s->FltTyp;
        s->LineFlg0 = s->LineFlg;
        s->ChgFlg = 0;
        s->FltFlg = 0;
        s->FltTyp = 0;
        s->LineFlg = 0;
        s->IistFlg = 0;
        for (j = 0; j < MES_PHASES; j++) {
            sn = &s->IcsSns[j];
//			if(!sn->Cfg->UseCT) continue; 		  // Skip this phase for current process
            memset(&sn->Iist, 0, PHASOR_Size); // clear I Instant
            sn->FltTyp0 = sn->FltTyp;
            sn->FltTyp = 0;
            sn->IfRdyFlg = 0;
            b0 = &sn->Ibuf[0];
// Set Fault Current Instaneous Value
            sn->Iist.flg = b0->flg;
            sn->Iist.t = b0->t;
            if (sn->Iist.flg == DATA_NORMAL) { // SV data is good/valid, or a fault current
                s->LineFlgTimer = 0.0;         // Reset LineFlgTimer
                sn->Iist.mag = XY2Mag(b0->x, b0->y);
                sn->Iist.ang = XY2Ang(b0->x, b0->y);
                Setbit(s->LineFlg, sn->Phase); // Phase is On
                if (sn->Iist.mag >= 1.1*IcsNode.Cfg->Imin || sn->Ibuf[1].flg && sn->Iist.mag >= 0.9*IcsNode.Cfg->Imin) {
                    Setbit(s->IistFlg, sn->Phase); // Phase has good current
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
                memset (&v, 0, XY_Size);
//                v.flg = DATA_NO;
                sn->Ifa.t = sn->Iist.t;
                for (k = 0; k < FLT_BUF_SIZE; k++) {
                    m += sn->Ibuf[k].flg;		// count the fault flags over the last 3 cycles
                    v.x += sn->Ibuf[k].x/float (FLT_BUF_SIZE);
                    v.y += sn->Ibuf[k].y/float (FLT_BUF_SIZE);
                }
                if (m == DATA_NORMAL) {
                    sn->Ifa.mag = XY2Mag(v.x, v.y);
                    sn->Ifa.ang = XY2Ang(v.x, v.y);
//                    v.flg = sn->Iist.flg;
                    sn->IfRdyFlg = AVERAGE_READY;
                }
                else if (m == FLT_BUF_SIZE*DATA_FAULT) { // discard the first cycle of fault
                    v.x = 0.9*sn->Ibuf[0].x + 0.1*sn->Ibuf[1].x;
                    v.y = 0.9*sn->Ibuf[0].y + 0.1*sn->Ibuf[1].y;
                    sn->Ifa.mag = XY2Mag(v.x, v.y);
                    sn->Ifa.ang = XY2Ang(v.x, v.y);
//                    v.flg = DATA_FAULT;
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
                    sn->Ifa.mag = XY2Mag(v.x, v.y);
                    sn->Ifa.ang = XY2Ang(v.x, v.y);
                }
                else {  // from fault to normal, set to instaneous current
                    sn->Ifa = sn->Iist;
//                    v.flg = DATA_NO;
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
            else if (sn->IaCnt <= AVG_BUF_SIZE) {
                sn->IaCnt = 0;
                sn->Ia.x = 0.0;
                sn->Ia.y = 0.0;
                sn->IaRdyFlg = AVERAGE_NO;
                sn->Ia.flg = DATA_NO;
            }

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
            if (sn->IaCnt > AVG_BUF_SIZE) {
                x += sn->Ibuf[0].x;
                y += sn->Ibuf[0].y;
                sn->Ifc.mag = XY2Mag(x, y);
                sn->Ifc.ang = XY2Ang(x, y);
                sn->Ifc.t   = sn->Iist.t;
                sn->Ifc.flg = sn->Iist.flg;
            }
            else {
                memset(&sn->Ifc, 0, PHASOR_Size);
                sn->Ifc.flg = DATA_NO;
            }

// Instaneous Voltage Average (up to the most recent 3 cycles)
            if(s->Cfg->VrtFlg || sn->Cfg->UseVT) { 		// VT in use for this phase
                b0 = &sn->Vbuf[0];
                if (b0->flg == DATA_NORMAL) {
                    x = XY2Mag(b0->x, b0->y);
//                    printf("Vist raw: %f, %f, %f", x, IcsNode.Cfg->Vmin, IcsNode.Cfg->Vmax);
                    if (x < IcsNode.Cfg->Vmin ||  x > IcsNode.Cfg->Vmax)
                        b0->flg = DATA_NO;
                }
                b0 = &sn->Vbuf[0];
                sn->Vist.t = b0->t;
                sn->Vist.mag = XY2Mag(b0->x, b0->y);
                sn->Vist.ang = XY2Ang(b0->x, b0->y);
                if (sn->Vist.mag >= IcsNode.Cfg->Vmin && b0->flg == DATA_NORMAL) {
                    sn->Vist.flg = DATA_NORMAL;
                }
                else {
                    memset(&sn->Vist, 0, XY_Size);
                    sn->Vist.flg = DATA_NO;
                }
//                printf("Vist calc: %d, %d, %f, %f, %f, %f", j, sn->Vbuf[0].flg, sn->Vbuf[0].x, sn->Vbuf[0].y, sn->Vist.mag, sn->Vist.ang);
// Prefault Voltage Average
                b0 = &sn->Vbuf[BUF_SIZE];
                if (b0->flg == DATA_NORMAL && sn->VaCnt > 0) {
                    sn->VaCnt--;
                    sn->Va.x -= b0->x;
                    sn->Va.y -= b0->y;
                }
                b0 = &sn->Vbuf[FLT_BUF_SIZE];
                if (b0->flg == DATA_NORMAL) {
                    x = XY2Mag(b0->x, b0->y);
                    if (x >= IcsNode.Cfg->Vmin &&  x <= IcsNode.Cfg->Vmax) {
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

                if (sn->Cfg->RefVT && sn->Vist.flg == DATA_NORMAL) {
                    sn->Vref = sn->Vist;
                    nref++;
                }
                else {
                    sn->Vref.flg = DATA_NO;
                }
            }
            memmove(&sn->Vbuf[1], &sn->Vbuf[0], BUF_SIZE*XY_Size);
            sn->Vbuf[0].flg = DATA_NO;
            memmove(&sn->Ibuf[1], &sn->Ibuf[0], BUF_SIZE*XY_Size);
            sn->Ibuf[0].flg = DATA_NO;
        }
// Check line Flag
        if (!s->LineFlg && s->LineFlgTimer < 0.7*IcsNode.Cfg->RtnTime) { // Line is no current on any phase
           s->LineFlgTimer += IcsNode.Cycle2Sec;  // Count timer for setting LineFlg dead
           s->LineFlg = PHASE_ABC;  // Fake the LineFlg until time up
        }
// Calculate Neutral Current
        if (s->IistFlg > PHASE_NO) { // any phase is of current
            abcxy2s0(&s->I0, &s->IcsSns[0].Ibuf[1], &s->IcsSns[1].Ibuf[1], &s->IcsSns[2].Ibuf[1]);
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
                        abc[k].mag = XY2Mag(s->IcsSns[k].Ibuf[1].x, s->IcsSns[k].Ibuf[1].y);
                        abc[k].ang = XY2Ang(s->IcsSns[k].Ibuf[1].x, s->IcsSns[k].Ibuf[1].y);
                    }
                    abc2s2(&s2, &abc[0], &abc[1], &abc[2]); // Phase-A's S2
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
/***
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
***/
                        y = fmod(s->I0.ang-s2.ang,360.0);
                        if (y > 180.0)
                            y -= 360.0;
                        else if (y < -180.0)
                            y += 360.0;
                        y = fabs(y);
//					abc2s2(&s2, &abc[1], &abc[2], &abc[0]); // Phase-B's S2
                        z = fmod(s->I0.ang-s2.ang+120.0,360.0);
                        if (z > 180.0)
                            z -= 360.0;
                        else if (y < -180.0)
                            z += 360.0;
                        z = fabs(z);
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
                        z = fmod(s->I0.ang-s2.ang-120.0,360.0);
                        if (z > 180.0)
                            z -= 360.0;
                        else if (y < -180.0)
                            z += 360.0;
                        z = fabs(z);
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
                    sn->Ifc.mag = XY2Mag(x, y);
                    sn->Ifc.ang = XY2Ang(x, y);
                }
            }
            else if(s->NtrFlg)
                s->NtrFlg = 0;
        }
    }
// Assign Node Vref
    memset(&IcsNode.Vref, 0 , PHASOR_Size);
    IcsNode.Vref.flg = DATA_NO;
    m = 99;
    for (i = 0; i < MAX_SETS; i++) {
        s = &IcsNode.IcsSet[i];
        if (!s->Cfg->SrvFlg || s->Cfg->VrtFlg || !s->LineFlg)  // the ICS Set is not installed or line dead, skip the set
            continue;
//Set Reference Voltage: only one active reference votlage is needed per node
        for (j = 0; j < MES_PHASES; j++) {
            sn = &s->IcsSns[j];
            if (!nref && sn->Va.flg == DATA_NORMAL && sn->Ibuf[0].flg == DATA_NORMAL &&
                    TimeDiff(&sn->Ibuf[0].t, &sn->Vref.t) < 0.5) { // no valid instantaneous V Phasor available, use Vavg
                    sn->Vref.mag = XY2Mag(sn->Va.x, sn->Va.y)/(float)sn->VaCnt;
                    sn->Vref.ang = XY2Ang(sn->Va.x, sn->Va.y);
                    sn->Vref.t = sn->Va.t;
                    sn->Vref.flg = DATA_NORMAL;
            }
            if(sn->Cfg->UseVT && sn->Cfg->RefVT > 0 && sn->Cfg->RefVT < m && sn->Vref.flg == DATA_NORMAL) {
                m = sn->Cfg->RefVT;				// RefVT is set with its highest priority, 1->10:H->L
                IcsNode.Vref = sn->Vref;
                if(sn->Phase == PHASE_B)
                    IcsNode.Vref.ang += 120.0;
                else if (sn->Phase == PHASE_C)
                    IcsNode.Vref.ang -= 120.0;
                IcsNode.Vref.flg = DATA_NORMAL;
            }
        }
    }
// Use Vref to set instaneous voltage for those phases without votlage sensors
    if (IcsNode.Vref.flg == DATA_NORMAL) {
        for (i = 0; i < MAX_SETS; i++) {
            s = &IcsNode.IcsSet[i];
            if (!s->Cfg->SrvFlg || !s->LineFlg)  // the ICS Set is not installed or line dead, skip the set
                continue;
            s->p = 0.0;
            s->q = 0.0;
            for (j = 0; j < MES_PHASES; j++) {
                sn = &s->IcsSns[j];
                if (!s->Cfg->VrtFlg && !sn->Cfg->UseVT) {
                    sn->Vist = IcsNode.Vref;
                    if (sn->Phase == PHASE_B)
                        sn->Vist.ang -= 120.0;
                    else if (sn->Phase == PHASE_C)
                        sn->Vist.ang += 120.0;
                    x = TimeDiff(&sn->Iist.t, &sn->Vist.t)*IcsNode.Ms2Degrees;
                    sn->Vist = PhasorShift(&sn->Vist, x);
                    sn->Vist.t = sn->Iist.t;
                }
                VI2PQ (&x, &y, &sn->Vist, &sn->Iist);
                sn->p = 0.6*sn->p + 0.4*x;
                sn->q = 0.6*sn->q + 0.4*y;
                s->p += sn->p;
                s->q += sn->q;
            }
            s->pf = s->p/XY2Mag(s->p, s->q);
        }
    }
}

// Set Faults
void TFDIRContainer::SetFault()		  // Set faults
{
    int      i, j, k;
    float    x;
    ICS_SET *s;
    ICS_SNS *sn;
    PHASOR 	 p, q;

    for (i = 0; i < MAX_SETS; i++) {
        s = &IcsNode.IcsSet[i];
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
            abc2s0(&p, &s->IcsSns[0].Ifc, &s->IcsSns[1].Ifc, &s->IcsSns[2].Ifc);
            abc2s2(&q, &s->IcsSns[0].Ifc, &s->IcsSns[1].Ifc, &s->IcsSns[2].Ifc);
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
                    abc2s0(&p, &s->IcsSns[0].Iist, &s->IcsSns[1].Iist, &s->IcsSns[2].Iist);
                    abc2s2(&q, &s->IcsSns[0].Iist, &s->IcsSns[1].Iist, &s->IcsSns[2].Iist);
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
void TFDIRContainer::DetectFault()		// Fault detection
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
        s = &IcsNode.IcsSet[i];
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
            }
        }
        else {	// line is hot
            hot++;
            if(!s->LineFlg0)  // line is from dead to hot
                firsthot++;
// Latch Instaneous Phase Fault
            if (!s->FltLch) { // No latched yet
                if(s->FltCnt > AVERAGE_READY) { // 3 cycles of fault captured
                    s->FltLch = s->FltFlg;
                    s->TypLch = FAULT_PHASE;
                    if (!IcsNode.LchFlg || IcsNode.LchTyp > FAULT_PHASE) {
                        IcsNode.LchFlg = 1;
                        IcsNode.LchTyp = FAULT_PHASE;
                    }
                    if (s->FltCnt > AVERAGE_READY)
                        s->IfTimer = 3.0*IcsNode.Cycle2Sec;
                    else
                        s->IfTimer = 0;
                    s->FltCnt = 0;
                    s->IfRtnTimer = 0.0;
                }
            }
            else { // Fault latched
                if (Isbitset(s->FltTyp, FAULT_PHASE)) {
                    s->IfRtnTimer = 0.0;
                    s->IfTimer += IcsNode.Cycle2Sec;
                    if (s->IfTimer >= IcsNode.Cfg->FltTime)  // fault too long
                        Setbit(IcsNode.Flt2Long, FAULT_PHASE);
                }
                else {
                    s->IfTimer = 0.0;
                    s->IfRtnTimer += IcsNode.Cycle2Sec;
                    if (s->IfRtnTimer >= IcsNode.Cfg->RtnTime) {
                        Setbit(IcsNode.FltLck, EVENT_MOMENTARY);
                        s->FltLch = 0;
                    }
                }
            }

// Latch Other faults
            if (!s->FltLch) {
// Latch Neutral Fault
                if (!s->NtrLch) { // No Neutral fault latched yet
                    if (Isbitset(s->FltTyp, FAULT_NEUTRAL)) {
                        if (!Isbitset(s->FltTyp0, FAULT_NEUTRAL)) { // initialize fault time counter
                            s->IfnTimer = IcsNode.Cycle2Sec;
                            s->IfnRtnTimer = 0.0;
                        }
                        else if (Isbitset(s->FltTyp0, FAULT_NEUTRAL)) { // Neutral fault lasted for two cycles
                            s->NtrLch = s->NtrFlg;
                            Setbit(s->TypLch, FAULT_NEUTRAL);
                            if (!IcsNode.LchFlg || IcsNode.LchTyp == FAULT_CHANGE) {
                                IcsNode.LchFlg = 1;
                                IcsNode.LchTyp = FAULT_NEUTRAL;
                            }
                        }
                    }
                }
                else { // Neutral fault latched
                    if (Isbitset(s->FltTyp, FAULT_NEUTRAL))
                        s->IfnTimer += IcsNode.Cycle2Sec;
                    else if (!Isbitset(s->FltTyp, FAULT_NEUTRAL) && Isbitset(s->FltTyp0, FAULT_NEUTRAL)) {
                        s->IfnRtnTimer = IcsNode.Cycle2Sec;
                        s->IfnTimer = 0.0;
                    }
                    else
                        s->IfnRtnTimer += IcsNode.Cycle2Sec;
                    if (s->IfnTimer > IcsNode.Cfg->FltTime) { // fault too long
                        Setbit(IcsNode.Flt2Long, FAULT_NEUTRAL);
                        s->IfnTimer = 0.0;
                    }
                    else if (s->IfnRtnTimer > IcsNode.Cfg->RtnTime) {
                        Setbit(IcsNode.FltLck, EVENT_MOMENTARY);
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
                                if (!IcsNode.LchFlg) {
                                    IcsNode.LchFlg = 1;
                                    IcsNode.LchTyp = FAULT_CHANGE;
                                }
                            }
                            s->IfcTimer = IcsNode.Cycle2Sec;
                        }
                    }
                    else {
                        s->IfcTimer += IcsNode.Cycle2Sec;  // Keep counting Delta change timer
                        if (s->IfcTimer > IcsNode.Cfg->FltTime) {
                            Setbit(IcsNode.FltLck, EVENT_MOMENTARY);
                            s->ChgLch = 0;
                            s->IfcTimer = 0.0;
                        }
                    }
                }
            }
        }
    }

// Latch fault current for all sets if a fault is detected in any set, any phase with any type
    if (IcsNode.LchFlg == 1) { // fault is detected in at least one ICS Set, set all ILch.
        k = (IcsNode.TrpCnt < EVT_SIZE) ? IcsNode.TrpCnt : EVT_SIZE-1;
        for (i = 0; i < MAX_SETS; i++) {
            s = &IcsNode.IcsSet[i];
            if (!s->Cfg->SrvFlg)  // the ICS Set is not installed or line dead, skip the set
                continue;
            evt = &s->Evt[k];
            memset(evt, 0, EVT_LOG_Size);
            evt->FltTyp = s->TypLch;
            evt->Vref = IcsNode.Vref;
            m = -1;
            for (j = 0; j < MES_PHASES; j++) {
//				if (s->IcsSns[j].Ifa.mag >= IcsNode.Cfg->Imin) {
                if (s->IcsSns[j].Iist.mag >= IcsNode.Cfg->Imin) {
                    m = j;
                    break;
                }
            }
            if (m < 0)
                continue;

//			x = TimeDiff(&evt->Vref.t, &s->IcsSns[m].Ifa.t); // + is Vref leading Ifa
            x = TimeDiff(&evt->Vref.t, &s->IcsSns[m].Iist.t); // + is Vref leading Ifa
            x *= IcsNode.Ms2Degrees;
            evt->Vref = PhasorShift(&evt->Vref, -x); // Set Vref angle vs. phase Ia
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

//                printf("Fault Detection: %d, %f, %f", j, evt->Vf[j].mag, evt->Vf[j].ang);
            }
            if(IcsNode.VoltSetNum < 4)	{	// Use Fault voltage Set's voltage, it is selected
                for (j = 0; j < MES_PHASES; j++) {
                    evt->Vf[j] = IcsNode.IcsSet[IcsNode.VoltSetNum].IcsSns[j].Vist;
                }
            }
// Calculate fault I0, V0 and I2, V2 and Zs0, Zs2.
            abc2s0(&evt->I0, &evt->If[0], &evt->If[1], &evt->If[2]);
            abc2s2(&evt->I2, &evt->If[0], &evt->If[1], &evt->If[2]);
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
                        vr = PhasorShift(&vr, x);
                        x = vr.ang;
                        if (x <= IcsNode.ToqAngLead && x >= IcsNode.ToqAngLag)
                            evt->FltDir[j] = FAULT_FORWARD;
                        else if (x >= 180.0+IcsNode.ToqAngLag || x <= IcsNode.ToqAngLead-180.0)
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
                vr = PhasorShift(&vr, x);
                x = vr.ang;
                if (x <= IcsNode.ToqAngLead && x >= IcsNode.ToqAngLag)
                    evt->FltDir[j] = FAULT_FORWARD;
                else if (x >= 180.0+IcsNode.ToqAngLag || x <= IcsNode.ToqAngLead-180.0)
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
                        vr = PhasorShift(&vr, x);
                        x = vr.ang;
                        if (x <= IcsNode.ToqAngLead && x >= IcsNode.ToqAngLag)
                            evt->FltDir[j] = FAULT_FORWARD;
                        else if (x >= 180.0+IcsNode.ToqAngLag || x <= IcsNode.ToqAngLead-180.0)
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

//        OldCalDist(IcsNode.LchFlg); // Calculate fault distance if fault is in the sensor's side (reversed direction)
        CalDist(IcsNode.LchFlg);
        IcsNode.LchFlg = 2;	// Prepare the trip type to wait for a fault trip
    }
    else if (IcsNode.LchFlg > 1)
        CalDist(IcsNode.LchFlg);

// Report Delta Change for fault detection at Master
/***
	if (!IcsNode.LchFlg && IcsNode.RptChg) {
		if (IcsNode.RptChgTimer <= IcsNode.Cycle2Sec) {
			for (i = 0; i < MAX_SETS; i++) {
				s = &IcsNode.IcsSet[i];
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
            IcsNode.DnTimer = 0.0;
            if (IcsNode.TrpCnt > IcsNode.RclCnt)
                IcsNode.RclCnt++;
        }
        IcsNode.UpTimer += IcsNode.Cycle2Sec;
        if (IcsNode.UpTimer >= IcsNode.Cfg->HotTime)  // Line is successfully energized no trip again
            if (IcsNode.TrpCnt > 0)  // Reclose successful
                IcsNode.FltLck = EVENT_TEMPORARY;
    }
    else if (dead == MAX_SETS) { // all lines dead
        if (firstdead > 0) {
            IcsNode.TrpCnt++;
/*** For skipping the no-fault trips during one side reclosing
            m = 1;
            if(IcsNode.TrpCnt >= 2) {
                m = 0;
                for (i = 0; i < MAX_SETS; i++) {
                    s = &IcsNode.IcsSet[i];
                    if (!s->Cfg->SrvFlg)
                        continue;
                    if (s->FltZone > 0) {
                        m++;
                        break;
                    }
                    else {
                        if (s->Evt[0].FltZone == 1) {
                            m++;
                            break;
                        }
                        else {
                            for (j = 1; j < IcsNode.TrpCnt; j++) {
                                if (s->Evt[j].FltFlg) {
                                    m++;
                                    break;
                                }
                            }
                        }
                    }
                }
            }
            if (!m)
                IcsNode.TrpCnt--;
***/

            if(IcsNode.Cfg->AutoSwitchOpen) { // Auto open switch for the faulted zone
                for (i = 0; i < MAX_SETS; i++) {
                    s = &IcsNode.IcsSet[i];
                    if (!s->Cfg->SrvFlg)
                        continue;
                    k = IcsNode.TrpCnt - 1;
                    if (k >= 0 && s->Evt[k].FltDirSum == FAULT_BACKWARD) {
                        if (s->Evt[k].FltZone > 0 && (s->Evt[k].FltZone < s->FltZone || s->FltZone == 0))
                            s->FltZone = s->Evt[k].FltZone;
                    }
                    if ((k > 0 && s->Evt[0].FltDirSum == FAULT_BACKWARD) ||
                        (k >= 1 && s->Evt[1].FltDirSum == FAULT_BACKWARD) ||
                        (k >= 2 && s->Evt[2].FltDirSum == FAULT_BACKWARD)) {
                        if (s->Cfg->UseCfgZone) {
                            if ((s->Cfg->UseCfgZone == 1 && k >= s->Cfg->CfgZone) ||
                                (s->Cfg->UseCfgZone == 2 && k == s->Cfg->CfgZone+s->FltZone) ||
                                (s->Cfg->UseCfgZone == 3 && s->FltZone == 1 && k == 1) ||
                                (s->Cfg->UseCfgZone == 3 && s->FltZone == 2 && k == s->Cfg->CfgZone+2)) {
                                if (s->Cfg->UseCfgZone == 1)
                                    s->FltZone = s->Cfg->CfgZone;
                                else if (s->Cfg->UseCfgZone == 2 ||(s->Cfg->UseCfgZone == 3 && s->FltZone == 2))
                                    s->FltZone += s->Cfg->CfgZone;
                                s->OpenSw = (s->OpenSw == 0) ? 1 : s->OpenSw;
                                if (s->Cfg->SetTyp == SET_TYPE_TAP && !s->Cfg->SwitchEnabled) { // Tap switch is not available to open
                                    stat = s->FltZone;
                                    for (j = 0; j < MAX_SETS; j++) {
                                        s = &IcsNode.IcsSet[j];
                                        if (!s->Cfg->SrvFlg)
                                            continue;
                                        if (s->Cfg->SetTyp == SET_TYPE_LINE) {
                                            s->FltZone = (s->OpenSw == 0) ? stat : s->FltZone;
                                            s->OpenSw = (s->OpenSw == 0) ? 1 : s->OpenSw;
                                        }
                                    }
                                }
                            }
                        }
                        else if ((s->FltZone > 0 && s->FltZone <= 2 && IcsNode.TrpCnt == s->FltZone+1) ||
                                 (IcsNode.TrpCnt == 1 && s->FltZone == 1 &&
                                  IcsNode.DnTimer >= IcsNode.Cfg->RclWaitTime)) {
                            s->OpenSw = (s->OpenSw == 0) ? 1 : s->OpenSw;
                            if (s->Cfg->SetTyp == SET_TYPE_TAP && !s->Cfg->SwitchEnabled) { // Tap switch is not available to open
                                stat = s->FltZone;
                                for (j = 0; j < MAX_SETS; j++) {
                                    s = &IcsNode.IcsSet[j];
                                    if (!s->Cfg->SrvFlg)
                                        continue;
                                    if (s->Cfg->SetTyp == SET_TYPE_LINE) {
                                        s->FltZone = (s->OpenSw == 0) ? stat : s->FltZone;
                                        s->OpenSw = (s->OpenSw == 0) ? 1 : s->OpenSw;
                                    }
                                }
                            }
                        }
                    }
                }
            }
// Clean Latch flags for each set
            IcsNode.UpTimer = 0.0;
            for (i = 0; i < MAX_SETS; i++) {
                s = &IcsNode.IcsSet[i];
                s->ChgLch = 0;
                s->FltLch = 0;
                s->NtrLch = 0;
                s->TypLch = 0;
                s->FltCnt = 0;
                s->NtrCnt = 0;
                s->ChgCnt = 0;
            }
            IcsNode.LchFlg = 0;
        }

        IcsNode.DnTimer += IcsNode.Cycle2Sec;    // count dead time in seconds, add one cycle time
        if (IcsNode.TrpCnt > 0) {
            if (IcsNode.RclCnt >= IcsNode.Cfg->MaxRclNum || // Max rcls number reached
                IcsNode.DnTimer > IcsNode.Cfg->DeadTime ||
                IcsNode.DnTimer > IcsNode.Cfg->RclWaitTime) // Line is permernently dead and locked
                IcsNode.FltLck = EVENT_PERMANENT;
        }
// Clear SV buffers and Prefault averages
        if (firstdead) {
            for (i = 0; i < MAX_SETS; i++) {
                s = &IcsNode.IcsSet[i];
                for (j = 0; j < MES_PHASES; j++) {
                    sn = &s->IcsSns[j];
                    memset(&sn->Ia, 0, XY_Size);
                    sn->Ia.flg = DATA_NO;
                    sn->IaCnt = 0;
                    memset (sn->Ibuf, 0, (BUF_SIZE+1)*XY_Size);
                    memset (sn->Vbuf, 0, (BUF_SIZE+1)*XY_Size);
                    for (k = 0; k <= BUF_SIZE; k++) {
                        sn->Ibuf[k].flg = DATA_NO;
                        sn->Vbuf[k].flg = DATA_NO;
                    }
                }
            }
        }
    }
}

// Newly updated Distance calculation with Negative Sequence Compensation
void TFDIRContainer::CalDist(FLAG LchFlg)
{
//	printf("Calc Dist");
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
        s = &IcsNode.IcsSet[i];
        if (!s->Cfg->SrvFlg)  // the ICS Set is not in service, skip the set
            continue;
        evt = &s->Evt[IcsNode.TrpCnt];
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
    }

    s = (k >= 0)? & IcsNode.IcsSet[k] : NULL;
    if(s) {
        if(LchFlg == 1) {
            evt = &s->Evt[IcsNode.TrpCnt];
            localEvt = s->Evt[IcsNode.TrpCnt];
        }
        else if (LchFlg > 1) {
            k = 0;
            evt = &localEvt;
            if ((Isbitset(evt->FltFlg, PHASE_A) && (evt->If[0].mag + s->Cfg->Ifdb/2.0 <= 1.03*s->IcsSns[0].Iist.mag)) ||
                (Isbitset(evt->FltFlg, PHASE_B) && (evt->If[1].mag + s->Cfg->Ifdb/2.0 <= 1.03*s->IcsSns[1].Iist.mag)) ||
                (Isbitset(evt->FltFlg, PHASE_C) && (evt->If[2].mag + s->Cfg->Ifdb/2.0 <= 1.03*s->IcsSns[2].Iist.mag)))
                k++;
            abc2s0(&VI0, &s->IcsSns[0].Iist, &s->IcsSns[1].Iist, &s->IcsSns[2].Iist); // Calculate fault I0

            if (evt->I0.mag + s->Cfg->Ifndb/2.0 <= 1.05*VI0.mag)
                k++;
            if (k >= 1)
                count++;
            if (count >= 2) {
                for (j = 0; j < MES_PHASES; j++) {
                    evt->If[j] = s->IcsSns[j].Iist;
                    evt->Vf[j] = s->IcsSns[j].Vist;
                }
                if(IcsNode.VoltSetNum < 4)	{	// Use Fault voltage Set's voltage, it is selected
                    for (j = 0; j < MES_PHASES; j++) {
                        evt->Vf[j] = IcsNode.IcsSet[IcsNode.VoltSetNum].IcsSns[j].Vist;
                    }
                }
                evt->I0 = VI0;
                count = 0;		// clear counter
            }
            else		// no need to do further calculation
                return;
        }

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
// Conventional T-Method Algorithm
            a = Zls2.r * Zls2.r + Zls2.x * Zls2.x;    // Zl1^2 (Zl2= Zl1)

            W1 = VectSub(&Zls0, &Zls2);
            r = W1.r * Zls2.r + W1.x * Zls2.x;
            x = -W1.r * Zls2.x + W1.x * Zls2.r;
            W1.r = r / a;                        // = k/3
            W1.x = x / a;
            I0.r = MagAng2X(evt->I0.mag, evt->I0.ang);
            I0.x = MagAng2Y(evt->I0.mag, evt->I0.ang);
            W2 = VectX2(&W1, &I0);            // = k*3I0

            W1.r = MagAng2X(evt->Vf[m].mag, evt->Vf[m].ang);
            W1.x = MagAng2Y(evt->Vf[m].mag, evt->Vf[m].ang);
//            printf("cfg Vf: %d, %f, %f", m, evt->Vf[m].mag, evt->Vf[m].ang);

            W2.r += MagAng2X(evt->If[m].mag, evt->If[m].ang);    // = Is + k*3I0
            W2.x += MagAng2Y(evt->If[m].mag, evt->If[m].ang);
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
            x = W1.x * I0.r - W1.r * I0.x;
            W3 = I0;
            W3.x = -I0.x;
            Z2 = VectX3(&Zls2, &W2, &W3);
            a = -x / Z2.x;
            r = a * s->Cfg->LineLength;
            printf("Modified T-Method: a =%6.4f, Dist =%6.4f\n", a, r);
// Fan Modified method by using I2 rather than I0 for compensation
            RX W0, Y, V2, I2;
            float b, c;
            // Calculate Load Impedance
            Y.r = 0.0;
            Y.x = 0.0;
            for (k = 0; k < 3; k++) {
                if (k == m) {
                    continue;
                }
                b = (evt->Vf[k].ang - evt->If[k].ang + 180.0);
                c = 0.0005 * evt->If[k].mag / (evt->Vf[k].mag + 1.0e-5);
                Y.r += MagAng2X(c, b);
                Y.x -= MagAng2Y(c, b);
            }
            abc2s2(&VI0, &evt->Vf[0], &evt->Vf[1], &evt->Vf[2]);  // V2
            VI0.mag *= 1000.0;
            VI0.ang += m * 120.0;
            V2.r = MagAng2X(VI0.mag, VI0.ang);
            V2.x = MagAng2Y(VI0.mag, VI0.ang);
            W0 = VectX2(&V2, &Y); //Load I2
            W0.r *= 0.7; // 1/2 Load I2
            W0.x *= 0.7;
            abc2s2(&VI0, &evt->If[0], &evt->If[1], &evt->If[2]);  // I2
            VI0.ang += m * 120.0 + 180.0;
            I2.r = MagAng2X(VI0.mag, VI0.ang);
            I2.x = MagAng2Y(VI0.mag, VI0.ang);
            I0 = VectSub(&I2, &W0); // Fault I2
// Fan Modified T-Method
            x = W1.x * I0.r - W1.r * I0.x;
            W3 = I0;
            W3.x = -I0.x;
            Z2 = VectX3(&Zls2, &W2, &W3);
            b = -x / Z2.x;
            r = b * s->Cfg->LineLength;
            printf("Fan Modified T-Method: b =%6.4f, Dist =%6.4f\n", b, r);

            if (b < 0.0 && a > 0.0 && a < 1.15) {
                if (b > -0.2 * a && a > -b)
                    b = 0.5 * (a + b);
                else
                    b = 0.4 * a;
                r = b * s->Cfg->LineLength;
                printf("Fan-Method Correction1: b =%6.4f, Dist =%6.4f \n", b, r);
            }
            else if ((b > 1.1*a || (a > 1.0 && b > 1.0)) && a > 0.0) {
                a = (a > 1.0) ? 1.0 : a;
                b = 0.7 * a;
                r = b * s->Cfg->LineLength;
                printf("Fan-Method Correction2: b =%6.4f, Dist =%6.4f \n", b, r);
            }
            a = b;
            if (evt->Vf[m].mag <= IcsNode.Cfg->Vmin && evt->If[m].mag >= IcsNode.Cfg->Imin) {
                a = 0.01;                       // Must be a very close and very low impedance fault
                r = a * s->Cfg->LineLength;
                printf("Fault too close, Set: a =%6.4f, Dist =%6.4f", a, r);
            }
        }
        else if (evt->FltFlg == PHASE_AB || evt->FltFlg == PHASE_BC || evt->FltFlg == PHASE_CA || evt->FltFlg == PHASE_ABC) {

            if (evt->FltFlg == PHASE_AB || evt->FltFlg == PHASE_ABC )
                m = 0;
            else if (evt->FltFlg == PHASE_BC)
                m = 1;
            else if (evt->FltFlg == PHASE_CA)
                m = 2;
            n = (m < 2) ? m + 1 : 0;

            W1.r = 1000.0*(MagAng2X(evt->Vf[m].mag, evt->Vf[m].ang) - MagAng2X(evt->Vf[n].mag, evt->Vf[n].ang));
            W1.x = 1000.0*(MagAng2Y(evt->Vf[m].mag, evt->Vf[m].ang) - MagAng2Y(evt->Vf[n].mag, evt->Vf[n].ang));
            W2.r = MagAng2X(evt->If[m].mag, evt->If[m].ang) - MagAng2X(evt->If[n].mag, evt->If[n].ang);
            W2.x = -MagAng2Y(evt->If[m].mag, evt->If[m].ang) + MagAng2Y(evt->If[n].mag, evt->If[n].ang);
            x = W2.r*W2.r + W2.x*W2.x;
            W2 = VectX2(&W1, &W2);
            a = W2.x/(x*Zls2.x);     // For wo phase fault, the P2P curretn angle is independent of flow direction
            r = a*s->Cfg->LineLength;
            printf ("Radial(2-3Phase Fault): a =%6.4f, Dist =%6.4f", a, r);
            x = XY2Mag(W1.r, W1.x);     // Phase-Phase voltage
            if (x <= IcsNode.Cfg->Vmin && evt->If[m].mag >= IcsNode.Cfg->Imin && evt->If[n].mag >= IcsNode.Cfg->Imin) {
                a = 0.01;        // Must be a very close and very low impedance fault)
                r = a*s->Cfg->LineLength;
                printf ("Fault too close, Set: a =%6.4f, Dist =%6.4f", a, r);
            }
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
            evt->FltZone = 7;   // Unknown Zone
//#ifdef PC_DEBUG
        printf("Fault Set: %d, Phase:%d, Zone:%d \n\n", s->Cfg->SeqNum, m+1, evt->FltZone);
//#endif
        if (a >= 0.0 && a <= 1.0) {
            if(evt == &localEvt) { // choice the best distance results between the tripEvt log and localEvt log
                if(evt->FltZone > 0 && evt->FltZone <= s->Evt[IcsNode.TrpCnt].FltZone) {
                    s->Evt[IcsNode.TrpCnt].FltDist = localEvt.FltDist;
                    s->Evt[IcsNode.TrpCnt].FltZone = localEvt.FltZone;
                }
            }

            for (i = 0; i < MAX_SETS; i++) { // Set the fault distance to the Forward Direction also
                if (!IcsNode.IcsSet[i].Cfg->SrvFlg || s == &IcsNode.IcsSet[i])  // the same or no-service ICS Set, skip
                    continue;
                evt = &IcsNode.IcsSet[i].Evt[IcsNode.TrpCnt];
                if (IcsNode.IcsSet[i].Cfg->SetTyp == SET_TYPE_LINE) {
                    evt->FltDist = s->Evt[IcsNode.TrpCnt].FltDist;
                    evt->FltZone = s->Evt[IcsNode.TrpCnt].FltZone;
                }
            }
        }
    }
}

int TFDIRContainer::EvtTrigger ()
{
    int i, k = 0;

    for (i = 0; i < MAX_SETS; i++) {
        if (!IcsNode.IcsSet[i].Cfg->SrvFlg || !IcsNode.IcsSet[i].LineFlg)
            continue;
        k += IcsNode.IcsSet[i].ChgLch + IcsNode.IcsSet[i].FltLch + IcsNode.IcsSet[i].NtrLch;
        if (k > 0)
            break;
    }
    return k;
}

int TFDIRContainer::TakeAction ()  // Report fault and normal V, I, P, Q
{
    int i, j, k, k1, k2;
    int Error = 0;
    static FLAG trpcnt;

    if (!IcsNode.EvtTrigger) {
        IcsNode.EvtTrigger = EvtTrigger();
        if (IcsNode.EvtTrigger) {
            trpcnt = IcsNode.TrpCnt;
#ifdef PC_DEBUG
            printf ("###############################\n");
            printf ("DFR Trigger: ON (Prior to Faulted Trip:%d) \n", IcsNode.TrpCnt+1);
            printf ("###############################\n\n");
#else
//??? Call DFR function which will be activated to record 10 - 15 sycles before the trigger time
//	and 50 - 45 cycles following the trigger, total no less than 60 cycles.
//	Once it is activated, it will not check the trigger status untill it completes the recording.
//	This call is just to trigger another function to start working, not allowed to hold up this caling process
#endif
        }
    }
    else if (IcsNode.TrpCnt > trpcnt) {
        trpcnt = 0;
        IcsNode.EvtTrigger = 0;
    }

#ifdef PC_DEBUG
    if (IcsNode.TrpCnt > 0 ) { // Issue control to the selected switch(es) to open
        k1 = -1;
        k2 = -1;
        for (i = 0; i < MAX_SETS; i++) {
            if (!IcsNode.IcsSet[i].Cfg->SrvFlg || IcsNode.IcsSet[i].LineFlg)
                continue;
            if (IcsNode.IcsSet[i].FltZone && IcsNode.IcsSet[i].Cfg->SetTyp && !IcsNode.IcsSet[i].Cfg->SwitchEnabled)
                // Fault on Tap lateral, SetSwitch not controllable
                k1 = i;
            if (IcsNode.IcsSet[i].OpenSw == 1 && IcsNode.IcsSet[i].Cfg->SwitchEnabled)
                k2 = i;
        }
        if (k1 < 0 && k2 >= 0) { // Only open one Switch
                k = 0;
                printf("-----------------------------------------------------------------\n");
                for (j = 0; j < IcsNode.TrpCnt; j++) {
                    printf("TripCt %d Fault: Set=%d, Zone=%d, Dir=%d, Dist=%f \n", j + 1, k2 + 1,
                           IcsNode.IcsSet[k2].Evt[j].FltZone, IcsNode.IcsSet[k2].Evt[j].FltDirSum,
                           IcsNode.IcsSet[k2].Evt[j].FltDist);
                    if (IcsNode.IcsSet[k2].FltZone == IcsNode.IcsSet[k2].Evt[j].FltZone) {
                        k = j;
                    }
                }
                printf("Resultant Fault: Set=%d, Zone=%d, Dir=%d, Dist=%f \n", k2 + 1, IcsNode.IcsSet[k2].FltZone,
                       IcsNode.IcsSet[k2].Evt[k].FltDirSum, IcsNode.IcsSet[k2].Evt[k].FltDist);
                printf("Command: Open Switch of Set %1d \n", k2 + 1);
                printf("-----------------------------------------------------------------\n");
//  tfdirMsgHandler.SendFaultData(i,0,IcsNode.IcsSet[i].Evt[resultantEvt].FltDirSum,IcsNode.IcsSet[i].Evt[resultantEvt].FltDist,IcsNode.IcsSet[i].FltZone);
                IcsNode.IcsSet[k2].OpenSw++; // mark as proceesed status
        }
        else if (k1 >= 0 && k2 >= 0) { // Tap lateral fault but open two line switches
            for (i = 0; i < MAX_SETS; i++) {
                if (!IcsNode.IcsSet[i].Cfg->SrvFlg || IcsNode.IcsSet[i].LineFlg)
                    continue;
                if (IcsNode.IcsSet[i].OpenSw == 1 && IcsNode.IcsSet[i].Cfg->SwitchEnabled) {
                    k = 0;
                    printf("-----------------------------------------------------------------\n");
                    for (j = 0; j < IcsNode.TrpCnt; j++) {
                        printf("TripCt %d Fault: Set=%d, Zone=%d, Dir=%d, Dist=%f \n", j + 1, k1 + 1,
                               IcsNode.IcsSet[k1].Evt[j].FltZone, IcsNode.IcsSet[k1].Evt[j].FltDirSum,
                               IcsNode.IcsSet[k1].Evt[j].FltDist);
                        if (IcsNode.IcsSet[k1].FltZone == IcsNode.IcsSet[k1].Evt[j].FltZone) {
                            k = j;
                        }
                    }
                    printf("Resultant Fault: Set=%d, Zone=%d, Dir=%d, Dist=%f \n", k1 + 1, IcsNode.IcsSet[i].FltZone,
                           IcsNode.IcsSet[k1].Evt[k].FltDirSum, IcsNode.IcsSet[k1].Evt[k].FltDist);
                    printf("Command: Open Switch of Set %1d \n", i + 1);
                    printf("-----------------------------------------------------------------\n");
//  tfdirMsgHandler.SendFaultData(i,0,IcsNode.IcsSet[i].Evt[resultantEvt].FltDirSum,IcsNode.IcsSet[i].Evt[resultantEvt].FltDist,IcsNode.IcsSet[i].FltZone);
                    IcsNode.IcsSet[i].OpenSw++; // mark as proceesed status
                }
            }
        }
    }
#endif

    if (IcsNode.FltLck) {// Process and report locked fault

#ifdef PC_DEBUG
        printf ("Fault Typ:  1->PHASE,   2->Neutral,  4->DeltaChange \n");
		printf ("Fault Pha:  1->A, 2->B, 3->AB, 4->C, 5->CA, 6->BC, 7->ABC \n");
		printf ("Fault D(x): 0->No, 1->Forward, 2->Backward, 3->Unknown \n");
#endif

        if (Isbitset(IcsNode.FltLck, EVENT_PERMANENT)) { // Permenent fault, line dead
            if (IcsNode.Cfg->RptPermFlt) { // Report fault with breaker lock
#ifndef PC_DEBUG
                for (j = 0; j < IcsNode.TrpCnt; j++) {
                    for (i = 0; i < MAX_SETS; i++) {
                        if (!IcsNode.IcsSet[i].Cfg->SrvFlg)
                            continue;
                        IcsNode.IcsSet[i].Evt[j].EvtTyp = EVENT_PERMANENT;

//???					  ReportFault(i+1, j, IcsNode.IcsSet[i].Evt[j]);
                    }
                }
#else
                for (j = 0; j < IcsNode.TrpCnt; j++) {
				  printf ("Trip: %d\n", j+1);
				  for (i = 0; i < MAX_SETS; i++) {
					if (!IcsNode.IcsSet[i].Cfg->SrvFlg)
						continue;
					IcsNode.IcsSet[i].Evt[j].EvtTyp = EVENT_PERMANENT;
//					ReportFault(i+1, j, IcsNode.IcsSet[i].Evt[j]);

					printf ("ICS Set: %d \n", i+1);

					printf ("PermFault: Typ:%1d, Pha:%1d, D(A):%1d  %8.1f/%7.2f D(B):%1d: %8.1f/%7.2f D(C):%1d %8.1f/%7.2f ## %8.1f/%7.2f\n",
					IcsNode.IcsSet[i].Evt[j].FltTyp,
					IcsNode.IcsSet[i].Evt[j].FltFlg, IcsNode.IcsSet[i].Evt[j].FltDir[0],
					IcsNode.IcsSet[i].Evt[j].If[0].mag, IcsNode.IcsSet[i].Evt[j].If[0].ang,
					IcsNode.IcsSet[i].Evt[j].FltDir[1],
					IcsNode.IcsSet[i].Evt[j].If[1].mag, IcsNode.IcsSet[i].Evt[j].If[1].ang,
					IcsNode.IcsSet[i].Evt[j].FltDir[2],
					IcsNode.IcsSet[i].Evt[j].If[2].mag, IcsNode.IcsSet[i].Evt[j].If[2].ang,
					IcsNode.IcsSet[i].Evt[j].Vref.mag, IcsNode.IcsSet[i].Evt[j].Vref.ang);
					printf ("I0:%8.3f/%6.3f \n", IcsNode.IcsSet[i].Evt[j].I0.mag,
							IcsNode.IcsSet[i].Evt[j].I0.ang);
					printf ("I2:%8.3f/%6.3f \n", IcsNode.IcsSet[i].Evt[j].I2.mag,
							IcsNode.IcsSet[i].Evt[j].I2.ang);
//					printf ("I0:%8.3f, V0:%6.3f \n", IcsNode.IcsSet[i].Evt[j].I0.mag,
//							IcsNode.IcsSet[i].Evt[j].V0.mag);
//					printf ("I2:%8.3f, V2:%6.3f \n", IcsNode.IcsSet[i].Evt[j].I2.mag,
//							IcsNode.IcsSet[i].Evt[j].V2.mag);
//					printf ("Zs0.r:%6.3f, Zs0.x:%6.3f\n", IcsNode.IcsSet[i].Evt[j].Zs0.r,
//							IcsNode.IcsSet[i].Evt[j].Zs0.x);
//					printf ("Zs2.r:%6.3f, Zs2.x:%6.3f\n", IcsNode.IcsSet[i].Evt[j].Zs2.r,
//							IcsNode.IcsSet[i].Evt[j].Zs2.x);
					printf ("Fault Dirrection:%1d \n", IcsNode.IcsSet[i].Evt[j].FltDirSum);
					printf ("Fault Distance:%8.3f \n", IcsNode.IcsSet[i].Evt[j].FltDist);
					printf ("Fault Zone: %1d \n\n", IcsNode.IcsSet[i].Evt[j].FltZone);
				 }
			  }
#endif
            }
        }
        else if (Isbitset(IcsNode.FltLck, EVENT_TEMPORARY)) { // Temporary fault, reclose successful
            if (IcsNode.Cfg->RptTempFlt) { // Report fault with a breaker trip
#ifndef PC_DEBUG
                for (j = 0; j < IcsNode.TrpCnt; j++) {
                    for (i = 0; i < MAX_SETS; i++) {
                        if (!IcsNode.IcsSet[i].Cfg->SrvFlg)
                            continue;
                        IcsNode.IcsSet[i].Evt[j].EvtTyp = EVENT_TEMPORARY;

//???					  ReportFault(i+1, j, IcsNode.IcsSet[i].Evt[j]);
                    }
                }
#else
                for (j = 0; j < IcsNode.TrpCnt; j++) {
				printf ("Trip: %d\n", j+1);
				for (i = 0; i < MAX_SETS; i++) {
					if (!IcsNode.IcsSet[i].Cfg->SrvFlg)
						continue;
					IcsNode.IcsSet[i].Evt[j].EvtTyp = EVENT_TEMPORARY;
//					ReportFault(i+1, j, IcsNode.IcsSet[i].Evt[j]);

					printf ("ICS Set: %d \n", i+1);
					printf ("TempFault: Typ:%1d, Pha:%1d, D(A):%1d  %8.1f/%7.2f D(B):%1d %8.1f/%7.2f D(C):%1d %8.1f/%7.2f ## %8.1f/%7.2f\n",
					IcsNode.IcsSet[i].Evt[j].FltTyp,
					IcsNode.IcsSet[i].Evt[j].FltFlg, IcsNode.IcsSet[i].Evt[j].FltDir[0],
					IcsNode.IcsSet[i].Evt[j].If[0].mag, IcsNode.IcsSet[i].Evt[j].If[0].ang,
					IcsNode.IcsSet[i].Evt[j].FltDir[1],
					IcsNode.IcsSet[i].Evt[j].If[1].mag, IcsNode.IcsSet[i].Evt[j].If[1].ang,
					IcsNode.IcsSet[i].Evt[j].FltDir[2],
					IcsNode.IcsSet[i].Evt[j].If[2].mag, IcsNode.IcsSet[i].Evt[j].If[2].ang,
					IcsNode.IcsSet[i].Evt[j].Vref.mag, IcsNode.IcsSet[i].Evt[j].Vref.ang);
					printf ("I0:%8.3f/%6.3f \n", IcsNode.IcsSet[i].Evt[j].I0.mag,
							IcsNode.IcsSet[i].Evt[j].I0.ang);
					printf ("I2:%8.3f/%6.3f \n", IcsNode.IcsSet[i].Evt[j].I2.mag,
							IcsNode.IcsSet[i].Evt[j].I2.ang);
//					printf ("I0:%8.3f, V0:%6.3f \n", IcsNode.IcsSet[i].Evt[j].I0.mag,
//							IcsNode.IcsSet[i].Evt[j].V0.mag);
//					printf ("I2:%8.3f, V2:%6.3f \n\n", IcsNode.IcsSet[i].Evt[j].I2.mag,
//							IcsNode.IcsSet[i].Evt[j].V2.mag);
					printf ("Fault Direction:%1d \n", IcsNode.IcsSet[i].Evt[j].FltDirSum);
					printf ("Fault Distance:%8.3f \n", IcsNode.IcsSet[i].Evt[j].FltDist);
					printf ("Fault Zone: %1d \n\n", IcsNode.IcsSet[i].Evt[j].FltZone);
				}
			  }
#endif
            }
        }
        else if (Isbitset(IcsNode.FltLck, EVENT_MOMENTARY)) { // Momentary fault, return to normal
            if (IcsNode.Cfg->RptMmtEvt) { // Report fault without causing a trip
#ifndef PC_DEBUG
                for (j = 0; j < 1; j++) {
                    for (i = 0; i < MAX_SETS; i++) {
                        if (!IcsNode.IcsSet[i].Cfg->SrvFlg)
                            continue;

//???					ReportFault(i+1, j, IcsNode.IcsSet[i].Evt[j]);
                    }
                }

#else
                for (j = 0; j < 1; j++) {
				printf ("Trip: %d\n", j+1);
				for (i = 0; i < MAX_SETS; i++) {
					if (!IcsNode.IcsSet[i].Cfg->SrvFlg)
						continue;
					IcsNode.IcsSet[i].Evt[j].EvtTyp = EVENT_MOMENTARY;
//					ReportFault(i+1, j, IcsNode.IcsSet[i].Evt[j]);

					printf ("ICS Set: %d \n", i+1);
					printf ("MomentFault: Type:%1d, Pha:%1d, D(A):%1d  %8.1f/%7.2f D(B):%1d: %8.1f/%7.2f D(C):%1d: %8.1f/%7.2f ## %8.1f/%7.2f\n",
					IcsNode.IcsSet[i].Evt[j].FltTyp,
					IcsNode.IcsSet[i].Evt[j].FltFlg, IcsNode.IcsSet[i].Evt[j].FltDir[0],
					IcsNode.IcsSet[i].Evt[j].If[0].mag, IcsNode.IcsSet[i].Evt[j].If[0].ang,
					IcsNode.IcsSet[i].Evt[j].FltDir[1],
					IcsNode.IcsSet[i].Evt[j].If[1].mag, IcsNode.IcsSet[i].Evt[j].If[1].ang,
					IcsNode.IcsSet[i].Evt[j].FltDir[2],
					IcsNode.IcsSet[i].Evt[j].If[2].mag, IcsNode.IcsSet[i].Evt[j].If[2].ang,
					IcsNode.IcsSet[i].Evt[j].Vref.mag, IcsNode.IcsSet[i].Evt[j].Vref.ang);
					printf ("I0:%8.3f/%6.3f \n", IcsNode.IcsSet[i].Evt[j].I0.mag,
							IcsNode.IcsSet[i].Evt[j].I0.ang);
					printf ("I2:%8.3f/%6.3f \n", IcsNode.IcsSet[i].Evt[j].I2.mag,
							IcsNode.IcsSet[i].Evt[j].I2.ang);
//					printf ("I0:%8.3f, V0:%6.3f \n", IcsNode.IcsSet[i].Evt[j].I0.mag,
//							IcsNode.IcsSet[i].Evt[j].V0.mag);
//					printf ("I2:%8.3f, V2:%6.3f \n\n", IcsNode.IcsSet[i].Evt[j].I2.mag,
//							IcsNode.IcsSet[i].Evt[j].V2.mag);
					printf ("Fault Direction:%1d \n", IcsNode.IcsSet[i].Evt[j].FltDirSum);
					printf ("Fault Distance:%8.3f \n", IcsNode.IcsSet[i].Evt[j].FltDist);
					printf ("Fault Zone: %1d \n\n", IcsNode.IcsSet[i].Evt[j].FltZone);
				}
			  }
#endif
            }
        }
// Clear Set Latches
        for (i = 0; i < MAX_SETS; i++) {
            IcsNode.IcsSet[i].ChgLch = 0;
            IcsNode.IcsSet[i].FltLch = 0;
            IcsNode.IcsSet[i].NtrLch = 0;
            IcsNode.IcsSet[i].TypLch = 0;
            IcsNode.IcsSet[i].TypLch = 0;
            IcsNode.IcsSet[i].FltCnt = 0;
            IcsNode.IcsSet[i].FltZone = 0;
            IcsNode.IcsSet[i].OpenSw = 0;

            memset(IcsNode.IcsSet[i].Evt, 0, EVT_Total_Size);

            //***
            //should clear out the samples and the samples?
            //***
            memset(&SmpNode.SmpSet[i].I, 0, MES_PHASES*SV_CYC_SIZE*floatSize);
            memset(&SmpNode.SmpSet[i].V, 0, MES_PHASES*SV_CYC_SIZE*floatSize);
            for (j = 0; j < MES_PHASES; j++) {
                memset(&IcsNode.IcsSet[i].IcsSns[j].Ibuf, 0, (BUF_SIZE+1)*XY_Size);
                memset(&IcsNode.IcsSet[i].IcsSns[j].Vbuf, 0, (BUF_SIZE+1)*XY_Size);
            }
        }
// Clear fault lock information
        IcsNode.RclCnt = 0;
        IcsNode.TrpCnt = 0;
        IcsNode.LchFlg = 0;
        IcsNode.FltLck = 0;   // Clear fault lock
        IcsNode.EvtTrigger = 0;
    }
    else if (IcsNode.Flt2Long) { // Fault for  too long, Abnormal fault
        // Report a fault for too long
        j = 0;
#ifndef PC_DEBUG
        for (i = 0; i < MAX_SETS; i++) {
            if (!IcsNode.IcsSet[i].Cfg->SrvFlg)
                continue;
            IcsNode.IcsSet[i].Evt[j].EvtTyp = EVENT_FAULT2LONG;

//???			ReportAbnormalFault(i+1, j, IcsNode.IcsSet[i].Evt[j]);
        }
#else
        printf ("Fault-2-Long ---\n");
		for (i = 0; i < MAX_SETS; i++) {
			if (!IcsNode.IcsSet[i].Cfg->SrvFlg)
				continue;
			IcsNode.IcsSet[i].Evt[j].EvtTyp = EVENT_FAULT2LONG;
//			ReportAbnormalFault(i+1, j, IcsNode.IcsSet[i].Evt[j]);

			printf ("ICS Set: %d \n", i+1);
			printf ("Fault2Long: Typ:%2d, Pha:%2d, Dir:%2d  %8.1f/%7.2f %2d: %8.1f/%7.2f %2d: %8.1f/%7.2f ## %8.1f/%7.2f\n",
			IcsNode.IcsSet[i].Evt[j].FltTyp,
			IcsNode.IcsSet[i].Evt[j].FltFlg, IcsNode.IcsSet[i].Evt[j].FltDir[0],
			IcsNode.IcsSet[i].Evt[j].If[0].mag, IcsNode.IcsSet[i].Evt[j].If[0].ang,
			IcsNode.IcsSet[i].Evt[j].FltDir[1],
			IcsNode.IcsSet[i].Evt[j].If[1].mag, IcsNode.IcsSet[i].Evt[j].If[1].ang,
			IcsNode.IcsSet[i].Evt[j].FltDir[2],
			IcsNode.IcsSet[i].Evt[j].If[2].mag, IcsNode.IcsSet[i].Evt[j].If[2].ang,
			IcsNode.IcsSet[i].Evt[j].Vref.mag, IcsNode.IcsSet[i].Evt[j].Vref.ang);
			printf ("I0:%8.3f/%6.3f \n", IcsNode.IcsSet[i].Evt[j].I0.mag,
					IcsNode.IcsSet[i].Evt[j].I0.ang);
			printf ("I2:%8.3f/%6.3f \n", IcsNode.IcsSet[i].Evt[j].I2.mag,
					IcsNode.IcsSet[i].Evt[j].I2.ang);
//			printf ("I0:%8.3f, V0:%6.3f \n", IcsNode.IcsSet[i].Evt[j].I0.mag,
//					IcsNode.IcsSet[i].Evt[j].V0.mag);
//			printf ("I2:%8.3f, V2:%6.3f \n\n", IcsNode.IcsSet[i].Evt[j].I2.mag,
//					IcsNode.IcsSet[i].Evt[j].V2.mag);
			printf ("Fault Direction:%1d \n", IcsNode.IcsSet[i].Evt[j].FltDirSum);
			printf ("Fault Distance:%8.3f \n", IcsNode.IcsSet[i].Evt[j].FltDist);
			printf ("Fault Zone: %1d \n\n", IcsNode.IcsSet[i].Evt[j].FltZone);
		}
#endif
// Clear Set Latches
        for (i = 0; i < MAX_SETS; i++) {
            IcsNode.IcsSet[i].ChgLch = 0;
            IcsNode.IcsSet[i].FltLch = 0;
            IcsNode.IcsSet[i].NtrLch = 0;
            IcsNode.IcsSet[i].TypLch = 0;
            IcsNode.IcsSet[i].FltCnt = 0;
            IcsNode.IcsSet[i].NtrCnt = 0;
            IcsNode.IcsSet[i].ChgCnt = 0;
            IcsNode.IcsSet[i].FltZone = 0;
            IcsNode.IcsSet[i].OpenSw = 0;
            memset(IcsNode.IcsSet[i].Evt, 0, EVT_Total_Size);

            //***
            //should clear out the samples and the samples?
            //***
            memset(&SmpNode.SmpSet[i].I, 0, MES_PHASES*SV_CYC_SIZE*floatSize);
            memset(&SmpNode.SmpSet[i].V, 0, MES_PHASES*SV_CYC_SIZE*floatSize);
            for (j = 0; j < MES_PHASES; j++) {
                memset(&IcsNode.IcsSet[i].IcsSns[j].Ibuf, 0, (BUF_SIZE+1)*XY_Size);
                memset(&IcsNode.IcsSet[i].IcsSns[j].Vbuf, 0, (BUF_SIZE+1)*XY_Size);
            }
        }
// Clear fault lock information
        IcsNode.Flt2Long = 0;
        IcsNode.LchFlg = 0;
        IcsNode.RclCnt = 0;
        IcsNode.TrpCnt = 0;
        IcsNode.EvtTrigger = 0;
    }
// Report Delta Change and Normal operation
/*
	if (IcsNode.RptChg && IcsNode.Cfg->RptChg) {  // Report Delta change to trigger the Master fault detection
#ifndef PC_DEBUG


#else
		printf ("Delta Report *** \n");
		j = EVT_SIZE - 1;
		for (i = 0; i < MAX_SETS; i++) {
			if (!IcsNode.IcsSet[i].Cfg->SrvFlg)
				continue;
			printf ("ICS Set: %d \n", i+1);
			printf ("DeltaReport: %8.1f/%8.2f %2d: %8.1f/%8.2f %2d: %8.1f/%8.2f ## %8.1f/%8.2f\n",
			IcsNode.IcsSet[i].Evt[j].If[0].mag, IcsNode.IcsSet[i].Evt[j].If[0].ang,
			IcsNode.IcsSet[i].Evt[j].FltDir[1],
			IcsNode.IcsSet[i].Evt[j].If[1].mag, IcsNode.IcsSet[i].Evt[j].If[1].ang,
			IcsNode.IcsSet[i].Evt[j].FltDir[2],
			IcsNode.IcsSet[i].Evt[j].If[2].mag, IcsNode.IcsSet[i].Evt[j].If[2].ang,
			IcsNode.IcsSet[i].Evt[j].Vref.mag, IcsNode.IcsSet[i].Evt[j].Vref.ang);
			printf ("I0:%8.3f, V0:%6.3f \n", IcsNode.IcsSet[i].Evt[j].I0.mag,
					IcsNode.IcsSet[i].Evt[j].V0.mag);
			printf ("I2:%8.3f, V2:%6.3f \n", IcsNode.IcsSet[i].Evt[j].I2.mag,
					IcsNode.IcsSet[i].Evt[j].V2.mag);
			printf ("ResultFaultDir:%1d \n", IcsNode.IcsSet[i].Evt[j].FltDirSum);
			printf ("FaultDistance:%8.3f \n", IcsNode.IcsSet[i].Evt[j].FltDist);
		}
#endif
		IcsNode.RptChg = 0;
	}
*/
    return Error;
}