// login_view.h —— 登录页。标识字段是"用户名"（如 Nagi_226），不是数字 id。
// "记住账户"勾选后，登录成功时由主窗口把用户名+加密密码落盘，下次启动自动预填。
#pragma once

#include <QWidget>

class QLineEdit;
class QLabel;
class QCheckBox;

class LoginView : public QWidget {
    Q_OBJECT
public:
    explicit LoginView(QWidget* parent = nullptr);
    void showError(const QString& msg);
    bool rememberChecked() const;

signals:
    void loginRequested(const QString& username, const QString& password);
    void registerRequested();

private:
    QLineEdit* user_;
    QLineEdit* pass_;
    QCheckBox* remember_;
    QLabel* err_;
};