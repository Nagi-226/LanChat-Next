#pragma once

#include "Database.h"
#include "UserRepository.h"

#include <string>
#include <vector>

namespace lanchat::server::db {

struct FriendRequestRecord {
    int from_uid = 0;
    int to_uid = 0;
    std::string status;
    std::string request_msg;
};

class FriendRepository {
public:
    FriendRepository(Database& db, UserRepository& users);

    bool sendRequest(int fromUid, int toUid, const std::string& requestMsg);
    bool acceptRequest(int fromUid, int toUid);
    bool rejectRequest(int fromUid, int toUid);
    bool removeFriendship(int uidA, int uidB);
    bool areFriends(int uidA, int uidB);
    std::vector<protocol::UserInfo> getFriends(int uid);
    std::vector<FriendRequestRecord> pendingRequests(int uid);

private:
    bool setStatus(int fromUid, int toUid, const std::string& status);

    Database& db_;
    UserRepository& users_;
};

} // namespace lanchat::server::db
