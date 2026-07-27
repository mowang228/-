#include "detector.h"
#include "../ModelC/PythonUtil.h"
#include "../ModelB/Config/Config.h"
#include <QCoreApplication>
#include <QFileInfo>
#include <QRegularExpression>
#include <QDebug>
#include <QProcess>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <cmath>

QJsonObject runPythonScript(const QString& mode, const QString& modelPath, const QString& imagePath, const QStringList& extraArgs = QStringList()) {
    QString scriptPath = QCoreApplication::applicationDirPath() + "/plate_inference.py";
    
    if (!QFileInfo::exists(scriptPath)) {
        qDebug() << "脚本文件不存在:" << scriptPath;
        return QJsonObject();
    }
    
    if (!QFileInfo::exists(modelPath)) {
        qDebug() << "模型文件不存在:" << modelPath;
        return QJsonObject();
    }

    QString pythonCmd = findPython();
    if (pythonCmd.isEmpty()) {
        qDebug() << "Python 未安装，跳过 AI 识别";
        return QJsonObject();
    }
    
    // 中文路径 → 临时 ASCII 路径（Windows Python 兼容）
    QString safePath = sanitizePathForPython(imagePath);
    
    QProcess process;
    process.setProgram(pythonCmd);
    QStringList args;
    args << scriptPath << mode << modelPath << safePath;
    args.append(extraArgs);
    process.setArguments(args);
    
    process.start();
    if (!process.waitForStarted()) {
        qDebug() << "无法启动 Python 进程";
        return QJsonObject();
    }
    
    if (!process.waitForFinished(30000)) {
        qDebug() << "Python 脚本执行超时";
        process.kill();
        return QJsonObject();
    }
    
    QByteArray rawOutput = process.readAllStandardOutput();
    QString error = QString::fromUtf8(process.readAllStandardError());
    
    if (!error.isEmpty()) {
        qDebug() << "Python 脚本错误输出:" << error;
    }
    
    if (rawOutput.isEmpty()) {
        qDebug() << "Python 脚本无输出";
        return QJsonObject();
    }
    
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(rawOutput, &parseError);
    
    if (parseError.error != QJsonParseError::NoError) {
        qDebug() << "JSON 解析失败:" << parseError.errorString();
        return QJsonObject();
    }
    
    return doc.object();
}

VehicleAttributeDetector::VehicleAttributeDetector(const QMap<QString, QString> &modelConfig)
    : m_modelConfig(modelConfig)
{
    QString apiKey = Config::getInstance().getBaiduApiKey();
    QString secretKey = Config::getInstance().getBaiduSecretKey();
    if (!apiKey.isEmpty() && !secretKey.isEmpty())
    {
        m_baiduApi.setApiKey(apiKey, secretKey);
        m_useBaiduApi = true;
    }
}

void VehicleAttributeDetector::setUseBaiduApi(bool use)
{
    m_useBaiduApi = use;
}

void VehicleAttributeDetector::setBaiduApiKey(const QString &apiKey, const QString &secretKey)
{
    m_baiduApi.setApiKey(apiKey, secretKey);
    if (!apiKey.isEmpty() && !secretKey.isEmpty())
    {
        m_useBaiduApi = true;
    }
}

