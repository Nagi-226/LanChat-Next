#include "ui/register_view.h"

#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>

RegisterView::RegisterView(QWidget* parent) : QWidget(parent)
{
    auto* lay = new QVBoxLayout(this);
    lay->setSpacing(10);
    auto* title = new QLabel("注册新账户", this);
    title->setObjectName("title");
    nick_ = new QLineEdit(this);
    nick_->setPlaceholderText("昵称（即登录用户名）");
    pass_ = new QLineEdit(this);
    pass_->setPlaceholderText("密码");
    pass_->setEchoMode(QLineEdit::Password);
    confirm_ = new QLineEdit(this);
    confirm_->setPlaceholderText("确认密码");
    confirm_->setEchoMode(QLineEdit::Password);
    auto* go = new QPushButton("注册", this);
    go->setObjectName("primary");
    err_ = new QLabel(this);
    err_->setObjectName("err");
    auto* back = new QPushButton("返回登录", this);
    back->setFlat(true);
    lay->addStretch(1);
    lay->addWidget(title, 0, Qt::AlignHCenter);
    lay->addWidget(nick_);
    lay->addWidget(pass_);
    lay->addWidget(confirm_);
    lay->addWidget(go);
    lay->addWidget(err_, 0, Qt::AlignHCenter);
    lay->addWidget(back, 0, Qt::AlignHCenter);
    lay->addStretch(2);

    connect(go, &QPushButton::clicked, this, [this] {
        if (pass_->text() != confirm_->text()) {
            showError("两次密码不一致");
            return;
        }
        emit registerSubmitted(nick_->text().trimmed(), pass_->text());
    });
    connect(back, &QPushButton::clicked, this, &RegisterView::backRequested);
}

void RegisterView::showError(const QString& msg) { err_->setText(msg); }