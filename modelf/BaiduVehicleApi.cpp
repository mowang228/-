#include "BaiduVehicleApi.h"
#include "../ModelB/util/GlobalUtil.h"
#include <algorithm>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QByteArray>
#include <QUrl>
#include <QUrlQuery>
#include <QDateTime>
#include <QEventLoop>
#include <QSslConfiguration>

BaiduVehicleApi::BaiduVehicleApi(QObject *parent)
    : QObject(parent)
{
    m_colorMap["白色"] = "白色";
    m_colorMap["黑色"] = "黑色";
    m_colorMap["银色"] = "灰色";
    m_colorMap["灰色"] = "灰色";
    m_colorMap["红色"] = "红色";
    m_colorMap["蓝色"] = "蓝色";
    m_colorMap["黄色"] = "黄色";
    m_colorMap["绿色"] = "绿色";
    m_colorMap["棕色"] = "棕色";
    m_colorMap["橙色"] = "橙色";

    m_typeMap["轿车"] = "轿车";
    m_typeMap["SUV"] = "SUV";
    m_typeMap["MPV"] = "MPV";
    m_typeMap["客车"] = "客车";
    m_typeMap["货车"] = "货车";
    m_typeMap["皮卡"] = "皮卡";
    m_typeMap["面包车"] = "面包车";
}

void BaiduVehicleApi::setApiKey(const QString &apiKey, const QString &secretKey)
{
    m_apiKey = apiKey;
    m_secretKey = secretKey;
    m_accessToken.clear();
    m_tokenExpireTime = QDateTime();
}

QString BaiduVehicleApi::getAccessToken()
{
    QDateTime now = QDateTime::currentDateTime();
    if (!m_accessToken.isEmpty() && now < m_tokenExpireTime)
    {
        return m_accessToken;
    }

    return requestToken();
}

QString BaiduVehicleApi::requestToken()
{
    if (m_apiKey.isEmpty() || m_secretKey.isEmpty())
    {
        GlobalUtil::printLog("BaiduVehicleApi: API Key 或 Secret Key 为空");
        return QString();
    }

    QNetworkAccessManager manager;
    QNetworkRequest request;

    QUrl url("https://aip.baidubce.com/oauth/2.0/token");
    QUrlQuery query;
    query.addQueryItem("grant_type", "client_credentials");
    query.addQueryItem("client_id", m_apiKey);
    query.addQueryItem("client_secret", m_secretKey);
    url.setQuery(query);

    request.setUrl(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QSslConfiguration sslConfig = QSslConfiguration::defaultConfiguration();
    sslConfig.setPeerVerifyMode(QSslSocket::VerifyNone);
    request.setSslConfiguration(sslConfig);

    QNetworkReply *reply = manager.get(request);
    QEventLoop loop;
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    BaiduVehicleResult result;

    if (reply->error() != QNetworkReply::NoError)
    {
        GlobalUtil::printLog("BaiduVehicleApi: 获取token失败: " + reply->errorString());
        reply->deleteLater();
        return QString();
    }

    QByteArray data = reply->readAll();
    reply->deleteLater();

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);
    if (parseError.error != QJsonParseError::NoError)
    {
        GlobalUtil::printLog("BaiduVehicleApi: Token响应解析失败");
        return QString();
    }

    QJsonObject obj = doc.object();
    if (obj.contains("error"))
    {
        GlobalUtil::printLog("BaiduVehicleApi: Token获取失败: " + obj["error_description"].toString());
        return QString();
    }

    m_accessToken = obj["access_token"].toString();
    int expiresIn = obj["expires_in"].toInt(3600);
    m_tokenExpireTime = QDateTime::currentDateTime().addSecs(expiresIn - 60);

    GlobalUtil::printLog("BaiduVehicleApi: Token获取成功，有效期至 " + m_tokenExpireTime.toString());

    return m_accessToken;
}

