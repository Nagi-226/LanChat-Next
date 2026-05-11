#include "HHTcpSocket.h"
#include "LegacyProtocolCodec.h"

HHTcpSocket* HHTcpSocket::instance = 0;

HHTcpSocket *HHTcpSocket::getInstance()
{
    if(instance == 0)
    {
        instance = new HHTcpSocket;
    }
    return instance;
}

void HHTcpSocket::releaseInstance()
{
    if(instance != 0)
    {
        instance->close();
        delete instance;
        instance = 0;
    }
}

HHTcpSocket::HHTcpSocket()
{
    reconnectTimer.setSingleShot(true);
    connect(this, SIGNAL(connected()), this, SLOT(connectedToServer()));
    connect(this, SIGNAL(disconnected()), this, SLOT(scheduleReconnect()));
    connect(&reconnectTimer, SIGNAL(timeout()), this, SLOT(reconnectNow()));
}

HHTcpSocket::~HHTcpSocket()
{
}

void HHTcpSocket::connectToServer(const QHostAddress& address, quint16 port)
{
    serverHost = address.toString();
    serverPort = port;
    reconnectDelayMs = 1000;
    connectToHost(address, port);
}

qint64 HHTcpSocket::sendJson(const QString& json)
{
    return write(LegacyProtocolCodec::encodeFrame(json));
}

QStringList HHTcpSocket::readJsonFrames()
{
    readBuffer.append(readAll());
    return LegacyProtocolCodec::decodeFrames(readBuffer);
}

void HHTcpSocket::connectedToServer()
{
    reconnectDelayMs = 1000;
    reconnectTimer.stop();
}

void HHTcpSocket::scheduleReconnect()
{
    if (serverHost.isEmpty() || serverPort == 0) {
        return;
    }
    if (!reconnectTimer.isActive()) {
        reconnectTimer.start(reconnectDelayMs);
        reconnectDelayMs = qMin(reconnectDelayMs * 2, 30000);
    }
}

void HHTcpSocket::reconnectNow()
{
    if (serverHost.isEmpty() || serverPort == 0) {
        return;
    }
    connectToHost(QHostAddress(serverHost), serverPort);
}
