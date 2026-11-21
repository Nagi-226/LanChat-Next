// credentials.h —— "记住账户"的落盘读写。
// 用户名明文存 ini（方便下次直接显示）；密码在 Windows 上用 DPAPI 加密后再存：
// DPAPI 是 Windows 自带的"跟当前登录用户绑定"的加密——密文拷到别的机器/别的账户解不开，
// 所以 ini 被拷走也拿不到密码。非 Windows 平台只记用户名，不记密码（没有等价保护就不存）。
#pragma once

#include <QString>

namespace credentials {

struct SavedLogin {
    QString username;
    QString password; // 非 Windows 或解密失败时为空
    bool remembered = false;
};

// 读上次记住的账户；没记住时 remembered=false
SavedLogin load();
// 登录成功后调用：记住用户名+密码（密码加密落盘）
void save(const QString& username, const QString& password);
// 取消勾选时调用：把记住的内容清掉
void clear();

} // namespace credentials