#pragma once

#include "Database.h"
#include "message_types.h"

#include <string>
#include <vector>

namespace lanchat::server::db {

struct ChannelInfo {
    int group_id = 0;
    int host_id = 0;
    std::string name;
};

class ChannelRepository {
public:
    explicit ChannelRepository(Database& db);

    ChannelInfo createGroup(int hostId, const std::string& name);
    std::vector<ChannelInfo> searchGroups(const std::string& keyword, int limit = 20);
    std::vector<ChannelInfo> groupsForUser(int userId);
    std::vector<int> members(int groupId);
    bool join(int userId, int groupId);
    bool leave(int userId, int groupId);
    bool exists(int groupId);

private:
    ChannelInfo fromRow(const Row& row) const;
    int nextGroupId();

    Database& db_;
};

} // namespace lanchat::server::db
