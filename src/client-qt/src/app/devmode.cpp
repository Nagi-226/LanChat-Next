#include "app/devmode.h"

#include <QSettings>

DevMode::DevMode(QObject* parent) : QObject(parent)
{
    QSettings s;
    enabled_ = s.value("developer_mode", false).toBool();
}

void DevMode::setEnabled(bool v)
{
    if (v == enabled_) return;
    enabled_ = v;
    QSettings s;
    s.setValue("developer_mode", v);
    emit changed(v);
}