#include "ui/board_connection_panel.h"
#include "app/board_connection.h"
#include "net/session.h"
#include <QPlainTextEdit>
#include <QJsonArray>
#include <QCheckBox>
#include <QDateTime>
#include <QFileDialog>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QHostAddress>
#include <QLabel>
#include <QLineEdit>
#include <QNetworkInterface>
#include <QPushButton>
#include <QSpinBox>
#include <QTimer>
#include <QVBoxLayout>
#include <QScrollArea>
#include <QComboBox>
#include <QGridLayout>

BoardConnectionPanel::BoardConnectionPanel(BoardConnection* bridge, QWidget* parent, Session* session)
    : QWidget(parent), bridge_(bridge), session_(session)
{
    setObjectName("boardConnectionPanel");
    setAttribute(Qt::WA_StyledBackground,true);
    auto* centered = new QHBoxLayout(this);
    centered->setContentsMargins(0,0,0,0);
    auto* column = new QWidget(this);
    column->setMaximumWidth(960);
    column->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);
    centered->addStretch();
    centered->addWidget(column,1);
    centered->addStretch();
    auto* outer = new QVBoxLayout(column);
    auto* scroll = new QScrollArea(column);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    auto* content = new QWidget(scroll);
    scroll->setWidget(content);
    outer->addWidget(scroll,1);
    auto* layout = new QVBoxLayout(content);
    layout->setContentsMargins(24, 22, 24, 22);
    layout->setSpacing(14);
    auto* title = new QLabel("板端连接", this);
    title->setStyleSheet("font-size:22px;font-weight:600;color:#E7EAF0;background:transparent;");
    layout->addWidget(title);
    auto* description = new QLabel("板端状态与告警上行 · 本机消息下行显示\n电脑可用 5GHz，板子用同一路由器的 2.4GHz。", this);
    description->setStyleSheet("color:#8A94A4;background:transparent;");
    layout->addWidget(description);
    status_ = new QLabel(this);
    status_->setObjectName("boardConnectionStatus");
    status_->setStyleSheet("background:#1D212A;padding:14px;border-radius:8px;font-weight:600;");
    layout->addWidget(status_);
    stats_ = new QLabel(this);
    stats_->setObjectName("boardConnectionStats");
    stats_->setWordWrap(true);
    stats_->setStyleSheet("color:#8A94A4;background:transparent;");
    layout->addWidget(stats_);
    auto* form = new QFormLayout;
    form->setVerticalSpacing(12);
    auto spin = [this](int maximum, int value, const QString& name) {
        auto* input = new QSpinBox(this);
        input->setRange(0, maximum);
        input->setValue(value);
        input->setObjectName(name);
        input->setMinimumHeight(36);
        return input;
    };
    const auto& config = bridge_->config();
    port_ = spin(65535, config.listenPort, "boardListenPort");
    port_->setMinimum(1);
    target_ = spin(2147483647, config.target, "boardTargetId");
    group_ = spin(2147483647, config.groupId, "boardGroupId");
    target_->setSpecialValueText("关闭私聊转发");
    group_->setSpecialValueText("关闭群转发");
    form->addRow("板端上报端口", port_);
    form->addRow("私聊接收账号 ID", target_);
    form->addRow("告警群 ID", group_);
    account_ = new QLabel(this);
    account_->setWordWrap(true);
    form->addRow("专用板端账号", account_);
    automatic_ = new QCheckBox("启动 LanChat 时自动连接板端桥", this);
    automatic_->setObjectName("boardAutoStart");
    automatic_->setChecked(config.autoStart);
    form->addRow(automatic_);
    layout->addLayout(form);
    auto* hint = new QLabel("两个目标均为 0 时只监测连接；同时配置时群和私聊各发一份。\n登录后，未配置的私聊目标会自动填为当前账号。", this);
    hint->setWordWrap(true);
    hint->setStyleSheet("color:#8A94A4;background:transparent;");
    layout->addWidget(hint);
    consolePath_ = new QLineEdit(config.consolePath, this);
    consolePath_->setReadOnly(true);
    auto* browse = new QPushButton("选择 Console", this);
    auto* executableRow = new QHBoxLayout;
    executableRow->addWidget(consolePath_, 1);
    executableRow->addWidget(browse);
    layout->addLayout(executableRow);
    auto* messageTitle = new QLabel("发送到板端屏幕", this);
    layout->addWidget(messageTitle);
    message_ = new QPlainTextEdit(this);
    message_->setObjectName("boardMessageText");
    message_->setPlaceholderText("输入消息。正文最多 384 字节，约 128 个汉字。板端可查看详情。");
    message_->setMaximumHeight(84);
    layout->addWidget(message_);
    auto* messageRow = new QHBoxLayout;
    send_ = new QPushButton("发送到板端", this);
    send_->setObjectName("boardSendMessage");
    delivery_ = new QLabel(this);
    delivery_->setObjectName("boardMessageDelivery");
    delivery_->setWordWrap(true);
    messageRow->addWidget(send_); messageRow->addWidget(delivery_, 1);
    layout->addLayout(messageRow);
    controls_=new QWidget(this);
    auto* controlsLayout=new QVBoxLayout(controls_);
    controlsLayout->setContentsMargins(0,0,0,0);
    auto* pageRow=new QHBoxLayout;
    auto* page=new QComboBox(controls_);page->setObjectName("boardScreenPage");
    const QStringList pageNames={"总览","收件箱","PPM 感知","VVOS 链路","设备状态"};
    const QStringList pageActions={"home","inbox","ppm","link","device"};
    for(int i=0;i<pageNames.size();++i)page->addItem(pageNames[i],pageActions[i]);
    auto* language=new QComboBox(controls_);language->setObjectName("boardScreenLanguage");
    language->addItem("中文","lang_zh");language->addItem("English","lang_en");
    pageRow->addWidget(new QLabel("板端页面",controls_));pageRow->addWidget(page,1);pageRow->addWidget(language);
    controlsLayout->addLayout(pageRow);
    connect(page,&QComboBox::activated,this,[this,page](int index){bridge_->navigate(page->itemData(index).toString());});
    connect(language,&QComboBox::activated,this,[this,language](int index){bridge_->navigate(language->itemData(index).toString());});
    auto* navigation=new QGridLayout;
    const QStringList labels={"最新消息详情","上一条","下一条","正文向上","正文向下","本地标为已读"};
    const QStringList navActions={"detail","previous","next","scroll_up","scroll_down","read"};
    for(int i=0;i<labels.size();++i){
        auto* button=new QPushButton(labels[i],controls_);button->setObjectName("boardNav_"+navActions[i]);
        navigation->addWidget(button,i/3,i%3);
        connect(button,&QPushButton::clicked,this,[this,action=navActions[i]]{bridge_->navigate(action);});
    }
    controlsLayout->addLayout(navigation);layout->addWidget(controls_);
    view_=new QLabel("等待板端页面状态",this);view_->setObjectName("boardScreenState");view_->setWordWrap(true);
    layout->addWidget(view_);
    layout->addWidget(new QLabel("在上方「板端页面」切换五个界面；板端暂不支持触摸翻页。",this));
    connect(send_, &QPushButton::clicked, this, [this] {
        const QString text = message_->toPlainText().trimmed();
        if (!session_ || session_->selfId() < 1 || text.isEmpty() || text.toUtf8().size() > 384) return;
        pendingText_ = text; pendingId_.clear(); pendingSince_ = QDateTime::currentMSecsSinceEpoch();
        session_->sendPrivate(bridge_->config().accountId, text);
        delivery_->setText("正在提交到 LanChat…"); send_->setEnabled(false);
    });
    connect(message_, &QPlainTextEdit::textChanged, this, &BoardConnectionPanel::refresh);
    if (session_) connect(session_, &Session::messageReceived, this, [this](const QJsonObject& message) {
        if (pendingText_.isEmpty() || message.value("fromId").toInt() != session_->selfId()
            || message.value("toId").toInt() != bridge_->config().accountId || message.value("msg").toString() != pendingText_) return;
        pendingId_ = message.value("msg_id").toVariant().toString(); refresh();
    });
    connect(browse, &QPushButton::clicked, this, [this] {
        const auto path = QFileDialog::getOpenFileName(this, "选择 Console 桥", consolePath_->text(), "程序 (*.exe)");
        if (!path.isEmpty()) consolePath_->setText(path);
    });
    auto* actions = new QHBoxLayout;
    actions->setContentsMargins(24,0,24,0);
    auto* start = new QPushButton("保存并连接", this);
    start->setObjectName("primary");
    auto* stop = new QPushButton("断开连接", this);
    auto* close = new QPushButton("返回 LanChat", this);
    close->setObjectName("boardReturnToLanChat");
    actions->addWidget(start);
    actions->addWidget(stop);
    actions->addStretch();
    actions->addWidget(close);
    outer->addLayout(actions);
    connect(start, &QPushButton::clicked, this, [this] {
        auto config = bridge_->config();
        config.listenPort = static_cast<quint16>(port_->value());
        config.target = target_->value();
        config.groupId = group_->value();
        config.autoStart = automatic_->isChecked();
        config.consolePath = consolePath_->text();
        bridge_->configure(config);
        bridge_->start();
    });
    connect(stop, &QPushButton::clicked, bridge_, &BoardConnection::stop);
    connect(close, &QPushButton::clicked, this, &BoardConnectionPanel::returnRequested);
    connect(bridge_, &BoardConnection::changed, this, &BoardConnectionPanel::refresh);
    auto* timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &BoardConnectionPanel::refresh);
    timer->start(500);
    refresh();
}

