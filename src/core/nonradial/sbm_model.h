/**
 * @file sbm_model.h
 * @brief SBM 模型族：SBM（3 定向）/加权 SBM/超效率 SBM（design_docs/06 册 §4.1-§4.2）。
 * @details Tone 2001 SBM 经 Charnes-Cooper 变换为 LP；投入/产出定向为无 CC 的
 *          线性形式（MaxDEA 口径，对拍确认）。超效率 SBM 为 Tone 2002
 *          exclude_self 形式（06 册 §4.2，不可行处理同径向）。
 */
#pragma once

#include "core/registry/solvable_model.h"

namespace opensuperdea::nonradial
{
/**
 * @brief SBM 求解器（满足 registry::SolvableModel concept）。
 * @details 变体由 context.options 决定：
 *          - orientation：in / out / non（非导向 = 标准图形测度）；
 *          - rts：CRS / VRS（VRS 加 Σλ=t 约束）；
 *          - model_options["super"] = "1" 时构造超效率 LP（参考集应为 exclude_self）；
 *          - model_options["weight"] = "1" 时消费 Dataset.weights（加权 SBM，
 *            FR-NRAD-004 #6-8；权重和须为 1，V-010 前置）。
 */
class SbmModel
{
  public:
    [[nodiscard]] static solver::LpProblem build_problem(const registry::SolveContext &context);
    [[nodiscard]] static result::DmuResult assemble_result(const registry::SolveContext &context,
                                                           const solver::SolveOutcome &outcome);
};
} // namespace opensuperdea::nonradial
