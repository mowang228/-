#include "LLMService.h"

#include "../ModelD/LicensePlateDetector.h"
#include "../ModelD/OCRService.h"

#include <QCoreApplication>
#include <QFileInfo>
#include <QTimer>
#include <QDebug>
#include <QDir>
#include <QDateTime>
#include <QImage>

class LLMService::Impl
{
public:
    LicensePlateDetector detector;
    OCRService ocrService;
    QString modelPath;

    Impl()
    {
        modelPath = QCoreApplication::applicationDirPath() + "/best.pt";
        QFileInfo modelInfo(modelPath);
        if (!modelInfo.exists())
        {
            modelPath = QCoreApplication::applicationDirPath() + "/../best.pt";
            if (!QFileInfo::exists(modelPath))
            {
                modelPath = "best.pt";
            }
        }
    }
};

LLMService::LLMService(QObject *parent)
    : QObject(parent)
    , m_impl(new Impl)
{
    m_impl->detector.loadModel(m_impl->modelPath);
}

LLMService::~LLMService()
{
    delete m_impl;
}

void LLMService::analyze(const QString &imagePath)
{
    if (imagePath.isEmpty())
    {
        emit analysisFailed("图片路径为空");
        return;
    }

    if (!QFileInfo::exists(imagePath))
    {
        emit analysisFailed(QString("文件不存在: %1").arg(imagePath));
        return;
    }

    QTimer::singleShot(100, this, [this, imagePath]() {
        onAnalyzeInternal(imagePath);
    });
}

void LLMService::onAnalyzeInternal(const QString &imagePath)
{
    AnalysisResult result;
    result.plateNumber = "";
    result.vehicleBrand = "";
    result.conclusion = "未检测到车牌";

    qDebug() << "=== 开始车牌检测 ===";

    QList<DetectedPlate> plates = m_impl->detector.detect(imagePath);

    if (plates.isEmpty())
    {
        emit analysisCompleted(result);
        return;
    }

    qDebug() << "检测到" << plates.size() << "个车牌";

    QString outputDir = QCoreApplication::applicationDirPath() + "/output";
    QDir().mkpath(outputDir);

    QString resultImagePath = outputDir + "/detection_result_" +
        QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss") + ".png";

    m_impl->detector.drawDetection(imagePath, plates, resultImagePath);
    result.resultImagePath = resultImagePath;

    QImage srcImage(imagePath);
    if (srcImage.isNull())
    {
        emit analysisCompleted(result);
        return;
    }

    QStringList detectedPlateNumbers;

    for (const DetectedPlate &plate : plates)
    {
        QRect rect = plate.rect;

        if (rect.x() < 0) rect.setX(0);
        if (rect.y() < 0) rect.setY(0);
        if (rect.x() + rect.width() > srcImage.width()) rect.setWidth(srcImage.width() - rect.x());
        if (rect.y() + rect.height() > srcImage.height()) rect.setHeight(srcImage.height() - rect.y());

        QImage plateRegion = srcImage.copy(rect);

        QString plateImagePath = outputDir + "/plate_" +
            QString::number(detectedPlateNumbers.size()) + ".png";
        plateRegion.save(plateImagePath);

        QString plateNumber = m_impl->ocrService.recognizePlate(plateImagePath);

        if (!plateNumber.isEmpty())
        {
            detectedPlateNumbers.append(plateNumber);
            qDebug() << "识别车牌号:" << plateNumber;
        }
    }

    result.detectedPlates = detectedPlateNumbers;

    if (!detectedPlateNumbers.isEmpty())
    {
        result.plateNumber = detectedPlateNumbers.first();

        if (detectedPlateNumbers.size() > 1)
        {
            result.conclusion = "疑似套牌车（检测到多个车牌）";
        }
        else
        {
            result.conclusion = "车辆信息正常";
        }
    }

    qDebug() << "=== 分析完成 ===";
    qDebug() << "车牌号:" << result.plateNumber;
    qDebug() << "结论:" << result.conclusion;

    emit analysisCompleted(result);
}