#ifndef GLOBALUTIL_H
#define GLOBALUTIL_H

#include <QString>
#include <QDateTime>
#include <QFile>
#include <QDebug>
#include <QMutex>

/// 系统工具类
///   - 日志输出（控制台 + 文件 debug.log）
///   - 通用工具函数
class GlobalUtil
{
public:
    /// 获取当前时间字符串  yyyy-MM-dd HH:mm:ss
    static QString getNowTime();

    /// 字符串空值判断
    static bool strIsEmpty(const QString &str);

    /// 文件是否存在
    static bool fileExists(const QString &filePath);

    /// 统一日志输出（写入控制台 + debug.log 日志文件）
    static void printLog(const QString &msg);

    /// 设置日志文件目录（默认 exe 所在目录）
    static void setLogDir(const QString &dir);

private:
    static QString m_logDir;
    static QMutex  m_logMutex;
};

#endif
