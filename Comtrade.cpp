/*
 * comtrade.c
 *
 *  Created on: Mar 17, 2015
 *      Author: hpatel
 */

#include "Comtrade.h"
#include <stdlib.h>

FILE* openCOMTRADE(char *path) {
	FILE* dat;
//	dat = fopen(COMTRADE_DAT_PATH,"r");
	dat = fopen(path,"r");
	if(dat==NULL){
		printf("Error opening DAT file in: %s",path);
		return NULL;
	}
	else{
		return dat;
	}
}

int getPKT(FILE* dat, SIGNAL_INFO *sigList, PKT *packet) {

	int i, j, error=0;
	char line[LINE_BUFFER];
	uint_32 usec;
	float val[NUM_SIGNALS];

	for(i=0;i<SAMPLESperPKT;i++) {
		if(feof(dat) || !fgets(line,LINE_BUFFER,dat) || strlen(line) < 3) { //get line
			error = 1;
			packet->dataQuality = 1;
			break;
		}
		strtok(line,","); //drop sample number

		sscanf(strtok(NULL,","),"%u",&usec); //extract usec timesamp
		if(i==0){
			packet->timestamp_sec = usec/1000000;
			packet->timestamp_usec = usec - 1000000*packet->timestamp_sec;
			packet->dataQuality = 0;
		}

		for(j=0;j<NUM_SIGNALS;j++){ //extract sample values
			sscanf(strtok(NULL,","),"%f",&val[j]);

			//find matching phase and signal and assign value
			if(!strncmp(sigList[j].name,"VA",2)){ //scale and update matching values
				packet->vA[i]= 0.001*((sigList[j].gain * val[j])+sigList[j].offset);
				//packet.vA[i]=val[j];
			}
			else if(!strncmp(sigList[j].name,"VB",2)){
				packet->vB[i]= 0.001*((sigList[j].gain * val[j])+sigList[j].offset);
				//packet.vB[i]=val[j];
			}
			else if(!strncmp(sigList[j].name,"VC",2)){
				packet->vC[i]= 0.001*((sigList[j].gain * val[j])+sigList[j].offset);
				//packet.vC[i]=val[j];
			}
			else if(!strncmp(sigList[j].name,"AA",2)){
				packet->iA[i]=(sigList[j].gain * val[j])+sigList[j].offset;
				//packet.iA[i]=val[j];
			}
			else if(!strncmp(sigList[j].name,"AB",2)){
				packet->iB[i]=(sigList[j].gain * val[j])+sigList[j].offset;
				//packet.iB[i]=val[j];
			}
			else if(!strncmp(sigList[j].name,"AC",2)){
				packet->iC[i]=(sigList[j].gain * val[j])+sigList[j].offset;
				//packet.iC[i]=val[j];
			}
		}
	}
	return error;
}

//int signalList(SIGNAL_INFO *sigList)
void signalList(FILE *cfg, SIGNAL_INFO *sigList)
{
	int i;
	float gain,offset;
	char line[LINE_BUFFER], *phase, *type;

//dump first 2 lines (title & #signals)
	for(i=0;i<2;i++){
		fgets(line,LINE_BUFFER,cfg);
	}

	for(i=0;i<NUM_SIGNALS;i++){

		fgets(line,LINE_BUFFER,cfg);
		strtok(line,","); //drop channel number
		strtok(NULL,","); //drop name
		phase  = strtok(NULL,", ");//extract phase & remove spaces
		strtok(NULL,","); //drop ccbm
		type = strtok(NULL,", "); //extract signal type & remove spaces
		sscanf(strtok(NULL,", "),"%e",&gain); //extract gain
		sscanf(strtok(NULL,", "),"%e",&offset); //extract offset

		//assign to structure
		sigList[i].gain = gain;
		sigList[i].offset = offset;
		sigList[i].name[0]=*type;
		sigList[i].name[1]=*phase;
	}
	fclose(cfg);
}
