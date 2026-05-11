#ifndef LANCHAT_SERVER_TCP_SERVER_H
#define LANCHAT_SERVER_TCP_SERVER_H

#include <atomic>
#include <cstdint>
#include <string>

namespace lanchat::server {

class TcpServer {
public:
    explicit TcpServer(std::uint16_t port);
    ~TcpServer();

    TcpServer(const TcpServer&) = delete;
    TcpServer& operator=(const TcpServer&) = delete;

    int run();
    void stop();

private:
    void handleClient(std::uintptr_t socketHandle);
    std::string responseFor(const std::string& request) const;

    std::uint16_t port_;
    std::atomic<bool> running_{false};
};

} // namespace lanchat::server

#endif // LANCHAT_SERVER_TCP_SERVER_H
