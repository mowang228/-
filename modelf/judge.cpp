#include "judge.h"
#include "ai_interface.h"
#include <QMap>
#include <cmath>

// AI 接口实例（静态，初始为 nullptr 表示未接入 AI）
AiInterface *VehicleJudge::m_aiInterface = nullptr;

// ============================================================
//  配置常量（对应 Python config.py）
// ============================================================
static constexpr double SUSPECT_THRESHOLD = 0.45;

static const QMap<QString, double> COMPARE_WEIGHTS = {
    {"vehicle_type", 0.40},
    {"color",        0.30},
    {"brand",        0.30},
};

// 字段中文名（对应 Python FIELD_CN）
static QString fieldCN(const QString &field)
{
    static const QMap<QString, QString> MAP = {
        {"vehicle_type", QStringLiteral("车辆类型")},
        {"color",        QStringLiteral("车身颜色")},
        {"brand",        QStringLiteral("车辆品牌")},
    };
    return MAP.value(field, field);
}

// ============================================================
//  值标准化（支持中英文对照）
// ============================================================
QString VehicleJudge::normalizeValue(const QString &value)
{
    if (value.isEmpty()) return {};

    static const QMap<QString, QString> CN_TO_EN = {
        // 颜色对照
        {QStringLiteral("黄色"),   "yellow"},
        {QStringLiteral("橙色"),   "orange"},
        {QStringLiteral("绿色"),   "green"},
        {QStringLiteral("灰色"),   "gray"},
        {QStringLiteral("红色"),   "red"},
        {QStringLiteral("蓝色"),   "blue"},
        {QStringLiteral("白色"),   "white"},
        {QStringLiteral("金色"),   "golden"},
        {QStringLiteral("棕色"),   "brown"},
        {QStringLiteral("黑色"),   "black"},
        // 车型对照
        {QStringLiteral("轿车"),   "car"},
        {QStringLiteral("SUV"),    "suv"},
        {QStringLiteral("面包车"), "van"},
        {QStringLiteral("两厢车"), "hatchback"},
        {QStringLiteral("MPV"),    "mpv"},
        {QStringLiteral("皮卡"),   "pickup"},
        {QStringLiteral("客车"),   "bus"},
        {QStringLiteral("货车"),   "truck"},
        {QStringLiteral("旅行车"), "estate"},
    };

    QString v = value.trimmed();
    // 如果是中文，转成英文再 toLower
    if (CN_TO_EN.contains(v))
        return CN_TO_EN[v];
    // 否则直接 toLower（兼容原有英文值）
    return v.toLower();
}

// ============================================================
//  无登记信息
// ============================================================
JudgeResult VehicleJudge::noRegistration(const VehicleAttribute &observed)
{
    Q_UNUSED(observed);
    JudgeResult r;
    r.status    = QStringLiteral("无登记信息");
    r.riskLevel = QStringLiteral("unknown");
    r.score     = 0.0;
    r.differences.append(QStringLiteral("车辆登记库中未查询到该车牌"));
    return r;
}

// ============================================================
//  未识别到车牌
// ============================================================
JudgeResult VehicleJudge::noPlateRecognized()
{
    JudgeResult r;
    r.status    = QStringLiteral("无法判断");
    r.riskLevel = QStringLiteral("unknown");
    r.score     = 0.0;
    r.differences.append(QStringLiteral("未识别到有效车牌号码"));
    return r;
}

// ============================================================
//  AI 接口管理
// ============================================================
void VehicleJudge::setAiInterface(AiInterface *ai)
{
    m_aiInterface = ai;
}

bool VehicleJudge::hasAiInterface()
{
    return m_aiInterface != nullptr;
}

// ============================================================
//  核心比对逻辑
// ============================================================
JudgeResult VehicleJudge::compareVehicle(const VehicleAttribute &observed,
                                          const VehicleInfo &registered)
{
    // ---- AI 判定分支 ----
    // 如果已接入 AI 接口且可用，优先使用 AI 判定
    if (hasAiInterface()) {
        JudgeResult aiResult = m_aiInterface->judge(observed, registered);
        // AI 返回的结果直接使用，不做二次处理
        return aiResult;
    }

    // ---- 规则判定分支（无 AI 时的原有逻辑） ----
    JudgeResult r;
    r.score = 0.0;

    // 定义字段映射：字段名 → observed值 → registered值
    struct FieldPair {
        QString name;
        QString obsVal;
        QString regVal;
    };

    FieldPair fields[] = {
        {"vehicle_type", observed.vehicleType, registered.vehicleType},
        {"color",        observed.color,       registered.color},
        {"brand",        observed.brand,       registered.brand},
    };

    for (const auto &f : fields) {
        QString obs = normalizeValue(f.obsVal);
        QString reg = normalizeValue(f.regVal);

        qDebug().noquote() << QStringLiteral("差异比对 [%1]: 识别=\"%2\" →归一化=\"%3\" | 登记=\"%4\" →归一化=\"%5\"")
                               .arg(f.name, f.obsVal, obs, f.regVal, reg);

        // 只有双方都有值且不一致时才计分
        if (!obs.isEmpty() && !reg.isEmpty() && obs != reg) {
            r.score += COMPARE_WEIGHTS.value(f.name, 0.0);
            r.differences.append(
                QStringLiteral("%1不一致：识别为 %2，登记为 %3")
                    .arg(fieldCN(f.name))
                    .arg(f.obsVal)
                    .arg(f.regVal)
            );
        }
    }

    // 判定风险等级
    if (r.score >= SUSPECT_THRESHOLD) {
        r.status    = QStringLiteral("疑似套牌");
        r.riskLevel = QStringLiteral("high");
    } else if (r.score > 0) {
        r.status    = QStringLiteral("存在差异，建议复核");
        r.riskLevel = QStringLiteral("middle");
    } else {
        r.status    = QStringLiteral("基本正常");
        r.riskLevel = QStringLiteral("low");
        r.differences.append(QStringLiteral("识别属性与登记信息基本一致"));
    }

    // 保留两位小数
    r.score = std::round(r.score * 1000.0) / 1000.0;
    return r;
}
