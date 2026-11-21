#include "ui/main_window.h"

#include "app/credentials.h"
#include "app/board_connection.h"
#include "ui/board_connection_panel.h"
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
#include <QTimer>
#include <QComboBox>
#include <QSpinBox>
#include <QFormLayout>
#include <QMessageBox>
#include <QFileDialog>
#include <QFile>
#include <QImage>
#include <QDesktopServices>
#include <QUrl>
#include "net/transfer_manager.h"

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
    transfers_=new TransferManager(sess_,this);
    setWindowFlags(Qt::FramelessWindowHint);
    resize(1300, 800);
    // 板端账号：Console 桥登录用的那个 LanChat 账号 id；左栏齿轮可改，存 ini
    boardId_ = QSettings().value("board_account_id", 0).toInt();
    bridge_ = new BoardConnection(this);
    if (bridge_->config().accountId > 0) boardId_ = bridge_->config().accountId;
    connect(bridge_, &BoardConnection::accountChanged, this, [this](int id) {
        if (id > 0) {
            boardId_ = id;
            QSettings().setValue("board_account_id", id);
        }
    });
    QTimer::singleShot(700, this, [this] { if (bridge_->config().autoStart) bridge_->start(); });

    stack_ = new QStackedWidget(this);
    stack_->setObjectName("mainPages");
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

    connect(sess_,&Session::registered,this,[this](int id){reg_->showError(QString("注册成功，账号 ID：%1，请返回登录").arg(id));});
    connect(sess_, &Session::authError, this, [this](const QString& m) {
        if (stack_->currentWidget() == login_) login_->showError(m);
        else reg_->showError(m);
    });
    connect(sess_, &Session::ready, this, [this](int uid, const QString&) {
        if(selfId_>0 && selfId_!=uid) {store_->clear();unread_.clear();groups_.clear();groupMembers_.clear();chat_->clear();currentPeer_=0;}
        selfId_ = uid;
        bridge_->setDefaultRecipient(uid);
        store_->setSelfId(uid);
        chat_->setSelfId(uid);
        // 记住账户只在登录成功后落盘：错密码不会被记下来
        if (login_->rememberChecked()) credentials::save(pendingUser_, pendingPass_);
        else credentials::clear();
        if(stack_->currentWidget()==boardPanel_)boardReturnPage_=stack_->widget(2);
        else stack_->setCurrentIndex(2);
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
        m.id=o.value("msg_id").toVariant().toString();
        m.contentType=o.value("content_type").toString("text");m.attachment=o.value("attachment").toObject();
        auto timestamp=o.value("timestamp").toVariant().toLongLong();m.ts=timestamp>0?(timestamp<100000000000LL?timestamp*1000:timestamp):nowMs();
        m.kind = (m.groupId > 0) ? "group" : "direct";
        if(!store_->addMessage(m))return;
        const int peer = (m.kind == "group") ? m.groupId : (m.fromId == selfId_ ? m.toId : m.fromId);
        const QString key = (m.kind == "group") ? Store::groupKey(m.groupId) : Store::directKey(peer);
        const bool grouped = groupedWith(lastMsg_.value(key, {-1, 0}), m.fromId, m.ts);
        lastMsg_[key] = {m.fromId, m.ts};
        // 板端告警：进左栏告警分节；会话里渲染成告警卡片
        const int alertLevel = isBoardAlert(m) ? alertLevelFor(m.text) : 0;
        if (alertLevel > 0) {
            alerts_.append(AlertItem{peer, m.text,
                                     QDateTime::fromMSecsSinceEpoch(m.ts).toString("HH:mm"),
                                     alertLevel, m.kind == "group"});
            refreshAlerts();
        }
        const bool matches = (m.kind == "group")
                                 ? (currentIsGroup_ && m.groupId == currentPeer_)
                                 : (!currentIsGroup_ && peer == currentPeer_);
        if (matches) {
            renderMessage(m,grouped,alertLevel);
        } else if (m.fromId != selfId_) {
            unread_[key] = unread_.value(key, 0) + 1;
            refreshContacts();refreshGroups();
        }
    });
    connect(sess_, &Session::presenceChanged, this, [this](int id, bool on) {
        store_->setOnline(id, on);
        refreshContacts();
        refreshMembers();
    });
    connect(sess_,&Session::historyReceived,this,[this](const QJsonObject& o){
        if(o.value("status").toString()=="error")return;
        int gid=o.value("groupId").toInt(),to=o.value("toId").toInt();
        QString key=gid>0?Store::groupKey(gid):Store::directKey(to);
        for(auto v:o.value("messages").toArray()) {auto a=v.toObject();Message m;m.fromId=a["fromId"].toInt();m.toId=a["toId"].toInt();m.groupId=a["groupId"].toInt();
            m.id=a["msg_id"].toVariant().toString();m.text=a["msg"].toString();m.kind=m.groupId>0?"group":"direct";m.contentType=a.value("content_type").toString("text");m.attachment=a["attachment"].toObject();
            auto ts=a["timestamp"].toVariant().toLongLong();m.ts=ts<100000000000LL?ts*1000:ts;store_->addMessage(m);}
        if(!o["cursor"].toString().isEmpty())historyCursors_[key]=o["cursor"].toString();
        if(currentPeer_==(gid>0?gid:to)&&currentIsGroup_==(gid>0)){renderConversation();chat_->setHistoryAvailable(o["has_more"].toBool());}
    });
    connect(sess_,&Session::groupEvent,this,[this](const QJsonObject& o){
        int type=o["type"].toInt(),gid=o["groupId"].toInt();
        if(o.value("status").toString()=="error"){QMessageBox::information(this,"群组",o["msg"].toString("操作失败"));return;}
        if(type==10||type==14){bool known=false;for(const auto& g:groups_)if(g.first==gid)known=true;
            if(gid>0&&!known)groups_.append({gid,o["name"].toString()});refreshGroups();openConversation(gid,true);}
        if(type==61||type==14){groupMembers_[gid].clear();for(auto v:o["users"].toArray()){auto u=v.toObject();int uid=u["id"].toInt();
            groupMembers_[gid].append(uid);store_->upsertContact(uid,u["nickname"].toString(),u["status"].toString()=="ok");}refreshMembers();refreshContacts();}
        if(type==18||type==19){
            if(type==19 && o["id"].toInt()==selfId_){for(int i=groups_.size()-1;i>=0;--i)if(groups_[i].first==gid)groups_.removeAt(i);
                groupMembers_.remove(gid);refreshGroups();if(currentIsGroup_&&currentPeer_==gid){currentPeer_=0;currentIsGroup_=false;chat_->clear();chat_->setPeerLabel("已退出群组");chat_->setGroup(false);refreshMembers();}}
            else if(currentIsGroup_&&currentPeer_==gid)sess_->requestMembers(gid);
        }
    });
    connect(sess_, &Session::broadcastReceived, this, [this](const QJsonObject& o) {
        if(o.value("status").toString()=="error")QMessageBox::information(this,"服务器",o.value("msg").toString());
        if (o.value("event").toString() == "announce")
            chat_->appendMessage(0, "公告", o.value("msg").toString(), nowMs(), false);
    });

    connect(contacts_, &ContactsPanel::contactSelected, this, [this](int id) { openConversation(id, false); });
    connect(contacts_, &ContactsPanel::groupSelected, this, [this](int id) { openConversation(id, true); });
    connect(contacts_, &ContactsPanel::alertSelected, this, [this](int id, bool group) { openConversation(id, group); });
    connect(contacts_, &ContactsPanel::boardConfigRequested, this, &MainWindow::showBoardConnection);
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
        // The server echoes both direct and group messages to their sender.
        // Store and render that reply through messageReceived exactly once.
        if (currentIsGroup_) sess_->sendGroup(currentPeer_, t);
        else sess_->sendPrivate(currentPeer_, t);
    });

    connect(chat_,&ChatView::leaveRequested,this,[this]{if(currentIsGroup_&&currentPeer_>0 && QMessageBox::question(this,"退出群组","确认退出当前群组？")==QMessageBox::Yes)sess_->leaveGroup(currentPeer_);});
    connect(chat_,&ChatView::historyRequested,this,[this]{QString key=currentIsGroup_?Store::groupKey(currentPeer_):Store::directKey(currentPeer_);sess_->requestConversationHistory(currentPeer_,currentIsGroup_,historyCursors_.value(key));});
    connect(chat_,&ChatView::filesRequested,this,[this](const QStringList& files){if(currentPeer_<=0)return;for(const auto& f:files)transfers_->upload(f,currentIsGroup_?0:currentPeer_,currentIsGroup_?currentPeer_:0);});
    connect(chat_,&ChatView::attachmentRequested,transfers_,&TransferManager::download);
    connect(chat_,&ChatView::transferAction,this,[this](const QString& id,const QString& action){if(action=="pause")transfers_->pause(id);else if(action=="resume")transfers_->resume(id);else transfers_->cancel(id);});
    connect(transfers_,&TransferManager::changed,this,&MainWindow::refreshTransfers);
    connect(transfers_,&TransferManager::error,this,[this](const QString& text){QMessageBox::information(this,"附件",text);});
    connect(transfers_,&TransferManager::downloaded,this,[this](const QString& id,const QString& path){chat_->attachmentReady(id,path);});
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
    auto* network=new QPushButton("服务器",titleBar_);network->setObjectName("serverSettingsButton");
    connect(network,&QPushButton::clicked,this,&MainWindow::showConnectionSettings);lay->addWidget(network);
    connect(sess_,&Session::stateChanged,network,[this,network](const QString& state){network->setToolTip(sess_->endpoint()+" · "+state);});
    auto* board = new QPushButton("板端连接", titleBar_);
    board->setObjectName("boardConnectionButton");
    connect(board, &QPushButton::clicked, this, &MainWindow::showBoardConnection);
    connect(bridge_, &BoardConnection::changed, board, [this, board] {
        board->setText(bridge_->snapshot().value("board_connected").toBool() ? "板端连接 · 已连接" : "板端连接");
        board->setToolTip(bridge_->state());
    });
    lay->addWidget(board);
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
        it.unread = unread_.value(Store::directKey(c.id), 0);
        const auto conv = store_->conversation(Store::directKey(c.id));
        if (!conv.isEmpty()) {
            it.preview = conv.last().text;
            it.time = QDateTime::fromMSecsSinceEpoch(conv.last().ts).toString("HH:mm");
        }
        items.append(it);
    }
    contacts_->setContacts(items);
    contacts_->setSelected(currentPeer_,currentIsGroup_);
}

