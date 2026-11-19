// search_panel.h —— 右栏：本地消息搜索（all/direct/group 过滤）。AI 面板 P2 不渲染。
#pragma once

#include <QWidget>

class QLineEdit;
class QListWidget;

class SearchPanel : public QWidget {
    Q_OBJECT
public:
    explicit SearchPanel(QWidget* parent = nullptr);
    void setResults(const QStringList& lines);

signals:
    void queryChanged(const QString& query, const QString& scope);

private:
    QLineEdit* query_;
    QListWidget* results_;
    QString scope_ = "all";
};