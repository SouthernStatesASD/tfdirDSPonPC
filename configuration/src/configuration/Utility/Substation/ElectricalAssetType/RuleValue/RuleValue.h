//
// Created by Andre Henriques on 2019-08-15.
//

#ifndef SAM_COORDINATOR_ARM_RULEVALUE_H
#define SAM_COORDINATOR_ARM_RULEVALUE_H

#include <iostream>

using namespace std;

class RuleValue {
private:
    string value;
    string displayName;
    string color;
public:
    RuleValue();

    const string &getValue() const;

    void setValue(const string &value);

    const string &getDisplayName() const;

    void setDisplayName(const string &displayName);

    const string &getColor() const;

    void setColor(const string &color);
};


#endif //SAM_COORDINATOR_ARM_RULEVALUE_H
