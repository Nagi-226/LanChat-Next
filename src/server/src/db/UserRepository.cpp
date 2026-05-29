#include "lanchat/server/db/UserRepository.h"

#include "lanchat_bcrypt.hpp"
#include "sha256.hpp"

#include <algorithm>
#include <ctime>

namespace lanchat::server::db {

namespace {

int passwordVersion(const Row& row) {
    auto it = row.find("password_version");
    if (it == row.end() || !it->second.is_number()) return 0;
    return static_cast<int>(it->second.as_int());
}

UserInfo userFromRow(const Row& r) {
    UserInfo u;
    u.id = static_cast<int>(r.at("id").as_int());
    u.password = r.at("password_hash").as_string();
    u.nickname = r.at("nickname").as_string();
    u.avatar_id = static_cast<int>(r.at("avatar_id").as_int());
    return u;
}

} // namespace

UserRepository::UserRepository(Database& db) : db_(db) {
    db_.ensureTable("users");
}

std::optional<UserInfo> UserRepository::findById(int id) {
    auto rows = db_.query("users", [id](const Row& r) {
        return r.at("id").as_int() == id;
    });
    if (rows.empty()) return std::nullopt;

    return userFromRow(rows[0]);
}

std::optional<UserInfo> UserRepository::findByNickname(const std::string& nickname) {
    auto rows = db_.query("users", [&nickname](const Row& r) {
        return r.at("nickname").as_string() == nickname;
    });
    if (rows.empty()) return std::nullopt;

    return userFromRow(rows[0]);
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
    row["password_hash"] = vendor::json::Value(vendor::bcrypt::hash_password(password, 12));
    row["password_version"] = vendor::json::Value(static_cast<int64_t>(1));
    row["nickname"] = vendor::json::Value(nickname);
    row["avatar_id"] = vendor::json::Value(static_cast<int64_t>(avatarId));
    row["created_at"] = vendor::json::Value(static_cast<int64_t>(std::time(nullptr)));
    row["last_seen_at"] = vendor::json::Value(static_cast<int64_t>(0));
    row["is_online"] = vendor::json::Value(static_cast<int64_t>(0));

    db_.insert("users", row);
    return maxId;
}

bool UserRepository::verifyPassword(int userId, const std::string& password) {
    auto rows = db_.query("users", [userId](const Row& r) {
        return r.at("id").as_int() == userId;
    });
    if (rows.empty()) return false;

    const auto& row = rows[0];
    const std::string stored = row.at("password_hash").as_string();
    const int version = passwordVersion(row);

    if (version >= 1) {
        return vendor::bcrypt::verify_password(password, stored);
    }

    if (!vendor::crypto::verify_hash(password, stored)) {
        return false;
    }

    const std::string upgraded = vendor::bcrypt::hash_password(password, 12);
    db_.update("users", [userId](const Row& r) {
        return r.at("id").as_int() == userId;
    }, [&upgraded](Row& r) {
        r["password_hash"] = vendor::json::Value(upgraded);
        r["password_version"] = vendor::json::Value(static_cast<int64_t>(1));
    });
    return true;
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
        result.push_back(userFromRow(r));
    }
    return result;
}

} // namespace lanchat::server::db
