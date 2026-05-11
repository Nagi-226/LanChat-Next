#ifndef MESSAGEDAO_H
#define MESSAGEDAO_H

#include <QSqlDatabase>
#include <QString>
#include <vector>

struct MessageRecord {
    int    id;
    int    fromId;
    int    toId;          // user ID for private, channel ID for group
    QString content;
    QString contentType;   // "text" | "image" | "file"
    int    channelId;      // 0 = private message, >0 = group message
    qint64 createdAt;      // Unix timestamp (seconds)
};

struct ChannelRecord {
    int id;
    QString name;
    int ownerId;
    qint64 createdAt;
};

class MessageDao
{
public:
    static MessageDao* getInstance();
    static void releaseInstance();

    // Insert a message and return its DB id
    int insertPrivateMsg(int fromId, int toId, const QString& content);
    int insertGroupMsg(int fromId, int channelId, const QString& content);

    // Query history (most recent first, paginated)
    std::vector<MessageRecord> getPrivateHistory(int user1, int user2, int limit = 50, int offset = 0);
    std::vector<MessageRecord> getChannelHistory(int channelId, int limit = 50, int offset = 0);

    // Get unread messages for a user (delivered after login)
    std::vector<MessageRecord> getOfflinePrivateMsgs(int userId);
    void markPrivateMessagesRead(int userId);

    int nextChannelId(int baseId = 200000);
    int createChannel(int channelId, const QString& name, int ownerId);
    std::vector<ChannelRecord> listChannels();
    void addChannelMember(int channelId, int userId, const QString& role = "member");
    void removeChannelMember(int channelId, int userId);
    std::vector<int> getChannelMembers(int channelId);

private:
    MessageDao();
    ~MessageDao();
    void createTables();

    static MessageDao* instance;
    QSqlDatabase db;
};

#endif // MESSAGEDAO_H
