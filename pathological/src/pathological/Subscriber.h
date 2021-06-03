//
// Created by Andre Henriques on 2019-08-16.
//

#ifndef SAM_COORDINATOR_ARM_SUBSCRIBER_H
#define SAM_COORDINATOR_ARM_SUBSCRIBER_H

#include <iostream>
#include "zhelpers.hpp"

using namespace std;

class Subscriber {
private:
    zmq::socket_t socket;
    stringstream stream;

public:
    Subscriber(zmq::context_t *context, const string& address);

    const zmq::socket_t &getSocket() const;

//    void setSocket(const zmq::socket_t &socket);

    const stringstream &getStream() const;

//    void setStream(const stringstream &stream);
};


#endif //SAM_COORDINATOR_ARM_SUBSCRIBER_H
