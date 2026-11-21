#include "app/credentials.h"

#include <QSettings>

#ifdef Q_OS_WIN
#include <windows.h>
#include <wincrypt.h>
#endif

namespace credentials {

namespace {
constexpr auto kKeyRemember = "remember_login";
constexpr auto kKeyUser = "login_user";
constexpr auto kKeySecret = "login_secret"; // DPAPI 密文的 base64，不是密码本身

#ifdef Q_OS_WIN
// 用当前 Windows 用户的密钥加密一段字节；失败返回空 QByteArray
QByteArray protect(const QByteArray& plain)
{
    DATA_BLOB in{static_cast<DWORD>(plain.size()), reinterpret_cast<BYTE*>(const_cast<char*>(plain.data()))};
    DATA_BLOB out{};
    if (!CryptProtectData(&in, L"LanChat-Qt remembered login", nullptr, nullptr, nullptr,
                          CRYPTPROTECT_UI_FORBIDDEN, &out))
        return {};
    QByteArray cipher(reinterpret_cast<const char*>(out.pbData), static_cast<int>(out.cbData));
    LocalFree(out.pbData);
    return cipher;
}

QByteArray unprotect(const QByteArray& cipher)
{
    DATA_BLOB in{static_cast<DWORD>(cipher.size()), reinterpret_cast<BYTE*>(const_cast<char*>(cipher.data()))};
    DATA_BLOB out{};
    if (!CryptUnprotectData(&in, nullptr, nullptr, nullptr, nullptr,
                            CRYPTPROTECT_UI_FORBIDDEN, &out))
        return {};
    QByteArray plain(reinterpret_cast<const char*>(out.pbData), static_cast<int>(out.cbData));
    LocalFree(out.pbData);
    return plain;
}
#endif
} // namespace

QByteArray encryptSecret(const QString& password)
{
#ifdef Q_OS_WIN
    return protect(password.toUtf8()).toBase64();
#else
    Q_UNUSED(password);
    return {};
#endif
}

QString decryptSecret(const QByteArray& encoded)
{
#ifdef Q_OS_WIN
    return QString::fromUtf8(unprotect(QByteArray::fromBase64(encoded)));
#else
    Q_UNUSED(encoded);
    return {};
#endif
}

SavedLogin load()
{
    QSettings s;
    SavedLogin r;
    r.remembered = s.value(kKeyRemember, false).toBool();
    if (!r.remembered) return r;
    r.username = s.value(kKeyUser).toString();
#ifdef Q_OS_WIN
    const QByteArray cipher = QByteArray::fromBase64(s.value(kKeySecret).toByteArray());
    if (!cipher.isEmpty()) r.password = QString::fromUtf8(unprotect(cipher));
#endif
    return r;
}

void save(const QString& username, const QString& password)
{
    QSettings s;
    s.setValue(kKeyRemember, true);
    s.setValue(kKeyUser, username);
#ifdef Q_OS_WIN
    s.setValue(kKeySecret, protect(password.toUtf8()).toBase64());
#else
    // 没有 DPAPI 等价保护的平台：只记用户名，密码不落盘
    s.remove(kKeySecret);
#endif
}

void clear()
{
    QSettings s;
    s.setValue(kKeyRemember, false);
    s.remove(kKeyUser);
    s.remove(kKeySecret);
}

} // namespace credentials
