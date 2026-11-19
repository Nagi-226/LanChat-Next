// chat_view.h —— 聊天区：气泡列表 + 输入框 + 发送。
// 气泡 delegate：自己发的靠右 accent 色，对方靠左 panel 色（圆角矩形）。
#pragma once

#include <QWidget>
#include <QStyledItemDelegate>

class QListWidget;
class QLineEdit;
class QListWidgetItem;
class QLabel;

class BubbleDelegate : public QStyledItemDelegate {
public:
    int selfId = 0;
    void paint(QPainter* p, const QStyleOptionViewItem& opt, const QModelIndex& i) const override;
    QSize sizeHint(const QStyleOptionViewItem& opt, const QModelIndex& i) const override;
};

class ChatView : public QWidget {
    Q_OBJECT
public:
    explicit ChatView(QWidget* parent = nullptr);
    void setSelfId(int id);
    void setPeerLabel(const QString& text);
    void appendMessage(int fromId, const QString& text);
    void clear();

signals:
    void sendRequested(const QString& text);

private:
    QListWidget* list_;
    QLineEdit* input_;
    BubbleDelegate* delegate_;
    QLabel* peer_;
};