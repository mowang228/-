#include <QApplication>
#include <QTabWidget>
#include <QWidget>
#include <QFile>
#include <QIcon>
#include <QMenuBar>
#include <QAction>
#include "mainwindow.h"
#include "LoginDialog.h"
#include "vehicleentrydialog.h"
#include "ProfileDialog.h"
#include "PersonnelManagementDialog.h"
#include "Config.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    // 初始化 MySQL 数据库配置（自动生成 config.ini）
    Config::getInstance();

    // 设置应用图标（标题栏、任务栏、Alt+Tab）
    app.setWindowIcon(QIcon("appicon.ico"));

    // 加载全局样式（登录窗口也应用）
    QFile qss(QCoreApplication::applicationDirPath() + "/style.qss");
    if (qss.open(QFile::ReadOnly | QFile::Text))
    {
        app.setStyleSheet(qss.readAll());
        qss.close();
    }

    // 登录 / 注册
    LoginDialog login;
    if (login.exec() != QDialog::Accepted)
    {
        return 0;   // 用户关闭窗口或登录失败 → 退出程序
    }

    // 登录成功 → 打开主界面
    MainWindow w;
    w.setWindowTitle(QString("套牌车稽查系统 V0.1 — 当前用户: %1").arg(login.loggedInUser()));
    w.setUsername(login.loggedInUser());

    // 根据用户权限添加菜单项
    if (login.isSpecialUser())
    {
        w.setCanEnterData(true);
        auto *dataMenu = w.menuBar()->addMenu("数据管理");
        auto *entryAction = dataMenu->addAction("车辆数据录入");
        QObject::connect(entryAction, &QAction::triggered, [&w]() {
            auto *dialog = new VehicleEntryDialog(&w);
            dialog->setAttribute(Qt::WA_DeleteOnClose);
            dialog->show();
        });

        // 人员管理
        auto *personnelAction = dataMenu->addAction("人员管理");
        QObject::connect(personnelAction, &QAction::triggered, [&w]() {
            auto *dialog = new PersonnelManagementDialog(&w);
            dialog->setAttribute(Qt::WA_DeleteOnClose);
            dialog->show();
        });
    }

    w.show();

    return app.exec();
}
