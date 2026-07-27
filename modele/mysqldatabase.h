#ifndef MYSQLDATABASE_H
#define MYSQLDATABASE_H

#include <QString>
#include <QSqlDatabase>
#include <QList>

/// 车辆录入记录（对应 MySQL car 表）
struct VehicleRecord
{
    int     id          = 0;
    QString plateNo;       // 车牌号
    QString vehicleType;   // 车辆类型
    QString color;         // 车身颜色
    QString brand;         // 品牌
    QString model;         // 型号
    QString recordTime;    // 录入时间
};

/// MySQL 数据库管理 — 连接本地 car_check 数据库
class MysqlDatabase
{
public:
    MysqlDatabase();
    ~MysqlDatabase();

    /// 连接到本地 MySQL（无参时从 Config 读取连接参数）
    bool connect(const QString &host = QString(),
                 int port = 0,
                 const QString &dbName = QString(),
                 const QString &user = QString(),
                 const QString &password = QString());

    /// 断开连接
    void disconnect();

    /// 是否已连接
    bool isConnected() const;

    /// 插入一条车辆记录，返回新记录的 id（失败返回 -1）
    int insertRecord(const VehicleRecord &record);

    /// 查询所有车辆记录（按录入时间倒序）
    QList<VehicleRecord> queryAllRecords() const;

    /// 按车牌号模糊搜索
    QList<VehicleRecord> searchByPlate(const QString &keyword) const;

    /// 根据 id 删除记录
    bool deleteRecord(int id);

    /// 获取错误信息
    QString lastError() const { return m_lastError; }

private:
    VehicleRecord rowToRecord(const QSqlQuery &query) const;

    QSqlDatabase m_db;
    QString      m_lastError;
};

#endif // MYSQLDATABASE_H
