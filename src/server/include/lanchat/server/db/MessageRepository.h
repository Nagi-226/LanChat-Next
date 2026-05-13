#pragma once

#include "Database.h"

#include <cstdint>
#include <string>
#include <vector>

namespace lanchat::server::db {

struct StoredMessage {
    std::int64_t msg_id = 0;
    int from_id = 0;
    int to_id = 0;
    int group_id = 0;
    std::string content;
    std::string content_type = "text";
    std::int64_t timestamp = 0;
    bool delivered = false;
    bool read = false;
};

class MessageRepository {
public:
    explicit MessageRepository(Database& db);

    StoredMessage savePrivate(
        int fromId,
        int toId,
        const std::string& content,
        const std::string& contentType,
        bool delivered);
    StoredMessage saveGroup(
        int fromId,
        int groupId,
        const std::string& content,
        const std::string& contentType);

    std::vector<StoredMessage> offlineForUser(int userId, int limit = 100);
    std::vector<StoredMessage> privateHistory(int userA, int userB, int limit = 100);
    std::vector<StoredMessage> groupHistory(int groupId, int limit = 100);
    std::vector<StoredMessage> search(const std::string& keyword, int limit = 50);

    void markDelivered(std::int64_t msgId);
    void markRead(std::int64_t msgId);
    Row toRow(const StoredMessage& message) const;

private:
    StoredMessage fromRow(const Row& row) const;
    std::int64_t nextMessageId();

    Database& db_;
};

} // namespace lanchat::server::db
