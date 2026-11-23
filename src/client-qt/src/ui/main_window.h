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

class TransferManager;
class Session;
class Store;
class DevMode;
class BoardConnection;
class BoardConnectionPanel;
class QStackedWidget;
class QLabel;
class QPushButton;
class LoginView;
class RegisterView;
class ChatView;
class ContactsPanel;
class MembersPanel;
class SettingsPanel;
class QSystemTrayIcon;
class QCloseEvent;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    MainWindow(Session* s, Store* st, DevMode* dev, QWidget* parent = nullptr);
    ~MainWindow() override;
    // UI 预览 hook：注入演示数据并直跳主界面（供 --demo 截图，不联网）
    // openBoard=true 时打开板端会话，供告警卡片目检
    void loadDemoData(bool withMessages = true, bool openBoard = false);
    void showBoardConnection();
    void setHostState(const QString& state);

signals:
    void connectionRequested(const QString& host,quint16 port,bool hosted,int maxAttachmentMb);

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;
    void showEvent(QShowEvent* event) override;
    void changeEvent(QEvent* event) override;
    bool nativeEvent(const QByteArray& eventType, void* message, qintptr* result) override;
    void closeEvent(QCloseEvent* event) override;

private:
    void showConnectionSettings();
    void showSettings();
    void ensureSettingsPage();
    void showExtendedSettings(int section);
    void logoutAccount();
    void applyPreferences();
    void notify(const QString& title,const QString& text,bool test=false);
    bool configureLoginConnection(const QString& address);
    void changeConnection(const QString& host,quint16 port,bool hosted,int maxAttachmentMb);
    void renderConversation();
    void renderMessage(const Message& m,bool grouped,int level);
    void refreshTransfers();
    void buildTitleBar();
    void rebuildTitleBar();
    void updateWindowControls();
    void toggleMaximized();
    void configureNativeFrame();
    void setMaximizeHover(bool hovered);
    void setCloseHover(bool hovered);
    void buildMainPage();
    void refreshContacts();
    void refreshGroups();
    void refreshMembers();
    void openConversation(int id, bool isGroup);
    QString senderName(int id) const;
    void refreshAlerts();
    // 板端告警判定：发送者==配置的板端账号，支持私聊及群；级别从正文解析
    bool isBoardAlert(const Message& m) const;
    int alertLevelFor(const QString& text) const;

    bool transferUpdatePending_=false;
    TransferManager* transfers_;
    QHash<int,QList<int>> groupMembers_;
    QHash<QString,QString> historyCursors_;
    Session* sess_;
    Store* store_;
    DevMode* dev_;
    BoardConnection* bridge_;
    BoardConnectionPanel* boardPanel_=nullptr;
    SettingsPanel* extendedSettings_=nullptr;
    QSystemTrayIcon* tray_=nullptr;
    QString hostState_="消息主机状态待确认";
    bool quitting_=false;
    QWidget* settingsPage_=nullptr;
    QStackedWidget* settingsStack_=nullptr;
    QWidget* serverPage_=nullptr;
    QPushButton* serverSettingsBtn_=nullptr;
    QPushButton* boardSettingsBtn_=nullptr;
    QStackedWidget* stack_;
    LoginView* login_;
    RegisterView* reg_;
    ChatView* chat_;
    ContactsPanel* contacts_;
    MembersPanel* members_;
    QWidget* titleBar_=nullptr;
    QPushButton* consoleBtn_;
    QPushButton* maximizeBtn_=nullptr;
    QPushButton* closeBtn_=nullptr;
    quintptr nativeFrameId_=0;
    bool maximizePressed_=false;
    int selfId_ = 0;
    int currentPeer_ = 0;
    bool currentIsGroup_ = false;
    QList<QPair<int, QString>> groups_;
    QHash<QString, int> unread_;                       // UI 本地未读
    QHash<QString, QPair<int, qint64>> lastMsg_;   // 会话 -> (lastFrom, lastTs) 用于分组
    QString pendingUser_;                          // 本次登录输入，登录成功后决定是否记住
    QString pendingPass_;
    int boardId_ = 0;                              // 板端账号 id（0=未配置，不归类告警）
    QList<AlertItem> alerts_;                      // 板端告警列表（UI 本地状态）
};
