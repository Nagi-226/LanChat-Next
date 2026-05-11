#ifndef HHTCPSOCKET_H
#define HHTCPSOCKET_H

#include <QByteArray>
#include <QHostAddress>
#include <QTcpSocket>
#include <QStringList>
#include <QTimer>
#include "MsgBuilder.h"

class HHTcpSocket : public QTcpSocket
{
    Q_OBJECT
public:
    static HHTcpSocket* getInstance();
    static void releaseInstance();
    inline void setHostData(MsgBuilder::UserData data){hostData = data;}
    inline MsgBuilder::UserData getHostData(){return hostData;}
    void connectToServer(const QHostAddress& address, quint16 port);
    qint64 sendJson(const QString& json);
    QStringList readJsonFrames();
private slots:
    void connectedToServer();
    void scheduleReconnect();
    void reconnectNow();
private:
    HHTcpSocket();
    ~HHTcpSocket();

    static HHTcpSocket* instance;
    MsgBuilder::UserData hostData;
    QByteArray readBuffer;
    QString serverHost;
    quint16 serverPort = 0;
    int reconnectDelayMs = 1000;
    QTimer reconnectTimer;
};

#endif // HHTCPSOCKET_H
