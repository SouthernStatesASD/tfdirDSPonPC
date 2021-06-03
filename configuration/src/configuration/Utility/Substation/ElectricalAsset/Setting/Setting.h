//
// Created by Andre Henriques on 2019-08-15.
//

#ifndef SAM_COORDINATOR_ARM_SETTING_H
#define SAM_COORDINATOR_ARM_SETTING_H

#include <iostream>

using namespace std;

class Setting {
private:
    long serialNumber;
public:
    Setting();

    Setting(long serialNumber);

    long getSerialNumber();

    void setSerialNumber(long serialNumber);
};


#endif //SAM_COORDINATOR_ARM_SETTING_H
