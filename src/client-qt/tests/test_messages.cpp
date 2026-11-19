// test_messages.cpp —— protocol 编解码单测。
// 覆盖：encode→FrameBuffer→decode roundtrip；login 字符串 id 保持 string、
// loginNumeric 保持 number；坏 JSON decode 返回 false 且 err 非空；
// 未知 type 不拒（decode 只拒坏 JSON，类型语义留给上层）。
#include "protocol/messages.h"
#include "net/frame_buffer.h"

#include <QJsonObject>
#include <cassert>
#include <iostream>
#include "test_assert.h" // Release 下也真正求值的断言（防 NDEBUG 假绿）

int main()
{
    // roundtrip：login(用户名)
    const QJsonObject login = protocol::login("Nagi_226", "965163");
    assert(login.value("id").isString());
    assert(login.value("id").toString() == "Nagi_226");
    const QByteArray wire = protocol::encode(login);
    FrameBuffer fb;
    fb.feed(wire);
    QByteArray payload;
    assert(fb.next(payload) == FrameBuffer::Status::Ok);
    QJsonObject back;
    QString err;
    assert(protocol::decode(payload, back, err));
    assert(back == login);

    // loginNumeric：id 保持 number
    const QJsonObject ln = protocol::loginNumeric(1000, "965163");
    assert(ln.value("id").isDouble());
    assert(ln.value("id").toInt() == 1000);

    // 其余构造器 type 字段正确
    assert(protocol::registerUser("nick", "pw").value("type").toInt() == 0);
    assert(protocol::sendMsg(1, 2, "hi").value("type").toInt() == 5);
    assert(protocol::sendGroupMsg(1, 3, "hi").value("type").toInt() == 16);
    assert(protocol::heartbeat().value("type").toInt() == 20);
    assert(protocol::historyRequest(1, 2, 50, "").value("type").toInt() == 24);
    assert(protocol::announce(1, "hello all").value("type").toInt() == 33);

    // 坏 JSON：decode false + err 非空
    QJsonObject bad;
    QString berr;
    assert(!protocol::decode(QByteArray("{not json"), bad, berr));
    assert(!berr.isEmpty());

    // 未知 type：decode 成功（不拒），上层再决定忽略
    QJsonObject unk;
    QString uerr;
    assert(protocol::decode(QByteArray("{\"type\":999}"), unk, uerr));
    assert(unk.value("type").toInt() == 999);

    std::cout << "messages tests passed\n";
    return 0;
}