//
// Created by Andre Henriques on 2019-08-15.
//

#ifndef SAM_COORDINATOR_ARM_MODEL_H
#define SAM_COORDINATOR_ARM_MODEL_H

#include <iostream>

using namespace std;

class Model {
private:
    string electrical;
    string threeDimentional;
public:
    Model();

    const string &getElectrical() const;

    void setElectrical(const string &electrical);

    const string &getThreeDimentional() const;

    void setThreeDimentional(const string &threeDimentional);
};


#endif //SAM_COORDINATOR_ARM_MODEL_H
