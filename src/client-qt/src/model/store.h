// store.h —— 客户端内存模型：联系人 / 会话消息 / 本地搜索。
// 不联网、不落盘；数据来自 Session 信号。搜索 = 大小写不敏感 substring（对应现 UI Local Search）。
#pragma once

#include <QObject>
#include <QJsonObject>
#include <QHash>
#include <QList>
#include <QString>

struct Contact {
    int id = 0;
    QString nickname;
    bool online = false;
};

struct Message {
    int fromId = 0;
    int toId = 0;
    int groupId = 0;
    QString text;
    qint64 ts = 0;
    QString kind; // "direct" / "group"
    QString id;
    QString contentType="text";
    QJsonObject attachment;
};

class Store : public QObject {
    Q_OBJECT
public:
    explicit Store(QObject* parent = nullptr);

    // 登录成功后告知自己的 id，direct 会话才能按"对方 id"归档
    void setSelfId(int id);
    void upsertContact(int id, const QString& nickname, bool online);
    void setOnline(int id, bool online);
    bool addMessage(const Message& m);
    void clear();

    QList<Contact> contacts() const;
    // key = "d:<peerId>" 或 "g:<groupId>"
    QList<Message> conversation(const QString& key) const;
    QList<Message> search(const QString& query) const;

    static QString directKey(int peerId);
    static QString groupKey(int groupId);

signals:
    void changed();

private:
    QHash<int, Contact> contacts_;
    QHash<QString, QList<Message>> convs_;
    int selfId_ = 0;
};