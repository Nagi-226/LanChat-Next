// test_session.cpp —— Session 网络会话 loopback 测试。
// 测试内起 QTcpServer 假服务端（独立用 FrameBuffer 切帧），对 Login 回 type3、推 type6；
// 断言 ready / messageReceived；断开后断言进入重连态。收尾先断回调再关监听，防悬垂。
#include "net/session.h"
#include "net/frame_buffer.h"
#include "protocol/messages.h"

#include <QTcpServer>
#include <QTcpSocket>
#include <QSignalSpy>
#include <QTest>
#include <QJsonObject>
#include <cassert>
#include <iostream>
#include "test_assert.h" // Release 下也真正求值的断言（防 NDEBUG 假绿）

int main(int argc, char** argv)
{
    QCoreApplication app(argc, argv);

    QTcpServer server;
    assert(server.listen(QHostAddress::LocalHost, 0));
    const quint16 port = server.serverPort();

    FrameBuffer peerFb;
    QObject::connect(&server, &QTcpServer::newConnection, [&]() {
        QTcpSocket* p = server.nextPendingConnection();
        if (!p) return;
        QObject::connect(p, &QTcpSocket::readyRead, [p, &peerFb]() {
            if (p->state() != QAbstractSocket::ConnectedState) return;
            peerFb.feed(p->readAll());
            QByteArray payload;
            while (peerFb.next(payload) == FrameBuffer::Status::Ok) {
                QJsonObject req;
                QString err;
                if (!protocol::decode(payload, req, err)) continue;
                if (req.value("type").toInt() == 2) {
                    QJsonObject resp{
                        {"type", 3}, {"id", 1000}, {"nickname", "Nagi_226"}, {"headId", 0},
                    };
                    if (p->state() == QAbstractSocket::ConnectedState)
                        p->write(protocol::encode(resp));
                }
            }
        });
    });

    Session sess;
    QSignalSpy readySpy(&sess, &Session::ready);
    QSignalSpy msgSpy(&sess, &Session::messageReceived);
    QSignalSpy stateSpy(&sess, &Session::stateChanged);

    sess.connectToServer("127.0.0.1", port);
    sess.login("Nagi_226", "965163");
    assert(readySpy.wait(3000));
    assert(readySpy.last().at(0).toInt() == 1000);
    assert(readySpy.last().at(1).toString() == "Nagi_226");

    // 假服务端推一条私聊 type6：取服务端侧当前 socket
    QTcpSocket* peer = nullptr;
    for (QObject* o : server.children())
        if (auto* s = qobject_cast<QTcpSocket*>(o)) peer = s;
    assert(peer);
    QJsonObject incoming{{"type", 6}, {"fromId", 2}, {"toId", 1000}, {"msg", "hello"}};
    peer->write(protocol::encode(incoming));
    assert(msgSpy.wait(3000));
    assert(msgSpy.last().at(0).toJsonObject().value("msg").toString() == "hello");

    // 断开 → Session 应进入重连态
    peer->disconnectFromHost();
    bool sawReconnect = false;
    for (int i = 0; i < 40 && !sawReconnect; ++i) {
        QTest::qWait(100);
        for (const auto& a : stateSpy)
            if (a.at(0).toString().contains("重连")) sawReconnect = true;
    }
    assert(sawReconnect);

    // 干净收尾：停会话重连 → 断所有 peer 回调 → 关监听
    sess.logout();
    for (QObject* o : server.children())
        if (auto* s = qobject_cast<QTcpSocket*>(o)) s->disconnect();
    server.close();
    QTest::qWait(50);

    std::cout << "session tests passed\n";
    return 0;
}