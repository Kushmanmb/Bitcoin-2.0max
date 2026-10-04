#pragma once

#include <cstddef>

#ifdef _WIN32

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <winsock2.h>
#include <ws2tcpip.h>

namespace bitcoin2max {

using SocketHandle = SOCKET;

constexpr SocketHandle INVALID_SOCKET_HANDLE = INVALID_SOCKET;
constexpr int SOCKET_SEND_FLAGS = 0;

inline bool socketValid(SocketHandle socket) {
    return socket != INVALID_SOCKET;
}

inline void closeSocket(SocketHandle socket) {
    if (socketValid(socket)) {
        ::closesocket(socket);
    }
}

inline void shutdownSocket(SocketHandle socket) {
    if (socketValid(socket)) {
        ::shutdown(socket, SD_BOTH);
    }
}

inline bool initializeSockets() {
    WSADATA data{};
    return ::WSAStartup(MAKEWORD(2, 2), &data) == 0;
}

inline void cleanupSockets() {
    ::WSACleanup();
}

inline int socketLastError() {
    return ::WSAGetLastError();
}

} // namespace bitcoin2max

#else

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cerrno>

namespace bitcoin2max {

using SocketHandle = int;

constexpr SocketHandle INVALID_SOCKET_HANDLE = -1;

#ifdef MSG_NOSIGNAL
constexpr int SOCKET_SEND_FLAGS = MSG_NOSIGNAL;
#else
constexpr int SOCKET_SEND_FLAGS = 0;
#endif

inline bool socketValid(SocketHandle socket) {
    return socket >= 0;
}

inline void closeSocket(SocketHandle socket) {
    if (socketValid(socket)) {
        ::close(socket);
    }
}

inline void shutdownSocket(SocketHandle socket) {
    if (socketValid(socket)) {
        ::shutdown(socket, SHUT_RDWR);
    }
}

inline bool initializeSockets() {
    return true;
}

inline void cleanupSockets() {}

inline int socketLastError() {
    return errno;
}

} // namespace bitcoin2max

#endif
