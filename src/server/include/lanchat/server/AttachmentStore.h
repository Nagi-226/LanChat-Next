#pragma once
#include "ProtocolJson.h"
#include "db/Database.h"
#include "db/ChannelRepository.h"
#include <filesystem>

namespace lanchat::server {
// Bytes stay on disk. Each authenticated request moves at most 48 KiB.
class AttachmentStore {
public:
    AttachmentStore(db::Database& db, db::ChannelRepository& channels);
    protocol_json::Object process(int uid, const protocol_json::Object& request);
    void markPublished(const std::string& id, std::int64_t msgId);
private:
    bool canRead(int uid, const protocol_json::Object& row);
    db::Database& db_;
    db::ChannelRepository& channels_;
    std::filesystem::path root_;
    std::int64_t maxBytes_ = 100LL * 1024 * 1024;
    std::int64_t quotaBytes_ = 5LL * 1024 * 1024 * 1024;
};
}
