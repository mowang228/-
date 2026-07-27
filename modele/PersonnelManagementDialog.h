#ifndef PERSONNELMANAGEMENTDIALOG_H
#define PERSONNELMANAGEMENTDIALOG_H

#include <QDialog>
#include <QString>

class QTableWidget;
class QLineEdit;
class QPushButton;
class QLabel;

/// 人员管理对话框（管理员专用）
/// 位于 modele/ 数据库模块中
class PersonnelManagementDialog : public QDialog
{
    Q_OBJECT

public:
    explicit PersonnelManagementDialog(QWidget *parent = nullptr);
    ~PersonnelManagementDialog() override = default;

private slots:
    void onSearch();
    void onToggleSpecial();
    void onResetPassword();
    void onDeleteUser();
    void onRefresh();

private:
    void setupUI();
    void loadUsers(const QString &filter = QString());

    QTableWidget *m_table    = nullptr;
    QLineEdit    *m_search   = nullptr;
    QPushButton  *m_searchBtn = nullptr;
    QLabel       *m_status   = nullptr;
};

#endif // PERSONNELMANAGEMENTDIALOG_H
