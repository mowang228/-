#include "LoginDialog.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QStackedWidget>
#include <QMessageBox>
#include <QFile>
#include <QApplication>

// ============================================================
//  构造函数
// ============================================================
LoginDialog::LoginDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle("套牌车稽查系统 — 用户登录");
    setFixedSize(420, 420);
    setObjectName("loginDialog");

    setupUI();

    // 认证服务
    m_authService = new AuthService(this);
    connect(m_authService, &AuthService::loginCompleted,
            this, &LoginDialog::onLoginResult);
    connect(m_authService, &AuthService::registerCompleted,
            this, &LoginDialog::onRegisterResult);
}

// ============================================================
//  界面搭建
// ============================================================
void LoginDialog::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(40, 30, 40, 30);
    mainLayout->setSpacing(16);

    // ---- 标题 ----
    QLabel *title = new QLabel("套牌车稽查系统", this);
    title->setObjectName("loginTitle");
    title->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(title);

    // ---- 页面栈（登录页 / 注册页） ----
    QStackedWidget *stack = new QStackedWidget(this);
    stack->setObjectName("loginStack");

    // ---- 登录页 ----
    m_loginPage = new QWidget(this);
    QVBoxLayout *loginLayout = new QVBoxLayout(m_loginPage);
    loginLayout->setSpacing(12);

    m_loginUser = new QLineEdit(this);
    m_loginUser->setPlaceholderText("用户名");
    m_loginUser->setObjectName("loginInput");

    m_loginPass = new QLineEdit(this);
    m_loginPass->setPlaceholderText("密码");
    m_loginPass->setEchoMode(QLineEdit::Password);
    m_loginPass->setObjectName("loginInput");

    m_loginBtn = new QPushButton("登  录", this);
    m_loginBtn->setObjectName("loginBtn");
    m_loginBtn->setCursor(Qt::PointingHandCursor);
    connect(m_loginBtn, &QPushButton::clicked, this, &LoginDialog::onLoginClicked);

    m_loginMsg = new QLabel(this);
    m_loginMsg->setObjectName("loginMsg");
    m_loginMsg->setAlignment(Qt::AlignCenter);
    m_loginMsg->setWordWrap(true);

    m_switchToReg = new QPushButton("没有账号？去注册", this);
    m_switchToReg->setObjectName("switchBtn");
    m_switchToReg->setCursor(Qt::PointingHandCursor);
    connect(m_switchToReg, &QPushButton::clicked, this, &LoginDialog::switchToRegister);

    loginLayout->addWidget(m_loginUser);
    loginLayout->addWidget(m_loginPass);
    loginLayout->addWidget(m_loginBtn);
    loginLayout->addWidget(m_loginMsg);
    loginLayout->addWidget(m_switchToReg);

    // ---- 注册页 ----
    m_registerPage = new QWidget(this);
    QVBoxLayout *regLayout = new QVBoxLayout(m_registerPage);
    regLayout->setSpacing(12);

    m_regUser = new QLineEdit(this);
    m_regUser->setPlaceholderText("用户名");
    m_regUser->setObjectName("loginInput");

    m_regPass = new QLineEdit(this);
    m_regPass->setPlaceholderText("密码（至少 6 位）");
    m_regPass->setEchoMode(QLineEdit::Password);
    m_regPass->setObjectName("loginInput");

    m_regPass2 = new QLineEdit(this);
    m_regPass2->setPlaceholderText("确认密码");
    m_regPass2->setEchoMode(QLineEdit::Password);
    m_regPass2->setObjectName("loginInput");

    m_regInvite = new QLineEdit(this);
    m_regInvite->setPlaceholderText("邀请码（可选）");
    m_regInvite->setObjectName("loginInput");

    m_regBtn = new QPushButton("注  册", this);
    m_regBtn->setObjectName("loginBtn");
    m_regBtn->setCursor(Qt::PointingHandCursor);
    connect(m_regBtn, &QPushButton::clicked, this, &LoginDialog::onRegisterClicked);

    m_regMsg = new QLabel(this);
    m_regMsg->setObjectName("loginMsg");
    m_regMsg->setAlignment(Qt::AlignCenter);
    m_regMsg->setWordWrap(true);

    m_switchToLogin = new QPushButton("已有账号？去登录", this);
    m_switchToLogin->setObjectName("switchBtn");
    m_switchToLogin->setCursor(Qt::PointingHandCursor);
    connect(m_switchToLogin, &QPushButton::clicked, this, &LoginDialog::switchToLogin);

    regLayout->addWidget(m_regUser);
    regLayout->addWidget(m_regPass);
    regLayout->addWidget(m_regPass2);
    regLayout->addWidget(m_regInvite);
    regLayout->addWidget(m_regBtn);
    regLayout->addWidget(m_regMsg);
    regLayout->addWidget(m_switchToLogin);

    // 加入栈
    stack->addWidget(m_loginPage);
    stack->addWidget(m_registerPage);
    stack->setCurrentIndex(0);

    mainLayout->addWidget(stack);

    // ---- 底部提示 ----
    QLabel *footer = new QLabel("演示版 · 密码至少 6 位", this);
    footer->setObjectName("loginFooter");
    footer->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(footer);
}

