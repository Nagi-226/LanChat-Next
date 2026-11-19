// test_devmode.cpp —— 开发者模式开关单测。
// 用 IniFormat + 临时目录重定向 QSettings，避免污染用户真实配置。
#include "app/devmode.h"
#include "test_assert.h"

#include <QCoreApplication>
#include <QSettings>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <iostream>

int main(int argc, char** argv)
{
    QCoreApplication app(argc, argv);
    QTemporaryDir tmp;
    assert(tmp.isValid());
    QSettings::setDefaultFormat(QSettings::IniFormat);
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, tmp.path());

    DevMode d;
    assert(!d.enabled()); // 默认 false

    QSignalSpy spy(&d, &DevMode::changed);
    d.setEnabled(true);
    assert(d.enabled());
    assert(spy.count() == 1);
    assert(spy.last().at(0).toBool() == true);

    // 落盘
    QSettings s;
    assert(s.value("developer_mode").toBool() == true);

    // 重复 set 同值不发信号
    d.setEnabled(true);
    assert(spy.count() == 1);

    d.setEnabled(false);
    assert(!d.enabled());
    assert(spy.count() == 2);

    std::cout << "devmode tests passed\n";
    return 0;
}