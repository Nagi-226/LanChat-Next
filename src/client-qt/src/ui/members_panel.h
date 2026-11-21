// members_panel.h —— 右栏 236px：栏头 56「成员」+ 成员列表（在线绿点/离开黄点）。
// 取代原先与左栏重复的「本地搜索」栏；数据来自 PresenceManager 在线状态（Store.online）。
#pragma once

#include <QWidget>
#include <QList>
#include "ui/contacts_panel.h" // ContactItem

class QVBoxLayout;

class MembersPanel : public QWidget {
    Q_OBJECT
public:
    explicit MembersPanel(QWidget* parent = nullptr);
    void setMembers(const QList<ContactItem>& items);

private:
    QVBoxLayout* lay_;
};