/**
 * @file ddf_model.h
 * @brief 方向距离函数 DDF：max β s.t. Xλ ≤ x0 − β·g_x; Yλ ≥ y0 + β·g_y（06 册 §5）。
 * @details 方向来源（model_options["direction"]）：
 *          own（默认，g=(x0,y0)）/ mean（数据集均值）/ unit（全 1）/
 *          custom（Dataset.directions 的 di_/do_ 列）/ rdm（观测范围方向，FR-DDF-002）。
 *          定向：in 仅投入侧移动（g_y=0）、out 仅产出侧、non 双侧。
 */
#pragma once

#include "core/registry/solvable_model.h"

namespace opensuperdea::distance
{
/** @brief DDF 模型（满足 registry::SolvableModel concept）。 */
class DdfModel
{
  public:
    [[nodiscard]] static solver::LpProblem build_problem(const registry::SolveContext &context);
    [[nodiscard]] static result::DmuResult assemble_result(const registry::SolveContext &context,
                                                           const solver::SolveOutcome &outcome);
};
} // namespace opensuperdea::distance
