#include "app/board_connection.h"
#include "app/credentials.h"
#include "protocol/messages.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QProcessEnvironment>
#include <QSaveFile>
#include <QSettings>
#include <QStandardPaths>
#include <QUuid>

BoardConnection::BoardConnection(QObject* parent, const QString& profileDir) : QObject(parent)
{
    profileDir_ = profileDir.isEmpty()
        ? QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation) + "/board-bridge"
        : profileDir;
    QSettings s;
    cfg_.autoStart = s.value("board_bridge/auto_start", false).toBool();
    cfg_.listenPort = static_cast<quint16>(s.value("board_bridge/listen_port", 9000).toUInt());
    cfg_.serverHost = s.value("board_bridge/server_host", "127.0.0.1").toString();
    cfg_.serverPort = static_cast<quint16>(s.value("board_bridge/server_port", 12346).toUInt());
    cfg_.target = s.value("board_bridge/target", 0).toInt();
    cfg_.groupId = s.value("board_bridge/group_id", 0).toInt();
    cfg_.accountId = s.value("board_bridge/account_id", 0).toInt();
    cfg_.password = credentials::decryptSecret(s.value("board_bridge/secret").toByteArray());
    cfg_.consolePath = s.value("board_bridge/console_path",
        QCoreApplication::applicationDirPath() + "/robomind-console/RoboMindConsole.exe").toString();
    registrationTimeout_.setSingleShot(true);
    registrationTimeout_.setInterval(8000);
    connect(&registrationTimeout_, &QTimer::timeout, this, [this] {
        registering_ = false;
        registration_.abort();
        setState("创建板端账号超时，请检查 LanChat 服务端");
    });
    connect(&registration_, &QTcpSocket::connected, this, [this] {
        const QString nickname = "RoboMind-S3-" + QUuid::createUuid().toString(QUuid::Id128).left(8);
        registration_.write(protocol::encode(protocol::registerUser(nickname, cfg_.password)));
    });
    connect(&registration_, &QTcpSocket::readyRead, this, &BoardConnection::readRegistration);
    connect(&registration_, &QTcpSocket::errorOccurred, this, [this] {
        if (!registering_) return;
        registering_ = false;
        registrationTimeout_.stop();
        setState("LanChat 连接失败：" + registration_.errorString());
    });
    connect(&proc_, &QProcess::errorOccurred, this, [this](QProcess::ProcessError error) {
        if (!stopping_ && error == QProcess::FailedToStart) setState("Console 启动失败：" + proc_.errorString());
    });
    connect(&proc_, qOverload<int, QProcess::ExitStatus>(&QProcess::finished), this,
            [this](int code, QProcess::ExitStatus) {
        status_ = {};
        poll_.stop();
        if (!stopping_) setState(QString("Console 已退出（%1），可重新连接").arg(code));
    });
    // 消费进程输出，避免长时间运行时缓存增长；状态来自原子文件快照。
    connect(&proc_, &QProcess::readyReadStandardOutput, this, [this] { proc_.readAllStandardOutput(); });
    connect(&proc_, &QProcess::readyReadStandardError, this, [this] { proc_.readAllStandardError(); });
    poll_.setInterval(500);
    connect(&poll_, &QTimer::timeout, this, &BoardConnection::pollStatus);
}

BoardConnection::~BoardConnection() { stop(); }

void BoardConnection::configure(const Config& config)
{
    stop();
    cfg_ = config;
    saveConfig();
    emit accountChanged(cfg_.accountId);
    emit changed();
}

void BoardConnection::saveConfig()
{
    QSettings s;
    s.setValue("board_bridge/auto_start", cfg_.autoStart);
    s.setValue("board_bridge/listen_port", cfg_.listenPort);
    s.setValue("board_bridge/server_host", cfg_.serverHost);
    s.setValue("board_bridge/server_port", cfg_.serverPort);
    s.setValue("board_bridge/target", cfg_.target);
    s.setValue("board_bridge/group_id", cfg_.groupId);
    s.setValue("board_bridge/account_id", cfg_.accountId);
    s.setValue("board_bridge/secret", credentials::encryptSecret(cfg_.password));
    s.setValue("board_bridge/console_path", cfg_.consolePath);
    if (cfg_.accountId > 0) s.setValue("board_account_id", cfg_.accountId);
    s.sync();
}

void BoardConnection::start()
{
    if (running()) return;
    if (!QFile::exists(cfg_.consolePath)) { setState("缺少 Console 桥，请重新部署或选择 Console 程序"); return; }
    if (cfg_.listenPort == 0 || cfg_.serverPort == 0 || cfg_.target < 0 || cfg_.groupId < 0) {
        setState("连接配置无效，请检查端口和告警目标"); return;
    }
    if (cfg_.accountId > 0 && !cfg_.password.isEmpty()) { launchConsole(); return; }
    // 专用账号避免第二个连接覆盖聊天账号的服务端会话路由。
    cfg_.password = QUuid::createUuid().toString(QUuid::Id128);
    registering_ = true;
    registrationFrames_.reset();
    setState("正在创建专用板端账号…");
    registrationTimeout_.start();
    registration_.connectToHost(cfg_.serverHost, cfg_.serverPort);
}

