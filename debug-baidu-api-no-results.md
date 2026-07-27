# Debug Session: Baidu API No Results

**Session ID**: baidu-api-no-results
**Status**: [RESOLVED]
**Created**: 2026-07-13
**Resolved**: 2026-07-13

## Symptom
勾选"使用百度API识别"后，结果中车辆类型仍为空，品牌显示为"未知"，说明百度API识别结果未被正确使用。

## Hypotheses

### H1: `m_useBaiduApi` 标志未正确设置为 true
**Result**: ❌ REJECTED — 日志显示 `m_useBaiduApi=true`

### H2: `detectWithBaiduApi()` 调用失败（token获取失败、网络错误、API返回error_code）
**Result**: ❌ REJECTED (partial) — Token成功获取，`/v1/car` API成功返回品牌和颜色，`/v2/vehicle_attr` 返回错误码6（无权限）

### H3: `detect()` 中条件判断 `!baiduResult.color.isEmpty() || !baiduResult.brand.isEmpty()` 为 false
**Result**: ❌ REJECTED — 日志显示 `conditionMet=true`

### H4: `m_baiduApi` 的 API Key 未正确设置
**Result**: ❌ REJECTED — `isValid()=true`，API Key非空

### H5: 百度API请求被网络/SSL拦截
**Result**: ❌ REJECTED — `/v1/car` 请求成功返回数据

## Root Cause Found (through instrumentation)

1. **车辆类型为空**: `v2/vehicle_attr` API权限不足（错误码6: No permission to access data），且合并代码无条件覆盖本地模型的"轿车"
2. **品牌被覆盖为"未知"**: LLMService 异步完成时，`onAnalysisCompleted()` 用 LLM 的 `vehicleBrand="未知"` 覆盖了已正确设置的 UI 字段
3. **品牌提取不完整**: 品牌列表缺少"宾利"，导致"宾利飞驰"无法拆分

## Fixes Applied

1. `detect()`: 仅当百度返回非空 `vehicleType` 时才覆盖（[detector.cpp](file:///D:/test/Qt2/modelf/detector.cpp)）
2. `onAnalysisCompleted()`: 不覆盖已设置的品牌字段（[mainwindow.cpp](file:///D:/test/Qt2/ModelA/mainwindow.cpp)）
3. LLMService: 移除硬编码的 `vehicleBrand="未知"`（[LLMService.cpp](file:///D:/test/Qt2/modelf/LLMService.cpp)）
4. 品牌列表补充"宾利"、"法拉利"等缺失品牌（[BaiduVehicleApi.cpp](file:///D:/test/Qt2/modelf/BaiduVehicleApi.cpp)）

