#include <stdint.h>
#ifndef PHASOR_H_
#define PHASOR_H_

#define SLOTS			(16)
#define SLOTS2			(2*SLOTS-1)
#define SLOTS_1			(SLOTS-1)
#define DEGREES2PI 3.141592654/180.0
#define SVDEGREE   0.18 // SV interval in degree = 360(360 degrees)/2000(NumSV per second)

/* Phasor Structure */
typedef struct PH {
    float c1[SLOTS];
    float s1[SLOTS];
    float c2;
    float s2;
    float ax;
    float ay;
    float t;
} Phasor;

//typedef struct PH* PhasorHandle;

void phasor_init(Phasor* phasor, float freq);
void phasor_calc(Phasor* phasor, float* y);

#endif /* PHASOR_H_ */
