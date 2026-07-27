#ifndef DATABASE_H
#define DATABASE_H

#include <QString>
#include <QMap>
#include <QSqlDatabase>

/**
 * 车辆登记信息结构体
 */
struct VehicleInfo
{
    int     id;              // 数据库记录 ID（0 表示无效）
    QString plateNo;         // 车牌号
    QString vehicleType;     // 车辆类型：car / suv / bus / truck
    QString color;           // 车身颜色
    QString brand;           // 品牌
    QString model;           // 型号

    bool isValid() const { return id > 0 && !plateNo.isEmpty(); }
};

/**
 * 车辆登记信息数据库管理类
 * 连接本地 MySQL car_check 库，从 car 表查询登记信息
 */
class VehicleDatabase
{
public:
    VehicleDatabase();
    ~VehicleDatabase();

    /** 初始化 MySQL 连接 */
    bool init();

    /** 根据车牌号查询登记信息（自动标准化处理） */
    VehicleInfo queryByPlate(const QString &plateNo) const;

    /** 写入新车辆信息到 car 表 */
    bool insertCar(const QString &plateNo, const QString &vehicleType,
                   const QString &color, const QString &brand, const QString &model);

    /** 标准化车牌号：去空格、转大写 */
    static QString normalizePlate(const QString &plateNo);

private:
    bool connectMysql();

    QSqlDatabase m_db;
};

#endif // DATABASE_H
