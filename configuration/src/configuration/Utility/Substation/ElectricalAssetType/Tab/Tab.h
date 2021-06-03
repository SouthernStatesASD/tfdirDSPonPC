//
// Created by Andre Henriques on 2019-08-15.
//

#ifndef SAM_COORDINATOR_ARM_TAB_H
#define SAM_COORDINATOR_ARM_TAB_H

#include <iostream>
#include "configuration/Utility/Substation/ElectricalAssetType/Field/Field.h"
#include "configuration/Utility/Substation/ElectricalAssetType/Reference/Reference.h"

using namespace std;

class Tab {
private:
    string id;
    string displayName;
    vector<Field> fields;
    vector<Reference> references;

public:
    Tab();

    vector<Tab> getVector(const Value &tabs);

    const string &getId() const;

    void setId(const string &id);

    const string &getDisplayName() const;

    void setDisplayName(const string &displayName);

    const vector<Field> &getFields() const;

    void setFields(const Value &fields);

    const vector<Reference> &getReferences() const;

    void setReferences(const Value &references);
};


#endif //SAM_COORDINATOR_ARM_TAB_H
