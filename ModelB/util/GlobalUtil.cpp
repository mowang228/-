#include "GlobalUtil.h"
#include <QCoreApplication>
#include <QTextStream>
#include <QMutexLocker>

QString GlobalUtil::m_logDir;
QMutex  GlobalUtil::m_logMutex;

// ------------------------------------------------------------
//  设置日志目录
// ------------------------------------------------------------
void GlobalUtil::setLogDir(const QString &dir)
{
    m_logDir = dir;
}

// ------------------------------------------------------------
//  获取当前时间
// ------------------------------------------------------------
QString GlobalUtil::getNowTime()
{
    return QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");
}

// ------------------------------------------------------------
//  字符串空值判断
// ------------------------------------------------------------
bool GlobalUtil::strIsEmpty(const QString &str)
{
    return str.trimmed().isEmpty();
}

// ------------------------------------------------------------
//  文件是否存在
// ------------------------------------------------------------
bool GlobalUtil::fileExists(const QString &filePath)
{
    QFile f(filePath);
    return f.exists();
}

// ------------------------------------------------------------
//  统一日志输出（控制台 + 文件 debug.log）
// ------------------------------------------------------------
void GlobalUtil::printLog(const QString &msg)
{
    QMutexLocker locker(&m_logMutex);
    QString timestamp = "[" + getNowTime() + "] ";

    // 控制台输出
    qDebug().noquote() << timestamp + msg;

    // 文件输出
    QString logDir = m_logDir.isEmpty()
                         ? QCoreApplication::applicationDirPath()
                         : m_logDir;

    QFile file(logDir + "/debug.log");
    if (file.open(QIODevice::Append | QIODevice::Text))
    {
        QTextStream out(&file);
        out << timestamp << msg << "\n";
        file.close();
    }
}
