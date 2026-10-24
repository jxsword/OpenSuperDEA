/**
 * @file additive_models.h
 * @brief 加权加性族 + 方向 SBM + ERM（FR-NRAD-003/004；06 册 §4.3-§4.4，P-006 核定清单）。
 * @details 加性族统一 LP：max Σw_i·s⁻ + Σw_r·s⁺ s.t. Xλ+s⁻=x0, Yλ−s⁺=y0，
 *          score = 加权目标和（有效 DMU 为 0；排序语义与径向相反，见导出说明）。
 *          权重模式（model_options["weight_mode"]）：
 *          raw（ADD #1）/ mean（归一化加权 #2，Lovell-Pastor 1995）/
 *          range（RAM #3，Cooper-Pastor-Tone 1997）/ bam（BAM #4，界 = 数据范围，
 *          用户界参数化留 M5）/ user（wADD #9，消费 w_in_/w_out_ 列，V-010 前置）。
 *          DSBM #5：方向 SBM（ρ_g = [1−(1/m)Σs⁻/g_x]/[1+(1/s)Σs⁺/g_y]，CC 变换）。
 *          ERM：Russell 测度（io：min(1/m)Σθ_i；oo：1/max((1/s)Σφ_r)；
 *          non：图测度比值形 = SBM non（Tone 2001 恒等，委托 SbmModel）。
 */
#pragma once

#include "core/registry/solvable_model.h"

namespace opensuperdea::nonradial
{
/** @brief 加权加性族求解器（权重模式经 model_options 注入，见文件头）。 */
class AdditiveSlacksModel
{
  public:
    [[nodiscard]] static solver::LpProblem build_problem(const registry::SolveContext &context);
    [[nodiscard]] static result::DmuResult assemble_result(const registry::SolveContext &context,
                                                           const solver::SolveOutcome &outcome);
};

/** @brief 方向 SBM（DSBM，06 册 §4.4 #5；方向来源同 DDF 的 --direction）。 */
class DsbmModel
{
  public:
    [[nodiscard]] static solver::LpProblem build_problem(const registry::SolveContext &context);
    [[nodiscard]] static result::DmuResult assemble_result(const registry::SolveContext &context,
                                                           const solver::SolveOutcome &outcome);
};

/** @brief ERM 增强 Russell 测度（io/oo Russell；non 委托 SBM，见文件头）。 */
class ErmModel
{
  public:
    [[nodiscard]] static solver::LpProblem build_problem(const registry::SolveContext &context);
    [[nodiscard]] static result::DmuResult assemble_result(const registry::SolveContext &context,
                                                           const solver::SolveOutcome &outcome);
};
} // namespace opensuperdea::nonradial
