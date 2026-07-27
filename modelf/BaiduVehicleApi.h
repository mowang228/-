#ifndef BAIDUVEHICLEAPI_H
#define BAIDUVEHICLEAPI_H

#include <QObject>
#include <QString>
#include <QMap>
#include <QDateTime>

struct BaiduVehicleResult
{
    QString plateNo;
    QString vehicleType;
    QString color;
    QString brand;
    QString model;
    double confidence;
    bool success;
    QString errorMessage;

    BaiduVehicleResult() : confidence(0.0), success(false) {}
};

class BaiduVehicleApi : public QObject
{
    Q_OBJECT

public:
    explicit BaiduVehicleApi(QObject *parent = nullptr);

    void setApiKey(const QString &apiKey, const QString &secretKey);

    bool isValid() const { return !m_apiKey.isEmpty() && !m_secretKey.isEmpty(); }

    BaiduVehicleResult recognize(const QString &imagePath);

private:
    QString getAccessToken();

    QString requestToken();

    /** 根据品牌+型号名称推断车辆类型（轿车/SUV/MPV等） */
    QString inferVehicleType(const QString &brand, const QString &model, const QString &fullName) const;

    QString m_apiKey;
    QString m_secretKey;
    QString m_accessToken;
    QDateTime m_tokenExpireTime;
    QMap<QString, QString> m_colorMap;
    QMap<QString, QString> m_typeMap;
};

#endif // BAIDUVEHICLEAPI_H