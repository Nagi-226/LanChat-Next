// main_window.h —— 主窗口：无边框 + 自绘标题栏 + 登录/注册/主界面三态。
// Console 按钮仅在开发者模式渲染（非开发者模式零入口，向主流社交产品看齐）。
// Ctrl+Shift+D 切换开发者模式。
#pragma once

#include <QMainWindow>
#include <QJsonObject>
#include <QPoint>

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
class SearchPanel;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    MainWindow(Session* s, Store* st, DevMode* dev, QWidget* parent = nullptr);

protected:
    void mousePressEvent(QMouseEvent* e) override;
    void mouseMoveEvent(QMouseEvent* e) override;

private:
    void buildTitleBar();
    void rebuildTitleBar();
    void buildMainPage();
    void refreshContacts();
    void refreshGroups();
    void openConversation(int id, bool isGroup);
    void onSearch(const QString& query, const QString& scope);

    Session* sess_;
    Store* store_;
    DevMode* dev_;
    QStackedWidget* stack_;
    LoginView* login_;
    RegisterView* reg_;
    ChatView* chat_;
    ContactsPanel* contacts_;
    SearchPanel* search_;
    QWidget* titleBar_;
    QPushButton* consoleBtn_;
    QPoint dragPos_;
    int selfId_ = 0;
    int currentPeer_ = 0;
    bool currentIsGroup_ = false;
    QList<QPair<int, QString>> groups_;
};