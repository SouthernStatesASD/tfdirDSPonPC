//
// Created by Andre Henriques on 2019-08-14.
//

#include "Coordinate.hpp"

Coordinate::Coordinate(string lat, string lng, string alt) {
    this -> lat = lat;
    this -> lng = lng;
    this -> alt = alt;
}

Coordinate::Coordinate() {
    this -> lat = "";
    this -> lng = "";
    this -> alt = "";
}

string Coordinate::getLat() {
    return lat;
}

void Coordinate::setLat(string lat) {
    Coordinate::lat = lat;
}

string Coordinate::getLng() {
    return lng;
}

void Coordinate::setLng(string lng) {
    Coordinate::lng = lng;
}

string Coordinate::getAlt() {
    return alt;
}

void Coordinate::setAlt(string alt) {
    Coordinate::alt = alt;
}
