#include "mainwindow.h"
#include "database.h"
#include "detector.h"
#include "judge.h"
#include "ai_interface.h"
#include "LLMService.h"
#include "ProfileDialog.h"
#include <QCoreApplication>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QCheckBox>
#include <QFileDialog>
#include <QEvent>
#include <QMessageBox>
#include <QFile>
#include <QFileInfo>
#include <QApplication>
#include <QListWidget>
#include <QTableWidget>
#include <QHeaderView>
#include <QPixmap>
#include <QStyle>
#include <QScrollArea>
#include <QPropertyAnimation>
#include <QSequentialAnimationGroup>
#include <QParallelAnimationGroup>
#include <QPainter>
#include <QIcon>
#include <QDir>
#include <QEasingCurve>
#include <QTimer>
#include <QMenuBar>
#include <QFrame>

// ============================================================
//  构造函数 / 析构函数
// ============================================================
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle("套牌车稽查系统 V0.1");
    resize(1300, 800);

    // 初始化数据库（连接本地 MySQL car_check）
    m_db = new VehicleDatabase();
    if (!m_db->init()) {
        QMessageBox::critical(this, "数据库错误",
                              "车辆登记信息库初始化失败，部分功能不可用。");
    }

    // 初始化检测器
    m_detector = new VehicleAttributeDetector();

    VehicleJudge::setAiInterface(nullptr);

    setupUI();
    loadStyleSheet();

    // 初始化 LLM 服务
    m_llmService = new LLMService(this);
    connect(m_llmService, &LLMService::analysisCompleted,
            this, &MainWindow::onAnalysisCompleted);
    connect(m_llmService, &LLMService::analysisFailed,
            this, &MainWindow::onAnalysisFailed);
}

MainWindow::~MainWindow()
{
    delete m_detector;
    delete m_db;
}

void MainWindow::setCanEnterData(bool can)
{
    m_canEnterData = can;
}

// ============================================================
//  加载 QSS 样式表
// ============================================================
void MainWindow::loadStyleSheet()
{
    QFile qss(QCoreApplication::applicationDirPath() + "/style.qss");
    if (qss.open(QFile::ReadOnly | QFile::Text))
    {
        setStyleSheet(qss.readAll());
        qss.close();
    }
}

// ============================================================
//  主界面搭建
// ============================================================
void MainWindow::setupUI()
{
    // ---- 中央滚动容器 ----
    QScrollArea *scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setObjectName("scrollArea");
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

    QWidget *container = new QWidget(this);
    container->setObjectName("container");
    scrollArea->setWidget(container);
    setCentralWidget(scrollArea);

    QVBoxLayout *mainLayout = new QVBoxLayout(container);
    mainLayout->setContentsMargins(30, 30, 30, 30);
    mainLayout->setSpacing(16);

    // ---- 顶部标题 ----
    QLabel *title = new QLabel("套牌车稽查系统 Demo", this);
    title->setObjectName("header");
    title->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(title);

    // ---- 右上角个人账户管理 ----
    auto *menuBar = new QMenuBar(this);
    menuBar->setCornerWidget(nullptr);  // 清除默认
    m_profileAction = menuBar->addAction("👤 个人账户");
    m_profileAction->setToolTip("编辑个人信息 / 注销账号");
    connect(m_profileAction, &QAction::triggered, this, &MainWindow::onProfileClicked);
    // 将菜单栏放到右上角
    auto *topLayout = new QHBoxLayout;
    topLayout->addStretch();
    topLayout->addWidget(menuBar);
    // 插入到 mainLayout 的标题下面
    mainLayout->insertLayout(2, topLayout);

    // ---- 主体内容（三栏） ----
    QHBoxLayout *contentLayout = new QHBoxLayout;
    contentLayout->setSpacing(24);
    contentLayout->addWidget(createServicePrepare());   // 左侧
    contentLayout->addWidget(createUploadPredict());     // 中间
    contentLayout->addWidget(createResultShow());         // 右侧
    mainLayout->addLayout(contentLayout);

    // ---- 对比表格（分析完成后显示） ----
    mainLayout->addWidget(createCompareTable());

    // 底部留白（至少 40px 间距）
    mainLayout->addSpacing(40);
}

