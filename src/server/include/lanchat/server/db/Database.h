#pragma once

#include "mini_json.hpp"

#include <cstdint>
#include <functional>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

struct sqlite3;

namespace lanchat::server::db {

using Row = vendor::json::Object;
using RowList = std::vector<Row>;

class Database {
public:
    explicit Database(const std::string& dataDir);
    ~Database();

    Database(const Database&) = delete;
    Database& operator=(const Database&) = delete;

    // Lifecycle
    bool open();
    void close();
    int schemaVersion() const;
    void setSchemaVersion(int version);
    bool migrate(int targetVersion);

    // Table operations
    bool tableExists(const std::string& name) const;
    void ensureTable(const std::string& name);
    void createIndex(const std::string& table,
                     const std::string& indexName,
                     const std::string& expression);
    RowList query(const std::string& table,
                  std::function<bool(const Row&)> predicate = nullptr) const;
    void insert(const std::string& table, const Row& row);
    void update(const std::string& table,
                std::function<bool(const Row&)> predicate,
                std::function<void(Row&)> updater);
    void remove(const std::string& table,
                std::function<bool(const Row&)> predicate);

    // Atomic read-modify-write
    template<typename F>
    auto withLock(F&& fn) -> decltype(fn()) {
        std::lock_guard<std::recursive_mutex> lock(mutex_);
        return fn();
    }

    std::string dataDir() const { return data_dir_; }
    sqlite3* nativeHandle() const { return db_; }

private:
    bool loadTable(const std::string& name) const;
    void saveTable(const std::string& name) const;
    bool execSql(const std::string& sql) const;
    void createFriendshipsTable();

    std::string data_dir_;
    sqlite3* db_ = nullptr;
    // In-memory tables: table name -> list of row objects
    mutable std::unordered_map<std::string, RowList> tables_;
    mutable std::recursive_mutex mutex_;
};

} // namespace lanchat::server::db
