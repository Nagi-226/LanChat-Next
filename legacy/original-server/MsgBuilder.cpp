#include "MsgBuilder.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

namespace {
QJsonObject parseObject(const QString& jsonStr)
{
    QJsonParseError error;
    const QJsonDocument doc = QJsonDocument::fromJson(jsonStr.toUtf8(), &error);
    if (error.error != QJsonParseError::NoError || !doc.isObject()) {
        return QJsonObject();
    }
    return doc.object();
}

QString dumpObject(const QJsonObject& object)
{
    return QString::fromUtf8(QJsonDocument(object).toJson(QJsonDocument::Compact));
}

QJsonObject userObject(const MsgBuilder::UserData& user)
{
    QJsonObject object;
    object["id"] = user.id;
    object["headId"] = user.headId;
    object["nickname"] = user.nickname;
    return object;
}

MsgBuilder::UserData parseUser(const QJsonObject& object)
{
    MsgBuilder::UserData user;
    user.id = object.value("id").toInt();
    user.headId = object.value("headId").toInt();
    user.nickname = object.value("nickname").toString();
    return user;
}
}

int MsgBuilder::msgType(QString jsonStr)
{
    return parseObject(jsonStr).value("type").toInt(-1);
}

QString MsgBuilder::buildErrorMsg(int code, QString message)
{
    QJsonObject object;
    object["type"] = systemBroadcast;
    object["status"] = "error";
    object["code"] = code;
    object["msg"] = message;
    return dumpObject(object);
}

QString MsgBuilder::buildRegisterUserMsg(QString password, int headId, QString nickname)
{
    QJsonObject object;
    object["type"] = registerUser;
    object["password"] = password;
    object["headId"] = headId;
    object["nickname"] = nickname;
    return dumpObject(object);
}

MsgBuilder::UserData MsgBuilder::parseRegisterUserMsg(QString jsonStr)
{
    const QJsonObject object = parseObject(jsonStr);
    UserData data = parseUser(object);
    data.password = object.value("password").toString();
    return data;
}

QString MsgBuilder::buildRegisterUserReturnMsg(int id)
{
    QJsonObject object;
    object["type"] = registerUserReturn;
    object["id"] = id;
    return dumpObject(object);
}

int MsgBuilder::parseRegisterUserReturnMsg(QString jsonStr)
{
    return parseObject(jsonStr).value("id").toInt(-1);
}

QString MsgBuilder::buildLoginMsg(int id, QString password)
{
    QJsonObject object;
    object["type"] = login;
    object["id"] = id;
    object["password"] = password;
    return dumpObject(object);
}

MsgBuilder::UserData MsgBuilder::parseLoginMsg(QString jsonStr)
{
    const QJsonObject object = parseObject(jsonStr);
    UserData data;
    data.id = object.value("id").toInt();
    data.password = object.value("password").toString();
    return data;
}

QString MsgBuilder::buildLoginSucReturnMsg(MsgBuilder::UserData hostData, std::vector<UserData>& friends)
{
    QJsonObject object;
    object["type"] = loginSucReturn;
    object["hostId"] = hostData.id;
    object["hostHeadId"] = hostData.headId;
    object["hostNickname"] = hostData.nickname;

    QJsonArray friendsArray;
    for (std::vector<UserData>::const_iterator it = friends.begin(); it != friends.end(); ++it) {
        friendsArray.append(userObject(*it));
    }
    object["friends"] = friendsArray;
    return dumpObject(object);
}

std::vector<MsgBuilder::UserData> MsgBuilder::parseLoginSucReturnMsg(MsgBuilder::UserData& hostData, QString jsonStr)
{
    std::vector<UserData> friends;
    const QJsonObject object = parseObject(jsonStr);
    hostData.id = object.value("hostId").toInt();
    hostData.headId = object.value("hostHeadId").toInt();
    hostData.nickname = object.value("hostNickname").toString();

    const QJsonArray friendsArray = object.value("friends").toArray();
    for (QJsonArray::const_iterator it = friendsArray.begin(); it != friendsArray.end(); ++it) {
        friends.push_back(parseUser((*it).toObject()));
    }
    return friends;
}

