#include "Config.h"
#include "GlobalUtil.h"
#include <QCoreApplication>
#include <QFile>
#include <QFileInfo>
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>

// ============================================================
//  从 exe 目录的 db_credentials.txt 读取自定义账密
//  文件格式（每行一组）：
//    username,password
//    root,my_password
// ============================================================
static QList<QPair<QString,QString>> readCustomCredentials()
{
    QList<QPair<QString,QString>> list;
    QString path = QCoreApplication::applicationDirPath() + "/db_credentials.txt";
    if (!QFileInfo::exists(path))
    {
        // 不存在则自动生成模板
        QFile tmpl(path);
        if (tmpl.open(QIODevice::WriteOnly | QIODevice::Text))
        {
            tmpl.write("# 数据库账密配置文件\n");
            tmpl.write("# 每行一组 用户名,密码（逗号分隔）\n");
            tmpl.write("# 删除行首 # 即可启用\n");
            tmpl.write("#root,your_password_here\n");
            tmpl.close();
        }
        return list;
    }

    QFile f(path);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
        return list;

    while (!f.atEnd())
    {
        QString line = QString::fromUtf8(f.readLine()).trimmed();
        if (line.isEmpty() || line.startsWith('#'))
            continue;
        int comma = line.indexOf(',');
        if (comma < 1) continue;
        QString user = line.left(comma).trimmed();
        QString pwd  = line.mid(comma + 1).trimmed();
        if (!user.isEmpty())
            list.append({user, pwd});
    }
    return list;
}

// 固定的候选列表（作为 fallback）
static QList<QPair<QString,QString>> defaultCredentials()
{
    return {
        {"root",   "123456"},
        {"root",   "11"},
        {"root",   "2007228l"},
        {"root",   "SqlSecretland4080@"},
        {"root",   "@Zjl13564349900"},
        {"mownag", "123456"},
        {"mownag", "11"},
        {"mownag", "2007228l"},
        {"mownag", "SqlSecretland4080@"},
        {"mownag", "@Zjl13564349900"},
    };
}

Config::Config()
    : m_set(QCoreApplication::applicationDirPath() + "/config.ini", QSettings::IniFormat)
    , m_connName("car_check_conn")
{
    // 首次运行生成默认配置（仅 host/port/dbname，账密由 probe 填充）
    if (!QFile::exists(m_set.fileName()))
    {
        m_set.setValue("MYSQL/host", "127.0.0.1");
        m_set.setValue("MYSQL/port", 3306);
        m_set.setValue("MYSQL/dbname", "car_check");
        m_set.sync();
    }

    if (!m_set.contains("BAIDU/api_key"))
    {
        m_set.setValue("BAIDU/api_key", "KTbGsDrH7BRBYLDTrwoqYgtT");
        m_set.setValue("BAIDU/secret_key", "c7vJDlcWsIDyzuIO9TQd8ey6gWF1LPJG");
        m_set.sync();
    }

    // 探测正确的账密（优先级：外部文件 > config.ini > 内置候选列表）
    if (probeCredentials())
        initDatabase();
    else
        GlobalUtil::printLog("Config: 所有账密组合均无法连接 MySQL 服务器");
}

Config& Config::getInstance()
{
    static Config obj;
    return obj;
}

