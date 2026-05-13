#include "lanchat/server/db/ChannelRepository.h"

#include <algorithm>
#include <ctime>

namespace lanchat::server::db {

namespace {

int asInt(const Row& row, const std::string& key, int fallback = 0) {
    auto it = row.find(key);
    if (it == row.end() || !it->second.is_number()) {
        return fallback;
    }
    return static_cast<int>(it->second.as_int());
}

std::string asString(
    const Row& row,
    const std::string& key,
    const std::string& fallback = "") {
    auto it = row.find(key);
    if (it == row.end() || !it->second.is_string()) {
        return fallback;
    }
    return it->second.as_string();
}

} // namespace

ChannelRepository::ChannelRepository(Database& db) : db_(db) {
    db_.ensureTable("groups");
    db_.ensureTable("group_members");
}

ChannelInfo ChannelRepository::createGroup(int hostId, const std::string& name) {
    ChannelInfo group;
    group.group_id = nextGroupId();
    group.host_id = hostId;
    group.name = name;

    Row row;
    row["group_id"] = vendor::json::Value(group.group_id);
    row["host_id"] = vendor::json::Value(group.host_id);
    row["name"] = vendor::json::Value(group.name);
    row["created_at"] = vendor::json::Value(static_cast<std::int64_t>(std::time(nullptr)));
    db_.insert("groups", row);
    join(hostId, group.group_id);
    return group;
}

std::vector<ChannelInfo> ChannelRepository::searchGroups(
    const std::string& keyword,
    int limit) {
    auto rows = db_.query("groups", [&keyword](const Row& row) {
        return keyword.empty() || asString(row, "name").find(keyword) != std::string::npos;
    });

    std::vector<ChannelInfo> groups;
    for (const auto& row : rows) {
        groups.push_back(fromRow(row));
        if (limit > 0 && groups.size() >= static_cast<size_t>(limit)) {
            break;
        }
    }
    return groups;
}

std::vector<ChannelInfo> ChannelRepository::groupsForUser(int userId) {
    std::vector<ChannelInfo> groups;
    auto memberships = db_.query("group_members", [userId](const Row& row) {
        return asInt(row, "user_id") == userId;
    });
    for (const auto& membership : memberships) {
        const int groupId = asInt(membership, "group_id");
        auto rows = db_.query("groups", [groupId](const Row& row) {
            return asInt(row, "group_id") == groupId;
        });
        if (!rows.empty()) {
            groups.push_back(fromRow(rows[0]));
        }
    }
    return groups;
}

std::vector<int> ChannelRepository::members(int groupId) {
    auto rows = db_.query("group_members", [groupId](const Row& row) {
        return asInt(row, "group_id") == groupId;
    });
    std::vector<int> result;
    for (const auto& row : rows) {
        result.push_back(asInt(row, "user_id"));
    }
    return result;
}

bool ChannelRepository::join(int userId, int groupId) {
    if (!exists(groupId)) {
        return false;
    }
    auto duplicate = db_.query("group_members", [userId, groupId](const Row& row) {
        return asInt(row, "user_id") == userId && asInt(row, "group_id") == groupId;
    });
    if (!duplicate.empty()) {
        return true;
    }

    Row row;
    row["user_id"] = vendor::json::Value(userId);
    row["group_id"] = vendor::json::Value(groupId);
    row["joined_at"] = vendor::json::Value(static_cast<std::int64_t>(std::time(nullptr)));
    db_.insert("group_members", row);
    return true;
}

bool ChannelRepository::leave(int userId, int groupId) {
    bool removed = false;
    db_.remove("group_members", [userId, groupId, &removed](const Row& row) {
        const bool match = asInt(row, "user_id") == userId && asInt(row, "group_id") == groupId;
        if (match) {
            removed = true;
        }
        return match;
    });
    return removed;
}

bool ChannelRepository::exists(int groupId) {
    auto rows = db_.query("groups", [groupId](const Row& row) {
        return asInt(row, "group_id") == groupId;
    });
    return !rows.empty();
}

ChannelInfo ChannelRepository::fromRow(const Row& row) const {
    ChannelInfo group;
    group.group_id = asInt(row, "group_id");
    group.host_id = asInt(row, "host_id");
    group.name = asString(row, "name");
    return group;
}

int ChannelRepository::nextGroupId() {
    int maxId = 2000;
    for (const auto& row : db_.query("groups")) {
        maxId = std::max(maxId, asInt(row, "group_id"));
    }
    return maxId + 1;
}

} // namespace lanchat::server::db
