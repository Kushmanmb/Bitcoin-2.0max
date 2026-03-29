// tests/test_handshake.cpp
//
// Unit tests for the Bitcoin 2.0max P2P peer handshake protocol.
//
// Tests cover:
//   • Message checksum computation
//   • `verack` message construction and round-trip serialisation
//   • `version` message construction and round-trip serialisation
//   • Full opening handshake between two in-process Peer instances
//     connected via socketpair(2)

#include <catch2/catch_test_macros.hpp>
#include "net/message.h"
#include "net/peer.h"
#include "bitcoin2max/params.h"

#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <cstring>
#include <thread>

using namespace bitcoin2max;
using namespace bitcoin2max::net;

// ── computeChecksum ───────────────────────────────────────────────────────────

TEST_CASE("Checksum: empty payload returns known SHA256d value", "[handshake]") {
    // SHA256(SHA256("")) is the well-known double-hash of an empty string.
    // First four bytes = 5D F6 E0 E2
    std::vector<uint8_t> empty;
    auto cs = computeChecksum(empty);
    REQUIRE(cs[0] == 0x5D);
    REQUIRE(cs[1] == 0xF6);
    REQUIRE(cs[2] == 0xE0);
    REQUIRE(cs[3] == 0xE2);
}

TEST_CASE("Checksum: two calls with identical data produce identical result", "[handshake]") {
    std::vector<uint8_t> data = {0x01, 0x02, 0x03};
    REQUIRE(computeChecksum(data) == computeChecksum(data));
}

TEST_CASE("Checksum: different data produces different checksum", "[handshake]") {
    std::vector<uint8_t> a = {0x00};
    std::vector<uint8_t> b = {0x01};
    REQUIRE(computeChecksum(a) != computeChecksum(b));
}

// ── verack message ────────────────────────────────────────────────────────────

TEST_CASE("verack: command and payload are correct", "[handshake]") {
    auto msg = buildVerackMsg();
    REQUIRE(msg.header.commandStr() == "verack");
    REQUIRE(msg.header.magic        == network::MAGIC);
    REQUIRE(msg.header.length       == 0u);
    REQUIRE(msg.payload.empty());
}

TEST_CASE("verack: serialise/deserialise round-trip", "[handshake]") {
    auto original = buildVerackMsg();
    auto wire     = original.serialise();

    REQUIRE(wire.size() == MessageHeader::WIRE_SIZE); // header only, no payload

    auto restored = NetMessage::deserialise(wire.data(), wire.size());
    REQUIRE(restored.header.commandStr() == "verack");
    REQUIRE(restored.header.magic        == network::MAGIC);
    REQUIRE(restored.header.length       == 0u);
    REQUIRE(restored.header.checksum     == original.header.checksum);
    REQUIRE(restored.payload.empty());
}

// ── version message ───────────────────────────────────────────────────────────

TEST_CASE("version: command, magic, and non-empty payload", "[handshake]") {
    auto msg = buildVersionMsg(/*bestHeight=*/100, /*ourPort=*/8333);
    REQUIRE(msg.header.commandStr() == "version");
    REQUIRE(msg.header.magic        == network::MAGIC);
    REQUIRE(msg.header.length       >  0u);
    REQUIRE(msg.payload.size()      == msg.header.length);
}

TEST_CASE("version: checksum matches payload", "[handshake]") {
    auto msg = buildVersionMsg(42, 8333);
    REQUIRE(msg.header.checksum == computeChecksum(msg.payload));
}

TEST_CASE("version: serialise/deserialise round-trip", "[handshake]") {
    auto original = buildVersionMsg(777, 8333);
    auto wire     = original.serialise();

    REQUIRE(wire.size() == MessageHeader::WIRE_SIZE + original.payload.size());

    auto restored = NetMessage::deserialise(wire.data(), wire.size());
    REQUIRE(restored.header.commandStr() == "version");
    REQUIRE(restored.header.magic        == network::MAGIC);
    REQUIRE(restored.header.length       == original.header.length);
    REQUIRE(restored.header.checksum     == original.header.checksum);
    REQUIRE(restored.payload             == original.payload);
}

// ── MessageHeader round-trip ──────────────────────────────────────────────────

TEST_CASE("MessageHeader: serialise/deserialise preserves all fields", "[handshake]") {
    MessageHeader hdr;
    hdr.magic    = network::MAGIC;
    hdr.command  = {};
    std::copy("version", "version" + 7, hdr.command.begin());
    hdr.length   = 0x1234u;
    hdr.checksum = {0xAA, 0xBB, 0xCC, 0xDD};

    auto wire     = hdr.serialise();
    REQUIRE(wire.size() == MessageHeader::WIRE_SIZE);

    auto restored = MessageHeader::deserialise(wire.data(), wire.size());
    REQUIRE(restored.magic       == hdr.magic);
    REQUIRE(restored.commandStr() == "version");
    REQUIRE(restored.length      == 0x1234u);
    REQUIRE(restored.checksum    == hdr.checksum);
}

// ── Full handshake over socketpair ────────────────────────────────────────────

TEST_CASE("Peer: full handshake completes over socketpair", "[handshake]") {
    int sv[2] = {-1, -1};
    REQUIRE(::socketpair(AF_UNIX, SOCK_STREAM, 0, sv) == 0);

    bool serverOk              = false;
    bool serverComplete        = false;
    int32_t serverRemoteVersion = 0;
    int32_t serverRemoteHeight  = 0;

    // Run the inbound (server) side in a background thread.
    std::thread serverThread([&]() {
        Peer inbound(sv[0], /*outbound=*/false);
        serverOk       = inbound.doHandshake(/*bestHeight=*/100, /*ourPort=*/8333);
        serverComplete = inbound.isHandshakeComplete();
        serverRemoteVersion = inbound.remoteVersion();
        serverRemoteHeight  = inbound.remoteHeight();
    });

    // Run the outbound (client) side on the main test thread.
    Peer outbound(sv[1], /*outbound=*/true);
    bool clientOk = outbound.doHandshake(/*bestHeight=*/200, /*ourPort=*/8333);

    serverThread.join();

    // Both sides must report success.
    REQUIRE(clientOk);
    REQUIRE(serverOk);

    // Both sides must be in COMPLETE state.
    REQUIRE(outbound.isHandshakeComplete());
    REQUIRE(serverComplete);

    // Each side must have parsed the other's protocol version and height.
    REQUIRE(outbound.remoteVersion() == network::PROTOCOL_VERSION);
    REQUIRE(outbound.remoteHeight()  == 100); // server sent bestHeight=100

    REQUIRE(serverRemoteVersion == network::PROTOCOL_VERSION);
    REQUIRE(serverRemoteHeight  == 200); // client sent bestHeight=200
}

TEST_CASE("Peer: user-agent is correctly exchanged", "[handshake]") {
    int sv[2] = {-1, -1};
    REQUIRE(::socketpair(AF_UNIX, SOCK_STREAM, 0, sv) == 0);

    std::string serverUA;

    std::thread serverThread([&]() {
        Peer inbound(sv[0], false);
        inbound.doHandshake(0, 8333);
        serverUA = inbound.remoteUserAgent();
    });

    Peer outbound(sv[1], true);
    outbound.doHandshake(0, 8333);

    serverThread.join();

    REQUIRE(outbound.remoteUserAgent() == network::USER_AGENT);
    REQUIRE(serverUA                   == network::USER_AGENT);
}
