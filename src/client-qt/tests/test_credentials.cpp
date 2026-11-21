// test_credentials.cpp —— 记住账户落盘往返测试。
// 判据：save 后 load 能拿回用户名+密码；ini 文件里搜不到明文密码（Windows DPAPI）；
// clear 后 remembered=false。非 Windows 只记用户名（密码为空）。
#include "app/credentials.h"
#include "test_assert.h"

#include <QCoreApplication>
#include <QFile>
#include <QSettings>

int main(int argc, char** argv)
{
    QCoreApplication app(argc, argv);
    // 用独立 org/app 名，测试不碰真实用户的记住账户
    app.setOrganizationName("LanChat-Qt-Test");
    app.setApplicationName("CredentialsTest");

    credentials::clear();
    assert(!credentials::load().remembered);

    const QString user = "Nagi_226";
    const QString pass = "secret-pass-965163";
    credentials::save(user, pass);
    const auto got = credentials::load();
    assert(got.remembered);
    assert(got.username == user);
#ifdef Q_OS_WIN
    assert(got.password == pass);
    // 独立判据：直接读 ini 原文，明文密码不允许出现在里面
    QFile f(QSettings().fileName());
    assert(f.open(QIODevice::ReadOnly));
    const QByteArray raw = f.readAll();
    assert(!raw.contains(pass.toUtf8()));
    assert(raw.contains("login_secret"));
#else
    assert(got.password.isEmpty());
#endif

    credentials::clear();
    const auto gone = credentials::load();
    assert(!gone.remembered);
    assert(gone.username.isEmpty());

    std::printf("CREDENTIALS TEST PASS\n");
    return 0;
}