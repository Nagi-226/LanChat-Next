#include "ui/main_window.h"

#include "app/credentials.h"
#include "app/devmode.h"
#include "model/store.h"
#include "net/session.h"
#include "ui/chat_view.h"
#include "ui/contacts_panel.h"
#include "ui/login_view.h"
#include "ui/members_panel.h"
#include "ui/register_view.h"

#include <QDateTime>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QJsonArray>
#include <QLabel>
#include <QLineEdit>
#include <QMouseEvent>
#include <QPushButton>
#include <QRegularExpression>
#include <QSettings>
#include <QShortcut>
#include <QStackedWidget>
#include <QVBoxLayout>

namespace {
qint64 nowMs() { return QDateTime::currentMSecsSinceEpoch(); }
// 连续消息分组：同发送者且间隔 < 5 分钟
bool groupedWith(const QPair<int, qint64>& last, int from, qint64 ts)
{
    return last.first == from && (ts - last.second) < 5 * 60 * 1000;
}
}

MainWindow::MainWindow(Session* s, Store* st, DevMode* dev, QWidget* parent)
    : QMainWindow(parent), sess_(s), store_(st), dev_(dev), consoleBtn_(nullptr)
{
    setWindowFlags(Qt::FramelessWindowHint);
    resize(1300, 800);
    // 板端账号：Console 桥登录用的那个 LanChat 账号 id；左栏齿轮可改，存 ini
    boardId_ = QSettings().value("board_account_id", 0).toInt();

    stack_ = new QStackedWidget(this);
    login_ = new LoginView(this);
    reg_ = new RegisterView(this);
    stack_->addWidget(login_);
    stack_->addWidget(reg_);
    buildMainPage();
    setCentralWidget(stack_);
    buildTitleBar();

    connect(login_, &LoginView::registerRequested, this, [this] { stack_->setCurrentWidget(reg_); });
    connect(reg_, &RegisterView::backRequested, this, [this] { stack_->setCurrentWidget(login_); });
    connect(login_, &LoginView::loginRequested, this, [this](const QString& u, const QString& p) {
        login_->showError("");
        pendingUser_ = u;
        pendingPass_ = p;
        sess_->login(u, p);
    });
    connect(reg_, &RegisterView::registerSubmitted, this, [this](const QString& n, const QString& p) {
        reg_->showError("");
        sess_->registerUser(n, p);
    });

    connect(sess_, &Session::authError, this, [this](const QString& m) {
        if (stack_->currentWidget() == login_) login_->showError(m);
        else reg_->showError(m);
    });
    connect(sess_, &Session::ready, this, [this](int uid, const QString&) {
        selfId_ = uid;
        store_->setSelfId(uid);
        chat_->setSelfId(uid);
        // 记住账户只在登录成功后落盘：错密码不会被记下来
        if (login_->rememberChecked()) credentials::save(pendingUser_, pendingPass_);
        else credentials::clear();
        stack_->setCurrentIndex(2);
    });
    connect(sess_, &Session::usersReceived, this, [this](const QJsonObject& o) {
        const auto users = o.value("users").toArray();
        for (const auto& u : users) {
            const auto uo = u.toObject();
            store_->upsertContact(uo.value("id").toInt(), uo.value("nickname").toString(),
                                  uo.value("status").toString() == "ok");
        }
        groups_.clear();
        const auto groups = o.value("groups").toArray();
        for (const auto& g : groups) {
            const auto go = g.toObject();
            groups_.append({go.value("groupId").toInt(), go.value("name").toString()});
        }
        refreshContacts();
        refreshGroups();
        refreshMembers();
    });
    connect(sess_, &Session::messageReceived, this, [this](const QJsonObject& o) {
        Message m;
        m.fromId = o.value("fromId").toInt();
        m.toId = o.value("toId").toInt();
        m.groupId = o.value("groupId").toInt();
        m.text = o.value("msg").toString();
        m.ts = nowMs();
        m.kind = (m.groupId > 0) ? "group" : "direct";
        store_->addMessage(m);
        const int peer = (m.kind == "group") ? m.groupId : m.fromId;
        const QString key = (m.kind == "group") ? Store::groupKey(m.groupId) : Store::directKey(peer);
        const bool grouped = groupedWith(lastMsg_.value(key, {-1, 0}), m.fromId, m.ts);
        lastMsg_[key] = {m.fromId, m.ts};
        // 板端告警：进左栏告警分节；会话里渲染成告警卡片
        const int alertLevel = isBoardAlert(m) ? alertLevelFor(m.text) : 0;
        if (alertLevel > 0) {
            alerts_.append(AlertItem{m.fromId, m.text,
                                     QDateTime::fromMSecsSinceEpoch(m.ts).toString("HH:mm"),
                                     alertLevel});
            refreshAlerts();
        }
        const bool matches = (m.kind == "group")
                                 ? (currentIsGroup_ && m.groupId == currentPeer_)
                                 : (!currentIsGroup_ && m.fromId == currentPeer_);
        if (matches) {
            chat_->appendMessage(m.fromId, senderName(m.fromId), m.text, m.ts, grouped, alertLevel);
        } else if (m.fromId != selfId_) {
            unread_[peer] = unread_.value(peer, 0) + 1;
            refreshContacts();
        }
    });
    connect(sess_, &Session::presenceChanged, this, [this](int id, bool on) {
        store_->setOnline(id, on);
        refreshContacts();
        refreshMembers();
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
            chat_->appendMessage(0, "公告", o.value("msg").toString(), nowMs(), false);
    });

    connect(contacts_, &ContactsPanel::contactSelected, this, [this](int id) { openConversation(id, false); });
    connect(contacts_, &ContactsPanel::groupSelected, this, [this](int id) { openConversation(id, true); });
    connect(contacts_, &ContactsPanel::alertSelected, this, [this](int id) { openConversation(id, false); });
    connect(contacts_, &ContactsPanel::boardConfigRequested, this, [this] {
        bool ok = false;
        const int v = QInputDialog::getInt(
            this, "板端账号",
            "板端账号的数字 id（Console 桥用哪个 LanChat 账号发告警就填哪个；0 = 关闭归类）：",
            boardId_, 0, 1000000, 1, &ok);
        if (!ok) return;
        boardId_ = v;
        QSettings().setValue("board_account_id", v);
        refreshAlerts();
    });
    connect(contacts_, &ContactsPanel::createGroupRequested, this, [this] {
        bool ok = false;
        const QString name = QInputDialog::getText(this, "建群", "群名称：", QLineEdit::Normal, "", &ok);
        if (ok && !name.isEmpty()) sess_->createGroup(name);
    });
    connect(contacts_, &ContactsPanel::joinGroupRequested, this, [this] {
        bool ok = false;
        const int gid = QInputDialog::getInt(this, "加入群", "群 ID：", 1, 1, 1000000, 1, &ok);
        if (ok) sess_->joinGroup(gid);
    });
    connect(chat_, &ChatView::sendRequested, this, [this](const QString& t) {
        if (currentPeer_ <= 0) return;
        const qint64 ts = nowMs();
        if (currentIsGroup_) sess_->sendGroup(currentPeer_, t);
        else sess_->sendPrivate(currentPeer_, t);
        Message m{selfId_, currentIsGroup_ ? 0 : currentPeer_,
                  currentIsGroup_ ? currentPeer_ : 0, t, ts,
                  currentIsGroup_ ? "group" : "direct"};
        store_->addMessage(m);
        const QString key = currentIsGroup_ ? Store::groupKey(currentPeer_) : Store::directKey(currentPeer_);
        const bool grouped = groupedWith(lastMsg_.value(key, {-1, 0}), selfId_, ts);
        lastMsg_[key] = {selfId_, ts};
        chat_->appendMessage(selfId_, "我", t, ts, grouped);
    });

    auto* sc = new QShortcut(QKeySequence("Ctrl+Shift+D"), this);
    connect(sc, &QShortcut::activated, this, [this] { dev_->setEnabled(!dev_->enabled()); });
    connect(dev_, &DevMode::changed, this, [this](bool) { rebuildTitleBar(); });
}

