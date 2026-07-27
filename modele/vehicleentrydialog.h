#ifndef VEHICLEENTRYDIALOG_H
#define VEHICLEENTRYDIALOG_H

#include <QDialog>
#include "mysqldatabase.h"

class QLineEdit;
class QComboBox;
class QTextEdit;
class QPushButton;
class QTableWidget;

/// 车辆数据录入对话框
class VehicleEntryDialog : public QDialog
{
    Q_OBJECT

public:
    explicit VehicleEntryDialog(QWidget *parent = nullptr);
    ~VehicleEntryDialog() override = default;

private slots:
    void onSave();
    void onRefreshList();
    void onDelete();

private:
    void setupUI();
    void loadRecords();

    MysqlDatabase  m_mysql;

    // 录入表单
    QLineEdit   *m_editPlateNo   = nullptr;
    QComboBox   *m_comboType     = nullptr;
    QLineEdit   *m_editColor     = nullptr;
    QLineEdit   *m_editBrand     = nullptr;
    QLineEdit   *m_editModel     = nullptr;

    // 记录列表
    QTableWidget *m_table        = nullptr;
};

#endif // VEHICLEENTRYDIALOG_H
