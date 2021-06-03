//
// Created by Andre Henriques on 2019-08-14.
//

#ifndef SAM_COORDINATOR_ARM_GEOMETRY_HPP
#define SAM_COORDINATOR_ARM_GEOMETRY_HPP


#include <iostream>
#include <vector>
#include "rapidjson/document.h"
#include "configuration/Utility/Substation/Coordinate/Coordinate.hpp"

using namespace std;
using namespace rapidjson;

class Geometry {
private:
    string orientation;
    string scale;
    string lat;
    string lng;
    string alt;
    vector<Coordinate> coordinates;
public:
    Geometry();

    Geometry(const Value &geometry);

    Geometry(string orientation, string scale, string lat, string lng, string alt, vector<Coordinate> coordinates);

    string getOrientation();

    void setOrientation(string orientation);

    string getScale();

    void setScale(string scale);

    string getLat();

    void setLat(string lat);

    string getLng();

    void setLng(string lng);

    string getAlt();

    void setAlt(string alt);

    vector<Coordinate> getCoordinates();

    void setCoordinates(const Value &coordinates);
};


#endif //SAM_COORDINATOR_ARM_GEOMETRY_HPP
