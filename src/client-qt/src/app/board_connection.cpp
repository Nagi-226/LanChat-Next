#include "app/board_connection.h"
#include "app/credentials.h"
#include "protocol/messages.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
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
    cfg_.voiceProvider=s.value("board_bridge/voice_provider","auto").toString();
    cfg_.logEnabled = s.value("board_bridge/log_enabled",false).toBool();
    cfg_.logPort = s.value("board_bridge/log_port","COM3").toString();
    cfg_.aiEnabled = s.value("board_ai/enabled",false).toBool();
    cfg_.aiProvider = s.value("board_ai/provider","anthropic").toString();
    cfg_.aiEndpoint = s.value("board_ai/endpoint",QString()).toString();
    cfg_.aiModel = s.value("board_ai/model",QString()).toString();
    cfg_.aiReasoningEffort = s.value("board_ai/reasoning_effort",QString()).toString();
    cfg_.aiMaxTokens=s.value("board_ai/max_tokens",1200).toInt();cfg_.aiTimeoutSeconds=s.value("board_ai/timeout_seconds",45).toInt();
    cfg_.aiDailyLimit=s.value("board_ai/daily_limit",100).toInt();cfg_.aiIntervalSeconds=s.value("board_ai/interval_seconds",2).toInt();cfg_.aiConcurrency=s.value("board_ai/concurrency",4).toInt();
    cfg_.aiKey = credentials::decryptSecret(s.value("board_ai/secret").toByteArray());
    migrateAiProfiles();
    cfg_.listenPort = static_cast<quint16>(s.value("board_bridge/listen_port", 9000).toUInt());
    cfg_.serverHost = s.value("board_bridge/server_host", "127.0.0.1").toString();
    cfg_.serverPort = static_cast<quint16>(s.value("board_bridge/server_port", 12346).toUInt());
    cfg_.target = s.value("board_bridge/target", 0).toInt();
    cfg_.groupId = s.value("board_bridge/group_id", 0).toInt();
    cfg_.accountId = s.value("board_bridge/account_id", 0).toInt();
    cfg_.password = credentials::decryptSecret(s.value("board_bridge/secret").toByteArray());
    cfg_.gptAccountId = s.value("board_bridge/gpt_account_id",0).toInt();
    cfg_.gptPassword = credentials::decryptSecret(s.value("board_bridge/gpt_secret").toByteArray());
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
        if (!stopping_) {setState(QString("Console 已退出（%1），可重新连接").arg(code));
            if(wantRunning_&&QSettings().value("preferences/bridge_reconnect",true).toBool())QTimer::singleShot(5000,this,[this]{if(wantRunning_)start();});}
    });
    // 消费进程输出，避免长时间运行时缓存增长；状态来自原子文件快照。
    connect(&proc_, &QProcess::readyReadStandardOutput, this, [this] { proc_.readAllStandardOutput(); });
    connect(&proc_, &QProcess::readyReadStandardError, this, [this] { proc_.readAllStandardError(); });
    poll_.setInterval(500);
    connect(&poll_, &QTimer::timeout, this, &BoardConnection::pollStatus);
    logPoll_.setInterval(200);connect(&logPoll_,&QTimer::timeout,this,&BoardConnection::pollLogs);logPoll_.start();pollLogs();
}

BoardConnection::~BoardConnection() { stop(); }

namespace {
QString aiProfilePath(int slot, bool verified)
{
    return QStringLiteral("board_ai_profiles/%1/%2").arg(slot).arg(verified ? "verified" : "draft");
}

QVariantMap profileValues(const BoardConnection::AiProfile& profile, const QByteArray& encrypted)
{
    return {{"provider",profile.provider},{"endpoint",profile.endpoint},{"model",profile.model},
            {"secret",encrypted},{"reasoning_effort",profile.reasoningEffort},{"verified_at",profile.verifiedAt}};
}
}

QString BoardConnection::aiProfileName(int slot)
{
    static const QStringList names{"DeepSeek","GPT","配置 3"};
    return names.value(slot);
}

