#include "vehicleentrydialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QComboBox>
#include <QPushButton>
#include <QTableWidget>
#include <QHeaderView>
#include <QMessageBox>
#include <QGroupBox>
#include <QSplitter>

VehicleEntryDialog::VehicleEntryDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle("车辆数据录入");
    resize(900, 600);

    setupUI();

    // 连接 MySQL
    if (!m_mysql.connect()) {
        QMessageBox::warning(this, "数据库错误",
            "无法连接到 MySQL 数据库:\n" + m_mysql.lastError());
    } else {
        loadRecords();
    }
}

void VehicleEntryDialog::setupUI()
{
    auto *mainLayout = new QVBoxLayout(this);
    auto *splitter   = new QSplitter(Qt::Vertical, this);

    // ============================================================
    //  上半部分：录入表单
    // ============================================================
    auto *formGroup = new QGroupBox("录入车辆信息");
    auto *formLayout = new QFormLayout(formGroup);

    m_editPlateNo = new QLineEdit;
    formLayout->addRow("车牌号：", m_editPlateNo);

    m_comboType = new QComboBox;
    m_comboType->addItems({"轿车", "SUV", "客车", "货车", "其他"});
    m_comboType->setCurrentIndex(-1);
    formLayout->addRow("车辆类型：", m_comboType);

    m_editColor = new QLineEdit;
    formLayout->addRow("车身颜色：", m_editColor);

    m_editBrand = new QLineEdit;
    formLayout->addRow("品牌：", m_editBrand);

    m_editModel = new QLineEdit;
    formLayout->addRow("型号：", m_editModel);

    auto *btnLayout = new QHBoxLayout;
    auto *saveBtn = new QPushButton("保存");
    auto *clearBtn = new QPushButton("清空");
    btnLayout->addStretch();
    btnLayout->addWidget(saveBtn);
    btnLayout->addWidget(clearBtn);

    formLayout->addRow("", btnLayout);

    splitter->addWidget(formGroup);

    // ============================================================
    //  下半部分：已录入记录列表
    // ============================================================
    auto *listGroup = new QGroupBox("已录入记录");
    auto *listLayout = new QVBoxLayout(listGroup);

    m_table = new QTableWidget;
    m_table->setColumnCount(7);
    m_table->setHorizontalHeaderLabels({"ID", "车牌号", "类型", "颜色", "品牌", "型号", "录入时间"});
    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setColumnHidden(0, true);  // 隐藏 ID 列

    auto *listBtnLayout = new QHBoxLayout;
    auto *refreshBtn = new QPushButton("刷新");
    auto *deleteBtn = new QPushButton("删除选中");
    listBtnLayout->addStretch();
    listBtnLayout->addWidget(refreshBtn);
    listBtnLayout->addWidget(deleteBtn);

    listLayout->addWidget(m_table);
    listLayout->addLayout(listBtnLayout);

    splitter->addWidget(listGroup);

    mainLayout->addWidget(splitter);

    // ---- 信号连接 ----
    connect(saveBtn,    &QPushButton::clicked, this, &VehicleEntryDialog::onSave);
    connect(clearBtn,   &QPushButton::clicked, this, [this]() {
        m_editPlateNo->clear();
        m_comboType->setCurrentIndex(-1);
        m_editColor->clear();
        m_editBrand->clear();
        m_editModel->clear();
    });
    connect(refreshBtn, &QPushButton::clicked, this, &VehicleEntryDialog::onRefreshList);
    connect(deleteBtn,  &QPushButton::clicked, this, &VehicleEntryDialog::onDelete);
}

// ============================================================
//  保存
// ============================================================
void VehicleEntryDialog::onSave()
{
    // 基本校验
    if (m_editPlateNo->text().trimmed().isEmpty()) {
        QMessageBox::warning(this, "提示", "请输入车牌号");
        m_editPlateNo->setFocus();
        return;
    }

    if (!m_mysql.isConnected()) {
        QMessageBox::warning(this, "错误", "数据库未连接");
        return;
    }

    VehicleRecord rec;
    rec.plateNo     = m_editPlateNo->text().trimmed().toUpper();
    rec.vehicleType = m_comboType->currentText();
    rec.color       = m_editColor->text().trimmed();
    rec.brand       = m_editBrand->text().trimmed();
    rec.model       = m_editModel->text().trimmed();

    int newId = m_mysql.insertRecord(rec);
    if (newId < 0) {
        QMessageBox::critical(this, "错误", "保存失败:\n" + m_mysql.lastError());
        return;
    }

    QMessageBox::information(this, "成功",
        QString("车辆记录已保存（ID: %1）").arg(newId));

    // 清空表单
    m_editPlateNo->clear();
    m_comboType->setCurrentIndex(-1);
    m_editColor->clear();
    m_editBrand->clear();
    m_editModel->clear();
    m_editPlateNo->setFocus();

    // 刷新列表
    loadRecords();
}

// ============================================================
//  刷新列表
// ============================================================
void VehicleEntryDialog::onRefreshList()
{
    loadRecords();
}

void VehicleEntryDialog::loadRecords()
{
    if (!m_mysql.isConnected()) return;

    auto records = m_mysql.queryAllRecords();
    m_table->setRowCount(records.size());

    for (int i = 0; i < records.size(); ++i) {
        const auto &r = records[i];
        m_table->setItem(i, 0, new QTableWidgetItem(QString::number(r.id)));
        m_table->setItem(i, 1, new QTableWidgetItem(r.plateNo));
        m_table->setItem(i, 2, new QTableWidgetItem(r.vehicleType));
        m_table->setItem(i, 3, new QTableWidgetItem(r.color));
        m_table->setItem(i, 4, new QTableWidgetItem(r.brand));
        m_table->setItem(i, 5, new QTableWidgetItem(r.model));
        m_table->setItem(i, 6, new QTableWidgetItem(r.recordTime));
    }

    m_table->resizeColumnsToContents();
}

// ============================================================
//  删除选中记录
// ============================================================
void VehicleEntryDialog::onDelete()
{
    int row = m_table->currentRow();
    if (row < 0) {
        QMessageBox::information(this, "提示", "请先选中要删除的记录");
        return;
    }

    int id = m_table->item(row, 0)->text().toInt();
    QString plateNo = m_table->item(row, 1)->text();

    auto ret = QMessageBox::question(this, "确认删除",
        QString("确定要删除 %1 的记录吗？").arg(plateNo),
        QMessageBox::Yes | QMessageBox::No);

    if (ret != QMessageBox::Yes) return;

    if (m_mysql.deleteRecord(id)) {
        loadRecords();
    } else {
        QMessageBox::warning(this, "错误", "删除失败:\n" + m_mysql.lastError());
    }
}
