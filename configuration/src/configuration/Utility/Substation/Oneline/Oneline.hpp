//
// Created by Andre Henriques on 2019-08-14.
//

#ifndef SAM_COORDINATOR_ARM_ONELINE_HPP
#define SAM_COORDINATOR_ARM_ONELINE_HPP

#include <iostream>
#include "rapidjson/document.h"
#include <vector>

using namespace std;
using namespace rapidjson;

class Oneline {
private:
    string x;
    string y;
    string z;
public:
    Oneline();

    Oneline(string x, string y, string z);

    vector<Oneline> getVector(const Value &oneline);

    string getX();

    void setX(string x);

    string getY();

    void setY(string y);

    string getZ();

    void setZ(string z);
};


#endif //SAM_COORDINATOR_ARM_ONELINE_HPP
