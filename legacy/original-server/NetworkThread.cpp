#include "NetworkThread.h"
#include <QDebug>
#include "UserDao.h"
#include "UserEntity.h"
#include "LegacyProtocolCodec.h"
#include "LegacyLogger.h"
#include <QMutexLocker>
#include <QStringList>


NetWorkThread::NetWorkThread()
{
    
}

NetWorkThread::NetWorkThread(qintptr handle, QObject *parent):
    QThread(parent)
{
    //鑷畾涔変俊鍙峰甫鑷畾涔夊弬鏁扮被鍨嬶紝闇€瑕佹敞鍐屽弬鏁扮被鍨?
    //qRegisterMetaType<MsgBuilder::UserData>("MsgBuilder::UserData");
        
    //杩欎釜socket杩樺彲浠ュ湪绾跨▼鐨勯€昏緫涓洿鎺ュ垱寤?
    socket = new QTcpSocket;
    socket->setSocketDescriptor(handle);
    test = socket->peerPort();
    socket->moveToThread(this);//鏋勯€犲嚱鏁扮殑鎵ц鏄湪涓荤嚎绋嬶紝鎵€浠ヨ繖涓渶瑕佹妸鑷繁鍒涘缓鐨凲TcpSocket瀵硅薄绉讳氦缁欏瓙绾跨▼
    connect(socket, SIGNAL(readyRead()), this, SLOT(readyRead()));
    connect(socket, SIGNAL(disconnected()), this, SLOT(disconnected()));   
    LegacyLogger::info(QString("Client connected: %1:%2")
        .arg(socket->peerAddress().toString())
        .arg(socket->peerPort()));
}

NetWorkThread::~NetWorkThread()
{
    
}

void NetWorkThread::addLoginSuccessInfo(MsgBuilder::UserData user, std::vector<MsgBuilder::UserData> &friends)
{
    QString ret = MsgBuilder::buildLoginSucReturnMsg(user, friends);
    mutex.lock();
    msgQueue.push(ret);
    mutex.unlock();
}

void NetWorkThread::addReceiveMsgInfo(MsgBuilder::UserData from, MsgBuilder::UserData to, QString msg)
{
    QString ret = MsgBuilder::buildReceiveMsg(from, to, msg);
    mutex.lock();
    msgQueue.push(ret);
    mutex.unlock();
}

void NetWorkThread::addUserOnlineInfo(MsgBuilder::UserData user)
{
    QString ret = MsgBuilder::buildUserOnline(user);
    mutex.lock();
    msgQueue.push(ret);
    mutex.unlock();
}

void NetWorkThread::addUserOfflineInfo(MsgBuilder::UserData user)
{
    QString ret = MsgBuilder::buildUserOffline(user);
    mutex.lock();
    msgQueue.push(ret);
    mutex.unlock();
}

void NetWorkThread::addCreateGroupReturnInfo(int groupId)
{
    QString ret = MsgBuilder::buildCreateGroupReturn(groupId);
    mutex.lock();
    msgQueue.push(ret);
    mutex.unlock();
}

void NetWorkThread::addSearchGroupReturnInfo(std::vector<int>& ids, std::vector<QString>& names)
{
    QString ret = MsgBuilder::buildSearchGroupReturn(ids, names);
    mutex.lock();
    msgQueue.push(ret);
    mutex.unlock();
}

void NetWorkThread::addJoinGroupReturnInfo(int groupId, std::vector<MsgBuilder::UserData> &groupFriends)
{  
    QString ret = MsgBuilder::buildJoinGroupReturn(groupId, groupFriends);
    mutex.lock();
    msgQueue.push(ret);
    mutex.unlock();
}

void NetWorkThread::addUserJoinGroupInfo(MsgBuilder::UserData user, int groupId)
{
    QString ret = MsgBuilder::buildUserJoinGroup(user, groupId);
    mutex.lock();
    msgQueue.push(ret);
    mutex.unlock();
}

void NetWorkThread::addUserLeaveGroupInfo(MsgBuilder::UserData user, int groupId)
{
    QString ret = MsgBuilder::buildUserLeaveGroup(user, groupId);
    mutex.lock();
    msgQueue.push(ret);
    mutex.unlock();
}

