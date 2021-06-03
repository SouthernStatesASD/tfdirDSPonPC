//
// Created by Andre Henriques on 2019-08-14.
//

#ifndef SAM_COORDINATOR_ARM_SUBSTATION_HPP
#define SAM_COORDINATOR_ARM_SUBSTATION_HPP

#include <iostream>
#include "rapidjson/document.h"
#include <configuration/Utility/Substation/Oneline/Oneline.hpp>
#include <configuration/Utility/Substation/ElectricalAsset/ElectricalAsset.hpp>
#include <configuration/Utility/Substation/ElectricalAssetType/ElectricalAssetType.h>
#include "configuration/Utility/Substation/Geometry/Geometry.hpp"

using namespace std;
using namespace rapidjson;

class Substation {
private:
    long id;
    string name;
    Geometry geometry;
    vector<Oneline> oneline;
    vector<ElectricalAsset> electricalAssets;
    vector<ElectricalAssetType> electricalAssetTypes;
public:
    Substation();

    vector<Substation> getVector(const Value &substations);

    void setId(long id);

    void setName(string name);

    long getId();

    string getName();

    Geometry getGeometry();

    void setGeometry(const Value &geometry);

    vector<Oneline> getOneline();

    void setOneline(const Value &oneline);

    vector<ElectricalAsset> getElectricalAssets();

    void setElectricalAssets(const Value &electricalAssets);

    vector<ElectricalAssetType> getElectricalAssetTypes();

    void setElectricalAssetTypes(const Value &electricalAssetTypes);
};


#endif //SAM_COORDINATOR_ARM_SUBSTATION_HPP