void MainWindow::refreshGroups() { contacts_->setGroups(groups_,unread_);contacts_->setSelected(currentPeer_,currentIsGroup_); }

void MainWindow::refreshAlerts()
{
    // 新告警排最前，左栏一眼看到最近一条
    QList<AlertItem> reversed;
    for (auto it = alerts_.crbegin(); it != alerts_.crend(); ++it) reversed.append(*it);
    contacts_->setAlerts(reversed);
}

bool MainWindow::isBoardAlert(const Message& m) const
{
    return boardId_ > 0 && (m.kind == "direct" || m.kind == "group") && m.fromId == boardId_ && m.contentType=="text";
}

void MainWindow::showBoardConnection()
{
    // One persistent page keeps the draft, pending delivery and scroll position intact.
    if(!boardPanel_){
        boardPanel_=new BoardConnectionPanel(bridge_,stack_,sess_);
        stack_->addWidget(boardPanel_);
        connect(boardPanel_,&BoardConnectionPanel::returnRequested,this,[this]{
            stack_->setCurrentWidget(boardReturnPage_?boardReturnPage_:static_cast<QWidget*>(login_));
        });
    }
    if(stack_->currentWidget()!=boardPanel_)boardReturnPage_=stack_->currentWidget();
    stack_->setCurrentWidget(boardPanel_);
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
        if(currentIsGroup_ && !groupMembers_.value(currentPeer_).contains(c.id))continue;
        if(!currentIsGroup_ && c.id!=currentPeer_ && c.id!=selfId_)continue;
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
    unread_.remove(isGroup?Store::groupKey(id):Store::directKey(id));
    contacts_->setSelected(id,isGroup);
    refreshContacts();
    chat_->clear();
    const QString key = isGroup ? Store::groupKey(id) : Store::directKey(id);
    QPair<int, qint64> last{-1, 0};
    for (const auto& m : store_->conversation(key)) {
        const bool grouped = groupedWith(last, m.fromId, m.ts);
        last = {m.fromId, m.ts};
        const int lvl = isBoardAlert(m) ? alertLevelFor(m.text) : 0;
        renderMessage(m,grouped,lvl);
    }
    lastMsg_[key] = last;
    QString label=senderName(id);if(isGroup){for(const auto& g:groups_)if(g.first==id)label=g.second;label+=QString(" · 群 ID %1").arg(id);}
    chat_->setPeerLabel(label);chat_->setGroup(isGroup);chat_->setHistoryAvailable(false);
    refreshMembers();if(sess_->selfId()>0){sess_->requestConversationHistory(id,isGroup);if(isGroup)sess_->requestMembers(id);}
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
        store_->addMessage(Message{2, 1, 0, "看看这个设计稿", now - 330000, "direct"});
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
        unread_[Store::directKey(5)] = 3;
    }
    unread_[Store::directKey(3)] = 2;
    unread_[Store::directKey(4)] = 1;
    refreshGroups();
    refreshMembers();
    openConversation(openBoard ? 5 : 2, false);
    if (!openBoard) contacts_->setHovered(3); // 状态对照：Bob 行强制 hover
    stack_->setCurrentIndex(2);
}