BaiduVehicleResult BaiduVehicleApi::recognize(const QString &imagePath)
{
    BaiduVehicleResult result;

    QFile file(imagePath);
    if (!file.open(QIODevice::ReadOnly))
    {
        result.success = false;
        result.errorMessage = "无法打开图片文件";
        return result;
    }

    QByteArray imageData = file.readAll();
    file.close();

    QString base64Image = QString::fromUtf8(imageData.toBase64());

    // [DEBUG] H2: 检查 token 获取
    qDebug() << "===== [DEBUG] recognize: 开始获取token ====="
             << "apiKey非空=" << !m_apiKey.isEmpty()
             << "secretKey非空=" << !m_secretKey.isEmpty()
             << "已有token=" << !m_accessToken.isEmpty();

    QString token = getAccessToken();
    qDebug() << "===== [DEBUG] recognize: token获取结果 ====="
             << "token为空=" << token.isEmpty()
             << "errorMessage=" << result.errorMessage;

    if (token.isEmpty())
    {
        result.success = false;
        result.errorMessage = "无法获取访问令牌";
        return result;
    }

    QNetworkAccessManager manager;
    QNetworkRequest request;

    // 先调用车牌识别API
    {
        QNetworkAccessManager ocrManager;
        QNetworkRequest ocrRequest;

        QUrl ocrUrl("https://aip.baidubce.com/rest/2.0/ocr/v1/license_plate");
        QUrlQuery ocrQuery;
        ocrQuery.addQueryItem("access_token", token);
        ocrUrl.setQuery(ocrQuery);

        ocrRequest.setUrl(ocrUrl);
        ocrRequest.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");

        QSslConfiguration ocrSslConfig = QSslConfiguration::defaultConfiguration();
        ocrSslConfig.setPeerVerifyMode(QSslSocket::VerifyNone);
        ocrRequest.setSslConfiguration(ocrSslConfig);

        QByteArray ocrPostData = "image=" + QUrl::toPercentEncoding(base64Image);

        QNetworkReply *ocrReply = ocrManager.post(ocrRequest, ocrPostData);
        QEventLoop ocrLoop;
        QObject::connect(ocrReply, &QNetworkReply::finished, &ocrLoop, &QEventLoop::quit);
        ocrLoop.exec();

        if (ocrReply->error() == QNetworkReply::NoError)
        {
            QByteArray ocrData = ocrReply->readAll();
            QJsonParseError ocrParseError;
            QJsonDocument ocrDoc = QJsonDocument::fromJson(ocrData, &ocrParseError);
            if (ocrParseError.error == QJsonParseError::NoError)
            {
                QJsonObject ocrObj = ocrDoc.object();
                if (!ocrObj.contains("error_code"))
                {
                    QJsonArray wordsResult = ocrObj["words_result"].toArray();
                    if (!wordsResult.isEmpty())
                    {
                        QJsonObject plateObj = wordsResult[0].toObject();
                        result.plateNo = plateObj["number"].toString();
                        GlobalUtil::printLog("BaiduVehicleApi: 车牌识别成功: " + result.plateNo);
                    }
                }
            }
        }
        ocrReply->deleteLater();
    }

    // 再调用车型识别API（获取品牌、型号、颜色）
    {
        QNetworkRequest carRequest;

        QUrl carUrl("https://aip.baidubce.com/rest/2.0/image-classify/v1/car");
        QUrlQuery carQuery;
        carQuery.addQueryItem("access_token", token);
        carUrl.setQuery(carQuery);

        carRequest.setUrl(carUrl);
        carRequest.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");

        QSslConfiguration carSslConfig = QSslConfiguration::defaultConfiguration();
        carSslConfig.setPeerVerifyMode(QSslSocket::VerifyNone);
        carRequest.setSslConfiguration(carSslConfig);

        QByteArray carPostData = "image=" + QUrl::toPercentEncoding(base64Image) + "&top_num=3";

        QNetworkReply *carReply = manager.post(carRequest, carPostData);
        QEventLoop carLoop;
        QObject::connect(carReply, &QNetworkReply::finished, &carLoop, &QEventLoop::quit);
        carLoop.exec();

        if (carReply->error() == QNetworkReply::NoError)
        {
            QByteArray carData = carReply->readAll();
            qDebug() << "BaiduVehicleApi: 车型识别API响应:" << carData.left(200);
            QJsonParseError carParseError;
            QJsonDocument carDoc = QJsonDocument::fromJson(carData, &carParseError);
            if (carParseError.error == QJsonParseError::NoError)
            {
                QJsonObject carObj = carDoc.object();
                if (!carObj.contains("error_code"))
                {
                    // 解析颜色
                    if (carObj.contains("color_result"))
                    {
                        QString color = carObj["color_result"].toString();
                        if (m_colorMap.contains(color))
                        {
                            result.color = m_colorMap[color];
                        }
                        else
                        {
                            result.color = color;
                        }
                    }

                    // 解析品牌和型号（取置信度最高的结果）
                    QJsonArray results = carObj["result"].toArray();
                    if (!results.isEmpty())
                    {
                        QJsonObject topResult = results[0].toObject();
                        QString fullName = topResult["name"].toString();  // 如"宝马X6"、"特斯拉Model3"

                        // 常见汽车品牌列表（按长度降序，优先匹配长品牌名）
                        static QStringList brandList = {
                            "阿斯顿马丁", "阿尔法罗密欧", "兰博基尼", "劳斯莱斯", "雷克萨斯",
                            "迈凯伦", "玛莎拉蒂", "凯迪拉克", "布加迪",
                            "特斯拉", "沃尔沃", "路特斯", "保时捷",
                            "路虎", "捷豹", "林肯", "雪佛兰",
                            "宾利", "法拉利", "MINI", "Smart",
                            "别克", "本田", "丰田", "日产", "现代",
                            "起亚", "标致", "雪铁龙", "雷诺", "菲亚特",
                            "宝马", "奔驰", "奥迪", "大众", "福特",
                            "马自达", "三菱", "斯巴鲁", "铃木", "吉利",
                            "比亚迪", "长城", "奇瑞", "长安", "五菱",
                            "江淮", "奔腾", "荣威", "名爵", "传祺",
                            "红旗", "蔚来", "小鹏", "理想", "哪吒",
                            "零跑", "威马", "极氪", "问界", "阿维塔",
                            "深蓝", "仰望", "方程豹"
                        };

                        result.brand = fullName;
                        result.model = "";
                        for (const QString &b : brandList)
                        {
                            if (fullName.startsWith(b))
                            {
                                result.brand = b;
                                result.model = fullName.mid(b.length());
                                break;
                            }
                        }

                        double score = topResult["score"].toDouble();
                        result.confidence = score;

                        // 从品牌+型号推断车辆类型（当 v2/vehicle_attr 接口不可用时）
                        if (result.vehicleType.isEmpty())
                        {
                            result.vehicleType = inferVehicleType(result.brand, result.model, fullName);
                        }

                        GlobalUtil::printLog("BaiduVehicleApi: 车型识别成功 - 品牌:" + result.brand +
                                             " 型号:" + result.model +
                                             " 颜色:" + result.color +
                                             " 置信度:" + QString::number(score));
                    }
                    else
                    {
                        GlobalUtil::printLog("BaiduVehicleApi: 车型API返回结果为空");
                    }
                }
                else
                {
                    GlobalUtil::printLog("BaiduVehicleApi: 车型识别API错误 " + QString::number(carObj["error_code"].toInt()) + ": " + carObj["error_msg"].toString());
                }
            }
        }
        else
        {
            GlobalUtil::printLog("BaiduVehicleApi: 车型识别网络请求失败: " + carReply->errorString());
        }
        carReply->deleteLater();
    }

    // 最后调用车辆属性识别API（获取车辆类型：轿车/SUV/货车等）
    {
        QNetworkRequest attrRequest;

        QUrl attrUrl("https://aip.baidubce.com/rest/2.0/image-classify/v2/vehicle_attr");
        QUrlQuery attrQuery;
        attrQuery.addQueryItem("access_token", token);
        attrUrl.setQuery(attrQuery);

        attrRequest.setUrl(attrUrl);
        attrRequest.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");

        QSslConfiguration attrSslConfig = QSslConfiguration::defaultConfiguration();
        attrSslConfig.setPeerVerifyMode(QSslSocket::VerifyNone);
        attrRequest.setSslConfiguration(attrSslConfig);

        QByteArray attrPostData = "image=" + QUrl::toPercentEncoding(base64Image);

        QNetworkReply *attrReply = manager.post(attrRequest, attrPostData);
        QEventLoop attrLoop;
        QObject::connect(attrReply, &QNetworkReply::finished, &attrLoop, &QEventLoop::quit);
        attrLoop.exec();

        if (attrReply->error() == QNetworkReply::NoError)
        {
            QByteArray attrData = attrReply->readAll();
            qDebug() << "BaiduVehicleApi: 车辆属性API响应:" << attrData.left(200);
            QJsonParseError attrParseError;
            QJsonDocument attrDoc = QJsonDocument::fromJson(attrData, &attrParseError);
            if (attrParseError.error == QJsonParseError::NoError)
            {
                QJsonObject attrObj = attrDoc.object();
                if (!attrObj.contains("error_code"))
                {
                    if (attrObj.contains("vehicle_type"))
                    {
                        QString type = attrObj["vehicle_type"].toString();
                        if (m_typeMap.contains(type))
                        {
                            result.vehicleType = m_typeMap[type];
                        }
                        else
                        {
                            result.vehicleType = type;
                        }
                        GlobalUtil::printLog("BaiduVehicleApi: 车辆属性识别 - 类型:" + result.vehicleType);
                    }

                    if (attrObj.contains("vehicle_color"))
                    {
                        QString color = attrObj["vehicle_color"].toString();
                        // 如果之前没有获取到颜色，用v2 API的颜色
                        if (result.color.isEmpty())
                        {
                            if (m_colorMap.contains(color))
                                result.color = m_colorMap[color];
                            else
                                result.color = color;
                        }
                    }

                    // 解析颜色子项（如果vehicle_color是对象而非字符串）
                    if (result.color.isEmpty() && attrObj.contains("color"))
                    {
                        QJsonObject colorObj = attrObj["color"].toObject();
                        if (!colorObj.isEmpty())
                        {
                            result.color = colorObj["name"].toString();
                        }
                    }
                }
                else
                {
                    GlobalUtil::printLog("BaiduVehicleApi: 车辆属性API错误 " + QString::number(attrObj["error_code"].toInt()) + ": " + attrObj["error_msg"].toString());
                }
            }
        }
        else
        {
            GlobalUtil::printLog("BaiduVehicleApi: 车辆属性网络请求失败: " + attrReply->errorString());
        }
        attrReply->deleteLater();
    }

    result.confidence = 0.95;
    result.success = !result.color.isEmpty() || !result.brand.isEmpty() || !result.plateNo.isEmpty();

    if (result.success)
    {
        GlobalUtil::printLog("BaiduVehicleApi: 识别完成 - 车牌:" + result.plateNo +
                             " 颜色:" + result.color +
                             " 类型:" + result.vehicleType +
                             " 品牌:" + result.brand);
    }
    else
    {
        GlobalUtil::printLog("BaiduVehicleApi: 识别失败，未获取到有效信息");
    }

    return result;
}

