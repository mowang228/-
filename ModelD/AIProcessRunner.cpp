#include "AIProcessRunner.h"
#include "PythonUtil.h"
#include <QFileInfo>
#include <QDebug>
#include <QProcess>
#include <QJsonDocument>
#include <QJsonParseError>

AIProcessRunner::AIProcessRunner(QObject *parent)
    : QObject(parent)
{
}

bool AIProcessRunner::checkFileExists(const QString &filePath, const QString &description)
{
    if (!QFileInfo::exists(filePath))
    {
        qDebug() << description << "不存在:" << filePath;
        return false;
    }
    return true;
}

QJsonObject AIProcessRunner::runPythonScript(const QString &scriptPath, const QStringList &args)
{
    if (!checkFileExists(scriptPath, "脚本文件"))
    {
        return QJsonObject({{"success", false}, {"error", "脚本文件不存在"}});
    }

    QString pythonCmd = findPython();
    if (pythonCmd.isEmpty())
    {
        qDebug() << "AIProcessRunner: 未安装 Python，无法运行 AI 脚本";
        return QJsonObject({{"success", false}, {"error", "未安装 Python"}});
    }

    QProcess process;
    process.setProgram(pythonCmd);
    QStringList fullArgs = {scriptPath};
    fullArgs.append(args);
    process.setArguments(fullArgs);

    qDebug() << "启动 Python 脚本:" << process.program() << fullArgs.join(" ");

    process.start();
    if (!process.waitForStarted())
    {
        qDebug() << "无法启动 Python 进程";
        return QJsonObject({{"success", false}, {"error", "无法启动 Python 进程"}});
    }

    if (!process.waitForFinished(30000))
    {
        qDebug() << "Python 脚本执行超时";
        process.kill();
        return QJsonObject({{"success", false}, {"error", "Python 脚本执行超时"}});
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
        return QJsonObject({{"success", false}, {"error", "Python 脚本无输出"}});
    }

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(output.toUtf8(), &parseError);

    if (parseError.error != QJsonParseError::NoError)
    {
        qDebug() << "JSON 解析失败:" << parseError.errorString();
        return QJsonObject({{"success", false}, {"error", "JSON 解析失败: " + parseError.errorString()}});
    }

    return doc.object();
}