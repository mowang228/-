#ifndef JUDGE_H
#define JUDGE_H

#include <QString>
#include <QStringList>
#include "detector.h"   // VehicleAttribute
#include "database.h"   // VehicleInfo

class AiInterface;  // 前向声明

/**
 * 套牌判断结果
 * 对应 Python 版 compare_vehicle() 的返回字典
 */
struct JudgeResult
{
    QString status;       // "基本正常" / "存在差异，建议复核" / "疑似套牌" / "无登记信息" / "无法判断"
    QString riskLevel;    // "low" / "middle" / "high" / "unknown"
    double  score;        // 差异总分（0.0 ~ 1.0）
    QStringList differences; // 差异描述列表
};

/**
 * 套牌嫌疑判定模块
 * 对应 Python 版 judge.py
 *
 * 根据识别属性 observed 与登记属性 registered 计算差异分值。
 * 差异分越高 → 嫌疑越大。
 *
 * 权重配置（与 Python config.py 中 COMPARE_WEIGHTS 一致）：
 *   vehicle_type  0.35
 *   color         0.25
 *   brand         0.25
 *   model         0.15
 *
 * 嫌疑阈值 SUSPECT_THRESHOLD = 0.45
 */
class VehicleJudge
{
public:
    /** 执行比对，返回判定结果 */
    static JudgeResult compareVehicle(const VehicleAttribute &observed,
                                       const VehicleInfo &registered);

    /** 当 registered 为空（未查到登记信息）时调用 */
    static JudgeResult noRegistration(const VehicleAttribute &observed);

    /** 当 observed 无效（未识别到车牌）时调用 */
    static JudgeResult noPlateRecognized();

    // ---- AI 接口支持 ----

    /** 设置 AI 判定接口（设为 nullptr 则关闭 AI 判定） */
    static void setAiInterface(AiInterface *ai);

    /** 返回当前是否已接入 AI 接口 */
    static bool hasAiInterface();

private:
    static QString normalizeValue(const QString &value);

    /** AI 接口实例（静态，全局共享） */
    static AiInterface *m_aiInterface;
};

#endif // JUDGE_H
