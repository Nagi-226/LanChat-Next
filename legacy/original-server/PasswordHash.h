#ifndef PASSWORDHASH_H
#define PASSWORDHASH_H

#include <QCryptographicHash>
#include <QByteArray>
#include <QString>
#include <random>

// Salted SHA-256 password hashing (v1.0.2 interim).
// Upgrade to bcrypt/argon2 when migrating to C++17 asio server (v1.2.0).

class PasswordHash
{
public:
    // Returns "$salt_hex$hash_hex" (e.g. "$a3f2b1$9d2c...")
    static QString hash(const QString& password)
    {
        QByteArray salt = generateSalt(16);
        QByteArray hash = computeHash(password, salt);
        return QString("$%1$%2")
            .arg(QString::fromLatin1(salt.toHex()))
            .arg(QString::fromLatin1(hash.toHex()));
    }

    // Verifies password against stored "$salt$hash" string
    static bool verify(const QString& password, const QString& storedHash)
    {
        QStringList parts = storedHash.split('$', Qt::SkipEmptyParts);
        if (parts.size() != 2) return false; // old plaintext format or corrupt

        QByteArray salt = QByteArray::fromHex(parts[0].toLatin1());
        QByteArray expectedHash = QByteArray::fromHex(parts[1].toLatin1());
        QByteArray actualHash = computeHash(password, salt);
        return actualHash == expectedHash;
    }

    // Check if a stored value is in the old plaintext format (for migration)
    static bool isLegacyPlaintext(const QString& storedValue)
    {
        return !storedValue.startsWith('$');
    }

private:
    static QByteArray generateSalt(int length)
    {
        static std::random_device rd;
        static std::mt19937 gen(rd());
        static std::uniform_int_distribution<> dis(0, 255);

        QByteArray salt;
        salt.resize(length);
        for (int i = 0; i < length; i++) {
            salt[i] = static_cast<char>(dis(gen));
        }
        return salt;
    }

    static QByteArray computeHash(const QString& password, const QByteArray& salt)
    {
        QByteArray data = salt + password.toUtf8();
        return QCryptographicHash::hash(data, QCryptographicHash::Sha256);
    }
};

#endif // PASSWORDHASH_H
