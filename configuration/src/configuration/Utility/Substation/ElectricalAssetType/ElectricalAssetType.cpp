//
// Created by Andre Henriques on 2019-08-15.
//

#include "ElectricalAssetType.h"

ElectricalAssetType::ElectricalAssetType() {
    vector<Measurement> measurements;
    vector<Tab> tabs;
    this->id = NULL;
    this->type = "";
    this->measurements = measurements;
    this->specification = Specification();
    this->tabs = tabs;
}

vector<ElectricalAssetType> ElectricalAssetType::getVector(const Value &electricalAssetTypes) {
    vector<ElectricalAssetType> electricalAssetTypeVector;
    for (auto& thisAsset : electricalAssetTypes.GetArray()) {
        ElectricalAssetType assetType;
        if(thisAsset.HasMember("id")) assetType.setId(thisAsset["id"].GetUint64());
        if(thisAsset.HasMember("type")) assetType.setType(thisAsset["type"].GetString());
        if(thisAsset.HasMember("measurements")) assetType.setMeasurements(thisAsset["measurements"]);
        if(thisAsset.HasMember("model")) assetType.setModel(thisAsset["model"]);
        if(thisAsset.HasMember("specification")) assetType.setSpecification(thisAsset["specification"]);
        if(thisAsset.HasMember("tabs")) assetType.setTabs(thisAsset["tabs"]);
        electricalAssetTypeVector.push_back(assetType);
    }
    return electricalAssetTypeVector;
}

long ElectricalAssetType::getId() const {
    return id;
}

void ElectricalAssetType::setId(long id) {
    ElectricalAssetType::id = id;
}

const string &ElectricalAssetType::getType() const {
    return type;
}

void ElectricalAssetType::setType(const string &type) {
    ElectricalAssetType::type = type;
}

const vector<Measurement> &ElectricalAssetType::getMeasurements() const {
    return measurements;
}

void ElectricalAssetType::setMeasurements(const Value &measurements) {
    vector<Measurement> measurementsVector;
    for(auto &thisMeasurement : measurements.GetArray()) {
        Measurement measurement;
        if(thisMeasurement.HasMember("controlType")) measurement.setControlType(thisMeasurement["controlType"].GetString());
        if(thisMeasurement.HasMember("field")) measurement.setField(thisMeasurement["field"].GetString());
        if(thisMeasurement.HasMember("rule")) measurement.setRule(thisMeasurement["rule"]);
        if(thisMeasurement.HasMember("showOnList")) measurement.setShowOnList(thisMeasurement["showOnList"]);
        measurementsVector.push_back(measurement);
    }
    ElectricalAssetType::measurements = measurementsVector;
}

const Model &ElectricalAssetType::getModel() const {
    return model;
}

void ElectricalAssetType::setModel(const Value &model) {
    Model modelObject;
    if(model.HasMember("electrical")) modelObject.setElectrical(model["electrical"].GetString());
    if(model.HasMember("threeDimentional")) modelObject.setThreeDimentional(model["threeDimentional"].GetString());
    ElectricalAssetType::model = modelObject;
}

const Specification &ElectricalAssetType::getSpecification() const {
    return specification;
}

void ElectricalAssetType::setSpecification(const Value &specification) {
    Specification specificationObject = Specification(specification);
    this->specification = specificationObject;
}

const vector<Tab> &ElectricalAssetType::getTabs() const {
    return tabs;
}

void ElectricalAssetType::setTabs(const Value &tabs) {
    Tab tab;
    vector<Tab> tabsVector = tab.getVector(tabs);
    this->tabs = tabsVector;
}
