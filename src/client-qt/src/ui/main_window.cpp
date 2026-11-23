#include "ui/main_window.h"

#include "app/credentials.h"
#include "app/preferences.h"
#include "ui/settings_panel.h"
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
#include <QGuiApplication>
#include <QWindow>
#include <QShowEvent>
#include <QIconEngine>
#include <QPainter>
#include <QStyle>
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
#include <QSystemTrayIcon>
#include <QMenu>
#include <QCloseEvent>
#include <QButtonGroup>
#include <QApplication>
#include "net/transfer_manager.h"

#ifdef Q_OS_WIN
#include <qt_windows.h>
#include <windowsx.h>
#include <dwmapi.h>
#endif

namespace {
enum class CaptionAction { Minimize, Maximize, Restore, Close };
// 统一 12x12 逻辑像素的矢量画布，不依赖不同字体的 — / □ / × 字形。
class CaptionIcon : public QIconEngine {
public:
    explicit CaptionIcon(CaptionAction action):action_(action){}
    QIconEngine* clone() const override { return new CaptionIcon(action_); }
    QPixmap pixmap(const QSize& size,QIcon::Mode mode,QIcon::State state) override {
        QPixmap image(size);image.fill(Qt::transparent);QPainter painter(&image);paint(&painter,QRect(QPoint(),size),mode,state);return image;
    }
    void paint(QPainter* painter,const QRect& rect,QIcon::Mode,QIcon::State) override {
        painter->save();painter->setRenderHint(QPainter::Antialiasing);painter->translate(rect.center()+QPointF(0.5,0.5));
        painter->scale(rect.width()/12.0,rect.height()/12.0);painter->setPen(QPen(QColor("#e7eaf0"),1.2));painter->setBrush(Qt::NoBrush);
        if(action_==CaptionAction::Minimize)painter->drawLine(QPointF(-5.25,0),QPointF(5.25,0));
        else if(action_==CaptionAction::Maximize)painter->drawRect(QRectF(-5.25,-5.25,10.5,10.5));
        else if(action_==CaptionAction::Close){painter->drawLine(QPointF(-5.25,-5.25),QPointF(5.25,5.25));painter->drawLine(QPointF(-5.25,5.25),QPointF(5.25,-5.25));}
        else {painter->drawPolyline(QPolygonF{QPointF(-2.25,-2.25),QPointF(-2.25,-5.25),QPointF(5.25,-5.25),QPointF(5.25,2.25),QPointF(2.25,2.25)});painter->drawRect(QRectF(-5.25,-2.25,7.5,7.5));}
        painter->restore();
    }
private:
    CaptionAction action_;
};
QIcon captionIcon(CaptionAction action) { return QIcon(new CaptionIcon(action)); }
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
    setWindowFlags(Qt::Window|Qt::FramelessWindowHint|Qt::WindowSystemMenuHint|Qt::WindowMinimizeButtonHint|Qt::WindowMaximizeButtonHint|Qt::WindowCloseButtonHint);
    setWindowTitle("LanChat-Next");
    resize(1300, 800);
    // 专用板端账号由登录后的设置面板管理，与普通聊天账号分开。
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
    tray_=new QSystemTrayIcon(QIcon(":/icons/lanchat-256.png"),this);tray_->setToolTip("LanChat-Next");
    auto* trayMenu=new QMenu(this);trayMenu->addAction("打开 LanChat",this,[this]{showNormal();raise();activateWindow();});
    trayMenu->addAction("退出 LanChat（本客户端启动的主机也会停止）",this,[this]{quitting_=true;close();});tray_->setContextMenu(trayMenu);
    connect(tray_,&QSystemTrayIcon::activated,this,[this](QSystemTrayIcon::ActivationReason reason){if(reason==QSystemTrayIcon::DoubleClick||reason==QSystemTrayIcon::Trigger){showNormal();raise();activateWindow();}});
    applyPreferences();

