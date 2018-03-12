#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <assert.h>
#include "phasor.h"

// initialize phasor object
void phasor_init(Phasor* phasor, float freq)
{
    int i;

    assert(phasor != NULL);
//incorect	phasor->t = 648.0/freq*DEGREES2PI;
    phasor->t = SVDEGREE*freq*DEGREES2PI;

    phasor->c2 = 1.0;
    phasor->s2 = 0.0;

    for (i = 0; i < SLOTS; i++)
    {
        phasor->c1[i] = cos(phasor->t*(float)(i+1));
        phasor->s1[i] = sin(phasor->t*(float)(i+1));
        phasor->c2 += 2.0*(phasor->c1[i]*phasor->c1[i]);
        phasor->s2 += 2.0*(phasor->s1[i]*phasor->s1[i]);
    }
}

// phasor object returns ax & ay values
// y[] has all 17 samples in the correct order and
// NO missing samples within the array
void phasor_calc(Phasor* phasor, float *y)
{
    int i;

    assert(phasor != NULL);
    phasor->ax = 0.0;
    phasor->ay = y[SLOTS]/phasor->c2;

    for (i = 0; i < SLOTS; i++)
    {
        phasor->ax += (y[i]*phasor->s1[SLOTS_1-i]-y[SLOTS2-i]*phasor->s1[SLOTS_1-i])/phasor->s2; // y[0] is the most new SV
//		phasor->ax -= (y[i]*phasor->s1[SLOTS_1-i]-y[SLOTS2-i]*phasor->s1[SLOTS_1-i])/phasor->s2; // y[0] is the oldest SV
        phasor->ay += (y[i]*phasor->c1[SLOTS_1-i]+y[SLOTS2-i]*phasor->c1[SLOTS_1-i])/phasor->c2;
    }
}
