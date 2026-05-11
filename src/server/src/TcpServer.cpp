#include "lanchat/server/TcpServer.h"

#include "lanchat/server/FrameCodec.h"

#include <array>
#include <iostream>
#include <thread>
#include <vector>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

namespace lanchat::server {
namespace {
#ifdef _WIN32
using Socket = SOCKET;
constexpr Socket kInvalidSocket = INVALID_SOCKET;
void closeSocket(Socket socket) { closesocket(socket); }
#else
using Socket = int;
constexpr Socket kInvalidSocket = -1;
void closeSocket(Socket socket) { close(socket); }
#endif
}

TcpServer::TcpServer(std::uint16_t port) : port_(port) {}
TcpServer::~TcpServer() { stop(); }

void TcpServer::stop()
{
    running_ = false;
}

int TcpServer::run()
{
#ifdef _WIN32
    WSADATA data{};
    if (WSAStartup(MAKEWORD(2, 2), &data) != 0) {
        std::cerr << "WSAStartup failed\n";
        return 1;
    }
#endif

    Socket listener = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listener == kInvalidSocket) {
        std::cerr << "socket() failed\n";
        return 1;
    }

    int reuse = 1;
    setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&reuse), sizeof(reuse));

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_ANY);
    address.sin_port = htons(port_);

    if (::bind(listener, reinterpret_cast<sockaddr*>(&address), sizeof(address)) != 0) {
        std::cerr << "bind() failed on port " << port_ << "\n";
        closeSocket(listener);
        return 1;
    }

    if (::listen(listener, SOMAXCONN) != 0) {
        std::cerr << "listen() failed\n";
        closeSocket(listener);
        return 1;
    }

    running_ = true;
    std::cout << "LanChat server-next listening on 0.0.0.0:" << port_ << "\n";
#ifdef LANCHAT_HAS_STANDALONE_ASIO
    std::cout << "standalone asio detected; full async implementation is scheduled for v1.1.3\n";
#else
    std::cout << "using WinSock/POSIX listener fallback for the v1.1.0 skeleton\n";
#endif

    while (running_) {
        Socket client = ::accept(listener, nullptr, nullptr);
        if (client == kInvalidSocket) {
            continue;
        }
        std::thread(&TcpServer::handleClient, this, static_cast<std::uintptr_t>(client)).detach();
    }

    closeSocket(listener);
#ifdef _WIN32
    WSACleanup();
#endif
    return 0;
}

void TcpServer::handleClient(std::uintptr_t socketHandle)
{
    auto client = static_cast<Socket>(socketHandle);
    std::vector<std::uint8_t> buffer;
    std::array<char, 4096> chunk{};

    while (running_) {
#ifdef _WIN32
        const int received = ::recv(client, chunk.data(), static_cast<int>(chunk.size()), 0);
#else
        const int received = static_cast<int>(::recv(client, chunk.data(), chunk.size(), 0));
#endif
        if (received <= 0) {
            break;
        }

        buffer.insert(buffer.end(), chunk.begin(), chunk.begin() + received);
        std::string request;
        while (FrameCodec::tryDecode(buffer, request)) {
            const auto response = FrameCodec::encode(responseFor(request));
#ifdef _WIN32
            ::send(client, reinterpret_cast<const char*>(response.data()), static_cast<int>(response.size()), 0);
#else
            ::send(client, response.data(), response.size(), 0);
#endif
        }
    }

    closeSocket(client);
}

std::string TcpServer::responseFor(const std::string& request) const
{
    if (request.find("\"type\":20") != std::string::npos || request.find("\"type\": 20") != std::string::npos) {
        return R"({"type":21,"status":"ok"})";
    }
    return R"({"type":33,"status":"ok","msg":"server-next skeleton received frame"})";
}

} // namespace lanchat::server
