#include "ui/main_window.h"

#include "app/devmode.h"
#include "model/store.h"
#include "net/session.h"
#include "ui/chat_view.h"
#include "ui/contacts_panel.h"
#include "ui/login_view.h"
#include "ui/register_view.h"
#include "ui/search_panel.h"

#include <QApplication>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QJsonArray>
#include <QLabel>
#include <QLineEdit>
#include <QMouseEvent>
#include <QPushButton>
#include <QShortcut>
#include <QStackedWidget>
#include <QVBoxLayout>

MainWindow::MainWindow(Session* s, Store* st, DevMode* dev, QWidget* parent)
    : QMainWindow(parent), sess_(s), store_(st), dev_(dev), consoleBtn_(nullptr)
{
    setWindowFlags(Qt::FramelessWindowHint);
    resize(1180, 760);

    stack_ = new QStackedWidget(this);
    login_ = new LoginView(this);
    reg_ = new RegisterView(this);
    stack_->addWidget(login_);
    stack_->addWidget(reg_);
    buildMainPage();
    setCentralWidget(stack_);

    buildTitleBar();

    // 登录/注册切换
    connect(login_, &LoginView::registerRequested, this, [this] { stack_->setCurrentWidget(reg_); });
    connect(reg_, &RegisterView::backRequested, this, [this] { stack_->setCurrentWidget(login_); });
    connect(login_, &LoginView::loginRequested, this, [this](const QString& u, const QString& p) {
        login_->showError("");
        sess_->login(u, p);
    });
    connect(reg_, &RegisterView::registerSubmitted, this, [this](const QString& n, const QString& p) {
        reg_->showError("");
        sess_->registerUser(n, p);
    });

    // 会话信号
    connect(sess_, &Session::authError, this, [this](const QString& m) {
        if (stack_->currentWidget() == login_) login_->showError(m);
        else reg_->showError(m);
    });
    connect(sess_, &Session::ready, this, [this](int uid, const QString&) {
        selfId_ = uid;
        store_->setSelfId(uid);
        chat_->setSelfId(uid);
        stack_->setCurrentIndex(2);
    });
    connect(sess_, &Session::usersReceived, this, [this](const QJsonObject& o) {
        const auto users = o.value("users").toArray();
        for (const auto& u : users) {
            const auto uo = u.toObject();
            store_->upsertContact(uo.value("id").toInt(), uo.value("nickname").toString(),
                                  uo.value("status").toString() == "ok");
        }
        const auto groups = o.value("groups").toArray();
        groups_.clear();
        for (const auto& g : groups) {
            const auto go = g.toObject();
            groups_.append({go.value("groupId").toInt(), go.value("name").toString()});
        }
        refreshContacts();
        refreshGroups();
    });
    connect(sess_, &Session::messageReceived, this, [this](const QJsonObject& o) {
        Message m;
        m.fromId = o.value("fromId").toInt();
        m.toId = o.value("toId").toInt();
        m.groupId = o.value("groupId").toInt();
        m.text = o.value("msg").toString();
        m.kind = o.contains("groupId") && o.value("groupId").toInt() > 0 ? "group" : "direct";
        store_->addMessage(m);
        const bool matches = m.kind == "group"
                                 ? (currentIsGroup_ && m.groupId == currentPeer_)
                                 : (!currentIsGroup_ && m.fromId == currentPeer_);
        if (matches) chat_->appendMessage(m.fromId, m.text);
    });
    connect(sess_, &Session::presenceChanged, this, [this](int id, bool on) {
        store_->setOnline(id, on);
        refreshContacts();
    });
    connect(sess_, &Session::groupEvent, this, [this](const QJsonObject& o) {
        const int t = o.value("type").toInt();
        if (t == 10 || t == 14) {
            const int gid = o.value("groupId").toInt();
            const QString name = o.value("name").toString();
            if (gid > 0 && !name.isEmpty()) groups_.append({gid, name});
            refreshGroups();
        }
    });
    connect(sess_, &Session::broadcastReceived, this, [this](const QJsonObject& o) {
        if (o.value("event").toString() == "announce")
            chat_->appendMessage(0, "[公告] " + o.value("msg").toString());
    });

    // 主界面交互
    connect(contacts_, &ContactsPanel::contactSelected, this, [this](int id) { openConversation(id, false); });
    connect(contacts_, &ContactsPanel::groupSelected, this, [this](int id) { openConversation(id, true); });
    connect(contacts_, &ContactsPanel::createGroupRequested, this, [this] {
        bool ok = false;
        const QString name = QInputDialog::getText(this, "创建群", "群名称：", QLineEdit::Normal, "", &ok);
        if (ok && !name.isEmpty()) sess_->createGroup(name);
    });
    connect(contacts_, &ContactsPanel::joinGroupRequested, this, [this] {
        bool ok = false;
        const int gid = QInputDialog::getInt(this, "加入群", "群 ID：", 1, 1, 1000000, 1, &ok);
        if (ok) sess_->joinGroup(gid);
    });
    connect(chat_, &ChatView::sendRequested, this, [this](const QString& t) {
        if (currentPeer_ <= 0) return;
        if (currentIsGroup_) sess_->sendGroup(currentPeer_, t);
        else sess_->sendPrivate(currentPeer_, t);
        store_->addMessage(Message{selfId_, currentIsGroup_ ? 0 : currentPeer_,
                                   currentIsGroup_ ? currentPeer_ : 0, t, 0,
                                   currentIsGroup_ ? "group" : "direct"});
        chat_->appendMessage(selfId_, t);
    });
    connect(search_, &SearchPanel::queryChanged, this, &MainWindow::onSearch);

    // 开发者模式快捷键 + 重画标题栏
    auto* sc = new QShortcut(QKeySequence("Ctrl+Shift+D"), this);
    connect(sc, &QShortcut::activated, this, [this] { dev_->setEnabled(!dev_->enabled()); });
    connect(dev_, &DevMode::changed, this, [this](bool) { rebuildTitleBar(); });
}

