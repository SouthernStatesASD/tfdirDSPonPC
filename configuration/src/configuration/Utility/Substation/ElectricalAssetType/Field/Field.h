//
// Created by Andre Henriques on 2019-08-15.
//

#ifndef SAM_COORDINATOR_ARM_FIELD_H
#define SAM_COORDINATOR_ARM_FIELD_H

#include <iostream>
#include "configuration/Utility/Substation/ElectricalAssetType/Show/Show.h"
#include "configuration/Utility/Substation/ElectricalAssetType/Rule/Rule.h"

using namespace std;

class Field {
private:
    string fieldName;
    string displayName;
    Show showOnList;
    Rule rule;

public:
    Field();

    vector<Field> getVector(const Value &fields);

    const string &getFieldName() const;

    void setFieldName(const string &fieldName);

    const string &getDisplayName() const;

    void setDisplayName(const string &displayName);

    const Show &getShowOnList() const;

    void setShowOnList(const Value &showOnList);

    const Rule &getRule() const;

    void setRule(const Value &rule);
};


#endif //SAM_COORDINATOR_ARM_FIELD_H
