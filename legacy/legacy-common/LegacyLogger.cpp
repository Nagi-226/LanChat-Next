#include "LegacyLogger.h"

#include <QDateTime>
#include <QDebug>
#include <QFile>
#include <QMutex>
#include <QMutexLocker>
#include <QTextStream>

namespace {
QMutex& logMutex()
{
    static QMutex mutex;
    return mutex;
}
}

void LegacyLogger::info(const QString& message)
{
    write("INFO", message);
}

void LegacyLogger::warn(const QString& message)
{
    write("WARN", message);
}

void LegacyLogger::error(const QString& message)
{
    write("ERROR", message);
}

void LegacyLogger::write(const QString& level, const QString& message)
{
    QMutexLocker locker(&logMutex());
    const QString line = QString("[%1] [%2] %3")
        .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss.zzz"))
        .arg(level)
        .arg(message);

    QFile file("server.log");
    if (file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        QTextStream stream(&file);
        stream.setCodec("UTF-8");
        stream << line << "\n";
    }
    qDebug().noquote() << line;
}
