#ifndef COMTRADE_H_
#define COMTRADE_H_

#include <stdio.h>
#include <string.h>

typedef unsigned int uint_32;

//#define SAMPLESperCYCLE 1664 // for ATP ComTrade
//#define SAMPLESperCYCLE 32 // for ICS ComTrade
#define SAMPLESperCYCLE 167 // for ATP Entergy ComTrade
//#define	PKTperCYCLE 8
#define	PKTperCYCLE 1
#define SAMPLESperPKT (SAMPLESperCYCLE/PKTperCYCLE)
#define SV_CFG_PATH "/Users/stevefan/Documents/SS/TFDIR/TFDIR src/src-03-06-2018/"
//#define SV_CFG_PATH "C:/Users/jfan/Workspace_v6.0/TFDIR/Data/GTC-SIM/"
//#define SV_CFG_PATH "C:/Users/jfan/Workspace_v6.0/TFDIR/Data/ICS/"
//#define SV_CFG_PATH "C:/Users/jfan/Workspace_v6.0/TFDIR/Data/Lumpkin/"

#define LINE_BUFFER 150
#define NUM_SIGNALS 6

typedef struct {
	uint_32 timestamp_sec;
	uint_32 timestamp_usec;
	float iA[SAMPLESperPKT];
	float iB[SAMPLESperPKT];
	float iC[SAMPLESperPKT];
	float vA[SAMPLESperPKT];
	float vB[SAMPLESperPKT];
	float vC[SAMPLESperPKT];
	unsigned short dataQuality;
} PKT;

typedef struct {
	char name[2];
	float gain;
	float offset;
} SIGNAL_INFO;

FILE* openCOMTRADE(char *);     //initilize pointer to data file
int	  getPKT(FILE* dat, SIGNAL_INFO *sigList, PKT *); //open data file and return packet
void  signalList(FILE* cfg, SIGNAL_INFO *sigList);

#endif
