#include "ui/members_panel.h"
#include "ui/avatar.h"

#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QScrollArea>
#include <QVBoxLayout>

MembersPanel::MembersPanel(QWidget* parent) : QWidget(parent)
{
    setFixedWidth(236);
    setAttribute(Qt::WA_StyledBackground, true);
    auto* outer = new QVBoxLayout(this);
    outer->setContentsMargins(0, 0, 0, 0);
    outer->setSpacing(0);
    auto* header = new QWidget(this);
    header->setFixedHeight(56);
    header->setAttribute(Qt::WA_StyledBackground, true);
    auto* hl = new QHBoxLayout(header);
    hl->setContentsMargins(16, 0, 16, 0);
    auto* t = new QLabel("成员", header);
    t->setObjectName("headerTitle");
    hl->addWidget(t);
    hl->addStretch(1);
    outer->addWidget(header);
    auto* div = new QFrame(this);
    div->setObjectName("divider");
    div->setFixedHeight(1);
    outer->addWidget(div);

    auto* scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    auto* body = new QWidget;
    body->setAttribute(Qt::WA_StyledBackground, true);
    lay_ = new QVBoxLayout(body);
    lay_->setContentsMargins(8, 8, 8, 8);
    lay_->setSpacing(4);
    lay_->addStretch(1);
    scroll->setWidget(body);
    outer->addWidget(scroll, 1);
}

void MembersPanel::setMembers(const QList<ContactItem>& items)
{
    QLayoutItem* child;
    while ((child = lay_->takeAt(0)) != nullptr) {
        if (child->widget()) child->widget()->deleteLater();
        delete child;
    }
    for (const auto& it : items) {
        auto* row = new QWidget;
        row->setObjectName("memberRow");
        row->setAttribute(Qt::WA_StyledBackground, true);
        auto* h = new QHBoxLayout(row);
        h->setContentsMargins(8, 6, 8, 6);
        h->setSpacing(8);
        h->addWidget(new Avatar(it.name, 34, row));
        auto* nm = new QLabel(it.name, row);
        nm->setObjectName("contactName");
        h->addWidget(nm, 1);
        auto* dot = new QLabel(row);
        dot->setObjectName(it.online ? "dotOnline" : "dotIdle");
        dot->setFixedSize(8, 8);
        h->addWidget(dot);
        lay_->addWidget(row);
    }
    lay_->addStretch(1);
}