BoardConnection::AiProfile BoardConnection::aiProfile(int slot, bool verified) const
{
    if (slot < 0 || slot >= AiProfileCount) return {};
    const auto p = QSettings().value(aiProfilePath(slot,verified)).toMap();
    return {p.value("provider",slot == 2 ? "anthropic" : "openai").toString(),
            p.value("endpoint",slot == 1 && !verified ? "https://agentrouter.org/v1" : "").toString(),
            p.value("model",slot == 1 && !verified ? "gpt-6.2-sol" : "").toString(),
            credentials::decryptSecret(p.value("secret").toByteArray()),p.value("reasoning_effort").toString(),
            p.value("verified_at").toLongLong()};
}

bool BoardConnection::saveAiProfile(int slot, const AiProfile& profile, bool verified)
{
    if (slot < 0 || slot >= AiProfileCount) return false;
    const auto secret = profile.key.isEmpty() ? QByteArray() : credentials::encryptSecret(profile.key);
    if (!profile.key.isEmpty() && secret.isEmpty()) return false;
    QSettings settings;
    settings.setValue(aiProfilePath(slot,verified),profileValues(profile,secret));
    settings.sync();
    return settings.status() == QSettings::NoError;
}

void BoardConnection::migrateAiProfiles()
{
    QSettings settings;
    const auto path = aiProfilePath(0,false);
    if (settings.contains(path) || !settings.contains("board_ai/endpoint") || cfg_.aiEndpoint.isEmpty()) return;
    const auto values = profileValues({cfg_.aiProvider,cfg_.aiEndpoint,cfg_.aiModel,cfg_.aiKey,cfg_.aiReasoningEffort},
                                      settings.value("board_ai/secret").toByteArray());
    settings.setValue(path,values);
    if (cfg_.aiEnabled && !cfg_.aiModel.isEmpty() && !cfg_.aiKey.isEmpty() &&
        !settings.contains(aiProfilePath(0,true)))
        settings.setValue(aiProfilePath(0,true),values); // Active legacy baseline, without inventing a verification date.
    settings.sync();
}

void BoardConnection::applyPrimaryAiProfile(const AiProfile& profile)
{
    if (profile.verifiedAt <= 0 || profile.key.isEmpty()) return;
    const bool restart = running();
    auto config = cfg_;
    config.aiProvider=profile.provider;
    config.aiEndpoint=profile.endpoint;
    config.aiModel=profile.model;
    config.aiKey=profile.key;
    config.aiReasoningEffort=profile.reasoningEffort;
    config.aiEnabled=true;
    configure(config);
    if (restart) start();
}

void BoardConnection::applyGptAiProfile()
{
    if (running() && cfg_.gptAccountId > 0 && cfg_.gptAccountId != cfg_.accountId && !cfg_.gptPassword.isEmpty()) {
        stop();
        start();
    }
}

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
    s.setValue("board_bridge/log_enabled",cfg_.logEnabled);
    s.setValue("board_bridge/log_port",cfg_.logPort);
    s.setValue("board_ai/enabled",cfg_.aiEnabled);
    s.setValue("board_ai/provider",cfg_.aiProvider);
    s.setValue("board_ai/endpoint",cfg_.aiEndpoint);
    s.setValue("board_ai/model",cfg_.aiModel);
    s.setValue("board_ai/reasoning_effort",cfg_.aiReasoningEffort);
    s.setValue("board_ai/max_tokens",cfg_.aiMaxTokens);s.setValue("board_ai/timeout_seconds",cfg_.aiTimeoutSeconds);s.setValue("board_ai/daily_limit",cfg_.aiDailyLimit);
    s.setValue("board_ai/interval_seconds",cfg_.aiIntervalSeconds);s.setValue("board_ai/concurrency",cfg_.aiConcurrency);
    s.setValue("board_ai/secret",credentials::encryptSecret(cfg_.aiKey));
    s.setValue("board_bridge/listen_port", cfg_.listenPort);
    s.setValue("board_bridge/server_host", cfg_.serverHost);
    s.setValue("board_bridge/server_port", cfg_.serverPort);
    s.setValue("board_bridge/target", cfg_.target);
    s.setValue("board_bridge/group_id", cfg_.groupId);
    s.setValue("board_bridge/account_id", cfg_.accountId);
    s.setValue("board_bridge/secret", credentials::encryptSecret(cfg_.password));
    s.setValue("board_bridge/gpt_account_id",cfg_.gptAccountId);
    s.setValue("board_bridge/gpt_secret",credentials::encryptSecret(cfg_.gptPassword));
    s.setValue("board_bridge/console_path", cfg_.consolePath);
    s.setValue("board_bridge/voice_provider",cfg_.voiceProvider);
    if (cfg_.accountId > 0) s.setValue("board_account_id", cfg_.accountId);
    s.sync();
}

