#ifndef LANCHAT_MESSAGE_TYPES_H
#define LANCHAT_MESSAGE_TYPES_H

// ============================================================
// LanChat Protocol  Message Type Definitions
// ============================================================
// This header is framework-agnostic (no Qt dependency).
// It can be included by:
//   - Legacy Qt 5 server/client (via MsgBuilder wrapper)
//   - Future C++17 asio server
//   - Any protocol test/validation tool
// ============================================================

#include <cstdint>
#include <string>
#include <vector>

namespace lanchat {
namespace protocol {

// ============================================================
// Message Type Enum (34 types)
// Types 0-19:  Legacy compatible (original graduation project)
// Types 20-33: New for LanChat-Next
// ============================================================
enum class MsgType : int {
    // --- Auth (0-4) ---
    RegisterUser       = 0,   // Client  Server: register new user
    RegisterUserReturn = 1,   // Server  Client: registration result with assigned ID
    Login              = 2,   // Client  Server: login request
    LoginSuccessReturn = 3,   // Server  Client: login success + friend list
    LoginFailedReturn  = 4,   // Server  Client: login failed

    // --- Private Chat (5-6) ---
    SendMsg            = 5,   // Client  Server: send private message
    ReceiveMsg         = 6,   // Server  Client: deliver private message

    // --- Presence (7-8) ---
    UserOnline         = 7,   // Server  Client: user came online
    UserOffline        = 8,   // Server  Client: user went offline

    // --- Group Management (9-15) ---
    CreateGroup        = 9,   // Client  Server: create group channel
    CreateGroupReturn  = 10,  // Server  Client: group created with ID
    SearchGroup        = 11,  // Client  Server: search groups by name
    SearchGroupReturn  = 12,  // Server  Client: search results
    JoinGroup          = 13,  // Client  Server: request to join group
    JoinGroupReturn    = 14,  // Server  Client: join result + member list
    LeaveGroup         = 15,  // Client  Server: leave a group

    // --- Group Chat (16-19) ---
    SendGroupMsg       = 16,  // Client  Server: send group message
    ReceiveGroupMsg    = 17,  // Server  Client: deliver group message
    UserJoinGroup      = 18,  // Server  Client: broadcast new member
    UserLeaveGroup     = 19,  // Server  Client: broadcast member left

    // ============================================================
    // New message types (20-33) for LanChat-Next v1.1.0+
    // ============================================================

    // --- Connection Health (20-21) ---
    Heartbeat          = 20,  // Client  Server: keepalive ping
    HeartbeatAck       = 21,  // Server  Client: keepalive pong

    // --- Offline & History (22-25) ---
    OfflineMessages    = 22,  // Server  Client: push queued offline messages
    Logout             = 23,  // Client  Server: explicit logout
    RequestHistory     = 24,  // Client  Server: request message history
    HistoryResponse    = 25,  // Server  Client: paginated history response

    // --- File Transfer (26-28) ---
    SendFile           = 26,  // Client  Server: file transfer request
    ReceiveFile        = 27,  // Server  Client: incoming file notification
    FileTransferDone   = 28,  // Bidirectional: transfer complete ack

    // --- AI Features (29-31) ---
    AIRequest          = 29,  // Client  Server: AI request (summarize/translate/search)
    AIResponse         = 30,  // Server  Client: AI result (start/done/error)
    AIStreamChunk      = 31,  // Server  Client: streaming text chunk

    // --- Profile & Admin (32-33) ---
    UserProfileUpdate  = 32,  // Client  Server: update nickname/avatar
    SystemBroadcast    = 33   // Server  Client: admin broadcast message
};

// ============================================================
// User Info (protocol-level, no Qt types)
// ============================================================
struct UserInfo {
    int id = 0;
    std::string password;    // bcrypt hash in storage, plaintext only during auth
    std::string nickname;
    int avatar_id = 0;       // avatar index or file hash

    bool operator<(const UserInfo& other) const { return id < other.id; }
    bool operator==(const UserInfo& other) const { return id == other.id; }
};

// ============================================================
// JSON Field Name Constants
// ============================================================
namespace field {
    // Common
    constexpr const char* TYPE       = "type";
    constexpr const char* TIMESTAMP  = "timestamp";
    constexpr const char* MSG_ID     = "msg_id";

    // User
    constexpr const char* ID         = "id";
    constexpr const char* PASSWORD   = "password";
    constexpr const char* NICKNAME   = "nickname";
    constexpr const char* AVATAR_ID  = "headId";
    constexpr const char* HOST_ID    = "hostId";
    constexpr const char* HOST_AVATAR = "hostHeadId";
    constexpr const char* HOST_NICK  = "hostNickname";

    // Message
    constexpr const char* FROM_ID    = "fromId";
    constexpr const char* TO_ID      = "toId";
    constexpr const char* CONTENT    = "msg";
    constexpr const char* CONTENT_TYPE = "content_type";
    constexpr const char* REPLY_TO   = "reply_to";

    // Group
    constexpr const char* GROUP_ID   = "groupId";
    constexpr const char* GROUP_NAME = "name";
    constexpr const char* GROUPS     = "groups";
    constexpr const char* MEMBERS    = "users";
    constexpr const char* FRIENDS    = "friends";

    // AI
    constexpr const char* REQUEST_ID = "request_id";
    constexpr const char* AI_TYPE    = "ai_type";
    constexpr const char* CHUNK_IDX  = "chunk_index";
    constexpr const char* IS_FINAL   = "is_final";
    constexpr const char* STATUS     = "status";

    // File
    constexpr const char* FILE_NAME  = "file_name";
    constexpr const char* FILE_SIZE  = "file_size";
    constexpr const char* FILE_HASH  = "file_hash";
} // namespace field

// ============================================================
// Content Type Constants
// ============================================================
namespace content_type {
    constexpr const char* TEXT  = "text";
    constexpr const char* IMAGE = "image";
    constexpr const char* FILE  = "file";
    constexpr const char* SYSTEM = "system";
} // namespace content_type

// ============================================================
// AI Type Constants
// ============================================================
namespace ai_type {
    constexpr const char* SUMMARIZE  = "summarize";
    constexpr const char* TRANSLATE  = "translate";
    constexpr const char* SEARCH     = "search";
    constexpr const char* CHAT       = "chat";
} // namespace ai_type

// ============================================================
// Helper: convert MsgType enum to int (for JSON serialization)
// ============================================================
inline int toInt(MsgType t) { return static_cast<int>(t); }
inline MsgType fromInt(int v) {
    if (v >= 0 && v <= 33) return static_cast<MsgType>(v);
    return static_cast<MsgType>(-1); // invalid
}
inline bool isLegacyType(MsgType t) { return static_cast<int>(t) <= 19; }
inline bool isValidType(MsgType t) { return static_cast<int>(t) >= 0 && static_cast<int>(t) <= 33; }

} // namespace protocol
} // namespace lanchat

#endif // LANCHAT_MESSAGE_TYPES_H
