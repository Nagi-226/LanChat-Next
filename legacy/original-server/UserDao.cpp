#include "UserDao.h"
#include "PasswordHash.h"
#include <QSqlQuery>
#include <QSqlRecord>
#include <QVariant>

UserDao* UserDao::instance = nullptr;

UserDao* UserDao::getInstance()
{
    if (instance == nullptr) {
        instance = new UserDao;
    }
    return instance;
}

void UserDao::releaseInstance()
{
    delete instance;
    instance = nullptr;
}

int UserDao::insertUser(UserEntity& e)
{
    QString hashedPw = PasswordHash::hash(e.password);

    QSqlQuery query;
    query.prepare("INSERT INTO user(password, nickname, headid) VALUES(?, ?, ?)");
    query.bindValue(0, hashedPw);
    query.bindValue(1, e.nickname);
    query.bindValue(2, e.headId);
    query.exec();

    query.exec("SELECT LAST_INSERT_ROWID()");
    int id = 0;
    if (query.next()) {
        id = query.record().value(0).toInt();
    }
    return id;
}

bool UserDao::verifyPassword(const UserEntity& e)
{
    QSqlQuery query;
    query.prepare("SELECT password, nickname, headid FROM user WHERE id = ?");
    query.bindValue(0, e.id);
    query.exec();

    while (query.next()) {
        QString storedPw = query.record().value("password").toString();

        // Handle legacy plaintext passwords: if stored value doesn't start with '$',
        // do direct comparison, then upgrade to hash on success
        bool ok;
        if (PasswordHash::isLegacyPlaintext(storedPw)) {
            ok = (storedPw == e.password);
            if (ok) {
                // Upgrade legacy plaintext to hash
                QString hashed = PasswordHash::hash(e.password);
                QSqlQuery upgrade;
                upgrade.prepare("UPDATE user SET password = ? WHERE id = ?");
                upgrade.bindValue(0, hashed);
                upgrade.bindValue(1, e.id);
                upgrade.exec();
            }
        } else {
            ok = PasswordHash::verify(e.password, storedPw);
        }

        if (ok) {
            // Fill entity fields from DB
            const_cast<UserEntity&>(e).nickname = query.record().value("nickname").toString();
            const_cast<UserEntity&>(e).headId   = query.record().value("headid").toInt();
            return true;
        }
    }
    return false;
}

std::vector<UserEntity> UserDao::getAllUsers()
{
    std::vector<UserEntity> users;
    QSqlQuery query;
    query.exec("SELECT id, nickname, headid FROM user ORDER BY id");
    while (query.next()) {
        UserEntity u;
        u.id       = query.record().value("id").toInt();
        u.nickname = query.record().value("nickname").toString();
        u.headId   = query.record().value("headid").toInt();
        users.push_back(u);
    }
    return users;
}

bool UserDao::getUserById(int id, UserEntity& out)
{
    QSqlQuery query;
    query.prepare("SELECT id, nickname, headid FROM user WHERE id = ?");
    query.bindValue(0, id);
    query.exec();
    if (!query.next()) {
        return false;
    }
    out.id = query.record().value("id").toInt();
    out.nickname = query.record().value("nickname").toString();
    out.headId = query.record().value("headid").toInt();
    return true;
}

UserDao::UserDao()
{
    db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName("hhuser.db");
    if (!db.open()) {
        return;
    }

    QSqlQuery query;
    // Password field is TEXT (supports "$salt$hash" format, legacy plaintext, or future bcrypt)
    query.exec(
        "CREATE TABLE IF NOT EXISTS user("
        "  id       INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  password TEXT,"
        "  nickname TEXT,"
        "  headid   INTEGER"
        ")"
    );

    // Seed admin account: id=100000, password="admin123" (hashed on first insert)
    query.prepare("INSERT OR IGNORE INTO user(id, password, nickname, headid) VALUES(?, ?, ?, ?)");
    query.bindValue(0, 100000);
    query.bindValue(1, PasswordHash::hash("admin123"));
    query.bindValue(2, "Administrator");
    query.bindValue(3, 0);
    query.exec();
}

UserDao::~UserDao()
{
    db.close();
}
