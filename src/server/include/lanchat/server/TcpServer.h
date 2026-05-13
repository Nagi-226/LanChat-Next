#pragma once

#include "AsyncSession.h"
#include "MessageRouter.h"
#include "PresenceManager.h"
#include "SessionPool.h"
#include "mini_asio.hpp"

#include <cstdint>
#include <memory>
#include <string>

namespace lanchat::server {

namespace db {
class ChannelRepository;
class MessageRepository;
class UserRepository;
}

class TcpServer {
public:
    TcpServer(std::uint16_t port,
              db::UserRepository& users,
              db::MessageRepository& messages,
              db::ChannelRepository& channels,
              std::size_t maxConnections = 500);
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
    void startHeartbeatSweep();
    void sweepHeartbeatTimeouts();
    void markOfflineAndBroadcast(int userId);

    std::uint16_t port_;
    vendor::asio::io_context ctx_;
    std::unique_ptr<vendor::asio::ip::tcp::acceptor> acceptor_;
    std::unique_ptr<vendor::asio::steady_timer> heartbeat_timer_;
    SessionPool sessions_;
    PresenceManager presence_;
    MessageRouter router_;
    db::UserRepository& users_;
};

} // namespace lanchat::server
