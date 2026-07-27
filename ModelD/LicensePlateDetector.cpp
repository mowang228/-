#include "LicensePlateDetector.h"
#include "PythonUtil.h"
#include <QFileInfo>
#include <QDebug>
#include <QImage>
#include <QPainter>
#include <QDir>
#include <QDateTime>
#include <QProcess>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QCoreApplication>

LicensePlateDetector::LicensePlateDetector(QObject *parent)
    : QObject(parent)
{
}

LicensePlateDetector::~LicensePlateDetector()
{
}

bool LicensePlateDetector::loadModel(const QString &modelPath)
{
    m_modelLoaded = true;
    m_modelPath = modelPath;
    qDebug() << "LicensePlateDetector initialized with model:" << modelPath;
    return true;
}

QList<DetectedPlate> LicensePlateDetector::detect(const QString &imagePath)
{
    QList<DetectedPlate> results;

    if (!QFileInfo::exists(imagePath))
    {
        qDebug() << "图片文件不存在:" << imagePath;
        return results;
    }

    QString scriptPath = QCoreApplication::applicationDirPath() + "/plate_inference.py";
    QString modelPath = QCoreApplication::applicationDirPath() + "/plate_detect.onnx";

    if (!QFileInfo::exists(scriptPath))
    {
        qDebug() << "脚本文件不存在:" << scriptPath;
        return results;
    }

    if (!QFileInfo::exists(modelPath))
    {
        qDebug() << "检测模型文件不存在:" << modelPath;
        return results;
    }

    QString pythonCmd = findPython();
    if (pythonCmd.isEmpty())
    {
        qDebug() << "LicensePlateDetector: 未安装 Python，无法进行车牌检测";
        return results;
    }

    // 中文路径 → 临时 ASCII 路径（Windows Python 兼容）
    QString safePath = sanitizePathForPython(imagePath);

    QProcess process;
    process.setProgram(pythonCmd);
    QStringList args;
    args << scriptPath << "detect" << modelPath << safePath;
    process.setArguments(args);

    qDebug() << "启动 Python 检测脚本:" << pythonCmd << args.join(" ");

    process.start();
    if (!process.waitForStarted())
    {
        qDebug() << "无法启动 Python 进程";
        return results;
    }

    if (!process.waitForFinished(30000))
    {
        qDebug() << "Python 脚本执行超时";
        process.kill();
        return results;
    }

    QString output = process.readAllStandardOutput();
    QString error = process.readAllStandardError();

    if (!error.isEmpty())
    {
        qDebug() << "Python 脚本错误输出:" << error;
    }

    if (output.isEmpty())
    {
        qDebug() << "Python 脚本无输出";
        return results;
    }

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(output.toUtf8(), &parseError);

    if (parseError.error != QJsonParseError::NoError)
    {
        qDebug() << "JSON 解析失败:" << parseError.errorString();
        return results;
    }

    QJsonObject obj = doc.object();
    if (!obj.contains("success") || !obj["success"].toBool())
    {
        if (obj.contains("error"))
        {
            qDebug() << "检测失败:" << obj["error"].toString();
        }
        return results;
    }

    QJsonArray platesArray = obj["plates"].toArray();
    for (const QJsonValue &val : platesArray)
    {
        QJsonObject plateObj = val.toObject();
        DetectedPlate plate;
        plate.rect = QRect(
            plateObj["x1"].toInt(),
            plateObj["y1"].toInt(),
            plateObj["x2"].toInt() - plateObj["x1"].toInt(),
            plateObj["y2"].toInt() - plateObj["y1"].toInt()
        );
        plate.confidence = plateObj["confidence"].toDouble();
        plate.label = "license_plate";
        results.append(plate);
    }

    qDebug() << "检测到" << results.size() << "个车牌区域";
    return results;
}

QString LicensePlateDetector::drawDetection(const QString &imagePath, const QList<DetectedPlate> &plates, const QString &outputPath)
{
    QImage image(imagePath);
    if (image.isNull())
    {
        qDebug() << "无法读取图片:" << imagePath;
        return QString();
    }

    QPainter painter(&image);
    QPen pen(Qt::green, 2);
    painter.setPen(pen);

    QFont font("Arial", 10);
    painter.setFont(font);

    for (const DetectedPlate &plate : plates)
    {
        painter.drawRect(plate.rect);
        QString label = QString("车牌 %1").arg(plate.confidence, 0, 'f', 2);
        painter.drawText(plate.rect.x(), plate.rect.y() - 5, label);
    }

    QDir().mkpath(QFileInfo(outputPath).absolutePath());
    bool saved = image.save(outputPath);

    if (saved)
    {
        qDebug() << "检测结果已保存:" << outputPath;
        return outputPath;
    }
    else
    {
        qDebug() << "保存检测结果失败:" << outputPath;
        return QString();
    }
}