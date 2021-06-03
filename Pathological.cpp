//
// Created by Steve Fan on 10/15/19.
//
#include <thread>
#include <chrono>
#include "zhelpers.hpp"
#include "Pathological.hpp"
#include "configuration/Utility/Utility.hpp"
#include "Subscriber.h"

using namespace std;

class sub_phasor_recv_thread_obj {
public:
    void operator()(string serverAddress, PHASORS_MSGS *phasorsMsgs) {
        zmq::context_t context(1);
        Subscriber *simSubscriber = new Subscriber(&context, serverAddress);
        zmq::socket_t &socket = (zmq::socket_t&) simSubscriber->getSocket();
        socket.setsockopt( ZMQ_SUBSCRIBE, "fromSim", 7);

        while (1) {
            try {
                string data = s_recv(socket);
                if (strcmp(data.c_str(), "fromSim") == 0) {
                    //topic is fromSim, so try to get the data again
                    data = s_recv(socket);

                    if (strcmp(data.c_str(), "fromSim")==0) {
                        //this is wrong as data can't be "fromSim"
                        printf("received fromSim topic twice\n");
                        continue;
                    }
                }

//                printf("received simulator data: %s\n", serverAddress.c_str());

//                if (strcmp(data.c_str(), "fromSim")==0) {
//                    //data is the topic, something weird happened
//                    printf("received weird simulator topic: %s\n", topic.c_str());
//                    printf("received weird simulator data: %s\n", data.c_str());
//                }

                if (strcmp(data.c_str(), "heartbeat") == 0)
                    continue;

                phasorsMsgs->msgs[phasorsMsgs->msgWriteIdx].data = data;
                phasorsMsgs->msgs[phasorsMsgs->msgWriteIdx].needToProcess = true;
                phasorsMsgs->msgWriteIdx++;
                if (phasorsMsgs->msgWriteIdx == MAX_PHASOR_MSGS)
                    phasorsMsgs->msgWriteIdx = 0;
            } catch (exception& e) {
                cout << e.what() << endl;
            }
        }
    }
};

class sub_peer_recv_thread_obj {
public:
    void operator()(string peerAddress, string topic, PEERS_MSGS *peersMsgs) {
        zmq::context_t context(1);
        Subscriber *peerSubscriber = new Subscriber(&context, peerAddress);
        zmq::socket_t &socket = (zmq::socket_t&) peerSubscriber->getSocket();
        socket.setsockopt( ZMQ_SUBSCRIBE, topic.c_str(), topic.length());

        while (1) {
            string topic = s_recv(socket);  //this is the topic, ignore since the socket does the filtering

            zmq::mutable_buffer mbuffer(peersMsgs->msgs[peersMsgs->msgWriteIdx].msgBuffer, MSG_SIZE);
            peersMsgs->msgs[peersMsgs->msgWriteIdx].size = s_recv_binary(socket, &mbuffer);  //get the actual binary data
//            printf("received peer data: %s, %d bytes\n", peerAddress.c_str(), peersMsgs->msgs[peersMsgs->msgWriteIdx].size);

            peersMsgs->msgWriteIdx++;
            peersMsgs->msgs[peersMsgs->msgWriteIdx].needToProcess = true;
            if (peersMsgs->msgWriteIdx == MAX_PEER_MSGS)
                peersMsgs->msgWriteIdx = 0;

//            printf("received peer msg: %d/%d\n", peersMsgs->msgWriteIdx, peersMsgs->msgProcessedIdx);
        }
    }
};

