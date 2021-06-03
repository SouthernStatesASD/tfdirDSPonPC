//
// Created by Andre Henriques on 2019-08-15.
//

#include "Field.h"

Field::Field() {
    this->fieldName = "";
    this->displayName = "";
    this->showOnList = Show();
    this->rule = Rule();
}

vector<Field> Field::getVector(const Value &fields) {
    vector<Field> fieldsVector;
    for(auto &thisField : fields.GetArray()) {
        Field field;
        if(thisField.HasMember("fieldName")) field.setFieldName(thisField["fieldName"].GetString());
        if(thisField.HasMember("displayName")) field.setDisplayName(thisField["displayName"].GetString());
        if(thisField.HasMember("showOnList")) field.setShowOnList(thisField["showOnList"]);
        if(thisField.HasMember("rule")) field.setRule(thisField["rule"]);
        fieldsVector.push_back(field);
    }
    return fieldsVector;
}

const string &Field::getFieldName() const {
    return fieldName;
}

void Field::setFieldName(const string &fieldName) {
    Field::fieldName = fieldName;
}

const string &Field::getDisplayName() const {
    return displayName;
}

void Field::setDisplayName(const string &displayName) {
    Field::displayName = displayName;
}

const Show &Field::getShowOnList() const {
    return showOnList;
}

void Field::setShowOnList(const Value &showOnList) {
    Show showOnListObject;
    if(showOnList.HasMember("cellColor")) showOnListObject.setCellColor(showOnList["cellColor"].GetBool());
    if(showOnList.HasMember("icon")) showOnListObject.setIcon(showOnList["icon"].GetBool());
    if(showOnList.HasMember("value")) showOnListObject.setValue(showOnList["value"].GetBool());
    this->showOnList = showOnListObject;
}

const Rule &Field::getRule() const {
    return rule;
}

void Field::setRule(const Value &rule) {
    Rule ruleObject;
    if(rule.HasMember("requried")) ruleObject.setRequired(rule["requried"].GetBool());
    if(rule.HasMember("disabled")) ruleObject.setDisabled(rule["disabled"].GetBool());
    if(rule.HasMember("controlType")) ruleObject.setControlType(rule["controlType"].GetString());
    if(rule.HasMember("dataType")) ruleObject.setDataType(rule["dataType"].GetString());
    if(rule.HasMember("numericType")) ruleObject.setNumericType(rule["numericType"].GetString());
    if(rule.HasMember("prefix")) ruleObject.setPrefix(rule["prefix"].GetString());
    if(rule.HasMember("suffix")) ruleObject.setSuffix(rule["suffix"].GetString());
    if(rule.HasMember("scalar")) ruleObject.setScalar(rule["scalar"].GetFloat());
    if(rule.HasMember("min")) ruleObject.setMin(rule["min"].GetFloat());
    if(rule.HasMember("max")) ruleObject.setMax(rule["max"].GetFloat());
    if(rule.HasMember("numberFormat")) ruleObject.setNumberFormat(rule["numberFormat"].GetString());
    if(rule.HasMember("icon")) ruleObject.setIcon(rule["icon"].GetString());
    if(rule.HasMember("values")) ruleObject.setValues(rule["values"]);
    this->rule = ruleObject;
}
