#include "lanchat/server/TcpServer.h"

#include "lanchat/server/ServerLogger.h"
#include "lanchat/server/db/ChannelRepository.h"
#include "lanchat/server/db/FriendRepository.h"
#include "lanchat/server/db/MessageRepository.h"
#include "lanchat/server/db/UserRepository.h"

#include <chrono>
#include <stdexcept>

namespace lanchat::server {

TcpServer::TcpServer(std::uint16_t port,
                     db::UserRepository& users,
                     db::MessageRepository& messages,
                     db::ChannelRepository& channels,
                     db::FriendRepository& friends,
                     std::size_t maxConnections)
    : port_(port),
      sessions_(maxConnections),
      router_(sessions_, presence_, users, messages, channels, friends),
      users_(users) {}

TcpServer::~TcpServer() {
    stop();
}

int TcpServer::run() {
    try {
        ServerLogger::instance().init("logs");
        ServerLogger::instance().info("LanChat server-next v1.1.9-prep starting");

        acceptor_ = std::make_unique<net::ip::tcp::acceptor>(
            ctx_, net::ip::tcp::endpoint(net::ip::tcp::v4(), port_));
        ServerLogger::instance().info(
            "listening on 0.0.0.0:" + std::to_string(port_));

        startAccept();
        startHeartbeatSweep();
        ctx_.run();

        ServerLogger::instance().info("server stopped");
        return 0;
    } catch (const std::exception& e) {
        ServerLogger::instance().error(
            std::string("server failed to start: ") + e.what());
        return 1;
    }
}

void TcpServer::stop() {
    ctx_.stop();
}

void TcpServer::startAccept() {
    auto session = std::make_shared<AsyncSession>(ctx_, *this);
    acceptor_->async_accept(session->socket(),
        [this, session](const net::error_code& ec) {
            if (!ec) {
                if (!sessions_.add(session)) {
                    ServerLogger::instance().error("max connections reached; rejecting session");
                    session->close();
                    startAccept();
                    return;
                }
                session->start();
                ServerLogger::instance().debug(
                    "accepted session " + std::to_string(session->id())
                    + ", total: " + std::to_string(sessions_.size()));
            }
            startAccept(); // chain next accept
        });
}

void TcpServer::onMessage(std::shared_ptr<AsyncSession> session,
                          const std::string& json) {
    router_.route(session, json);
}

void TcpServer::removeSession(uint64_t sessionId) {
    auto session = sessions_.findSession(sessionId);
    const int userId = session ? session->userId() : 0;
    sessions_.remove(sessionId);
    if (userId > 0) {
        markOfflineAndBroadcast(userId);
    }
    ServerLogger::instance().debug(
        "session " + std::to_string(sessionId)
        + " removed, remaining: " + std::to_string(sessions_.size()));
}

void TcpServer::broadcast(const std::string& json, uint64_t excludeSessionId) {
    for (const auto& session : sessions_.all()) {
        if (session && session->id() != excludeSessionId) {
            session->deliver(json);
        }
    }
}

size_t TcpServer::connectionCount() const {
    return sessions_.size();
}

void TcpServer::startHeartbeatSweep() {
    heartbeat_timer_ = std::make_unique<net::steady_timer>(ctx_);
    heartbeat_timer_->expires_after(std::chrono::seconds(30));
    heartbeat_timer_->async_wait([this](const net::error_code& ec) {
        if (!ec) {
            sweepHeartbeatTimeouts();
            startHeartbeatSweep();
        }
    });
}

void TcpServer::sweepHeartbeatTimeouts() {
    for (const auto& session : sessions_.timedOut()) {
        if (!session) {
            continue;
        }
        ServerLogger::instance().info(
            "session " + std::to_string(session->id()) + " heartbeat timed out");
        const auto sessionId = session->id();
        session->close();
        removeSession(sessionId);
    }
}

void TcpServer::markOfflineAndBroadcast(int userId) {
    presence_.setOffline(userId);
    users_.setOnlineStatus(userId, false);
    auto user = users_.findById(userId);
    if (!user) {
        return;
    }
    vendor::json::Object payload;
    payload["type"] = vendor::json::Value(lanchat::protocol::toInt(
        lanchat::protocol::MsgType::UserOffline));
    payload["id"] = vendor::json::Value(userId);
    payload["nickname"] = vendor::json::Value(user->nickname);
    broadcast(vendor::json::serialize(vendor::json::Value(payload)));
}

} // namespace lanchat::server
