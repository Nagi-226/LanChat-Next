#pragma once

#include "PresenceManager.h"
#include "ProtocolJson.h"
#include "SessionPool.h"
#include "db/ChannelRepository.h"
#include "db/FriendRepository.h"
#include "db/MessageRepository.h"
#include "db/UserRepository.h"

#include <memory>
#include <string>

namespace lanchat::server {

class MessageRouter {
public:
    MessageRouter(SessionPool& sessions,
                  PresenceManager& presence,
                  db::UserRepository& users,
                  db::MessageRepository& messages,
                  db::ChannelRepository& channels,
                  db::FriendRepository& friends);

    void route(const std::shared_ptr<AsyncSession>& session, const std::string& json);

private:
    void handleRegister(const std::shared_ptr<AsyncSession>& session,
                        const protocol_json::Object& request);
    void handleLogin(const std::shared_ptr<AsyncSession>& session,
                     const protocol_json::Object& request);
    void handleLogout(const std::shared_ptr<AsyncSession>& session,
                      const protocol_json::Object& request);
    void handlePrivateMessage(const std::shared_ptr<AsyncSession>& session,
                              const protocol_json::Object& request);
    void handleGroupMessage(const std::shared_ptr<AsyncSession>& session,
                            const protocol_json::Object& request);
    void handleCreateGroup(const std::shared_ptr<AsyncSession>& session,
                           const protocol_json::Object& request);
    void handleSearchGroup(const std::shared_ptr<AsyncSession>& session,
                           const protocol_json::Object& request);
    void handleJoinGroup(const std::shared_ptr<AsyncSession>& session,
                         const protocol_json::Object& request);
    void handleLeaveGroup(const std::shared_ptr<AsyncSession>& session,
                          const protocol_json::Object& request);
    void handleHistory(const std::shared_ptr<AsyncSession>& session,
                       const protocol_json::Object& request);
    void handleFileTransfer(const std::shared_ptr<AsyncSession>& session,
                            const protocol_json::Object& request);
    void handleAIRequest(const std::shared_ptr<AsyncSession>& session,
                         const protocol_json::Object& request);
    void handleProfileUpdate(const std::shared_ptr<AsyncSession>& session,
                             const protocol_json::Object& request);
    void handleFriendRequest(const std::shared_ptr<AsyncSession>& session,
                             const protocol_json::Object& request);
    void handleFriendAccept(const std::shared_ptr<AsyncSession>& session,
                            const protocol_json::Object& request);
    void handleFriendRemove(const std::shared_ptr<AsyncSession>& session,
                            const protocol_json::Object& request);
    void handleFriendList(const std::shared_ptr<AsyncSession>& session,
                          const protocol_json::Object& request);
    void handleSystemBroadcast(const std::shared_ptr<AsyncSession>& session,
                               const protocol_json::Object& request);
    void handleMessageEdit(const std::shared_ptr<AsyncSession>& session,
                           const protocol_json::Object& request);
    void handleMessageDelete(const std::shared_ptr<AsyncSession>& session,
                             const protocol_json::Object& request);
    void handleMessageReaction(const std::shared_ptr<AsyncSession>& session,
                               const protocol_json::Object& request);
    void handleReadReceipt(const std::shared_ptr<AsyncSession>& session,
                           const protocol_json::Object& request);
    void handleProtocolHello(const std::shared_ptr<AsyncSession>& session,
                             const protocol_json::Object& request);

    void deliverOffline(const std::shared_ptr<AsyncSession>& session, int userId);
    void deliverToConversation(const std::string& payload,
                               int senderId,
                               int toId,
                               int groupId);
    void broadcastPresence(lanchat::protocol::MsgType type, int userId);
    int authenticatedUserId(const std::shared_ptr<AsyncSession>& session,
                            const protocol_json::Object& request,
                            const std::string& field) const;

    SessionPool& sessions_;
    PresenceManager& presence_;
    db::UserRepository& users_;
    db::MessageRepository& messages_;
    db::ChannelRepository& channels_;
    db::FriendRepository& friends_;
};

} // namespace lanchat::server
