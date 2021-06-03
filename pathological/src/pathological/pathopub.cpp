//
// Created by Andre Henriques on 2019-07-30.
//

//  Pathological publisher
//  Sends out 1,000 topics and then one random update per second

#include <thread>
#include <chrono>
#include "zhelpers.hpp"
#include "pathological/pathological.hpp"

using namespace std;

zmq::context_t context(1);
zmq::socket_t publisher(context, ZMQ_PUB);

int pathological::pub (int argc, char *argv [])
{
    //  Initialize random number generator
    srandom ((unsigned) time (NULL));

    cout << "about to connect" << endl;
    if (argc == 2) {
        publisher.bind(argv[1]);
        cout << "connecting to " << argv [1] << endl;
    } else {
        cout << "connecting to tcp://*:5556" << endl;
        publisher.bind("tcp://*:5556");
    }

    //  Ensure subscriber connection has time to complete
    this_thread::sleep_for(chrono::seconds(1));

    //  Send out all 1,000 topic messages
    int topic_nbr;
    for (topic_nbr = 0; topic_nbr < 5; topic_nbr++) {
        stringstream ss;
        ss << dec << setw(3) << setfill('0') << topic_nbr;

        s_sendmore (publisher, ss.str());
        s_send (publisher, "Starting SAM");
    }

    //  Send one random update per second


//    this_thread::sleep_for(chrono::seconds(5));
//    stringstream ss;
//    stringstream ss_all;
////    ss << dec << setw(3) << setfill('0') << within(5);
//    ss << dec << setw(3) << setfill('0') << 1;
//    ss_all << dec << setw(3) << setfill('0') << 0;
//
//    s_sendmore (publisher, ss.str());
//    s_send (publisher, "SAM");
    return 0;
}

int pathological::pub_send (string value) {
    stringstream ss;
    ss << dec << setw(3) << setfill('0') << 1;
    s_sendmore (publisher, ss.str());
    s_send (publisher, value);
    return 0;
}