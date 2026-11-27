// avatar.h —— 确定性渐变圆头像：名字首字母 + 由名字哈希决定的渐变色，
// 同一个人每次颜色一致。自定义 paintEvent 画正圆（QSS 圆角对顶层/裁剪不可靠）。
#pragma once

#include <QWidget>
#include <QPixmap>

class Avatar : public QWidget {
    Q_OBJECT
public:
    explicit Avatar(const QString& name, int diameter, QWidget* parent = nullptr, int avatarId = 0);
    void setName(const QString& name);

protected:
    void paintEvent(QPaintEvent* e) override;

private:
    QString name_;
    int diameter_;
    QPixmap portrait_;
};