void NetWorkThread::addReceiveGroupMsgInfo(MsgBuilder::UserData to, int from, QString msg)
{
    QString ret = MsgBuilder::buildReceiveGroupMsg(from, to, msg);
    mutex.lock();
    msgQueue.push(ret);
    mutex.unlock();
}

void NetWorkThread::run()
{
    while(true)
    {
        if(isDisconnected)//鏂紑杩炴帴
        {
            break;
        }
        if(isRead)//璇诲彇鏁版嵁
        {
            isRead = false;
            while(socket->bytesAvailable()>0)//鍙兘鎺ユ敹鍒颁笉鍙竴甯ф暟鎹?
            {
                QByteArray byteArray = socket->readAll();
                inputBuffer.append(byteArray);
                const QStringList frames = LegacyProtocolCodec::decodeFrames(inputBuffer);
                QMutexLocker locker(&mutex);
                for (QStringList::const_iterator it = frames.begin(); it != frames.end(); ++it) {
                    msgQueue.push(*it);
                }
            }
        }
        socket->waitForReadyRead(10);//浣跨敤waitForReadyRead鍋氬欢鏃讹紝闃叉鍥犱负寰幆閫熷害杩囧揩涓嶈兘鏀跺埌socket淇℃伅
        mutex.lock();
        if(msgQueue.empty())//濡傛灉娑堟伅闃熷垪鏄┖锛屽惊鐜笅闈㈢殑浠ｇ爜灏变笉鎵ц浜?
        {
            mutex.unlock();
            continue;
        }
        QString data = msgQueue.front();  //娑堟伅鍑洪槦     
        msgQueue.pop();
        mutex.unlock();
        switch(MsgBuilder::msgType(data))
        {
        case MsgBuilder::registerUser:
            dealRegisterUser(data);
            break;
        case MsgBuilder::login:
            dealLogin(data);
            break;
        case MsgBuilder::loginSucReturn:
            dealLoginSuc(data);
            break;
        case MsgBuilder::sendMsg:
            dealSendMsg(data);
            break;
        case MsgBuilder::receiveMsg:
            dealReceiveMsg(data);
            break;
        case MsgBuilder::userOnline:
            dealUserOnline(data);
            break;
        case MsgBuilder::userOffline:
            dealUserOffline(data);
            break;
        case MsgBuilder::createGroup:
            dealCreateGroup(data);
            break;
        case MsgBuilder::createGroupReturn:
            dealCreateGroupReturn(data);
            break;
        case MsgBuilder::searchGroup:
            dealSearchGroup(data);
            break;
        case MsgBuilder::searchGroupReturn:
            dealSearchGroupReturn(data);
            break;
        case MsgBuilder::joinGroup:
            dealJoinGroup(data);
            break;
        case MsgBuilder::joinGroupReturn:
            dealJoinGroupReturn(data);
            break;
        case MsgBuilder::userJoinGroup:
            dealUserJoinGroup(data);
            break;
        case MsgBuilder::leaveGroup:
            dealLeaveGroup(data);
            break;
        case MsgBuilder::userLeaveGroup:
            dealUserLeaveGroup(data);
            break;
        case MsgBuilder::sendGroupMsg:
            dealSendGroupMsg(data);
            break;
        case MsgBuilder::receiveGroupMsg:
            dealReceiveGroupMsg(data);
            break;
        default:
            LegacyLogger::warn(QString("Invalid message type from port %1: %2").arg(test).arg(data.left(200)));
            sendJson(MsgBuilder::buildErrorMsg(400, "invalid message type"));
            break;
        }
    }
    LegacyLogger::info(QString("Client disconnected: user=%1 port=%2").arg(user.id).arg(test));
    emit userOffline(user);
}

