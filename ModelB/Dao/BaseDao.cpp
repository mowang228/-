#include "BaseDao.h"
#include "Config.h"
#include "GlobalUtil.h"
#include <QMutexLocker>

QMutex BaseDao::m_dbMutex;

/// 带锁安全执行 SQL
/// 自动从 Config 获取数据库连接
QSqlQuery BaseDao::safeExecute(const QString &sql, const QVariantList &args)
{
    QMutexLocker locker(&m_dbMutex);
    QSqlDatabase db = Config::getInstance().getDatabase();
    QSqlQuery query(db);
    query.prepare(sql);

    for (int i = 0; i < args.size(); i++)
    {
        query.bindValue(i, args[i]);
    }

    if (!query.exec())
    {
        GlobalUtil::printLog(
            QString("数据库查询失败：%1 SQL:%2")
            .arg(query.lastError().text(), sql));
    }
    return query;
}
