/**
 * @file malmquist.h
 * @brief 相邻 Malmquist 指数（FR-TFP-001；06 册 §10）：距离函数内核 + EC/TC 分解。
 * @details Färe 1994 惯例（MI > 1 = 进步，R productivity 同口径）：
 *          以 Shephard 距离 D = 1/φ 表达：EC = D_{t+1}(t+1)/D_t(t)，
 *          MI = √[D_t(t)/D_t(t+1) × D_{t+1}(t)/D_{t+1}(t+1)]；TC = MI/EC
 *          （PR-05 恒等式随套件断言）。距离函数内核 = 产出导向径向包络 φ。
 *          不平衡面板：相邻期交集缺失的 DMU 跳过并记入 warnings（02 册 §2.3）。
 *          M5 七参比只换技术集定义（参考集选择器），内核不动。
 */
#pragma once

#include "core/dataset/dataset.h"
#include "core/registry/model_runner.h"

namespace opensuperdea::tfp
{
/**
 * @brief 运行相邻 Malmquist（全部相邻期对 × 交集 DMU）。
 * @param panel 面板数据（≥2 期）
 * @param options 定向（in/out 均可，默认语义为产出导向距离）/RTS（VRS 跨期不可行跳过）
 * @param config 元信息
 * @return 结果：每行一个 (dmu, 相邻期对)，score = MI，annotations 含 ec/tc
 */
[[nodiscard]] result::ModelResult run_malmquist(const dataset::PanelData &panel,
                                                const registry::SolveOptions &options,
                                                const registry::RunConfig &config);
} // namespace opensuperdea::tfp