QString MsgBuilder::buildLoginLoseReturnMsg()
{
    QJsonObject object;
    object["type"] = loginLoseReturn;
    return dumpObject(object);
}

QString MsgBuilder::buildSendMsg(MsgBuilder::UserData from, MsgBuilder::UserData to, QString msg)
{
    QJsonObject object;
    object["type"] = sendMsg;
    object["fromId"] = from.id;
    object["toId"] = to.id;
    object["msg"] = msg;
    return dumpObject(object);
}

QString MsgBuilder::parseSendMsg(QString jsonStr, MsgBuilder::UserData& from, MsgBuilder::UserData& to)
{
    const QJsonObject object = parseObject(jsonStr);
    from.id = object.value("fromId").toInt();
    to.id = object.value("toId").toInt();
    return object.value("msg").toString();
}

QString MsgBuilder::buildReceiveMsg(MsgBuilder::UserData from, MsgBuilder::UserData to, QString msg)
{
    QJsonObject object;
    object["type"] = receiveMsg;
    object["fromId"] = from.id;
    object["toId"] = to.id;
    object["msg"] = msg;
    return dumpObject(object);
}

QString MsgBuilder::parseReceiveMsg(QString jsonStr, MsgBuilder::UserData& from, MsgBuilder::UserData& to)
{
    const QJsonObject object = parseObject(jsonStr);
    from.id = object.value("fromId").toInt();
    to.id = object.value("toId").toInt();
    return object.value("msg").toString();
}

QString MsgBuilder::buildUserOnline(MsgBuilder::UserData user)
{
    QJsonObject object = userObject(user);
    object["type"] = userOnline;
    return dumpObject(object);
}

MsgBuilder::UserData MsgBuilder::parseUserOnline(QString jsonStr)
{
    return parseUser(parseObject(jsonStr));
}

QString MsgBuilder::buildUserOffline(MsgBuilder::UserData user)
{
    QJsonObject object = userObject(user);
    object["type"] = userOffline;
    return dumpObject(object);
}

MsgBuilder::UserData MsgBuilder::parseUserOffline(QString jsonStr)
{
    return parseUser(parseObject(jsonStr));
}

QString MsgBuilder::buildCreateGroup(QString name)
{
    QJsonObject object;
    object["type"] = createGroup;
    object["name"] = name;
    return dumpObject(object);
}

QString MsgBuilder::parseCreateGroup(QString jsonStr)
{
    return parseObject(jsonStr).value("name").toString();
}

QString MsgBuilder::buildCreateGroupReturn(int groupId)
{
    QJsonObject object;
    object["type"] = createGroupReturn;
    object["groupId"] = groupId;
    return dumpObject(object);
}

int MsgBuilder::parseCreateGroupReturn(QString jsonStr)
{
    return parseObject(jsonStr).value("groupId").toInt(-1);
}

QString MsgBuilder::buildSearchGroup()
{
    QJsonObject object;
    object["type"] = searchGroup;
    return dumpObject(object);
}

QString MsgBuilder::buildSearchGroupReturn(std::vector<int>& ids, std::vector<QString>& names)
{
    QJsonObject object;
    object["type"] = searchGroupReturn;
    QJsonArray groupsArray;
    for (size_t i = 0; i < ids.size() && i < names.size(); ++i) {
        QJsonObject group;
        group["id"] = ids[i];
        group["name"] = names[i];
        groupsArray.append(group);
    }
    object["groups"] = groupsArray;
    return dumpObject(object);
}

void MsgBuilder::parseSearchGroupReturn(QString jsonStr, std::vector<int>& ids, std::vector<QString>& names)
{
    const QJsonArray groupsArray = parseObject(jsonStr).value("groups").toArray();
    for (QJsonArray::const_iterator it = groupsArray.begin(); it != groupsArray.end(); ++it) {
        const QJsonObject group = (*it).toObject();
        ids.push_back(group.value("id").toInt());
        names.push_back(group.value("name").toString());
    }
}

