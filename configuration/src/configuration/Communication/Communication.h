//
// Created by Andre Henriques on 2019-08-21.
//

#ifndef SAM_COORDINATOR_ARM_COMMUNICATION_H
#define SAM_COORDINATOR_ARM_COMMUNICATION_H

#include <iostream>
#include <vector>
#include "rapidjson/document.h"

using namespace std;
using namespace rapidjson;

class Communication {
private:
    string type;
    string ipAddress;
    string port;
public:
    vector<string> types = { "tcp/ip", "radio" };

    enum CommunicationTypesEnum {
        tcp = 0,
        radio
    };

    CommunicationTypesEnum communicationTypesEnum;

    Communication();

    Communication(string type, string ipAddress, string port);

    Communication(string type);

    vector<Communication> getVector(const Value &communications);

    const string &getType() const;

    void setType(const string &type);

    const string &getIpAddress() const;

    void setIpAddress(const string &ipAddress);

    const string &getPort() const;

    void setPort(const string &port);
};


#endif //SAM_COORDINATOR_ARM_COMMUNICATION_H
