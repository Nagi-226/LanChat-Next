// login_view.h —— 登录页。标识字段是"用户名"（如 Nagi_226），不是数字 id。
#pragma once

#include <QWidget>

class QLineEdit;
class QLabel;

class LoginView : public QWidget {
    Q_OBJECT
public:
    explicit LoginView(QWidget* parent = nullptr);
    void showError(const QString& msg);

signals:
    void loginRequested(const QString& username, const QString& password);
    void registerRequested();

private:
    QLineEdit* user_;
    QLineEdit* pass_;
    QLabel* err_;
};