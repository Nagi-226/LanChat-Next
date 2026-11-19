// devmode.h —— 开发者模式开关。默认 false。
// 非开发者模式：Console 按钮/协议日志/数字 id 一律不渲染（向主流社交产品看齐）。
// 开启途径：Ctrl+Shift+D 或改 ini；设置页开关仅在已进入开发者模式时显示（用于关闭）。
#pragma once

#include <QObject>

class DevMode : public QObject {
    Q_OBJECT
public:
    explicit DevMode(QObject* parent = nullptr);
    bool enabled() const { return enabled_; }
    void setEnabled(bool v);

signals:
    void changed(bool enabled);

private:
    bool enabled_ = false;
};