#pragma once

#include <string>
#include <unordered_map>
#include <vector>

namespace lanchat::server {

class PresenceManager {
public:
    void setOnline(int userId, const std::string& status = "ok");
    void setOffline(int userId);
    bool isOnline(int userId) const;
    std::string status(int userId) const;
    std::vector<int> onlineUsers() const;

private:
    std::unordered_map<int, std::string> presence_;
};

} // namespace lanchat::server
