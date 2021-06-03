//
// Created bnode Andre Henriques on 2019-08-15.
//

#ifndef SAM_COORDINATOR_ARM_TERMINAL_H
#define SAM_COORDINATOR_ARM_TERMINAL_H

#include <iostream>
#include "rapidjson/document.h"

using namespace std;
using namespace rapidjson;

class Terminal {
private:
    long id;
    long node;
public:
    Terminal();

    Terminal(long id, long node);

    vector<Terminal> getVector(const Value &terminals);

    long getId();

    void setId(long id);

    long getNode();

    void setNode(long node);
};


#endif //SAM_COORDINATOR_ARM_TERMINAL_H
