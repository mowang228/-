#include "PythonUtil.h"
#include <QProcess>
#include <QDebug>
#include <QMutex>
#include <QMutexLocker>
#include <QFileInfo>
#include <QFile>
#include <QDir>
#include <QCoreApplication>
#include <QDateTime>

QString findPython()
{
    static QString cachedPython;
    static QMutex mutex;
    QMutexLocker locker(&mutex);

    if (!cachedPython.isEmpty())
        return cachedPython;

    // 依次尝试三个候选命令
    const QStringList candidates = {"python", "py", "python3"};
    for (const QString &cmd : candidates)
    {
        QProcess proc;
        proc.setProgram(cmd);
        proc.setArguments({"--version"});
        proc.start();

        if (proc.waitForStarted(3000) && proc.waitForFinished(3000))
        {
            QString output = QString::fromUtf8(proc.readAllStandardOutput()).trimmed();
            if (output.contains("Python", Qt::CaseInsensitive))
            {
                cachedPython = cmd;
                qDebug() << "PythonUtil: 找到 Python →" << cmd << "(" << output << ")";
                return cachedPython;
            }
        }
    }

    qDebug() << "PythonUtil: 未找到 Python，请安装 Python 并确保可从命令行调用";
    cachedPython.clear();
    return cachedPython;
}

// ============================================================
//  将含中文路径转为纯 ASCII 临时路径（Python 子进程兼容）
// ============================================================
QString sanitizePathForPython(const QString &originalPath)
{
    // 检查是否包含非 ASCII 字符
    bool hasNonAscii = false;
    for (const QChar &ch : originalPath) {
        if (ch.unicode() > 127) {
            hasNonAscii = true;
            break;
        }
    }

    if (!hasNonAscii)
        return originalPath;

    QFileInfo fi(originalPath);
    if (!fi.exists())
        return originalPath;

    // 在 exe 目录下创建临时目录
    QString tempDir = QCoreApplication::applicationDirPath() + "/.temp_ascii";
    QDir().mkpath(tempDir);

    // 用时间戳 + 原始扩展名生成纯 ASCII 文件名
    QString safeName = QString("img_%1%2")
                           .arg(QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss_zzz"))
                           .arg(fi.suffix().isEmpty() ? ".jpg" : "." + fi.suffix());
    QString targetPath = tempDir + "/" + safeName;

    // 拷贝文件
    if (QFileInfo::exists(targetPath))
        QFile::remove(targetPath);

    if (QFile::copy(originalPath, targetPath)) {
        qDebug() << "PythonUtil: 已拷贝含中文路径文件到临时路径" << targetPath;
        return targetPath;
    }

    // 拷贝失败则回退原路径
    qDebug() << "PythonUtil: 拷贝临时文件失败，使用原路径";
    return originalPath;
}
