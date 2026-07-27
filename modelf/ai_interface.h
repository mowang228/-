#ifndef AI_INTERFACE_H
#define AI_INTERFACE_H

#include "judge.h"
#include "detector.h"
#include "database.h"

/// AI 判定接口 — 抽象基类
class AiInterface
{
public:
    virtual ~AiInterface() = default;

    /// 对 observed（识别属性）与 registered（登记信息）进行 AI 判定
    virtual JudgeResult judge(const VehicleAttribute &observed,
                              const VehicleInfo &registered) = 0;
};

#endif // AI_INTERFACE_H
