//
// Created by Andre Henriques on 2019-08-15.
//

#ifndef SAM_COORDINATOR_ARM_MEASUREMENT_H
#define SAM_COORDINATOR_ARM_MEASUREMENT_H

#include <iostream>
#include "rapidjson/document.h"
#include "configuration/Utility/Substation/ElectricalAssetType/Show/Show.h"
#include "configuration/Utility/Substation/ElectricalAssetType/Rule/Rule.h"

using namespace std;
using namespace rapidjson;

class Measurement {
private:
    string field;
    string controlType;
    Show showOnList;
    Rule rule;
public:
    Measurement();

    const string &getField() const;

    void setField(const string &field);

    const string &getControlType() const;

    void setControlType(const string &controlType);

    const Show &getShowOnList() const;

    void setShowOnList(const Value &showOnList);

    const Rule &getRule() const;

    void setRule(const Value &rule);
};


#endif //SAM_COORDINATOR_ARM_MEASUREMENT_H
