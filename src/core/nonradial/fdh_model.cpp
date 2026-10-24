/**
 * @file fdh_model.cpp
 * @brief FDH 实现0-1 整数支配 MILP 与状态映射（FR-FDH-001）。
 * @details LP 形式（06 册 §7）：投入导向 min θ s.t. Xλ ≤ θ·x0, Yλ ≥ y0, λ ∈ {0,1}；
 *          输出导向 max φ（效率 = 1/φ）。无 Σλ 约束（自由处置并集，无凸性）。
 */
#include "fdh_model.h"

#include <cmath>

#include "core/registry/engine_error.h"

namespace opensuperdea::nonradial
{
namespace
{
using opensuperdea::registry::EngineException;
using opensuperdea::registry::Orientation;
using opensuperdea::registry::SolveContext;
using opensuperdea::solver::MatrixTriplet;
using opensuperdea::solver::MilpProblem;
using opensuperdea::solver::NEGATIVE_INFINITY;
using opensuperdea::solver::POSITIVE_INFINITY;
using opensuperdea::solver::SolveOutcome;
using opensuperdea::solver::SolveStatus;

/** @brief 效率值 1 的判定容差。 */
constexpr double K_UNIT_TOLERANCE = 1e-9;

/** @brief size_t → Eigen 索引（显式转换，避免实现定义收窄）。 */
Eigen::Index as_index(std::size_t value)
{
    return static_cast<Eigen::Index>(value);
}
} // namespace

MilpProblem FdhModel::build_milp_problem(const SolveContext &context)
{
    const auto &data = *context.data;
    const std::size_t m = data.input_count();
    const std::size_t s = data.output_count();
    const std::size_t k = context.reference_indices.size();
    const bool is_input = context.options.orientation == Orientation::INPUT;

    const auto &x0_row = data.inputs.row(as_index(context.dmu_index));
    const auto &y0_row = data.outputs.row(as_index(context.dmu_index));

    // 变量布局：[测度(θ 或 φ), λ_k]
    MilpProblem problem;
    problem.variable_count = 1 + k;
    problem.is_minimization = is_input;
    problem.objective.assign(problem.variable_count, 0.0);
    problem.objective[0] = 1.0;

    if (is_input)
    {
        // Xλ ≤ θ·x0（m 行）；Yλ ≥ y0（s 行）
        for (std::size_t i = 0; i < m; ++i)
        {
            const std::size_t row = problem.constraint_count++;
            problem.row_lower_bounds.push_back(NEGATIVE_INFINITY);
            problem.row_upper_bounds.push_back(0.0);
            for (std::size_t j = 0; j < k; ++j)
            {
                problem.matrix.push_back(
                    {.row = row,
                     .column = 1 + j,
                     .value = data.inputs(as_index(context.reference_indices[j]), as_index(i))});
            }
            problem.matrix.push_back({.row = row, .column = 0, .value = -x0_row(as_index(i))});
        }
        for (std::size_t r = 0; r < s; ++r)
        {
            const std::size_t row = problem.constraint_count++;
            problem.row_lower_bounds.push_back(y0_row(as_index(r)));
            problem.row_upper_bounds.push_back(POSITIVE_INFINITY);
            for (std::size_t j = 0; j < k; ++j)
            {
                problem.matrix.push_back(
                    {.row = row,
                     .column = 1 + j,
                     .value = data.outputs(as_index(context.reference_indices[j]), as_index(r))});
            }
        }
    }
    else
    {
        // Yλ ≥ φ·y0（s 行）；Xλ ≤ x0（m 行）
        for (std::size_t r = 0; r < s; ++r)
        {
            const std::size_t row = problem.constraint_count++;
            problem.row_lower_bounds.push_back(0.0);
            problem.row_upper_bounds.push_back(POSITIVE_INFINITY);
            for (std::size_t j = 0; j < k; ++j)
            {
                problem.matrix.push_back(
                    {.row = row,
                     .column = 1 + j,
                     .value = data.outputs(as_index(context.reference_indices[j]), as_index(r))});
            }
            problem.matrix.push_back({.row = row, .column = 0, .value = -y0_row(as_index(r))});
        }
        for (std::size_t i = 0; i < m; ++i)
        {
            const std::size_t row = problem.constraint_count++;
            problem.row_lower_bounds.push_back(NEGATIVE_INFINITY);
            problem.row_upper_bounds.push_back(x0_row(as_index(i)));
            for (std::size_t j = 0; j < k; ++j)
            {
                problem.matrix.push_back(
                    {.row = row,
                     .column = 1 + j,
                     .value = data.inputs(as_index(context.reference_indices[j]), as_index(i))});
            }
        }
    }

    // 变量界：测度自由；λ ∈ {0,1}（自由处置支配：每个参考 DMU 入选与否）
    problem.column_lower_bounds = {NEGATIVE_INFINITY};
    problem.column_lower_bounds.resize(problem.variable_count, 0.0);
    problem.column_upper_bounds.assign(problem.variable_count, POSITIVE_INFINITY);
    problem.integrality.assign(problem.variable_count, 0);
    for (std::size_t j = 0; j < k; ++j)
    {
        problem.integrality[1 + j] = 1;
        problem.column_upper_bounds[1 + j] = 1.0;
    }
    return problem;
}

result::DmuResult FdhModel::assemble_result(const SolveContext &context,
                                            const SolveOutcome &outcome)
{
    const std::size_t k = context.reference_indices.size();
    const bool is_input = context.options.orientation == Orientation::INPUT;

    result::DmuResult out;
    out.dmu_index = context.dmu_index;

    if (outcome.status == SolveStatus::INFEASIBLE)
    {
        out.score = std::numeric_limits<double>::quiet_NaN();
        out.status = result::DmuStatus::INFEASIBLE;
        return out;
    }
    if (outcome.status != SolveStatus::OPTIMAL)
    {
        throw EngineException(3, "solve",
                              "FDH MILP 求解失败（状态码 " +
                                  std::to_string(static_cast<int>(outcome.status)) + "）");
    }

    const double measure = outcome.objective_value;
    out.score = is_input ? measure : 1.0 / measure;
    if (out.score > 1.0 + K_UNIT_TOLERANCE)
    {
        out.status = result::DmuStatus::SUPER_EFFICIENT;
    }
    else if (out.score >= 1.0 - K_UNIT_TOLERANCE)
    {
        out.status = result::DmuStatus::EFFICIENT;
    }
    else
    {
        out.status = result::DmuStatus::INEFFICIENT;
    }

    // λ 非零项（MILP 解）
    for (std::size_t j = 0; j < k; ++j)
    {
        if (std::abs(outcome.primal_solution[1 + j]) > 0.5) // 0-1 变量以 0.5 阈值判
        {
            out.lambdas.emplace_back(context.reference_indices[j], 1.0);
        }
    }
    return out;
}
} // namespace opensuperdea::nonradial
