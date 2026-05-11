#include "MessageDao.h"
#include <QSqlQuery>
#include <QSqlRecord>
#include <QVariant>
#include <QDateTime>
#include <QSqlError>

MessageDao* MessageDao::instance = nullptr;

MessageDao* MessageDao::getInstance()
{
    if (instance == nullptr) {
        instance = new MessageDao;
    }
    return instance;
}

void MessageDao::releaseInstance()
{
    delete instance;
    instance = nullptr;
}

int MessageDao::insertPrivateMsg(int fromId, int toId, const QString& content)
{
    QSqlQuery query;
    query.prepare(
        "INSERT INTO private_messages(from_id, to_id, content, content_type, created_at) "
        "VALUES(?, ?, ?, 'text', ?)"
    );
    query.bindValue(0, fromId);
    query.bindValue(1, toId);
    query.bindValue(2, content);
    query.bindValue(3, QDateTime::currentSecsSinceEpoch());
    query.exec();

    query.exec("SELECT LAST_INSERT_ROWID()");
    if (query.next()) return query.record().value(0).toInt();
    return -1;
}

int MessageDao::insertGroupMsg(int fromId, int channelId, const QString& content)
{
    QSqlQuery query;
    query.prepare(
        "INSERT INTO channel_messages(channel_id, from_id, content, content_type, created_at) "
        "VALUES(?, ?, ?, 'text', ?)"
    );
    query.bindValue(0, channelId);
    query.bindValue(1, fromId);
    query.bindValue(2, content);
    query.bindValue(3, QDateTime::currentSecsSinceEpoch());
    query.exec();

    query.exec("SELECT LAST_INSERT_ROWID()");
    if (query.next()) return query.record().value(0).toInt();
    return -1;
}

std::vector<MessageRecord> MessageDao::getPrivateHistory(int user1, int user2, int limit, int offset)
{
    std::vector<MessageRecord> results;
    QSqlQuery query;
    query.prepare(
        "SELECT id, from_id, to_id, content, content_type, 0, created_at "
        "FROM private_messages "
        "WHERE (from_id = ? AND to_id = ?) OR (from_id = ? AND to_id = ?) "
        "ORDER BY created_at DESC LIMIT ? OFFSET ?"
    );
    query.bindValue(0, user1);
    query.bindValue(1, user2);
    query.bindValue(2, user2);
    query.bindValue(3, user1);
    query.bindValue(4, limit);
    query.bindValue(5, offset);
    query.exec();

    while (query.next()) {
        MessageRecord m;
        m.id          = query.record().value(0).toInt();
        m.fromId      = query.record().value(1).toInt();
        m.toId        = query.record().value(2).toInt();
        m.content     = query.record().value(3).toString();
        m.contentType = query.record().value(4).toString();
        m.channelId   = query.record().value(5).toInt();
        m.createdAt   = query.record().value(6).toLongLong();
        results.push_back(m);
    }
    return results;
}

std::vector<MessageRecord> MessageDao::getChannelHistory(int channelId, int limit, int offset)
{
    std::vector<MessageRecord> results;
    QSqlQuery query;
    query.prepare(
        "SELECT id, from_id, 0, content, content_type, channel_id, created_at "
        "FROM channel_messages "
        "WHERE channel_id = ? "
        "ORDER BY created_at DESC LIMIT ? OFFSET ?"
    );
    query.bindValue(0, channelId);
    query.bindValue(1, limit);
    query.bindValue(2, offset);
    query.exec();

    while (query.next()) {
        MessageRecord m;
        m.id          = query.record().value(0).toInt();
        m.fromId      = query.record().value(1).toInt();
        m.toId        = query.record().value(2).toInt();
        m.content     = query.record().value(3).toString();
        m.contentType = query.record().value(4).toString();
        m.channelId   = query.record().value(5).toInt();
        m.createdAt   = query.record().value(6).toLongLong();
        results.push_back(m);
    }
    return results;
}

std::vector<MessageRecord> MessageDao::getOfflinePrivateMsgs(int userId)
{
    std::vector<MessageRecord> results;
    QSqlQuery query;
    query.prepare(
        "SELECT id, from_id, to_id, content, content_type, 0, created_at "
        "FROM private_messages WHERE to_id = ? AND is_read = 0 ORDER BY created_at ASC"
    );
    query.bindValue(0, userId);
    query.exec();

    while (query.next()) {
        MessageRecord m;
        m.id          = query.record().value(0).toInt();
        m.fromId      = query.record().value(1).toInt();
        m.toId        = query.record().value(2).toInt();
        m.content     = query.record().value(3).toString();
        m.contentType = query.record().value(4).toString();
        m.channelId   = query.record().value(5).toInt();
        m.createdAt   = query.record().value(6).toLongLong();
        results.push_back(m);
    }
    return results;
}

void MessageDao::markPrivateMessagesRead(int userId)
{
    QSqlQuery query;
    query.prepare("UPDATE private_messages SET is_read = 1 WHERE to_id = ?");
    query.bindValue(0, userId);
    query.exec();
}

