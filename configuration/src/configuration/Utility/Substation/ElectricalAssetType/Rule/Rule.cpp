//
// Created by Andre Henriques on 2019-08-15.
//

#include "Rule.h"

Rule::Rule() {
    vector<RuleValue> values;
    this->required = false;
    this->disabled = false;
    this->controlType = "textfield";
    this->dataType = "string";
    this->numericType = "discrete";
    this->prefix = "";
    this->suffix = "";
    this->scalar = 1.0;
    this->min = NULL;
    this->max = NULL;
    this->numberFormat = "1.0-0";
    this->icon = "";
    this->values = values;
}

bool Rule::isRequired() const {
    return required;
}

void Rule::setRequired(bool required) {
    Rule::required = required;
}

bool Rule::isDisabled() const {
    return disabled;
}

void Rule::setDisabled(bool disabled) {
    Rule::disabled = disabled;
}

const string &Rule::getControlType() const {
    return controlType;
}

void Rule::setControlType(const string &controlType) {
    Rule::controlType = controlType;
}

const string &Rule::getDataType() const {
    return dataType;
}

void Rule::setDataType(const string &dataType) {
    Rule::dataType = dataType;
}

const string &Rule::getNumericType() const {
    return numericType;
}

void Rule::setNumericType(const string &numericType) {
    Rule::numericType = numericType;
}

const string &Rule::getPrefix() const {
    return prefix;
}

void Rule::setPrefix(const string &prefix) {
    Rule::prefix = prefix;
}

const string &Rule::getSuffix() const {
    return suffix;
}

void Rule::setSuffix(const string &suffix) {
    Rule::suffix = suffix;
}

float Rule::getScalar() const {
    return scalar;
}

void Rule::setScalar(float scalar) {
    Rule::scalar = scalar;
}

float Rule::getMin() const {
    return min;
}

void Rule::setMin(float min) {
    Rule::min = min;
}

float Rule::getMax() const {
    return max;
}

void Rule::setMax(float max) {
    Rule::max = max;
}

const string &Rule::getNumberFormat() const {
    return numberFormat;
}

void Rule::setNumberFormat(const string &numberFormat) {
    Rule::numberFormat = numberFormat;
}

const string &Rule::getIcon() const {
    return icon;
}

void Rule::setIcon(const string &icon) {
    Rule::icon = icon;
}

const vector<RuleValue> &Rule::getValues() const {
    return values;
}

void Rule::setValues(const Value &values) {
    vector<RuleValue> valuesVector;
    for(auto& thisValue : values.GetArray()) {
        RuleValue value;
        if(thisValue.HasMember("value")) value.setValue(thisValue["value"].GetString());
        if(thisValue.HasMember("displayName")) value.setDisplayName(thisValue["displayName"].GetString());
        if(thisValue.HasMember("color")) value.setColor(thisValue["color"].GetString());
        valuesVector.push_back(value);
    }
    this->values = valuesVector;
}
