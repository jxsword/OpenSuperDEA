/**
 * @file multiplier_model.h
 * @brief 径向乘子（对偶）模型：CCR/BCC × in/out（design_docs/06 册 §3.4）。
 * @details 包络的对偶形式直接实现（非解包络取对偶）：
 *          投入导向 max u·y0(+ω) s.t. v·x0=1, u·Y_j − v·X_j(+ω) ≤ 0, u,v ≥ 0；
 *          输出导向 min v·x0(−ω)（效率 = 1/最优值）。L3 PR-06：最优值 = 包络最优值。
 */
#pragma once

#include "core/registry/solvable_model.h"

namespace opensuperdea::radial
{
/** @brief 乘子模型（满足 registry::SolvableModel concept；RTS 限 crs/vrs）。 */
class MultiplierModel
{
  public:
    /**
     * @brief 构造乘子 LP（变量 = v(m) + u(s) + ω(VRS)）。
     * @param context 求解上下文
     * @return LP 问题
     */
    [[nodiscard]] static solver::LpProblem build_problem(const registry::SolveContext &context);

    /**
     * @brief 组装结果：效率 = 最优目标值（投入导向）或其倒数（输出导向），输出乘子。
     * @param context 求解上下文
     * @param outcome 求解结果
     * @return 单 DMU 结果（input_multipliers=v / output_multipliers=u）
     */
    [[nodiscard]] static result::DmuResult assemble_result(const registry::SolveContext &context,
                                                           const solver::SolveOutcome &outcome);
};
} // namespace opensuperdea::radial
