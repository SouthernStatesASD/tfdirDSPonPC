//
// Created by Andre Henriques on 2019-08-15.
//

#ifndef SAM_COORDINATOR_ARM_GROUP_H
#define SAM_COORDINATOR_ARM_GROUP_H

#include <iostream>
#include "configuration/Utility/Substation/ElectricalAssetType/Field/Field.h"

using namespace std;

class Group {
private:
    bool isOpen;
    string displayName;
    vector<Field> fields;

public:
    Group();

    vector<Group> getVector(const Value &groups);

    bool isOpen1() const;

    void setIsOpen(bool isOpen);

    const string &getDisplayName() const;

    void setDisplayName(const string &displayName);

    const vector<Field> &getFields() const;

    void setFields(const Value &fields);
};


#endif //SAM_COORDINATOR_ARM_GROUP_H
