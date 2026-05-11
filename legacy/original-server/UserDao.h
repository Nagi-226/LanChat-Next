#ifndef USERDAO_H
#define USERDAO_H

#include <QSqlDatabase>
#include <QString>
#include <vector>
#include "UserEntity.h"

class UserDao
{
public:
    static UserDao* getInstance();
    static void releaseInstance();

    int  insertUser(UserEntity& e);                // hashes password, returns assigned ID
    bool verifyPassword(const UserEntity& e);      // checks password hash, fills nickname/headId on success
    std::vector<UserEntity> getAllUsers();         // returns all registered users
    bool getUserById(int id, UserEntity& out);

private:
    UserDao();
    ~UserDao();
    static UserDao* instance;
    QSqlDatabase db;
};

#endif // USERDAO_H
