#ifndef PROFILEDIALOG_H
#define PROFILEDIALOG_H

#include <QDialog>
#include <QString>

class QLineEdit;
class QPushButton;
class QLabel;

/// 个人账户管理对话框 — 所有用户可通过右上角打开
/// 功能：修改密码、注销账号
class ProfileDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ProfileDialog(const QString &username, QWidget *parent = nullptr);
    ~ProfileDialog() override = default;

    /// 是否请求注销
    bool isAccountDeleted() const { return m_accountDeleted; }

private slots:
    void onChangePassword();
    void onDeleteAccount();

private:
    void setupUI();

    QString m_username;
    int     m_userId    = 0;       // 数据库中的用户 ID
    bool    m_accountDeleted = false;

    QLineEdit   *m_oldPass   = nullptr;
    QLineEdit   *m_newPass   = nullptr;
    QLineEdit   *m_confirmPass = nullptr;
    QPushButton *m_changeBtn = nullptr;
    QLabel      *m_msgLabel  = nullptr;
    QPushButton *m_deleteBtn = nullptr;
};

#endif // PROFILEDIALOG_H