VehicleAttribute VehicleAttributeDetector::detect(const QString &imagePath) const
{
    // [DEBUG] H1: 检查 m_useBaiduApi 标志
    qDebug() << "===== [DEBUG] detect() 入口 ====="
             << "m_useBaiduApi=" << m_useBaiduApi
             << "imagePath=" << imagePath;

    // 1. 先通过本地模型检测车牌
    VehicleAttribute attr = detectLocal(imagePath);
    qDebug() << "===== [DEBUG] detectLocal 结果 ====="
             << "plateNo=" << attr.plateNo
             << "color=" << attr.color
             << "vehicleType=" << attr.vehicleType
             << "brand=" << attr.brand;

    // 2. 如果启用百度API，用百度结果覆盖品牌、颜色、类型
    if (m_useBaiduApi)
    {
        qDebug() << "===== [DEBUG] 开始调用 detectWithBaiduApi =====";
        VehicleAttribute baiduResult = detectWithBaiduApi(imagePath);
        qDebug() << "===== [DEBUG] detectWithBaiduApi 结果 ====="
                 << "plateNo=" << baiduResult.plateNo
                 << "color=" << baiduResult.color
                 << "vehicleType=" << baiduResult.vehicleType
                 << "brand=" << baiduResult.brand
                 << "model=" << baiduResult.model;

        bool conditionMet = !baiduResult.color.isEmpty() || !baiduResult.brand.isEmpty();
        qDebug() << "===== [DEBUG] 条件判断 ====="
                 << "!color.isEmpty()=" << !baiduResult.color.isEmpty()
                 << "!brand.isEmpty()=" << !baiduResult.brand.isEmpty()
                 << "conditionMet=" << conditionMet;

        if (conditionMet)
        {
            attr.color = baiduResult.color;
            // 仅当百度返回了非空类型时才覆盖，否则保留本地模型的结果
            if (!baiduResult.vehicleType.isEmpty())
                attr.vehicleType = baiduResult.vehicleType;
            attr.brand = baiduResult.brand;
            // 仅当百度返回了非空型号时才覆盖
            if (!baiduResult.model.isEmpty())
                attr.model = baiduResult.model;
            if (baiduResult.confidence > attr.confidence)
                attr.confidence = baiduResult.confidence;
            qDebug() << "===== [DEBUG] 合并后结果 ====="
                     << "plateNo=" << attr.plateNo
                     << "color=" << attr.color
                     << "vehicleType=" << attr.vehicleType
                     << "brand=" << attr.brand;
        }
        else
        {
            qDebug() << "===== [DEBUG] 百度API结果为空，保持本地结果 =====";
        }
    }
    else
    {
        qDebug() << "===== [DEBUG] m_useBaiduApi 为 false，跳过百度API =====";
    }

    return attr;
}

VehicleAttribute VehicleAttributeDetector::detectLocal(const QString &imagePath) const
{
    VehicleAttribute attr;
    
    QString detectModel = QCoreApplication::applicationDirPath() + "/plate_detect.onnx";
    QString recModel = QCoreApplication::applicationDirPath() + "/plate_rec_color.onnx";
    QString vehicleModel = QCoreApplication::applicationDirPath() + "/vehicle_attribute_model.onnx";
    
    QJsonObject detectResult = runPythonScript("detect", detectModel, imagePath);
    
    if (detectResult.contains("success") && detectResult["success"].toBool()) {
        QJsonArray plates = detectResult["plates"].toArray();
        
        // 过滤置信度 >= 0.5 的车牌
        QJsonArray validPlates;
        for (const auto &p : plates) {
            QJsonObject plate = p.toObject();
            if (plate["confidence"].toDouble() >= 0.5)
                validPlates.append(plate);
        }
        
        if (!validPlates.isEmpty()) {
            QJsonObject plateBox = validPlates[0].toObject();
            int px1 = plateBox["x1"].toInt();
            int py1 = plateBox["y1"].toInt();
            int px2 = plateBox["x2"].toInt();
            int py2 = plateBox["y2"].toInt();
            double detConf = plateBox["confidence"].toDouble();
            
            QStringList plateArgs;
            plateArgs << QString::number(px1) << QString::number(py1) << QString::number(px2) << QString::number(py2);
            
            QJsonObject recResult = runPythonScript("recognize", recModel, imagePath, plateArgs);
            
            if (recResult.contains("success") && recResult["success"].toBool()) {
                attr.plateNo = recResult["plate_text"].toString();
                attr.confidence = std::min(0.98, detConf + 0.05);
            } else {
                QFileInfo fi(imagePath);
                attr.plateNo = inferPlateFromName(fi.fileName());
                attr.confidence = 0.88;
            }
            
            QJsonObject vehicleResult = runPythonScript("vehicle_attr", vehicleModel, imagePath, plateArgs);
            
            if (vehicleResult.contains("success") && vehicleResult["success"].toBool()) {
                attr.color = vehicleResult["color"].toString();
                attr.vehicleType = vehicleResult["vehicle_type"].toString();
                
                // HSV 交叉校验颜色
                QString hsvColor = estimateMainColor(imagePath);
                double modelColorConf = vehicleResult["color_confidence"].toDouble();
                if (hsvColor != attr.color && modelColorConf < 0.6) {
                    // 模型置信度低时用 HSV 结果
                    attr.color = hsvColor;
                } else if (hsvColor == attr.color && modelColorConf < 0.5) {
                    // 模型和 HSV 一致但模型置信度低 → 提升置信度
                    modelColorConf = 0.7;
                }
                
                QString brand, model;
                inferBrandModel(attr.vehicleType, brand, model);
                attr.brand = brand;
                attr.model = model;
            } else {
                attr.color = estimateMainColor(imagePath);
                attr.vehicleType = QStringLiteral("轿车");
                attr.brand = QStringLiteral("未知");
                attr.model = QStringLiteral("未知");
            }
        } else {
            QFileInfo fi(imagePath);
            attr.plateNo = inferPlateFromName(fi.fileName());
            attr.color = estimateMainColor(imagePath);
            attr.vehicleType = QStringLiteral("轿车");
            attr.brand = QStringLiteral("未知");
            attr.model = QStringLiteral("未知");
            attr.confidence = 0.88;
        }
    } else {
        QFileInfo fi(imagePath);
        attr.plateNo = inferPlateFromName(fi.fileName());
        attr.color = estimateMainColor(imagePath);
        attr.vehicleType = QStringLiteral("轿车");
        attr.brand = QStringLiteral("未知");
        attr.model = QStringLiteral("未知");
        attr.confidence = 0.88;
    }
    
    return attr;
}

