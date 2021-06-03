//
// Created by Andre Henriques on 2019-08-21.
//

#include "Communication.h"

Communication::Communication() {
    type = types[tcp];
    ipAddress = "127.0.0.1";
    port = "5556";
}

Communication::Communication(string _type, string _ipAddress, string _port) {
    type = _type;
    ipAddress = _ipAddress;
    port = _port;
}

Communication::Communication(string _type) {
    type = _type;
}

vector<Communication> Communication::getVector(const Value &communications) {
    vector<Communication> communicationsVector;
    for(auto &thisCommunications : communications.GetArray()) {
        Communication communication;
        if(thisCommunications.HasMember("type")) communication.setType(thisCommunications["type"].GetString());
        if(thisCommunications.HasMember("ipAddress")) communication.setIpAddress(thisCommunications["ipAddress"].GetString());
        if(thisCommunications.HasMember("port")) communication.setPort(thisCommunications["port"].GetString());
        communicationsVector.push_back(communication);
    }
    return communicationsVector;
}

const string &Communication::getType() const {
    return type;
}

void Communication::setType(const string &type) {
    Communication::type = type;
}

const string &Communication::getIpAddress() const {
    return ipAddress;
}

void Communication::setIpAddress(const string &ipAddress) {
    Communication::ipAddress = ipAddress;
}

const string &Communication::getPort() const {
    return port;
}

void Communication::setPort(const string &port) {
    Communication::port = port;
}
