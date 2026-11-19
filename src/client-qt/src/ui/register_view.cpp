#include "ui/register_view.h"

#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>

RegisterView::RegisterView(QWidget* parent) : QWidget(parent)
{
    // 同登录页：水平+垂直居中的固定宽卡片
    auto* outer = new QVBoxLayout(this);
    outer->addStretch(1);
    auto* card = new QWidget(this);
    card->setFixedWidth(380);
    auto* lay = new QVBoxLayout(card);
    lay->setSpacing(10);
    auto* title = new QLabel("注册新账户", card);
    title->setObjectName("title");
    nick_ = new QLineEdit(card);
    nick_->setPlaceholderText("昵称（即登录用户名）");
    pass_ = new QLineEdit(card);
    pass_->setPlaceholderText("密码");
    pass_->setEchoMode(QLineEdit::Password);
    confirm_ = new QLineEdit(card);
    confirm_->setPlaceholderText("确认密码");
    confirm_->setEchoMode(QLineEdit::Password);
    auto* go = new QPushButton("注册", card);
    go->setObjectName("primary");
    err_ = new QLabel(card);
    err_->setObjectName("err");
    auto* back = new QPushButton("返回登录", card);
    back->setFlat(true);
    lay->addWidget(title, 0, Qt::AlignHCenter);
    lay->addWidget(nick_);
    lay->addWidget(pass_);
    lay->addWidget(confirm_);
    lay->addWidget(go);
    lay->addWidget(err_, 0, Qt::AlignHCenter);
    lay->addWidget(back, 0, Qt::AlignHCenter);
    outer->addWidget(card, 0, Qt::AlignHCenter);
    outer->addStretch(1);

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