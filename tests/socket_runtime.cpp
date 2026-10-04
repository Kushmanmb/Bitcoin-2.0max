#include "platform/socket_compat.h"

#include <cstdlib>
#include <iostream>

namespace {

struct SocketTestRuntime {
    SocketTestRuntime() {
        if (!bitcoin2max::initializeSockets()) {
            std::cerr << "Failed to initialize test networking." << std::endl;
            std::abort();
        }
    }

    ~SocketTestRuntime() {
        bitcoin2max::cleanupSockets();
    }
};

SocketTestRuntime socketTestRuntime;

}
