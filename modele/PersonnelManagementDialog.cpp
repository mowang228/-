#include "PersonnelManagementDialog.h"
#include "UserDao.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTableWidget>
#include <QHeaderView>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QMessageBox>
#include <QSqlQuery>
#include <QDebug>

PersonnelManagementDialog::PersonnelManagementDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle("人员管理");
    setMinimumSize(700, 500);
    resize(750, 550);
    setupUI();
    loadUsers();
}

void PersonnelManagementDialog::setupUI()
{
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(12);
    mainLayout->setContentsMargins(20, 16, 20, 16);

    // 标题
    auto *title = new QLabel("人员管理（管理员）", this);
    title->setStyleSheet("font-size: 18px; font-weight: bold;");
    mainLayout->addWidget(title);

    // 搜索行
    auto *searchLayout = new QHBoxLayout;
    m_search = new QLineEdit(this);
    m_search->setPlaceholderText("按用户名搜索…");
    m_search->setMinimumHeight(32);
    searchLayout->addWidget(m_search);

    m_searchBtn = new QPushButton("搜索", this);
    m_searchBtn->setCursor(Qt::PointingHandCursor);
    m_searchBtn->setMinimumHeight(32);
    connect(m_searchBtn, &QPushButton::clicked, this, &PersonnelManagementDialog::onSearch);
    searchLayout->addWidget(m_searchBtn);

    auto *refreshBtn = new QPushButton("刷新", this);
    refreshBtn->setCursor(Qt::PointingHandCursor);
    refreshBtn->setMinimumHeight(32);
    connect(refreshBtn, &QPushButton::clicked, this, &PersonnelManagementDialog::onRefresh);
    searchLayout->addWidget(refreshBtn);

    mainLayout->addLayout(searchLayout);

    // 表格
    m_table = new QTableWidget(0, 4, this);
    m_table->setHorizontalHeaderLabels({"ID", "用户名", "邀请码", "管理员权限"});
    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_table->verticalHeader()->setVisible(false);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setMinimumHeight(260);
    mainLayout->addWidget(m_table);

    // 状态提示
    m_status = new QLabel(this);
    m_status->setStyleSheet("color: #888; font-size: 12px;");
    mainLayout->addWidget(m_status);

    // 操作按钮行
    auto *btnLayout = new QHBoxLayout;

    auto *toggleBtn = new QPushButton("切换管理员权限", this);
    toggleBtn->setCursor(Qt::PointingHandCursor);
    connect(toggleBtn, &QPushButton::clicked, this, &PersonnelManagementDialog::onToggleSpecial);
    btnLayout->addWidget(toggleBtn);

    auto *resetPwdBtn = new QPushButton("重置密码", this);
    resetPwdBtn->setCursor(Qt::PointingHandCursor);
    connect(resetPwdBtn, &QPushButton::clicked, this, &PersonnelManagementDialog::onResetPassword);
    btnLayout->addWidget(resetPwdBtn);

    auto *deleteBtn = new QPushButton("删除用户", this);
    deleteBtn->setCursor(Qt::PointingHandCursor);
    deleteBtn->setStyleSheet(
        "QPushButton { background: #e74c3c; color: white; border: none;"
        "  padding: 6px 12px; border-radius: 4px; }"
        "QPushButton:hover { background: #c0392b; }");
    connect(deleteBtn, &QPushButton::clicked, this, &PersonnelManagementDialog::onDeleteUser);
    btnLayout->addWidget(deleteBtn);

    mainLayout->addLayout(btnLayout);
}

void PersonnelManagementDialog::loadUsers(const QString &filter)
{
    m_table->setRowCount(0);
    UserDao &dao = UserDao::getInstance();
    QSqlQuery query;

    if (filter.isEmpty())
        query = dao.safeExecute("SELECT id, username, invite_code, is_special FROM people ORDER BY id");
    else
    {
        QVariantList args;
        args << QString("%%%1%").arg(filter);
        query = dao.safeExecute(
            "SELECT id, username, invite_code, is_special FROM people WHERE username LIKE ? ORDER BY id",
            args);
    }

    int row = 0;
    while (query.next())
    {
        m_table->insertRow(row);
        m_table->setItem(row, 0, new QTableWidgetItem(query.value("id").toString()));
        m_table->setItem(row, 1, new QTableWidgetItem(query.value("username").toString()));
        m_table->setItem(row, 2, new QTableWidgetItem(query.value("invite_code").toString()));
        bool special = query.value("is_special").toInt() == 1;
        auto *item = new QTableWidgetItem(special ? "是" : "否");
        item->setForeground(special ? QColor("#27ae60") : QColor("#999"));
        m_table->setItem(row, 3, item);
        ++row;
    }

    m_status->setText(QString("共 %1 个用户").arg(row));
}

void PersonnelManagementDialog::onSearch()
{
    loadUsers(m_search->text().trimmed());
}

void PersonnelManagementDialog::onRefresh()
{
    m_search->clear();
    loadUsers();
}

void PersonnelManagementDialog::onToggleSpecial()
{
    int row = m_table->currentRow();
    if (row < 0)
    {
        QMessageBox::information(this, "提示", "请先选择一个用户");
        return;
    }

    int id = m_table->item(row, 0)->text().toInt();
    QString username = m_table->item(row, 1)->text();
    bool currentSpecial = (m_table->item(row, 3)->text() == "是");

    auto reply = QMessageBox::question(this, "确认",
        QString("确定将用户「%1」的权限%2吗？")
            .arg(username)
            .arg(currentSpecial ? "取消管理员" : "设为管理员"),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);

    if (reply != QMessageBox::Yes)
        return;

    UserDao &dao = UserDao::getInstance();
    QVariantList args;
    int newVal = currentSpecial ? 0 : 1;
    args << newVal << id;
    dao.safeExecute("UPDATE people SET is_special = ? WHERE id = ?", args);

    loadUsers(m_search->text().trimmed());
}

void PersonnelManagementDialog::onResetPassword()
{
    int row = m_table->currentRow();
    if (row < 0)
    {
        QMessageBox::information(this, "提示", "请先选择一个用户");
        return;
    }

    int id = m_table->item(row, 0)->text().toInt();
    QString username = m_table->item(row, 1)->text();

    auto reply = QMessageBox::question(this, "确认",
        QString("确定要重置用户「%1」的密码为 123456 吗？").arg(username),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);

    if (reply != QMessageBox::Yes)
        return;

    UserDao &dao = UserDao::getInstance();
    QVariantList args;
    args << "123456" << id;
    dao.safeExecute("UPDATE people SET password = ? WHERE id = ?", args);

    QMessageBox::information(this, "成功",
        QString("用户「%1」的密码已重置为 123456").arg(username));
}

void PersonnelManagementDialog::onDeleteUser()
{
    int row = m_table->currentRow();
    if (row < 0)
    {
        QMessageBox::information(this, "提示", "请先选择一个用户");
        return;
    }

    int id = m_table->item(row, 0)->text().toInt();
    QString username = m_table->item(row, 1)->text();

    auto reply = QMessageBox::warning(this, "确认删除",
        QString("确定要删除用户「%1」吗？\n此操作不可撤销！").arg(username),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);

    if (reply != QMessageBox::Yes)
        return;

    UserDao &dao = UserDao::getInstance();
    dao.deleteById(id);

    loadUsers(m_search->text().trimmed());
}
