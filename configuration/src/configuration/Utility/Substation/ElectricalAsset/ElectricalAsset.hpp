//
// Created by Andre Henriques on 2019-08-15.
//

#ifndef SAM_COORDINATOR_ARM_ELECTRICALASSET_HPP
#define SAM_COORDINATOR_ARM_ELECTRICALASSET_HPP

#include <configuration/Communication/Communication.h>
#include <iostream>
#include <vector>
#include <configuration/Utility/Substation/Oneline/Oneline.hpp>
#include <configuration/Utility/Substation/ElectricalAsset/Setting/Setting.h>
#include <configuration/Utility/Substation/ElectricalAsset/Terminal/Terminal.h>
#include "configuration/Utility/Substation/Geometry/Geometry.hpp"

using namespace std;
using namespace rapidjson;

class ElectricalAsset {
private:
    long id;
    long typeId;
    Setting settings;
    vector<Communication> communications;
    Geometry geometry;
    vector<Oneline> oneline;
    vector<Terminal> terminals;
public:
    ElectricalAsset();

    vector<ElectricalAsset> getVector(const Value &electricalAssets);

    long getId() const;

    void setId(long id);

    const vector<Communication> &getCommunications() const;

    void setCommunications(const Value &communications);

    long getTypeId() const;

    void setTypeId(long typeId);

    const Setting &getSettings() const;

    void setSettings(const Value &settings);

    const Geometry &getGeometry() const;

    void setGeometry(const Value &geometry);

    const vector<Oneline> &getOneline() const;

    void setOneline(const Value &oneline);

    const vector<Terminal> &getTerminals() const;

    void setTerminals(const Value &terminals);
};


#endif //SAM_COORDINATOR_ARM_ELECTRICALASSET_HPP