void MainWindow::buildMainPage()
{
    auto* page = new QWidget(this);
    page->setAttribute(Qt::WA_StyledBackground, true);
    auto* lay = new QHBoxLayout(page);
    lay->setContentsMargins(0, 0, 0, 0);
    lay->setSpacing(0);
    contacts_ = new ContactsPanel(page);   // 内部 setFixedWidth(268)
    chat_ = new ChatView(page);
    chat_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    members_ = new MembersPanel(page);     // 内部 setFixedWidth(236)
    lay->addWidget(contacts_);
    lay->addWidget(chat_, 1);
    lay->addWidget(members_);
    stack_->addWidget(page);
}

void MainWindow::buildTitleBar()
{
    titleBar_ = new QWidget(this);
    titleBar_->setFixedHeight(40);
    titleBar_->setAttribute(Qt::WA_StyledBackground, true);
    titleBar_->setStyleSheet("background-color:#0E1014;");
    auto* lay = new QHBoxLayout(titleBar_);
    lay->setContentsMargins(12, 0, 8, 0);
    lay->setSpacing(4);
    auto* title = new QLabel("LanChat-Next", titleBar_);
    title->setObjectName("headerTitle");
    lay->addWidget(title);
    lay->addStretch(1);
    auto* min = new QPushButton("—", titleBar_);
    auto* max = new QPushButton("□", titleBar_);
    auto* close = new QPushButton("×", titleBar_);
    for (auto* b : {min, max, close}) b->setObjectName("icon");
    connect(min, &QPushButton::clicked, this, [this] { showMinimized(); });
    connect(max, &QPushButton::clicked, this, [this] { isMaximized() ? showNormal() : showMaximized(); });
    connect(close, &QPushButton::clicked, this, [this] { this->close(); });
    lay->addWidget(min);
    lay->addWidget(max);
    lay->addWidget(close);
    rebuildTitleBar();
    setMenuWidget(titleBar_);
}

