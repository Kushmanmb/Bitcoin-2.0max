#pragma once
// src/net/peer.h
//
// Bitcoin 2.0max — single P2P peer connection and opening handshake.
//
// The handshake follows the standard Bitcoin protocol:
//   Outbound: send version → recv version → send verack → recv verack
//   Inbound:  recv version → send version → send verack → recv verack

#include "message.h"

#include <cstdint>
#include <string>

namespace bitcoin2max {
namespace net {

/// Owns a connected socket and drives the P2P opening handshake.
class Peer {
public:
    /// Handshake state machine states.
    enum class HandshakeState {
        IDLE,              ///< Not started yet
        VERSION_SENT,      ///< We sent our version; awaiting theirs
        VERSION_RECEIVED,  ///< Their version received; awaiting their verack
        COMPLETE,          ///< Both sides exchanged version + verack
        FAILED             ///< Irrecoverable error; connection should be closed
    };

    /// @param fd        Connected socket fd — ownership is transferred to Peer.
    /// @param outbound  true if we initiated the connection; false if inbound.
    Peer(int fd, bool outbound);
    ~Peer();

    Peer(const Peer&)            = delete;
    Peer& operator=(const Peer&) = delete;

    /// Perform the full opening handshake (blocking).
    /// @param bestHeight  Our current best chain height.
    /// @param ourPort     Our P2P listening port.
    /// @returns true when the handshake completes successfully.
    bool doHandshake(int32_t bestHeight, uint16_t ourPort);

    // ── Accessors ─────────────────────────────────────────────────────────────

    bool           isHandshakeComplete() const { return state_ == HandshakeState::COMPLETE; }
    HandshakeState handshakeState()      const { return state_; }
    int            fd()                  const { return fd_; }
    bool           isOutbound()          const { return outbound_; }

    /// Remote peer's advertised protocol version (valid after handshake).
    int32_t            remoteVersion()   const { return remoteVersion_; }
    /// Remote peer's advertised best height (valid after handshake).
    int32_t            remoteHeight()    const { return remoteHeight_; }
    /// Remote peer's user-agent string (valid after handshake).
    const std::string& remoteUserAgent() const { return remoteUserAgent_; }

private:
    int            fd_;
    bool           outbound_;
    HandshakeState state_{HandshakeState::IDLE};

    int32_t     remoteVersion_{0};
    int32_t     remoteHeight_{0};
    std::string remoteUserAgent_;

    bool sendMsg(const NetMessage& msg);
    bool recvMsg(NetMessage& msg);
    bool parseVersionPayload(const std::vector<uint8_t>& payload);
};

} // namespace net
} // namespace bitcoin2max
