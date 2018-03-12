/*
 * PhasorContainerV2.cpp
 *
 *  Created on: Feb 12, 2018
 *      Author: user
 */

#include <stdio.h>
#include "PhasorContainerV2.h"

#ifdef PC_DEBUG
#include "Comtrade.h"
#else
#include "dspDebugLog.h"
#endif

PhasorContainerV2::PhasorContainerV2() {
    // TODO Auto-generated constructor stub
    Initialize();
}

PhasorContainerV2::~PhasorContainerV2() {
    // TODO Auto-generated destructor stubDEBU
}

void PhasorContainerV2::Initialize() {
    configLoaded = false;
    XY_Size = sizeof(XY);
    PHASOR_Size = sizeof(PHASOR);

    configContentsArray = new char*[300];
    for(int i=0; i<300; i++) {
        configContentsArray[i] = new char[LINE_READ_BUFFER];
    }

//	const char* buf_name = MsgBase::DSP_PHASOR_RECORDING_NAME;
//	sharedMemory = new SharedMemoryBuffer(buf_name, MsgBase::DSP_BINARY_RECORDING_BUF_SIZE);

//	SetInit(0);

    //test only
//	genRawSamples();
    genRawSamplesHardCoded();
}

int PhasorContainerV2::Configure(FLAG SetOption, char *configContents) {
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

    Ir = 2.0*CfgNode.CfgSet[0].Irated;
    Vr = CfgNode.Vrated;

    return Error;
}

