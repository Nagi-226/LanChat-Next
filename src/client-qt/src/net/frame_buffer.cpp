#include "net/frame_buffer.h"

#include <QtEndian>

namespace {
// 单帧上限 1 MiB：与服务端 FrameCodec 同口径，防异常长度头撑爆内存。
constexpr quint32 kMaxFrame = 1u << 20;
}

void FrameBuffer::feed(const QByteArray& chunk)
{
    buf_.append(chunk);
}

FrameBuffer::Status FrameBuffer::next(QByteArray& payload)
{
    if (buf_.size() < 4) {
        return Status::Incomplete;
    }
    const quint32 n = qFromBigEndian<quint32>(
        reinterpret_cast<const uchar*>(buf_.constData()));
    if (n == 0 || n > kMaxFrame) {
        buf_.clear();
        return Status::BadHeader;
    }
    if (buf_.size() < 4 + int(n)) {
        return Status::Incomplete;
    }
    payload = buf_.mid(4, int(n));
    buf_.remove(0, 4 + int(n));
    return Status::Ok;
}

void FrameBuffer::reset()
{
    buf_.clear();
}

int FrameBuffer::pending() const
{
    return buf_.size();
}