// ------------------------------------------------------------
//  探测正确的 MySQL 账密
//  优先级：1. 外部 db_credentials.txt  2. config.ini 已有配置  3. 内置候选列表
// ------------------------------------------------------------
bool Config::probeCredentials()
{
    // ---- 1. 外部文件账密（用户可编辑） ----
    QList<QPair<QString,QString>> external = readCustomCredentials();
    for (const auto &cred : external)
    {
        QString dsn = QString(
            "DRIVER={MySQL ODBC 9.7 Unicode Driver};"
            "SERVER=%1;PORT=%2;"
            "UID=%3;PWD=%4;"
            "CHARSET=utf8mb4;"
        ).arg(getMysqlHost()).arg(getMysqlPort())
         .arg(cred.first).arg(cred.second);

        if (tryConnect(dsn))
        {
            GlobalUtil::printLog("Config: 外部文件账密连接成功 " + cred.first);
            setMysqlUser(cred.first);
            setMysqlPwd(cred.second);
            m_set.sync();
            return true;
        }
    }

    // ---- 2. config.ini 已有配置 ----
    QString existingUser = m_set.value("MYSQL/user").toString();
    QString existingPwd  = m_set.value("MYSQL/password").toString();
    if (!existingUser.isEmpty())
    {
        QString dsn = QString(
            "DRIVER={MySQL ODBC 9.7 Unicode Driver};"
            "SERVER=%1;PORT=%2;"
            "UID=%3;PWD=%4;"
            "CHARSET=utf8mb4;"
        ).arg(getMysqlHost()).arg(getMysqlPort())
         .arg(existingUser).arg(existingPwd);

        if (tryConnect(dsn))
        {
            GlobalUtil::printLog("Config: 使用已有配置连接成功 " + existingUser);
            return true;
        }
        GlobalUtil::printLog("Config: 已有配置连接失败，开始探测可用账密...");
    }

    // ---- 3. 内置候选列表（fallback） ----
    for (const auto &cred : defaultCredentials())
    {
        QString dsn = QString(
            "DRIVER={MySQL ODBC 9.7 Unicode Driver};"
            "SERVER=%1;PORT=%2;"
            "UID=%3;PWD=%4;"
            "CHARSET=utf8mb4;"
        ).arg(getMysqlHost()).arg(getMysqlPort())
         .arg(cred.first).arg(cred.second);

        if (tryConnect(dsn))
        {
            GlobalUtil::printLog("Config: 探测到可用账密 " + cred.first);
            setMysqlUser(cred.first);
            setMysqlPwd(cred.second);
            m_set.sync();
            return true;
        }
    }

    return false;
}

// ------------------------------------------------------------
//  尝试一次 ODBC 连接（不指定数据库）
// ------------------------------------------------------------
bool Config::tryConnect(const QString &dsn)
{
    const QString connName = m_connName + "_probe";
    // 清理旧连接
    if (QSqlDatabase::contains(connName))
    {
        QSqlDatabase::database(connName).close();
        QSqlDatabase::removeDatabase(connName);
    }

    bool ok = false;
    {
        QSqlDatabase db = QSqlDatabase::addDatabase("QODBC", connName);
        db.setDatabaseName(dsn);
        ok = db.open();
        if (ok) db.close();
    }
    QSqlDatabase::removeDatabase(connName);
    return ok;
}

// ------------------------------------------------------------
//  自动初始化 MySQL 库
//  1. 无库连接 → CREATE DATABASE IF NOT EXISTS
//  2. 连接目标库
// ------------------------------------------------------------
bool Config::initDatabase()
{
    // ---- 第一步：确保 car_check 库存在 ----
    {
        const QString initConn = m_connName + "_init";
        {
            QSqlDatabase tmpDb;
            if (QSqlDatabase::contains(initConn))
                tmpDb = QSqlDatabase::database(initConn);
            else
                tmpDb = QSqlDatabase::addDatabase("QODBC", initConn);

            QString dsn = QString(
                "DRIVER={MySQL ODBC 9.7 Unicode Driver};"
                "SERVER=%1;PORT=%2;"
                "UID=%3;PWD=%4;"
                "CHARSET=utf8mb4;"
            ).arg(getMysqlHost()).arg(getMysqlPort())
             .arg(getMysqlUser()).arg(getMysqlPwd());

            tmpDb.setDatabaseName(dsn);

            if (tmpDb.open())
            {
                QSqlQuery q(tmpDb);
                q.exec(QString("CREATE DATABASE IF NOT EXISTS `%1` "
                               "DEFAULT CHARACTER SET utf8mb4")
                       .arg(getMysqlDbName()));
                tmpDb.close();
            }
            else
            {
                GlobalUtil::printLog("Config: 无法连接 MySQL 服务器：" +
                                     tmpDb.lastError().text());
            }
        }
        QSqlDatabase::removeDatabase(initConn);
    }

    // ---- 第二步：连接 car_check 库 ----
    {
        if (QSqlDatabase::contains(m_connName))
            QSqlDatabase::removeDatabase(m_connName);

        QSqlDatabase db = QSqlDatabase::addDatabase("QODBC", m_connName);
        QString dsn = QString(
            "DRIVER={MySQL ODBC 9.7 Unicode Driver};"
            "SERVER=%1;PORT=%2;DATABASE=%3;"
            "UID=%4;PWD=%5;"
            "CHARSET=utf8mb4;"
        ).arg(getMysqlHost()).arg(getMysqlPort())
         .arg(getMysqlDbName())
         .arg(getMysqlUser()).arg(getMysqlPwd());

        db.setDatabaseName(dsn);

        if (!db.open())
        {
            GlobalUtil::printLog("Config: 数据库连接失败：" +
                                 db.lastError().text());
            return false;
        }

        // ---- 第三步：自动创建必要表单 ----
        createTables(db);

        GlobalUtil::printLog("Config: 已连接数据库 " + getMysqlDbName());
        return true;
    }
}

