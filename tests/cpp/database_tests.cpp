#include "lanchat/server/db/Database.h"
#include "lanchat/server/db/UserRepository.h"
#include "lanchat_bcrypt.hpp"
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
    // v2 = bcrypt(sha256hex(password))：预哈希防 bcrypt 72 字节截断
    assert(created[0].at("password_version").as_int() == 2);
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
    // legacy v0 登录成功后应一路升到 v2（不是停在 v1）
    assert(upgraded[0].at("password_version").as_int() == 2);
    assert(upgraded[0].at("password_hash").as_string().find("$2") == 0);
    // 升级后复登仍通过（v2 路径）
    assert(users.verifyPassword(id + 1, "legacy"));

    db.close();
    std::filesystem::remove_all(dir);
}

// v1 老哈希（bcrypt 直哈，带 72 字节截断）登录成功应自动升 v2；
// 且 v2 新注册不再受截断影响：前 72 字节相同的长密码不再等价。
void assertV1UpgradeAndTruncationDefeated()
{
    const auto dir = tempDbDir("v2upgrade");
    Database db(dir);
    assert(db.open());

    UserRepository users(db);

    // 手工塞一条 v1 行：bcrypt 直哈 80 字符密码（模拟修复前注册的老用户）
    const std::string long_pw = std::string(72, 'A') + "TAIL-SECRET";
    lanchat::server::db::Row v1;
    v1["id"] = lanchat::vendor::json::Value(static_cast<std::int64_t>(2001));
    v1["password_hash"] = lanchat::vendor::json::Value(
        lanchat::vendor::bcrypt::hash_password(long_pw, 12));
    v1["password_version"] = lanchat::vendor::json::Value(static_cast<std::int64_t>(1));
    v1["nickname"] = lanchat::vendor::json::Value("v1user");
    v1["avatar_id"] = lanchat::vendor::json::Value(static_cast<std::int64_t>(0));
    db.insert("users", v1);

    // v1 路径：完整密码能登（截断行为仍在），登录后升 v2
    assert(users.verifyPassword(2001, long_pw));
    auto row = db.query("users", [](const auto& r) { return r.at("id").as_int() == 2001; });
    assert(row.size() == 1);
    assert(row[0].at("password_version").as_int() == 2);
    // 升 v2 后：前 72 字符不再能登（截断等价被切断）
    assert(!users.verifyPassword(2001, std::string(72, 'A')));
    assert(users.verifyPassword(2001, long_pw));

    // v2 新注册：一开始就不受截断影响
    const int nid = users.create(long_pw, "v2user", 0);
    assert(nid > 0);
    assert(users.verifyPassword(nid, long_pw));
    assert(!users.verifyPassword(nid, std::string(72, 'A')));

    db.close();
    std::filesystem::remove_all(dir);
}

} // namespace

// 管理员种子：新建带 is_admin；同名已存在则保留 id 提升并重设密码；
// 普通 create 永远不带 is_admin（权限不能靠注册拿到）。
void assertEnsureAdminPromotesAndResetsPassword()
{
    const auto dir = tempDbDir("admin");
    Database db(dir);
    assert(db.open());

    UserRepository users(db);
    const int id = users.create("old-pass", "nagi", 0);
    assert(id > 0);
    assert(!users.isAdmin(id));

    const int same = users.ensureAdmin("nagi", "new-pass");
    assert(same == id);  // 保留原 id（客户端记着它）
    assert(users.isAdmin(id));
    assert(users.verifyPassword(id, "new-pass"));
    assert(!users.verifyPassword(id, "old-pass"));

    const int fresh = users.ensureAdmin("root", "root-pass");
    assert(fresh > 0 && fresh != id);
    assert(users.isAdmin(fresh));
    assert(users.verifyPassword(fresh, "root-pass"));

    db.close();
    std::filesystem::remove_all(dir);
}

int main()
{
    assertMigrationCreatesFriendshipsAndIsIdempotent();
    assertBcryptCreateAndShaUpgrade();
    assertV1UpgradeAndTruncationDefeated();
    assertEnsureAdminPromotesAndResetsPassword();
    std::cout << "Database tests passed\n";
    return 0;
}
