// register_view.h —— 注册页：昵称 + 密码 + 确认。
#pragma once

#include <QWidget>

class QLineEdit;
class QLabel;

class RegisterView : public QWidget {
    Q_OBJECT
public:
    explicit RegisterView(QWidget* parent = nullptr);
    void showError(const QString& msg);

signals:
    void registerSubmitted(const QString& nickname, const QString& password);
    void backRequested();

private:
    QLineEdit* nick_;
    QLineEdit* pass_;
    QLineEdit* confirm_;
    QLabel* err_;
};