// ============================================================
//  左侧：服务准备 — 车辆雷达图 + 功能列表
// ============================================================
QWidget* MainWindow::createServicePrepare()
{
    QWidget *module = new QWidget(this);
    module->setObjectName("servicePrepare");

    QVBoxLayout *layout = new QVBoxLayout(module);
    layout->setContentsMargins(20, 20, 20, 20);
    layout->setSpacing(16);

    // 步骤标题
    {
        auto *hdr = new QWidget(this);
        auto *row = new QHBoxLayout(hdr);
        row->setContentsMargins(0, 0, 0, 0);
        row->setSpacing(8);
        auto *nLbl = new QLabel("1", this);
        nLbl->setObjectName("stepNum_1");
        auto *tLbl = new QLabel("服务准备", this);
        tLbl->setObjectName("stepText_1");
        row->addWidget(nLbl);
        row->addWidget(tLbl);
        row->addStretch();
        layout->addWidget(hdr);
    }

    // ---- 车辆雷达图 ----
    QWidget *diagram = new QWidget(this);
    diagram->setObjectName("carDiagram");
    diagram->setMinimumSize(380, 400);

    // 中心车辆图片占位
    QLabel *carCenter = new QLabel(diagram);
    carCenter->setObjectName("carCenter");
    carCenter->setFixedSize(160, 160);
    carCenter->move(110, 120);
    carCenter->raise();

    // 加载车辆图片
    QPixmap carPix("OIP-C.png");
    if (!carPix.isNull())
    {
        carCenter->setPixmap(carPix.scaled(160, 160, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        carCenter->setStyleSheet("background: transparent; border: none;");
    }

    // 8 个功能节点，环绕在中央图片周围
    QStringList nodeTexts = {
        "车牌识别", "车型匹配", "颜色识别", "年检信息",
        "车主信息", "违章记录", "保险信息", "行驶轨迹"
    };

    struct { int x; int y; } positions[8] = {
        {190, 20},   // 上中
        {35,  65},   // 左上
        {15,  180},  // 左中
        {35,  310},  // 左下
        {190, 360},  // 下中
        {275, 310},  // 右下
        {295, 180},  // 右中
        {275, 65},   // 右上
    };

    for (int i = 0; i < 8; ++i)
    {
        QLabel *node = new QLabel(nodeTexts[i], diagram);
        node->setObjectName(QString("node_%1").arg(i + 1));
        node->setFixedSize(80, 28);
        node->setAlignment(Qt::AlignCenter);
        node->move(positions[i].x, positions[i].y);
    }

    // 将汽车图片再次置顶
    carCenter->raise();

    // 动画：8 个节点从中央图片后方飘出到指定位置
    {
        QPoint centerPos(150, 186);
        auto *animGroup = new QParallelAnimationGroup(this);

        for (int i = 0; i < 8; ++i)
        {
            auto *node = diagram->findChild<QLabel*>(QString("node_%1").arg(i + 1));
            if (!node) continue;

            node->move(centerPos);

            auto *seq = new QSequentialAnimationGroup(animGroup);
            seq->addPause(50 + i * 80);

            auto *anim = new QPropertyAnimation(node, "pos");
            anim->setDuration(600);
            anim->setStartValue(centerPos);
            anim->setEndValue(QPoint(positions[i].x, positions[i].y));
            anim->setEasingCurve(QEasingCurve::OutBack);
            seq->addAnimation(anim);

            animGroup->addAnimation(seq);
        }

        // 页面显示后启动动画
        QTimer::singleShot(200, this, [animGroup]() { animGroup->start(); });

        // 落位后创建跳动动画
        QObject::connect(animGroup, &QParallelAnimationGroup::finished, this,
            [this, diagram, positions]()
        {
            for (int i = 0; i < 8; ++i)
            {
                auto *node = diagram->findChild<QLabel*>(
                    QString("node_%1").arg(i + 1));
                if (!node) continue;

                QPoint targetPos(positions[i].x, positions[i].y);

                auto *bounce = new QPropertyAnimation(node, "pos");
                bounce->setDuration(1800 + i * 150);
                bounce->setKeyValueAt(0.0,  targetPos);
                bounce->setKeyValueAt(0.5,  targetPos - QPoint(0, 4));
                bounce->setKeyValueAt(1.0,  targetPos);
                bounce->setLoopCount(-1);
                bounce->setEasingCurve(QEasingCurve::SineCurve);

                m_bounceAnims[node->objectName()] = bounce;
                node->installEventFilter(this);
            }
        });
    }

    layout->addWidget(diagram);

    layout->addStretch();
    return module;
}

// ============================================================
//  中间：上传 & 预测模块
// ============================================================
QWidget* MainWindow::createUploadPredict()
{
    QWidget *module = new QWidget(this);
    module->setObjectName("uploadPredict");

    QVBoxLayout *layout = new QVBoxLayout(module);
    layout->setAlignment(Qt::AlignTop);

    QWidget *box = new QWidget(this);
    QVBoxLayout *boxLayout = new QVBoxLayout(box);
    boxLayout->setSpacing(16);

    // 步骤标题
    {
        auto *hdr = new QWidget(this);
        auto *row = new QHBoxLayout(hdr);
        row->setContentsMargins(0, 0, 0, 0);
        row->setSpacing(8);
        auto *nLbl = new QLabel("2", this);
        nLbl->setObjectName("stepNum_2");
        auto *tLbl = new QLabel("上传 & 预测", this);
        tLbl->setObjectName("stepText_2");
        row->addWidget(nLbl);
        row->addWidget(tLbl);
        row->addStretch();
        boxLayout->addWidget(hdr);
    }

    // 选择文件按钮
    m_fileBtn = new QPushButton("选择文件", this);
    m_fileBtn->setObjectName("fileBtn");
    m_fileBtn->setCursor(Qt::PointingHandCursor);
    connect(m_fileBtn, &QPushButton::clicked, this, &MainWindow::onSelectFile);
    boxLayout->addWidget(m_fileBtn);

    // 文件名显示
    m_fileName = new QLineEdit(this);
    m_fileName->setObjectName("fileName");
    m_fileName->setText("未选择文件");
    m_fileName->setReadOnly(true);
    m_fileName->setAlignment(Qt::AlignCenter);
    boxLayout->addWidget(m_fileName);

    // 开始分析按钮
    m_analyzeBtn = new QPushButton("开始分析", this);
    m_analyzeBtn->setObjectName("analyzeBtn");
    m_analyzeBtn->setCursor(Qt::PointingHandCursor);
    connect(m_analyzeBtn, &QPushButton::clicked, this, &MainWindow::onAnalyze);
    boxLayout->addWidget(m_analyzeBtn);

    // 使用百度API复选框
    m_baiduApiCheck = new QCheckBox("使用百度API识别", this);
    m_baiduApiCheck->setObjectName("baiduApiCheck");
    m_baiduApiCheck->setChecked(true);

    // 生成对勾图片文件，用 QSS 的 image 属性引用
    {
        QString checkPath = QCoreApplication::applicationDirPath() + "/check.png";
        if (!QFile::exists(checkPath))
        {
            QPixmap px(48, 48);
            px.fill(Qt::transparent);
            {
                QPainter p(&px);
                p.setRenderHint(QPainter::Antialiasing);
                p.setPen(QPen(QColor("#2563eb"), 4));
                p.setBrush(Qt::NoBrush);
                // 画对勾
                QPolygonF polygon;
                polygon << QPointF(8, 26) << QPointF(20, 38) << QPointF(40, 10);
                p.drawPolyline(polygon);
            }
            px.save(checkPath, "PNG");
        }
        m_baiduApiCheck->setStyleSheet(
            "#baiduApiCheck {"
            "  font-size: 13px; font-weight: 500; color: #475569; padding: 6px 0; spacing: 6px;"
            "}"
            "#baiduApiCheck::indicator {"
            "  width: 16px; height: 16px; border-radius: 3px;"
            "  border: 2px solid #cbd5e1; background-color: #ffffff;"
            "}"
            "#baiduApiCheck::indicator:hover {"
            "  border-color: #3b82f6;"
            "}"
            "#baiduApiCheck::indicator:checked {"
            "  background-color: #eff6ff; border-color: #3b82f6;"
            "  image: url(" + checkPath + ");"
            "}"
        );
    }

    boxLayout->addWidget(m_baiduApiCheck);

    // 提示信息
    m_tip = new QLabel("上次检查：车辆信息正常", this);
    m_tip->setObjectName("tip");
    m_tip->setAlignment(Qt::AlignCenter);
    boxLayout->addWidget(m_tip);

    // 适配格式说明
    QLabel *formatInfo = new QLabel("支持格式：JPG / PNG / WEBP", this);
    formatInfo->setObjectName("formatInfo");
    formatInfo->setAlignment(Qt::AlignCenter);
    boxLayout->addWidget(formatInfo);

    layout->addWidget(box);
    return module;
}

// ============================================================
//  对比表格：识别项目条目 vs 图片识别结果 vs 登记信息
// ============================================================
QWidget* MainWindow::createCompareTable()
{
    m_compareSection = new QWidget(this);
    m_compareSection->setObjectName("compareSection");
    m_compareSection->setVisible(true);

    QVBoxLayout *layout = new QVBoxLayout(m_compareSection);
    layout->setContentsMargins(20, 16, 20, 16);
    layout->setSpacing(8);

    QLabel *title = new QLabel("识别信息与登记信息对比", this);
    title->setObjectName("tableTitle");
    layout->addWidget(title);

    // 分隔线
    auto *sep1 = new QFrame(this);
    sep1->setFrameShape(QFrame::HLine);
    sep1->setFrameShadow(QFrame::Sunken);
    sep1->setStyleSheet("color: #e4e8ed; margin: 0;");
    sep1->setFixedHeight(1);
    layout->addWidget(sep1);

    m_compareTable = new QTableWidget(6, 3, this);
    m_compareTable->setObjectName("compareTable");
    m_compareTable->setHorizontalHeaderLabels({"识别项目条目", "图片识别结果", "登记信息"});
    m_compareTable->verticalHeader()->setVisible(false);
    m_compareTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_compareTable->setSelectionMode(QAbstractItemView::NoSelection);
    m_compareTable->setShowGrid(false);
    m_compareTable->setAlternatingRowColors(true);

    QHeaderView *header = m_compareTable->horizontalHeader();
    // 列 0 固定宽度 120px，列 1 / 列 2 自动拉伸填满
    header->setSectionResizeMode(0, QHeaderView::Fixed);
    header->resizeSection(0, 120);
    header->setSectionResizeMode(1, QHeaderView::Stretch);
    header->setSectionResizeMode(2, QHeaderView::Stretch);
    header->setStretchLastSection(false);

    QString rowLabels[] = {"编号", "车牌号", "车辆类型", "车身颜色", "车辆品牌"};
    for (int i = 0; i < 5; ++i) {
        m_compareTable->setItem(i, 0, new QTableWidgetItem(rowLabels[i]));
        m_compareTable->setItem(i, 1, new QTableWidgetItem("-"));
        m_compareTable->setItem(i, 2, new QTableWidgetItem("-"));
    }

    layout->addWidget(m_compareTable);

    // 固定每行高度，让表格正常展开
    {
        int rowH = 32;
        for (int row = 0; row < 6; ++row)
            m_compareTable->setRowHeight(row, rowH);
        // 表格总高度 ≈ 表头 ~28px + 6行 × 32px
        int totalH = 28 + 6 * rowH;
        m_compareTable->setMinimumHeight(totalH);
        m_compareTable->setMaximumHeight(totalH);
    }

    layout->addStretch();
    return m_compareSection;
}

// ============================================================
//  右侧：结果展示（图片预览 + 状态徽章 + 表单 + 差异说明）
// ============================================================
QWidget* MainWindow::createResultShow()
{
    QWidget *module = new QWidget(this);
    module->setObjectName("resultShow");

    QVBoxLayout *layout = new QVBoxLayout(module);
    layout->setContentsMargins(16, 16, 16, 16);
    layout->setSpacing(12);

    // 步骤标题
    {
        auto *hdr = new QWidget(this);
        auto *row = new QHBoxLayout(hdr);
        row->setContentsMargins(0, 0, 0, 0);
        row->setSpacing(8);
        auto *nLbl = new QLabel("3", this);
        nLbl->setObjectName("stepNum_3");
        auto *tLbl = new QLabel("结果展示", this);
        tLbl->setObjectName("stepText_3");
        row->addWidget(nLbl);
        row->addWidget(tLbl);
        row->addStretch();
        layout->addWidget(hdr);
    }

    // 图片预览
    m_resultImg = new QLabel(this);
    m_resultImg->setObjectName("resultImg");
    m_resultImg->setMaximumHeight(200);
    m_resultImg->setMinimumHeight(120);
    m_resultImg->setAlignment(Qt::AlignCenter);
    m_resultImg->setText("暂无图片");
    layout->addWidget(m_resultImg);

    // 状态徽章
    m_statusBadge = new QLabel("待分析", this);
    m_statusBadge->setObjectName("statusBadge");
    m_statusBadge->setAlignment(Qt::AlignCenter);
    m_statusBadge->setMinimumHeight(40);
    m_statusBadge->setProperty("riskLevel", "unknown");
    layout->addWidget(m_statusBadge);

    // 表单：车牌号 / 品牌 / 颜色 / 车型 / 稽查结论
    struct FormRow { QString label; };
    FormRow rows[5] = {
        {"车牌号"}, {"车辆品牌"}, {"车身颜色"}, {"车辆类型"}, {"稽查结论"}
    };

    QWidget *form = new QWidget(this);
    QVBoxLayout *formLayout = new QVBoxLayout(form);
    formLayout->setContentsMargins(0, 0, 0, 0);
    formLayout->setSpacing(8);

    for (int i = 0; i < 5; ++i)
    {
        QWidget *rowWidget = new QWidget(this);
        QHBoxLayout *rowLayout = new QHBoxLayout(rowWidget);
        rowLayout->setContentsMargins(0, 0, 0, 0);
        rowLayout->setSpacing(10);

        QLabel *label = new QLabel(rows[i].label, this);
        label->setObjectName("formLabel");
        label->setFixedWidth(64);
        label->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

        QLineEdit *input = new QLineEdit("", this);
        input->setObjectName("formInput");
        input->setReadOnly(true);
        input->setFocusPolicy(Qt::NoFocus);
        input->setMinimumHeight(32);

        if (i == 0)      m_plateInput = input;
        else if (i == 1) m_brandInput = input;
        else if (i == 2) m_colorInput = input;
        else if (i == 3) m_typeInput  = input;
        else {
            m_conclusionInput = input;
            input->setObjectName("conclusionInput");
            input->setStyleSheet("color: #e74c3c; font-weight: bold;");
        }

        rowLayout->addWidget(label);
        rowLayout->addWidget(input);
        formLayout->addWidget(rowWidget);
    }

    layout->addWidget(form);

    // 差异说明列表
    QLabel *diffTitle = new QLabel("差异说明", this);
    diffTitle->setObjectName("diffTitle");
    layout->addWidget(diffTitle);

    m_diffList = new QListWidget(this);
    m_diffList->setObjectName("diffList");
    m_diffList->setMinimumHeight(60);
    m_diffList->setMaximumHeight(90);
    layout->addWidget(m_diffList);

    return module;
}

// ============================================================
//  槽函数：选择文件
// ============================================================
void MainWindow::onSelectFile()
{
    QString path = QFileDialog::getOpenFileName(
        this, "选择车辆图片", QString(),
        "图片文件 (*.png *.jpg *.jpeg *.bmp *.gif);;所有文件 (*)");

    if (!path.isEmpty())
    {
        m_selectedFilePath = path;
        m_fileName->setText(path.section('/', -1).section('\\', -1));

        // 加载图片预览
        QPixmap pix(path);
        if (!pix.isNull()) {
            m_resultImg->setPixmap(pix.scaled(m_resultImg->width(), 220,
                                               Qt::KeepAspectRatio,
                                               Qt::SmoothTransformation));
        }

        // 恢复边框为灰色（检测前无色状态）
        setImageBorder("#cbd5e1");
    }
}

// ============================================================
//  槽函数：开始分析（检测 + 数据库比对 + LLM 分析）
// ============================================================
void MainWindow::onAnalyze()
{
    if (m_fileName->text().isEmpty() || m_fileName->text() == "未选择文件")
    {
        QMessageBox::warning(this, "提示", "请先选择车辆图片");
        return;
    }

    // ---- 重置上次结果 ----
    resetDisplay();

    // ---- 1. 用检测器识别车辆属性 + 数据库比对 ----
    bool useBaidu = m_baiduApiCheck->isChecked();
    m_detector->setUseBaiduApi(useBaidu);

    if (useBaidu)
    {
        m_tip->setText("正在调用百度API识别，请稍候…");
    }
    else
    {
        m_tip->setText("正在使用本地模型识别，请稍候…");
    }

    VehicleAttribute detected = m_detector->detect(m_selectedFilePath);

    VehicleInfo info;
    if (detected.isValid()) {
        info = m_db->queryByPlate(detected.plateNo);
    }

    updateCompareTable(detected, info.isValid() ? &info : nullptr);
    updateResultPanel(detected);

    // ---- 2. 同时启动 LLM 分析 ----
    m_analyzeBtn->setEnabled(false);
    m_analyzeBtn->setText("分析中…");
    m_tip->setText("正在调用大模型分析…");

    m_llmService->analyze(m_selectedFilePath);
}

// ============================================================
//  根据检测结果与数据库比对，更新右侧面板
// ============================================================
void MainWindow::updateResultPanel(const VehicleAttribute &detected)
{
    // 检查车牌是否有效
    if (!detected.isValid()) {
        // 未识别到车牌：所有字段显示"无"，稽查结论显示"无车牌"
        m_plateInput->setText(QStringLiteral("无"));
        m_brandInput->setText(QStringLiteral("无"));
        m_colorInput->setText(QStringLiteral("无"));
        m_typeInput->setText(QStringLiteral("无"));
        if (m_conclusionInput)
            m_conclusionInput->setText(QStringLiteral("无车牌"));
        JudgeResult jr = VehicleJudge::noPlateRecognized();
        applyJudgeResult(jr);
        setImageBorder("#cbd5e1");   // 检测失败 → 灰色
        return;
    }

    // 展示检测到的信息
    m_plateInput->setText(detected.plateNo);
    m_brandInput->setText(detected.brand.isEmpty() ? QStringLiteral("无") : detected.brand);
    m_colorInput->setText(detected.color.isEmpty() ? QStringLiteral("无") : detected.color);
    m_typeInput->setText(detected.vehicleType.isEmpty() ? QStringLiteral("无") : detected.vehicleType);

    // 差异分值 Tooltip 说明
    const QString scoreTooltip =
        QStringLiteral("差异分值计算公式：\n"
                       "  车辆类型不一致 +0.40\n"
                       "  车身颜色不一致 +0.30\n"
                       "  车辆品牌不一致 +0.30\n"
                       "总分 = 以上累加（0.00 ~ 1.00）\n"
                       "0 分 = 完全一致，分数越高差异越大。\n"
                       "若该车无登记记录则分值为 0。\n\n"
                       "当前识别值 vs 数据库登记值：");

    // 2. 查询数据库
    VehicleInfo info = m_db->queryByPlate(detected.plateNo);

    if (!info.isValid()) {
        // ---- 结论 3：查无此车 ----
        // 自动写入新数据
        m_db->insertCar(detected.plateNo, detected.vehicleType,
                        detected.color, detected.brand, QString());

        m_statusBadge->setText(QStringLiteral("查无此车（差异分值：0.00）"));
        m_statusBadge->setProperty("riskLevel", "unknown");
        m_statusBadge->setToolTip(scoreTooltip);
        m_statusBadge->style()->unpolish(m_statusBadge);
        m_statusBadge->style()->polish(m_statusBadge);

        if (m_conclusionInput)
            m_conclusionInput->setText(QStringLiteral("查无此车"));

        m_diffList->clear();
        m_diffList->addItem(QStringLiteral("车辆登记库中未查询到该车牌，已自动录入新数据"));

        m_tip->setText(QStringLiteral("查无此车 — 信息已自动录入数据库"));
        setImageBorder("#22c55e");   // 查无此车 → 绿色
        return;
    }

    // ---- 3. 有登记信息：执行比对 ----
    JudgeResult jr = VehicleJudge::compareVehicle(detected, info);

    if (jr.score == 0.0) {
        // ---- 结论 1：检测无误 ----
        m_statusBadge->setText(QStringLiteral("检测无误（差异分值：0.00）"));
        m_statusBadge->setProperty("riskLevel", "low");
        m_statusBadge->setToolTip(scoreTooltip);
        m_statusBadge->style()->unpolish(m_statusBadge);
        m_statusBadge->style()->polish(m_statusBadge);

        if (m_conclusionInput)
            m_conclusionInput->setText(QStringLiteral("检测无误"));

        m_diffList->clear();
        m_diffList->addItem(QStringLiteral("识别属性与登记信息完全一致"));

        m_tip->setText(QStringLiteral("检测无误 — 信息完全一致"));
        setImageBorder("#22c55e");   // 正常 → 绿色
    } else {
        // ---- 结论 2：套牌车 ----
        m_statusBadge->setText(QStringLiteral("套牌车（差异分值：%1）").arg(jr.score, 0, 'f', 2));
        m_statusBadge->setProperty("riskLevel", "high");
        m_statusBadge->setToolTip(scoreTooltip);
        m_statusBadge->style()->unpolish(m_statusBadge);
        m_statusBadge->style()->polish(m_statusBadge);

        if (m_conclusionInput)
            m_conclusionInput->setText(QStringLiteral("套牌车"));

        m_diffList->clear();
        if (jr.differences.isEmpty()) {
            m_diffList->addItem(QStringLiteral("存在差异，建议复核"));
        } else {
            for (const QString &diff : jr.differences) {
                m_diffList->addItem(diff);
            }
        }

        m_tip->setText(QStringLiteral("套牌车 — 差异分 %1").arg(jr.score, 0, 'f', 2));
        setImageBorder("#ef4444");   // 套牌车 → 红色
    }
}

// ============================================================
//  将判定结果应用到界面（状态徽章 + 差异列表 + 提示）
// ============================================================
void MainWindow::applyJudgeResult(const JudgeResult &jr)
{
    // 状态徽章
    m_statusBadge->setText(QString("%1（差异分值：%2）")
                               .arg(jr.status)
                               .arg(jr.score, 0, 'f', 2));
    m_statusBadge->setProperty("riskLevel", jr.riskLevel);

    // 刷新样式
    m_statusBadge->style()->unpolish(m_statusBadge);
    m_statusBadge->style()->polish(m_statusBadge);

    // 差异说明列表
    m_diffList->clear();
    if (jr.differences.isEmpty()) {
        m_diffList->addItem(jr.status);
    } else {
        for (const QString &diff : jr.differences) {
            m_diffList->addItem(diff);
        }
    }

    // 提示信息
    QString tip;
    if (jr.riskLevel == "unknown") {
        tip = jr.differences.isEmpty() ? jr.status : jr.differences.first();
    } else {
        tip = QString("差异分 %1 — %2")
                  .arg(jr.score, 0, 'f', 2)
                  .arg(jr.differences.isEmpty() ? jr.status : jr.differences.first());
    }

    // 如果使用了 AI 判定，在提示中标注
    if (VehicleJudge::hasAiInterface()) {
        tip = QString("[AI] %1").arg(tip);
        m_statusBadge->setText(QString("%1（差异分值：%2）[AI]")
                                   .arg(jr.status)
                                   .arg(jr.score, 0, 'f', 2));
    }

    m_tip->setText(tip);
}

// ============================================================
//  设置图片边框颜色
// ============================================================
void MainWindow::setImageBorder(const QString &color)
{
    m_resultImg->setStyleSheet(QStringLiteral(
        "border: 2px solid %1;"
        "border-radius: 10px;"
        "background-color: #f1f5f9;"
    ).arg(color));
}

// ============================================================
//  更新对比表格
// ============================================================
void MainWindow::updateCompareTable(const VehicleAttribute &observed,
                                     const VehicleInfo *registered)
{
    QString rowKeys[] = {"id", "plate_no", "vehicle_type", "color", "brand"};
    QString obsVals[] = {"-", observed.plateNo, observed.vehicleType,
                         observed.color, observed.brand};

    // 未识别到车牌时，图片识别结果全部显示"无"
    if (observed.plateNo.isEmpty()) {
        for (int i = 1; i < 5; ++i)
            obsVals[i] = QStringLiteral("无");
    } else {
        // 仅转换个别空字段
        for (int i = 1; i < 5; ++i) {
            if (obsVals[i].isEmpty())
                obsVals[i] = QStringLiteral("无");
        }
    }

    // 图片识别结果列（索引 1）
    for (int i = 0; i < 5; ++i) {
        m_compareTable->item(i, 1)->setText(obsVals[i]);
    }

    // 登记信息列（索引 2）
    if (registered) {
        QString regVals[5];
        regVals[0] = QString::number(registered->id);
        regVals[1] = registered->plateNo;
        regVals[2] = registered->vehicleType;
        regVals[3] = registered->color;
        regVals[4] = registered->brand;

        for (int j = 0; j < 5; ++j) {
            m_compareTable->item(j, 2)->setText(regVals[j]);
        }
    } else {
        for (int j = 0; j < 5; ++j) {
            m_compareTable->item(j, 2)->setText("无");
        }
    }

    m_compareSection->setVisible(true);
}

// ============================================================
//  清空上次显示结果
// ============================================================
void MainWindow::resetDisplay()
{
    m_statusBadge->setText("分析中…");
    m_statusBadge->setProperty("riskLevel", "unknown");
    m_statusBadge->style()->unpolish(m_statusBadge);
    m_statusBadge->style()->polish(m_statusBadge);

    m_diffList->clear();

    // 对比表格重新隐藏
    m_compareSection->setVisible(false);
}

// ============================================================
//  LLM 分析回调
// ============================================================
void MainWindow::onAnalysisCompleted(const AnalysisResult &result)
{
    // LLM 的稽查结论写入提示栏
    m_tip->setText(QString("上次检查：%1").arg(result.conclusion));

    // LLM 分析的结果只更新结论，不覆盖已检测的车牌和品牌
    // （车牌和品牌已在 updateResultPanel 中由 detect() 正确设置）
    // 用 LLM 校正后的数据更新对比表格的「图片识别结果」列
    // 行：4=车辆品牌

    if (!result.resultImagePath.isEmpty() && QFile::exists(result.resultImagePath))
    {
        QPixmap pixmap(result.resultImagePath);
        if (!pixmap.isNull())
        {
            m_resultImg->setPixmap(pixmap.scaled(m_resultImg->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
            m_resultImg->setAlignment(Qt::AlignCenter);
        }
                                                      }
    else if (!m_selectedFilePath.isEmpty())
    {
        QPixmap pixmap(m_selectedFilePath);
        if (!pixmap.isNull())
        {
            m_resultImg->setPixmap(pixmap.scaled(m_resultImg->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
            m_resultImg->setAlignment(Qt::AlignCenter);
        }
    }

    m_analyzeBtn->setEnabled(true);
    m_analyzeBtn->setText("开始分析");
}

void MainWindow::onAnalysisFailed(const QString &errorMessage)
{
    QMessageBox::critical(this, "分析失败", errorMessage);
    m_tip->setText("分析失败");
    m_analyzeBtn->setEnabled(true);
    m_analyzeBtn->setText("重新分析");
}

// ============================================================
//  事件过滤器：节点 hover 跳动控制
// ============================================================
bool MainWindow::eventFilter(QObject *obj, QEvent *event)
{
    auto *anim = m_bounceAnims.value(obj->objectName());
    if (!anim)
        return QMainWindow::eventFilter(obj, event);

    if (event->type() == QEvent::Enter)
    {
        anim->start();
    }
    else if (event->type() == QEvent::Leave)
    {
        anim->stop();
        // 复位到目标位置
        auto *node = qobject_cast<QLabel*>(obj);
        if (node)
            node->move(anim->endValue().toPoint());
    }

    return QMainWindow::eventFilter(obj, event);
}

// ============================================================
//  设置当前用户名
// ============================================================
void MainWindow::setUsername(const QString &name)
{
    m_username = name;
    if (m_profileAction)
        m_profileAction->setText(QString("👤 %1").arg(name));
}

// ============================================================
//  右上角个人账户管理
// ============================================================
void MainWindow::onProfileClicked()
{
    if (m_username.isEmpty())
    {
        QMessageBox::information(this, "提示", "未登录");
        return;
    }

    ProfileDialog dlg(m_username, this);
    if (dlg.exec() == QDialog::Accepted && dlg.isAccountDeleted())
    {
        QMessageBox::information(this, "已注销", "账号已注销，程序将退出");
        close();
    }
}
