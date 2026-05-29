#include "lanchat/server/db/Database.h"
#include "lanchat/server/ServerLogger.h"

#include <sqlite3.h>

#ifdef _WIN32
#include <direct.h>
#define mkdir(p) _mkdir(p)
#else
#include <sys/stat.h>
#endif

#include <algorithm>
#include <cctype>
#include <sstream>

namespace lanchat::server::db {

namespace {

bool isIdentifier(const std::string& value) {
    if (value.empty()) return false;
    return std::all_of(value.begin(), value.end(), [](unsigned char ch) {
        return std::isalnum(ch) || ch == '_';
    });
}

} // namespace

Database::Database(const std::string& dataDir) : data_dir_(dataDir) {}

Database::~Database() { close(); }

bool Database::open() {
    mkdir(data_dir_.c_str());

    std::string dbPath = data_dir_ + "/lanchat.db";
    int rc = sqlite3_open(dbPath.c_str(), &db_);
    if (rc != SQLITE_OK) {
        ServerLogger::instance().error(
            "Failed to open database: " + std::string(sqlite3_errmsg(db_)));
        return false;
    }

    sqlite3_exec(db_, "PRAGMA journal_mode=WAL;", nullptr, nullptr, nullptr);
    sqlite3_exec(db_, "PRAGMA foreign_keys=ON;", nullptr, nullptr, nullptr);

    if (!tableExists("schema_version")) {
        ensureTable("schema_version");
        Row v;
        v["version"] = vendor::json::Value(static_cast<int64_t>(0));
        insert("schema_version", v);
    }

    int version = schemaVersion();
    ServerLogger::instance().info(
        "Database opened: " + dbPath + ", schema v" + std::to_string(version));
    return true;
}

void Database::close() {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    tables_.clear();
    if (db_) {
        sqlite3_close(db_);
        db_ = nullptr;
    }
}

int Database::schemaVersion() const {
    auto rows = query("schema_version");
    if (rows.empty()) return 0;
    return static_cast<int>(rows[0].at("version").as_int());
}

void Database::setSchemaVersion(int version) {
    auto rows = query("schema_version");
    if (rows.empty()) {
        Row v;
        v["version"] = vendor::json::Value(static_cast<int64_t>(version));
        insert("schema_version", v);
    } else {
        update("schema_version", [](const Row&) { return true; },
               [version](Row& r) { r["version"] = vendor::json::Value(static_cast<int64_t>(version)); });
    }
}

bool Database::tableExists(const std::string& name) const {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    sqlite3_stmt* stmt = nullptr;
    bool exists = false;
    const char* sql = "SELECT 1 FROM sqlite_master WHERE type='table' AND name=?;";
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, name.c_str(), -1, SQLITE_STATIC);
        exists = (sqlite3_step(stmt) == SQLITE_ROW);
    }
    sqlite3_finalize(stmt);
    return exists;
}

void Database::ensureTable(const std::string& name) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    if (tableExists(name)) return;
    char* errMsg = nullptr;
    std::string sql = "CREATE TABLE \"" + name + "\" (data TEXT NOT NULL);";
    if (sqlite3_exec(db_, sql.c_str(), nullptr, nullptr, &errMsg) != SQLITE_OK) {
        ServerLogger::instance().error("Failed to ensure table " + name + ": " +
                                       (errMsg ? errMsg : "unknown error"));
        sqlite3_free(errMsg);
    }
}

void Database::createIndex(const std::string& table,
                           const std::string& indexName,
                           const std::string& expression) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    if (!isIdentifier(table) || !isIdentifier(indexName) || expression.empty()) {
        ServerLogger::instance().error("Refusing unsafe index definition: " + indexName);
        return;
    }
    const std::string sql = "CREATE INDEX IF NOT EXISTS \"" + indexName + "\" ON \"" +
                            table + "\"(" + expression + ");";
    execSql(sql);
}

bool Database::migrate(int targetVersion) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    int current = schemaVersion();
    if (targetVersion < current) return false;

    while (current < targetVersion) {
        if (current == 0) {
            ensureTable("users");
            ensureTable("messages");
            ensureTable("groups");
            ensureTable("group_members");

            createIndex("users", "idx_users_id", "json_extract(data, '$.id')");
            createIndex("users", "idx_users_nickname", "json_extract(data, '$.nickname')");
            createIndex("messages", "idx_messages_from_id", "json_extract(data, '$.from_id')");
            createIndex("messages", "idx_messages_to_id", "json_extract(data, '$.to_id')");
            createIndex("messages", "idx_messages_group_id", "json_extract(data, '$.group_id')");
            createIndex("messages", "idx_messages_timestamp", "json_extract(data, '$.timestamp')");
            createIndex("groups", "idx_groups_group_id", "json_extract(data, '$.group_id')");
            createIndex("group_members", "idx_gm_user_id", "json_extract(data, '$.user_id')");
            createIndex("group_members", "idx_gm_group_id", "json_extract(data, '$.group_id')");
            createFriendshipsTable();

            setSchemaVersion(1);
            current = 1;
            continue;
        }

        // Version slots above 1 are intentionally reserved for future migrations.
        setSchemaVersion(current + 1);
        ++current;
    }

    return true;
}

