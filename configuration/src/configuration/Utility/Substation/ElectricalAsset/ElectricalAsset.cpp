//
// Created by Andre Henriques on 2019-08-15.
//

#include "ElectricalAsset.hpp"


ElectricalAsset::ElectricalAsset() {
    vector<Communication> _communications;
    vector<Oneline> _oneline;
    vector<Terminal> _terminals;

    id = NULL;
    typeId = NULL;
    settings = Setting();
    geometry = Geometry();
    communications = _communications;
    oneline = _oneline;
    terminals = _terminals;
}

vector<ElectricalAsset> ElectricalAsset::getVector(const Value &electricalAssets) {
    vector<ElectricalAsset> electricalAssetsVector;
    for (auto &thisAsset : electricalAssets.GetArray()) {
        ElectricalAsset asset;
        if (thisAsset.HasMember("id")) asset.setId(thisAsset["id"].GetUint64());
        if (thisAsset.HasMember("typeId")) asset.setTypeId(thisAsset["typeId"].GetUint64());
        if (thisAsset.HasMember("communications")) asset.setCommunications(thisAsset["communications"]);
        if (thisAsset.HasMember("settings")) asset.setSettings(thisAsset["settings"]);
        if (thisAsset.HasMember("geometry")) asset.setGeometry(thisAsset["geometry"]);
        if (thisAsset.HasMember("oneline")) asset.setOneline(thisAsset["oneline"]);
        if (thisAsset.HasMember("terminals")) asset.setTerminals(thisAsset["terminals"]);
        electricalAssetsVector.push_back(asset);
    }
    return electricalAssetsVector;
}

long ElectricalAsset::getId() const {
    return id;
}

void ElectricalAsset::setId(long id) {
    this->id = id;
}

long ElectricalAsset::getTypeId() const {
    return typeId;
}

void ElectricalAsset::setTypeId(long typeId) {
    this->typeId = typeId;
}

const Setting &ElectricalAsset::getSettings() const {
    return settings;
}

void ElectricalAsset::setSettings(const Value &settings) {
    Setting setting;
    setting.setSerialNumber(settings["serialNumber"].GetUint64());
    this->settings = setting;
}

const Geometry &ElectricalAsset::getGeometry() const {
    return geometry;
}

void ElectricalAsset::setGeometry(const Value &geometry) {
    Geometry geometryObject;
    if (geometry.HasMember("orientation")) geometryObject.setOrientation(geometry["orientation"].GetString());
    if (geometry.HasMember("scale")) geometryObject.setScale(geometry["scale"].GetString());
    if (geometry.HasMember("lat")) geometryObject.setLat(geometry["lat"].GetString());
    if (geometry.HasMember("lng")) geometryObject.setLng(geometry["lng"].GetString());
    if (geometry.HasMember("alt")) geometryObject.setAlt(geometry["alt"].GetString());
    if (geometry.HasMember("coordinates")) geometryObject.setCoordinates(geometry["coordinates"]);

    this->geometry = geometryObject;
}

const vector<Oneline> &ElectricalAsset::getOneline() const {
    return oneline;
}

void ElectricalAsset::setOneline(const Value &_oneline) {
    Oneline onelineObject;
    vector<Oneline> onelineVector = onelineObject.getVector(_oneline);
    oneline = onelineVector;
}

const vector<Terminal> &ElectricalAsset::getTerminals() const {
    return terminals;
}

void ElectricalAsset::setTerminals(const Value &terminals) {
    Terminal terminal;
    vector<Terminal> terminalsVector = terminal.getVector(terminals);
    this->terminals = terminalsVector;
}

const vector<Communication> &ElectricalAsset::getCommunications() const {
    return communications;
}

void ElectricalAsset::setCommunications(const Value &_communications) {
    Communication communication;
    vector<Communication> communicationsVector;
    communicationsVector = communication.getVector(_communications);
    communications = communicationsVector;
}
