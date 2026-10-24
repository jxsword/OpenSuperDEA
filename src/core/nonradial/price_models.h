/**
 * @file price_models.h
 * @brief 价格族模型：成本 / 收益 / 利润效率（FR-COST-001~003；06 册 §6）。
 * @details 教科书形式（Cooper-Seiford-Tone）：
 *          - 成本：CE = w·x0 / w·x*，x* = argmin w·x s.t. 技术可行（Yλ ≥ y0）；
 *          - 收益：RE = p·y0 / p·y*，y* = argmax p·y s.t. Xλ ≤ x0；
 *          - 利润：PE = (p·y0 − w·x0)/(p·y* − w·x*)（可为负；CRS 下可能无界，
 *            无界记 infeasible 不输出数值，建议 VRS）。
 *          价格取被评 DMU 的 pi_/po_ 行（逐 DMU 价格，FR-IO-003）。
 */
#pragma once

#include "core/registry/solvable_model.h"

namespace opensuperdea::nonradial
{
/** @brief 成本效率模型（含技术/配置分解标注，FR-COST-001）。 */
class CostModel
{
  public:
    [[nodiscard]] static solver::LpProblem build_problem(const registry::SolveContext &context);
    [[nodiscard]] static result::DmuResult assemble_result(const registry::SolveContext &context,
                                                           const solver::SolveOutcome &outcome);
};

/** @brief 收益效率模型（FR-COST-002）。 */
class RevenueModel
{
  public:
    [[nodiscard]] static solver::LpProblem build_problem(const registry::SolveContext &context);
    [[nodiscard]] static result::DmuResult assemble_result(const registry::SolveContext &context,
                                                           const solver::SolveOutcome &outcome);
};

/** @brief 利润效率模型（FR-COST-003；效率值可为负）。 */
class ProfitModel
{
  public:
    [[nodiscard]] static solver::LpProblem build_problem(const registry::SolveContext &context);
    [[nodiscard]] static result::DmuResult assemble_result(const registry::SolveContext &context,
                                                           const solver::SolveOutcome &outcome);
};
} // namespace opensuperdea::nonradial
