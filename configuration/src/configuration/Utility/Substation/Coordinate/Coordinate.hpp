//
// Created by Andre Henriques on 2019-08-14.
//

#ifndef SAM_COORDINATOR_ARM_COORDINATE_HPP
#define SAM_COORDINATOR_ARM_COORDINATE_HPP

#include <iostream>

using namespace std;

class Coordinate {
private:
    string lat;
    string lng;
    string alt;
public:
    Coordinate();

    Coordinate(string lat, string lng, string alt);

     string getLat();

    void setLat( string lat);

     string getLng();

    void setLng( string lng);

     string getAlt();

    void setAlt( string alt);
};


#endif //SAM_COORDINATOR_ARM_COORDINATE_HPP
