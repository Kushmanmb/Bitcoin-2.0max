// tests/test_handshake.cpp
//
// Unit tests for the Bitcoin 2.0max P2P peer handshake protocol.

#include <catch2/catch_test_macros.hpp>

#include "bitcoin2max/params.h"
#include "net/message.h"
#include "net/peer.h"
#include "platform/socket_compat.h"

#include <algorithm>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

using namespace bitcoin2max;
using namespace bitcoin2max::net;

namespace {

struct SocketPair {
    SocketHandle first{INVALID_SOCKET_HANDLE};
    SocketHandle second{INVALID_SOCKET_HANDLE};
};

SocketPair createSocketPair() {
    SocketHandle listener =
        ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    if (!socketValid(listener)) {
        throw std::runtime_error(
            "Unable to create test listener"
        );
    }

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr =
        htonl(INADDR_LOOPBACK);
    address.sin_port = 0;

    if (
        ::bind(
            listener,
            reinterpret_cast<sockaddr*>(&address),
            sizeof(address)
        ) != 0
    ) {
        closeSocket(listener);

        throw std::runtime_error(
            "Unable to bind test listener"
        );
    }

    if (::listen(listener, 1) != 0) {
        closeSocket(listener);

        throw std::runtime_error(
            "Unable to listen on test socket"
        );
    }

#ifdef _WIN32
    int addressLength =
        sizeof(address);
#else
    socklen_t addressLength =
        sizeof(address);
#endif

    if (
        ::getsockname(
            listener,
            reinterpret_cast<sockaddr*>(&address),
            &addressLength
        ) != 0
    ) {
        closeSocket(listener);

        throw std::runtime_error(
            "Unable to determine test socket port"
        );
    }

    SocketHandle client =
        ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    if (!socketValid(client)) {
        closeSocket(listener);

        throw std::runtime_error(
            "Unable to create test client socket"
        );
    }

    if (
        ::connect(
            client,
            reinterpret_cast<sockaddr*>(&address),
            sizeof(address)
        ) != 0
    ) {
        closeSocket(client);
        closeSocket(listener);

        throw std::runtime_error(
            "Unable to connect test socket"
        );
    }

    SocketHandle server =
        ::accept(
            listener,
            nullptr,
            nullptr
        );

    closeSocket(listener);

    if (!socketValid(server)) {
        closeSocket(client);

        throw std::runtime_error(
            "Unable to accept test socket"
        );
    }

    return {server, client};
}

} // namespace

TEST_CASE(
    "Checksum: empty payload returns known SHA256d value",
    "[handshake]"
) {
    std::vector<uint8_t> empty;

    const auto checksum =
        computeChecksum(empty);

    REQUIRE(checksum[0] == 0x5D);
    REQUIRE(checksum[1] == 0xF6);
    REQUIRE(checksum[2] == 0xE0);
    REQUIRE(checksum[3] == 0xE2);
}

TEST_CASE(
    "Checksum: two calls with identical data produce identical result",
    "[handshake]"
) {
    std::vector<uint8_t> data{
        0x01,
        0x02,
        0x03
    };

    REQUIRE(
        computeChecksum(data) ==
        computeChecksum(data)
    );
}

TEST_CASE(
    "Checksum: different data produces different checksum",
    "[handshake]"
) {
    std::vector<uint8_t> a{0x00};
    std::vector<uint8_t> b{0x01};

    REQUIRE(
        computeChecksum(a) !=
        computeChecksum(b)
    );
}

TEST_CASE(
    "verack: command and payload are correct",
    "[handshake]"
) {
    const auto message =
        buildVerackMsg();

    REQUIRE(
        message.header.commandStr() ==
        "verack"
    );

    REQUIRE(
        message.header.magic ==
        network::MAGIC
    );

    REQUIRE(
        message.header.length == 0u
    );

    REQUIRE(
        message.payload.empty()
    );
}

TEST_CASE(
    "verack: serialise/deserialise round-trip",
    "[handshake]"
) {
    const auto original =
        buildVerackMsg();

    const auto wire =
        original.serialise();

    REQUIRE(
        wire.size() ==
        MessageHeader::WIRE_SIZE
    );

    const auto restored =
        NetMessage::deserialise(
            wire.data(),
            wire.size()
        );

    REQUIRE(
        restored.header.commandStr() ==
        "verack"
    );

    REQUIRE(
        restored.header.magic ==
        network::MAGIC
    );

    REQUIRE(
        restored.header.length == 0u
    );

    REQUIRE(
        restored.header.checksum ==
        original.header.checksum
    );

    REQUIRE(
        restored.payload.empty()
    );
}

