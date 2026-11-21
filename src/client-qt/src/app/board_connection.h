#pragma once

#include <QObject>
#include <QProcess>
#include <QTcpSocket>
#include <QTimer>
#include <QJsonObject>
#include "net/frame_buffer.h"

// 管理现有 Console 进程，不在聊天客户端重做 VVOS 解码或 PPM 判定。
class BoardConnection : public QObject {
    Q_OBJECT
public:
    struct Config {
        bool autoStart = false;
        quint16 listenPort = 9000;
        QString serverHost = "127.0.0.1";
        quint16 serverPort = 12346;
        int target = 0;
        int groupId = 0;
        int accountId = 0;
        QString password;
        QString consolePath;
    };
    explicit BoardConnection(QObject* parent = nullptr, const QString& profileDir = {});
    ~BoardConnection() override;
    const Config& config() const { return cfg_; }
    void configure(const Config& config);
    void start();
    void stop();
    void setDefaultRecipient(int uid);
    bool running() const { return proc_.state() != QProcess::NotRunning || registering_; }
    QString state() const { return state_; }
    QJsonObject snapshot() const;
    QString profileDirectory() const { return profileDir_; }
    bool navigate(const QString& action,QString* requestId=nullptr);
signals:
    void changed();
    void accountChanged(int uid);
private:
    void saveConfig();
    void launchConsole();
    void readRegistration();
    void pollStatus();
    void setState(const QString& state);
    Config cfg_;
    QString profileDir_;
    QString state_ = "未启动";
    QJsonObject status_;
    QProcess proc_;
    QTcpSocket registration_;
    FrameBuffer registrationFrames_;
    QTimer registrationTimeout_;
    QTimer poll_;
    bool registering_ = false;
    bool stopping_ = false;
};
