//
// Created by Andre Henriques on 2019-08-15.
//

#include "Tab.h"

Tab::Tab() {
    vector<Field> fields;
    vector<Reference> references;
    this->id = "";
    this->displayName = "";
    this->fields = fields;
    this->references = references;
}

vector<Tab> Tab::getVector(const Value &tabs) {
    vector<Tab> tabsVector;
    for(auto &thisTab : tabs.GetArray()) {
        Tab tab;
        if(thisTab.HasMember("id")) tab.setId(thisTab["id"].GetString());
        if(thisTab.HasMember("displayName")) tab.setDisplayName(thisTab["displayName"].GetString());
        if(thisTab.HasMember("fields")) tab.setFields(thisTab["fields"]);
        if(thisTab.HasMember("references")) tab.setReferences(thisTab["references"]);
        tabsVector.push_back(tab);
    }
    return tabsVector;
}

const string &Tab::getId() const {
    return id;
}

void Tab::setId(const string &id) {
    Tab::id = id;
}

const string &Tab::getDisplayName() const {
    return displayName;
}

void Tab::setDisplayName(const string &displayName) {
    Tab::displayName = displayName;
}

const vector<Field> &Tab::getFields() const {
    return fields;
}

void Tab::setFields(const Value &fields) {
    Field field;
    vector<Field> fieldsVector = field.getVector(fields);
    this->fields = fieldsVector;
}

const vector<Reference> &Tab::getReferences() const {
    return references;
}

void Tab::setReferences(const Value &references) {
    Reference reference;
    vector<Reference> referencesVector = reference.getVector(references);
    this->references = referencesVector;
}
