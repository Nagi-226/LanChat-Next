#include "app/server_process.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QProcessEnvironment>
#include <QTcpSocket>

namespace {
// 统一数据目录：%APPDATA%/LanChat-Next/data（与客户端 lib.rs 约定一致）
QString unifiedDataDir()
{
    const QString appdata = qEnvironmentVariable("APPDATA");
    if (!appdata.isEmpty()) return appdata + "/LanChat-Next/data";
    return QDir::homePath() + "/.local/share/LanChat-Next/data";
}
}

ServerProcess::ServerProcess(QObject* parent) : QObject(parent) {connect(&proc_,qOverload<int,QProcess::ExitStatus>(&QProcess::finished),this,[this](int code,QProcess::ExitStatus){spawned_=false;state_=QString("本客户端托管的消息主机已退出（%1）。").arg(code);emit stateChanged(state_);});}

ServerProcess::~ServerProcess() { stop(); }

void ServerProcess::ensureRunning(quint16 port,int maxAttachmentMb)
{
    // 探测：已有服务端在跑则复用
    QTcpSocket probe;
    probe.connectToHost("127.0.0.1", port);
    if (probe.waitForConnected(300)) {
        probe.disconnectFromHost();
        state_="本机消息主机已有进程运行；退出此客户端不会停止该进程。";emit stateChanged(state_);
        return;
    }
    const QString exe = QCoreApplication::applicationDirPath() + "/lanchat_server_next.exe";
    if (!QFile::exists(exe)) {state_="未找到配套服务端，无法托管本机主机。";emit stateChanged(state_);return;}
    QDir().mkpath(unifiedDataDir());
    auto env=QProcessEnvironment::systemEnvironment();env.insert("LANCHAT_ATTACHMENT_MAX_MB",QString::number(maxAttachmentMb));proc_.setProcessEnvironment(env);
    proc_.start(exe, {"--port", QString::number(port), "--data", unifiedDataDir()});
    spawned_ = proc_.waitForStarted(3000);
    state_=spawned_?"本客户端托管消息主机；退出程序会停止主机，收起到托盘会继续运行。":"消息主机启动失败。";emit stateChanged(state_);
}

void ServerProcess::stop()
{
    if (spawned_ && proc_.state() != QProcess::NotRunning) {
        proc_.kill();
        proc_.waitForFinished(2000);
    }
    spawned_ = false;
    state_="未托管本机消息主机";emit stateChanged(state_);
}