void BoardConnection::start()
{
    wantRunning_=true;
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
    QFile::remove(profileDir_ + "/logs.request");
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
    env.insert("ROBOMIND_TTS_PROVIDER",cfg_.voiceProvider);
    env.insert("ROBOMIND_LANCHAT_USER_ID", QString::number(cfg_.accountId));
    env.insert("ROBOMIND_LANCHAT_PASSWORD", cfg_.password);
    env.insert("ROBOMIND_AI_ENABLED",cfg_.aiEnabled?"1":"0");
    env.insert("ROBOMIND_AI_PROVIDER",cfg_.aiProvider);
    env.insert("ROBOMIND_AI_ENDPOINT",cfg_.aiEndpoint);
    env.insert("ROBOMIND_AI_MODEL",cfg_.aiModel);
    env.insert("ROBOMIND_AI_REASONING_EFFORT",cfg_.aiReasoningEffort);
    env.insert("ROBOMIND_AI_KEY",cfg_.aiKey);
    env.insert("ROBOMIND_AI_MAX_TOKENS",QString::number(cfg_.aiMaxTokens));env.insert("ROBOMIND_AI_TIMEOUT_SECONDS",QString::number(cfg_.aiTimeoutSeconds));
    env.insert("ROBOMIND_AI_DAILY_LIMIT",QString::number(cfg_.aiDailyLimit));env.insert("ROBOMIND_AI_INTERVAL_SECONDS",QString::number(cfg_.aiIntervalSeconds));
    env.insert("ROBOMIND_AI_CONCURRENCY",QString::number(cfg_.aiConcurrency));env.insert("ROBOMIND_AI_USAGE_FILE",profileDir_+"/ai-usage.json");
    for (const auto& name : env.keys())
        if (name.startsWith("ROBOMIND_GPT_")) env.remove(name);
    const auto gpt = aiProfile(1,true);
    if (cfg_.gptAccountId > 0 && cfg_.gptAccountId != cfg_.accountId && !cfg_.gptPassword.isEmpty()
        && gpt.verifiedAt > 0 && !gpt.endpoint.isEmpty() && !gpt.model.isEmpty() && !gpt.key.isEmpty()) {
        env.insert("ROBOMIND_GPT_ENABLED","1");
        env.insert("ROBOMIND_GPT_USER_ID",QString::number(cfg_.gptAccountId));
        env.insert("ROBOMIND_GPT_PASSWORD",cfg_.gptPassword);
        env.insert("ROBOMIND_GPT_PROVIDER",gpt.provider);
        env.insert("ROBOMIND_GPT_ENDPOINT",gpt.endpoint);
        env.insert("ROBOMIND_GPT_MODEL",gpt.model);
        env.insert("ROBOMIND_GPT_REASONING_EFFORT",gpt.reasoningEffort);
        env.insert("ROBOMIND_GPT_KEY",gpt.key);
        env.insert("ROBOMIND_GPT_USAGE_FILE",profileDir_+"/gpt-ai-usage.json");
    }
    proc_.setProcessEnvironment(env);
    proc_.setWorkingDirectory(profileDir_);
    QStringList arguments={"--bridge", "--port", QString::number(cfg_.listenPort),
        "--status-file", profileDir_ + "/status.json", "--stop-file", profileDir_ + "/stop.request"};
    if(cfg_.logEnabled)arguments << "--log-port" << cfg_.logPort;
    proc_.start(cfg_.consolePath,arguments);
    status_ = {};
    defaultPageApplied_=false;
    setState("正在启动 Console 桥…");
    poll_.start();
}

