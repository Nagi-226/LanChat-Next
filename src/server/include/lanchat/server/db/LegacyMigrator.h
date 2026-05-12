#pragma once

#include "Database.h"
#include "UserRepository.h"

#include <string>
#include <vector>

namespace lanchat::server::db {

class LegacyMigrator {
public:
    struct MigrationReport {
        int users_migrated = 0;
        int users_skipped = 0;
        std::vector<std::string> errors;
    };

    // Attempt to migrate users from a legacy JSON dump file.
    // Expected format: array of objects with "id", "password", "nickname", "headId" fields.
    static MigrationReport migrateUsers(Database& db, const std::string& legacyJsonPath);
};

} // namespace lanchat::server::db
