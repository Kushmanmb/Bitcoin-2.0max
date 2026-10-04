#pragma once

// Bitcoin 2.0max — single P2P peer connection and opening handshake.

#include "message.h"
#include "../platform/socket_compat.h"

#include <cstdint>
#include <string>

namespace bitcoin2max {
namespace net {

class Peer {
public:
    enum class HandshakeState {
        IDLE,
        VERSION_SENT,
        VERSION_RECEIVED,
        COMPLETE,
        FAILED
    };

    Peer(SocketHandle fd, bool outbound);
    ~Peer();

    Peer(const Peer&) = delete;
    Peer& operator=(const Peer&) = delete;

    bool doHandshake(int32_t bestHeight, uint16_t ourPort);

    bool isHandshakeComplete() const {
        return state_ == HandshakeState::COMPLETE;
    }

    HandshakeState handshakeState() const {
        return state_;
    }

    SocketHandle fd() const {
        return fd_;
    }

    bool isOutbound() const {
        return outbound_;
    }

    int32_t remoteVersion() const {
        return remoteVersion_;
    }

    int32_t remoteHeight() const {
        return remoteHeight_;
    }

    const std::string& remoteUserAgent() const {
        return remoteUserAgent_;
    }

private:
    SocketHandle fd_{INVALID_SOCKET_HANDLE};
    bool outbound_;
    HandshakeState state_{HandshakeState::IDLE};

    int32_t remoteVersion_{0};
    int32_t remoteHeight_{0};
    std::string remoteUserAgent_;

    bool sendMsg(const NetMessage& msg);
    bool recvMsg(NetMessage& msg);
    bool parseVersionPayload(const std::vector<uint8_t>& payload);
};

} // namespace net
} // namespace bitcoin2max
