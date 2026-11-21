#include "lanchat/server/db/UserRepository.h"

#include "lanchat_bcrypt.hpp"
#include "sha256.hpp"
#include "sha256_hex.hpp"

#include <algorithm>
#include <ctime>

namespace lanchat::server::db {

namespace {

int passwordVersion(const Row& row) {
    auto it = row.find("password_version");
    if (it == row.end() || !it->second.is_number()) return 0;
    return static_cast<int>(it->second.as_int());
}

// 预哈希：先把任意长密码压成 64 字符 SHA-256 hex，再喂 bcrypt。
// 原因：bcrypt 只读前 72 字节，直接哈长密码会让"前 72 字节相同的两个密码"等价。
// 64 字符 hex < 72 字节，截断不再可能发生。真 SHA-256 见 vendor/sha256_hex.hpp
// （vendor/sha256.hpp 是玩具哈希，只用于 legacy v0 校验，不能拿来预哈希）。
std::string prehash(const std::string& password) {
    return vendor::sha256::hex_digest(password);
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
    // v2 = bcrypt(sha256hex(password))：防 72 字节截断，见 prehash 注释
    row["password_hash"] = vendor::json::Value(vendor::bcrypt::hash_password(prehash(password), 12));
    row["password_version"] = vendor::json::Value(static_cast<int64_t>(2));
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

    // v2（当前格式）：bcrypt 校验预哈希后的密码
    if (version >= 2) {
        return vendor::bcrypt::verify_password(prehash(password), stored);
    }

    // v1 = bcrypt 直哈（带截断的老格式）；v0 = 玩具哈希 legacy 格式。
    // 两条老路径保留，让存量用户仍能登录；登录成功即升级到 v2（一次性迁移）。
    const bool ok = (version == 1)
                        ? vendor::bcrypt::verify_password(password, stored)
                        : vendor::crypto::verify_hash(password, stored);
    if (!ok) {
        return false;
    }

    const std::string upgraded = vendor::bcrypt::hash_password(prehash(password), 12);
    db_.update("users", [userId](const Row& r) {
        return r.at("id").as_int() == userId;
    }, [&upgraded](Row& r) {
        r["password_hash"] = vendor::json::Value(upgraded);
        r["password_version"] = vendor::json::Value(static_cast<int64_t>(2));
    });
    return true;
}

// 管理员种子/提升：按昵称找账户——已存在则保留原 id（客户端 localStorage 记着它），
// 重设密码为给定值（v2 预哈希）并置 is_admin=1；不存在则新建带 is_admin 的账户。
// 只由服务端启动参数 --init-admin 调用（凭据走环境变量），普通注册永远拿不到 is_admin。
int UserRepository::ensureAdmin(const std::string& nickname, const std::string& password) {
    const std::string hash = vendor::bcrypt::hash_password(prehash(password), 12);
    auto existing = findByNickname(nickname);
    if (existing.has_value()) {
        const int id = existing->id;
        db_.update("users", [id](const Row& r) {
            return r.at("id").as_int() == id;
        }, [&hash](Row& r) {
            r["password_hash"] = vendor::json::Value(hash);
            r["password_version"] = vendor::json::Value(static_cast<int64_t>(2));
            r["is_admin"] = vendor::json::Value(static_cast<int64_t>(1));
        });
        return id;
    }
    const int id = create(password, nickname, 0);
    if (id <= 0) return id;
    db_.update("users", [id](const Row& r) {
        return r.at("id").as_int() == id;
    }, [](Row& r) {
        r["is_admin"] = vendor::json::Value(static_cast<int64_t>(1));
    });
    return id;
}

bool UserRepository::isAdmin(int userId) {
    auto rows = db_.query("users", [userId](const Row& r) {
        return r.at("id").as_int() == userId;
    });
    if (rows.empty()) return false;
    auto it = rows[0].find("is_admin");
    return it != rows[0].end() && it->second.is_number() && it->second.as_int() == 1;
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
