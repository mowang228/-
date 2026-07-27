#ifndef BASEDAO_H
#define BASEDAO_H

#include <QSqlQuery>
#include <QSqlDatabase>
#include <QVariant>
#include <QMutex>
#include <QSqlError>

/// 数据库操作基类
/// 子类继承后实现 selectById / insert / deleteById / update 四个接口
class BaseDao
{
public:
    virtual QVariant selectById(qint64 id) = 0;
    virtual bool insert(const QVariant& model) = 0;
    virtual bool deleteById(qint64 id) = 0;
    virtual bool update(const QVariant& model) = 0;

public:
    /// 带锁安全执行 SQL（自动从 Config 获取数据库连接）
    QSqlQuery safeExecute(const QString& sql, const QVariantList& args = {});

private:
    static QMutex m_dbMutex;
};

#endif
