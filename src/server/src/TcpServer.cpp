#include "lanchat/server/TcpServer.h"

#include "lanchat/server/ServerLogger.h"
#include "lanchat/server/db/UserRepository.h"

#include <stdexcept>

namespace lanchat::server {

TcpServer::TcpServer(std::uint16_t port, db::UserRepository* users)
    : port_(port), users_(users) {}

TcpServer::~TcpServer() {
    stop();
}

int TcpServer::run() {
    try {
        ServerLogger::instance().init("logs");
        ServerLogger::instance().info("LanChat server-next v1.1.9-prep starting");

        acceptor_ = std::make_unique<vendor::asio::ip::tcp::acceptor>(ctx_, port_);
        ServerLogger::instance().info(
            "listening on 0.0.0.0:" + std::to_string(port_));

        startAccept();
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
        [this, session](vendor::asio::error_code ec) {
            if (!ec) {
                session->start();
                sessions_[session->id()] = session;
                ServerLogger::instance().debug(
                    "accepted session " + std::to_string(session->id())
                    + ", total: " + std::to_string(sessions_.size()));
            }
            startAccept(); // chain next accept
        });
}

void TcpServer::onMessage(std::shared_ptr<AsyncSession> session,
                          const std::string& json) {
    session->deliver(dispatchResponse(json));
}

void TcpServer::removeSession(uint64_t sessionId) {
    sessions_.erase(sessionId);
    ServerLogger::instance().debug(
        "session " + std::to_string(sessionId)
        + " removed, remaining: " + std::to_string(sessions_.size()));
}

void TcpServer::broadcast(const std::string& json, uint64_t excludeSessionId) {
    for (auto& [id, session] : sessions_) {
        if (id != excludeSessionId) {
            session->deliver(json);
        }
    }
}

size_t TcpServer::connectionCount() const {
    return sessions_.size();
}

std::string TcpServer::dispatchResponse(const std::string& request) const {
    // Heartbeat is handled by AsyncSession::onMessage (which also tracks
    // heartbeat timestamps).  This check remains as a safety net for any
    // future code path that calls dispatchResponse directly.
    if (request.find("\"type\":20") != std::string::npos ||
        request.find("\"type\": 20") != std::string::npos) {
        return R"({"type":21,"status":"ok"})";
    }

    // Quick field extraction helpers
    auto findInt = [&](const char* key) -> int {
        std::string pat = std::string("\"") + key + "\":";
        auto pos = request.find(pat);
        if (pos == std::string::npos) return -1;
        pos += pat.size();
        while (pos < request.size() && (request[pos] == ' ' || request[pos] == '\t')) ++pos;
        if (pos < request.size() && request[pos] == '"') return -1; // string, not int
        int val = 0;
        while (pos < request.size() && request[pos] >= '0' && request[pos] <= '9') {
            val = val * 10 + (request[pos] - '0');
            ++pos;
        }
        return val > 0 ? val : -1;
    };
    auto findStr = [&](const char* key) -> std::string {
        std::string pat = std::string("\"") + key + "\":\"";
        auto pos = request.find(pat);
        if (pos == std::string::npos) return "";
        pos += pat.size();
        std::string val;
        while (pos < request.size() && request[pos] != '"') {
            val += request[pos++];
        }
        return val;
    };

    // Register
    if (request.find("\"type\":0") != std::string::npos ||
        request.find("\"type\": 0") != std::string::npos) {
        if (users_) {
            std::string nickname = findStr("nickname");
            std::string password = findStr("password");
            if (nickname.empty()) nickname = findStr("name");
            if (nickname.empty()) nickname = "user_" + std::to_string(std::time(nullptr));
            if (password.empty()) password = "default";
            int id = users_->create(password, nickname, 0);
            if (id > 0) {
                return R"({"type":1,"status":"ok","id":)" + std::to_string(id)
                     + R"(,"msg":"registered"})";
            }
            return R"({"type":1,"status":"error","msg":"nickname taken or invalid"})";
        }
        return R"xx({"type":1,"status":"ok","id":1001,"msg":"registered (no db)"})xx";
    }

    // Login
    if (request.find("\"type\":2") != std::string::npos ||
        request.find("\"type\": 2") != std::string::npos) {
        if (users_) {
            int uid = findInt("id");
            std::string password = findStr("password");
            if (uid > 0 && users_->verifyPassword(uid, password)) {
                auto user = users_->findById(uid);
                if (user.has_value()) {
                    return R"({"type":3,"id":)" + std::to_string(uid)
                         + R"(,"nickname":")" + user->nickname
                         + R"(","headId":)" + std::to_string(user->avatar_id)
                         + R"(,"friends":[],"groups":[]})";
                }
            }
            return R"({"type":4,"status":"error","code":401,"msg":"invalid credentials"})";
        }
        return R"({"type":3,"id":1,"nickname":"Demo User","headId":0,"friends":[],"groups":[]})";
    }

    // Send message (echo fallback)
    if (request.find("\"type\":5") != std::string::npos ||
        request.find("\"type\": 5") != std::string::npos) {
        std::string msg = findStr("msg");
        if (msg.empty()) msg = "echo";
        return R"({"type":6,"fromId":0,"toId":0,"nickname":"server","headId":0,"msg":")"
             + msg + R"(","content_type":"text"})";
    }

    return R"({"type":33,"status":"ok","msg":"server-next v1.2.0 received frame"})";
}

} // namespace lanchat::server
