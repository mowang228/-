#ifndef AUTHSERVICE_H
#define AUTHSERVICE_H

#include <QObject>
#include <QString>

/// 认证结果
struct AuthResult
{
    bool    success   = false;
    QString message;       // 成功/失败提示信息
    QString username;      // 登录成功后的用户名
    bool    isSpecial = false;  // 是否特殊用户（有邀请码）
};

class AuthService : public QObject
{
    Q_OBJECT

public:
    explicit AuthService(QObject *parent = nullptr);
    ~AuthService() override = default;

    /// 登录
    void login(const QString &username, const QString &password);

    /// 注册（支持邀请码）
    void registerUser(const QString &username, const QString &password,
                      const QString &inviteCode = QString());

signals:
    void loginCompleted(const AuthResult &result);
    void registerCompleted(const AuthResult &result);

private:
    void onLoginInternal(const QString &username, const QString &password);
    void onRegisterInternal(const QString &username, const QString &password,
                            const QString &inviteCode);
};

#endif // AUTHSERVICE_H
