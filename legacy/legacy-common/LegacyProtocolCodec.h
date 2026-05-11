#ifndef LEGACY_PROTOCOL_CODEC_H
#define LEGACY_PROTOCOL_CODEC_H

#include <QByteArray>
#include <QString>
#include <QStringList>

class LegacyProtocolCodec
{
public:
    static QByteArray encodeFrame(const QString& json)
    {
        QByteArray body = json.toUtf8();
        const quint32 len = static_cast<quint32>(body.size());

        QByteArray frame;
        frame.reserve(4 + body.size());
        frame.append(static_cast<char>((len >> 24) & 0xff));
        frame.append(static_cast<char>((len >> 16) & 0xff));
        frame.append(static_cast<char>((len >> 8) & 0xff));
        frame.append(static_cast<char>(len & 0xff));
        frame.append(body);
        return frame;
    }

    static QStringList decodeFrames(QByteArray& buffer)
    {
        QStringList frames;
        while (!buffer.isEmpty()) {
            if (buffer.at(0) == '{') {
                frames << QString::fromUtf8(buffer);
                buffer.clear();
                break;
            }

            if (buffer.size() < 4) {
                break;
            }

            const quint32 len =
                ((static_cast<quint32>(static_cast<quint8>(buffer.at(0))) << 24) |
                 (static_cast<quint32>(static_cast<quint8>(buffer.at(1))) << 16) |
                 (static_cast<quint32>(static_cast<quint8>(buffer.at(2))) << 8) |
                 static_cast<quint32>(static_cast<quint8>(buffer.at(3))));

            if (len == 0 || len > kMaxFrameBytes) {
                frames << QString::fromUtf8(buffer);
                buffer.clear();
                break;
            }

            if (buffer.size() < static_cast<int>(4 + len)) {
                break;
            }

            const QByteArray body = buffer.mid(4, len);
            frames << QString::fromUtf8(body);
            buffer.remove(0, 4 + len);
        }
        return frames;
    }

private:
    static const quint32 kMaxFrameBytes = 4 * 1024 * 1024;
};

#endif // LEGACY_PROTOCOL_CODEC_H