int MessageDao::nextChannelId(int baseId)
{
    QSqlQuery query;
    query.exec("SELECT COALESCE(MAX(id), 0) FROM channels");
    if (query.next()) {
        const int maxId = query.record().value(0).toInt();
        return qMax(baseId, maxId + 1);
    }
    return baseId;
}

int MessageDao::createChannel(int channelId, const QString& name, int ownerId)
{
    QSqlQuery query;
    query.prepare("INSERT OR IGNORE INTO channels(id, name, owner_id, created_at) VALUES(?, ?, ?, ?)");
    query.bindValue(0, channelId);
    query.bindValue(1, name);
    query.bindValue(2, ownerId);
    query.bindValue(3, QDateTime::currentSecsSinceEpoch());
    query.exec();
    addChannelMember(channelId, ownerId, "owner");
    return channelId;
}

std::vector<ChannelRecord> MessageDao::listChannels()
{
    std::vector<ChannelRecord> channels;
    QSqlQuery query;
    query.exec("SELECT id, name, owner_id, created_at FROM channels ORDER BY id");
    while (query.next()) {
        ChannelRecord c;
        c.id = query.record().value(0).toInt();
        c.name = query.record().value(1).toString();
        c.ownerId = query.record().value(2).toInt();
        c.createdAt = query.record().value(3).toLongLong();
        channels.push_back(c);
    }
    return channels;
}

void MessageDao::addChannelMember(int channelId, int userId, const QString& role)
{
    QSqlQuery query;
    query.prepare(
        "INSERT OR REPLACE INTO channel_members(channel_id, user_id, role, joined_at) "
        "VALUES(?, ?, ?, COALESCE((SELECT joined_at FROM channel_members WHERE channel_id = ? AND user_id = ?), ?))"
    );
    query.bindValue(0, channelId);
    query.bindValue(1, userId);
    query.bindValue(2, role);
    query.bindValue(3, channelId);
    query.bindValue(4, userId);
    query.bindValue(5, QDateTime::currentSecsSinceEpoch());
    query.exec();
}

void MessageDao::removeChannelMember(int channelId, int userId)
{
    QSqlQuery query;
    query.prepare("DELETE FROM channel_members WHERE channel_id = ? AND user_id = ?");
    query.bindValue(0, channelId);
    query.bindValue(1, userId);
    query.exec();
}

std::vector<int> MessageDao::getChannelMembers(int channelId)
{
    std::vector<int> users;
    QSqlQuery query;
    query.prepare("SELECT user_id FROM channel_members WHERE channel_id = ? ORDER BY joined_at");
    query.bindValue(0, channelId);
    query.exec();
    while (query.next()) {
        users.push_back(query.record().value(0).toInt());
    }
    return users;
}

void MessageDao::createTables()
{
    QSqlQuery query;
    query.exec(
        "CREATE TABLE IF NOT EXISTS private_messages("
        "  id           INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  from_id      INTEGER NOT NULL,"
        "  to_id        INTEGER NOT NULL,"
        "  content      TEXT NOT NULL,"
        "  content_type TEXT DEFAULT 'text',"
        "  is_read      INTEGER DEFAULT 0,"
        "  created_at   INTEGER NOT NULL"
        ")"
    );
    query.exec(
        "CREATE INDEX IF NOT EXISTS idx_pm_from_to ON private_messages(from_id, to_id)"
    );
    query.exec(
        "CREATE INDEX IF NOT EXISTS idx_pm_created ON private_messages(created_at)"
    );

    query.exec(
        "CREATE TABLE IF NOT EXISTS channel_messages("
        "  id           INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  channel_id   INTEGER NOT NULL,"
        "  from_id      INTEGER NOT NULL,"
        "  content      TEXT NOT NULL,"
        "  content_type TEXT DEFAULT 'text',"
        "  created_at   INTEGER NOT NULL"
        ")"
    );
    query.exec(
        "CREATE INDEX IF NOT EXISTS idx_cm_channel ON channel_messages(channel_id)"
    );
    query.exec(
        "CREATE INDEX IF NOT EXISTS idx_cm_created ON channel_messages(created_at)"
    );

    query.exec(
        "CREATE TABLE IF NOT EXISTS channels("
        "  id         INTEGER PRIMARY KEY,"
        "  name       TEXT NOT NULL,"
        "  owner_id   INTEGER NOT NULL,"
        "  created_at INTEGER NOT NULL"
        ")"
    );
    query.exec(
        "CREATE TABLE IF NOT EXISTS channel_members("
        "  channel_id INTEGER NOT NULL,"
        "  user_id    INTEGER NOT NULL,"
        "  role       TEXT DEFAULT 'member',"
        "  muted      INTEGER DEFAULT 0,"
        "  joined_at  INTEGER NOT NULL,"
        "  PRIMARY KEY(channel_id, user_id)"
        ")"
    );
    query.exec(
        "CREATE INDEX IF NOT EXISTS idx_channel_members_user ON channel_members(user_id)"
    );
}

MessageDao::MessageDao()
{
    db = QSqlDatabase::addDatabase("QSQLITE", "message_connection");
    db.setDatabaseName("hhchat.db");
    if (db.open()) {
        createTables();
    }
}

MessageDao::~MessageDao()
{
    db.close();
}
