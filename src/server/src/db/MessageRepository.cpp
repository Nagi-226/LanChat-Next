#include "lanchat/server/db/MessageRepository.h"

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

std::int64_t asInt64(const Row& row, const std::string& key, std::int64_t fallback = 0) {
    auto it = row.find(key);
    if (it == row.end() || !it->second.is_number()) {
        return fallback;
    }
    return it->second.as_int();
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

bool asBool(const Row& row, const std::string& key, bool fallback = false) {
    auto it = row.find(key);
    if (it == row.end()) {
        return fallback;
    }
    if (it->second.is_bool()) {
        return it->second.b;
    }
    if (it->second.is_number()) {
        return it->second.as_int() != 0;
    }
    return fallback;
}

void trimToLimit(std::vector<StoredMessage>& messages, int limit) {
    std::sort(messages.begin(), messages.end(), [](const auto& a, const auto& b) {
        return a.timestamp < b.timestamp;
    });
    if (limit > 0 && messages.size() > static_cast<size_t>(limit)) {
        messages.erase(messages.begin(), messages.end() - limit);
    }
}

} // namespace

MessageRepository::MessageRepository(Database& db) : db_(db) {
    db_.ensureTable("messages");
}

StoredMessage MessageRepository::savePrivate(
    int fromId,
    int toId,
    const std::string& content,
    const std::string& contentType,
    bool delivered) {
    StoredMessage message;
    message.msg_id = nextMessageId();
    message.from_id = fromId;
    message.to_id = toId;
    message.content = content;
    message.content_type = contentType.empty() ? "text" : contentType;
    message.timestamp = static_cast<std::int64_t>(std::time(nullptr));
    message.delivered = delivered;

    db_.insert("messages", toRow(message));
    return message;
}

StoredMessage MessageRepository::saveGroup(
    int fromId,
    int groupId,
    const std::string& content,
    const std::string& contentType) {
    StoredMessage message;
    message.msg_id = nextMessageId();
    message.from_id = fromId;
    message.group_id = groupId;
    message.content = content;
    message.content_type = contentType.empty() ? "text" : contentType;
    message.timestamp = static_cast<std::int64_t>(std::time(nullptr));
    message.delivered = true;

    db_.insert("messages", toRow(message));
    return message;
}

std::vector<StoredMessage> MessageRepository::offlineForUser(int userId, int limit) {
    auto rows = db_.query("messages", [userId](const Row& row) {
        return asInt(row, "to_id") == userId
            && asInt(row, "group_id") == 0
            && !asBool(row, "delivered");
    });

    std::vector<StoredMessage> messages;
    for (const auto& row : rows) {
        messages.push_back(fromRow(row));
    }
    trimToLimit(messages, limit);
    for (const auto& message : messages) {
        markDelivered(message.msg_id);
    }
    return messages;
}

std::vector<StoredMessage> MessageRepository::privateHistory(
    int userA,
    int userB,
    int limit) {
    auto rows = db_.query("messages", [userA, userB](const Row& row) {
        const int from = asInt(row, "from_id");
        const int to = asInt(row, "to_id");
        return asInt(row, "group_id") == 0
            && ((from == userA && to == userB) || (from == userB && to == userA));
    });

    std::vector<StoredMessage> messages;
    for (const auto& row : rows) {
        messages.push_back(fromRow(row));
    }
    trimToLimit(messages, limit);
    return messages;
}

std::vector<StoredMessage> MessageRepository::groupHistory(int groupId, int limit) {
    auto rows = db_.query("messages", [groupId](const Row& row) {
        return asInt(row, "group_id") == groupId;
    });

    std::vector<StoredMessage> messages;
    for (const auto& row : rows) {
        messages.push_back(fromRow(row));
    }
    trimToLimit(messages, limit);
    return messages;
}

std::vector<StoredMessage> MessageRepository::search(
    const std::string& keyword,
    int limit) {
    if (keyword.empty()) {
        return {};
    }
    auto rows = db_.query("messages", [&keyword](const Row& row) {
        return asString(row, "content").find(keyword) != std::string::npos;
    });

    std::vector<StoredMessage> messages;
    for (const auto& row : rows) {
        messages.push_back(fromRow(row));
    }
    trimToLimit(messages, limit);
    return messages;
}

void MessageRepository::markDelivered(std::int64_t msgId) {
    db_.update("messages", [msgId](const Row& row) {
        return asInt64(row, "msg_id") == msgId;
    }, [](Row& row) {
        row["delivered"] = vendor::json::Value(true);
    });
}

void MessageRepository::markRead(std::int64_t msgId) {
    db_.update("messages", [msgId](const Row& row) {
        return asInt64(row, "msg_id") == msgId;
    }, [](Row& row) {
        row["read"] = vendor::json::Value(true);
    });
}

Row MessageRepository::toRow(const StoredMessage& message) const {
    Row row;
    row["msg_id"] = vendor::json::Value(message.msg_id);
    row["from_id"] = vendor::json::Value(message.from_id);
    row["to_id"] = vendor::json::Value(message.to_id);
    row["group_id"] = vendor::json::Value(message.group_id);
    row["content"] = vendor::json::Value(message.content);
    row["content_type"] = vendor::json::Value(message.content_type);
    row["timestamp"] = vendor::json::Value(message.timestamp);
    row["delivered"] = vendor::json::Value(message.delivered);
    row["read"] = vendor::json::Value(message.read);
    return row;
}

StoredMessage MessageRepository::fromRow(const Row& row) const {
    StoredMessage message;
    message.msg_id = asInt64(row, "msg_id");
    message.from_id = asInt(row, "from_id");
    message.to_id = asInt(row, "to_id");
    message.group_id = asInt(row, "group_id");
    message.content = asString(row, "content");
    message.content_type = asString(row, "content_type", "text");
    message.timestamp = asInt64(row, "timestamp");
    message.delivered = asBool(row, "delivered");
    message.read = asBool(row, "read");
    return message;
}

std::int64_t MessageRepository::nextMessageId() {
    std::int64_t maxId = 0;
    for (const auto& row : db_.query("messages")) {
        maxId = std::max(maxId, asInt64(row, "msg_id"));
    }
    return maxId + 1;
}

} // namespace lanchat::server::db
