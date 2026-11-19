#include "ui/search_panel.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QVBoxLayout>

SearchPanel::SearchPanel(QWidget* parent) : QWidget(parent)
{
    auto* lay = new QVBoxLayout(this);
    lay->setSpacing(8);
    auto* title = new QLabel("本地搜索", this);
    title->setObjectName("title");
    query_ = new QLineEdit(this);
    query_->setPlaceholderText("搜索消息…");
    auto* row = new QHBoxLayout;
    for (const QString s : {QString("all"), QString("direct"), QString("group")}) {
        auto* b = new QPushButton(s, this);
        b->setCheckable(true);
        b->setChecked(s == "all");
        row->addWidget(b);
        connect(b, &QPushButton::clicked, this, [this, s, b] {
            scope_ = s;
            // 互斥选中
            for (auto* sib : findChildren<QPushButton*>())
                if (sib->isCheckable()) sib->setChecked(sib == b);
            emit queryChanged(query_->text(), scope_);
        });
    }
    results_ = new QListWidget(this);
    lay->addWidget(title);
    lay->addWidget(query_);
    lay->addLayout(row);
    lay->addWidget(results_, 1);

    connect(query_, &QLineEdit::textChanged, this, [this](const QString& t) {
        emit queryChanged(t, scope_);
    });
}

void SearchPanel::setResults(const QStringList& lines)
{
    results_->clear();
    results_->addItems(lines);
}