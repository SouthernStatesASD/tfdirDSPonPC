//
// Created by Andre Henriques on 2019-07-30.
//

#ifndef ZEROMQ_PATHOLOGICAL_HPP
#define ZEROMQ_PATHOLOGICAL_HPP

#include <vector>
#include <configuration/configuration.hpp>
#include "Subscriber.h"
#include "rapidjson/document.h"
#include "rapidjson/stringbuffer.h"

#define MAX_PEER_MSGS 100
#define MSG_SIZE 518
#define MAX_PHASOR_MSGS 10

typedef struct {
    bool needToProcess;
    int cyclesWaited;  //this is how many execution cycles to wait before processing the msg
    int size;
    char msgBuffer[MSG_SIZE];
} PEER_MSG;

typedef struct {
    int msgWriteIdx;
    int msgProcessedIdx;
    PEER_MSG msgs[MAX_PEER_MSGS];
} PEERS_MSGS;

typedef struct {
    bool needToProcess;
    string data;
} PHASOR_MSG;

typedef struct {
    int msgWriteIdx;
    int msgProcessedIdx;
    PHASOR_MSG msgs[MAX_PHASOR_MSGS];
} PHASORS_MSGS;

enum OPERATINGMODE {LIVE, SIMULATION};

class Pathological {
public:
    zmq::context_t *context;
    zmq::socket_t *publisher;

    Configuration *configuration;

    PEERS_MSGS peersMsgs;
    PHASORS_MSGS phasorsMsgs;

    int peerMsgSent;
    int numChecked;

    Pathological(string configFile, OPERATINGMODE mode, string serverAddress);

    int pub_send_to_sim(string value);
    int pub_send_to_peers (string topic, char *buffer, int size);
    int sub_get_sim();
    int sub_get_peers();
    void sub_recv_peers(Subscriber &subscriber);
    void test_thread(string serverAddress, PHASORS_MSGS *phasorsMsgs);
    void checkSendReceives();

private:

};

#endif //ZEROMQ_PATHOLOGICAL_HPP