void MainWindow::buildMainPage()
{
    auto* page = new QWidget(this);
    auto* lay = new QHBoxLayout(page);
    lay->setContentsMargins(8, 8, 8, 8);
    contacts_ = new ContactsPanel(page);
    chat_ = new ChatView(page);
    search_ = new SearchPanel(page);
    contacts_->setFixedWidth(260);
    search_->setFixedWidth(300);
    lay->addWidget(contacts_);
    lay->addWidget(chat_, 1);
    lay->addWidget(search_);
    stack_->addWidget(page);
}

void MainWindow::buildTitleBar()
{
    titleBar_ = new QWidget(this);
    titleBar_->setFixedHeight(40);
    auto* lay = new QHBoxLayout(titleBar_);
    lay->setContentsMargins(10, 0, 10, 0);
    auto* title = new QLabel("LanChat-Next", titleBar_);
    title->setObjectName("title");
    lay->addWidget(title);
    lay->addStretch(1);
    auto* min = new QPushButton("—", titleBar_);
    auto* max = new QPushButton("□", titleBar_);
    auto* close = new QPushButton("×", titleBar_);
    for (auto* b : {min, max, close}) b->setFixedWidth(36);
    connect(min, &QPushButton::clicked, this, [this] { showMinimized(); });
    connect(max, &QPushButton::clicked, this, [this] {
        isMaximized() ? showNormal() : showMaximized();
    });
    connect(close, &QPushButton::clicked, this, [this] { this->close(); });
    lay->addWidget(min);
    lay->addWidget(max);
    lay->addWidget(close);
    rebuildTitleBar();

    // 标题栏置于中央 widget 之上：用 QMainWindow 的 menuWidget 位置
    setMenuWidget(titleBar_);
}

void MainWindow::rebuildTitleBar()
{
    // 移除旧 Console 按钮
    if (consoleBtn_) {
        consoleBtn_->deleteLater();
        consoleBtn_ = nullptr;
    }
    if (dev_->enabled()) {
        consoleBtn_ = new QPushButton("Console", titleBar_);
        // 插到 stretch 之后、min 之前：简单追加到布局末尾前
        auto* lay = qobject_cast<QHBoxLayout*>(titleBar_->layout());
        lay->insertWidget(lay->count() - 3, consoleBtn_);
        connect(consoleBtn_, &QPushButton::clicked, this, [this] {
            // 开发者 Console：显示协议状态（P1 简化为状态文本弹窗）
            QInputDialog::getText(this, "Developer Console", "state:", QLineEdit::Normal,
                                  "selfId=" + QString::number(selfId_), nullptr);
        });
    }
}

void MainWindow::refreshContacts()
{
    QList<QPair<int, QString>> items;
    for (const auto& c : store_->contacts())
        if (c.id != selfId_) items.append({c.id, c.nickname});
    contacts_->setContacts(items);
}

void MainWindow::refreshGroups() { contacts_->setGroups(groups_); }

void MainWindow::openConversation(int id, bool isGroup)
{
    currentPeer_ = id;
    currentIsGroup_ = isGroup;
    chat_->clear();
    const QString key = isGroup ? Store::groupKey(id) : Store::directKey(id);
    for (const auto& m : store_->conversation(key)) chat_->appendMessage(m.fromId, m.text);
    chat_->setPeerLabel(isGroup ? QStringLiteral("群 %1").arg(id)
                                : store_->contacts().isEmpty() ? QString()
                                : QString());
    // peer 标签：找昵称
    for (const auto& c : store_->contacts())
        if (c.id == id) chat_->setPeerLabel(c.nickname);
}

void MainWindow::onSearch(const QString& query, const QString& scope)
{
    QStringList lines;
    const auto hits = store_->search(query);
    for (const auto& m : hits) {
        if (scope == "direct" && m.kind != "direct") continue;
        if (scope == "group" && m.kind != "group") continue;
        lines.append(QStringLiteral("[%1] %2: %3")
                         .arg(m.kind, QString::number(m.fromId), m.text));
    }
    search_->setResults(lines);
}

void MainWindow::mousePressEvent(QMouseEvent* e)
{
    if (e->button() == Qt::LeftButton && titleBar_->geometry().contains(e->pos()))
        dragPos_ = e->globalPosition().toPoint() - frameGeometry().topLeft();
}

void MainWindow::mouseMoveEvent(QMouseEvent* e)
{
    if (e->buttons() & Qt::LeftButton && !dragPos_.isNull())
        move(e->globalPosition().toPoint() - dragPos_);
}