// ============================================================
//  槽函数
// ============================================================
void LoginDialog::onLoginClicked()
{
    QString user = m_loginUser->text().trimmed();
    QString pass = m_loginPass->text();

    if (user.isEmpty() || pass.isEmpty())
    {
        m_loginMsg->setText("请输入用户名和密码");
        return;
    }

    setFormEnabled(false);
    m_loginMsg->setText("正在登录…");
    m_authService->login(user, pass);
}

void LoginDialog::onRegisterClicked()
{
    QString user   = m_regUser->text().trimmed();
    QString pass   = m_regPass->text();
    QString pass2  = m_regPass2->text();
    QString invite = m_regInvite->text().trimmed();

    if (user.isEmpty())
    {
        m_regMsg->setText("请输入用户名");
        return;
    }
    if (pass.length() < 6)
    {
        m_regMsg->setText("密码长度不能少于 6 位");
        return;
    }
    if (pass != pass2)
    {
        m_regMsg->setText("两次输入的密码不一致");
        return;
    }

    setFormEnabled(false);
    m_regMsg->setText("正在注册…");
    m_authService->registerUser(user, pass, invite);
}

void LoginDialog::switchToRegister()
{
    m_loginMsg->clear();
    QStackedWidget *stack = findChild<QStackedWidget *>("loginStack");
    if (stack) stack->setCurrentIndex(1);
}

void LoginDialog::switchToLogin()
{
    m_regMsg->clear();
    QStackedWidget *stack = findChild<QStackedWidget *>("loginStack");
    if (stack) stack->setCurrentIndex(0);
}

void LoginDialog::onLoginResult(const AuthResult &result)
{
    setFormEnabled(true);

    if (result.success)
    {
        m_loggedInUser = result.username;
        m_isSpecial    = result.isSpecial;
        accept();   // 关闭对话框，返回 Accepted
    }
    else
    {
        m_loginMsg->setText(result.message);
    }
}

void LoginDialog::onRegisterResult(const AuthResult &result)
{
    setFormEnabled(true);

    if (result.success)
    {
        QMessageBox::information(this, "注册成功", result.message);
        switchToLogin();
        m_loginUser->setText(m_regUser->text());
    }
    else
    {
        m_regMsg->setText(result.message);
    }
}

void LoginDialog::setFormEnabled(bool enabled)
{
    m_loginUser->setEnabled(enabled);
    m_loginPass->setEnabled(enabled);
    m_loginBtn->setEnabled(enabled);
    m_regUser->setEnabled(enabled);
    m_regPass->setEnabled(enabled);
    m_regPass2->setEnabled(enabled);
    m_regInvite->setEnabled(enabled);
    m_regBtn->setEnabled(enabled);
}
