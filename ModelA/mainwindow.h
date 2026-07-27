#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QString>
#include <QMap>
#include <QHash>
#include "LLMService.h"

class QLabel;
class QPushButton;
class QLineEdit;
class QListWidget;
class QTableWidget;
class QStackedWidget;
class QCheckBox;
class QPropertyAnimation;
class VehicleDatabase;
class VehicleAttributeDetector;
class LLMService;
struct VehicleAttribute;
struct JudgeResult;
struct VehicleInfo;
struct AnalysisResult;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

    /// 设置当前用户是否拥有录入权限（邀请码用户）
    void setCanEnterData(bool can);

    /// 设置当前登录用户名（右上角显示）
    void setUsername(const QString &name);

private slots:
    void onSelectFile();
    void onAnalyze();
    void onAnalysisCompleted(const AnalysisResult &result);
    void onAnalysisFailed(const QString &errorMessage);
    void onProfileClicked();

private:
    void setupUI();
    void loadStyleSheet();

    // QObject interface
    bool eventFilter(QObject *obj, QEvent *event) override;

    QWidget* createServicePrepare();
    QWidget* createUploadPredict();
    QWidget* createResultShow();
    QWidget* createCompareTable();

    /** 清空右侧面板和对比表格 */
    void resetDisplay();
    /** 将检测结果与数据库比对后更新右侧结果面板 */
    void updateResultPanel(const VehicleAttribute &detected);
    /** 将判定结果应用到界面 */
    void applyJudgeResult(const JudgeResult &jr);
    /** 设置图片边框颜色：gray / #22c55e(绿) / #ef4444(红) */
    void setImageBorder(const QString &color);
    /** 更新对比表格 */
    void updateCompareTable(const VehicleAttribute &observed,
                            const VehicleInfo *registered);

    // 中间模块
    QPushButton *m_fileBtn    = nullptr;
    QLineEdit   *m_fileName   = nullptr;
    QPushButton *m_analyzeBtn = nullptr;
    QLabel      *m_tip        = nullptr;
    QCheckBox   *m_baiduApiCheck = nullptr;

    // 右侧模块
    QLabel      *m_resultImg        = nullptr;
    QLabel      *m_statusBadge      = nullptr;   // 稽查结论徽章
    QLineEdit   *m_plateInput       = nullptr;
    QLineEdit   *m_brandInput       = nullptr;
    QLineEdit   *m_colorInput       = nullptr;
    QLineEdit   *m_typeInput        = nullptr;
    QLineEdit   *m_conclusionInput   = nullptr;
    QListWidget *m_diffList         = nullptr;   // 差异说明列表

    // 对比表格
    QTableWidget *m_compareTable    = nullptr;
    QWidget      *m_compareSection  = nullptr;   // 包含表格的容器

    // 数据库
    VehicleDatabase *m_db = nullptr;

    // 检测器
    VehicleAttributeDetector *m_detector = nullptr;

    // 当前选择的图片路径
    QString m_selectedFilePath;

    // ---- LLM 服务 ----
    LLMService *m_llmService = nullptr;

    // ---- 权限 ----
    bool m_canEnterData = false;

    // ---- 当前用户 ----
    QString m_username;
    QAction *m_profileAction = nullptr;

    // ---- 动画 ----
    QMap<QString, QPropertyAnimation*> m_bounceAnims;
};

#endif // MAINWINDOW_H