QString BaiduVehicleApi::inferVehicleType(const QString &brand, const QString &model, const QString &fullName) const
{
    Q_UNUSED(model);

    // === 品牌级推断（特定品牌几乎只生产某一类车型）===
    // 货车/卡车品牌
    static QStringList truckBrands = {
        "解放", "重汽", "陕汽", "福田", "东风", "红岩", "欧曼",
        "江淮帅铃", "五十铃", "江铃", "庆铃", "凯马"
    };
    if (std::any_of(truckBrands.begin(), truckBrands.end(),
        [&brand](const QString &tb) { return brand.contains(tb); }))
        return "货车";

    // 客车/公交品牌
    static QStringList busBrands = {
        "宇通", "金龙", "金旅", "中通", "安凯", "海格", "申沃", "比亚迪K"
    };
    if (std::any_of(busBrands.begin(), busBrands.end(),
        [&brand](const QString &bb) { return brand.contains(bb); }))
        return "客车";

    // === 关键词级推断 ===
    QString lowerFull = fullName.toLower();

    // SUV 关键词
    static QStringList suvKeywords = {
        "SUV", "越野", "X1", "X2", "X3", "X4", "X5", "X6", "X7",
        "GLA", "GLB", "GLC", "GLE", "GLS",
        "Q2", "Q3", "Q5", "Q7", "Q8",
        "途观", "途岳", "途昂", "途锐", "探岳", "探歌",
        "CR-V", "HR-V", "XR-V", "缤智", "皓影", "冠道", "UR-V",
        "RAV4", "威兰达", "汉兰达", "普拉多", "兰德酷路泽",
        "奇骏", "逍客", "楼兰", "途达",
        "CX-3", "CX-4", "CX-5", "CX-8", "CX-30",
        "欧蓝德", "劲炫",
        "H1", "H2", "H3", "H5", "H6", "H7", "H8", "H9",
        "VV5", "VV6", "VV7",
        "CS35", "CS55", "CS75", "CS95",
        "RX3", "RX5", "RX8", "RX9",
        "博越", "领克01", "领克02", "领克03", "领克05", "领克06", "领克09",
        "蔚来ES", "蔚来EC", "小鹏G", "理想L", "理想ONE",
        "Model Y", "Model X",
        "元", "宋", "唐",
        "哪吒U", "哪吒X", "零跑C11",
        "卫士", "发现", "发现神行", "揽胜", "揽运",
        "卡宴", "Macan",
        "牧马人", "大切诺基",
        "森林人", "傲虎",
        "CX70", "X70", "X90", "X95",
        "BJ40", "BJ80", "BJ90"
    };

    // MPV 关键词
    static QStringList mpvKeywords = {
        "MPV", "商务车", "GL8", "奥德赛", "赛那", "格瑞维亚",
        "埃尔法", "威尔法", "雷克萨斯LM",
        "传祺M8", "传祺M6", "腾势D9", "极氪009",
        "威然", "途安", "夏朗",
        "艾力绅", "五菱宏光", "五菱佳辰",
        "岚图梦想家", "大通G10", "大通G20", "大通G50",
        "菱智", "瑞风", "宋MAX"
    };

    // 皮卡关键词
    static QStringList pickupKeywords = {
        "皮卡", "福特F-150", "猛禽", "坦途", "Tacoma",
        "长城炮", "风骏", "金刚炮", "山海炮",
        "D-MAX", "铃拓", "瑞迈",
        "纳瓦拉", "锐骐",
        "大通T60", "大通T70", "大通T90"
    };

    // 跑车关键词
    static QStringList sportKeywords = {
        "911", "718", "Cayman", "Boxster", "Panamera",
        "法拉利", "兰博基尼", "迈凯伦", "阿斯顿马丁", "布加迪",
        "科尔维特", "Mustang", "野马",
        "MX-5", "Supra", "GT-R",
        "Model S", "Model 3", "Taycan",
        "奥迪R8", "宝马Z4", "奔驰SL", "奔驰AMG GT"
    };

    if (std::any_of(sportKeywords.begin(), sportKeywords.end(),
        [&lowerFull](const QString &kw) { return lowerFull.contains(kw.toLower()); }))
        return "跑车";

    if (std::any_of(pickupKeywords.begin(), pickupKeywords.end(),
        [&lowerFull](const QString &kw) { return lowerFull.contains(kw.toLower()); }))
        return "皮卡";

    if (std::any_of(mpvKeywords.begin(), mpvKeywords.end(),
        [&lowerFull](const QString &kw) { return lowerFull.contains(kw.toLower()); }))
        return "MPV";

    if (std::any_of(suvKeywords.begin(), suvKeywords.end(),
        [&lowerFull](const QString &kw) { return lowerFull.contains(kw.toLower()); }))
        return "SUV";

    // === 品牌级默认推断 ===
    static QStringList suvBrands = {
        "路虎", "Jeep", "哈弗", "WEY", "坦克"
    };
    if (std::any_of(suvBrands.begin(), suvBrands.end(),
        [&brand](const QString &sb) { return brand.contains(sb); }))
        return "SUV";

    // 默认返回轿车（最常见车型）
    return "轿车";
}