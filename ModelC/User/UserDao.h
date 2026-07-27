#ifndef USERDAO_H
#define USERDAO_H

#include "BaseDao.h"
#include <QVariantMap>

class UserDao : public BaseDao
{
public:
    static UserDao& getInstance();

    // 实现父类纯虚接口
    QVariant selectById(qint64 id) override;
    bool insert(const QVariant& model) override;
    bool deleteById(qint64 id) override;
    bool update(const QVariant& model) override;

    // 登录专用查询：根据用户名密码查找
    bool checkUserLogin(const QString& username, const QString& password);
    // 判断用户名是否存在
    bool isUsernameExist(const QString& username);
    // 判断用户是否特殊用户（有邀请码）
    bool isUserSpecial(const QString& username);

private:
    UserDao() = default;
    static UserDao instance;
};

#endif
