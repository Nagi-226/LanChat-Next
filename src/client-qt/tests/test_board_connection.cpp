#include "app/board_connection.h"
#include "test_assert.h"
#include <QCoreApplication>
#include <QDateTime>
#include <QElapsedTimer>
#include <QFile>
#include <QJsonDocument>
#include <QSaveFile>
#include <QSettings>
#include <QTcpServer>
#include <QTemporaryDir>
#include <QThread>
#include <QtEndian>
#include <functional>
#include <memory>

static bool waitFor(const std::function<bool()>& check)
{
    QElapsedTimer timer;
    timer.start();
    while (timer.elapsed() < 6000) {
        QCoreApplication::processEvents();
        if (check()) return true;
        QThread::msleep(10);
    }
    return false;
}

int main(int argc, char** argv)
{
    QCoreApplication app(argc, argv);
    if (app.arguments().contains("--bridge")) {
        const auto arguments = app.arguments();
        const QString status = arguments.value(arguments.indexOf("--status-file") + 1);
        const QString stop = arguments.value(arguments.indexOf("--stop-file") + 1);
        QTimer timer;
        QObject::connect(&timer, &QTimer::timeout, &app, [&] {
            if (QFile::exists(stop)) { app.quit(); return; }
            QSaveFile file(status);
            if (!file.open(QIODevice::WriteOnly)) qFatal("fake Console status open failed");
            const qint64 timestamp = QDateTime::currentMSecsSinceEpoch()
                - (qEnvironmentVariableIsSet("BOARD_TEST_STALE") ? 10000 : 0);
            file.write(QJsonDocument(QJsonObject{{"updated_ms", timestamp},
                {"listening", true}, {"board_connected", true}, {"frames", 42},
                {"board_peer", "192.0.2.3"}, {"decode_failures", 0},
                {"display",QJsonObject{{"terminal_ready",true},{"navigation",true}}},
                {"lanchat_state", "已登录"}}).toJson());
            if (!file.commit()) qFatal("fake Console status commit failed");
        });
        timer.start(100);
        return app.exec();
    }
    QTemporaryDir dir;
    assert(dir.isValid());
    app.setOrganizationName("LanChat-Qt-Test");
    app.setApplicationName("BoardConnectionTest");
    QSettings::setDefaultFormat(QSettings::IniFormat);
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, dir.path());
    QSettings().clear();
    QTcpServer server;
    assert(server.listen(QHostAddress::LocalHost, 0));
    QString registeredPassword;
    int registrations = 0;
    QObject::connect(&server, &QTcpServer::newConnection, [&] {
        auto* socket = server.nextPendingConnection();
        auto buffer = std::make_shared<QByteArray>();
        QObject::connect(socket, &QTcpSocket::readyRead, socket, [&, socket, buffer] {
            buffer->append(socket->readAll());
            if (buffer->size() < 4) return;
            const auto size = qFromBigEndian<quint32>(reinterpret_cast<const uchar*>(buffer->constData()));
            if (buffer->size() < 4 + static_cast<int>(size)) return;
            const auto request = QJsonDocument::fromJson(buffer->mid(4, size)).object();
            if (request.value("type").toInt() != 0) qFatal("expected registration request");
            registeredPassword = request.value("password").toString();
            if (registeredPassword.size() != 32) qFatal("invalid generated password");
            if (!request.value("nickname").toString().startsWith("RoboMind-S3-")) qFatal("invalid board nickname");
            ++registrations;
            const auto reply = QJsonDocument(QJsonObject{{"type", 1}, {"status", "ok"}, {"id", 4242}}).toJson();
            QByteArray frame(4, 0);
            qToBigEndian<quint32>(reply.size(), reinterpret_cast<uchar*>(frame.data()));
            socket->write(frame + reply);
        });
        QObject::connect(socket, &QTcpSocket::disconnected, socket, &QObject::deleteLater);
    });
    BoardConnection bridge(nullptr, dir.path() + "/profile");
    auto config = bridge.config();
    config.consolePath = app.applicationFilePath();
    config.serverPort = server.serverPort();
    config.target = 55;
    config.groupId = 9;
    config.aiEnabled=true;config.aiEndpoint="https://example.com/anthropic";config.aiModel="test-model";config.aiKey="isolated-ai-secret";config.aiReasoningEffort="none";
    bridge.configure(config);
    QSettings().setValue("preferences/board_default_page","ppm");
    bridge.start();
    assert(waitFor([&] { return bridge.snapshot().value("frames").toInt() == 42; }));
    assert(registrations == 1);
    assert(bridge.config().accountId == 4242);
    assert(bridge.snapshot().value("board_connected").toBool());
    const auto navigation=bridge.profileDirectory()+"/navigation.request";assert(waitFor([&]{return QFile::exists(navigation);}));QFile nav(navigation);assert(nav.open(QIODevice::ReadOnly));assert(QJsonDocument::fromJson(nav.readAll()).object().value("action").toString()=="ppm");nav.close();assert(QFile::remove(navigation));QThread::msleep(650);QCoreApplication::processEvents();assert(!QFile::exists(navigation));
    QSettings generated(bridge.profileDirectory() + "/robomind-console.ini", QSettings::IniFormat);
    assert(generated.value("lanchat/group_id").toInt() == 9);
    assert(generated.value("lanchat/target").toInt() == 55);
    QFile ini(generated.fileName());
    assert(ini.open(QIODevice::ReadOnly));
    const auto iniBytes=ini.readAll();assert(!iniBytes.contains(registeredPassword.toUtf8())&&!iniBytes.contains("isolated-ai-secret"));
    ini.close();
    QFile settings(QSettings().fileName());
    assert(settings.open(QIODevice::ReadOnly));
    assert(!settings.readAll().contains(registeredPassword.toUtf8()));
    settings.close();
    bridge.stop();
    assert(!bridge.running());
    assert(bridge.snapshot().isEmpty());
#ifdef Q_OS_WIN
    BoardConnection restored(nullptr, dir.path() + "/restored");
    assert(restored.config().accountId == 4242);
    assert(restored.config().password == registeredPassword);
    assert(restored.config().aiEnabled&&restored.config().aiModel=="test-model"&&restored.config().aiKey=="isolated-ai-secret"&&restored.config().aiReasoningEffort=="none");
#endif
    bridge.start();
    const bool restarted = waitFor([&] { return bridge.snapshot().value("frames").toInt() == 42; });
    if (!restarted) std::fprintf(stderr, "bridge state: %s\n", qPrintable(bridge.state()));
    assert(restarted);
    assert(registrations == 1);
    bridge.stop();
    qputenv("BOARD_TEST_STALE", "1");
    bridge.start();
    assert(waitFor([&] { return bridge.state() == "Console 状态超时，等待恢复"; }));
    assert(bridge.snapshot().isEmpty());
    bridge.stop();
    qunsetenv("BOARD_TEST_STALE");
    std::printf("BOARD CONNECTION PASS: provision, encrypted persistence, dual targets, status, stop, restart\n");
    return 0;
}
