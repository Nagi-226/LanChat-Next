#ifndef NETWORKTHREAD_H
#define NETWORKTHREAD_H

#include <QByteArray>
#include <QMutex>
#include <QThread>
#include <QTcpSocket>
#include <queue>
#include <vector>
#include "MsgBuilder.h"

class NetWorkThread : public QThread
{
    Q_OBJECT
public:
    NetWorkThread();
    NetWorkThread(qintptr handle, QObject* parent);
    ~NetWorkThread();

    inline MsgBuilder::UserData getUserData(){return user;}

    void addLoginSuccessInfo(MsgBuilder::UserData user, std::vector<MsgBuilder::UserData>& friends);
    void addReceiveMsgInfo(MsgBuilder::UserData from, MsgBuilder::UserData to, QString msg);
    void addUserOnlineInfo(MsgBuilder::UserData user);
    void addUserOfflineInfo(MsgBuilder::UserData user);
    void addCreateGroupReturnInfo(int groupId);
    void addSearchGroupReturnInfo(std::vector<int>& ids, std::vector<QString>& names);
    void addJoinGroupReturnInfo(int groupId, std::vector<MsgBuilder::UserData>& groupFriends);
    void addUserJoinGroupInfo(MsgBuilder::UserData user, int groupId);
    void addUserLeaveGroupInfo(MsgBuilder::UserData user, int groupId);
    void addReceiveGroupMsgInfo(MsgBuilder::UserData from, int to, QString msg);
private:
    void run();
    void sendJson(QString jsonStr);

    void dealRegisterUser(QString jsonStr);
    void dealLogin(QString jsonStr);
    void dealLoginSuc(QString jsonStr);
    void dealSendMsg(QString jsonStr);
    void dealReceiveMsg(QString jsonStr);
    void dealUserOnline(QString jsonStr);
    void dealUserOffline(QString jsonStr);
    void dealCreateGroup(QString jsonStr);
    void dealCreateGroupReturn(QString jsonStr);
    void dealSearchGroup(QString jsonStr);
    void dealSearchGroupReturn(QString jsonStr);
    void dealJoinGroup(QString jsonStr);
    void dealJoinGroupReturn(QString jsonStr);
    void dealUserJoinGroup(QString jsonStr);
    void dealLeaveGroup(QString jsonStr);
    void dealUserLeaveGroup(QString jsonStr);
    void dealSendGroupMsg(QString jsonStr);
    void dealReceiveGroupMsg(QString jsonStr);
private slots:
    void readyRead();
    void disconnected();
signals:
    void loginSuccess(MsgBuilder::UserData user, QThread* thread);
    void sendMsg(MsgBuilder::UserData from, MsgBuilder::UserData to, QString msg);
    void userOffline(MsgBuilder::UserData user);
    void createGroup(QString name, QThread* self);
    void searchGroup(QThread* self);
    void joinGroup(QThread* self, int groupId);
    void leaveGroup(QThread* self, int groupId);
    void sendGroupMsg(QThread* self, MsgBuilder::UserData from, int to, QString msg);
private:
    QTcpSocket* socket = 0;
    std::queue<QString> msgQueue;
    QByteArray inputBuffer;
    bool isDisconnected = false;
    bool isRead = false;
    int test = 0;
    QMutex mutex;
    MsgBuilder::UserData user;
};

#endif // NETWORKTHREAD_H
