#include "lanchat/server/LanChatServer.h"
#include "lanchat/server/ServerLogger.h"
#include "lanchat/server/db/Database.h"
#include "lanchat/server/db/LegacyMigrator.h"
#include "lanchat/server/db/UserRepository.h"

#include <cstdlib>
#include <cstring>
#include <iostream>

int main(int argc, char* argv[]) {
    uint16_t port = 12346;
    std::string dataDir = "data";

    // 第一遍：只解析参数（--port / --data / 模式开关）。
    // 模式动作必须等所有参数解析完再执行——否则 --init-admin 排在 --data 前面时
    // 会拿着默认 dataDir 就 return，把种子写错库（实测踩过）。
    enum class Mode { Run, Migrate, InitAdmin };
    Mode mode = Mode::Run;
    std::string migratePath;
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--init-admin") == 0) {
            mode = Mode::InitAdmin;
        } else if (std::strcmp(argv[i], "--migrate") == 0 && i + 1 < argc) {
            mode = Mode::Migrate;
            migratePath = argv[++i];
        } else if (std::strcmp(argv[i], "--port") == 0 && i + 1 < argc) {
            int p = std::atoi(argv[++i]);
            if (p <= 0 || p >= 65535) {
                std::cerr << "Invalid port: " << p << " (expected 1-65534)\n";
                return 2;
            }
            if (p < 1024) {
                std::cerr << "Warning: port " << p << " is privileged on some systems\n";
            }
            port = static_cast<uint16_t>(p);
        } else if (std::strcmp(argv[i], "--data") == 0 && i + 1 < argc) {
            dataDir = argv[++i];
        }
    }

    // 第二遍：执行模式动作
    if (mode == Mode::InitAdmin) {
        // 管理员种子/提升：凭据只从环境变量读（不进 argv/日志/仓库）。
        // 已存在同名账户则保留其 id 提升为 admin 并重设密码；不存在则新建。
        const char* nick = std::getenv("LANCHAT_ADMIN_NICK");
        const char* pass = std::getenv("LANCHAT_ADMIN_PASS");
        if (!nick || !pass || !*nick || !*pass) {
            std::cerr << "--init-admin requires LANCHAT_ADMIN_NICK and LANCHAT_ADMIN_PASS env vars\n";
            return 2;
        }
        lanchat::server::db::Database db(dataDir);
        if (!db.open()) {
            std::cerr << "cannot open database in " << dataDir << "\n";
            return 2;
        }
        lanchat::server::db::UserRepository users(db);
        const int id = users.ensureAdmin(nick, pass);
        if (id <= 0) {
            std::cerr << "admin seed failed\n";
            return 2;
        }
        std::cout << "admin ready: id=" << id << " nickname=" << nick
                  << " data=" << dataDir << "\n";
        return 0;
    }
    if (mode == Mode::Migrate) {
        lanchat::server::ServerLogger::instance().init("logs");
        lanchat::server::db::Database db(dataDir);
        db.open();
        auto report = lanchat::server::db::LegacyMigrator::migrateUsers(db, migratePath);
        std::cout << "Migration complete: "
                  << report.users_migrated << " migrated, "
                  << report.users_skipped << " skipped, "
                  << report.errors.size() << " errors\n";
        for (auto& e : report.errors) {
            std::cerr << "  " << e << "\n";
        }
        return 0;
    }

    lanchat::server::LanChatServer server(port, dataDir);
    return server.run();
}
