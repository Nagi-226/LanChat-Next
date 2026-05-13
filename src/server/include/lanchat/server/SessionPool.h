#pragma once

#include "AsyncSession.h"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <unordered_map>
#include <vector>

namespace lanchat::server {

class SessionPool {
public:
    explicit SessionPool(std::size_t maxConnections = 500);

    bool add(const std::shared_ptr<AsyncSession>& session);
    void remove(std::uint64_t sessionId);
    bool bindUser(std::uint64_t sessionId, int userId);
    void unbindUser(int userId);

    std::shared_ptr<AsyncSession> findSession(std::uint64_t sessionId) const;
    std::shared_ptr<AsyncSession> findByUserId(int userId) const;
    std::vector<std::shared_ptr<AsyncSession>> all() const;
    std::vector<std::shared_ptr<AsyncSession>> timedOut() const;

    std::size_t size() const { return sessions_.size(); }
    std::size_t maxConnections() const { return max_connections_; }

private:
    std::size_t max_connections_;
    std::unordered_map<std::uint64_t, std::shared_ptr<AsyncSession>> sessions_;
    std::unordered_map<int, std::uint64_t> user_to_session_;
};

} // namespace lanchat::server
