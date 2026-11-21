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

ServerProcess::ServerProcess(QObject* parent) : QObject(parent) {}

ServerProcess::~ServerProcess() { stop(); }

void ServerProcess::ensureRunning(quint16 port,int maxAttachmentMb)
{
    // 探测：已有服务端在跑则复用
    QTcpSocket probe;
    probe.connectToHost("127.0.0.1", port);
    if (probe.waitForConnected(300)) {
        probe.disconnectFromHost();
        return;
    }
    const QString exe = QCoreApplication::applicationDirPath() + "/lanchat_server_next.exe";
    if (!QFile::exists(exe)) return; // 无 sidecar 则不 spawn（用户可自行起服务端）
    QDir().mkpath(unifiedDataDir());
    auto env=QProcessEnvironment::systemEnvironment();env.insert("LANCHAT_ATTACHMENT_MAX_MB",QString::number(maxAttachmentMb));proc_.setProcessEnvironment(env);
    proc_.start(exe, {"--port", QString::number(port), "--data", unifiedDataDir()});
    spawned_ = proc_.waitForStarted(3000);
}

void ServerProcess::stop()
{
    if (spawned_ && proc_.state() != QProcess::NotRunning) {
        proc_.kill();
        proc_.waitForFinished(2000);
    }
    spawned_ = false;
}