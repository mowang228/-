#ifndef CONFIG_H
#define CONFIG_H

#include <QSettings>
#include <QString>
#include <QSqlDatabase>

class Config
{
public:
    static Config& getInstance();

    // MySQL 配置读取
    QString getMysqlHost();
    int     getMysqlPort();
    QString getMysqlDbName();
    QString getMysqlUser();
    QString getMysqlPwd();

    QString getBaiduApiKey();
    QString getBaiduSecretKey();

    // 获取已连接的数据库句柄（自动重连）
    QSqlDatabase getDatabase();

private:
    Config();
    bool initDatabase();                     // 自动建库（如不存在）
    void createTables(QSqlDatabase &db);     // 自动建表
    bool probeCredentials();                 // 探测正确的账密
    bool tryConnect(const QString &dsn);     // 尝试一次 ODBC 连接

    void setMysqlUser(const QString &v);
    void setMysqlPwd(const QString &v);

    QSettings m_set;
    QString   m_connName;
};

#endif
