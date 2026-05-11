#include "MainWindow.h"
#include "ui_mainwindow.h"
#include <QHostInfo>
#include "NetworkThread.h"
#include "MessageDao.h"
#include "UserDao.h"
#include "LegacyLogger.h"
#include <QList>
#include <QDebug>
#include <algorithm>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    qRegisterMetaType<MsgBuilder::UserData>("MsgBuilder::UserData");
    ui->setupUi(this);
    showHostIp();
    createGroupId = MessageDao::getInstance()->nextChannelId(createGroupId);
    tcpServer.listen(QHostAddress::Any, PORT);
    connect(&tcpServer, SIGNAL(newConnectionForThread(qintptr)), this, SLOT(newConnection(qintptr)));
    appendServerStatus(QString("Server listening on port %1").arg(PORT));
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::showHostIp()
{
    QString localHostName = QHostInfo::localHostName();
    QHostInfo info = QHostInfo::fromName(localHostName);

    QList<QHostAddress> list = info.addresses();
    QString ip;
    for(int i = 0;i < list.size();i++)
    {
        if(list[i].protocol() == QAbstractSocket::IPv4Protocol)
        {
            ip += "*";
            ip += list[i].toString();
        }
    }
    ui->label_ip->setText(ip);
}

void MainWindow::newConnection(qintptr handle)
{
    activeConnections++;
    appendServerStatus(QString("Client connected. active=%1").arg(activeConnections));

    NetWorkThread* thread = new NetWorkThread(handle, this);
    connect(thread, SIGNAL(loginSuccess(MsgBuilder::UserData,QThread*)), this, SLOT(loginSuccess(MsgBuilder::UserData,QThread*)));
    connect(thread, SIGNAL(sendMsg(MsgBuilder::UserData,MsgBuilder::UserData,QString)), this, SLOT(sendMsg(MsgBuilder::UserData,MsgBuilder::UserData,QString)));
    connect(thread, SIGNAL(userOffline(MsgBuilder::UserData)), this, SLOT(userOffline(MsgBuilder::UserData)));
    connect(thread, SIGNAL(createGroup(QString, QThread*)), this, SLOT(createGroup(QString, QThread*)));
    connect(thread, SIGNAL(searchGroup(QThread*)), this, SLOT(searchGroup(QThread*)));
    connect(thread, SIGNAL(joinGroup(QThread*,int)), this, SLOT(joinGroup(QThread*,int)));
    connect(thread, SIGNAL(leaveGroup(QThread*,int)), this, SLOT(leaveGroup(QThread*,int)));
    connect(thread, SIGNAL(sendGroupMsg(QThread*,MsgBuilder::UserData,int,QString)), this, SLOT(sendGroupMsg(QThread*,MsgBuilder::UserData,int,QString)));
    thread->start();
}

void MainWindow::loginSuccess(MsgBuilder::UserData user, QThread *thread)
{
    if(usersOnline.count(user))
    {
        return;
    }

    std::vector<MsgBuilder::UserData> data;
    for(std::map<MsgBuilder::UserData,QThread*>::iterator iter = usersOnline.begin();
        iter != usersOnline.end();iter++)
    {
        data.push_back(iter->first);
        NetWorkThread* userThread = dynamic_cast<NetWorkThread*>(iter->second);
        userThread->addUserOnlineInfo(user);
    }

    NetWorkThread* netWorkThread = dynamic_cast<NetWorkThread*>(thread);
    netWorkThread->addLoginSuccessInfo(user, data);
    usersOnline.insert(std::pair<MsgBuilder::UserData,QThread*>(user, thread));

    std::vector<MessageRecord> offlineMsgs = MessageDao::getInstance()->getOfflinePrivateMsgs(user.id);
    for (std::vector<MessageRecord>::const_iterator it = offlineMsgs.begin(); it != offlineMsgs.end(); ++it) {
        MsgBuilder::UserData from;
        from.id = it->fromId;
        netWorkThread->addReceiveMsgInfo(from, user, it->content);
    }
    MessageDao::getInstance()->markPrivateMessagesRead(user.id);
    appendServerStatus(QString("User %1 login success. online=%2").arg(user.id).arg(usersOnline.size()));
}

void MainWindow::sendMsg(MsgBuilder::UserData from, MsgBuilder::UserData to, QString msg)
{
    MessageDao::getInstance()->insertPrivateMsg(from.id, to.id, msg);

    if(usersOnline.count(to) == 0)
    {
        return;
    }
    NetWorkThread* netWorkThread = dynamic_cast<NetWorkThread*>(usersOnline[to]);
    netWorkThread->addReceiveMsgInfo(from, to, msg);
    appendServerStatus(QString("private %1 -> %2").arg(from.id).arg(to.id));
}

