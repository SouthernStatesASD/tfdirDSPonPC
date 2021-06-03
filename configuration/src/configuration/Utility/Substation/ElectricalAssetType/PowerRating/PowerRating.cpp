//
// Created by Andre Henriques on 2019-08-15.
//

#include "PowerRating.h"

PowerRating::PowerRating() {
    this->kvar = NULL;
    this->frequency = NULL;
}

int PowerRating::getKvar() const {
    return kvar;
}

void PowerRating::setKvar(int kvar) {
    PowerRating::kvar = kvar;
}

int PowerRating::getFrequency() const {
    return frequency;
}

void PowerRating::setFrequency(int frequency) {
    PowerRating::frequency = frequency;
}
