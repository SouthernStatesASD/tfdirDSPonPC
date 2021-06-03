//
// Created by Andre Henriques on 2019-07-30.
//

//  Pathological subscriber
//  Subscribes to one random topic and prints received messages

#include <thread>
#include <chrono>
#include "zhelpers.hpp"
#include "pathological.hpp"
#include <configuration/configuration.hpp>
#include "configuration/Utility/Utility.hpp"
#include "Subscriber.h"

using namespace std;

int pathological::sub (int argc, char *argv [])
{
    zmq::context_t context(1);
    ExecMode mode = LIVE;
    string simulationServerAddress;
    Configuration *configuration;
    if (argc == 4) {
        configuration = new Configuration(argv[1], false);
    } else if (argc > 4) {
        if (strcmp(argv[4], "simulation") == 0 && argc == 6) {
            mode = SIMULATION;
            configuration = new Configuration(argv[1], false);
            simulationServerAddress = argv[4];
            simulationServerAddress = "tcp://" + simulationServerAddress;

            simSubscriber = new Subscriber(&context, simulationServerAddress);
            zmq::socket_t &socket = (zmq::socket_t&) simSubscriber->getSocket();
            socket.setsockopt( ZMQ_SUBSCRIBE, "sim", 24);
        }
    } else {
        configuration = new Configuration();
    }

    // Get neighbor addresses from config file.
    vector<ExternalConfiguration> neighbors;
    neighbors = configuration->getNeighbors(*configuration);

//    vector<Subscriber*> subscribers;
    for(auto &neighbor : neighbors) {
        vector<Communication> communications = neighbor.getCommunications();
        for(auto &communication : communications) {
            if(communication.getType() == communication.types[communication.tcp]) {
                string address = "tcp://" + communication.getIpAddress() + ":" + communication.getPort();
                cout << address << endl;
                Subscriber *thisSubscriber = new Subscriber(&context, address);
                peersSubscribers.push_back(thisSubscriber);
            }
            if(communication.getType() == communication.types[communication.radio]) {
                // TODO: Handle subscribing to radio communication
            }
        }
    }

    // instantiate peers sockets
    for (auto& subscriber : peersSubscribers) {
        stringstream &ss = (stringstream&) subscriber->getStream();
        ss << dec << setw(3) << setfill('0') << 1;
        cout << "sam:" << ss.str() << endl;

        zmq::socket_t &socket = (zmq::socket_t&) subscriber->getSocket();
        socket.setsockopt( ZMQ_SUBSCRIBE, ss.str().c_str(), ss.str().size());
    }

    // prepare publisher
    // TODO: Get port from config file.
    vector<Communication> communications = configuration->getCommunications();
    char *port_pub[] = {
            NULL,
            NULL
    };
    for(auto &communication : communications) {
        if(communication.getType() == communication.types[communication.tcp]) {
            string port = communication.getPort();
            string address = "tcp://*:" + port;
            char *addressChar = const_cast<char *>(address.c_str());
            port_pub[1] = addressChar;
            pathological::pub(argc, port_pub);
        }
    }

    // receive and send
//    bool loopForever = true;
//    while (loopForever) {
//        for (auto &subscriber : subscribers) {
//            auto &ss = (stringstream&) subscriber->getStream();
//            auto &socket = (zmq::socket_t&) subscriber->getSocket();
////            socket.setsockopt( ZMQ_SUBSCRIBE, ss.str().c_str(), ss.str().size());
//
//            string topic = s_recv (socket);
//            string data = s_recv (socket);
//
//            if (topic != ss.str()) break;
//
//            cout << data << endl;
//        }
//
//        pathological::pub_send("test");
//    }

//    subscriber.connect("tcp://localhost:5556");
//    subscriber2.connect("tcp://localhost:5556");
//
//    stringstream ss_all;
//    stringstream ss;
//    ss << dec << setw(3) << setfill('0') << within(5);
//    ss_all << dec << setw(3) << setfill('0') << 0;
//    ss << dec << setw(3) << setfill('0') << 1;
//    cout << "public topic:" << ss_all.str() << endl;
//    cout << "sam:" << ss.str() << endl;
//
//    subscriber.setsockopt( ZMQ_SUBSCRIBE, ss_all.str().c_str(), ss_all.str().size());
//    subscriber.setsockopt( ZMQ_SUBSCRIBE, ss.str().c_str(), ss.str().size());
//
//    subscriber2.setsockopt( ZMQ_SUBSCRIBE, ss_all.str().c_str(), ss_all.str().size());
//    subscriber2.setsockopt( ZMQ_SUBSCRIBE, ss.str().c_str(), ss.str().size());
//
//    subscriber.setsockopt( ZMQ_SUBSCRIBE, {"002"}, ss.str().size());
//
//
//    // TODO: Get port from config file.
//    char *port_pub[] = {
//            NULL,
//            (char*) "tcp://*:5555"
//    };
//
//    pathological::pub(argc, port_pub);
//
//
//    int counter = 0;
//
//    while (1) {
//        string topic = s_recv (subscriber);
//        string data = s_recv (subscriber);
//
//        string topic2 = s_recv (subscriber2);
//        string data2 = s_recv (subscriber2);
//
//        if (
//                (topic != ss_all.str() && topic != ss.str()) || (topic2 != ss_all.str() && topic2 != ss.str())
//                )
//            break;
//
//        if(topic == ss.str()) {
//            cout << data << endl;
//        }
//
//        if(counter > 0) {
//            counter = 0;
//            pathological::pub_send("test");
//        }
//
//        counter+=1;
//    }
    return 0;
}

int pathological::sub_get_sim () {
    auto &ss = (stringstream&) simSubscriber->getStream();
    auto &socket = (zmq::socket_t&) simSubscriber->getSocket();

    string topic = s_recv (socket);
    string data = s_recv (socket);

    cout << data << endl;

    return 0;
}

int pathological::sub_get_peers () {
    for (auto &subscriber : peersSubscribers) {
        auto &ss = (stringstream&) subscriber->getStream();
        auto &socket = (zmq::socket_t&) subscriber->getSocket();

        string topic = s_recv (socket);
        string data = s_recv (socket);

        if (topic != ss.str()) break;

        cout << data << endl;
    }
    return 0;
}