#pragma once

#include "PresenceManager.h"
#include "ProtocolJson.h"
#include "SessionPool.h"
#include "db/ChannelRepository.h"
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
                  db::ChannelRepository& channels);

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
    void handleAIRequest(const std::shared_ptr<AsyncSession>& session,
                         const protocol_json::Object& request);
    void handleProfileUpdate(const std::shared_ptr<AsyncSession>& session,
                             const protocol_json::Object& request);

    void deliverOffline(const std::shared_ptr<AsyncSession>& session, int userId);
    void broadcastPresence(lanchat::protocol::MsgType type, int userId);
    int authenticatedUserId(const std::shared_ptr<AsyncSession>& session,
                            const protocol_json::Object& request,
                            const std::string& field) const;

    SessionPool& sessions_;
    PresenceManager& presence_;
    db::UserRepository& users_;
    db::MessageRepository& messages_;
    db::ChannelRepository& channels_;
};

} // namespace lanchat::server
