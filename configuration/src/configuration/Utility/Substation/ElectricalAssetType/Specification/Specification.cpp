//
// Created by Andre Henriques on 2019-08-15.
//

#include "Specification.h"

Specification::Specification() {
    vector<int> frequency;
    vector<PowerRating> powerRatings;
    vector<int> BIL;
    vector<string> dischargeResistors;
    vector<int> temperatureRange;
    this->manufacture = "";
    this->model = "";
    this->shape = "";
    this->technology = "";
    this->ratedVoltage = NULL;
    this->frequency = frequency;
    this->powerRatings = powerRatings;
    this->maxCurrent = "";
    this->BIL = BIL;
    this->dielectric = "";
    this->impregnant = "";
    this->dischargeResistors = dischargeResistors;
    this->losses = NULL;
    this->temperatureRange = temperatureRange;
}

Specification::Specification(const Value &specification) {
    Specification specificationObject;
    if(specification.HasMember("manufacture")) specificationObject.setManufacture(specification["manufacture"].GetString());
    if(specification.HasMember("model")) specificationObject.setModel(specification["model"].GetString());
    if(specification.HasMember("shape")) specificationObject.setShape(specification["shape"].GetString());
    if(specification.HasMember("technology")) specificationObject.setTechnology(specification["technology"].GetString());
    if(specification.HasMember("ratedVoltage")) specificationObject.setRatedVoltage(specification["ratedVoltage"].GetInt());
    if(specification.HasMember("frequency")) specificationObject.setFrequency(specification["frequency"]);
    if(specification.HasMember("powerRatings")) specificationObject.setPowerRatings(specification["powerRatings"]);
    if(specification.HasMember("maxCurrent")) specificationObject.setMaxCurrent(specification["maxCurrent"].GetString());
    if(specification.HasMember("BIL")) specificationObject.setBil(specification["BIL"]);
    if(specification.HasMember("dielectric")) specificationObject.setDielectric(specification["dielectric"].GetString());
    if(specification.HasMember("impregnant")) specificationObject.setImpregnant(specification["impregnant"].GetString());
    if(specification.HasMember("dischargeResistors")) specificationObject.setDischargeResistors(specification["dischargeResistors"]);
    if(specification.HasMember("losses")) specificationObject.setLosses(specification["losses"].GetFloat());
    if(specification.HasMember("temperatureRange")) specificationObject.setTemperatureRange(specification["temperatureRange"]);

    this->manufacture = specificationObject.getManufacture();
    this->model = specificationObject.getModel();
    this->shape = specificationObject.getShape();
    this->technology = specificationObject.getTechnology();
    this->ratedVoltage = specificationObject.getRatedVoltage();
    this->frequency = specificationObject.getFrequency();
    this->powerRatings = specificationObject.getPowerRatings();
    this->maxCurrent = specificationObject.getMaxCurrent();
    this->BIL = specificationObject.getBil();
    this->dielectric = specificationObject.getDielectric();
    this->impregnant = specificationObject.getImpregnant();
    this->dischargeResistors = specificationObject.getDischargeResistors();
    this->losses = specificationObject.getLosses();
    this->temperatureRange = specificationObject.getTemperatureRange();
}

const string &Specification::getManufacture() const {
    return manufacture;
}

void Specification::setManufacture(const string &manufacture) {
    Specification::manufacture = manufacture;
}

const string &Specification::getModel() const {
    return model;
}

void Specification::setModel(const string &model) {
    Specification::model = model;
}

const string &Specification::getShape() const {
    return shape;
}

void Specification::setShape(const string &shape) {
    Specification::shape = shape;
}

const string &Specification::getTechnology() const {
    return technology;
}

void Specification::setTechnology(const string &technology) {
    Specification::technology = technology;
}

int Specification::getRatedVoltage() const {
    return ratedVoltage;
}

void Specification::setRatedVoltage(int ratedVoltage) {
    Specification::ratedVoltage = ratedVoltage;
}

const vector<int> &Specification::getFrequency() const {
    return frequency;
}

void Specification::setFrequency(const Value &frequency) {
    vector<int> frequencyVector;
    for(auto &thisFrequency : frequency.GetArray()) {
        int frequencyInt = thisFrequency.GetInt();
        frequencyVector.push_back(frequencyInt);
    }
    this->frequency = frequencyVector;
}

const vector<PowerRating> &Specification::getPowerRatings() const {
    return powerRatings;
}

void Specification::setPowerRatings(const Value &powerRatings) {
    vector<PowerRating> powerRatingsVector;
    for(auto &thisPowerRating : powerRatings.GetArray()) {
        PowerRating powerRating;
        if(thisPowerRating.HasMember("kvar")) powerRating.setKvar(thisPowerRating["kvar"].GetInt());
        if(thisPowerRating.HasMember("frequency")) powerRating.setFrequency(thisPowerRating["frequency"].GetInt());
        powerRatingsVector.push_back(powerRating);
    }
    this->powerRatings = powerRatingsVector;
}

const string &Specification::getMaxCurrent() const {
    return maxCurrent;
}

void Specification::setMaxCurrent(const string &maxCurrent) {
    Specification::maxCurrent = maxCurrent;
}

const vector<int> &Specification::getBil() const {
    return BIL;
}

void Specification::setBil(const Value &bil) {
    vector<int> bilVector;
    for(auto &thisBil : bil.GetArray()) {
        int bilInt = thisBil.GetInt();
        bilVector.push_back(bilInt);
    }
    this->BIL = bilVector;
}

const string &Specification::getDielectric() const {
    return dielectric;
}

void Specification::setDielectric(const string &dielectric) {
    Specification::dielectric = dielectric;
}

const string &Specification::getImpregnant() const {
    return impregnant;
}

void Specification::setImpregnant(const string &impregnant) {
    Specification::impregnant = impregnant;
}

const vector<string> &Specification::getDischargeResistors() const {
    return dischargeResistors;
}

void Specification::setDischargeResistors(const Value &dischargeResistors) {
    vector<string> dischargeResistorsVector;
    for(auto &thisDischargeResistor : dischargeResistors.GetArray()) {
        string dischargeResistorString = thisDischargeResistor.GetString();
        dischargeResistorsVector.push_back(dischargeResistorString);
    }
    this->dischargeResistors = dischargeResistorsVector;
}

float Specification::getLosses() const {
    return losses;
}

void Specification::setLosses(float losses) {
    Specification::losses = losses;
}

const vector<int> &Specification::getTemperatureRange() const {
    return temperatureRange;
}

void Specification::setTemperatureRange(const Value &temperatureRange) {
    vector<int> temperatureRangeVector;
    for(auto &thisTemperatureRange : temperatureRange.GetArray()) {
        int temperatureRangeInt = thisTemperatureRange.GetInt();
        temperatureRangeVector.push_back(temperatureRangeInt);
    }
    this->temperatureRange = temperatureRangeVector;
}
