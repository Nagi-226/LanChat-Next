#ifndef LEGACY_LOGGER_H
#define LEGACY_LOGGER_H

#include <QString>

class LegacyLogger
{
public:
    static void info(const QString& message);
    static void warn(const QString& message);
    static void error(const QString& message);

private:
    static void write(const QString& level, const QString& message);
};

#endif // LEGACY_LOGGER_H
