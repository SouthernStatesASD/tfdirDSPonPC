//
// Created by Andre Henriques on 2019-08-13.
//

#include "Utility.hpp"
#include <iostream>
#include "configuration/Utility/Substation/Geometry/Geometry.hpp"

using namespace std;
using namespace rapidjson;

Utility::Utility(long id, string name, vector<Substation> substations) {
    this -> id = id;
    this -> name = name;
    this -> substations = substations;
}

Utility::Utility() {
    vector<Substation> newVector;
    this -> id = NULL;
    this -> name = "New Utility";
    this -> substations = newVector;
}

Utility::Utility(const Value &utility) {
    Utility utilityObject;
    if (utility.HasMember("id")) utilityObject.setId(utility["id"].GetUint64());
    if (utility.HasMember("name")) utilityObject.setName(utility["name"].GetString());
    if (utility.HasMember("substations")) utilityObject.setSubstations(utility["substations"]);

    this -> id = utilityObject.getId();
    this -> name = utilityObject.getName();
    this -> substations = utilityObject.getSubstations();
}

void Utility::setId(long id) {
    this->id = id;
}

long Utility::getId() {
    return this->id;
}

void Utility::setName(string name) {
    this->name = name;
}

string Utility::getName() {
    return this->name;
}

void Utility::setSubstations(const Value &substations) {
    Substation substation;
    vector<Substation> substationsVector = substation.getVector(substations);
    this->substations = substationsVector;
}

vector<Substation> Utility::getSubstations() const {
    return this->substations;
}