QString MsgBuilder::buildJoinGroup(UserData user, int groupId)
{
    QJsonObject object = userObject(user);
    object["type"] = joinGroup;
    object["groupId"] = groupId;
    return dumpObject(object);
}

MsgBuilder::UserData MsgBuilder::parseJoinGroup(QString jsonStr, int& groupId)
{
    const QJsonObject object = parseObject(jsonStr);
    groupId = object.value("groupId").toInt();
    return parseUser(object);
}

QString MsgBuilder::buildJoinGroupReturn(int groupId, std::vector<UserData>& users)
{
    QJsonObject object;
    object["type"] = joinGroupReturn;
    object["groupId"] = groupId;
    QJsonArray usersArray;
    for (std::vector<UserData>::const_iterator it = users.begin(); it != users.end(); ++it) {
        usersArray.append(userObject(*it));
    }
    object["users"] = usersArray;
    return dumpObject(object);
}

int MsgBuilder::parseJoinGroupReturn(QString jsonStr, std::vector<UserData>& users)
{
    const QJsonObject object = parseObject(jsonStr);
    const QJsonArray usersArray = object.value("users").toArray();
    for (QJsonArray::const_iterator it = usersArray.begin(); it != usersArray.end(); ++it) {
        users.push_back(parseUser((*it).toObject()));
    }
    return object.value("groupId").toInt();
}

QString MsgBuilder::buildLeaveGroup(UserData user, int groupId)
{
    QJsonObject object = userObject(user);
    object["type"] = leaveGroup;
    object["groupId"] = groupId;
    return dumpObject(object);
}

int MsgBuilder::parseLeaveGroup(QString jsonStr, UserData& user)
{
    const QJsonObject object = parseObject(jsonStr);
    user = parseUser(object);
    return object.value("groupId").toInt();
}

QString MsgBuilder::buildSendGroupMsg(UserData from, int to, QString msg)
{
    QJsonObject object = userObject(from);
    object["type"] = sendGroupMsg;
    object["groupId"] = to;
    object["msg"] = msg;
    return dumpObject(object);
}

QString MsgBuilder::parseSendGroupMsg(QString jsonStr, UserData& from, int& to)
{
    const QJsonObject object = parseObject(jsonStr);
    from = parseUser(object);
    to = object.value("groupId").toInt();
    return object.value("msg").toString();
}

QString MsgBuilder::buildReceiveGroupMsg(int from, UserData to, QString msg)
{
    QJsonObject object = userObject(to);
    object["type"] = receiveGroupMsg;
    object["groupId"] = from;
    object["msg"] = msg;
    return dumpObject(object);
}

QString MsgBuilder::parseReceiveGroupMsg(QString jsonStr, int& from, UserData& to)
{
    const QJsonObject object = parseObject(jsonStr);
    to = parseUser(object);
    from = object.value("groupId").toInt();
    return object.value("msg").toString();
}

QString MsgBuilder::buildUserJoinGroup(UserData user, int groupId)
{
    QJsonObject object = userObject(user);
    object["type"] = userJoinGroup;
    object["groupId"] = groupId;
    return dumpObject(object);
}

void MsgBuilder::parseUserJoinGroup(QString jsonStr, UserData& user, int& groupId)
{
    const QJsonObject object = parseObject(jsonStr);
    user = parseUser(object);
    groupId = object.value("groupId").toInt();
}

QString MsgBuilder::buildUserLeaveGroup(UserData user, int groupId)
{
    QJsonObject object = userObject(user);
    object["type"] = userLeaveGroup;
    object["groupId"] = groupId;
    return dumpObject(object);
}

void MsgBuilder::parseUserLeaveGroup(QString jsonStr, UserData& user, int& groupId)
{
    const QJsonObject object = parseObject(jsonStr);
    user = parseUser(object);
    groupId = object.value("groupId").toInt();
}
