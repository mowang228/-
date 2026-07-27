#include "mysqldatabase.h"
#include "Config.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QSqlRecord>
#include <QDebug>
#include <QDateTime>

// ============================================================
//  构造函数 / 析构
// ============================================================
MysqlDatabase::MysqlDatabase()
{
}

MysqlDatabase::~MysqlDatabase()
{
    disconnect();
}

// ============================================================
//  连接
// ============================================================
bool MysqlDatabase::connect(const QString &host, int port,
                            const QString &dbName,
                            const QString &user,
                            const QString &password)
{
    // 无参调用时使用 Config 配置
    QString h = host;
    int p = port;
    QString d = dbName;
    QString u = user;
    QString pw = password;

    if (h.isEmpty()) {
        Config &cfg = Config::getInstance();
        h  = cfg.getMysqlHost();
        p  = cfg.getMysqlPort();
        d  = cfg.getMysqlDbName();
        u  = cfg.getMysqlUser();
        pw = cfg.getMysqlPwd();
    }

    const QString connName = "mysql_car_check";
    if (QSqlDatabase::contains(connName)) {
        m_db = QSqlDatabase::database(connName);
        if (m_db.isOpen()) return true;
    } else {
        m_db = QSqlDatabase::addDatabase("QODBC", connName);
    }

    // ODBC 连接字符串
    QString connStr = QString(
        "DRIVER={MySQL ODBC 9.7 Unicode Driver};"
        "SERVER=%1;"
        "PORT=%2;"
        "DATABASE=%3;"
        "UID=%4;"
        "PWD=%5;"
        "CHARSET=utf8mb4;"
    ).arg(h).arg(p).arg(d).arg(u).arg(pw);

    m_db.setDatabaseName(connStr);

    if (!m_db.open()) {
        m_lastError = m_db.lastError().text();
        qWarning() << "MySQL connect failed:" << m_lastError;
        return false;
    }

    qDebug() << "MySQL connected:" << host << port << dbName;
    return true;
}

void MysqlDatabase::disconnect()
{
    if (m_db.isOpen()) {
        m_db.close();
    }
}

bool MysqlDatabase::isConnected() const
{
    return m_db.isOpen();
}

// ============================================================
//  插入记录
// ============================================================
int MysqlDatabase::insertRecord(const VehicleRecord &record)
{
    if (!m_db.isOpen()) {
        m_lastError = "Database not connected";
        return -1;
    }

    QSqlQuery query(m_db);
    query.prepare(
        "INSERT INTO car "
        "(plate_no, vehicle_type, color, brand, model, record_time) "
        "VALUES (:plate_no, :vehicle_type, :color, :brand, :model, NOW())"
    );

    query.bindValue(":plate_no",     record.plateNo);
    query.bindValue(":vehicle_type", record.vehicleType);
    query.bindValue(":color",        record.color);
    query.bindValue(":brand",        record.brand);
    query.bindValue(":model",        record.model);

    if (!query.exec()) {
        m_lastError = query.lastError().text();
        qWarning() << "Insert failed:" << m_lastError;
        return -1;
    }

    // 返回自增 id
    return query.lastInsertId().toInt();
}

// ============================================================
//  查询所有记录
// ============================================================
QList<VehicleRecord> MysqlDatabase::queryAllRecords() const
{
    QList<VehicleRecord> records;
    if (!m_db.isOpen()) return records;

    QSqlQuery query(m_db);
    query.exec("SELECT * FROM car ORDER BY record_time DESC");

    while (query.next()) {
        records.append(rowToRecord(query));
    }
    return records;
}

// ============================================================
//  按车牌号搜索
// ============================================================
QList<VehicleRecord> MysqlDatabase::searchByPlate(const QString &keyword) const
{
    QList<VehicleRecord> records;
    if (!m_db.isOpen()) return records;

    QSqlQuery query(m_db);
    query.prepare("SELECT * FROM car WHERE plate_no LIKE :kw ORDER BY record_time DESC");
    query.bindValue(":kw", "%" + keyword + "%");

    if (!query.exec()) return records;

    while (query.next()) {
        records.append(rowToRecord(query));
    }
    return records;
}

// ============================================================
//  删除记录
// ============================================================
bool MysqlDatabase::deleteRecord(int id)
{
    if (!m_db.isOpen()) return false;

    QSqlQuery query(m_db);
    query.prepare("DELETE FROM car WHERE id = :id");
    query.bindValue(":id", id);

    if (!query.exec()) {
        m_lastError = query.lastError().text();
        return false;
    }
    return true;
}

// ============================================================
//  工具：行 → 结构体
// ============================================================
VehicleRecord MysqlDatabase::rowToRecord(const QSqlQuery &query) const
{
    VehicleRecord r;
    r.id          = query.value("id").toInt();
    r.plateNo     = query.value("plate_no").toString();
    r.vehicleType = query.value("vehicle_type").toString();
    r.color       = query.value("color").toString();
    r.brand       = query.value("brand").toString();
    r.model       = query.value("model").toString();
    r.recordTime  = query.value("record_time").toString();
    return r;
}
