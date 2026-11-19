// frame_buffer.h —— 把 TCP 字节流切成一帧帧 JSON 载荷。
// 帧格式：[4 字节大端长度][JSON payload]。串口/TCP 都是字节流，没有消息边界，
// 所以必须靠长度头切帧；一次 recv 可能给半帧、也可能给好几帧（粘包）。
// 坏长度头（0 或超上限）说明流已错位：清空缓冲重新对齐，绝不把垃圾当帧、不 panic。
#pragma once

#include <QByteArray>

class FrameBuffer {
public:
    enum class Status {
        Ok,          // 切出一帧完整 payload
        Incomplete,  // 半包，等更多字节
        BadHeader,   // 长度头非法，缓冲已清空
    };

    void feed(const QByteArray& chunk);
    Status next(QByteArray& payload);
    void reset();
    int pending() const;

private:
    QByteArray buf_;
};