void MainWindow::userOffline(MsgBuilder::UserData user)
{
    if(usersOnline.count(user) == 1)
    {
        usersOnline.erase(user);
    }
    if (activeConnections > 0) {
        activeConnections--;
    }

    for(std::map<MsgBuilder::UserData,QThread*>::iterator iter = usersOnline.begin();
        iter != usersOnline.end();iter++)
    {
        NetWorkThread* userThread = dynamic_cast<NetWorkThread*>(iter->second);
        userThread->addUserOfflineInfo(user);
    }
    appendServerStatus(QString("User %1 disconnected. active=%2 online=%3")
        .arg(user.id)
        .arg(activeConnections)
        .arg(usersOnline.size()));
}

void MainWindow::createGroup(QString name, QThread* thread)
{
    NetWorkThread* netWorkThread = dynamic_cast<NetWorkThread*>(thread);
    GroupKey gk;
    gk.groupId = createGroupId++;
    gk.name = name;
    groups[gk] = std::vector<QThread*>();
    MessageDao::getInstance()->createChannel(gk.groupId, name, netWorkThread->getUserData().id);
    netWorkThread->addCreateGroupReturnInfo(gk.groupId);
    appendServerStatus(QString("Channel created: %1 %2").arg(gk.groupId).arg(name));
}

void MainWindow::searchGroup(QThread *thread)
{
    std::vector<int> ids;
    std::vector<QString> names;
    std::vector<ChannelRecord> channels = MessageDao::getInstance()->listChannels();
    for(std::vector<ChannelRecord>::iterator iter = channels.begin(); iter != channels.end(); ++iter)
    {
        ids.push_back(iter->id);
        names.push_back(iter->name);
    }
    NetWorkThread* netWorkThread = dynamic_cast<NetWorkThread*>(thread);
    netWorkThread->addSearchGroupReturnInfo(ids, names);
}

void MainWindow::joinGroup(QThread *thread, int groupId)
{
    NetWorkThread* netWorkThread = dynamic_cast<NetWorkThread*>(thread);
    GroupKey gk = groupKeyById(groupId);
    std::vector<QThread*>& members = groups[gk];
    if (std::find(members.begin(), members.end(), thread) == members.end()) {
        members.push_back(thread);
    }
    MessageDao::getInstance()->addChannelMember(groupId, netWorkThread->getUserData().id);

    std::vector<MsgBuilder::UserData> groupFriends;
    for(int i = 0;i < members.size();i++)
    {
        NetWorkThread* groupFriend = dynamic_cast<NetWorkThread*>(members[i]);
        groupFriends.push_back(groupFriend->getUserData());
        if(groupFriend != netWorkThread)
        {
            groupFriend->addUserJoinGroupInfo(netWorkThread->getUserData(), groupId);
        }
    }
    netWorkThread->addJoinGroupReturnInfo(groupId, groupFriends);
}

void MainWindow::leaveGroup(QThread *thread, int groupId)
{
    NetWorkThread* netWorkThread = dynamic_cast<NetWorkThread*>(thread);
    GroupKey gk = groupKeyById(groupId);
    MessageDao::getInstance()->removeChannelMember(groupId, netWorkThread->getUserData().id);
    for(std::vector<QThread*>::iterator iter = groups[gk].begin();iter != groups[gk].end();)
    {
        if(*iter == thread)
        {
            iter = groups[gk].erase(iter);
        }
        else
        {
            NetWorkThread* groupFriend = dynamic_cast<NetWorkThread*>(*iter);
            groupFriend->addUserLeaveGroupInfo(netWorkThread->getUserData(), groupId);
            iter++;
        }
    }
}

void MainWindow::sendGroupMsg(QThread *thread, MsgBuilder::UserData from, int to, QString msg)
{
    MessageDao::getInstance()->insertGroupMsg(from.id, to, msg);

    GroupKey gk = groupKeyById(to);
    std::vector<QThread*> threads = groups[gk];
    for(int i = 0;i < threads.size();i++)
    {
        if(thread == threads[i])
        {
            continue;
        }
        NetWorkThread* groupFriend = dynamic_cast<NetWorkThread*>(threads[i]);
        groupFriend->addReceiveGroupMsgInfo(from, to, msg);
    }
}

void MainWindow::appendServerStatus(const QString& message)
{
    ui->textBrowser->append(message);
    LegacyLogger::info(message);
}

MainWindow::GroupKey MainWindow::groupKeyById(int groupId) const
{
    for (std::map<GroupKey, std::vector<QThread*> >::const_iterator it = groups.begin(); it != groups.end(); ++it) {
        if (it->first.groupId == groupId) {
            return it->first;
        }
    }
    std::vector<ChannelRecord> channels = MessageDao::getInstance()->listChannels();
    for (std::vector<ChannelRecord>::const_iterator it = channels.begin(); it != channels.end(); ++it) {
        if (it->id == groupId) {
            GroupKey key;
            key.groupId = it->id;
            key.name = it->name;
            return key;
        }
    }
    GroupKey fallback;
    fallback.groupId = groupId;
    return fallback;
}