void MainWindow::rebuildTitleBar()
{
    if (consoleBtn_) {
        consoleBtn_->deleteLater();
        consoleBtn_ = nullptr;
    }
    if (dev_->enabled()) {
        consoleBtn_ = new QPushButton("Console", titleBar_);
        auto* lay = qobject_cast<QHBoxLayout*>(titleBar_->layout());
        lay->insertWidget(lay->count() - 3, consoleBtn_);
        connect(consoleBtn_, &QPushButton::clicked, this, [this] {
            QInputDialog::getText(this, "Developer Console", "state:", QLineEdit::Normal,
                                  "selfId=" + QString::number(selfId_), nullptr);
        });
    }
}

QString MainWindow::senderName(int id) const
{
    if (id == selfId_) return "我";
    if (id == 0) return "公告";
    for (const auto& c : store_->contacts())
        if (c.id == id) return c.nickname;
    return QString::number(id);
}

void MainWindow::refreshContacts()
{
    QList<ContactItem> items;
    for (const auto& c : store_->contacts()) {
        if (c.id == selfId_) continue;
        ContactItem it;
        it.id = c.id;
        it.name = c.nickname;
        it.online = c.online;
        it.unread = unread_.value(c.id, 0);
        const auto conv = store_->conversation(Store::directKey(c.id));
        if (!conv.isEmpty()) {
            it.preview = conv.last().text;
            it.time = QDateTime::fromMSecsSinceEpoch(conv.last().ts).toString("HH:mm");
        }
        items.append(it);
    }
    contacts_->setContacts(items);
    contacts_->setSelected(currentPeer_);
}

void MainWindow::refreshGroups() { contacts_->setGroups(groups_); }

void MainWindow::refreshAlerts()
{
    // 新告警排最前，左栏一眼看到最近一条
    QList<AlertItem> reversed;
    for (auto it = alerts_.crbegin(); it != alerts_.crend(); ++it) reversed.append(*it);
    contacts_->setAlerts(reversed);
}

bool MainWindow::isBoardAlert(const Message& m) const
{
    return boardId_ > 0 && m.kind == "direct" && m.fromId == boardId_;
}

