//
// Created by Andre Henriques on 2019-08-15.
//

#ifndef SAM_COORDINATOR_ARM_RULE_H
#define SAM_COORDINATOR_ARM_RULE_H

#include <iostream>
#include <vector>
#include "configuration/Utility/Substation/ElectricalAssetType/RuleValue/RuleValue.h"
#include "rapidjson/document.h"

using namespace std;
using namespace rapidjson;

class Rule {
private:
    bool required;
    bool disabled;
    string controlType;
    string dataType;
    string numericType;
    string prefix;
    string suffix;
    float scalar;
    float min;
    float max;
    string numberFormat;
    string icon;
    vector<RuleValue> values;

public:
    Rule();

    bool isRequired() const;

    void setRequired(bool required);

    bool isDisabled() const;

    void setDisabled(bool disabled);

    const string &getControlType() const;

    void setControlType(const string &controlType);

    const string &getDataType() const;

    void setDataType(const string &dataType);

    const string &getNumericType() const;

    void setNumericType(const string &numericType);

    const string &getPrefix() const;

    void setPrefix(const string &prefix);

    const string &getSuffix() const;

    void setSuffix(const string &suffix);

    float getScalar() const;

    void setScalar(float scalar);

    float getMin() const;

    void setMin(float min);

    float getMax() const;

    void setMax(float max);

    const string &getNumberFormat() const;

    void setNumberFormat(const string &numberFormat);

    const string &getIcon() const;

    void setIcon(const string &icon);

    const vector<RuleValue> &getValues() const;

    void setValues(const Value &values);
};


#endif //SAM_COORDINATOR_ARM_RULE_H
