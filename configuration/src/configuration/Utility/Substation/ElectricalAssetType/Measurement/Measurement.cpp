//
// Created by Andre Henriques on 2019-08-15.
//

#include "Measurement.h"

Measurement::Measurement() {
    this->field = "";
    this->controlType = "";
    this->showOnList = Show();
    this->rule = Rule();
}

const string &Measurement::getField() const {
    return field;
}

void Measurement::setField(const string &field) {
    Measurement::field = field;
}

const string &Measurement::getControlType() const {
    return controlType;
}

void Measurement::setControlType(const string &controlType) {
    Measurement::controlType = controlType;
}

const Show &Measurement::getShowOnList() const {
    return showOnList;
}

void Measurement::setShowOnList(const Value &showOnList) {
    Show showOnListObject;
    if(showOnList.HasMember("cellColor")) showOnListObject.setCellColor(showOnList["cellColor"].GetBool());
    if(showOnList.HasMember("icon")) showOnListObject.setIcon(showOnList["icon"].GetBool());
    if(showOnList.HasMember("value")) showOnListObject.setValue(showOnList["value"].GetBool());
    Measurement::showOnList = showOnListObject;
}

const Rule &Measurement::getRule() const {
    return rule;
}

void Measurement::setRule(const Value &rule) {
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
