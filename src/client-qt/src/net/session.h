// session.h —— 客户端网络会话：连接 / 登录 / 收发 / 退避重连 / heartbeat。
// 状态机：Disconnected → Connecting → LoggingIn → Ready。
// 重连退避 1→2→4→8→16→30s（上限 30s），连上即复位 1s；heartbeat type20 每 10s（仅 Ready 后）。
// 所有对外信号把原始 QJsonObject 抛给上层（model/ui），Session 不做业务解释。
#pragma once

#include <QObject>
#include <QTcpSocket>
#include <QTimer>
#include <QJsonObject>

#include "net/frame_buffer.h"

class Session : public QObject {
    Q_OBJECT
public:
    explicit Session(QObject* parent = nullptr);

    void connectToServer(const QString& host, quint16 port);
    void login(const QString& idOrName, const QString& password);
    void loginNumeric(int id, const QString& password);
    void registerUser(const QString& nickname, const QString& password);
    void sendPrivate(int toId, const QString& msg);
    void sendGroup(int groupId, const QString& msg);
    void requestHistory(int toId, int limit, const QString& cursor);
    void sendAnnounce(const QString& msg);
    void createGroup(const QString& name);
    void joinGroup(int groupId);
    void logout();
    void leaveGroup(int groupId);
    void requestMembers(int groupId);
    void requestConversationHistory(int id,bool group,const QString& before={});
    void sendAttachment(const QJsonObject& object);
    QString endpoint() const { return host_+":"+QString::number(port_); }

    int selfId() const { return selfId_; }
    QString selfNick() const { return selfNick_; }
    QString state() const {return connectionState_;}

signals:
    void ready(int uid, const QString& nick);
    void usersReceived(const QJsonObject& loginSuccess);
    void groupEvent(const QJsonObject& evt);
    void authError(const QString& msg);
    void registered(int id);
    void attachmentResponse(const QJsonObject& object);
    void messageReceived(const QJsonObject& msg);
    void presenceChanged(int id, bool online);
    void historyReceived(const QJsonObject& resp);
    void broadcastReceived(const QJsonObject& msg);
    void stateChanged(const QString& state);
    void connectionError(const QString& msg);

private slots:
    void onConnected();
    void onDisconnected();
    void onError(QAbstractSocket::SocketError);
    void onReadyRead();
    void onReconnectTimer();
    void onHeartbeat();

private:
    void send(const QJsonObject& obj);
    void scheduleReconnect();
    void dispatch(const QJsonObject& obj);

    QTcpSocket sock_;
    FrameBuffer fb_;
    QTimer reconnectTimer_;
    QTimer heartbeatTimer_;
    QString host_;
    quint16 port_ = 0;
    int backoffMs_ = 1000;
    bool wantConnected_ = false;
    int selfId_ = 0;
    QString selfNick_;
    QString connectionState_="未连接";
    QString pendingUser_;
    QString pendingPass_;
    bool pendingIsNumeric_ = false;
    int pendingNumericId_ = 0;
};