void NetWorkThread::dealRegisterUser(QString jsonStr)
{
    //瑙ｆ瀽鏁版嵁
    MsgBuilder::UserData data = MsgBuilder::parseRegisterUserMsg(jsonStr);
    //鎻掑叆鏁版嵁搴?
    UserEntity ue;
    ue.password = data.password;
    ue.headId = data.headId;
    ue.nickname = data.nickname;
    UserDao* dao = UserDao::getInstance();
    int id = dao->insertUser(ue);//鎻掑叆鏁版嵁搴?
    //杩斿洖id淇℃伅
    QString retData = MsgBuilder::buildRegisterUserReturnMsg(id);
    sendJson(retData);
}

void NetWorkThread::dealLogin(QString jsonStr)
{
    //瑙ｆ瀽鏁版嵁
    MsgBuilder::UserData data = MsgBuilder::parseLoginMsg(jsonStr);
    //鏌ヨ鏁版嵁搴?
    UserEntity ue;
    ue.password = data.password;
    ue.id = data.id;
    UserDao* dao = UserDao::getInstance();
    if(dao->verifyPassword(ue))// Login success
    {
        data.nickname = ue.nickname;
        data.headId = ue.headId;
        user = data;//绾跨▼鎸佹湁鐢ㄦ埛淇℃伅
        emit loginSuccess(data, this);
    }
    else//鐧诲綍澶辫触
    {
        QString ret = MsgBuilder::buildLoginLoseReturnMsg();
        sendJson(ret);
    }
}

void NetWorkThread::dealLoginSuc(QString jsonStr)
{
    sendJson(jsonStr);
}

void NetWorkThread::dealSendMsg(QString jsonStr)
{
    MsgBuilder::UserData from;
    MsgBuilder::UserData to;
    QString msg = MsgBuilder::parseSendMsg(jsonStr, from, to);
    emit sendMsg(from, to, msg);
}

void NetWorkThread::dealReceiveMsg(QString jsonStr)
{
    sendJson(jsonStr);
}

void NetWorkThread::dealUserOnline(QString jsonStr)
{
    sendJson(jsonStr);
}

void NetWorkThread::dealUserOffline(QString jsonStr)
{
    sendJson(jsonStr);
}

void NetWorkThread::dealCreateGroup(QString jsonStr)
{
    QString name = MsgBuilder::parseCreateGroup(jsonStr);
    emit createGroup(name, this);
}

void NetWorkThread::dealCreateGroupReturn(QString jsonStr)
{
    sendJson(jsonStr);
}

void NetWorkThread::dealSearchGroup(QString jsonStr)
{
    emit searchGroup(this);
}

void NetWorkThread::dealSearchGroupReturn(QString jsonStr)
{
    sendJson(jsonStr);
}

void NetWorkThread::dealJoinGroup(QString jsonStr)
{
    int groupId;
    MsgBuilder::UserData user = MsgBuilder::parseJoinGroup(jsonStr, groupId);
    emit joinGroup(this, groupId);
}

void NetWorkThread::dealJoinGroupReturn(QString jsonStr)
{
    sendJson(jsonStr);
}

void NetWorkThread::dealUserJoinGroup(QString jsonStr)
{
    sendJson(jsonStr);
}

void NetWorkThread::dealLeaveGroup(QString jsonStr)
{
    MsgBuilder::UserData user;
    int groupId = MsgBuilder::parseLeaveGroup(jsonStr, user);
    emit leaveGroup(this, groupId);
}

void NetWorkThread::dealUserLeaveGroup(QString jsonStr)
{
    sendJson(jsonStr);
}

void NetWorkThread::dealSendGroupMsg(QString jsonStr)
{
    int groupId;
    MsgBuilder::UserData user;
    QString msg = MsgBuilder::parseSendGroupMsg(jsonStr, user, groupId);
    emit sendGroupMsg(this, user, groupId, msg);
}

void NetWorkThread::dealReceiveGroupMsg(QString jsonStr)
{
    sendJson(jsonStr);
}

void NetWorkThread::readyRead()
{
    isRead = true;
}

void NetWorkThread::disconnected()
{
    isDisconnected = true;
}


void NetWorkThread::sendJson(QString jsonStr)
{
    socket->write(LegacyProtocolCodec::encodeFrame(jsonStr));
}