int PhasorContainerV2::parseConfig(char *configContents) {

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
            //		"VMaa", // Voltage model matrix (symetrical)
            //		"VMab",
            //		"VMac",
            //		"VMbb",
            //		"VMbc",
            //		"VMcc",
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
    ///

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

    if (configContents != NULL) {
        char *readline = strtok(configContents, "\n");
        int lineCt = 0;
        while(readline != NULL) {
            strncpy(configContentsArray[lineCt], readline, LINE_READ_BUFFER);
            readline = strtok(NULL, "\n");
            lineCt++;
        }
    }

    for(int lineCt=0; lineCt<300; lineCt++){
        strncpy(line, configContentsArray[lineCt], LINE_READ_BUFFER);

//		printf("Config Line %d: %s", lineCt, line);

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

//	if (Error == 0) {
    configLoaded = true;
//	}

    return Error;
}

void PhasorContainerV2::populateParam(char *line, PARAM *param){

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

int PhasorContainerV2::populateNode(PARAM param){

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

void PhasorContainerV2::populateSet(PARAM param, CFG_SET *Set){

    FLAG *setFlagMap[] = {
            &Set->SeqNum,
            &Set->SrvFlg,
            &Set->VrtFlg,
            &Set->SetTyp,
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

void PhasorContainerV2::populateSns(PARAM param, CFG_SNS *Sensor){

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

int PhasorContainerV2::checkNode(){
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

int PhasorContainerV2::checkSet(){
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

int PhasorContainerV2::checkSns(){
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

int PhasorContainerV2::VoltModel()
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

void PhasorContainerV2::CalibVolt ()
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

        if ((k < 0 && yi < 1.0e5) || (im < 1.2*IcsNode.Cfg->Imin && yi > im) ||
            (im >= 1.2*IcsNode.Cfg->Imin && yv > vm))
            k = i;

        im = (im > yi)? im : yi; // select the biggest
        vm = (vm < yv)? vm : yv; // select the smallest;

#ifdef PC_DEBUG
        printf ("\n DynClib Set(%2d): %6.3f/%6.3f  %6.3f/%6.3f  %6.3f/%6.3f\n\n", i,
                IcsNode.IcsSet[i].IcsSns[0].Vmdcf, IcsNode.IcsSet[i].IcsSns[0].Vadcf,
                IcsNode.IcsSet[i].IcsSns[1].Vmdcf, IcsNode.IcsSet[i].IcsSns[1].Vadcf,
                IcsNode.IcsSet[i].IcsSns[2].Vmdcf, IcsNode.IcsSet[i].IcsSns[2].Vadcf);
#endif
    }

    IcsNode.VoltSetNum = (k >= 0) ? k : IcsNode.VoltSetNum;

    if (im < 1.2*IcsNode.Cfg->Imin || vm < 1.2*IcsNode.Cfg->Vmin)
        IcsNode.CtrlSusp = 1; // Suspend switch control due to too low current or voltage
    else
        IcsNode.CtrlSusp = 0;
}

void PhasorContainerV2::genRawSamples() {//test only
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

void PhasorContainerV2::genRawSamplesHardCoded() {//test only

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

#ifdef PC_DEBUG
int PhasorContainerV2::GetSamples(FLAG CallFlag)
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
            if (error)
                return error;
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
#endif

#ifndef PC_DEBUG
void PhasorContainerV2::addNewSample(DataStream& setDataStream, int set) {
    if (configLoaded) {
        //only perform sampling if configuration is actually loaded
        //get the previous time
        double lastSmpSetTime = SmpNode.SmpSet[set].t.sec + (double)SmpNode.SmpSet[set].t.usec/1000000.0;
        double curSampleTime = setDataStream.data.time.secs + (double)setDataStream.data.time.nsecs/1000000000.0;

        int samplesAway = numSamplesAway(curSampleTime, lastSmpSetTime);

//		printf("DSP: lastime=%f, samptime=%f, away=%d", lastSmpSetTime, curSampleTime, samplesAway);

        if (lastSmpSetTime == 0) { //very first time, just set the idx to 0
            samplesAway = 0;
        }

        processSmpSetData(setDataStream, set, samplesAway);
    }
}

void PhasorContainerV2::processSmpSetData(DataStream& setDataStream, int set, int idx) {
    int phase = setDataStream.data.id/2;
    if (phase > MES_PHASES-1)
        phase = phase-MES_PHASES;

    if(idx >= 0) {
        //this means it's a newer sample, which will require shifting of the existing values down.
        //Also requires it to set the time
        SmpNode.SmpSet[set].t.sec = setDataStream.data.time.secs;
        SmpNode.SmpSet[set].t.usec = setDataStream.data.time.nsecs/1000;

        if (idx > 6) {
            //missed too many samples, reset to 0s
            memset(&SmpNode.SmpSet[set].I, 0, MES_PHASES*SV_CYC_SIZE*XY_Size);
            memset(&SmpNode.SmpSet[set].V, 0, MES_PHASES*SV_CYC_SIZE*XY_Size);

        } else if (idx > 0) {
            //shift the existing samples down by the idx
            memmove(&SmpNode.SmpSet[set].I[phase][idx], &SmpNode.SmpSet[set].I[phase][0], (SV_CYC_SIZE-idx)*XY_Size);
            memmove(&SmpNode.SmpSet[set].V[phase][idx], &SmpNode.SmpSet[set].V[phase][0], (SV_CYC_SIZE-idx)*XY_Size);

            if (idx > 2) {
                //need to fill in the missed samples using the last know phasors
                double curSampleTime = setDataStream.data.time.secs + (double)setDataStream.data.time.nsecs/1000000000.0;
                XY *iPhasor = &IcsNode.IcsSet[set].IcsSns[phase].Ibuf[0];
                XY *vPhasor = &IcsNode.IcsSet[set].IcsSns[phase].Vbuf[0];
                double missedI, missedV;

                for (int i=3; i<=idx; i++) {
                    //sample calc, I/V=Mag*(2*pi*f*t + Ang)
                    missedI = iPhasor->x*cos(2*M_PI*CfgNode.SysFreq*(curSampleTime-0.0005*i) + iPhasor->y)/Ir;
                    missedV = vPhasor->x*cos(2*M_PI*CfgNode.SysFreq*(curSampleTime-0.0005*i) + vPhasor->y)/Ir;

                    setSmpSetValues(set, phase, (i-1), missedI, missedV);

                    printf("DSP: Missed Sample time=%f, idx=%d/%d, I=%f, V=%f", curSampleTime, i, idx, missedI, missedV);
                }
            }
        }

        //always sets the newest sample to 0th obj
        idx = 0;
    }

    if (idx >= -31 && idx <= 0) {
        //set the newest sample that should always include the data from 500us ago too
        setSmpSetValues(set, phase, idx, setDataStream.data.sample.currentNow, setDataStream.data.sample.voltageNow);
        setSmpSetValues(set, phase, idx+1, setDataStream.data.sample.current0_5msAgo, setDataStream.data.sample.voltageNow); //needs to be voltage0_5msAgo

        double curSampleTime = setDataStream.data.time.secs + (double)setDataStream.data.time.nsecs/1000000000.0;
//		printf("DSP: new Sample time=%f, idx=%d, I=%f, V=%f", curSampleTime, idx, SmpNode.SmpSet[set].I[phase][idx], SmpNode.SmpSet[set].V[phase][idx]);
//		printf("DSP: new Sample2 time=%f, idx=%d, I=%f, V=%f", curSampleTime, idx+1, SmpNode.SmpSet[set].I[phase][idx+1], SmpNode.SmpSet[set].I[phase][idx+1]);
    }
}
#endif

void PhasorContainerV2::setSmpSetValues(int set, int phase, int idx, float i, float v) {

    SmpNode.SmpSet[set].I[phase][idx] = Ir*i;
    SmpNode.SmpSet[set].V[phase][idx] = Vr*v;
}

int PhasorContainerV2::numSamplesAway(double time, double timeRef) {
    double timeRange = 0.000375;//  375us, 75% of 1 fpga sampling period

    double diff = time-timeRef;

    return (int)(diff/timeRange);
}

void PhasorContainerV2::calcPhasors() {
    int i, j, k, l;
    float x, y, w, z;
//    float Ir = 2.0*IcsNode.Cfg->CfgSet[0].Irated;
//    float Vr = IcsNode.Cfg->Vrated;
    FLAG  Vbad;

    float F = 60.0;
    Phasor VP, IP;

    phasor_init(&VP, F);
    phasor_init(&IP, F);

    for (i = 0; i < MAX_SETS; i++) {
        if (!IcsNode.IcsSet[i].Cfg->SrvFlg || IcsNode.IcsSet[i].Cfg->VrtFlg)
            continue;
        Vbad = 0;
        for (j = 0; j < MES_PHASES; j++) {
            k = (int)IcsNode.IcsSet[i].IcsSns[j].Cfg->SeqNum - 1; // Use the SeqNum to correct the Phase lable
            l = (int)IcsNode.IcsSet[i].IcsSns[j].Cfg->PhaseID - 1;
            if (l == 3) l = 2;
            IcsNode.IcsSet[i].IcsSns[l].Ibuf[0].flg = SmpNode.SmpSet[i].Iflg[k];
            IcsNode.IcsSet[i].IcsSns[l].Vbuf[0].t   = SmpNode.SmpSet[i].t;
            IcsNode.IcsSet[i].IcsSns[l].Vbuf[0].flg = SmpNode.SmpSet[i].Vflg[k];
            if (!SmpNode.SmpSet[i].Iflg[k]) {
                x = y = 0.0;

//				phasor_calc(&IP, SmpNode.SmpSet[i].I[k]);
//				x = IP.ax;
//				y = IP.ay;

                fft(&x, &y, IcsNode.CosV, IcsNode.SinV, SmpNode.SmpSet[i].I[k], SV_CYC_SIZE);

                w = XY2Mag(x, y);
                w *= 1.4142*Ir*CfgNode.CfgSet[i].AmpSclFactor/(float) SmpNode.SmpSet[i].NumI[k];
                z = XY2Ang(x, y) + CfgNode.CfgSet[i].AmpAngOffset;
                MagAng2XY(&x, &y, &w, &z);
                IcsNode.IcsSet[i].IcsSns[l].Ibuf[0].x = x;
                IcsNode.IcsSet[i].IcsSns[l].Ibuf[0].y = y;
                if (w < IcsNode.Cfg->Imin)
                    IcsNode.IcsSet[i].IcsSns[j].Ibuf[0].flg = DATA_NO;
            }
            else { // bad data or no data, set to the previous cycle value
                IcsNode.IcsSet[i].IcsSns[j].Ibuf[0].x = IcsNode.IcsSet[i].IcsSns[j].Ibuf[1].x;
                IcsNode.IcsSet[i].IcsSns[j].Ibuf[0].y = IcsNode.IcsSet[i].IcsSns[j].Ibuf[1].y;
            }

            if (!SmpNode.SmpSet[i].Vflg[j]) {
                x = y = 0.0;

//				phasor_calc(&VP, SmpNode.SmpSet[i].V[k]);
//				x = VP.ax;
//				y = VP.ay;

                fft(&x, &y, IcsNode.CosV, IcsNode.SinV, SmpNode.SmpSet[i].V[k], SV_CYC_SIZE);

                w = XY2Mag(x, y);
                w *= 1.4142*Vr*CfgNode.CfgSet[i].VoltSclFactor/(float) SmpNode.SmpSet[i].NumV[k];
                z = XY2Ang(x, y) + CfgNode.CfgSet[i].VoltAngOffset;
                IcsNode.IcsSet[i].VmIcs[l].mag = w;	// Volt Mag before dynamic calibration
                IcsNode.IcsSet[i].VmIcs[l].ang = z;	// Volt Ang before dynamic calibration
                w *= IcsNode.IcsSet[i].IcsSns[l].Vmdcf; // Apply dinamic calibration on Volt Mag
                z += IcsNode.IcsSet[i].IcsSns[l].Vadcf; // Apply dinamic calibration on Volt Ang
                if (w < IcsNode.Cfg->Vmin) {
                    IcsNode.IcsSet[i].IcsSns[l].Vbuf[0].flg = DATA_NO;
                    Vbad ++;
                }
                MagAng2XY(&x, &y, &w, &z);
                IcsNode.IcsSet[i].IcsSns[l].Vbuf[0].x = x;
                IcsNode.IcsSet[i].IcsSns[l].Vbuf[0].y = y;
            }
            else { // bad data or no data, set to the previous cycle value
//				IcsNode.IcsSet[i].IcsSns[j].Vbuf[0].x = IcsNode.IcsSet[i].IcsSns[j].Vbuf[1].x ;
//				IcsNode.IcsSet[i].IcsSns[j].Vbuf[0].y = IcsNode.IcsSet[i].IcsSns[j].Vbuf[1].y ;
                Vbad ++;
            }

//			printf("DSP Phasor %d, %d, %d, %d: %.2f, %.2f\n", i, j, k, l, IcsNode.IcsSet[i].IcsSns[l].Ibuf[0].x, IcsNode.IcsSet[i].IcsSns[l].Ibuf[0].y);

            if(IcsNode.IcsSet[i].IcsSns[j].Cfg->CTPolarity) {
                IcsNode.IcsSet[i].IcsSns[l].Ibuf[0].x *= -1.0;
                IcsNode.IcsSet[i].IcsSns[l].Ibuf[0].y *= -1.0;
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
                break;
            }
            y = 1.e6;
            for (j = 0; j < MES_PHASES; j++) {
                w = XY2Mag(IcsNode.IcsSet[i].IcsSns[j].Ibuf[0].x, IcsNode.IcsSet[i].IcsSns[j].Ibuf[0].y);
                if (w > 2*IcsNode.Cfg->Imin && w < y)
                    y = w;
            }

            if (y < 1.e4 && y > x) {
                x = y;
                l = i;
            }
        }

        if (k >= 0) {
            for (j = 0; j < MES_PHASES; j++) {
                memset(&IcsNode.IcsSet[k].IcsSns[j].Ibuf[0], 0, sizeof(XY));
                memset(&IcsNode.IcsSet[k].IcsSns[j].Vbuf[0], 0, sizeof(XY));
            }

            for (i = 0; i < MAX_SETS; i++) {
                if (!IcsNode.IcsSet[i].Cfg->SrvFlg || IcsNode.IcsSet[i].Cfg->VrtFlg)
                    continue;
                for (j = 0; j < MES_PHASES; j++) {
                    IcsNode.IcsSet[k].IcsSns[j].Ibuf[0].flg = IcsNode.IcsSet[i].IcsSns[j].Ibuf[0].flg;
                    IcsNode.IcsSet[k].IcsSns[j].Ibuf[0].t = IcsNode.IcsSet[i].IcsSns[j].Ibuf[0].t;
                    IcsNode.IcsSet[k].IcsSns[j].Ibuf[0].x -= IcsNode.IcsSet[i].IcsSns[j].Ibuf[0].x;
                    IcsNode.IcsSet[k].IcsSns[j].Ibuf[0].y -= IcsNode.IcsSet[i].IcsSns[j].Ibuf[0].y;
                    if (l >= 0) {
                        IcsNode.IcsSet[k].IcsSns[j].Vbuf[0].flg = IcsNode.IcsSet[l].IcsSns[j].Vbuf[0].flg;
                        IcsNode.IcsSet[k].IcsSns[j].Vbuf[0].t = IcsNode.IcsSet[l].IcsSns[j].Vbuf[0].t;
                        IcsNode.IcsSet[k].IcsSns[j].Vbuf[0].x = IcsNode.IcsSet[l].IcsSns[j].Vbuf[0].x;
                        IcsNode.IcsSet[k].IcsSns[j].Vbuf[0].y = IcsNode.IcsSet[l].IcsSns[j].Vbuf[0].y;
                    }
                }
            }
        }
    }

}

void PhasorContainerV2::sendToARM() {
    // Send the binary data
//	char* shared_buf = NULL;
//	int   shared_buf_size(0);
//	SharedMemoryBuffer* sharedBuf = *sharedMemIt;
//
//	if (sharedMemory->AcquireBuffer(&sharedMemory, shared_buf_size))
//	{
//		if (sharedMemory != NULL)
//		{
//			// copy the binary data into the shared memory buffer
//			gCircularBuf->Read(shared_buf, shared_buf_size);
//
//			// Release the shared buffer and notify the reader that data is available
//			if (sharedMemory->ReleaseAndNotify(gCircularBuf->BytesInBuffer()))
//			{
//				printf("DSP: Sending data...");
//				gCircularBuf->Reset(0);       // reset the buffer after sending (set to all 0)
//				test_mode_scalar = 1;
//			}
//		}
//	}
}

// calculate the magnitude from x & y values.
float PhasorContainerV2::XY2Mag(float x, float y) {
    return (float)(sqrt((x*x) + (y*y)));
}

// calculate the angle from x & y values.
float PhasorContainerV2::XY2Ang(float x, float y) {
    return (float)(atan2(y, x)*PI2DEGREES);
}

// calculate the x (real) component from magnitude & angle values.
float PhasorContainerV2::MagAng2X(float mag, float ang) {
    return (float)(mag * cos(ang * DEGREES2PI));
}

// calculate the y (reactive) component from magnitude & angle values.
float PhasorContainerV2::MagAng2Y(float mag, float ang) {
    return (float)(mag * sin(ang * DEGREES2PI));
}

// calculate the x (real) and y (reactive) components from magnitude & angle values.
void PhasorContainerV2::MagAng2XY(float *x, float *y, float *mag, float *ang) {
    float w = *ang * DEGREES2PI;
    *x = *mag * cos(w);
    *y = *mag * sin(w);
}

RX PhasorContainerV2::VectAdd(RX* Z1, RX* Z2) {
    RX w;
    w.r = Z1->r + Z2->r;
    w.x = Z1->x + Z2->x;
    return w;
}

RX PhasorContainerV2::VectSub(RX* Z1, RX* Z2) {
    RX w;
    w.r = Z1->r - Z2->r;
    w.x = Z1->x - Z2->x;
    return w;
}

RX PhasorContainerV2::VectX2(RX* Z1, RX* Z2) {
    RX w;
    w.r = Z1->r*Z2->r - Z1->x*Z2->x;
    w.x = Z1->r*Z2->x + Z1->x*Z2->r;
    return w;
}

RX PhasorContainerV2::VectX3(RX* Z1, RX* Z2, RX* Z3) {
    float r, x;
    RX w;
    r = Z1->r*Z2->r - Z1->x*Z2->x;
    x = Z1->r*Z2->x + Z1->x*Z2->r;
    w.r = r*Z3->r - x*Z3->x;
    w.x = r*Z3->x + x*Z3->r;
    return w;
}

void PhasorContainerV2::abcxy2s0(PHASOR *S, XY *A, XY *B, XY *C)  // Calculate 3.0*I0 from ABC vectors
{
    float x, y;
    x = A->x + B->x + C->x;
    y = A->y + B->y + C->y;
    S->mag = XY2Mag(x, y)/3.0;
    S->ang = XY2Ang(x, y);
}

void PhasorContainerV2::abc2s0(PHASOR *S, PHASOR *A, PHASOR *B, PHASOR *C)  // Calculate 3.0*I0 from ABC vectors
{
    float x, y;
    x = MagAng2X(A->mag, A->ang) + MagAng2X(B->mag, B->ang) + MagAng2X(C->mag, C->ang);
    y = MagAng2Y(A->mag, A->ang) + MagAng2Y(B->mag, B->ang) + MagAng2Y(C->mag, C->ang);
    S->mag = XY2Mag(x, y)/3.0;
    S->ang = XY2Ang(x, y);
}

void PhasorContainerV2::abc2s1(PHASOR *S, PHASOR *A, PHASOR *B, PHASOR *C)  // Calculate I1 from ABC vectors
{
    float x, y;
    x = MagAng2X(A->mag, A->ang) + MagAng2X(B->mag, B->ang+120.0) + MagAng2X(C->mag, C->ang-120.0);
    y = MagAng2Y(A->mag, A->ang) + MagAng2Y(B->mag, B->ang+120.0) + MagAng2Y(C->mag, C->ang-120.0);
    S->mag = XY2Mag(x, y)/3.0;
    S->ang = XY2Ang(x, y);
}

void PhasorContainerV2::abc2s2(PHASOR *S, PHASOR *A, PHASOR *B, PHASOR *C)  // Calculate I2 from ABC vectors
{
    float x, y;
    x = MagAng2X(A->mag, A->ang) + MagAng2X(B->mag, B->ang-120.0) + MagAng2X(C->mag, C->ang+120.0);
    y = MagAng2Y(A->mag, A->ang) + MagAng2Y(B->mag, B->ang-120.0) + MagAng2Y(C->mag, C->ang+120.0);
    S->mag = XY2Mag(x, y)/3.0;
    S->ang = XY2Ang(x, y);
}

void PhasorContainerV2::abc2s012(PHASOR *Seq012, PHASOR *ABC)  // Convert phase domain ABC vector into Seqeunce Domain 012 vector
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

void PhasorContainerV2::PhasorAdd(PHASOR *S, PHASOR *A, PHASOR *B)  // Phasor S = A + B
{
    float x, y;
    x = MagAng2X(A->mag, A->ang) + MagAng2X(B->mag, B->ang);
    y = MagAng2Y(A->mag, A->ang) + MagAng2Y(B->mag, B->ang);
    S->mag = XY2Mag(x, y);
    S->ang = XY2Ang(x, y);
}

void PhasorContainerV2::PhasorDiff(PHASOR *D, PHASOR *A, PHASOR *B)  // Phasor D = A - B
{
    float x, y;
    x = MagAng2X(A->mag, A->ang) - MagAng2X(B->mag, B->ang);
    y = MagAng2Y(A->mag, A->ang) - MagAng2Y(B->mag, B->ang);
    D->mag = XY2Mag(x, y);
    D->ang = XY2Ang(x, y);
}

PHASOR PhasorContainerV2::PhasorShift(PHASOR *A, float Ang)  // Phasor A->ang += Ang
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

void PhasorContainerV2::VI2PQ(float *P, float *Q, PHASOR *V, PHASOR *I)
{
    float vx, vy, ix, iy;
    vx = MagAng2X(V->mag, V->ang);
    vy = MagAng2Y(V->mag, V->ang);
    ix = MagAng2X(I->mag, I->ang);
    iy = MagAng2Y(I->mag, I->ang);
    *P = sqrt(vx*ix + vy*iy);
    *Q = sqrt(vy*ix - vx*iy);
}

float PhasorContainerV2::TimeDiff (TIME *t1, TIME *t2)
{
    float t = 1000.0*(float)(t1->sec - t2->sec) + (float)(t1->usec - t2->usec)/1000.0;
    return t; // Milli Seconds
}

void PhasorContainerV2::fft(float *X, float *Y, float *C, float *S, float *SV, int K)
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

int PhasorContainerV2::MatInv(float *b, float *a, int n) // Matrix Inverse:
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