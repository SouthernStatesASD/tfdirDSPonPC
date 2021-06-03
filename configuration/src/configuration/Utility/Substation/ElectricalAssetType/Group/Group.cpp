//
// Created by Andre Henriques on 2019-08-15.
//

#include "Group.h"

Group::Group() {
    vector<Field> fields;
    this->isOpen = false;
    this->displayName = "";
    this->fields = fields;
}

vector<Group> Group::getVector(const Value &groups) {
    vector<Group> referencesVector;
    for(auto &thisGroups : groups.GetArray()) {
        Group reference;
        if(thisGroups.HasMember("isOpen")) reference.setIsOpen(thisGroups["isOpen"].GetBool());
        if(thisGroups.HasMember("displayName")) reference.setDisplayName(thisGroups["displayName"].GetString());
        if(thisGroups.HasMember("fields")) reference.setFields(thisGroups["fields"]);
        referencesVector.push_back(reference);
    }
    return referencesVector;
}

bool Group::isOpen1() const {
    return isOpen;
}

void Group::setIsOpen(bool isOpen) {
    Group::isOpen = isOpen;
}

const string &Group::getDisplayName() const {
    return displayName;
}

void Group::setDisplayName(const string &displayName) {
    Group::displayName = displayName;
}

const vector<Field> &Group::getFields() const {
    return fields;
}

void Group::setFields(const Value &fields) {
    Field field;
    vector<Field> fieldsVector = field.getVector(fields);
    Group::fields = fieldsVector;
}
