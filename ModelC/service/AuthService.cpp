#include "AuthService.h"
#include <QTimer>
#include <QDebug>
#include "UserDao.h"
#include "GlobalUtil.h"

AuthService::AuthService(QObject *parent)
    : QObject(parent)
{
}

void AuthService::login(const QString &username, const QString &password)
{
    QTimer::singleShot(800, this, [this, username, password]() {
        onLoginInternal(username, password);
    });
}

void AuthService::registerUser(const QString &username, const QString &password,
                                const QString &inviteCode)
{
    QTimer::singleShot(800, this, [this, username, password, inviteCode]() {
        onRegisterInternal(username, password, inviteCode);
    });
}

void AuthService::onLoginInternal(const QString &username, const QString &password)
{
    AuthResult result;

    if (username.isEmpty())
    {
        result.success = false;
        result.message = "用户名不能为空";
        emit loginCompleted(result);
        return;
    }
    if (password.isEmpty())
    {
        result.success = false;
        result.message = "密码不能为空";
        emit loginCompleted(result);
        return;
    }

    // 调用 UserDao 查询
    UserDao &dao = UserDao::getInstance();
    if (dao.checkUserLogin(username, password))
    {
        result.success  = true;
        result.message  = "登录成功";
        result.username = username;
        // 查询用户的特殊状态
        result.isSpecial = dao.isUserSpecial(username);
    }
    else
    {
        result.success = false;
        result.message = "用户名或密码错误";
    }

    emit loginCompleted(result);
}

void AuthService::onRegisterInternal(const QString &username, const QString &password,
                                      const QString &inviteCode)
{
    AuthResult result;

    if (GlobalUtil::strIsEmpty(username))
    {
        result.success = false;
        result.message = "用户名不能为空";
        emit registerCompleted(result);
        return;
    }
    if (password.length() < 6)
    {
        result.success = false;
        result.message = "密码长度不能少于 6 位";
        emit registerCompleted(result);
        return;
    }

    UserDao &dao = UserDao::getInstance();
    if (dao.isUsernameExist(username))
    {
        result.success = false;
        result.message = "该用户名已被注册";
    }
    else
    {
        // 封装用户数据插入，包含邀请码
        QVariantMap user;
        user["username"]    = username;
        user["password"]    = password;
        user["invite_code"] = inviteCode;
        dao.insert(user);

        result.success  = true;
        result.message  = "注册成功，请登录";
        result.username = username;
        GlobalUtil::printLog("用户注册成功：" + username +
                            (inviteCode.isEmpty() ? "" : " (邀请码: " + inviteCode + ")"));
    }

    emit registerCompleted(result);
}
