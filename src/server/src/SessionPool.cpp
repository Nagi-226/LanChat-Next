#include "lanchat/server/SessionPool.h"

namespace lanchat::server {

SessionPool::SessionPool(std::size_t maxConnections)
    : max_connections_(maxConnections) {}

bool SessionPool::add(const std::shared_ptr<AsyncSession>& session) {
    if (!session || sessions_.size() >= max_connections_) {
        return false;
    }
    sessions_[session->id()] = session;
    return true;
}

void SessionPool::remove(std::uint64_t sessionId) {
    auto it = sessions_.find(sessionId);
    if (it != sessions_.end() && it->second && it->second->userId() != 0) {
        user_to_session_.erase(it->second->userId());
    }
    sessions_.erase(sessionId);
}

bool SessionPool::bindUser(std::uint64_t sessionId, int userId) {
    auto session = findSession(sessionId);
    if (!session || userId <= 0) {
        return false;
    }
    if (session->userId() != 0) {
        user_to_session_.erase(session->userId());
    }
    session->setUserId(userId);
    user_to_session_[userId] = sessionId;
    return true;
}

void SessionPool::unbindUser(int userId) {
    auto it = user_to_session_.find(userId);
    if (it == user_to_session_.end()) {
        return;
    }
    auto session = findSession(it->second);
    if (session) {
        session->setUserId(0);
    }
    user_to_session_.erase(it);
}

std::shared_ptr<AsyncSession> SessionPool::findSession(std::uint64_t sessionId) const {
    auto it = sessions_.find(sessionId);
    return it == sessions_.end() ? nullptr : it->second;
}

std::shared_ptr<AsyncSession> SessionPool::findByUserId(int userId) const {
    auto it = user_to_session_.find(userId);
    if (it == user_to_session_.end()) {
        return nullptr;
    }
    return findSession(it->second);
}

std::vector<std::shared_ptr<AsyncSession>> SessionPool::all() const {
    std::vector<std::shared_ptr<AsyncSession>> result;
    result.reserve(sessions_.size());
    for (const auto& [_, session] : sessions_) {
        if (session) {
            result.push_back(session);
        }
    }
    return result;
}

std::vector<std::shared_ptr<AsyncSession>> SessionPool::timedOut() const {
    std::vector<std::shared_ptr<AsyncSession>> result;
    for (const auto& [_, session] : sessions_) {
        if (session && session->isTimedOut()) {
            result.push_back(session);
        }
    }
    return result;
}

} // namespace lanchat::server
