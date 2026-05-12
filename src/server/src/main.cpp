#include "lanchat/server/LanChatServer.h"
#include "lanchat/server/ServerLogger.h"
#include "lanchat/server/db/LegacyMigrator.h"

#include <cstdlib>
#include <cstring>
#include <iostream>

int main(int argc, char* argv[]) {
    uint16_t port = 12346;
    std::string dataDir = "data";

    // Parse --migrate <path> and --port <N> flags
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--migrate") == 0 && i + 1 < argc) {
            lanchat::server::ServerLogger::instance().init("logs");
            lanchat::server::db::Database db(dataDir);
            db.open();
            auto report = lanchat::server::db::LegacyMigrator::migrateUsers(db, argv[++i]);
            std::cout << "Migration complete: "
                      << report.users_migrated << " migrated, "
                      << report.users_skipped << " skipped, "
                      << report.errors.size() << " errors\n";
            for (auto& e : report.errors) {
                std::cerr << "  " << e << "\n";
            }
            return 0;
        }
        if (std::strcmp(argv[i], "--port") == 0 && i + 1 < argc) {
            int p = std::atoi(argv[++i]);
            if (p > 0 && p <= 65535) port = static_cast<uint16_t>(p);
        }
        if (std::strcmp(argv[i], "--data") == 0 && i + 1 < argc) {
            dataDir = argv[++i];
        }
    }

    lanchat::server::LanChatServer server(port, dataDir);
    return server.run();
}
