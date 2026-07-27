#include "OCRService.h"
#include "PythonUtil.h"
#include <QCoreApplication>
#include <QFileInfo>
#include <QDebug>
#include <QDir>
#include <QDateTime>
#include <QProcess>
#include <QJsonDocument>
#include <QJsonObject>

OCRService::OCRService(QObject *parent)
    : QObject(parent)
{
}

QString OCRService::recognizePlate(const QString &imagePath)
{
    if (!QFileInfo::exists(imagePath))
    {
        qDebug() << "图片文件不存在:" << imagePath;
        return QString();
    }

    QString scriptPath = QCoreApplication::applicationDirPath() + "/plate_inference.py";
    QString modelPath = QCoreApplication::applicationDirPath() + "/plate_rec_color.onnx";

    if (!QFileInfo::exists(scriptPath))
    {
        qDebug() << "脚本文件不存在:" << scriptPath;
        return QString();
    }

    if (!QFileInfo::exists(modelPath))
    {
        qDebug() << "识别模型文件不存在:" << modelPath;
        return QString();
    }

    QString pythonCmd = findPython();
    if (pythonCmd.isEmpty())
    {
        qDebug() << "OCRService: 未安装 Python，无法进行车牌识别";
        return QString();
    }

    // 中文路径 → 临时 ASCII 路径（Windows Python 兼容）
    QString safePath = sanitizePathForPython(imagePath);

    QProcess process;
    process.setProgram(pythonCmd);
    QStringList args;
    args << scriptPath << "recognize" << modelPath << safePath;
    process.setArguments(args);

    qDebug() << "启动 Python 识别脚本:" << pythonCmd << args.join(" ");

    process.start();
    if (!process.waitForStarted())
    {
        qDebug() << "无法启动 Python 进程";
        return QString();
    }

    if (!process.waitForFinished(30000))
    {
        qDebug() << "Python 脚本执行超时";
        process.kill();
        return QString();
    }

    QByteArray rawOutput = process.readAllStandardOutput();
    QByteArray rawError = process.readAllStandardError();

    QString output = QString::fromUtf8(rawOutput);
    QString error = QString::fromUtf8(rawError);

    if (!error.isEmpty())
    {
        qDebug() << "Python 脚本错误输出:" << error;
    }

    if (output.isEmpty())
    {
        qDebug() << "Python 脚本无输出";
        return QString();
    }

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(rawOutput, &parseError);

    if (parseError.error != QJsonParseError::NoError)
    {
        qDebug() << "JSON 解析失败:" << parseError.errorString();
        return QString();
    }

    QJsonObject obj = doc.object();
    if (!obj.contains("success") || !obj["success"].toBool())
    {
        if (obj.contains("error"))
        {
            qDebug() << "识别失败:" << obj["error"].toString();
        }
        return QString();
    }

    QString plateText = obj["plate_text"].toString();
    qDebug() << "OCR 识别结果:" << plateText;
    return plateText;
}

QString OCRService::recognizePlate(const QVector<QString> &plateImages)
{
    QString fullResult;

    for (const QString &plateImage : plateImages)
    {
        QString result = recognizePlate(plateImage);
        if (!result.isEmpty())
        {
            if (!fullResult.isEmpty())
            {
                fullResult += "; ";
            }
            fullResult += result;
        }
    }

    return fullResult;
}