void BoardConnection::readRegistration()
{
    registrationFrames_.feed(registration_.readAll());
    QByteArray payload;
    while (registering_) {
        const auto result = registrationFrames_.next(payload);
        if (result == FrameBuffer::Status::Incomplete) return;
        if (result == FrameBuffer::Status::BadHeader) { stop(); setState("板端账号注册回包损坏"); return; }
        QJsonObject reply;
        QString error;
        if (!protocol::decode(payload, reply, error) || reply.value("type").toInt(-1) != 1) continue;
        registering_ = false;
        registrationTimeout_.stop();
        registration_.disconnectFromHost();
        if (reply.value("status").toString() != "ok" || reply.value("id").toInt() < 1) {
            setState("创建板端账号失败：" + reply.value("msg").toString()); return;
        }
        cfg_.accountId = reply.value("id").toInt();
        saveConfig();
        emit accountChanged(cfg_.accountId);
        launchConsole();
    }
}

void BoardConnection::launchConsole()
{
    QDir().mkpath(profileDir_);
    QFile::remove(profileDir_ + "/stop.request");
    QFile::remove(profileDir_ + "/status.json");
    QFile::remove(profileDir_ + "/navigation.request");
    {
        QSettings ini(profileDir_ + "/robomind-console.ini", QSettings::IniFormat);
        ini.clear();
        ini.beginGroup("lanchat");
        ini.setValue("enabled", true);
        ini.setValue("host", cfg_.serverHost);
        ini.setValue("port", cfg_.serverPort);
        ini.setValue("target", cfg_.target);
        ini.setValue("group_id", cfg_.groupId);
        ini.setValue("receive_messages", true);
        ini.sync();
        if (ini.status() != QSettings::NoError) { setState("无法写入板端桥配置"); return; }
    }
    auto env = QProcessEnvironment::systemEnvironment();
    env.insert("ROBOMIND_LANCHAT_USER_ID", QString::number(cfg_.accountId));
    env.insert("ROBOMIND_LANCHAT_PASSWORD", cfg_.password);
    proc_.setProcessEnvironment(env);
    proc_.setWorkingDirectory(profileDir_);
    proc_.start(cfg_.consolePath, {"--bridge", "--port", QString::number(cfg_.listenPort),
        "--status-file", profileDir_ + "/status.json", "--stop-file", profileDir_ + "/stop.request"});
    status_ = {};
    setState("正在启动 Console 桥…");
    poll_.start();
}

void BoardConnection::stop()
{
    stopping_ = true;
    registering_ = false;
    registrationTimeout_.stop();
    registration_.abort();
    poll_.stop();
    if (proc_.state() != QProcess::NotRunning) {
        QSaveFile stopFile(profileDir_ + "/stop.request");
        if (stopFile.open(QIODevice::WriteOnly)) { stopFile.write("stop\n"); stopFile.commit(); }
        if (!proc_.waitForFinished(1500)) { proc_.kill(); proc_.waitForFinished(1000); }
    }
    stopping_ = false;
    status_ = {};
    setState("未启动");
}

void BoardConnection::setDefaultRecipient(int uid)
{
    if (uid < 1 || cfg_.target > 0 || cfg_.groupId > 0) return;
    const bool restart = running();
    auto config = cfg_;
    config.target = uid;
    configure(config);
    if (restart) start();
}

void BoardConnection::pollStatus()
{
    QFile file(profileDir_ + "/status.json");
    if (!file.open(QIODevice::ReadOnly)) return;
    const auto object = QJsonDocument::fromJson(file.readAll()).object();
    if (object.isEmpty()) return;
    if (QDateTime::currentMSecsSinceEpoch() - object.value("updated_ms").toInteger() > 2500) {
        status_ = {};
        setState("Console 状态超时，等待恢复");
        return;
    }
    status_ = object;
    if (!object.value("listening").toBool()) setState("监听失败：" + object.value("listen_error").toString());
    else setState(object.value("board_connected").toBool() ? "板端已连接" : "正在监听，等待板端");
    emit changed();
}

QJsonObject BoardConnection::snapshot() const
{
    if (proc_.state() == QProcess::NotRunning) return {};
    if (QDateTime::currentMSecsSinceEpoch() - status_.value("updated_ms").toInteger() > 2500) return {};
    return status_;
}

void BoardConnection::setState(const QString& state) { state_ = state; emit changed(); }
bool BoardConnection::navigate(const QString& action,QString* requestId)
{
    const auto display=snapshot().value("display").toObject();
    const QStringList allowed={"home","inbox","ppm","link","device","detail","next","previous","read","scroll_down","scroll_up","lang_zh","lang_en"};
    if(!display.value("terminal_ready").toBool()||!display.value("navigation").toBool()||!allowed.contains(action)||QFile::exists(profileDir_+"/navigation.request"))return false;
    QSaveFile file(profileDir_+"/navigation.request");
    if(!file.open(QIODevice::WriteOnly))return false;
    const QString id=QUuid::createUuid().toString(QUuid::Id128);
    file.write(QJsonDocument(QJsonObject{{"id",id},{"action",action},{"created_ms",QDateTime::currentMSecsSinceEpoch()}}).toJson(QJsonDocument::Compact));
    const bool saved=file.commit();if(saved&&requestId)*requestId=id;return saved;
}