RowList Database::query(const std::string& name,
                         std::function<bool(const Row&)> predicate) const {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    loadTable(name);
    auto it = tables_.find(name);
    if (it == tables_.end()) return {};
    if (!predicate) return it->second;
    RowList result;
    for (const auto& row : it->second) {
        if (predicate(row)) result.push_back(row);
    }
    return result;
}

void Database::insert(const std::string& name, const Row& row) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    loadTable(name);
    tables_[name].push_back(row);
    saveTable(name);
}

void Database::update(const std::string& name,
                       std::function<bool(const Row&)> predicate,
                       std::function<void(Row&)> updater) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    loadTable(name);
    auto& rows = tables_[name];
    for (auto& row : rows) {
        if (predicate(row)) updater(row);
    }
    saveTable(name);
}

void Database::remove(const std::string& name,
                       std::function<bool(const Row&)> predicate) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    loadTable(name);
    auto& rows = tables_[name];
    rows.erase(std::remove_if(rows.begin(), rows.end(),
                               [&](const Row& r) { return predicate(r); }),
               rows.end());
    saveTable(name);
}

bool Database::loadTable(const std::string& name) const {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    if (tables_.find(name) != tables_.end()) return true;

    RowList rows;
    sqlite3_stmt* stmt = nullptr;
    std::string sql = "SELECT data FROM \"" + name + "\";";
    if (sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        tables_[name] = {};
        return true;
    }

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        const char* text = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        if (text && text[0] != '\0') {
            auto val = vendor::json::parse(std::string(text));
            if (val.is_object()) {
                rows.push_back(std::move(val.obj));
            }
        }
    }
    sqlite3_finalize(stmt);
    tables_[name] = std::move(rows);
    return true;
}

void Database::saveTable(const std::string& name) const {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    auto it = tables_.find(name);
    if (it == tables_.end()) return;

    sqlite3_exec(db_, "BEGIN TRANSACTION;", nullptr, nullptr, nullptr);

    std::string delSql = "DELETE FROM \"" + name + "\";";
    char* errMsg = nullptr;
    if (sqlite3_exec(db_, delSql.c_str(), nullptr, nullptr, &errMsg) != SQLITE_OK) {
        ServerLogger::instance().error("Failed to clear table " + name + ": " +
                                       (errMsg ? errMsg : "unknown error"));
        sqlite3_free(errMsg);
        sqlite3_exec(db_, "ROLLBACK;", nullptr, nullptr, nullptr);
        return;
    }

    sqlite3_stmt* stmt = nullptr;
    std::string insSql = "INSERT INTO \"" + name + "\" (data) VALUES (?);";
    if (sqlite3_prepare_v2(db_, insSql.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
        for (const auto& row : it->second) {
            std::string json = vendor::json::serialize(vendor::json::Value(row));
            sqlite3_bind_text(stmt, 1, json.c_str(), static_cast<int>(json.size()), SQLITE_TRANSIENT);
            sqlite3_step(stmt);
            sqlite3_reset(stmt);
        }
        sqlite3_finalize(stmt);
    }

    sqlite3_exec(db_, "COMMIT;", nullptr, nullptr, nullptr);
}

bool Database::execSql(const std::string& sql) const {
    char* errMsg = nullptr;
    if (sqlite3_exec(db_, sql.c_str(), nullptr, nullptr, &errMsg) != SQLITE_OK) {
        ServerLogger::instance().error("SQL failed: " + std::string(errMsg ? errMsg : "unknown error"));
        sqlite3_free(errMsg);
        return false;
    }
    return true;
}

void Database::createFriendshipsTable() {
    execSql(
        "CREATE TABLE IF NOT EXISTS friendships ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "from_uid INTEGER NOT NULL,"
        "to_uid INTEGER NOT NULL,"
        "status TEXT NOT NULL CHECK(status IN ('pending','accepted','rejected')),"
        "request_msg TEXT DEFAULT '',"
        "created_at INTEGER NOT NULL,"
        "updated_at INTEGER NOT NULL,"
        "UNIQUE(from_uid, to_uid)"
        ");");
    execSql("CREATE INDEX IF NOT EXISTS idx_fs_from ON friendships(from_uid, status);");
    execSql("CREATE INDEX IF NOT EXISTS idx_fs_to ON friendships(to_uid, status);");
    execSql("CREATE INDEX IF NOT EXISTS idx_fs_both ON friendships(from_uid, to_uid);");
}

} // namespace lanchat::server::db
