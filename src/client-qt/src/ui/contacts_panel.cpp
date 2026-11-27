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
#include <QIconEngine>
#include <QPainter>
#include <QPainterPath>
#include <cmath>

namespace {
class SettingsIcon : public QIconEngine {
public:
    QIconEngine* clone() const override {return new SettingsIcon;}
    QPixmap pixmap(const QSize& size,QIcon::Mode mode,QIcon::State state) override {
        QPixmap pixmap(size);pixmap.fill(Qt::transparent);QPainter painter(&pixmap);paint(&painter,QRect(QPoint(),size),mode,state);return pixmap;
    }
    void paint(QPainter* painter,const QRect& rect,QIcon::Mode mode,QIcon::State) override {
        painter->save();painter->setRenderHint(QPainter::Antialiasing);painter->translate(rect.center()+QPointF(.5,.5));painter->scale(rect.width()/20.,rect.height()/20.);
        QPainterPath gear;for(int i=0;i<32;++i){double angle=i*3.141592653589793/16.;double r=i%4==1||i%4==2?9.:7.;QPointF point(r*std::cos(angle),r*std::sin(angle));if(i==0)gear.moveTo(point);else gear.lineTo(point);}gear.closeSubpath();gear.addEllipse(QPointF(0,0),3.,3.);
        painter->setPen(Qt::NoPen);painter->setBrush(mode==QIcon::Active?QColor("#E7EAF0"):QColor("#98A1B2"));painter->drawPath(gear);painter->restore();
    }
};
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

    // 板端告警分节；连接与服务器设置统一从底部齿轮进入。
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
    alertLay_->addWidget(ahead);
    alertLay_->addStretch(1);
    as->setWidget(abody);
    lay->addWidget(as, 1);

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

    auto* footer=new QWidget(this);footer->setObjectName("sidebarFooter");
    auto* footerLayout=new QHBoxLayout(footer);footerLayout->setContentsMargins(12,8,12,12);
    settingsButton_=new QPushButton(footer);settingsButton_->setObjectName("settingsButton");settingsButton_->setIcon(QIcon(new SettingsIcon));settingsButton_->setIconSize(QSize(20,20));settingsButton_->setFixedSize(38,38);
    settingsButton_->setToolTip("设置：服务器与板端连接");settingsButton_->setAccessibleName("设置");settingsButton_->setCursor(Qt::PointingHandCursor);settingsButton_->setEnabled(false);
    footerLayout->addWidget(settingsButton_);auto* footerText=new QLabel("设置",footer);footerText->setObjectName("timestamp");footerLayout->addWidget(footerText);footerLayout->addStretch();lay->addWidget(footer);
    connect(settingsButton_,&QPushButton::clicked,this,&ContactsPanel::settingsRequested);

    connect(create, &QPushButton::clicked, this, &ContactsPanel::createGroupRequested);
    connect(join, &QPushButton::clicked, this, &ContactsPanel::joinGroupRequested);
    connect(filter_, &QLineEdit::textChanged, this, &ContactsPanel::filterChanged);
}

void ContactsPanel::setAuthenticated(bool authenticated) {settingsButton_->setEnabled(authenticated);}

QWidget* ContactsPanel::makeRow(const ContactItem& it, bool isGroup)
{
    auto* row = new QWidget;
    row->setObjectName("contactRow");
    row->setAttribute(Qt::WA_StyledBackground, true);
    row->setProperty("selected", it.id == selectedId_ && isGroup==selectedGroup_);
    auto* h = new QHBoxLayout(row);
    h->setContentsMargins(0, 4, 8, 4);
    h->setSpacing(8);
    // 左 3px accent 竖条：仅选中可见
    auto* bar = new QFrame(row);
    bar->setObjectName("accentBar");
    bar->setFixedWidth(3);
    bar->setVisible(it.id == selectedId_ && isGroup==selectedGroup_);
    h->addWidget(bar);
    h->addWidget(new Avatar(it.name, 34, row, it.avatarId));
    auto* mid = new QVBoxLayout;
    mid->setSpacing(2);
    auto* name = new QLabel(it.name, row);
    name->setTextFormat(Qt::PlainText);
    name->setObjectName("contactName");
    mid->addWidget(name);
    if (!isGroup && !it.preview.isEmpty()) {
        auto* pv = new QLabel(it.preview, row);
        pv->setTextFormat(Qt::PlainText);
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

void ContactsPanel::setGroups(const QList<QPair<int, QString>>& items,const QHash<QString,int>& unread)
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
        it.unread=unread.value(QString("g:%1").arg(g.first));
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
        row->setProperty("rowIsGroup", a.isGroup);
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

void ContactsPanel::setSelected(int id,bool group)
{
    selectedId_ = id;selectedGroup_=group;
    for(auto* row:findChildren<QWidget*>())if(row->property("rowIsGroup").toBool() && !row->property("rowIsAlert").toBool()){
        bool on=selectedGroup_&&row->property("rowId").toInt()==id;row->setProperty("selected",on);row->style()->unpolish(row);row->style()->polish(row);
        if(auto* bar=row->findChild<QFrame*>("accentBar"))bar->setVisible(on);
    }
    for (auto& rw : rows_) {
        rw.second->setProperty("selected", rw.first == id && !selectedGroup_);
        rw.second->style()->unpolish(rw.second);
        rw.second->style()->polish(rw.second);
        // 竖条可见性
        if (auto* bar = rw.second->findChild<QFrame*>("accentBar"))
            bar->setVisible(rw.first == id && !selectedGroup_);
    }
}

void ContactsPanel::setHovered(int id)
{
    for (auto& rw : rows_) {
        rw.second->setProperty("hovered", rw.first == id && !selectedGroup_);
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
            if (w->property("rowIsAlert").toBool()) emit alertSelected(id, w->property("rowIsGroup").toBool());
            else if (w->property("rowIsGroup").toBool()) emit groupSelected(id);
            else emit contactSelected(id);
            return true;
        }
    }
    return QWidget::eventFilter(obj, e);
}