// ------------------------------------------------------------
//  获取连接（自动重连）
// ------------------------------------------------------------
QSqlDatabase Config::getDatabase()
{
    if (QSqlDatabase::contains(m_connName))
    {
        QSqlDatabase db = QSqlDatabase::database(m_connName);
        if (db.isOpen())
            return db;
        // 断线重连
        db.open();
        return db;
    }

    initDatabase();
    return QSqlDatabase::database(m_connName);
}

// ------------------------------------------------------------
//  自动创建 car / people 表
// ------------------------------------------------------------
void Config::createTables(QSqlDatabase &db)
{
    QSqlQuery q(db);

    // 车辆表 car
    q.exec(QString(
        "CREATE TABLE IF NOT EXISTS `car` ("
        "  `id` INT AUTO_INCREMENT PRIMARY KEY,"
        "  `plate_no` VARCHAR(20) NOT NULL,"
        "  `vehicle_type` VARCHAR(20),"
        "  `color` VARCHAR(20),"
        "  `brand` VARCHAR(50),"
        "  `model` VARCHAR(50),"
        "  `record_time` DATETIME DEFAULT CURRENT_TIMESTAMP,"
        "  INDEX `idx_plate` (`plate_no`)"
        ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4"
    ));
    if (q.lastError().isValid())
        GlobalUtil::printLog("Config: 建表 car 失败 " + q.lastError().text());

    // 用户表 people
    q.exec(QString(
        "CREATE TABLE IF NOT EXISTS `people` ("
        "  `id` INT AUTO_INCREMENT PRIMARY KEY,"
        "  `username` VARCHAR(50) NOT NULL UNIQUE,"
        "  `password` VARCHAR(255) NOT NULL,"
        "  `invite_code` VARCHAR(50),"
        "  `is_special` INT DEFAULT 0"
        ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4"
    ));
    if (q.lastError().isValid())
        GlobalUtil::printLog("Config: 建表 people 失败 " + q.lastError().text());
}

// ------------------------------------------------------------
//  MySQL 配置读取 / 写入
// ------------------------------------------------------------
QString Config::getMysqlHost()
{
    return m_set.value("MYSQL/host").toString();
}

int Config::getMysqlPort()
{
    return m_set.value("MYSQL/port").toInt();
}

QString Config::getMysqlDbName()
{
    return m_set.value("MYSQL/dbname").toString();
}

QString Config::getMysqlUser()
{
    return m_set.value("MYSQL/user").toString();
}

QString Config::getMysqlPwd()
{
    return m_set.value("MYSQL/password").toString();
}

void Config::setMysqlUser(const QString &v)
{
    m_set.setValue("MYSQL/user", v);
}

void Config::setMysqlPwd(const QString &v)
{
    m_set.setValue("MYSQL/password", v);
}

QString Config::getBaiduApiKey()
{
    return m_set.value("BAIDU/api_key").toString();
}

QString Config::getBaiduSecretKey()
{
    return m_set.value("BAIDU/secret_key").toString();
}
