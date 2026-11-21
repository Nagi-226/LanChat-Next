#include "ui/contacts_panel.h"
#include "ui/avatar.h"

#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QStyle>
#include <QMouseEvent>
#include <QEvent>

namespace {
QLabel* sectionTitle(const QString& text, QWidget* parent)
{
    auto* l = new QLabel(text.toUpper(), parent); // uppercase 用代码（QSS 不支持 text-transform）
    l->setObjectName("sectionTitle");
    QFont f = l->font();
    f.setLetterSpacing(QFont::AbsoluteSpacing, 1.1); // letter-spacing 用 QFont（QSS 不支持）
    l->setFont(f);
    return l;
}
QLabel* dot(bool online, QWidget* parent)
{
    auto* d = new QLabel(parent);
    d->setObjectName(online ? "dotOnline" : "dotIdle");
    d->setFixedSize(8, 8);
    return d;
}
}

ContactsPanel::ContactsPanel(QWidget* parent) : QWidget(parent)
{
    setFixedWidth(268);
    setAttribute(Qt::WA_StyledBackground, true);
    auto* lay = new QVBoxLayout(this);
    lay->setContentsMargins(0, 0, 0, 0);
    lay->setSpacing(0);

    // 品牌条 56px
    auto* brand = new QWidget(this);
    brand->setFixedHeight(56);
    brand->setAttribute(Qt::WA_StyledBackground, true);
    auto* bl = new QHBoxLayout(brand);
    bl->setContentsMargins(16, 0, 16, 0);
    auto* logo = new QLabel("LanChat", brand);
    logo->setObjectName("headerTitle");
    bl->addWidget(logo);
    bl->addStretch(1);
    lay->addWidget(brand);

    // 搜索框
    auto* searchWrap = new QWidget(this);
    auto* sw = new QVBoxLayout(searchWrap);
    sw->setContentsMargins(12, 8, 12, 8);
    filter_ = new QLineEdit(searchWrap);
    filter_->setPlaceholderText("搜索名字或 ID");
    sw->addWidget(filter_);
    lay->addWidget(searchWrap);

    // 会话分节（可滚动）
    auto* cs = new QScrollArea(this);
    cs->setWidgetResizable(true);
    cs->setFrameShape(QFrame::NoFrame);
    auto* cbody = new QWidget;
    cbody->setAttribute(Qt::WA_StyledBackground, true);
    contactLay_ = new QVBoxLayout(cbody);
    contactLay_->setContentsMargins(8, 4, 8, 4);
    contactLay_->setSpacing(4);
    contactLay_->addWidget(sectionTitle("会话", cbody));
    contactLay_->addStretch(1);
    cs->setWidget(cbody);
    lay->addWidget(cs, 2);

    // 群组分节（可滚动）
    auto* gs = new QScrollArea(this);
    gs->setWidgetResizable(true);
    gs->setFrameShape(QFrame::NoFrame);
    auto* gbody = new QWidget;
    gbody->setAttribute(Qt::WA_StyledBackground, true);
    groupLay_ = new QVBoxLayout(gbody);
    groupLay_->setContentsMargins(8, 4, 8, 4);
    groupLay_->setSpacing(4);
    groupLay_->addWidget(sectionTitle("群组", gbody));
    groupLay_->addStretch(1);
    gs->setWidget(gbody);
    lay->addWidget(gs, 1);

    // 板端告警分节（可滚动）：标题行带齿轮，点齿轮配置"板端账号 id"
    auto* as = new QScrollArea(this);
    as->setWidgetResizable(true);
    as->setFrameShape(QFrame::NoFrame);
    auto* abody = new QWidget;
    abody->setAttribute(Qt::WA_StyledBackground, true);
    alertLay_ = new QVBoxLayout(abody);
    alertLay_->setContentsMargins(8, 4, 8, 4);
    alertLay_->setSpacing(4);
    auto* ahead = new QWidget(abody);
    auto* ah = new QHBoxLayout(ahead);
    ah->setContentsMargins(0, 0, 0, 0);
    ah->setSpacing(4);
    ah->addWidget(sectionTitle("板端告警", ahead));
    ah->addStretch(1);
    auto* gear = new QPushButton(QStringLiteral("⚙"), ahead);
    gear->setObjectName("gearBtn");
    gear->setFixedSize(20, 20);
    gear->setCursor(Qt::PointingHandCursor);
    gear->setToolTip("设置板端账号：Console 桥用哪个 LanChat 账号发告警，这里就填那个账号的数字 id");
    ah->addWidget(gear);
    alertLay_->addWidget(ahead);
    alertLay_->addStretch(1);
    as->setWidget(abody);
    lay->addWidget(as, 1);
    connect(gear, &QPushButton::clicked, this, &ContactsPanel::boardConfigRequested);

    // 底部建群/加入
    auto* bottom = new QWidget(this);
    auto* bo = new QHBoxLayout(bottom);
    bo->setContentsMargins(12, 8, 12, 12);
    bo->setSpacing(8);
    auto* create = new QPushButton("建群", bottom);
    auto* join = new QPushButton("加入", bottom);
    bo->addWidget(create);
    bo->addWidget(join);
    lay->addWidget(bottom);

    connect(create, &QPushButton::clicked, this, &ContactsPanel::createGroupRequested);
    connect(join, &QPushButton::clicked, this, &ContactsPanel::joinGroupRequested);
    connect(filter_, &QLineEdit::textChanged, this, &ContactsPanel::filterChanged);
}

