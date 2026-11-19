#include "ui/login_view.h"

#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>

LoginView::LoginView(QWidget* parent) : QWidget(parent)
{
    auto* lay = new QVBoxLayout(this);
    lay->setSpacing(10);
    auto* title = new QLabel("LanChat-Next", this);
    title->setObjectName("title");
    user_ = new QLineEdit(this);
    user_->setPlaceholderText("用户名");
    pass_ = new QLineEdit(this);
    pass_->setPlaceholderText("密码");
    pass_->setEchoMode(QLineEdit::Password);
    auto* go = new QPushButton("登录", this);
    go->setObjectName("primary");
    err_ = new QLabel(this);
    err_->setObjectName("err");
    auto* reg = new QPushButton("注册新账户", this);
    reg->setFlat(true);
    lay->addStretch(1);
    lay->addWidget(title, 0, Qt::AlignHCenter);
    lay->addWidget(user_);
    lay->addWidget(pass_);
    lay->addWidget(go);
    lay->addWidget(err_, 0, Qt::AlignHCenter);
    lay->addWidget(reg, 0, Qt::AlignHCenter);
    lay->addStretch(2);

    connect(go, &QPushButton::clicked, this, [this] {
        emit loginRequested(user_->text().trimmed(), pass_->text());
    });
    connect(pass_, &QLineEdit::returnPressed, go, &QPushButton::click);
    connect(reg, &QPushButton::clicked, this, &LoginView::registerRequested);
}

void LoginView::showError(const QString& msg) { err_->setText(msg); }