#include "lanchat/server/db/LegacyMigrator.h"
#include "lanchat/server/ServerLogger.h"
#include "mini_json.hpp"

#include <fstream>
#include <sstream>

namespace lanchat::server::db {

LegacyMigrator::MigrationReport LegacyMigrator::migrateUsers(
    Database& db, const std::string& legacyJsonPath) {
    MigrationReport report;

    std::ifstream f(legacyJsonPath);
    if (!f.good()) {
        report.errors.push_back("Cannot open legacy file: " + legacyJsonPath);
        return report;
    }

    std::stringstream buf;
    buf << f.rdbuf();
    auto val = vendor::json::parse(buf.str());
    if (!val.is_array()) {
        report.errors.push_back("Legacy file is not a JSON array");
        return report;
    }

    UserRepository userRepo(db);

    for (auto& item : val.arr) {
        if (!item.is_object()) continue;

        const auto& obj = item.obj;
        if (!obj.count("id") || !obj.count("password") || !obj.count("nickname")) {
            report.errors.push_back("Skipping legacy user with missing fields");
            report.users_skipped++;
            continue;
        }

        int legacyId = static_cast<int>(obj.at("id").as_int());
        std::string password = obj.at("password").as_string();
        std::string nickname = obj.at("nickname").as_string();
        int headId = obj.count("headId") ? static_cast<int>(obj.at("headId").as_int()) : 0;

        // Check if already migrated (by nickname)
        auto existing = userRepo.findByNickname(nickname);
        if (existing.has_value()) {
            report.users_skipped++;
            continue;
        }

        int newId = userRepo.create(password, nickname, headId);
        if (newId < 0) {
            report.errors.push_back("Failed to create user: " + nickname);
            report.users_skipped++;
        } else {
            report.users_migrated++;
        }
    }

    ServerLogger::instance().info(
        "Legacy migration complete: " + std::to_string(report.users_migrated)
        + " migrated, " + std::to_string(report.users_skipped) + " skipped");

    return report;
}

} // namespace lanchat::server::db
