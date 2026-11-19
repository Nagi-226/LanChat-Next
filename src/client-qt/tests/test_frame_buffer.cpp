// test_frame_buffer.cpp —— FrameBuffer 切帧的独立单测。
// 覆盖：整帧 / 半包 / 粘包 / 坏长度头（0xFFFFFFFF 与 0）。
// 坏头必须返回 BadHeader 且清空缓冲（重对齐），不能 panic、不能把垃圾当帧。
#include "net/frame_buffer.h"

#include <QtEndian>
#include <cassert>
#include <iostream>

// 手工组一帧：4 字节大端长度 + payload（不复用被测代码的 encode）
static QByteArray frame(const QByteArray& p)
{
    QByteArray f;
    const quint32 n = quint32(p.size());
    f.append(char((n >> 24) & 0xff));
    f.append(char((n >> 16) & 0xff));
    f.append(char((n >> 8) & 0xff));
    f.append(char(n & 0xff));
    f.append(p);
    return f;
}

int main()
{
    FrameBuffer fb;
    QByteArray out;

    // 整帧
    fb.feed(frame("{\"a\":1}"));
    assert(fb.next(out) == FrameBuffer::Status::Ok && out == "{\"a\":1}");
    assert(fb.next(out) == FrameBuffer::Status::Incomplete);

    // 半包：先 3 字节（长度头都不全），再补剩余
    const QByteArray f = frame("{\"b\":2}");
    fb.feed(f.left(3));
    assert(fb.next(out) == FrameBuffer::Status::Incomplete);
    fb.feed(f.mid(3));
    assert(fb.next(out) == FrameBuffer::Status::Ok && out == "{\"b\":2}");

    // 粘包：两帧一次喂，应切出两帧
    fb.feed(frame("{\"c\":3}") + frame("{\"d\":4}"));
    assert(fb.next(out) == FrameBuffer::Status::Ok && out == "{\"c\":3}");
    assert(fb.next(out) == FrameBuffer::Status::Ok && out == "{\"d\":4}");
    assert(fb.next(out) == FrameBuffer::Status::Incomplete);

    // 坏头 0xFFFFFFFF：BadHeader + 缓冲清空
    fb.feed(QByteArray::fromHex("ffffffff") + "{\"e\":5}");
    assert(fb.next(out) == FrameBuffer::Status::BadHeader);
    assert(fb.pending() == 0);

    // 零长度头同样拒
    fb.feed(QByteArray::fromHex("00000000"));
    assert(fb.next(out) == FrameBuffer::Status::BadHeader);
    assert(fb.pending() == 0);

    // 坏头之后仍能重新对齐收好帧
    fb.feed(frame("{\"f\":6}"));
    assert(fb.next(out) == FrameBuffer::Status::Ok && out == "{\"f\":6}");

    std::cout << "frame_buffer tests passed\n";
    return 0;
}