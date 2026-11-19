#include "protocol/messages.h"

#include "message_types.h" // 仓库 protocol/ 的框架无关枚举（CMake include dir 指向它）

#include <QJsonDocument>
#include <QJsonParseError>
#include <QtEndian>

namespace protocol {

using lanchat::protocol::MsgType;
using lanchat::protocol::toInt;

QByteArray encode(const QJsonObject& obj)
{
    const QByteArray payload = QJsonDocument(obj).toJson(QJsonDocument::Compact);
    QByteArray frame;
    frame.reserve(4 + payload.size());
    const quint32 n = quint32(payload.size());
    frame.append(char((n >> 24) & 0xff));
    frame.append(char((n >> 16) & 0xff));
    frame.append(char((n >> 8) & 0xff));
    frame.append(char(n & 0xff));
    frame.append(payload);
    return frame;
}

bool decode(const QByteArray& payload, QJsonObject& out, QString& err)
{
    QJsonParseError pe{};
    const QJsonDocument doc = QJsonDocument::fromJson(payload, &pe);
    if (pe.error != QJsonParseError::NoError || !doc.isObject()) {
        err = pe.errorString();
        return false;
    }
    out = doc.object();
    err.clear();
    return true;
}

QJsonObject login(const QString& idOrName, const QString& password)
{
    return QJsonObject{
        {"type", toInt(MsgType::Login)},
        {"id", idOrName}, // 字符串用户名；服务端按 nickname 解析
        {"password", password},
    };
}

QJsonObject loginNumeric(int id, const QString& password)
{
    return QJsonObject{
        {"type", toInt(MsgType::Login)},
        {"id", id},
        {"password", password},
    };
}

QJsonObject registerUser(const QString& nickname, const QString& password)
{
    return QJsonObject{
        {"type", toInt(MsgType::RegisterUser)},
        {"nickname", nickname},
        {"password", password},
    };
}

QJsonObject sendMsg(int fromId, int toId, const QString& msg)
{
    return QJsonObject{
        {"type", toInt(MsgType::SendMsg)},
        {"fromId", fromId},
        {"toId", toId},
        {"msg", msg},
    };
}

QJsonObject sendGroupMsg(int fromId, int groupId, const QString& msg)
{
    return QJsonObject{
        {"type", toInt(MsgType::SendGroupMsg)},
        {"fromId", fromId},
        {"groupId", groupId},
        {"msg", msg},
    };
}

QJsonObject heartbeat()
{
    return QJsonObject{{"type", toInt(MsgType::Heartbeat)}};
}

QJsonObject historyRequest(int id, int toId, int limit, const QString& cursor)
{
    QJsonObject o{
        {"type", toInt(MsgType::RequestHistory)},
        {"id", id},
        {"toId", toId},
        {"limit", limit},
    };
    if (!cursor.isEmpty()) o.insert("cursor", cursor);
    return o;
}

QJsonObject announce(int fromId, const QString& msg)
{
    return QJsonObject{
        {"type", toInt(MsgType::SystemBroadcast)},
        {"event", "announce"},
        {"fromId", fromId},
        {"msg", msg},
    };
}

} // namespace protocol