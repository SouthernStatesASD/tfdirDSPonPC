//
// Created by Andre Henriques on 2019-08-13.
//

#ifndef SAM_COORDINATOR_ARM_UTILITY_HPP
#define SAM_COORDINATOR_ARM_UTILITY_HPP

#include <iostream>
#include <vector>
#include "rapidjson/document.h"
#include "configuration/Utility/Substation/Substation.hpp"

using namespace std;
using namespace rapidjson;

class Utility {
private:
    long id;
    string name;
    vector<Substation> substations;
public:
    Utility();

    Utility(const Value &utility);

    Utility(long id, string name, vector<Substation> substations);

    void setId(long id);

    void setName(string name);

    void setSubstations(const Value &substations);

    long getId();

    string getName();

    vector<Substation> getSubstations() const;
};

#endif //SAM_COORDINATOR_ARM_UTILITY_HPP
