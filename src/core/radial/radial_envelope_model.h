/**
 * @file radial_envelope_model.h
 * @brief 径向包络模型：CCR/BCC/NIRS/NDRS 全 RTS 变体 × in/out 定向（design_docs/06 册 §3.1）。
 * @details 两阶段求解：一阶段 min θ（或 max φ）取得效率测度；二阶段固定测度后
 *          最大化松弛和，提取 slack/λ/投影（FR-OUT-002/003/004）。LP 经
 *          highs_adapter 中立接口求解（DR-004 隔离）。
 */
#pragma once

#include "core/registry/solvable_model.h"

namespace opensuperdea::radial
{
/** @brief 径向包络模型（满足 registry::SolvableModel concept）。 */
class RadialEnvelopeModel
{
  public:
    /**
     * @brief 构造一阶段包络 LP（06 册 §3.1：投入收缩 / 产出扩张 + RTS 截距约束）。
     * @param context 求解上下文（数据/被评 DMU/参考集/选项）
     * @return LP 问题（变量 = 测度 + |参考集| 个 λ；参考集为空时构造确定不可行问题）
     */
    [[nodiscard]] static solver::LpProblem build_problem(const registry::SolveContext &context);

    /**
     * @brief 组装结果：状态映射 + 二阶段松弛提取 + 投影合成（FR-OUT-002/003/004）。
     * @param context 求解上下文
     * @param outcome 一阶段求解结果
     * @return 单 DMU 结果（不可行时 status=INFEASIBLE 且不写数值）
     */
    [[nodiscard]] static result::DmuResult assemble_result(const registry::SolveContext &context,
                                                           const solver::SolveOutcome &outcome);
};
} // namespace opensuperdea::radial