int MainWindow::alertLevelFor(const QString& text) const
{
    // Console 桥的告警正文形如 "PPM 疲劳告警：drowsy_level=2"；级别>=3 严重、2 警告、其余提示
    static const QRegularExpression re(QStringLiteral("(?:drowsy_)?level=(\\d+)"));
    const auto match = re.match(text);
    if (match.hasMatch()) {
        const int n = match.captured(1).toInt();
        return n >= 3 ? 3 : n == 2 ? 2 : 1;
    }
    return 1;
}

void MainWindow::refreshMembers()
{
    QList<ContactItem> items;
    for (const auto& c : store_->contacts()) {
        ContactItem it;
        it.id = c.id;
        it.name = c.nickname;
        it.online = c.online;
        items.append(it);
    }
    members_->setMembers(items);
}

void MainWindow::openConversation(int id, bool isGroup)
{
    currentPeer_ = id;
    currentIsGroup_ = isGroup;
    unread_.remove(id);
    contacts_->setSelected(id);
    refreshContacts();
    chat_->clear();
    const QString key = isGroup ? Store::groupKey(id) : Store::directKey(id);
    QPair<int, qint64> last{-1, 0};
    for (const auto& m : store_->conversation(key)) {
        const bool grouped = groupedWith(last, m.fromId, m.ts);
        last = {m.fromId, m.ts};
        const int lvl = isBoardAlert(m) ? alertLevelFor(m.text) : 0;
        chat_->appendMessage(m.fromId, senderName(m.fromId), m.text, m.ts, grouped, lvl);
    }
    lastMsg_[key] = last;
    chat_->setPeerLabel(isGroup ? QStringLiteral("群 %1").arg(id) : senderName(id));
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

void MainWindow::loadDemoData(bool withMessages, bool openBoard)
{
    selfId_ = 1;
    store_->setSelfId(1);
    chat_->setSelfId(1);
    store_->upsertContact(2, "Alice", true);
    store_->upsertContact(3, "Bob", false);
    store_->upsertContact(4, "Carol", true);
    // 演示板端账号：真实环境由左栏齿轮配置（Console 桥的登录账号 id）
    boardId_ = 5;
    store_->upsertContact(5, "RoboMind-S3", true);
    groups_ = {{10, "项目群"}};
    const qint64 now = nowMs();
    const QString k = Store::directKey(2);
    if (withMessages) {
        store_->addMessage(Message{2, 1, 0, "嗨，在吗？", now - 6 * 60000, "direct"});
        store_->addMessage(Message{2, 1, 0, "看看这个设计稿", now - 5.5 * 60000, "direct"});
        store_->addMessage(Message{1, 2, 0, "在的，发我", now - 4 * 60000, "direct"});
        store_->addMessage(Message{2, 1, 0, "好嘞", now - 60000, "direct"});
        // 板端告警演示：与 Console 桥真实正文同格式，走同一套检测/渲染路径
        store_->addMessage(Message{5, 1, 0, "PPM 疲劳告警：drowsy_level=2", now - 9 * 60000, "direct"});
        store_->addMessage(Message{5, 1, 0, "PPM 疲劳告警：drowsy_level=3", now - 3 * 60000, "direct"});
        store_->addMessage(Message{5, 1, 0, "板端链路恢复：heartbeat 正常", now - 60000, "direct"});
        alerts_.append(AlertItem{5, "PPM 疲劳告警：drowsy_level=2",
                                 QDateTime::fromMSecsSinceEpoch(now - 9 * 60000).toString("HH:mm"), 2});
        alerts_.append(AlertItem{5, "PPM 疲劳告警：drowsy_level=3",
                                 QDateTime::fromMSecsSinceEpoch(now - 3 * 60000).toString("HH:mm"), 3});
        alerts_.append(AlertItem{5, "板端链路恢复：heartbeat 正常",
                                 QDateTime::fromMSecsSinceEpoch(now - 60000).toString("HH:mm"), 1});
        refreshAlerts();
        unread_[5] = 3;
    }
    unread_[3] = 2;
    unread_[4] = 1;
    refreshGroups();
    refreshMembers();
    openConversation(openBoard ? 5 : 2, false);
    if (!openBoard) contacts_->setHovered(3); // 状态对照：Bob 行强制 hover
    stack_->setCurrentIndex(2);
}