#include "lanchat/server/MessageRouter.h"

#include "lanchat/server/ServerLogger.h"

#include <algorithm>
#include <sstream>

namespace lanchat::server {

using lanchat::protocol::MsgType;
using protocol_json::Array;
using protocol_json::Object;
using protocol_json::Value;

namespace {

Object userToJson(const lanchat::protocol::UserInfo& user, const std::string& status) {
    Object obj;
    obj["id"] = Value(user.id);
    obj["nickname"] = Value(user.nickname);
    obj["headId"] = Value(user.avatar_id);
    obj["status"] = Value(status);
    return obj;
}

Object groupToJson(const db::ChannelInfo& group) {
    Object obj;
    obj["groupId"] = Value(group.group_id);
    obj["name"] = Value(group.name);
    obj["hostId"] = Value(group.host_id);
    return obj;
}

Object friendToJson(const lanchat::protocol::UserInfo& user, const std::string& status) {
    return userToJson(user, status);
}

Object messageToJson(MsgType type, const db::StoredMessage& message) {
    Object obj;
    obj["type"] = Value(lanchat::protocol::toInt(type));
    obj["msg_id"] = Value(message.msg_id);
    obj["fromId"] = Value(message.from_id);
    if (message.to_id > 0) {
        obj["toId"] = Value(message.to_id);
    }
    if (message.group_id > 0) {
        obj["groupId"] = Value(message.group_id);
    }
    obj["msg"] = Value(message.content);
    obj["content_type"] = Value(message.content_type);
    obj["timestamp"] = Value(message.timestamp);
    obj["read"] = Value(message.read);
    return obj;
}

Array messagesToArray(const std::vector<db::StoredMessage>& messages) {
    Array arr;
    for (const auto& message : messages) {
        arr.push_back(Value(messageToJson(
            message.group_id > 0 ? MsgType::ReceiveGroupMsg : MsgType::ReceiveMsg,
            message)));
    }
    return arr;
}

std::string summarizeMessages(const std::vector<db::StoredMessage>& messages) {
    if (messages.empty()) {
        return "No recent messages are available for this conversation.";
    }
    std::stringstream ss;
    ss << "Summary based on " << messages.size() << " recent messages.\n";
    ss << "- Time span: " << messages.front().timestamp << " to " << messages.back().timestamp << ".\n";
    ss << "- Latest topic: " << messages.back().content.substr(0, 180) << "\n";
    if (messages.size() > 1) {
        ss << "- Earlier context: " << messages.front().content.substr(0, 160) << "\n";
    }
    ss << "- Suggested follow-up: review unresolved asks and confirm owners in chat.";
    return ss.str();
}

void deliverSummaryChunks(
    const std::shared_ptr<AsyncSession>& session,
    const std::string& requestId,
    const std::string& summary) {
    Object start;
    start["type"] = Value(lanchat::protocol::toInt(MsgType::AIResponse));
    start["request_id"] = Value(requestId);
    start["status"] = Value("start");
    session->deliver(protocol_json::serialize(start));

    constexpr size_t chunkSize = 80;
    int chunkIndex = 0;
    for (size_t offset = 0; offset < summary.size(); offset += chunkSize) {
        Object chunk;
        chunk["type"] = Value(lanchat::protocol::toInt(MsgType::AIStreamChunk));
        chunk["request_id"] = Value(requestId);
        chunk["chunk_index"] = Value(chunkIndex++);
        chunk["is_final"] = Value(offset + chunkSize >= summary.size());
        chunk["msg"] = Value(summary.substr(offset, chunkSize));
        session->deliver(protocol_json::serialize(chunk));
    }
}

} // namespace

MessageRouter::MessageRouter(SessionPool& sessions,
                             PresenceManager& presence,
                             db::UserRepository& users,
                             db::MessageRepository& messages,
                             db::ChannelRepository& channels,
                             db::FriendRepository& friends)
    : sessions_(sessions),
      presence_(presence),
      users_(users),
      messages_(messages),
      channels_(channels),
      friends_(friends) {}

void MessageRouter::route(const std::shared_ptr<AsyncSession>& session, const std::string& json) {
    bool ok = false;
    const auto request = protocol_json::parseObject(json, ok);
    if (!ok) {
        session->deliver(protocol_json::error(MsgType::SystemBroadcast, 400, "invalid json"));
        return;
    }

    switch (protocol_json::typeField(request)) {
        case MsgType::RegisterUser:       handleRegister(session, request); break;
        case MsgType::Login:              handleLogin(session, request); break;
        case MsgType::Logout:             handleLogout(session, request); break;
        case MsgType::SendMsg:            handlePrivateMessage(session, request); break;
        case MsgType::SendGroupMsg:       handleGroupMessage(session, request); break;
        case MsgType::CreateGroup:        handleCreateGroup(session, request); break;
        case MsgType::SearchGroup:        handleSearchGroup(session, request); break;
        case MsgType::JoinGroup:          handleJoinGroup(session, request); break;
        case MsgType::LeaveGroup:         handleLeaveGroup(session, request); break;
        case MsgType::RequestHistory:     handleHistory(session, request); break;
        case MsgType::AIRequest:          handleAIRequest(session, request); break;
        case MsgType::UserProfileUpdate:  handleProfileUpdate(session, request); break;
        case MsgType::FriendRequest:      handleFriendRequest(session, request); break;
        case MsgType::FriendAccept:       handleFriendAccept(session, request); break;
        case MsgType::FriendRemove:       handleFriendRemove(session, request); break;
        case MsgType::FriendList:         handleFriendList(session, request); break;
        default:
            session->deliver(protocol_json::ok(
                MsgType::SystemBroadcast,
                "server-next v1.2.0 routed frame"));
            break;
    }
}

void MessageRouter::handleRegister(
    const std::shared_ptr<AsyncSession>& session,
    const Object& request) {
    const std::string nickname = protocol_json::stringField(request, "nickname",
        protocol_json::stringField(request, "name"));
    const std::string password = protocol_json::stringField(request, "password");
    const int avatarId = protocol_json::intField(request, "headId", 0);

    if (nickname.empty() || password.size() < 1) {
        session->deliver(protocol_json::error(
            MsgType::RegisterUserReturn, 400, "nickname and password required"));
        return;
    }

    const int id = users_.create(password, nickname, avatarId);
    if (id <= 0) {
        session->deliver(protocol_json::error(
            MsgType::RegisterUserReturn, 409, "nickname taken or invalid"));
        return;
    }

    Object response;
    response["type"] = Value(lanchat::protocol::toInt(MsgType::RegisterUserReturn));
    response["status"] = Value("ok");
    response["id"] = Value(id);
    response["msg"] = Value("registered");
    session->deliver(protocol_json::serialize(response));
}

void MessageRouter::handleLogin(
    const std::shared_ptr<AsyncSession>& session,
    const Object& request) {
    const int uid = protocol_json::intField(request, "id");
    const std::string password = protocol_json::stringField(request, "password");
    if (uid <= 0 || !users_.verifyPassword(uid, password)) {
        session->deliver(protocol_json::error(
            MsgType::LoginFailedReturn, 401, "invalid credentials"));
        return;
    }

    auto user = users_.findById(uid);
    if (!user) {
        session->deliver(protocol_json::error(
            MsgType::LoginFailedReturn, 404, "user not found"));
        return;
    }

    sessions_.bindUser(session->id(), uid);
    presence_.setOnline(uid);
    users_.setOnlineStatus(uid, true);

    Array users;
    for (const auto& knownUser : users_.getAllUsers()) {
        users.push_back(Value(userToJson(knownUser, presence_.status(knownUser.id))));
    }

    Array friends;
    for (const auto& friendUser : friends_.getFriends(uid)) {
        friends.push_back(Value(friendToJson(friendUser, presence_.status(friendUser.id))));
    }

    Array groups;
    for (const auto& group : channels_.groupsForUser(uid)) {
        groups.push_back(Value(groupToJson(group)));
    }

    Object response;
    response["type"] = Value(lanchat::protocol::toInt(MsgType::LoginSuccessReturn));
    response["id"] = Value(uid);
    response["nickname"] = Value(user->nickname);
    response["headId"] = Value(user->avatar_id);
    response["friends"] = Value(friends);
    response["users"] = Value(users);
    response["groups"] = Value(groups);
    session->deliver(protocol_json::serialize(response));
    deliverOffline(session, uid);
    broadcastPresence(MsgType::UserOnline, uid);
}

void MessageRouter::handleLogout(
    const std::shared_ptr<AsyncSession>& session,
    const Object& request) {
    const int uid = authenticatedUserId(session, request, "id");
    if (uid > 0) {
        presence_.setOffline(uid);
        users_.setOnlineStatus(uid, false);
        sessions_.unbindUser(uid);
        broadcastPresence(MsgType::UserOffline, uid);
    }
    session->deliver(protocol_json::ok(MsgType::SystemBroadcast, "logged out"));
}

void MessageRouter::handlePrivateMessage(
    const std::shared_ptr<AsyncSession>& session,
    const Object& request) {
    const int fromId = authenticatedUserId(session, request, "fromId");
    const int toId = protocol_json::intField(request, "toId");
    const std::string content = protocol_json::stringField(request, "msg");
    const std::string contentType = protocol_json::stringField(request, "content_type", "text");

    if (fromId <= 0 || toId <= 0 || content.empty()) {
        session->deliver(protocol_json::error(
            MsgType::SystemBroadcast, 400, "fromId, toId and msg required"));
        return;
    }

    auto target = sessions_.findByUserId(toId);
    const bool delivered = target != nullptr;
    const auto message = messages_.savePrivate(fromId, toId, content, contentType, delivered);
    auto payload = protocol_json::serialize(messageToJson(MsgType::ReceiveMsg, message));
    if (target) {
        target->deliver(payload);
    }
    session->deliver(payload);
}

void MessageRouter::handleGroupMessage(
    const std::shared_ptr<AsyncSession>& session,
    const Object& request) {
    const int fromId = authenticatedUserId(session, request, "fromId");
    const int groupId = protocol_json::intField(request, "groupId");
    const std::string content = protocol_json::stringField(request, "msg");
    const std::string contentType = protocol_json::stringField(request, "content_type", "text");

    if (fromId <= 0 || groupId <= 0 || content.empty()) {
        session->deliver(protocol_json::error(
            MsgType::SystemBroadcast, 400, "fromId, groupId and msg required"));
        return;
    }

    auto message = messages_.saveGroup(fromId, groupId, content, contentType);
    auto payload = protocol_json::serialize(messageToJson(MsgType::ReceiveGroupMsg, message));
    for (const int memberId : channels_.members(groupId)) {
        auto target = sessions_.findByUserId(memberId);
        if (target) {
            target->deliver(payload);
        }
    }
}

void MessageRouter::handleCreateGroup(
    const std::shared_ptr<AsyncSession>& session,
    const Object& request) {
    const int hostId = authenticatedUserId(session, request, "hostId");
    const std::string name = protocol_json::stringField(request, "name");
    if (hostId <= 0 || name.empty()) {
        session->deliver(protocol_json::error(
            MsgType::CreateGroupReturn, 400, "hostId and name required"));
        return;
    }
    const auto group = channels_.createGroup(hostId, name);
    Object response = groupToJson(group);
    response["type"] = Value(lanchat::protocol::toInt(MsgType::CreateGroupReturn));
    response["status"] = Value("ok");
    session->deliver(protocol_json::serialize(response));
}

void MessageRouter::handleSearchGroup(
    const std::shared_ptr<AsyncSession>& session,
    const Object& request) {
    const std::string keyword = protocol_json::stringField(request, "name");
    const int limit = protocol_json::intField(request, "limit", 20);
    Array groups;
    for (const auto& group : channels_.searchGroups(keyword, limit)) {
        groups.push_back(Value(groupToJson(group)));
    }
    Object response;
    response["type"] = Value(lanchat::protocol::toInt(MsgType::SearchGroupReturn));
    response["status"] = Value("ok");
    response["groups"] = Value(groups);
    session->deliver(protocol_json::serialize(response));
}

void MessageRouter::handleJoinGroup(
    const std::shared_ptr<AsyncSession>& session,
    const Object& request) {
    const int userId = authenticatedUserId(session, request, "id");
    const int groupId = protocol_json::intField(request, "groupId");
    if (!channels_.join(userId, groupId)) {
        session->deliver(protocol_json::error(
            MsgType::JoinGroupReturn, 404, "group not found"));
        return;
    }

    Array members;
    for (const int memberId : channels_.members(groupId)) {
        auto user = users_.findById(memberId);
        if (user) {
            members.push_back(Value(userToJson(*user, presence_.status(memberId))));
        }
    }

    Object response;
    response["type"] = Value(lanchat::protocol::toInt(MsgType::JoinGroupReturn));
    response["status"] = Value("ok");
    response["groupId"] = Value(groupId);
    response["users"] = Value(members);
    session->deliver(protocol_json::serialize(response));
}

void MessageRouter::handleLeaveGroup(
    const std::shared_ptr<AsyncSession>& session,
    const Object& request) {
    const int userId = authenticatedUserId(session, request, "id");
    const int groupId = protocol_json::intField(request, "groupId");
    channels_.leave(userId, groupId);
    session->deliver(protocol_json::ok(MsgType::SystemBroadcast, "left group"));
}

void MessageRouter::handleHistory(
    const std::shared_ptr<AsyncSession>& session,
    const Object& request) {
    const int userId = authenticatedUserId(session, request, "id");
    const int toId = protocol_json::intField(request, "toId");
    const int groupId = protocol_json::intField(request, "groupId");
    const int limit = (std::max)(1, protocol_json::intField(request, "limit", 100));
    const auto readMsgId = protocol_json::int64Field(request, "msg_id");
    if (readMsgId > 0) {
        messages_.markRead(readMsgId);
    }

    std::vector<db::StoredMessage> history;
    if (groupId > 0) {
        history = messages_.groupHistory(groupId, limit);
    } else if (userId > 0 && toId > 0) {
        history = messages_.privateHistory(userId, toId, limit);
    }

    Object response;
    response["type"] = Value(lanchat::protocol::toInt(MsgType::HistoryResponse));
    response["messages"] = Value(messagesToArray(history));
    response["has_more"] = Value(false);
    session->deliver(protocol_json::serialize(response));
}

void MessageRouter::handleAIRequest(
    const std::shared_ptr<AsyncSession>& session,
    const Object& request) {
    const std::string requestId = protocol_json::stringField(request, "request_id", "search");
    const std::string aiType = protocol_json::stringField(request, "ai_type");
    const std::string query = protocol_json::stringField(request, "msg");
    if (aiType == "summarize") {
        const int userId = authenticatedUserId(session, request, "id");
        std::vector<db::StoredMessage> history;
        try {
            if (query.rfind("group:", 0) == 0) {
                history = messages_.groupHistory(std::stoi(query.substr(6)), 50);
            } else if (query.rfind("direct:", 0) == 0 && userId > 0) {
                history = messages_.privateHistory(userId, std::stoi(query.substr(7)), 50);
            }
        } catch (...) {
            session->deliver(protocol_json::error(MsgType::AIResponse, 400, "invalid summarize target"));
            return;
        }
        deliverSummaryChunks(session, requestId, summarizeMessages(history));
        return;
    }

    if (aiType != "search") {
        session->deliver(protocol_json::error(
            MsgType::AIResponse, 501, "unsupported AI request type"));
        return;
    }

    Object searchPayload;
    searchPayload["query"] = Value(query);
    searchPayload["messages"] = Value(messagesToArray(
        messages_.search(query, protocol_json::intField(request, "limit", 50))));

    Object response;
    response["type"] = Value(lanchat::protocol::toInt(MsgType::AIResponse));
    response["request_id"] = Value(requestId);
    response["status"] = Value("done");
    response["msg"] = Value(protocol_json::serialize(searchPayload));
    session->deliver(protocol_json::serialize(response));
}

void MessageRouter::handleFriendRequest(
    const std::shared_ptr<AsyncSession>& session,
    const Object& request) {
    const int fromId = authenticatedUserId(session, request, "fromId");
    const int toId = protocol_json::intField(request, "toId");
    const std::string msg = protocol_json::stringField(request, "msg");

    Object ack;
    ack["type"] = Value(lanchat::protocol::toInt(MsgType::FriendRequestAck));
    ack["requestId"] = Value(std::to_string(fromId) + ":" + std::to_string(toId));
    if (fromId <= 0 || toId <= 0 || !friends_.sendRequest(fromId, toId, msg)) {
        ack["status"] = Value("error");
        ack["msg"] = Value("friend request failed");
        ack["code"] = Value(400);
        session->deliver(protocol_json::serialize(ack));
        return;
    }

    ack["status"] = Value("ok");
    ack["msg"] = Value("request sent");
    session->deliver(protocol_json::serialize(ack));

    if (auto target = sessions_.findByUserId(toId)) {
        Object notify;
        notify["type"] = Value(lanchat::protocol::toInt(MsgType::FriendRequest));
        notify["fromId"] = Value(fromId);
        notify["toId"] = Value(toId);
        notify["msg"] = Value(msg);
        target->deliver(protocol_json::serialize(notify));
    }
}

void MessageRouter::handleFriendAccept(
    const std::shared_ptr<AsyncSession>& session,
    const Object& request) {
    const int toId = authenticatedUserId(session, request, "toId");
    const int fromId = protocol_json::intField(request, "fromId");
    const bool ok = fromId > 0 && toId > 0 && friends_.acceptRequest(fromId, toId);
    auto fromUser = users_.findById(fromId);
    auto toUser = users_.findById(toId);

    auto deliverAccepted = [&](const std::shared_ptr<AsyncSession>& target, int friendId, const protocol::UserInfo* friendUser) {
        if (!target) return;
        Object response;
        response["type"] = Value(lanchat::protocol::toInt(MsgType::FriendAcceptReturn));
        response["status"] = Value(ok ? "ok" : "error");
        response["friendId"] = Value(friendId);
        if (friendUser) {
            response["nickname"] = Value(friendUser->nickname);
            response["headId"] = Value(friendUser->avatar_id);
        }
        if (!ok) {
            response["msg"] = Value("accept failed");
            response["code"] = Value(400);
        }
        target->deliver(protocol_json::serialize(response));
    };

    deliverAccepted(session, fromId, fromUser ? &*fromUser : nullptr);
    deliverAccepted(sessions_.findByUserId(fromId), toId, toUser ? &*toUser : nullptr);
}

void MessageRouter::handleFriendRemove(
    const std::shared_ptr<AsyncSession>& session,
    const Object& request) {
    const int uid = session && session->authenticated() ? session->userId() : 0;
    const int fromId = protocol_json::intField(request, "fromId");
    const int toId = protocol_json::intField(request, "toId");
    const int friendId = uid == fromId ? toId : fromId;
    bool rejectedPending = false;
    bool ok = false;
    if (uid > 0 && fromId > 0 && toId > 0) {
        rejectedPending = uid == toId && friends_.rejectRequest(fromId, toId);
        ok = rejectedPending || friends_.removeFriendship(fromId, toId);
    }

    auto deliverRemoved = [&](const std::shared_ptr<AsyncSession>& target, int removedId, bool notifyReject) {
        if (!target) return;
        Object response;
        response["type"] = Value(lanchat::protocol::toInt(MsgType::FriendRemoveReturn));
        response["status"] = Value(ok ? "ok" : "error");
        response["friendId"] = Value(removedId);
        if (notifyReject && rejectedPending) {
            response["msg"] = Value("request rejected");
        }
        if (!ok) {
            response["msg"] = Value("remove failed");
            response["code"] = Value(400);
        }
        target->deliver(protocol_json::serialize(response));
    };

    deliverRemoved(session, friendId, true);
    deliverRemoved(sessions_.findByUserId(friendId), uid, rejectedPending);
}

void MessageRouter::handleFriendList(
    const std::shared_ptr<AsyncSession>& session,
    const Object& request) {
    const int uid = authenticatedUserId(session, request, "id");
    Array friends;
    for (const auto& user : friends_.getFriends(uid)) {
        friends.push_back(Value(friendToJson(user, presence_.status(user.id))));
    }
    Object response;
    response["type"] = Value(lanchat::protocol::toInt(MsgType::FriendListReturn));
    response["friends"] = Value(friends);
    session->deliver(protocol_json::serialize(response));
}

void MessageRouter::handleProfileUpdate(
    const std::shared_ptr<AsyncSession>& session,
    const Object& request) {
    const int userId = authenticatedUserId(session, request, "id");
    const std::string nickname = protocol_json::stringField(request, "nickname");
    const int avatarId = protocol_json::intField(request, "headId", 0);
    if (userId <= 0 || nickname.empty()) {
        session->deliver(protocol_json::error(
            MsgType::SystemBroadcast, 400, "id and nickname required"));
        return;
    }
    if (!users_.updateProfile(userId, nickname, avatarId)) {
        session->deliver(protocol_json::error(
            MsgType::SystemBroadcast, 404, "user not found"));
        return;
    }
    session->deliver(protocol_json::ok(MsgType::SystemBroadcast, "profile updated"));
}

void MessageRouter::deliverOffline(const std::shared_ptr<AsyncSession>& session, int userId) {
    const auto offline = messages_.offlineForUser(userId);
    if (offline.empty()) {
        return;
    }
    Object response;
    response["type"] = Value(lanchat::protocol::toInt(MsgType::OfflineMessages));
    response["messages"] = Value(messagesToArray(offline));
    session->deliver(protocol_json::serialize(response));
}

void MessageRouter::broadcastPresence(MsgType type, int userId) {
    auto user = users_.findById(userId);
    if (!user) {
        return;
    }
    Object payload;
    payload["type"] = Value(lanchat::protocol::toInt(MsgType::FriendOnline));
    payload["friendId"] = Value(userId);
    payload["status"] = Value(type == MsgType::UserOnline ? "ok" : "failed");
    payload["nickname"] = Value(user->nickname);
    payload["headId"] = Value(user->avatar_id);
    auto json = protocol_json::serialize(payload);
    for (const auto& friendUser : friends_.getFriends(userId)) {
        auto target = sessions_.findByUserId(friendUser.id);
        if (target) target->deliver(json);
    }
}

int MessageRouter::authenticatedUserId(
    const std::shared_ptr<AsyncSession>& session,
    const Object& request,
    const std::string& field) const {
    (void)request;
    (void)field;
    if (session && session->authenticated()) {
        return session->userId();
    }
    return 0;
}

} // namespace lanchat::server
