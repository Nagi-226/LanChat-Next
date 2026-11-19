#include "ui/login_view.h"

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
    auto* go = new QPushButton("登录", card);
    go->setObjectName("primary");
    err_ = new QLabel(card);
    err_->setObjectName("err");
    auto* reg = new QPushButton("注册新账户", card);
    reg->setFlat(true);
    lay->addWidget(title, 0, Qt::AlignHCenter);
    lay->addWidget(user_);
    lay->addWidget(pass_);
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