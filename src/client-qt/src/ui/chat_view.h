// chat_view.h —— 中栏：会话头 56px + 可滚动消息区 + 输入区。
// 气泡：对方 #212630 圆角12(左上4)，自己 accent 蓝白字右对齐(右上4)；32px 圆头像；
// 名字+时间行；连续同发送者<5min 分组（隐藏头像/名字行、收紧间距）；空状态居中提示+淡图标。
#pragma once

#include <QWidget>

class QLineEdit;
class QLabel;
class QVBoxLayout;
class QScrollArea;

class ChatView : public QWidget {
    Q_OBJECT
public:
    explicit ChatView(QWidget* parent = nullptr);
    void setSelfId(int id);
    void setPeerLabel(const QString& text);
    // grouped=true 时隐藏头像与名字行、收紧上间距
    // alertLevel>0 时渲染为板端告警卡片（1提示/2警告/3严重），不用普通气泡
    void appendMessage(int fromId, const QString& senderName, const QString& text,
                       qint64 ts, bool grouped, int alertLevel = 0);
    void clear();

signals:
    void sendRequested(const QString& text);

private:
    void updateEmptyState();
    int selfId_ = 0;
    QLabel* peer_;
    QScrollArea* scroll_;
    QWidget* msgBody_;
    QVBoxLayout* msgLay_;
    QWidget* empty_;
    QLineEdit* input_;
    int lastFrom_ = -1;
    qint64 lastTs_ = 0;
};