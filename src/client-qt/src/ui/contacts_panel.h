// contacts_panel.h —— 左栏 268px：品牌条 56 + 搜索 + 分节(会话/群组/板端告警) + 底部建群/加入。
// 联系人行：34px 圆头像 + 名字(13/500) + 预览(11.5 muted) + 时间(10.5 muted) + 在线点 + 未读徽标。
// 选中 = bg-active 浅底 + 左 3px accent 竖条（QFrame，非整条红底）。
// 板端告警分节：Console 桥转发来的板子告警按严重度着色，点行跳到对应会话；齿轮配置板端账号。
#pragma once

#include <QWidget>
#include <QList>
#include <QPair>

class QListWidget;
class QLineEdit;
class QVBoxLayout;

struct ContactItem {
    int id = 0;
    QString name;
    QString preview;
    QString time;
    bool online = false;
    int unread = 0;
};

// 板端告警条目：level 1=提示(蓝) 2=警告(琥珀) 3=严重(红)
struct AlertItem {
    int peerId = 0;
    QString text;
    QString time;
    int level = 1;
    bool isGroup = false;
};

class ContactsPanel : public QWidget {
    Q_OBJECT
public:
    explicit ContactsPanel(QWidget* parent = nullptr);
    void setContacts(const QList<ContactItem>& items);
    void setGroups(const QList<QPair<int, QString>>& items);
    void setAlerts(const QList<AlertItem>& items);
    void setSelected(int id);
    void setHovered(int id); // 预览 hook：强制 hover 态供状态对照截图

protected:
    bool eventFilter(QObject* obj, QEvent* e) override;

signals:
    void contactSelected(int id);
    void groupSelected(int id);
    void alertSelected(int peerId, bool isGroup);
    void boardConfigRequested();
    void createGroupRequested();
    void joinGroupRequested();
    void filterChanged(const QString& text);

private:
    QWidget* makeRow(const ContactItem& it, bool isGroup);
    QVBoxLayout* contactLay_;
    QVBoxLayout* groupLay_;
    QVBoxLayout* alertLay_;
    QLineEdit* filter_;
    int selectedId_ = -1;
    QList<QPair<int, QWidget*>> rows_;
};