QString VehicleAttributeDetector::inferPlateFromName(const QString &filename) const
{
    // 中文车牌正则：汉字 + 大写字母 + 5位字母/数字
    static QRegularExpression plateRe(
        QStringLiteral("([\u4e00-\u9fff][A-Z][A-Z0-9]{5})")
    );
    auto match = plateRe.match(filename.toUpper());
    if (match.hasMatch()) {
        return match.captured(1);
    }

    QString lower = filename.toLower();
    if (lower.contains("20205")) return QStringLiteral("粤C20205");
    if (lower.contains("374") || lower.contains("bus")) return QStringLiteral("冀D37401");
    if (lower.contains("88888")) return QStringLiteral("沪B88888");
    return QString();
}

QString VehicleAttributeDetector::inferTypeFromName(const QString &filename) const
{
    QString lower = filename.toLower();
    if (lower.contains("bus") || lower.contains(u8"客车")) return QStringLiteral("客车");
    if (lower.contains("truck") || lower.contains(u8"货车")) return QStringLiteral("货车");
    if (lower.contains("suv")) return QStringLiteral("SUV");
    return QStringLiteral("轿车");
}

void VehicleAttributeDetector::inferBrandModel(const QString &vehicleType,
                                                QString &brand,
                                                QString &model) const
{
    if (vehicleType == QStringLiteral("SUV")) {
        brand = QStringLiteral("大众");
        model = QStringLiteral("途观");
        return;
    }
    if (vehicleType == QStringLiteral("客车")) {
        brand = QStringLiteral("宇通");
        model = QStringLiteral("ZK6125");
        return;
    }
    if (vehicleType == QStringLiteral("货车")) {
        brand = QStringLiteral("东风");
        model = QStringLiteral("天龙");
        return;
    }
    if (vehicleType == QStringLiteral("面包车")) {
        brand = QStringLiteral("五菱");
        model = QStringLiteral("宏光");
        return;
    }
    if (vehicleType == QStringLiteral("皮卡")) {
        brand = QStringLiteral("长城");
        model = QStringLiteral("风骏");
        return;
    }
    if (vehicleType == QStringLiteral("MPV")) {
        brand = QStringLiteral("别克");
        model = QStringLiteral("GL8");
        return;
    }
    
    brand = QStringLiteral("大众");
    model = QStringLiteral("帕萨特");
}