QWidget* ContactsPanel::makeRow(const ContactItem& it, bool isGroup)
{
    auto* row = new QWidget;
    row->setObjectName("contactRow");
    row->setAttribute(Qt::WA_StyledBackground, true);
    row->setProperty("selected", it.id == selectedId_);
    auto* h = new QHBoxLayout(row);
    h->setContentsMargins(0, 4, 8, 4);
    h->setSpacing(8);
    // 左 3px accent 竖条：仅选中可见
    auto* bar = new QFrame(row);
    bar->setObjectName("accentBar");
    bar->setFixedWidth(3);
    bar->setVisible(it.id == selectedId_);
    h->addWidget(bar);
    h->addWidget(new Avatar(it.name, 34, row));
    auto* mid = new QVBoxLayout;
    mid->setSpacing(2);
    auto* name = new QLabel(it.name, row);
    name->setObjectName("contactName");
    mid->addWidget(name);
    if (!isGroup && !it.preview.isEmpty()) {
        auto* pv = new QLabel(it.preview, row);
        pv->setObjectName("preview");
        mid->addWidget(pv);
    }
    h->addLayout(mid, 1);
    auto* right = new QVBoxLayout;
    right->setSpacing(4);
    right->setAlignment(Qt::AlignRight | Qt::AlignTop);
    if (!it.time.isEmpty()) {
        auto* t = new QLabel(it.time, row);
        t->setObjectName("timestamp");
        right->addWidget(t, 0, Qt::AlignRight);
    }
    auto* dotRow = new QHBoxLayout;
    dotRow->setSpacing(4);
    dotRow->addStretch(1);
    if (it.unread > 0) {
        auto* u = new QLabel(QString::number(it.unread), row);
        u->setObjectName("unread");
        u->setAlignment(Qt::AlignCenter);
        dotRow->addWidget(u);
    }
    dotRow->addWidget(dot(it.online, row));
    right->addLayout(dotRow);
    h->addLayout(right);
    row->setCursor(Qt::PointingHandCursor);
    // 点击整行
    row->installEventFilter(this);
    row->setProperty("rowId", it.id);
    row->setProperty("rowIsGroup", isGroup);
    return row;
}

void ContactsPanel::setContacts(const QList<ContactItem>& items)
{
    // 清空旧行（保留分节标题与 stretch）
    for (auto& rw : rows_) {
        contactLay_->removeWidget(rw.second);
        rw.second->deleteLater();
    }
    rows_.clear();
    // 移除标题后所有 widget 再重建：简单起见重建整个 contactLay 内容
    QLayoutItem* child;
    while ((child = contactLay_->takeAt(0)) != nullptr) {
        if (child->widget()) child->widget()->deleteLater();
        delete child;
    }
    contactLay_->addWidget(sectionTitle("会话", this));
    for (const auto& it : items) {
        auto* r = makeRow(it, false);
        contactLay_->addWidget(r);
        rows_.append({it.id, r});
    }
    contactLay_->addStretch(1);
}

