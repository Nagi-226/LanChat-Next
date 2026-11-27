#include "ui/avatar.h"

#include <QPainter>
#include <QPainterPath>
#include <QLinearGradient>
#include <QFont>
#include <QImageReader>
#include <QStandardPaths>

namespace {
// 名字哈希 → 色相；同一名字恒定 → 同人同色
quint32 hashName(const QString& s)
{
    quint32 h = 2166136261u;
    for (const QChar c : s) {
        h ^= c.unicode();
        h *= 16777619u;
    }
    return h;
}
}

Avatar::Avatar(const QString& name, int diameter, QWidget* parent, int avatarId)
    : QWidget(parent), name_(name), diameter_(diameter)
{
    setFixedSize(diameter, diameter);
    setAttribute(Qt::WA_TranslucentBackground);
    if (avatarId == 6201 || avatarId == 6202) {
        const auto folder = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
        QImageReader reader(folder + QStringLiteral("/avatars/%1.png").arg(avatarId), "png");
        const auto size = reader.size();
        if (size.isValid() && size.width() == size.height() && size.width() <= 512)
            portrait_ = QPixmap::fromImage(reader.read());
    }
}

void Avatar::setName(const QString& name)
{
    name_ = name;
    update();
}

void Avatar::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    QPainterPath path;
    path.addEllipse(0, 0, diameter_, diameter_);
    if (!portrait_.isNull()) {
        p.setRenderHint(QPainter::SmoothPixmapTransform);
        p.setClipPath(path);
        p.drawPixmap(rect(), portrait_);
        return;
    }
    const quint32 h = hashName(name_);
    const qreal hue1 = (h % 360);
    const qreal hue2 = fmod(hue1 + 40.0, 360.0);
    QLinearGradient g(0, 0, diameter_, diameter_);
    g.setColorAt(0, QColor::fromHsvF(hue1 / 360.0, 0.55, 0.75));
    g.setColorAt(1, QColor::fromHsvF(hue2 / 360.0, 0.60, 0.55));
    p.fillPath(path, g);
    // 首字母（中文取首字）
    const QString initial = name_.isEmpty() ? QStringLiteral("?") : name_.left(1).toUpper();
    QFont f = font();
    f.setPixelSize(int(diameter_ * 0.42));
    f.setWeight(QFont::DemiBold);
    p.setFont(f);
    p.setPen(QColor("#ffffff"));
    p.drawText(rect(), Qt::AlignCenter, initial);
}
