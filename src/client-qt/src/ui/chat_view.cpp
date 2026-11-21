#include "ui/chat_view.h"
#include "ui/avatar.h"

#include <QDateTime>
#include <QFrame>
#include <QGraphicsOpacityEffect>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QScrollArea>
#include <QScrollBar>
#include <QVBoxLayout>

ChatView::ChatView(QWidget* parent) : QWidget(parent)
{
    setAttribute(Qt::WA_StyledBackground, true);
    auto* lay = new QVBoxLayout(this);
    lay->setContentsMargins(0, 0, 0, 0);
    lay->setSpacing(0);

    // 会话头 56px
    auto* header = new QWidget(this);
    header->setFixedHeight(56);
    header->setAttribute(Qt::WA_StyledBackground, true);
    auto* hl = new QHBoxLayout(header);
    hl->setContentsMargins(24, 0, 24, 0);
    peer_ = new QLabel("选择一个联系人开始聊天", header);
    peer_->setObjectName("headerTitle");
    hl->addWidget(peer_);
    hl->addStretch(1);
    lay->addWidget(header);
    auto* div = new QFrame(this);
    div->setObjectName("divider");
    div->setFixedHeight(1);
    lay->addWidget(div);

    // 消息区（可滚动）
    scroll_ = new QScrollArea(this);
    scroll_->setWidgetResizable(true);
    scroll_->setFrameShape(QFrame::NoFrame);
    msgBody_ = new QWidget;
    msgBody_->setAttribute(Qt::WA_StyledBackground, true);
    msgLay_ = new QVBoxLayout(msgBody_);
    msgLay_->setContentsMargins(0, 8, 0, 8);
    msgLay_->setSpacing(0);
    // 空状态
    empty_ = new QWidget(msgBody_);
    auto* el = new QVBoxLayout(empty_);
    el->setAlignment(Qt::AlignCenter);
    el->setSpacing(12);
    auto* icon = new QLabel(QStringLiteral("✉"), empty_);
    QFont inf = icon->font();
    inf.setPixelSize(48);
    icon->setFont(inf);
    icon->setAlignment(Qt::AlignCenter);
    auto* eff = new QGraphicsOpacityEffect(icon);
    eff->setOpacity(0.25);
    icon->setGraphicsEffect(eff);
    icon->setStyleSheet("color:#66707F;");
    auto* hint = new QLabel("还没有消息，打个招呼吧", empty_);
    hint->setObjectName("emptyHint");
    hint->setAlignment(Qt::AlignCenter);
    el->addWidget(icon);
    el->addWidget(hint);
    msgLay_->addWidget(empty_);
    msgLay_->addStretch(1);
    scroll_->setWidget(msgBody_);
    lay->addWidget(scroll_, 1);

    // 输入区
    auto* inputWrap = new QWidget(this);
    inputWrap->setAttribute(Qt::WA_StyledBackground, true);
    auto* il = new QHBoxLayout(inputWrap);
    il->setContentsMargins(24, 12, 24, 16);
    il->setSpacing(8);
    input_ = new QLineEdit(inputWrap);
    input_->setPlaceholderText("输入消息…（Enter 发送）");
    auto* send = new QPushButton("发送", inputWrap);
    send->setObjectName("primary");
    il->addWidget(input_, 1);
    il->addWidget(send);
    lay->addWidget(inputWrap);

    connect(send, &QPushButton::clicked, this, [this] {
        const QString t = input_->text().trimmed();
        if (t.isEmpty()) return;
        input_->clear();
        emit sendRequested(t);
    });
    connect(input_, &QLineEdit::returnPressed, send, &QPushButton::click);
}

void ChatView::setSelfId(int id) { selfId_ = id; }

void ChatView::setPeerLabel(const QString& text) { peer_->setText(text); }

