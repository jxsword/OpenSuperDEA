/**
 * @file fdh_model.h
 * @brief FDH 自由处置包络（MILP，λ 为 0-1 整数变量；FR-FDH-001；06 册 §7）。
 * @details 去掉凸性假设（无 Σλ 约束、λ ∈ {0,1}）：参考技术 = 观测点的自由处置
 *          支配关系并集。经 HiGHS MILP 求解（adapters::solve_milp，注册表以
 *          手工工厂 lambda 封装，见 model_registry.cpp）。
 */
#pragma once

#include "core/registry/solvable_model.h"

namespace opensuperdea::nonradial
{
/** @brief FDH 模型（不走 SolvableModel concept：MILP 需整数性标记）。 */
class FdhModel
{
  public:
    /**
     * @brief 构造 FDH MILP（变量 = 测度 + λ；integrality 标记 λ 为 0-1 整数）。
     * @param context 求解上下文（in/out 定向；FDH 无 VRS 概念，rts 被忽略）
     */
    [[nodiscard]] static solver::MilpProblem build_milp_problem(
        const registry::SolveContext &context);

    /** @brief 组装结果（效率语义同径向：投入 θ / 产出 1/φ）。 */
    [[nodiscard]] static result::DmuResult assemble_result(const registry::SolveContext &context,
                                                           const solver::SolveOutcome &outcome);
};
} // namespace opensuperdea::nonradial
