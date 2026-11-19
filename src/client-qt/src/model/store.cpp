#include "model/store.h"

Store::Store(QObject* parent) : QObject(parent) {}

void Store::setSelfId(int id) { selfId_ = id; }

QString Store::directKey(int peerId) { return QStringLiteral("d:%1").arg(peerId); }
QString Store::groupKey(int groupId) { return QStringLiteral("g:%1").arg(groupId); }

void Store::upsertContact(int id, const QString& nickname, bool online)
{
    Contact c = contacts_.value(id);
    c.id = id;
    c.nickname = nickname;
    c.online = online;
    contacts_.insert(id, c);
    emit changed();
}

void Store::setOnline(int id, bool online)
{
    if (!contacts_.contains(id)) return;
    contacts_[id].online = online;
    emit changed();
}

void Store::addMessage(const Message& m)
{
    // direct 会话按"对方 id"归档：自己发的对方=toId，收到的对方=fromId
    const QString key = (m.kind == "group")
                            ? groupKey(m.groupId)
                            : directKey(m.fromId == selfId_ ? m.toId : m.fromId);
    convs_[key].append(m);
    emit changed();
}

QList<Contact> Store::contacts() const
{
    return contacts_.values();
}

QList<Message> Store::conversation(const QString& key) const
{
    return convs_.value(key);
}

QList<Message> Store::search(const QString& query) const
{
    QList<Message> hits;
    if (query.trimmed().isEmpty()) return hits;
    const QString q = query.toLower();
    for (const auto& list : convs_) {
        for (const auto& m : list) {
            if (m.text.toLower().contains(q)) hits.append(m);
        }
    }
    return hits;
}