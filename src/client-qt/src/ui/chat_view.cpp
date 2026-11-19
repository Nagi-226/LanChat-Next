#include "ui/chat_view.h"

#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPainter>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>

void BubbleDelegate::paint(QPainter* p, const QStyleOptionViewItem& opt, const QModelIndex& i) const
{
    const bool mine = i.data(Qt::UserRole + 1).toInt() == selfId;
    const QString text = i.data(Qt::DisplayRole).toString();
    p->save();
    p->setRenderHint(QPainter::Antialiasing);
    const QRect r = opt.rect.adjusted(8, 4, -8, -4);
    const QFontMetrics fm(opt.font);
    const int maxW = r.width() * 2 / 3;
    // 估算气泡宽：单行文本宽+padding，超 maxW 则用 maxW（多行换行）
    const int w = qMin(fm.horizontalAdvance(text) + 24, maxW);
    const QRect bubble(mine ? r.right() - w : r.left(), r.top(), w, r.height());
    p->setPen(Qt::NoPen);
    p->setBrush(mine ? QColor("#E8465A") : QColor("#1B212C"));
    p->drawRoundedRect(bubble, 10, 10);
    p->setPen(QColor("#E6E9F0"));
    p->drawText(bubble.adjusted(12, 8, -12, -8), Qt::AlignLeft | Qt::TextWordWrap, text);
    p->restore();
}

QSize BubbleDelegate::sizeHint(const QStyleOptionViewItem& opt, const QModelIndex& i) const
{
    const QString text = i.data(Qt::DisplayRole).toString();
    const QFontMetrics fm(opt.font);
    const int maxW = opt.rect.width() * 2 / 3;
    const QRect bound = fm.boundingRect(QRect(0, 0, maxW - 24, 10000), Qt::TextWordWrap, text);
    return QSize(opt.rect.width(), bound.height() + 24);
}

ChatView::ChatView(QWidget* parent) : QWidget(parent)
{
    auto* lay = new QVBoxLayout(this);
    lay->setSpacing(8);
    peer_ = new QLabel("选择一个联系人开始聊天", this);
    peer_->setObjectName("title");
    list_ = new QListWidget(this);
    delegate_ = new BubbleDelegate;
    list_->setItemDelegate(delegate_);
    list_->setSelectionMode(QAbstractItemView::NoSelection);
    list_->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    input_ = new QLineEdit(this);
    input_->setPlaceholderText("输入消息…（Enter 发送）");
    auto* send = new QPushButton("发送", this);
    send->setObjectName("primary");
    auto* row = new QHBoxLayout;
    row->addWidget(input_, 1);
    row->addWidget(send);
    lay->addWidget(peer_);
    lay->addWidget(list_, 1);
    lay->addLayout(row);

    connect(send, &QPushButton::clicked, this, [this] {
        const QString t = input_->text().trimmed();
        if (t.isEmpty()) return;
        input_->clear();
        emit sendRequested(t);
    });
    connect(input_, &QLineEdit::returnPressed, send, &QPushButton::click);
}

void ChatView::setSelfId(int id) { delegate_->selfId = id; }

void ChatView::setPeerLabel(const QString& text) { peer_->setText(text); }

void ChatView::appendMessage(int fromId, const QString& text)
{
    auto* item = new QListWidgetItem(text);
    item->setData(Qt::UserRole + 1, fromId);
    list_->addItem(item);
    list_->scrollToBottom();
}

void ChatView::clear() { list_->clear(); }