void ChatView::appendMessage(int fromId, const QString& senderName, const QString& text,
                             qint64 ts, bool grouped, int alertLevel)
{
    empty_->hide();
    const bool mine = (fromId == selfId_);
    auto* row = new QWidget(msgBody_);
    row->setAttribute(Qt::WA_StyledBackground, true);
    auto* h = new QHBoxLayout(row);
    h->setContentsMargins(24, grouped ? 2 : 12, 24, 0);
    h->setSpacing(8);
    if (mine) h->setDirection(QBoxLayout::RightToLeft);

    if (!grouped) h->addWidget(new Avatar(senderName, 32, row));

    auto* col = new QVBoxLayout;
    col->setSpacing(4);
    if (!grouped) {
        auto* nameRow = new QHBoxLayout;
        nameRow->setSpacing(8);
        auto* nm = new QLabel(mine ? "我" : senderName, row);
        nm->setObjectName("msgName");
        auto* tm = new QLabel(QDateTime::fromMSecsSinceEpoch(ts).toString("HH:mm"), row);
        tm->setObjectName("timestamp");
        nameRow->addWidget(nm);
        nameRow->addWidget(tm);
        nameRow->addStretch(1);
        col->addLayout(nameRow);
    }
    if (alertLevel > 0) {
        // 板端告警卡片：左 3px 严重度色条 + 级别徽章 + 正文，一眼区别于人聊的气泡
        auto* card = new QWidget(row);
        card->setObjectName("alertCard");
        card->setAttribute(Qt::WA_StyledBackground, true);
        card->setProperty("level", alertLevel);
        card->setMaximumWidth(520);
        auto* cl = new QVBoxLayout(card);
        cl->setContentsMargins(12, 8, 12, 8);
        cl->setSpacing(4);
        auto* head = new QHBoxLayout;
        head->setSpacing(8);
        auto* chip = new QLabel(alertLevel >= 3 ? "严重" : alertLevel == 2 ? "警告" : "提示", card);
        chip->setObjectName(alertLevel >= 3 ? "chipDanger" : alertLevel == 2 ? "chipWarn" : "chipInfo");
        head->addWidget(chip);
        auto* src = new QLabel("板端 · " + senderName, card);
        src->setObjectName("timestamp");
        head->addWidget(src);
        head->addStretch(1);
        cl->addLayout(head);
        auto* body = new QLabel(text, card);
        body->setObjectName("alertBody");
        body->setWordWrap(true);
        cl->addWidget(body);
        col->addWidget(card);
        h->addLayout(col, 1);
        if (mine) col->setAlignment(card, Qt::AlignRight);
    } else {
        auto* bubble = new QLabel(text, row);
        bubble->setObjectName(mine ? "bubbleSelf" : "bubblePeer");
        bubble->setWordWrap(true);
        bubble->setMaximumWidth(520);
        bubble->setAttribute(Qt::WA_StyledBackground, true);
        col->addWidget(bubble);
        h->addLayout(col, 1);
        // 让气泡靠发送者一侧：col 内 bubble 对齐
        // （RightToLeft 已处理整行方向；气泡在 col 内默认左对齐，self 需右对齐）
        if (mine) col->setAlignment(bubble, Qt::AlignRight);
    }

    // 插到 stretch 之前
    msgLay_->insertWidget(msgLay_->count() - 1, row);
    lastFrom_ = fromId;
    lastTs_ = ts;
    scroll_->verticalScrollBar()->setValue(scroll_->verticalScrollBar()->maximum());
}

void ChatView::clear()
{
    QLayoutItem* child;
    while ((child = msgLay_->takeAt(0)) != nullptr) {
        if (child->widget() && child->widget() != empty_) child->widget()->deleteLater();
        else if (child->widget() == empty_) { /* keep */ }
        delete child;
    }
    msgLay_->addWidget(empty_);
    msgLay_->addStretch(1);
    empty_->show();
    lastFrom_ = -1;
    lastTs_ = 0;
}

void ChatView::updateEmptyState()
{
    empty_->setVisible(msgLay_->count() <= 2);
}