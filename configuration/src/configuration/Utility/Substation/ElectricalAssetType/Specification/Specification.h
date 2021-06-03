//
// Created by Andre Henriques on 2019-08-15.
//

#ifndef SAM_COORDINATOR_ARM_SPECIFICATION_H
#define SAM_COORDINATOR_ARM_SPECIFICATION_H

#include <iostream>
#include <vector>
#include "rapidjson/document.h"
#include "configuration/Utility/Substation/ElectricalAssetType/PowerRating/PowerRating.h"

using namespace std;
using namespace rapidjson;

class Specification {
private:
    string manufacture;
    string model;
    string shape;
    string technology;
    int ratedVoltage;
    vector<int> frequency;
    vector<PowerRating> powerRatings;
    string maxCurrent;
    vector<int> BIL;
    string dielectric;
    string impregnant;
    vector<string> dischargeResistors;
    float losses;
    vector<int> temperatureRange;
public:
    Specification();

    Specification(const Value &specification);

    const string &getManufacture() const;

    void setManufacture(const string &manufacture);

    const string &getModel() const;

    void setModel(const string &model);

    const string &getShape() const;

    void setShape(const string &shape);

    const string &getTechnology() const;

    void setTechnology(const string &technology);

    int getRatedVoltage() const;

    void setRatedVoltage(int ratedVoltage);

    const vector<int> &getFrequency() const;

    void setFrequency(const Value &frequency);

    const vector<PowerRating> &getPowerRatings() const;

    void setPowerRatings(const Value &powerRatings);

    const string &getMaxCurrent() const;

    void setMaxCurrent(const string &maxCurrent);

    const vector<int> &getBil() const;

    void setBil(const Value &bil);

    const string &getDielectric() const;

    void setDielectric(const string &dielectric);

    const string &getImpregnant() const;

    void setImpregnant(const string &impregnant);

    const vector<string> &getDischargeResistors() const;

    void setDischargeResistors(const Value &dischargeResistors);

    float getLosses() const;

    void setLosses(float losses);

    const vector<int> &getTemperatureRange() const;

    void setTemperatureRange(const Value &temperatureRange);
};


#endif //SAM_COORDINATOR_ARM_SPECIFICATION_H
