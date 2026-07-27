#include "ProfileDialog.h"
#include "UserDao.h"
#include "GlobalUtil.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QMessageBox>
#include <QSqlQuery>
#include <QDebug>

ProfileDialog::ProfileDialog(const QString &username, QWidget *parent)
    : QDialog(parent)
    , m_username(username)
{
    // 查询用户 ID
    UserDao &dao = UserDao::getInstance();
    QVariantList args;
    args << username;
    QSqlQuery q = dao.safeExecute("SELECT id FROM people WHERE username = ?", args);
    if (q.next())
        m_userId = q.value("id").toInt();

    setWindowTitle("个人账户管理");
    setFixedSize(420, 360);
    setupUI();
}

void ProfileDialog::setupUI()
{
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(16);
    mainLayout->setContentsMargins(30, 20, 30, 20);

    // 当前用户
    auto *userLabel = new QLabel(QString("当前用户：%1").arg(m_username), this);
    userLabel->setStyleSheet("font-size: 16px; font-weight: bold;");
    mainLayout->addWidget(userLabel);

    // UID
    auto *uidLabel = new QLabel(QString("账号 UID：%1").arg(m_userId, 6, 10, QChar('0')), this);
    uidLabel->setStyleSheet("font-size: 12px; color: #888;");
    mainLayout->addWidget(uidLabel);

    // ---- 修改密码区域 ----
    auto *formLayout = new QFormLayout;
    formLayout->setSpacing(10);

    m_oldPass = new QLineEdit(this);
    m_oldPass->setEchoMode(QLineEdit::Password);
    m_oldPass->setPlaceholderText("请输入当前密码");
    formLayout->addRow("当前密码：", m_oldPass);

    m_newPass = new QLineEdit(this);
    m_newPass->setEchoMode(QLineEdit::Password);
    m_newPass->setPlaceholderText("请输入新密码（至少6位）");
    formLayout->addRow("新密码：", m_newPass);

    m_confirmPass = new QLineEdit(this);
    m_confirmPass->setEchoMode(QLineEdit::Password);
    m_confirmPass->setPlaceholderText("请再次输入新密码");
    formLayout->addRow("确认密码：", m_confirmPass);

    mainLayout->addLayout(formLayout);

    // 消息提示
    m_msgLabel = new QLabel(this);
    m_msgLabel->setStyleSheet("color: #e74c3c; font-size: 12px;");
    m_msgLabel->setVisible(false);
    mainLayout->addWidget(m_msgLabel);

    // 修改密码按钮
    m_changeBtn = new QPushButton("修改密码", this);
    m_changeBtn->setCursor(Qt::PointingHandCursor);
    connect(m_changeBtn, &QPushButton::clicked, this, &ProfileDialog::onChangePassword);
    mainLayout->addWidget(m_changeBtn);

    mainLayout->addStretch();

    // ---- 注销账号 ----
    auto *sep = new QLabel("━━ 危险操作 ━━", this);
    sep->setAlignment(Qt::AlignCenter);
    sep->setStyleSheet("color: #999; font-size: 11px;");
    mainLayout->addWidget(sep);

    m_deleteBtn = new QPushButton("注销当前账号", this);
    m_deleteBtn->setObjectName("dangerBtn");
    m_deleteBtn->setStyleSheet(
        "QPushButton { background: #e74c3c; color: white; border: none;"
        "  padding: 8px; border-radius: 4px; font-size: 13px; }"
        "QPushButton:hover { background: #c0392b; }");
    m_deleteBtn->setCursor(Qt::PointingHandCursor);
    connect(m_deleteBtn, &QPushButton::clicked, this, &ProfileDialog::onDeleteAccount);
    mainLayout->addWidget(m_deleteBtn);
}

void ProfileDialog::onChangePassword()
{
    QString oldPass = m_oldPass->text().trimmed();
    QString newPass = m_newPass->text().trimmed();
    QString confirm = m_confirmPass->text().trimmed();

    // 校验
    if (oldPass.isEmpty() || newPass.isEmpty() || confirm.isEmpty())
    {
        m_msgLabel->setText("请填写所有密码字段");
        m_msgLabel->setVisible(true);
        return;
    }

    if (newPass.length() < 6)
    {
        m_msgLabel->setText("新密码长度不能少于 6 位");
        m_msgLabel->setVisible(true);
        return;
    }

    if (newPass != confirm)
    {
        m_msgLabel->setText("两次输入的新密码不一致");
        m_msgLabel->setVisible(true);
        return;
    }

    // 验证旧密码
    UserDao &dao = UserDao::getInstance();
    if (!dao.checkUserLogin(m_username, oldPass))
    {
        m_msgLabel->setText("当前密码错误");
        m_msgLabel->setVisible(true);
        return;
    }

    // 更新密码
    QVariantMap userMap;
    userMap["password"] = newPass;
    // 通过用户名查找 id
    QVariantList args;
    args << m_username;
    QSqlQuery q = dao.safeExecute("SELECT id FROM people WHERE username = ?", args);
    if (!q.next())
    {
        m_msgLabel->setText("用户不存在");
        m_msgLabel->setVisible(true);
        return;
    }
    userMap["id"] = q.value("id").toInt();

    if (dao.update(userMap))
    {
        QMessageBox::information(this, "成功", "密码修改成功");
        m_oldPass->clear();
        m_newPass->clear();
        m_confirmPass->clear();
        m_msgLabel->setVisible(false);
    }
    else
    {
        m_msgLabel->setText("密码修改失败，请重试");
        m_msgLabel->setVisible(true);
    }
}

void ProfileDialog::onDeleteAccount()
{
    auto reply = QMessageBox::warning(this, "确认注销",
        QString("确定要注销账号「%1」吗？\n\n"
                "注销后该账号将无法登录，此操作不可撤销！")
            .arg(m_username),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);

    if (reply != QMessageBox::Yes)
        return;

    // 确认二次弹窗
    auto confirm = QMessageBox::question(this, "再次确认",
        "请再次确认：是否永久注销此账号？",
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);

    if (confirm != QMessageBox::Yes)
        return;

    // 执行删除
    UserDao &dao = UserDao::getInstance();
    QVariantList args;
    args << m_username;
    dao.safeExecute("DELETE FROM people WHERE username = ?", args);

    m_accountDeleted = true;
    QMessageBox::information(this, "已注销", "账号已成功注销");
    accept();  // 关闭对话框
}
