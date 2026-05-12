#include "lanchat/server/db/Database.h"
#include "lanchat/server/ServerLogger.h"

#ifdef _WIN32
#include <direct.h>
#define mkdir(p) _mkdir(p)
#else
#include <sys/stat.h>
#endif

#include <algorithm>
#include <fstream>
#include <sstream>
#include <unordered_map>

namespace lanchat::server::db {

Database::Database(const std::string& dataDir) : data_dir_(dataDir) {}

Database::~Database() { close(); }

bool Database::open() {
    mkdir(data_dir_.c_str());

    // Load schema version
    if (!tableExists("schema_version")) {
        ensureTable("schema_version");
        Row v;
        v["version"] = vendor::json::Value(static_cast<int64_t>(0));
        insert("schema_version", v);
    }
    auto rows = query("schema_version");
    int version = rows.empty() ? 0 : static_cast<int>(rows[0].at("version").as_int());

    ServerLogger::instance().info(
        "Database opened: " + data_dir_ + ", schema v" + std::to_string(version));
    return true;
}

void Database::close() {
    std::lock_guard<std::mutex> lock(mutex_);
    tables_.clear();
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
    std::string path = data_dir_ + "/" + name + ".json";
    std::ifstream f(path);
    return f.good();
}

void Database::ensureTable(const std::string& name) {
    if (!tableExists(name)) {
        RowList empty;
        tables_[name] = empty;
        saveTable(name);
    }
}

RowList Database::query(const std::string& name,
                         std::function<bool(const Row&)> predicate) const {
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
    loadTable(name);
    tables_[name].push_back(row);
    saveTable(name);
}

void Database::update(const std::string& name,
                       std::function<bool(const Row&)> predicate,
                       std::function<void(Row&)> updater) {
    loadTable(name);
    auto& rows = tables_[name];
    for (auto& row : rows) {
        if (predicate(row)) updater(row);
    }
    saveTable(name);
}

void Database::remove(const std::string& name,
                       std::function<bool(const Row&)> predicate) {
    loadTable(name);
    auto& rows = tables_[name];
    rows.erase(std::remove_if(rows.begin(), rows.end(),
                               [&](const Row& r) { return predicate(r); }),
               rows.end());
    saveTable(name);
}

bool Database::loadTable(const std::string& name) const {
    if (tables_.find(name) != tables_.end()) return true;

    std::string path = data_dir_ + "/" + name + ".json";
    std::ifstream f(path);
    if (!f.good()) return false;

    std::stringstream buf;
    buf << f.rdbuf();
    std::string content = buf.str();
    if (content.empty()) {
        tables_[name] = {};
        return true;
    }

    auto val = vendor::json::parse(content);
    RowList rows;
    if (val.is_array()) {
        for (auto& item : val.arr) {
            if (item.is_object()) rows.push_back(item.obj);
        }
    }
    tables_[name] = std::move(rows);
    return true;
}

void Database::saveTable(const std::string& name) const {
    auto it = tables_.find(name);
    if (it == tables_.end()) return;

    vendor::json::Array arr;
    for (const auto& row : it->second) {
        arr.push_back(vendor::json::Value(row));
    }

    std::string path = data_dir_ + "/" + name + ".json";
    std::ofstream f(path, std::ios::trunc);
    f << vendor::json::serialize(vendor::json::Value(arr));
}

} // namespace lanchat::server::db
