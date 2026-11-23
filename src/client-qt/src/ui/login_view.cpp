#include "ui/login_view.h"

#include "app/credentials.h"

#include <QCheckBox>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSettings>

LoginView::LoginView(QWidget* parent) : QWidget(parent)
{
    // 外层垂直居中；内层固定宽卡片水平居中 → 正常宽窄的居中矩形，不铺满全宽
    auto* outer = new QVBoxLayout(this);
    outer->addStretch(1);
    auto* card = new QWidget(this);
    card->setFixedWidth(380);
    auto* lay = new QVBoxLayout(card);
    lay->setSpacing(10);
    auto* title = new QLabel("LanChat-Next", card);
    title->setObjectName("title");
    auto* serverLabel=new QLabel("服务器地址",card);serverLabel->setObjectName("timestamp");
    auto* serverRow=new QHBoxLayout;serverRow->setSpacing(8);
    server_=new QLineEdit(card);server_->setObjectName("loginServerAddress");server_->setAccessibleName("服务器地址");server_->setPlaceholderText("主机地址:12346");server_->setToolTip("填写局域网或 Tailscale 主机地址和端口。完整设置在登录后查看。");
    QSettings settings;QString host=settings.value("network/hosted",true).toBool()?"127.0.0.1":settings.value("network/host","127.0.0.1").toString();if(host.contains(':'))host='['+host+']';
    server_->setText(host+":"+settings.value("network/port",12346).toString());
    auto* connectButton=new QPushButton("连接",card);connectButton->setObjectName("loginConnectButton");serverRow->addWidget(server_,1);serverRow->addWidget(connectButton);
    user_ = new QLineEdit(card);
    user_->setPlaceholderText("用户名");
    pass_ = new QLineEdit(card);
    pass_->setPlaceholderText("密码");
    pass_->setEchoMode(QLineEdit::Password);
    // 记住账户：勾选后下次启动预填用户名和密码（密码经 DPAPI 加密落盘，见 credentials.cpp）
    remember_ = new QCheckBox("记住账户，下次自动填写", card);
    remember_->setObjectName("remember");
    const auto saved = credentials::load();
    if (saved.remembered) {
        user_->setText(saved.username);
        pass_->setText(saved.password);
        remember_->setChecked(true);
    }
    auto* go = new QPushButton("登录", card);
    go->setObjectName("primary");
    err_ = new QLabel(card);
    err_->setObjectName("err");
    auto* reg = new QPushButton("注册新账户", card);
    reg->setFlat(true);
    lay->addWidget(title, 0, Qt::AlignHCenter);
    lay->addWidget(serverLabel);lay->addLayout(serverRow);
    lay->addWidget(user_);
    lay->addWidget(pass_);
    lay->addWidget(remember_);
    lay->addWidget(go);
    lay->addWidget(err_, 0, Qt::AlignHCenter);
    lay->addWidget(reg, 0, Qt::AlignHCenter);
    outer->addWidget(card, 0, Qt::AlignHCenter);
    outer->addStretch(1);

    connect(go, &QPushButton::clicked, this, [this] {
        emit loginRequested(user_->text().trimmed(), pass_->text());
    });
    connect(pass_, &QLineEdit::returnPressed, go, &QPushButton::click);
    connect(connectButton,&QPushButton::clicked,this,[this]{emit connectionRequested(serverAddress());});
    connect(server_,&QLineEdit::returnPressed,connectButton,&QPushButton::click);
    connect(reg, &QPushButton::clicked, this, &LoginView::registerRequested);
}

void LoginView::showError(const QString& msg) { err_->setText(msg); }

bool LoginView::rememberChecked() const { return remember_->isChecked(); }
QString LoginView::serverAddress() const {return server_->text().trimmed();}
void LoginView::setServerAddress(const QString& address) {server_->setText(address);}
