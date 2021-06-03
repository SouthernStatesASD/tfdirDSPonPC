//
// Created by Andre Henriques on 2019-07-30.
//

#ifndef ZEROMQ_PATHOLOGICAL_HPP
#define ZEROMQ_PATHOLOGICAL_HPP

#include <vector>
#include "Subscriber.h"

namespace pathological {
    Subscriber *simSubscriber;
    vector<Subscriber*> peersSubscribers;

    int pub(int argc, char *argv []);
    int pub_send(std::string value);
    int sub(int argc, char *argv []);
    int sub_get_sim();
    int sub_get_peers();
}

enum ExecMode {LIVE, SIMULATION};

#endif //ZEROMQ_PATHOLOGICAL_HPP
