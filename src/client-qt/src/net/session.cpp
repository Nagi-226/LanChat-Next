#include "net/session.h"

#include "protocol/messages.h"

#include <QJsonDocument>
#include <QJsonArray>

Session::Session(QObject* parent) : QObject(parent)
{
    connect(&sock_, &QTcpSocket::connected, this, &Session::onConnected);
    connect(&sock_, &QTcpSocket::disconnected, this, &Session::onDisconnected);
    connect(&sock_, &QTcpSocket::errorOccurred, this, &Session::onError);
    connect(&sock_, &QTcpSocket::readyRead, this, &Session::onReadyRead);

    reconnectTimer_.setSingleShot(true);
    connect(&reconnectTimer_, &QTimer::timeout, this, &Session::onReconnectTimer);

    heartbeatTimer_.setInterval(10000); // 10s 一次 keepalive
    connect(&heartbeatTimer_, &QTimer::timeout, this, &Session::onHeartbeat);
}

void Session::connectToServer(const QString& host, quint16 port)
{
    host_ = host;
    port_ = port;
    wantConnected_ = true;
    backoffMs_ = 1000;
    emit stateChanged("连接中");
    sock_.abort();
    sock_.connectToHost(host_, port_);
}

void Session::login(const QString& idOrName, const QString& password)
{
    pendingUser_ = idOrName;
    pendingPass_ = password;
    pendingIsNumeric_ = false;
    if (sock_.state() == QAbstractSocket::ConnectedState) {
        send(protocol::login(idOrName, password));
        emit stateChanged("登录中");
    }
}

void Session::loginNumeric(int id, const QString& password)
{
    pendingNumericId_ = id;
    pendingPass_ = password;
    pendingIsNumeric_ = true;
    if (sock_.state() == QAbstractSocket::ConnectedState) {
        send(protocol::loginNumeric(id, password));
        emit stateChanged("登录中");
    }
}

void Session::registerUser(const QString& nickname, const QString& password)
{
    send(protocol::registerUser(nickname, password));
}

void Session::sendPrivate(int toId, const QString& msg)
{
    send(protocol::sendMsg(selfId_, toId, msg));
}

void Session::sendGroup(int groupId, const QString& msg)
{
    send(protocol::sendGroupMsg(selfId_, groupId, msg));
}

void Session::requestHistory(int toId, int limit, const QString& cursor)
{
    send(protocol::historyRequest(selfId_, toId, limit, cursor));
}

void Session::sendAnnounce(const QString& msg)
{
    send(protocol::announce(selfId_, msg));
}

void Session::logout()
{
    heartbeatTimer_.stop();
    wantConnected_ = false;
    reconnectTimer_.stop();
    sock_.disconnectFromHost();
    selfId_ = 0;
    selfNick_.clear();
    emit stateChanged("已 logout");
}

void Session::send(const QJsonObject& obj)
{
    if (sock_.state() != QAbstractSocket::ConnectedState) return;
    sock_.write(protocol::encode(obj));
}

void Session::onConnected()
{
    backoffMs_ = 1000; // 连上即复位退避
    emit stateChanged("已连接");
    // 连上后若有待登录凭据，立即登录
    if (!pendingPass_.isEmpty()) {
        if (pendingIsNumeric_) send(protocol::loginNumeric(pendingNumericId_, pendingPass_));
        else send(protocol::login(pendingUser_, pendingPass_));
        emit stateChanged("登录中");
    }
}

void Session::onDisconnected()
{
    heartbeatTimer_.stop();
    selfId_ = 0;
    if (wantConnected_) {
        emit stateChanged("断线，等待重连");
        scheduleReconnect();
    } else {
        emit stateChanged("已断开");
    }
}

void Session::onError(QAbstractSocket::SocketError)
{
    emit connectionError(sock_.errorString());
    if (wantConnected_ && sock_.state() == QAbstractSocket::UnconnectedState) {
        emit stateChanged("连接失败，等待重连");
        scheduleReconnect();
    }
}

void Session::scheduleReconnect()
{
    if (reconnectTimer_.isActive()) return;
    reconnectTimer_.start(backoffMs_);
    backoffMs_ = qMin(backoffMs_ * 2, 30000); // 1,2,4,8,16,30,30...
}

void Session::onReconnectTimer()
{
    if (!wantConnected_) return;
    emit stateChanged("重连中");
    sock_.abort();
    sock_.connectToHost(host_, port_);
}

void Session::onHeartbeat()
{
    send(protocol::heartbeat());
}

void Session::onReadyRead()
{
    fb_.feed(sock_.readAll());
    QByteArray payload;
    while (true) {
        const auto st = fb_.next(payload);
        if (st == FrameBuffer::Status::Incomplete) break;
        if (st == FrameBuffer::Status::BadHeader) {
            emit connectionError("坏长度头，缓冲已重对齐");
            continue;
        }
        QJsonObject obj;
        QString err;
        if (!protocol::decode(payload, obj, err)) {
            emit connectionError("坏 JSON: " + err);
            continue;
        }
        dispatch(obj);
    }
}

void Session::dispatch(const QJsonObject& obj)
{
    const int type = obj.value("type").toInt(-1);
    switch (type) {
    case 3: { // LoginSuccessReturn
        selfId_ = obj.value("id").toInt();
        selfNick_ = obj.value("nickname").toString();
        heartbeatTimer_.start();
        emit stateChanged("已登录");
        emit ready(selfId_, selfNick_);
        break;
    }
    case 4: // LoginFailedReturn
        emit authError(obj.value("msg").toString());
        emit stateChanged("登录失败");
        break;
    case 6:  // ReceiveMsg
    case 17: // ReceiveGroupMsg
        emit messageReceived(obj);
        break;
    case 7: // UserOnline
        emit presenceChanged(obj.value("id").toInt(), true);
        break;
    case 8: // UserOffline
        emit presenceChanged(obj.value("id").toInt(), false);
        break;
    case 42: // FriendOnline
        emit presenceChanged(obj.value("friendId").toInt(),
                             obj.value("status").toString() == "ok");
        break;
    case 22: { // OfflineMessages：逐条抛给上层
        const auto arr = obj.value("messages").toArray();
        for (const auto& m : arr) emit messageReceived(m.toObject());
        break;
    }
    case 25: // HistoryResponse
        emit historyReceived(obj);
        break;
    case 33: // SystemBroadcast (announce/typing)
        emit broadcastReceived(obj);
        break;
    case 21: // HeartbeatAck：确认对端活着，无需上层动作
        break;
    default:
        break; // 未知 type 忽略（subset 之外）
    }
}