    connect(login_, &LoginView::registerRequested, this, [this] { if(configureLoginConnection(login_->serverAddress()))stack_->setCurrentWidget(reg_); });
    connect(login_,&LoginView::connectionRequested,this,[this](const QString& address){configureLoginConnection(address);});
    connect(reg_, &RegisterView::backRequested, this, [this] { stack_->setCurrentWidget(login_); });
    connect(login_, &LoginView::loginRequested, this, [this](const QString& u, const QString& p) {
        login_->showError("");
        if(!configureLoginConnection(login_->serverAddress()))return;
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
        contacts_->setAuthenticated(true);
        stack_->setCurrentIndex(2);
    });
    connect(sess_,&Session::stateChanged,this,[this](const QString&){
        if(selfId_>0&&sess_->selfId()<1){selfId_=0;contacts_->setAuthenticated(false);stack_->setCurrentWidget(login_);}
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
        if(m.fromId!=selfId_){
            const bool foreground=matches&&isVisible()&&!isMinimized()&&isActiveWindow()&&stack_->currentIndex()==2;
            if(preferences::shouldNotify(m.kind=="group",m.fromId==boardId_,foreground))notify(senderName(m.fromId),m.contentType=="text"?m.text:QString("收到附件：%1").arg(m.attachment.value("file_name").toString()));
            if(m.contentType!="text")transfers_->autoDownload(m.attachment);
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
    connect(contacts_, &ContactsPanel::settingsRequested, this, &MainWindow::showSettings);
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

MainWindow::~MainWindow(){
    // 设置面板依赖传输管理器和桥。先停止桥并销毁面板，避免 QObject 按创建顺序
    // 释放传输管理器后，桥的退出信号又让面板读取已经释放的对象。
    bridge_->stop();if(settingsPage_){delete settingsPage_;settingsPage_=nullptr;extendedSettings_=nullptr;boardPanel_=nullptr;}
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
    titleBar_->setObjectName("windowTitleBar");
    titleBar_->setFixedHeight(40);
    titleBar_->setAttribute(Qt::WA_StyledBackground, true);
    titleBar_->setStyleSheet("QWidget#windowTitleBar {background-color:#0E1014;}");
    auto* lay = new QHBoxLayout(titleBar_);
    lay->setContentsMargins(12, 0, 0, 0);
    lay->setSpacing(4);
    auto* title = new QLabel("LanChat-Next", titleBar_);
    title->setObjectName("headerTitle");
    title->setStyleSheet("background:transparent;");
    title->setAttribute(Qt::WA_TransparentForMouseEvents);
    titleBar_->installEventFilter(this);
    lay->addWidget(title);
    lay->addStretch(1);
    auto* controls=new QWidget(titleBar_);controls->setObjectName("windowCaptionButtons");controls->setStyleSheet("QWidget#windowCaptionButtons {background:transparent;}");
    auto* controlLayout=new QHBoxLayout(controls);controlLayout->setContentsMargins(0,0,0,0);controlLayout->setSpacing(0);
    auto* min = new QPushButton(controls);min->setObjectName("windowMinimizeButton");min->setIcon(captionIcon(CaptionAction::Minimize));min->setToolTip("最小化");min->setAccessibleName("最小化");
    auto* max = new QPushButton(controls);max->setObjectName("windowMaximizeButton");maximizeBtn_=max;
    auto* close = new QPushButton(controls);closeBtn_=close;close->setObjectName("windowCloseButton");close->setIcon(captionIcon(CaptionAction::Close));close->setToolTip("关闭");close->setAccessibleName("关闭");
    for (auto* b : {min, max, close}) {b->setProperty("windowControl",true);b->setFixedSize(46,40);b->setIconSize(QSize(12,12));controlLayout->addWidget(b);}
    close->installEventFilter(this);
    connect(min, &QPushButton::clicked, this, [this] { showMinimized(); });
    connect(max, &QPushButton::clicked, this, &MainWindow::toggleMaximized);
    connect(close, &QPushButton::clicked, this, [this] { this->close(); });
    lay->addWidget(controls);
    updateWindowControls();
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
        lay->insertWidget(lay->count() - 1, consoleBtn_);
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
    if(selfId_<1)return;
    ensureSettingsPage();
    // One persistent page keeps the draft, pending delivery and scroll position intact.
    if(!boardPanel_){
        boardPanel_=new BoardConnectionPanel(bridge_,settingsStack_,sess_);
        settingsStack_->addWidget(boardPanel_);
        connect(boardPanel_,&BoardConnectionPanel::aiChatRequested,this,[this](int accountId){
            if(selfId_<1){stack_->setCurrentWidget(login_);return;}
            openConversation(accountId,false);stack_->setCurrentIndex(2);
        });
        connect(boardPanel_,&BoardConnectionPanel::returnRequested,this,[this]{
            stack_->setCurrentIndex(selfId_>0?2:0);
        });
    }
    serverSettingsBtn_->setChecked(false);boardSettingsBtn_->setChecked(true);
    settingsStack_->setCurrentWidget(boardPanel_);stack_->setCurrentWidget(settingsPage_);
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

void MainWindow::updateWindowControls(){
    if(!maximizeBtn_)return;const bool restored=isMaximized();maximizeBtn_->setProperty("captionAction",restored?"restore":"maximize");
    maximizeBtn_->setIcon(captionIcon(restored?CaptionAction::Restore:CaptionAction::Maximize));
    maximizeBtn_->setToolTip(restored?"还原":"最大化");maximizeBtn_->setAccessibleName(maximizeBtn_->toolTip());
}
void MainWindow::toggleMaximized(){
#ifdef Q_OS_WIN
    if(QGuiApplication::platformName()=="windows"){
        const auto hwnd=reinterpret_cast<HWND>(winId());SendMessageW(hwnd,WM_SYSCOMMAND,IsZoomed(hwnd)?SC_RESTORE:SC_MAXIMIZE,0);return;
    }
#endif
    isMaximized()?showNormal():showMaximized();
}
void MainWindow::changeEvent(QEvent* event){QMainWindow::changeEvent(event);if(event->type()==QEvent::WindowStateChange)updateWindowControls();}
void MainWindow::showEvent(QShowEvent* event){QMainWindow::showEvent(event);configureNativeFrame();}
bool MainWindow::eventFilter(QObject* watched,QEvent* event){
    if(watched==closeBtn_&&(event->type()==QEvent::Enter||event->type()==QEvent::Leave))setCloseHover(event->type()==QEvent::Enter);
    // 非 Windows 平台也使用窗口管理器的移动流程；Windows 走下面的原生 HTCAPTION。
    if(watched==titleBar_&&event->type()==QEvent::MouseButtonDblClick){
        if(static_cast<QMouseEvent*>(event)->button()==Qt::LeftButton){toggleMaximized();return true;}
    }
    if(watched==titleBar_&&event->type()==QEvent::MouseButtonPress&&QGuiApplication::platformName()!="windows"){
        if(static_cast<QMouseEvent*>(event)->button()==Qt::LeftButton&&windowHandle()&&windowHandle()->startSystemMove())return true;
    }
    return QMainWindow::eventFilter(watched,event);
}
void MainWindow::setMaximizeHover(bool hovered){
    if(!maximizeBtn_||maximizeBtn_->property("nativeHover").toBool()==hovered)return;
    maximizeBtn_->setProperty("nativeHover",hovered);maximizeBtn_->style()->unpolish(maximizeBtn_);maximizeBtn_->style()->polish(maximizeBtn_);maximizeBtn_->update();
}
void MainWindow::setCloseHover(bool hovered){
    if(!closeBtn_||closeBtn_->property("nativeHover").toBool()==hovered)return;
    closeBtn_->setProperty("nativeHover",hovered);closeBtn_->style()->unpolish(closeBtn_);closeBtn_->style()->polish(closeBtn_);closeBtn_->update();
}
void MainWindow::configureNativeFrame(){
#ifdef Q_OS_WIN
    if(QGuiApplication::platformName()!="windows")return;const auto id=quintptr(winId());if(nativeFrameId_==id)return;nativeFrameId_=id;
    const auto hwnd=reinterpret_cast<HWND>(id);const auto style=GetWindowLongPtrW(hwnd,GWL_STYLE);
    SetWindowLongPtrW(hwnd,GWL_STYLE,style|WS_CAPTION|WS_THICKFRAME|WS_SYSMENU|WS_MINIMIZEBOX|WS_MAXIMIZEBOX);
    const BOOL dark=TRUE;DwmSetWindowAttribute(hwnd,20,&dark,sizeof(dark));const MARGINS shadow{1,1,1,1};DwmExtendFrameIntoClientArea(hwnd,&shadow);
    SetWindowPos(hwnd,nullptr,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE|SWP_NOZORDER|SWP_NOACTIVATE|SWP_FRAMECHANGED);
#endif
}
bool MainWindow::nativeEvent(const QByteArray& eventType,void* message,qintptr* result){
#ifdef Q_OS_WIN
    const auto* msg=static_cast<MSG*>(message);const auto hwnd=msg->hwnd;
    auto localPoint=[this,hwnd](LPARAM position,bool screen){POINT point{GET_X_LPARAM(position),GET_Y_LPARAM(position)};if(screen)ScreenToClient(hwnd,&point);return QPoint(qRound(point.x/devicePixelRatioF()),qRound(point.y/devicePixelRatioF()));};
    if(msg->message==WM_NCCALCSIZE&&msg->wParam){
        auto* params=reinterpret_cast<NCCALCSIZE_PARAMS*>(msg->lParam);
        if(IsZoomed(hwnd)){MONITORINFO monitor{sizeof(MONITORINFO)};if(GetMonitorInfoW(MonitorFromRect(&params->rgrc[0],MONITOR_DEFAULTTONEAREST),&monitor))params->rgrc[0]=monitor.rcWork;}
        *result=0;return true;
    }
    if(msg->message==WM_NCHITTEST&&titleBar_){
        const auto point=localPoint(msg->lParam,true);RECT bounds{};GetWindowRect(hwnd,&bounds);const int x=GET_X_LPARAM(msg->lParam),y=GET_Y_LPARAM(msg->lParam);
        if(!IsZoomed(hwnd)&&!isFullScreen()){
            const int border=qMax(1,qRound(6*devicePixelRatioF()));const bool left=x<bounds.left+border,right=x>=bounds.right-border,top=y<bounds.top+border,bottom=y>=bounds.bottom-border;
            if(top||bottom||left||right){*result=top?(left?HTTOPLEFT:right?HTTOPRIGHT:HTTOP):bottom?(left?HTBOTTOMLEFT:right?HTBOTTOMRIGHT:HTBOTTOM):left?HTLEFT:HTRIGHT;return true;}
        }
        if(maximizeBtn_&&maximizeBtn_->rect().contains(maximizeBtn_->mapFrom(this,point))){*result=HTMAXBUTTON;return true;}
        if(titleBar_->rect().contains(titleBar_->mapFrom(this,point))){
            for(auto* child=childAt(point);child&&child!=titleBar_;child=child->parentWidget())if(qobject_cast<QAbstractButton*>(child)){*result=HTCLIENT;return true;}
            *result=HTCAPTION;return true;
        }
        *result=HTCLIENT;return true;
    }
    if(msg->message==WM_NCMOUSEMOVE){
        setMaximizeHover(msg->wParam==HTMAXBUTTON);
        if(msg->wParam==HTMAXBUTTON){TRACKMOUSEEVENT tracking{sizeof(TRACKMOUSEEVENT),TME_LEAVE|TME_NONCLIENT,hwnd,0};TrackMouseEvent(&tracking);LRESULT value=0;DwmDefWindowProc(hwnd,msg->message,msg->wParam,msg->lParam,&value);}
    }
    if(msg->message==WM_NCMOUSELEAVE)setMaximizeHover(false);
    if((msg->message==WM_NCLBUTTONDOWN||msg->message==WM_NCLBUTTONDBLCLK)&&msg->wParam==HTMAXBUTTON){
        maximizePressed_=true;maximizeBtn_->setDown(true);SetCapture(hwnd);*result=0;return true;
    }
    if(maximizePressed_&&msg->message==WM_MOUSEMOVE){maximizeBtn_->setDown(maximizeBtn_->rect().contains(maximizeBtn_->mapFrom(this,localPoint(msg->lParam,false))));*result=0;return true;}
    if(maximizePressed_&&(msg->message==WM_LBUTTONUP||msg->message==WM_NCLBUTTONUP)){
        const bool activate=maximizeBtn_->rect().contains(maximizeBtn_->mapFrom(this,localPoint(msg->lParam,msg->message==WM_NCLBUTTONUP)));
        maximizePressed_=false;maximizeBtn_->setDown(false);ReleaseCapture();if(activate)maximizeBtn_->click();*result=0;return true;
    }
    if(msg->message==WM_CAPTURECHANGED){maximizePressed_=false;if(maximizeBtn_)maximizeBtn_->setDown(false);}
#endif
    return QMainWindow::nativeEvent(eventType,message,result);
}

void MainWindow::loadDemoData(bool withMessages, bool openBoard)
{
    selfId_ = 1;
    contacts_->setAuthenticated(true);
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
void MainWindow::ensureSettingsPage() {
    if(settingsPage_)return;
    settingsPage_=new QWidget(stack_);settingsPage_->setObjectName("settingsPage");
    auto* layout=new QHBoxLayout(settingsPage_);layout->setContentsMargins(0,0,0,0);layout->setSpacing(0);
    auto* navigation=new QWidget(settingsPage_);navigation->setObjectName("settingsNavigation");navigation->setFixedWidth(268);navigation->setAttribute(Qt::WA_StyledBackground);
    auto* side=new QVBoxLayout(navigation);side->setContentsMargins(16,24,16,16);side->setSpacing(10);
    auto* heading=new QLabel("设置",navigation);heading->setObjectName("headerTitle");side->addWidget(heading);
    auto* account=new QLabel(QString("已登录 · 账号 #%1").arg(selfId_),navigation);account->setObjectName("settingsAccountLabel");side->addWidget(account);side->addSpacing(18);
    serverSettingsBtn_=new QPushButton("服务器",navigation);serverSettingsBtn_->setObjectName("serverSettingsButton");
    boardSettingsBtn_=new QPushButton("板端连接",navigation);boardSettingsBtn_->setObjectName("boardConnectionButton");
    for(auto* button:{serverSettingsBtn_,boardSettingsBtn_}){button->setProperty("settingsNavigation",true);button->setCheckable(true);button->setCursor(Qt::PointingHandCursor);side->addWidget(button);}
    auto* selection=new QButtonGroup(navigation);selection->setExclusive(true);selection->addButton(serverSettingsBtn_);selection->addButton(boardSettingsBtn_);
    const QStringList sectionLabels={"账号与连接","通知与窗口","附件与存储","AI 与会话","显示与设备","关于与诊断"};
    const QStringList sectionIds={"accountSettingsButton","notificationSettingsButton","storageSettingsButton","aiSettingsButton","displaySettingsButton","aboutSettingsButton"};
    for(int i=0;i<sectionLabels.size();++i){auto* button=new QPushButton(sectionLabels[i],navigation);button->setObjectName(sectionIds[i]);button->setCheckable(true);button->setProperty("settingsNavigation",true);button->setCursor(Qt::PointingHandCursor);selection->addButton(button);side->insertWidget(i==0?3:side->count(),button);connect(button,&QPushButton::clicked,this,[this,i]{showExtendedSettings(i);});}
    side->addStretch();auto* back=new QPushButton("返回聊天",navigation);back->setObjectName("settingsReturnToChat");side->addWidget(back);
    settingsStack_=new QStackedWidget(settingsPage_);settingsStack_->setObjectName("settingsContents");layout->addWidget(navigation);layout->addWidget(settingsStack_,1);stack_->addWidget(settingsPage_);
    connect(back,&QPushButton::clicked,this,[this]{stack_->setCurrentIndex(selfId_>0?2:0);});
    connect(serverSettingsBtn_,&QPushButton::clicked,this,&MainWindow::showConnectionSettings);
    connect(boardSettingsBtn_,&QPushButton::clicked,this,&MainWindow::showBoardConnection);
    connect(bridge_,&BoardConnection::changed,boardSettingsBtn_,[this]{
        boardSettingsBtn_->setText(bridge_->snapshot().value("board_connected").toBool()?"板端连接 · 已连接":"板端连接");boardSettingsBtn_->setToolTip(bridge_->state());
    });
}
void MainWindow::showSettings() {
    if(selfId_<1)return;ensureSettingsPage();
    settingsPage_->findChild<QLabel*>("settingsAccountLabel")->setText(QString("已登录 · 账号 #%1").arg(selfId_));
    showExtendedSettings(0);
}
void MainWindow::showExtendedSettings(int section){
    if(selfId_<1)return;ensureSettingsPage();
    if(!extendedSettings_){extendedSettings_=new SettingsPanel(sess_,bridge_,transfers_,settingsStack_);settingsStack_->addWidget(extendedSettings_);extendedSettings_->setHostState(hostState_);
        connect(extendedSettings_,&SettingsPanel::logoutRequested,this,&MainWindow::logoutAccount);
        connect(extendedSettings_,&SettingsPanel::reconnectRequested,this,[this]{QSettings s;const auto host=s.value("network/hosted",true).toBool()?QString("127.0.0.1"):s.value("network/host").toString();sess_->connectToServer(host,quint16(s.value("network/port",12346).toInt()));});
        connect(extendedSettings_,&SettingsPanel::boardRequested,this,&MainWindow::showBoardConnection);
        connect(extendedSettings_,&SettingsPanel::preferencesChanged,this,&MainWindow::applyPreferences);
        connect(extendedSettings_,&SettingsPanel::testNotificationRequested,this,[this]{notify("LanChat 通知测试","这是一条本机测试通知。",true);});}
    const QStringList ids={"accountSettingsButton","notificationSettingsButton","storageSettingsButton","aiSettingsButton","displaySettingsButton","aboutSettingsButton"};
    settingsPage_->findChild<QPushButton*>(ids[section])->setChecked(true);extendedSettings_->setSection(section);settingsStack_->setCurrentWidget(extendedSettings_);stack_->setCurrentWidget(settingsPage_);
}
void MainWindow::setHostState(const QString& state){hostState_=state;if(extendedSettings_)extendedSettings_->setHostState(state);if(tray_)tray_->setToolTip("LanChat-Next\n"+state);}
void MainWindow::applyPreferences(){if(tray_&&QSystemTrayIcon::isSystemTrayAvailable())tray_->show();if(!credentials::load().remembered)login_->refreshSavedLogin();}
void MainWindow::notify(const QString& title,const QString& text,bool test){
    if(tray_&&QSystemTrayIcon::isSystemTrayAvailable()){tray_->show();tray_->showMessage(title,text.left(180),QSystemTrayIcon::Information,5000);}
    else if(test)QMessageBox::information(this,"通知测试","当前桌面未提供系统托盘，通知不可显示。Windows 正常桌面会使用托盘气泡。");
    if(QSettings().value("preferences/sound",false).toBool())QApplication::beep();
}
void MainWindow::logoutAccount(){
    sess_->logout();pendingPass_.clear();pendingUser_.clear();selfId_=0;contacts_->setAuthenticated(false);store_->clear();currentPeer_=0;currentIsGroup_=false;groups_.clear();groupMembers_.clear();historyCursors_.clear();unread_.clear();alerts_.clear();lastMsg_.clear();chat_->clear();refreshContacts();refreshGroups();refreshMembers();refreshAlerts();login_->refreshSavedLogin();stack_->setCurrentWidget(login_);
}
void MainWindow::closeEvent(QCloseEvent* event){
    if(!quitting_&&QSettings().value("preferences/close_to_tray",false).toBool()&&QSystemTrayIcon::isSystemTrayAvailable()){hide();event->ignore();return;}
    int pending=0;for(const auto& task:transfers_->tasks())if(task.state!="已完成")++pending;
    if(pending>0&&QMessageBox::question(this,"退出 LanChat",QString("还有 %1 项未完成传输，退出后将暂停，下次可继续。确认退出？").arg(pending),QMessageBox::Yes|QMessageBox::No,QMessageBox::No)!=QMessageBox::Yes){quitting_=false;event->ignore();return;}
    if(tray_)tray_->hide();event->accept();
}
void MainWindow::showConnectionSettings(){
    if(selfId_<1)return;ensureSettingsPage();
    if(!serverPage_){
        serverPage_=new QWidget(settingsStack_);serverPage_->setObjectName("serverSettingsPage");auto* outer=new QVBoxLayout(serverPage_);outer->setContentsMargins(28,28,28,28);
        auto* formWidget=new QFrame(serverPage_);formWidget->setObjectName("settingsCard");formWidget->setMaximumWidth(640);auto* form=new QFormLayout(formWidget);form->setContentsMargins(24,24,24,24);form->setVerticalSpacing(16);
        auto* title=new QLabel("服务器连接",formWidget);title->setObjectName("headerTitle");form->addRow(title);
        auto* mode=new QComboBox(formWidget);mode->setObjectName("serverMode");mode->addItems({"本机托管（其他电脑连接本机）","连接局域网 / Tailscale 消息主机"});
        auto* host=new QLineEdit(formWidget);host->setObjectName("serverHost");
        auto* port=new QSpinBox(formWidget);port->setObjectName("serverPort");port->setRange(1024,65534);
        auto* maximum=new QSpinBox(formWidget);maximum->setObjectName("attachmentLimit");maximum->setRange(1,1024);maximum->setSuffix(" MB");
        auto* hint=new QLabel("所有用户连接同一台常开消息主机。\n本机托管需允许专用网络入站连接。\n修改连接后需在目标服务器重新登录。",formWidget);hint->setObjectName("settingsHint");hint->setWordWrap(true);
        form->addRow("服务器模式",mode);form->addRow("主机地址",host);form->addRow("TCP 端口",port);form->addRow("单附件上限",maximum);form->addRow(hint);
        auto* apply=new QPushButton("保存并重新连接",formWidget);apply->setObjectName("serverSettingsApply");form->addRow(apply);
        connect(mode,&QComboBox::currentIndexChanged,host,[host](int index){host->setEnabled(index==1);});
        outer->addWidget(formWidget,0,Qt::AlignHCenter);outer->addStretch();settingsStack_->addWidget(serverPage_);
        connect(apply,&QPushButton::clicked,this,[this,mode,host,port,maximum]{
            if(selfId_<1)return;const bool hosted=mode->currentIndex()==0;const QString address=hosted?"127.0.0.1":host->text().trimmed();
            if(address.isEmpty()){host->setFocus();return;}changeConnection(address,quint16(port->value()),hosted,maximum->value());
        });
    }
    QSettings settings;auto* mode=serverPage_->findChild<QComboBox*>("serverMode");mode->setCurrentIndex(settings.value("network/hosted",true).toBool()?0:1);
    auto* host=serverPage_->findChild<QLineEdit*>("serverHost");host->setText(settings.value("network/host","127.0.0.1").toString());host->setEnabled(mode->currentIndex()==1);
    serverPage_->findChild<QSpinBox*>("serverPort")->setValue(settings.value("network/port",12346).toInt());serverPage_->findChild<QSpinBox*>("attachmentLimit")->setValue(settings.value("network/attachment_max_mb",100).toInt());
    serverSettingsBtn_->setChecked(true);boardSettingsBtn_->setChecked(false);settingsStack_->setCurrentWidget(serverPage_);stack_->setCurrentWidget(settingsPage_);
}
bool MainWindow::configureLoginConnection(const QString& address) {
    const QUrl url("tcp://"+address);const int port=url.port(12346);const QString host=url.host();
    if(!url.isValid()||host.isEmpty()||!url.userInfo().isEmpty()||!url.path().isEmpty()||url.hasQuery()||url.hasFragment()||port<1024||port>65534){login_->showError("请输入有效的主机地址和端口，例如 100.64.0.1:12346");return false;}
    QSettings settings;const bool hosted=host=="127.0.0.1"||host=="localhost"||host=="::1";
    const QString current=settings.value("network/hosted",true).toBool()?"127.0.0.1":settings.value("network/host","127.0.0.1").toString();
    if(current!=host||settings.value("network/port",12346).toInt()!=port)changeConnection(host,quint16(port),hosted,settings.value("network/attachment_max_mb",100).toInt());
    login_->showError("");return true;
}
void MainWindow::changeConnection(const QString& address,quint16 port,bool hosted,int maximum) {
    QSettings cfg;cfg.setValue("network/hosted",hosted);cfg.setValue("network/host",address);cfg.setValue("network/port",port);cfg.setValue("network/attachment_max_mb",maximum);
    sess_->logout();store_->clear();selfId_=0;contacts_->setAuthenticated(false);currentPeer_=0;currentIsGroup_=false;groups_.clear();groupMembers_.clear();historyCursors_.clear();unread_.clear();alerts_.clear();lastMsg_.clear();chat_->clear();refreshContacts();refreshGroups();refreshMembers();refreshAlerts();
    auto bridgeConfig=bridge_->config();const bool wasRunning=bridge_->running();bridge_->stop();
    const bool changedServer=bridgeConfig.serverHost!=address||bridgeConfig.serverPort!=port;bridgeConfig.serverHost=address;bridgeConfig.serverPort=port;
    if(changedServer){
        bridgeConfig.accountId=0;bridgeConfig.password.clear();bridgeConfig.target=0;bridgeConfig.groupId=0;bridgeConfig.autoStart=false;boardId_=0;QSettings().setValue("board_account_id",0);
        // IDs belong to one server. A new page must not reapply the previous server's draft settings.
        if(boardPanel_){settingsStack_->removeWidget(boardPanel_);boardPanel_->deleteLater();boardPanel_=nullptr;}
    }
    bridge_->configure(bridgeConfig);if(wasRunning&&!changedServer)bridge_->start();
    login_->setServerAddress((address.contains(':')?'['+address+']':address)+":"+QString::number(port));
    stack_->setCurrentWidget(login_);emit connectionRequested(address,port,hosted,maximum);
}
