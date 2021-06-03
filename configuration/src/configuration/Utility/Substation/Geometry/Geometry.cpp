//
// Created by Andre Henriques on 2019-08-14.
//

#include "Geometry.hpp"

Geometry::Geometry() {
    vector<Coordinate> newCoordinates;
    this -> orientation = "";
    this -> scale = "";
    this -> lat = "";
    this -> lng = "";
    this -> alt = "";
    this -> coordinates = newCoordinates;
}

Geometry::Geometry(const Value &geometry) {
    Geometry geometryObject;
    if(geometry.HasMember("orientation")) geometryObject.setOrientation(geometry["orientation"].GetString());
    if(geometry.HasMember("scale")) geometryObject.setScale(geometry["scale"].GetString());
    if(geometry.HasMember("lat")) geometryObject.setLat(geometry["lat"].GetString());
    if(geometry.HasMember("lng")) geometryObject.setLng(geometry["lng"].GetString());
    if(geometry.HasMember("alt")) geometryObject.setAlt(geometry["alt"].GetString());
    if(geometry.HasMember("coordinates")) geometryObject.setCoordinates(geometry["coordinates"]);

    this -> orientation = geometryObject.getOrientation();
    this -> scale = geometryObject.getScale();
    this -> lat = geometryObject.getLat();
    this -> lng = geometryObject.getLng();
    this -> alt = geometryObject.getAlt();
    this -> coordinates = geometryObject.getCoordinates();
}

Geometry::Geometry(string orientation, string scale, string lat, string lng, string alt, vector<Coordinate> coordinates) {
    if(!orientation.empty()) this -> orientation = orientation;
    if(!scale.empty()) this -> scale = scale;
    if(!lat.empty()) this -> lat = lat;
    if(!lng.empty()) this -> lng = lng;
    if(!alt.empty()) this -> alt = alt;
    if(coordinates.size() > 0) this -> coordinates = coordinates;

}

string Geometry::getOrientation() {
    return orientation;
}

void Geometry::setOrientation(string orientation) {
    this->orientation = orientation;
}

string Geometry::getScale() {
    return scale;
}

void Geometry::setScale(string scale) {
    this->scale = scale;
}

string Geometry::getLat() {
    return lat;
}

void Geometry::setLat(string lat) {
    this->lat = lat;
}

string Geometry::getLng() {
    return lng;
}

void Geometry::setLng(string lng) {
    this->lng = lng;
}

string Geometry::getAlt() {
    return alt;
}

void Geometry::setAlt(string alt) {
    this->alt = alt;
}

vector<Coordinate> Geometry::getCoordinates() {
    return coordinates;
}

void Geometry::setCoordinates(const Value &coordinates) {
    vector<Coordinate> coordinatesVector;
    for (auto& thisCoordinate : coordinates.GetArray()) {
        Coordinate coordinate;
        if(thisCoordinate.HasMember("lat")) coordinate.setLat(thisCoordinate["lat"].GetString());
        if(thisCoordinate.HasMember("lng")) coordinate.setLng(thisCoordinate["lng"].GetString());
        if(thisCoordinate.HasMember("alt")) coordinate.setAlt(thisCoordinate["alt"].GetString());
        coordinatesVector.push_back(coordinate);
    }
    this->coordinates = coordinatesVector;
}
