#ifndef LOGINDIALOG_H
#define LOGINDIALOG_H

#include <QDialog>
#include "AuthService.h"

class QLineEdit;
class QPushButton;
class QLabel;
class QStackedWidget;

class LoginDialog : public QDialog
{
    Q_OBJECT

public:
    explicit LoginDialog(QWidget *parent = nullptr);
    ~LoginDialog() override = default;

    /// 登录成功后获取当前用户名
    QString loggedInUser() const { return m_loggedInUser; }

    /// 是否特殊用户（输入邀请码的用户）
    bool isSpecialUser() const { return m_isSpecial; }

private slots:
    void onLoginClicked();
    void onRegisterClicked();
    void switchToRegister();
    void switchToLogin();

    void onLoginResult(const AuthResult &result);
    void onRegisterResult(const AuthResult &result);

private:
    void setupUI();
    void setFormEnabled(bool enabled);

    // 登录页控件
    QWidget     *m_loginPage     = nullptr;
    QLineEdit   *m_loginUser     = nullptr;
    QLineEdit   *m_loginPass     = nullptr;
    QPushButton *m_loginBtn      = nullptr;
    QLabel      *m_loginMsg      = nullptr;

    // 注册页控件
    QWidget     *m_registerPage  = nullptr;
    QLineEdit   *m_regUser       = nullptr;
    QLineEdit   *m_regPass       = nullptr;
    QLineEdit   *m_regPass2      = nullptr;
    QLineEdit   *m_regInvite     = nullptr;   // 邀请码（可选）
    QPushButton *m_regBtn        = nullptr;
    QLabel      *m_regMsg        = nullptr;

    // 切换按钮
    QPushButton *m_switchToReg   = nullptr;
    QPushButton *m_switchToLogin = nullptr;

    // 认证服务
    AuthService *m_authService   = nullptr;

    // 当前登录用户
    QString      m_loggedInUser;
    bool         m_isSpecial     = false;
};

#endif // LOGINDIALOG_H
