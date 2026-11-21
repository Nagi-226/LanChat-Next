#include "ui/login_view.h"

#include "app/credentials.h"

#include <QCheckBox>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>

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
    connect(reg, &QPushButton::clicked, this, &LoginView::registerRequested);
}

void LoginView::showError(const QString& msg) { err_->setText(msg); }

bool LoginView::rememberChecked() const { return remember_->isChecked(); }