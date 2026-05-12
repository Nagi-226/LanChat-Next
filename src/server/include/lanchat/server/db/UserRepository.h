#pragma once

#include "Database.h"
#include "message_types.h"

#include <optional>
#include <string>
#include <vector>

namespace lanchat::server::db {

using protocol::UserInfo;

class UserRepository {
public:
    explicit UserRepository(Database& db);

    std::optional<UserInfo> findById(int id);
    std::optional<UserInfo> findByNickname(const std::string& nickname);
    int create(const std::string& password, const std::string& nickname, int avatarId);
    bool verifyPassword(int userId, const std::string& password);
    bool updateProfile(int userId, const std::string& nickname, int avatarId);
    void setOnlineStatus(int userId, bool online);
    std::vector<UserInfo> getAllUsers();

private:
    Database& db_;
};

} // namespace lanchat::server::db