void MainWindow::renderMessage(const Message& m,bool grouped,int level) {
    if(m.contentType=="text") chat_->appendMessage(m.fromId,senderName(m.fromId),m.text,m.ts,grouped,level);
    else chat_->appendAttachment(m.fromId,senderName(m.fromId),m.attachment,m.ts,transfers_->cachedPath(m.attachment));
}
void MainWindow::renderConversation(){
    chat_->clear();QString key=currentIsGroup_?Store::groupKey(currentPeer_):Store::directKey(currentPeer_);QPair<int,qint64> last{-1,0};
    for(const auto& m:store_->conversation(key)){renderMessage(m,groupedWith(last,m.fromId,m.ts),isBoardAlert(m)?alertLevelFor(m.text):0);last={m.fromId,m.ts};}lastMsg_[key]=last;
}
void MainWindow::refreshTransfers(){if(transferUpdatePending_)return;transferUpdatePending_=true;QTimer::singleShot(100,this,[this]{transferUpdatePending_=false;chat_->setTransfers(transfers_->tasks());});}
void MainWindow::showConnectionSettings(){
    auto* page=new QWidget(stack_);auto* outer=new QVBoxLayout(page);outer->addStretch();
    auto* formWidget=new QWidget(page);formWidget->setMaximumWidth(560);auto* form=new QFormLayout(formWidget);
    auto* mode=new QComboBox(formWidget);mode->addItems({"本机托管（其他电脑连接本机）","连接公司局域网主机"});
    QSettings settings;mode->setCurrentIndex(settings.value("network/hosted",true).toBool()?0:1);
    auto* host=new QLineEdit(settings.value("network/host","127.0.0.1").toString(),formWidget);host->setObjectName("serverHost");
    auto* port=new QSpinBox(formWidget);port->setRange(1024,65534);port->setValue(settings.value("network/port",12346).toInt());
    auto* maximum=new QSpinBox(formWidget);maximum->setRange(1,1024);maximum->setValue(settings.value("network/attachment_max_mb",100).toInt());maximum->setSuffix(" MB");
    auto* hint=new QLabel("公司内的所有用户连接同一台常开 PC。\n本机托管时，请允许 Windows 防火墙的专用网络入站连接。\n主机的数据与附件统一保存在 LanChat-Next/data。",formWidget);hint->setWordWrap(true);
    form->addRow("服务器模式",mode);form->addRow("主机地址",host);form->addRow("TCP 端口",port);form->addRow("单附件上限",maximum);form->addRow(hint);
    auto* apply=new QPushButton("保存并重新连接",formWidget);auto* back=new QPushButton("返回",formWidget);form->addRow(apply,back);
    host->setEnabled(mode->currentIndex()==1);connect(mode,&QComboBox::currentIndexChanged,host,[host](int index){host->setEnabled(index==1);});
    outer->addWidget(formWidget,0,Qt::AlignHCenter);outer->addStretch();auto* previous=stack_->currentWidget();stack_->addWidget(page);stack_->setCurrentWidget(page);
    connect(back,&QPushButton::clicked,this,[this,page,previous]{stack_->setCurrentWidget(previous);stack_->removeWidget(page);page->deleteLater();});
    connect(apply,&QPushButton::clicked,this,[=,this]{
        bool hosted=mode->currentIndex()==0;QString address=hosted?"127.0.0.1":host->text().trimmed();if(address.isEmpty()){host->setFocus();return;}
        QSettings cfg;cfg.setValue("network/hosted",hosted);cfg.setValue("network/host",host->text().trimmed());cfg.setValue("network/port",port->value());cfg.setValue("network/attachment_max_mb",maximum->value());
        sess_->logout();store_->clear();selfId_=0;currentPeer_=0;groups_.clear();groupMembers_.clear();historyCursors_.clear();unread_.clear();alerts_.clear();lastMsg_.clear();chat_->clear();refreshContacts();refreshGroups();refreshMembers();refreshAlerts();
        auto bridgeConfig=bridge_->config();bool wasRunning=bridge_->running();bridge_->stop();
        bool changedServer=bridgeConfig.serverHost!=address||bridgeConfig.serverPort!=quint16(port->value());
        bridgeConfig.serverHost=address;bridgeConfig.serverPort=quint16(port->value());
        if(changedServer){bridgeConfig.accountId=0;bridgeConfig.password.clear();bridgeConfig.target=0;bridgeConfig.groupId=0;bridgeConfig.autoStart=false;boardId_=0;QSettings().setValue("board_account_id",0);}
        bridge_->configure(bridgeConfig);if(wasRunning&&!changedServer)bridge_->start();
        stack_->setCurrentWidget(login_);stack_->removeWidget(page);emit connectionRequested(address,quint16(port->value()),hosted,maximum->value());page->deleteLater();
    });
}