Pathological::Pathological(string configFile, OPERATINGMODE mode, string serverAddress) {
    memset(&phasorsMsgs, 0, sizeof(PHASORS_MSGS));
    memset(&peersMsgs, 0, sizeof(PEERS_MSGS));

    context = new zmq::context_t(1);
    publisher = new zmq::socket_t(*context, ZMQ_PUB);

    configuration = new Configuration(configFile, false);

    // prepare publisher
    vector<Communication> communications = configuration->getCommunications();
    for(auto &communication : communications) {
        if(communication.getType() == communication.types[communication.tcp]) {
            string port = communication.getPort();
            string address = "tcp://*:" + port;
            publisher->bind(address);
            printf("I'm %s: %s:%s\n", configuration->getSamId().c_str(), communication.getIpAddress().c_str(), communication.getPort().c_str());
        }
    }

    //setup the peers subscribers
    vector<ExternalConfiguration> neighbors;
    neighbors = configuration->getNeighbors(*configuration);
    PEERS_MSGS *peerMsg = &peersMsgs;
    for(auto &neighbor : neighbors) {
        vector<Communication> communications = neighbor.getCommunications();
        for(auto &communication : communications) {
            if(communication.getType() == communication.types[communication.tcp]) {
                string address = "tcp://" + communication.getIpAddress() + ":" + communication.getPort();
                printf("peer %s: %s\n", neighbor.getSamId().c_str(), address.c_str());

                thread peersThread(sub_peer_recv_thread_obj(), address, string("peer"), std::ref(peerMsg));
                peersThread.detach();

                this_thread::sleep_for(chrono::milliseconds(100));
            }
            if(communication.getType() == communication.types[communication.radio]) {
                // TODO: Handle subscribing to radio communication
            }
        }
    }

    //setup the n+1 peers subscribers
    vector<ExternalConfiguration> n1neighbors;
    n1neighbors = configuration->getN1Neighbors(*configuration);
    PEERS_MSGS *n1peerMsg = &peersMsgs;
    for(auto &n1neighbor : n1neighbors) {
        vector<Communication> communications = n1neighbor.getCommunications();
        for(auto &communication : communications) {
            if(communication.getType() == communication.types[communication.tcp]) {
                string address = "tcp://" + communication.getIpAddress() + ":" + communication.getPort();
                printf("n+1 Peer %s: %s\n", n1neighbor.getSamId().c_str(), address.c_str());

                thread peersThread(sub_peer_recv_thread_obj(), address, string("n1peer"), std::ref(n1peerMsg));
                peersThread.detach();

                this_thread::sleep_for(chrono::milliseconds(100));
            }
            if(communication.getType() == communication.types[communication.radio]) {
                // TODO: Handle subscribing to radio communication
            }
        }
    }

    if (mode == SIMULATION) {
        //setup the simulation phasor subscriber
        PHASORS_MSGS *phasorMsg = &phasorsMsgs;
        thread phasorsThread(sub_phasor_recv_thread_obj(), serverAddress, std::ref(phasorMsg));
        phasorsThread.detach();

        this_thread::sleep_for(chrono::milliseconds(100));
    }
}

int Pathological::pub_send_to_sim (string value) {
    stringstream ss;
    ss << "{\"samId\":\"" <<configuration->getSamId()<<"\",\"command\":\""<<value<<"\"}";
    s_sendmore(*publisher, "toSim");
    s_send (*publisher, ss.str());
    return 0;
}

int Pathological::pub_send_to_peers (string topic, char *buffer, int size) {
    int retries = 0;
    bool successful = false;
//    printf("send peer msg\n");

    while (!successful && retries < 3) {
        try {
            s_sendmore(*publisher, topic);
            s_send_binary(*publisher, buffer, size);
            successful = true;
            peerMsgSent++;
        } catch (...) {
            printf("ZMQ Failed to Send %s Msg: %d\n", topic.c_str(), retries);
            retries++;
            this_thread::sleep_for(chrono::milliseconds(2));
        }
    }

    return 0;
}

void Pathological::checkSendReceives() {
    numChecked++;
    if (numChecked >= 5000) {
        //it's been around 10sec print out the sends
        printf("Peer Msg Sent: %d\n", peerMsgSent);
        peerMsgSent = 0;
        numChecked = 0;
    }
}