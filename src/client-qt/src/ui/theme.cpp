#include "ui/theme.h"

#include <QFile>
#include <QTextStream>

namespace theme {

QString stylesheet()
{
    // 设计令牌 QSS 编进 qrc（自包含，无外部文件依赖）
    QFile f(":/styles/lanchat-dark.qss");
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) return QString();
    QTextStream ts(&f);
    return ts.readAll();
}

} // namespace theme