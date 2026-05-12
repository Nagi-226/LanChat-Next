#include "lanchat/server/db/UserRepository.h"

#include "sha256.hpp"

#include <algorithm>
#include <ctime>

namespace lanchat::server::db {

UserRepository::UserRepository(Database& db) : db_(db) {
    db_.ensureTable("users");
}

std::optional<UserInfo> UserRepository::findById(int id) {
    auto rows = db_.query("users", [id](const Row& r) {
        return r.at("id").as_int() == id;
    });
    if (rows.empty()) return std::nullopt;

    const auto& r = rows[0];
    UserInfo u;
    u.id = static_cast<int>(r.at("id").as_int());
    u.password = r.at("password_hash").as_string();
    u.nickname = r.at("nickname").as_string();
    u.avatar_id = static_cast<int>(r.at("avatar_id").as_int());
    return u;
}

std::optional<UserInfo> UserRepository::findByNickname(const std::string& nickname) {
    auto rows = db_.query("users", [&nickname](const Row& r) {
        return r.at("nickname").as_string() == nickname;
    });
    if (rows.empty()) return std::nullopt;

    const auto& r = rows[0];
    UserInfo u;
    u.id = static_cast<int>(r.at("id").as_int());
    u.password = r.at("password_hash").as_string();
    u.nickname = r.at("nickname").as_string();
    u.avatar_id = static_cast<int>(r.at("avatar_id").as_int());
    return u;
}

int UserRepository::create(const std::string& password, const std::string& nickname,
                            int avatarId) {
    // Check duplicate nickname
    auto existing = findByNickname(nickname);
    if (existing.has_value()) return -1;

    // Find max id
    int maxId = 1000;
    auto all = db_.query("users");
    for (const auto& r : all) {
        int rid = static_cast<int>(r.at("id").as_int());
        if (rid >= maxId) maxId = rid + 1;
    }

    Row row;
    row["id"] = vendor::json::Value(static_cast<int64_t>(maxId));
    row["password_hash"] = vendor::json::Value(vendor::crypto::make_hash(password));
    row["nickname"] = vendor::json::Value(nickname);
    row["avatar_id"] = vendor::json::Value(static_cast<int64_t>(avatarId));
    row["created_at"] = vendor::json::Value(static_cast<int64_t>(std::time(nullptr)));
    row["last_seen_at"] = vendor::json::Value(static_cast<int64_t>(0));
    row["is_online"] = vendor::json::Value(static_cast<int64_t>(0));

    db_.insert("users", row);
    return maxId;
}

bool UserRepository::verifyPassword(int userId, const std::string& password) {
    auto user = findById(userId);
    if (!user.has_value()) return false;
    return vendor::crypto::verify_hash(password, user->password);
}

bool UserRepository::updateProfile(int userId, const std::string& nickname, int avatarId) {
    bool found = false;
    db_.update("users", [userId](const Row& r) {
        return r.at("id").as_int() == userId;
    }, [&nickname, avatarId, &found](Row& r) {
        r["nickname"] = vendor::json::Value(nickname);
        r["avatar_id"] = vendor::json::Value(static_cast<int64_t>(avatarId));
        found = true;
    });
    return found;
}

void UserRepository::setOnlineStatus(int userId, bool online) {
    db_.update("users", [userId](const Row& r) {
        return r.at("id").as_int() == userId;
    }, [online](Row& r) {
        r["is_online"] = vendor::json::Value(static_cast<int64_t>(online ? 1 : 0));
        if (online) {
            r["last_seen_at"] = vendor::json::Value(static_cast<int64_t>(std::time(nullptr)));
        }
    });
}

std::vector<UserInfo> UserRepository::getAllUsers() {
    std::vector<UserInfo> result;
    auto rows = db_.query("users");
    for (const auto& r : rows) {
        UserInfo u;
        u.id = static_cast<int>(r.at("id").as_int());
        u.password = r.at("password_hash").as_string();
        u.nickname = r.at("nickname").as_string();
        u.avatar_id = static_cast<int>(r.at("avatar_id").as_int());
        result.push_back(u);
    }
    return result;
}

} // namespace lanchat::server::db
