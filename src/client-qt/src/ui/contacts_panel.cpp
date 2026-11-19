#include "ui/contacts_panel.h"

#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>

ContactsPanel::ContactsPanel(QWidget* parent) : QWidget(parent)
{
    auto* lay = new QVBoxLayout(this);
    lay->setSpacing(8);
    filter_ = new QLineEdit(this);
    filter_->setPlaceholderText("搜索名字或 ID");
    auto* cl = new QLabel("CONTACTS", this);
    cl->setObjectName("muted");
    contacts_ = new QListWidget(this);
    auto* gl = new QLabel("GROUPS", this);
    gl->setObjectName("muted");
    groups_ = new QListWidget(this);
    auto* row = new QHBoxLayout;
    auto* create = new QPushButton("Create", this);
    auto* join = new QPushButton("Join", this);
    row->addWidget(create);
    row->addWidget(join);
    lay->addWidget(filter_);
    lay->addWidget(cl);
    lay->addWidget(contacts_, 2);
    lay->addWidget(gl);
    lay->addWidget(groups_, 1);
    lay->addLayout(row);

    connect(contacts_, &QListWidget::itemClicked, this, [this](QListWidgetItem* i) {
        emit contactSelected(i->data(Qt::UserRole).toInt());
    });
    connect(groups_, &QListWidget::itemClicked, this, [this](QListWidgetItem* i) {
        emit groupSelected(i->data(Qt::UserRole).toInt());
    });
    connect(create, &QPushButton::clicked, this, &ContactsPanel::createGroupRequested);
    connect(join, &QPushButton::clicked, this, &ContactsPanel::joinGroupRequested);
    connect(filter_, &QLineEdit::textChanged, this, &ContactsPanel::filterChanged);
}

void ContactsPanel::setContacts(const QList<QPair<int, QString>>& items)
{
    contacts_->clear();
    for (const auto& it : items) {
        auto* w = new QListWidgetItem(it.second);
        w->setData(Qt::UserRole, it.first);
        contacts_->addItem(w);
    }
}

void ContactsPanel::setGroups(const QList<QPair<int, QString>>& items)
{
    groups_->clear();
    for (const auto& it : items) {
        auto* w = new QListWidgetItem(it.second);
        w->setData(Qt::UserRole, it.first);
        groups_->addItem(w);
    }
}