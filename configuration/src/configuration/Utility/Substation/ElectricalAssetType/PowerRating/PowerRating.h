//
// Created by Andre Henriques on 2019-08-15.
//

#ifndef SAM_COORDINATOR_ARM_POWERRATING_H
#define SAM_COORDINATOR_ARM_POWERRATING_H

#include <iostream>

using namespace std;

class PowerRating {
private:
    int kvar;
    int frequency;

public:
    PowerRating();

    int getKvar() const;

    void setKvar(int kvar);

    int getFrequency() const;

    void setFrequency(int frequency);
};


#endif //SAM_COORDINATOR_ARM_POWERRATING_H
