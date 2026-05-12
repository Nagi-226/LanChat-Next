#pragma once

#include "AsyncSession.h"
#include "mini_asio.hpp"

#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>

namespace lanchat::server {

namespace db { class UserRepository; }

class TcpServer {
public:
    explicit TcpServer(std::uint16_t port, db::UserRepository* users = nullptr);
    ~TcpServer();

    TcpServer(const TcpServer&) = delete;
    TcpServer& operator=(const TcpServer&) = delete;

    int run();
    void stop();

    void onMessage(std::shared_ptr<AsyncSession> session, const std::string& json);
    void removeSession(uint64_t sessionId);
    void broadcast(const std::string& json, uint64_t excludeSessionId = 0);
    size_t connectionCount() const;

private:
    void startAccept();
    std::string dispatchResponse(const std::string& request) const;

    std::uint16_t port_;
    vendor::asio::io_context ctx_;
    std::unique_ptr<vendor::asio::ip::tcp::acceptor> acceptor_;
    std::unordered_map<uint64_t, std::shared_ptr<AsyncSession>> sessions_;
    db::UserRepository* users_ = nullptr;
};

} // namespace lanchat::server
