//
// Created by Andre Henriques on 2019-08-15.
//

#include "Model.h"

Model::Model() {
    this->electrical = "";
    this->threeDimentional = "";
}

const string &Model::getElectrical() const {
    return electrical;
}

void Model::setElectrical(const string &electrical) {
    Model::electrical = electrical;
}

const string &Model::getThreeDimentional() const {
    return threeDimentional;
}

void Model::setThreeDimentional(const string &threeDimentional) {
    Model::threeDimentional = threeDimentional;
}
