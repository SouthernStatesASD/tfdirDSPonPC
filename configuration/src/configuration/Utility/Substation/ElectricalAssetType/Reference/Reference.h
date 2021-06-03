//
// Created by Andre Henriques on 2019-08-15.
//

#ifndef SAM_COORDINATOR_ARM_REFERENCE_H
#define SAM_COORDINATOR_ARM_REFERENCE_H

#include <iostream>
#include "configuration/Utility/Substation/ElectricalAssetType/Field/Field.h"
#include "configuration/Utility/Substation/ElectricalAssetType/Group/Group.h"

using namespace std;

class Reference {
private:
    string referenceName;
    vector<Field> fields;
    vector<Group> groups;
public:
    Reference();

    vector<Reference> getVector(const Value &references);

    const string &getReferenceName() const;

    void setReferenceName(const string &referenceName);

    const vector<Field> &getFields() const;

    void setFields(const Value &fields);

    const vector<Group> &getGroups() const;

    void setGroups(const Value &groups);
};


#endif //SAM_COORDINATOR_ARM_REFERENCE_H
