// main_window.h —— 主窗口：无边框 + 自绘标题栏 + 登录/注册/主界面三态。
// 三栏固定宽（左268/右236）+ 中栏拉伸；Console 仅开发者模式可见；Ctrl+Shift+D 切换。
// 未读计数为 UI 本地状态（不改消息模型 Store）。
#pragma once

#include <QMainWindow>
#include <QJsonObject>
#include <QPoint>
#include <QHash>
#include <QPair>

#include "ui/contacts_panel.h" // AlertItem
#include "model/store.h"      // Message（板端告警判定入参）

class Session;
class Store;
class DevMode;
class QStackedWidget;
class QLabel;
class QPushButton;
class LoginView;
class RegisterView;
class ChatView;
class ContactsPanel;
class MembersPanel;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    MainWindow(Session* s, Store* st, DevMode* dev, QWidget* parent = nullptr);
    // UI 预览 hook：注入演示数据并直跳主界面（供 --demo 截图，不联网）
    // openBoard=true 时打开板端会话，供告警卡片目检
    void loadDemoData(bool withMessages = true, bool openBoard = false);

protected:
    void mousePressEvent(QMouseEvent* e) override;
    void mouseMoveEvent(QMouseEvent* e) override;

private:
    void buildTitleBar();
    void rebuildTitleBar();
    void buildMainPage();
    void refreshContacts();
    void refreshGroups();
    void refreshMembers();
    void openConversation(int id, bool isGroup);
    QString senderName(int id) const;
    void refreshAlerts();
    // 板端告警判定：发送者==配置的板端账号 且为私聊；级别从正文 drowsy_level=N 解析
    bool isBoardAlert(const Message& m) const;
    int alertLevelFor(const QString& text) const;

    Session* sess_;
    Store* store_;
    DevMode* dev_;
    QStackedWidget* stack_;
    LoginView* login_;
    RegisterView* reg_;
    ChatView* chat_;
    ContactsPanel* contacts_;
    MembersPanel* members_;
    QWidget* titleBar_;
    QPushButton* consoleBtn_;
    QPoint dragPos_;
    int selfId_ = 0;
    int currentPeer_ = 0;
    bool currentIsGroup_ = false;
    QList<QPair<int, QString>> groups_;
    QHash<int, int> unread_;                       // UI 本地未读
    QHash<QString, QPair<int, qint64>> lastMsg_;   // 会话 -> (lastFrom, lastTs) 用于分组
    QString pendingUser_;                          // 本次登录输入，登录成功后决定是否记住
    QString pendingPass_;
    int boardId_ = 0;                              // 板端账号 id（0=未配置，不归类告警）
    QList<AlertItem> alerts_;                      // 板端告警列表（UI 本地状态）
};