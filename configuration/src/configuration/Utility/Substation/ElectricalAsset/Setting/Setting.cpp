//
// Created by Andre Henriques on 2019-08-15.
//

#include "Setting.h"


Setting::Setting(long serialNumber) {
    this -> serialNumber = serialNumber;
}

Setting::Setting() {
    this -> serialNumber = NULL;
}

long Setting::getSerialNumber() {
    return serialNumber;
}

void Setting::setSerialNumber(long serialNumber) {
    this->serialNumber = serialNumber;
}