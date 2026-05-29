#include "lanchat/server/db/Database.h"
#include "lanchat/server/db/UserRepository.h"
#include "sha256.hpp"

#include <cassert>
#include <chrono>
#include <filesystem>
#include <iostream>
#include <string>

using lanchat::server::db::Database;
using lanchat::server::db::UserRepository;

namespace {

std::string tempDbDir(const std::string& name)
{
    const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
    auto path = std::filesystem::temp_directory_path() / ("lanchat_" + name + "_" + std::to_string(stamp));
    return path.string();
}

void assertMigrationCreatesFriendshipsAndIsIdempotent()
{
    const auto dir = tempDbDir("migration");
    Database db(dir);
    assert(db.open());

    assert(db.migrate(1));
    assert(db.schemaVersion() == 1);
    assert(db.tableExists("friendships"));

    assert(db.migrate(1));
    assert(db.schemaVersion() == 1);

    db.close();
    std::filesystem::remove_all(dir);
}

void assertBcryptCreateAndShaUpgrade()
{
    const auto dir = tempDbDir("passwords");
    Database db(dir);
    assert(db.open());

    UserRepository users(db);
    const int id = users.create("secret", "alice", 3);
    assert(id > 0);
    auto created = db.query("users", [id](const auto& row) {
        return row.at("id").as_int() == id;
    });
    assert(created.size() == 1);
    assert(created[0].at("password_version").as_int() == 1);
    assert(created[0].at("password_hash").as_string().find("$2") == 0);
    assert(users.verifyPassword(id, "secret"));
    assert(!users.verifyPassword(id, "wrong"));

    lanchat::server::db::Row legacy;
    legacy["id"] = lanchat::vendor::json::Value(static_cast<std::int64_t>(id + 1));
    legacy["password_hash"] = lanchat::vendor::json::Value(lanchat::vendor::crypto::make_hash("legacy"));
    legacy["password_version"] = lanchat::vendor::json::Value(static_cast<std::int64_t>(0));
    legacy["nickname"] = lanchat::vendor::json::Value("legacy");
    legacy["avatar_id"] = lanchat::vendor::json::Value(static_cast<std::int64_t>(1));
    db.insert("users", legacy);

    assert(users.verifyPassword(id + 1, "legacy"));
    auto upgraded = db.query("users", [id](const auto& row) {
        return row.at("id").as_int() == id + 1;
    });
    assert(upgraded.size() == 1);
    assert(upgraded[0].at("password_version").as_int() == 1);
    assert(upgraded[0].at("password_hash").as_string().find("$2") == 0);

    db.close();
    std::filesystem::remove_all(dir);
}

} // namespace

int main()
{
    assertMigrationCreatesFriendshipsAndIsIdempotent();
    assertBcryptCreateAndShaUpgrade();
    std::cout << "Database tests passed\n";
    return 0;
}
