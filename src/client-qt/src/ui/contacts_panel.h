// contacts_panel.h —— 左栏：联系人 + 群 + 创建/加入 + 搜索框。
#pragma once

#include <QWidget>

class QListWidget;
class QLineEdit;

class ContactsPanel : public QWidget {
    Q_OBJECT
public:
    explicit ContactsPanel(QWidget* parent = nullptr);
    void setContacts(const QList<QPair<int, QString>>& items);
    void setGroups(const QList<QPair<int, QString>>& items);

signals:
    void contactSelected(int id);
    void groupSelected(int id);
    void createGroupRequested();
    void joinGroupRequested();
    void filterChanged(const QString& text);

private:
    QListWidget* contacts_;
    QListWidget* groups_;
    QLineEdit* filter_;
};