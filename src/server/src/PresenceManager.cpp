#include "lanchat/server/PresenceManager.h"

namespace lanchat::server {

void PresenceManager::setOnline(int userId, const std::string& status) {
    if (userId > 0) {
        presence_[userId] = status.empty() ? "online" : status;
    }
}

void PresenceManager::setOffline(int userId) {
    presence_.erase(userId);
}

bool PresenceManager::isOnline(int userId) const {
    return presence_.find(userId) != presence_.end();
}

std::string PresenceManager::status(int userId) const {
    auto it = presence_.find(userId);
    return it == presence_.end() ? "offline" : it->second;
}

std::vector<int> PresenceManager::onlineUsers() const {
    std::vector<int> result;
    result.reserve(presence_.size());
    for (const auto& [userId, _] : presence_) {
        result.push_back(userId);
    }
    return result;
}

} // namespace lanchat::server