void BoardConnection::stop()
{
    wantRunning_=false;
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
    pollLogs();emit logsChanged();
}

void BoardConnection::applyPreferences(){defaultPageApplied_=false;pollStatus();}
bool BoardConnection::localVoiceAvailable()const{
    QFile file(QFileInfo(cfg_.consolePath).absolutePath()+"/voice-backend.json");
    if(!file.open(QIODevice::ReadOnly)||file.size()>16384)return false;
    const auto data=QJsonDocument::fromJson(file.readAll()).object();
    for(const QString key:{"python","worker","runtime"}){const auto path=data.value(key).toString();if(!QFileInfo(path).isAbsolute()||!QFile::exists(path))return false;}
    return QFile::exists(data.value("runtime").toString()+"/voice.json");
}
bool BoardConnection::setVoiceProvider(const QString& provider){
    if(!QStringList{"auto","windows","local_vits"}.contains(provider)||(provider=="local_vits"&&!localVoiceAvailable()))return false;
    if(provider==cfg_.voiceProvider)return true;
    const bool restart=running();auto config=cfg_;config.voiceProvider=provider;configure(config);if(restart)start();return true;
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
    const auto display=object.value("display").toObject();
    if(!object.value("board_connected").toBool())defaultPageApplied_=false;
    const auto defaultPage=QSettings().value("preferences/board_default_page",QString()).toString();
    if(!defaultPageApplied_&&display.value("terminal_ready").toBool()&&display.value("navigation").toBool()){
        if(defaultPage.isEmpty()||navigate(defaultPage))defaultPageApplied_=true;
    }
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
bool BoardConnection::setLogCapture(bool enabled,const QString& port){
    if(enabled&&(port.trimmed().isEmpty()||port.size()>64))return false;
    cfg_.logEnabled=enabled;cfg_.logPort=port.trimmed();saveConfig();
    if(!running()){if(enabled)start();return enabled;}
    QSaveFile file(profileDir_+"/logs.request");if(!file.open(QIODevice::WriteOnly))return false;
    file.write(QJsonDocument(QJsonObject{{"action",enabled?"start":"stop"},{"port",cfg_.logPort},{"created_ms",QDateTime::currentMSecsSinceEpoch()}}).toJson(QJsonDocument::Compact));
    return file.commit();
}
void BoardConnection::pollLogs(){
    const QString path=profileDir_+"/logs/logs.snapshot.json";const auto modified=QFileInfo(path).lastModified().toMSecsSinceEpoch();if(modified==logFileModified_)return;
    QFile file(path);if(file.size()>12*1024*1024||!file.open(QIODevice::ReadOnly))return;
    const auto object=QJsonDocument::fromJson(file.readAll()).object();const auto updated=object.value("status").toObject().value("updated_ms").toInteger();
    if(object.isEmpty()||updated==logUpdated_)return;
    logFileModified_=modified;logUpdated_=updated;logSnapshot_=object;emit logsChanged();
}
bool BoardConnection::navigate(const QString& action,QString* requestId)
{
    const auto display=snapshot().value("display").toObject();
    const QStringList allowed={"home","inbox","ppm","link","device","detail","next","previous","read","scroll_down","scroll_up","lang_zh","lang_en","immersion","immersion_dragon","terminal","audio_test","stop_speech","volume_10","volume_20","volume_30","volume_40","volume_50"};
    if(!display.value("terminal_ready").toBool()||!display.value("navigation").toBool()||!allowed.contains(action)||(action=="immersion_dragon"&&!display.value("dragon_supported").toBool())||QFile::exists(profileDir_+"/navigation.request"))return false;
    QSaveFile file(profileDir_+"/navigation.request");
    if(!file.open(QIODevice::WriteOnly))return false;
    const QString id=QUuid::createUuid().toString(QUuid::Id128);
    file.write(QJsonDocument(QJsonObject{{"id",id},{"action",action},{"created_ms",QDateTime::currentMSecsSinceEpoch()}}).toJson(QJsonDocument::Compact));
    const bool saved=file.commit();if(saved&&requestId)*requestId=id;return saved;
}
