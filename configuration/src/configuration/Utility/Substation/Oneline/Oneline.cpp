//
// Created by Andre Henriques on 2019-08-14.
//

#include <utility>
#include <vector>
#include "Oneline.hpp"

Oneline::Oneline(string _x, string _y, string _z) {
    x = move(_x);
    y = move(_y);
    z = move(_z);
}

Oneline::Oneline() {
    this -> x = "";
    this -> y = "";
    this -> z = "";
}

vector<Oneline> Oneline::getVector(const Value &oneline) {
    vector<Oneline> onelineVector;
    for (auto& thisOneline : oneline.GetArray()) {
        Oneline point;
        if(thisOneline.HasMember("x")) point.setX(thisOneline["x"].GetString());
        if(thisOneline.HasMember("y")) point.setY(thisOneline["y"].GetString());
        if(thisOneline.HasMember("z")) point.setZ(thisOneline["z"].GetString());
        onelineVector.push_back(point);
    }
    return onelineVector;
}

string Oneline::getX() {
    return x;
}

void Oneline::setX(string x) {
    this->x = x;
}

string Oneline::getY() {
    return y;
}

void Oneline::setY(string y) {
    this->y = y;
}

string Oneline::getZ() {
    return z;
}

void Oneline::setZ(string z) {
    this->z = z;
}