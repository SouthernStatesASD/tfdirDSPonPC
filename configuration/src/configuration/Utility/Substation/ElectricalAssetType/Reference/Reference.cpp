//
// Created by Andre Henriques on 2019-08-15.
//

#include "Reference.h"

Reference::Reference() {
    vector<Field> fields;
    vector<Group> groups;
    this->referenceName = "";
    this->fields = fields;
    this->groups = groups;
}

vector<Reference> Reference::getVector(const Value &references) {
    vector<Reference> referencesVector;
    for(auto &thisReferences : references.GetArray()) {
        Reference reference;
        if(thisReferences.HasMember("referenceName")) reference.setReferenceName(thisReferences["referenceName"].GetString());
        if(thisReferences.HasMember("fields")) reference.setFields(thisReferences["fields"]);
        if(thisReferences.HasMember("groups")) reference.setGroups(thisReferences["groups"]);
        referencesVector.push_back(reference);
    }
    return referencesVector;
}

const string &Reference::getReferenceName() const {
    return referenceName;
}

void Reference::setReferenceName(const string &referenceName) {
    Reference::referenceName = referenceName;
}

const vector<Field> &Reference::getFields() const {
    return fields;
}

void Reference::setFields(const Value &fields) {
    Field field;
    vector<Field> fieldsVector = field.getVector(fields);
    this->fields = fieldsVector;
}

const vector<Group> &Reference::getGroups() const {
    return groups;
}

void Reference::setGroups(const Value &groups) {
    Group group;
    vector<Group> groupsVector = group.getVector(groups);
    this->groups = groupsVector;
}
