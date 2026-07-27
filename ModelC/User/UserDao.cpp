#include "UserDao.h"
#include <QVariantList>

UserDao UserDao::instance;

UserDao& UserDao::getInstance()
{
    return instance;
}

QVariant UserDao::selectById(qint64 id)
{
    QVariantList args;
    args << id;
    QSqlQuery query = safeExecute("SELECT * FROM people WHERE id = ?", args);
    QVariantMap map;
    if(query.next())
    {
        map["id"] = query.value("id");
        map["username"] = query.value("username");
        map["password"] = query.value("password");
        map["invite_code"] = query.value("invite_code");
        map["is_special"] = query.value("is_special");
    }
    return map;
}

bool UserDao::insert(const QVariant& model)
{
    QVariantMap userMap = model.toMap();
    QVariantList args;
    QString inviteCode = userMap["invite_code"].toString();
    bool hasInvite = (inviteCode == "manager");

    // 有邀请码则标记为特殊用户
    args << userMap["username"] << userMap["password"]
         << inviteCode << (hasInvite ? 1 : 0);
    QSqlQuery query = safeExecute(
        "INSERT INTO people(username,password,invite_code,is_special) VALUES(?,?,?,?)",
        args);
    return query.isActive();
}

bool UserDao::deleteById(qint64 id)
{
    QVariantList args;
    args << id;
    safeExecute("DELETE FROM people WHERE id = ?", args);
    return true;
}

bool UserDao::update(const QVariant& model)
{
    QVariantMap userMap = model.toMap();
    QVariantList args;
    args << userMap["password"] << userMap["id"];
    safeExecute("UPDATE people SET password = ? WHERE id = ?", args);
    return true;
}

bool UserDao::checkUserLogin(const QString &username, const QString &password)
{
    QVariantList args;
    args << username << password;
    QSqlQuery query = safeExecute(
        "SELECT username FROM people WHERE username = ? AND password = ?",
        args);
    return query.next();
}

bool UserDao::isUsernameExist(const QString &username)
{
    QVariantList args;
    args << username;
    QSqlQuery query = safeExecute(
        "SELECT username FROM people WHERE username = ?",
        args);
    return query.next();
}

bool UserDao::isUserSpecial(const QString &username)
{
    QVariantList args;
    args << username;
    QSqlQuery query = safeExecute(
        "SELECT is_special FROM people WHERE username = ?",
        args);
    if (query.next()) {
        return query.value("is_special").toInt() == 1;
    }
    return false;
}
