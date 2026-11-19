// messages.h —— 客户端协议编解码（subset）。
// 帧 = [4 字节大端长度][compact JSON]。type 值直接取自仓库 protocol/message_types.h
// （框架无关头），保证客户端与服务端枚举同源，不各自抄一份数字。
#pragma once

#include <QByteArray>
#include <QJsonObject>
#include <QString>

namespace protocol {

QByteArray encode(const QJsonObject& obj);
bool decode(const QByteArray& payload, QJsonObject& out, QString& err);

// 构造器：均带正确 type 字段。login 的 id 为用户名字符串；loginNumeric 为数字 id。
QJsonObject login(const QString& idOrName, const QString& password);
QJsonObject loginNumeric(int id, const QString& password);
QJsonObject registerUser(const QString& nickname, const QString& password);
QJsonObject sendMsg(int fromId, int toId, const QString& msg);
QJsonObject sendGroupMsg(int fromId, int groupId, const QString& msg);
QJsonObject heartbeat();
QJsonObject historyRequest(int id, int toId, int limit, const QString& cursor);
QJsonObject announce(int fromId, const QString& msg);

} // namespace protocol