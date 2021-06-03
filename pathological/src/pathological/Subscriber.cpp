//
// Created by Andre Henriques on 2019-08-16.
//

#include "Subscriber.h"

Subscriber::Subscriber(zmq::context_t *context, const string& address) {
    socket = zmq::socket_t(*context, ZMQ_SUB);
    socket.connect(address);
}

const zmq::socket_t &Subscriber::getSocket() const {
    return socket;
}

//void Subscriber::setSocket(const zmq::socket_t &socket) {
//    Subscriber::socket = socket;
//}

const stringstream &Subscriber::getStream() const {
    return stream;
}

//void Subscriber::setStream(const stringstream &stream) {
//    Subscriber::stream = stream;
//}