TEST_CASE(
    "version: command, magic, and non-empty payload",
    "[handshake]"
) {
    const auto message =
        buildVersionMsg(
            100,
            8333
        );

    REQUIRE(
        message.header.commandStr() ==
        "version"
    );

    REQUIRE(
        message.header.magic ==
        network::MAGIC
    );

    REQUIRE(
        message.header.length > 0u
    );

    REQUIRE(
        message.payload.size() ==
        message.header.length
    );
}

TEST_CASE(
    "version: checksum matches payload",
    "[handshake]"
) {
    const auto message =
        buildVersionMsg(
            42,
            8333
        );

    REQUIRE(
        message.header.checksum ==
        computeChecksum(
            message.payload
        )
    );
}

TEST_CASE(
    "version: serialise/deserialise round-trip",
    "[handshake]"
) {
    const auto original =
        buildVersionMsg(
            777,
            8333
        );

    const auto wire =
        original.serialise();

    REQUIRE(
        wire.size() ==
        MessageHeader::WIRE_SIZE +
            original.payload.size()
    );

    const auto restored =
        NetMessage::deserialise(
            wire.data(),
            wire.size()
        );

    REQUIRE(
        restored.header.commandStr() ==
        "version"
    );

    REQUIRE(
        restored.header.magic ==
        network::MAGIC
    );

    REQUIRE(
        restored.header.length ==
        original.header.length
    );

    REQUIRE(
        restored.header.checksum ==
        original.header.checksum
    );

    REQUIRE(
        restored.payload ==
        original.payload
    );
}

TEST_CASE(
    "MessageHeader: serialise/deserialise preserves all fields",
    "[handshake]"
) {
    MessageHeader header;

    header.magic =
        network::MAGIC;

    header.command = {};

    std::copy(
        "version",
        "version" + 7,
        header.command.begin()
    );

    header.length =
        0x1234u;

    header.checksum = {
        0xAA,
        0xBB,
        0xCC,
        0xDD
    };

    const auto wire =
        header.serialise();

    REQUIRE(
        wire.size() ==
        MessageHeader::WIRE_SIZE
    );

    const auto restored =
        MessageHeader::deserialise(
            wire.data(),
            wire.size()
        );

    REQUIRE(
        restored.magic ==
        header.magic
    );

    REQUIRE(
        restored.commandStr() ==
        "version"
    );

    REQUIRE(
        restored.length ==
        0x1234u
    );

    REQUIRE(
        restored.checksum ==
        header.checksum
    );
}

TEST_CASE(
    "Peer: full handshake completes over loopback sockets",
    "[handshake]"
) {
    const SocketPair sockets =
        createSocketPair();

    bool serverOk = false;
    bool serverComplete = false;

    int32_t serverRemoteVersion = 0;
    int32_t serverRemoteHeight = 0;

    std::thread serverThread(
        [&]() {
            Peer inbound(
                sockets.first,
                false
            );

            serverOk =
                inbound.doHandshake(
                    100,
                    8333
                );

            serverComplete =
                inbound.isHandshakeComplete();

            serverRemoteVersion =
                inbound.remoteVersion();

            serverRemoteHeight =
                inbound.remoteHeight();
        }
    );

    Peer outbound(
        sockets.second,
        true
    );

    const bool clientOk =
        outbound.doHandshake(
            200,
            8333
        );

    serverThread.join();

    REQUIRE(clientOk);
    REQUIRE(serverOk);

    REQUIRE(
        outbound.isHandshakeComplete()
    );

    REQUIRE(serverComplete);

    REQUIRE(
        outbound.remoteVersion() ==
        network::PROTOCOL_VERSION
    );

    REQUIRE(
        outbound.remoteHeight() ==
        100
    );

    REQUIRE(
        serverRemoteVersion ==
        network::PROTOCOL_VERSION
    );

    REQUIRE(
        serverRemoteHeight ==
        200
    );
}

TEST_CASE(
    "Peer: user-agent is correctly exchanged",
    "[handshake]"
) {
    const SocketPair sockets =
        createSocketPair();

    std::string serverUserAgent;

    std::thread serverThread(
        [&]() {
            Peer inbound(
                sockets.first,
                false
            );

            inbound.doHandshake(
                0,
                8333
            );

            serverUserAgent =
                inbound.remoteUserAgent();
        }
    );

    Peer outbound(
        sockets.second,
        true
    );

    outbound.doHandshake(
        0,
        8333
    );

    serverThread.join();

    REQUIRE(
        outbound.remoteUserAgent() ==
        network::USER_AGENT
    );

    REQUIRE(
        serverUserAgent ==
        network::USER_AGENT
    );
}
