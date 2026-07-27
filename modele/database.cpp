#include "database.h"
#include "Config.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>

// ============================================================
//  构造函数 / 析构函数
// ============================================================
VehicleDatabase::VehicleDatabase()
{
}

VehicleDatabase::~VehicleDatabase()
{
    if (m_db.isOpen()) {
        m_db.close();
    }
}

// ============================================================
//  初始化
// ============================================================
bool VehicleDatabase::init()
{
    return connectMysql();
}

// ============================================================
//  MySQL 连接（从 Config 读取参数）
// ============================================================
bool VehicleDatabase::connectMysql()
{
    const QString connName = "mysql_vehicle_db";
    if (QSqlDatabase::contains(connName)) {
        m_db = QSqlDatabase::database(connName);
        if (m_db.isOpen()) return true;
    } else {
        m_db = QSqlDatabase::addDatabase("QODBC", connName);
    }

    Config &cfg = Config::getInstance();
    QString connStr = QString(
        "DRIVER={MySQL ODBC 9.7 Unicode Driver};"
        "SERVER=%1;PORT=%2;DATABASE=%3;"
        "UID=%4;PWD=%5;"
        "CHARSET=utf8mb4;"
    ).arg(cfg.getMysqlHost())
     .arg(cfg.getMysqlPort())
     .arg(cfg.getMysqlDbName())
     .arg(cfg.getMysqlUser())
     .arg(cfg.getMysqlPwd());

    m_db.setDatabaseName(connStr);

    if (!m_db.open()) {
        qWarning() << "MySQL connect failed:" << m_db.lastError().text();
        return false;
    }
    qDebug() << "VehicleDatabase: connected to MySQL";
    return true;
}

// ============================================================
//  车牌号标准化
// ============================================================
QString VehicleDatabase::normalizePlate(const QString &plateNo)
{
    if (plateNo.isEmpty()) return {};
    QString normalized = plateNo.simplified().remove(' ');
    return normalized.toUpper();
}

// ============================================================
//  按车牌号查询
// ============================================================
VehicleInfo VehicleDatabase::queryByPlate(const QString &plateNo) const
{
    VehicleInfo info;

    QString normalized = normalizePlate(plateNo);
    if (normalized.isEmpty()) return info;

    QSqlDatabase &db = const_cast<QSqlDatabase &>(m_db);
    if (!db.isOpen()) return info;

    QSqlQuery query(db);
    query.prepare("SELECT id, plate_no, vehicle_type, color, brand, model "
                  "FROM car WHERE plate_no = :plate_no "
                  "ORDER BY record_time DESC LIMIT 1");
    query.bindValue(":plate_no", normalized);

    if (!query.exec()) {
        qWarning() << "MySQL query failed:" << query.lastError().text();
        return info;
    }

    if (query.next()) {
        info.id          = query.value("id").toInt();
        info.plateNo     = query.value("plate_no").toString();
        info.vehicleType = query.value("vehicle_type").toString();
        info.color       = query.value("color").toString();
        info.brand       = query.value("brand").toString();
        info.model       = query.value("model").toString();
    }

    return info;
}

// ============================================================
//  写入新车辆
// ============================================================
bool VehicleDatabase::insertCar(const QString &plateNo, const QString &vehicleType,
                                 const QString &color, const QString &brand, const QString &model)
{
    QString normalized = normalizePlate(plateNo);
    if (normalized.isEmpty()) return false;

    QSqlDatabase &db = const_cast<QSqlDatabase &>(m_db);
    if (!db.isOpen()) return false;

    QSqlQuery query(db);
    query.prepare("INSERT INTO car (plate_no, vehicle_type, color, brand, model, record_time) "
                  "VALUES (:plate_no, :vehicle_type, :color, :brand, :model, NOW())");
    query.bindValue(":plate_no", normalized);
    query.bindValue(":vehicle_type", vehicleType);
    query.bindValue(":color", color);
    query.bindValue(":brand", brand);
    query.bindValue(":model", model);

    if (!query.exec()) {
        qWarning() << "MySQL insert failed:" << query.lastError().text();
        return false;
    }

    qDebug() << "已自动录入新车:" << normalized;
    return true;
}
