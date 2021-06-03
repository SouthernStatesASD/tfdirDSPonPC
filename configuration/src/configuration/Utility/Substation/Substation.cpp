//
// Created by Andre Henriques on 2019-08-14.
//

#include "Substation.hpp"

#include <iostream>
#include <configuration/Utility/Substation/ElectricalAssetType/ElectricalAssetType.h>
#include <configuration/Utility/Substation/ElectricalAsset/ElectricalAsset.hpp>

using namespace std;
using namespace rapidjson;

Substation::Substation() {
    vector<Oneline> _oneline;
    vector<ElectricalAsset> _electricalAssets;
    vector<ElectricalAssetType> _electricalAssetTypes;
    id = NULL;
    name = "New Substation";
    geometry = Geometry();
    oneline = _oneline;
    electricalAssets = _electricalAssets;
    electricalAssetTypes = _electricalAssetTypes;
}

vector<Substation> Substation::getVector(const Value &substations) {
    vector<Substation> substationsVector;

    for (auto& sub : substations.GetArray()) {
        Substation substation;
        if(sub.HasMember("id")) substation.setId(sub["id"].GetUint64());
        if(sub.HasMember("name")) substation.setName(sub["name"].GetString());
        if(sub.HasMember("geometry")) substation.setGeometry(sub["geometry"]);
        if(sub.HasMember("oneline")) substation.setOneline(sub["oneline"]);
        if(sub.HasMember("electricalAssets")) substation.setElectricalAssets(sub["electricalAssets"]);
        if(sub.HasMember("electricalAssetTypes")) substation.setElectricalAssetTypes(sub["electricalAssetTypes"]);
        substationsVector.push_back(substation);
    }
    return substationsVector;
}

void Substation::setId(long id) {
    this->id = id;
}

long Substation::getId() {
    return this->id;
}

void Substation::setName(string name) {
    this->name = name;
}

string Substation::getName() {
    return this->name;
}

Geometry Substation::getGeometry() {
    return this->geometry;
}

void Substation::setGeometry(const Value &geometry) {
    Geometry geometryObject = Geometry(geometry);
    this->geometry = geometryObject;
}

vector<Oneline> Substation::getOneline() {
    return this->oneline;
}

void Substation::setOneline(const Value &oneline) {
    Oneline onelineObject;
    vector<Oneline> onelineVector = onelineObject.getVector(oneline);
    this->oneline = onelineVector;
}

vector<ElectricalAsset> Substation::getElectricalAssets() {
    return this->electricalAssets;
}

void Substation::setElectricalAssets(const Value &electricalAssets) {
    ElectricalAsset electricalAsset;
    vector<ElectricalAsset> electricalAssetsVector = electricalAsset.getVector(electricalAssets);
    this->electricalAssets = electricalAssetsVector;
}

vector<ElectricalAssetType> Substation::getElectricalAssetTypes() {
    return this->electricalAssetTypes;
}

void Substation::setElectricalAssetTypes(const Value &electricalAssetTypes) {
    ElectricalAssetType electricalAssetType;
    vector<ElectricalAssetType> electricalAssetTypeVector = electricalAssetType.getVector(electricalAssetTypes);
    Substation::electricalAssetTypes = electricalAssetTypeVector;
}

