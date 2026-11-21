// server_process.h —— 管理 C++ 服务端子进程。
// 启动时探测 12346：已有服务端在跑就复用（不重复 spawn）；否则 spawn exe 旁 sidecar，
// --data 指向统一 per-user 目录（与 Tauri 版 lib.rs 约定一致），退出时 kill。
#pragma once

#include <QObject>
#include <QProcess>

class ServerProcess : public QObject {
    Q_OBJECT
public:
    explicit ServerProcess(QObject* parent = nullptr);
    ~ServerProcess() override;
    void ensureRunning(quint16 port=12346,int maxAttachmentMb=100);
    void stop();

private:
    QProcess proc_;
    bool spawned_ = false;
};