//
// Created by Andre Henriques on 2019-08-15.
//

#include "RuleValue.h"

RuleValue::RuleValue() {
    this->value = "";
    this->displayName = "";
    this->color = "";
}

const string &RuleValue::getValue() const {
    return value;
}

void RuleValue::setValue(const string &value) {
    RuleValue::value = value;
}

const string &RuleValue::getDisplayName() const {
    return displayName;
}

void RuleValue::setDisplayName(const string &displayName) {
    RuleValue::displayName = displayName;
}

const string &RuleValue::getColor() const {
    return color;
}

void RuleValue::setColor(const string &color) {
    RuleValue::color = color;
}