void ContactsPanel::setGroups(const QList<QPair<int, QString>>& items)
{
    QLayoutItem* child;
    while ((child = groupLay_->takeAt(0)) != nullptr) {
        if (child->widget()) child->widget()->deleteLater();
        delete child;
    }
    groupLay_->addWidget(sectionTitle("群组", this));
    for (const auto& g : items) {
        ContactItem it;
        it.id = g.first;
        it.name = g.second;
        it.online = true;
        auto* r = makeRow(it, true);
        groupLay_->addWidget(r);
    }
    groupLay_->addStretch(1);
}

void ContactsPanel::setAlerts(const QList<AlertItem>& items)
{
    QLayoutItem* child;
    // 保留标题行（第一个 item），清掉其余旧告警行与 stretch
    while (alertLay_->count() > 1) {
        child = alertLay_->takeAt(1);
        if (child->widget()) child->widget()->deleteLater();
        delete child;
    }
    for (const auto& a : items) {
        auto* row = new QWidget;
        row->setObjectName("alertRow");
        row->setAttribute(Qt::WA_StyledBackground, true);
        row->setProperty("rowId", a.peerId);
        row->setProperty("rowIsAlert", true);
        row->setCursor(Qt::PointingHandCursor);
        auto* h = new QHBoxLayout(row);
        h->setContentsMargins(8, 6, 8, 6);
        h->setSpacing(8);
        auto* dot = new QLabel(row);
        dot->setObjectName(a.level >= 3 ? "dotDanger" : a.level == 2 ? "dotIdle" : "dotInfo");
        dot->setFixedSize(8, 8);
        h->addWidget(dot, 0, Qt::AlignTop);
        auto* mid = new QVBoxLayout;
        mid->setSpacing(2);
        auto* txt = new QLabel(row);
        txt->setObjectName("alertText");
        // 单行省略：告警摘要可能很长，左栏宽度固定，超出画省略号而不是撑破布局
        const QString elided = txt->fontMetrics().elidedText(a.text, Qt::ElideRight, 190);
        txt->setText(elided);
        txt->setToolTip(a.text);
        mid->addWidget(txt);
        auto* tm = new QLabel(a.time, row);
        tm->setObjectName("timestamp");
        mid->addWidget(tm);
        h->addLayout(mid, 1);
        row->installEventFilter(this);
        alertLay_->addWidget(row);
    }
    alertLay_->addStretch(1);
}

void ContactsPanel::setSelected(int id)
{
    selectedId_ = id;
    for (auto& rw : rows_) {
        rw.second->setProperty("selected", rw.first == id);
        rw.second->style()->unpolish(rw.second);
        rw.second->style()->polish(rw.second);
        // 竖条可见性
        if (auto* bar = rw.second->findChild<QFrame*>("accentBar"))
            bar->setVisible(rw.first == id);
    }
}

void ContactsPanel::setHovered(int id)
{
    for (auto& rw : rows_) {
        rw.second->setProperty("hovered", rw.first == id);
        rw.second->style()->unpolish(rw.second);
        rw.second->style()->polish(rw.second);
    }
}

bool ContactsPanel::eventFilter(QObject* obj, QEvent* e)
{
    if (e->type() == QEvent::MouseButtonRelease) {
        auto* w = qobject_cast<QWidget*>(obj);
        if (w && w->property("rowId").isValid()) {
            const int id = w->property("rowId").toInt();
            if (w->property("rowIsAlert").toBool()) emit alertSelected(id);
            else if (w->property("rowIsGroup").toBool()) emit groupSelected(id);
            else emit contactSelected(id);
            return true;
        }
    }
    return QWidget::eventFilter(obj, e);
}