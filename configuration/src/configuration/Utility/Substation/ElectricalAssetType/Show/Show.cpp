//
// Created by Andre Henriques on 2019-08-15.
//

#include "Show.h"

Show::Show() {
    this->cellColor = false;
    this->icon = false;
    this->value = false;
}

bool Show::isCellColor() const {
    return cellColor;
}

void Show::setCellColor(bool cellColor) {
    Show::cellColor = cellColor;
}

bool Show::isIcon() const {
    return icon;
}

void Show::setIcon(bool icon) {
    Show::icon = icon;
}

bool Show::isValue() const {
    return value;
}

void Show::setValue(bool value) {
    Show::value = value;
}
