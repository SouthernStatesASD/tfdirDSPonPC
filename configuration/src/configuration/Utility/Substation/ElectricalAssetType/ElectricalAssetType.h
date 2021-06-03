//
// Created by Andre Henriques on 2019-08-15.
//

#ifndef SAM_COORDINATOR_ARM_ELECTRICALASSETTYPE_H
#define SAM_COORDINATOR_ARM_ELECTRICALASSETTYPE_H

#include <iostream>
#include <vector>
#include "rapidjson/document.h"
#include <configuration/Utility/Substation/Oneline/Oneline.hpp>
#include <configuration/Utility/Substation/ElectricalAsset/Setting/Setting.h>
#include <configuration/Utility/Substation/ElectricalAsset/Terminal/Terminal.h>
#include <configuration/Utility/Substation/ElectricalAssetType/Measurement/Measurement.h>
#include <configuration/Utility/Substation/ElectricalAssetType/Model/Model.h>
#include <configuration/Utility/Substation/ElectricalAssetType/Specification/Specification.h>
#include <configuration/Utility/Substation/ElectricalAssetType/Tab/Tab.h>
#include "configuration/Utility/Substation/Geometry/Geometry.hpp"

using namespace std;
using namespace rapidjson;

class ElectricalAssetType {
private:
    long id;
    string type;
    vector<Measurement> measurements;
    Model model;
    Specification specification;
    vector<Tab> tabs;
public:
    ElectricalAssetType();

    vector<ElectricalAssetType> getVector(const Value &electricalAssetTypes);

    long getId() const;

    void setId(long id);

    const string &getType() const;

    void setType(const string &type);

    const vector<Measurement> &getMeasurements() const;

    void setMeasurements(const Value &measurements);

    const Model &getModel() const;

    void setModel(const Value &model);

    const Specification &getSpecification() const;

    void setSpecification(const Value &specification);

    const vector<Tab> &getTabs() const;

    void setTabs(const Value &tabs);
};


#endif //SAM_COORDINATOR_ARM_ELECTRICALASSETTYPE_H
