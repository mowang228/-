#ifndef DETECTOR_H
#define DETECTOR_H

#include <QString>
#include <QMap>
#include <QImage>
#include "BaiduVehicleApi.h"

/**
 * 车辆属性识别结果
 * 对应 Python 版 VehicleAttribute dataclass
 */
struct VehicleAttribute
{
    QString plateNo;       // 车牌号
    QString vehicleType;   // car / suv / bus / truck
    QString color;         // 车身颜色
    QString brand;         // 品牌
    QString model;         // 型号
    double  confidence;    // 置信度

    bool isValid() const { return !plateNo.isEmpty(); }
};

/**
 * 车辆属性识别器
 * 对应 Python 版 VehicleAttributeDetector
 *
 * 说明：
 * - 当前采用文件名规则 + 简单颜色统计生成预测结果
 * - 若有真实模型，只需替换 detect() 中的推理逻辑
 */
class VehicleAttributeDetector
{
public:
    VehicleAttributeDetector(const QMap<QString, QString> &modelConfig = {});

    void setUseBaiduApi(bool use);

    void setBaiduApiKey(const QString &apiKey, const QString &secretKey);

    VehicleAttribute detect(const QString &imagePath) const;

private:
    VehicleAttribute detectWithBaiduApi(const QString &imagePath) const;

    /** 仅使用本地模型检测（车牌+颜色+类型+品牌推测） */
    VehicleAttribute detectLocal(const QString &imagePath) const;

    QString inferPlateFromName(const QString &filename) const;

    QString inferTypeFromName(const QString &filename) const;

    void inferBrandModel(const QString &vehicleType,
                         QString &brand,
                         QString &model) const;

    QString estimateMainColor(const QString &imagePath) const;

    QMap<QString, QString> m_modelConfig;
    bool m_useBaiduApi = false;
    mutable BaiduVehicleApi m_baiduApi;
};

#endif // DETECTOR_H