void BoardConnectionPanel::refresh()
{
    status_->setText(bridge_->state());
    const auto data = bridge_->snapshot();
    QStringList addresses;
    for (const auto& address : QNetworkInterface::allAddresses()) {
        if (address.protocol() == QAbstractSocket::IPv4Protocol && !address.isLoopback()
            && !address.toString().startsWith("169.254.")) addresses << address.toString();
    }
    const qint64 last = data.value("last_frame_ms").toInteger();
    stats_->setText(QString("本机地址：%1\n板端地址：%2    已收 %3 帧    解码失败 %4\n最近上报：%5    LanChat：%6")
        .arg(addresses.join(" / "), data.value("board_peer").toString("未连接"))
        .arg(data.value("frames").toInteger()).arg(data.value("decode_failures").toInteger())
        .arg(last > 0 ? QDateTime::fromMSecsSinceEpoch(last).toString("HH:mm:ss") : "尚无有效帧",
             data.value("lanchat_state").toString("等待桥接状态")));
    account_->setText(bridge_->config().accountId > 0
        ? QString("%1（密码由 Windows 加密保存）").arg(bridge_->config().accountId)
        : "首次连接时自动创建，与聊天账号独立");
    const auto display = data.value("display").toObject();
    const bool ready = display.value("terminal_ready").toBool();
    controls_->setEnabled(ready&&display.value("navigation").toBool());
    const auto view=display.value("view").toObject();
    const QStringList pages={"总览","收件箱","PPM 感知","VVOS 链路","设备状态"};
    const int pageIndex=view.value("page").toInt(-1);
    view_->setText(pageIndex>=0&&pageIndex<pages.size()?QString("当前显示：%1%2 · %3 条消息 / %4 未读 · %5")
        .arg(pages[pageIndex],view.value("detail").toBool()?" / 详情":"").arg(view.value("count").toInt()).arg(view.value("unread").toInt()).arg(view.value("language").toString()):"等待板端页面状态");
    if(pageIndex>=0&&pageIndex<pages.size())controls_->findChild<QComboBox*>("boardScreenPage")->setCurrentIndex(pageIndex);
    controls_->findChild<QComboBox*>("boardScreenLanguage")->setCurrentIndex(view.value("language").toString()=="en"?1:0);
    const QString text = message_->toPlainText().trimmed();
    if (!pendingText_.isEmpty()) {
        for (const auto& item : display.value("deliveries").toArray()) {
            const auto result = item.toObject(); if (result.value("id").toString() != pendingId_) continue;
            const QString state = result.value("state").toString();
            if (state == "rendered") { delivery_->setText("板端已显示 · 消息 " + pendingId_); pendingText_.clear(); }
            else if (state == "accepted") delivery_->setText("板端已收到，等待屏幕显示…");
            else if (state == "rejected" || state == "expired") { delivery_->setText(result.value("reason").toString()); pendingText_.clear(); }
            else delivery_->setText("LanChat 已接收，等待板端…");
            break;
        }
        if (!pendingText_.isEmpty() && QDateTime::currentMSecsSinceEpoch() - pendingSince_ > 35000) {
            delivery_->setText("显示回执超时，连接恢复后可重发"); pendingText_.clear();
        }
    } else if (pendingSince_ == 0) {
        delivery_->setText(!session_ || session_->selfId() < 1 ? "先登录 LanChat" : !ready ? "等待板端终端固件连接" : "板端可接收消息");
    }
    send_->setEnabled(session_ && session_->selfId() > 0 && ready && pendingText_.isEmpty() && !text.isEmpty() && text.toUtf8().size() <= 384);
}