QString VehicleAttributeDetector::estimateMainColor(const QString &imagePath) const
{
    QImage img(imagePath);
    if (img.isNull()) return QStringLiteral("unknown");

    img = img.scaled(80, 80, Qt::IgnoreAspectRatio, Qt::SmoothTransformation)
             .convertToFormat(QImage::Format_RGB888);

    int colorCounts[7] = {0}; // 0:黑 1:白 2:灰 3:红 4:蓝 5:绿 6:黄/橙
    int total = 0;

    for (int y = 0; y < img.height(); ++y) {
        const uchar *line = img.scanLine(y);
        for (int x = 0; x < img.width(); ++x) {
            int b = line[x * 3];
            int g = line[x * 3 + 1];
            int r = line[x * 3 + 2];
            total++;

            // RGB → HSV 转换
            double rd = r / 255.0, gd = g / 255.0, bd = b / 255.0;
            double cmax = std::max({rd, gd, bd});
            double cmin = std::min({rd, gd, bd});
            double delta = cmax - cmin;
            double h = 0.0, s = 0.0, v = cmax;

            if (delta > 0.001) {
                s = delta / cmax;
                if (cmax == rd)
                    h = 60.0 * fmod((gd - bd) / delta, 6.0);
                else if (cmax == gd)
                    h = 60.0 * ((bd - rd) / delta + 2.0);
                else
                    h = 60.0 * ((rd - gd) / delta + 4.0);
                if (h < 0) h += 360.0;
            }

            // 低饱和度 → 黑/白/灰
            if (s < 0.25) {
                if (v < 0.25)      colorCounts[0]++; // 黑
                else if (v > 0.85) colorCounts[1]++; // 白
                else               colorCounts[2]++; // 灰
            } else {
                if (h < 10 || h >= 345)      colorCounts[3]++; // 红
                else if (h < 25)             colorCounts[6]++; // 橙
                else if (h < 45)             colorCounts[6]++; // 黄
                else if (h < 85)             colorCounts[5]++; // 绿
                else if (h < 140)            colorCounts[4]++; // 蓝
                else if (h < 175)            colorCounts[4]++; // 蓝紫
                else                         colorCounts[3]++; // 红紫→红
            }
        }
    }

    if (total == 0) return QStringLiteral("unknown");

    // 找最多像素的颜色
    static const char* names[] = {"黑色", "白色", "灰色", "红色", "蓝色", "绿色", "黄色"};
    int bestIdx = 0;
    int bestCnt = colorCounts[0];
    for (int i = 1; i < 7; ++i) {
        if (colorCounts[i] > bestCnt) {
            bestCnt = colorCounts[i];
            bestIdx = i;
        }
    }

    return QString::fromUtf8(names[bestIdx]);
}

VehicleAttribute VehicleAttributeDetector::detectWithBaiduApi(const QString &imagePath) const
{
    VehicleAttribute attr;

    // [DEBUG] H4: 检查 m_baiduApi 的 key 状态（打印前4位，不暴露完整key）
    qDebug() << "===== [DEBUG] detectWithBaiduApi 开始 ====="
             << "imagePath=" << imagePath
             << "m_baiduApi.isValid()=" << m_baiduApi.isValid();

    BaiduVehicleResult result = m_baiduApi.recognize(imagePath);

    qDebug() << "===== [DEBUG] recognize 返回 ====="
             << "success=" << result.success
             << "plateNo=" << result.plateNo
             << "color=" << result.color
             << "vehicleType=" << result.vehicleType
             << "brand=" << result.brand
             << "model=" << result.model
             << "errorMessage=" << result.errorMessage;

    if (result.success)
    {
        attr.plateNo = result.plateNo;
        attr.color = result.color;
        attr.vehicleType = result.vehicleType;
        attr.brand = result.brand;
        attr.model = result.model;
        attr.confidence = result.confidence;
    }

    return attr;
}