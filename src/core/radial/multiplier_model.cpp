/**
 * @file multiplier_model.cpp
 * @brief 乘子模型实现：对偶 LP 构造与乘子提取（FR-MULT-001/002、FR-OUT-005）。
 * @details 推导见 design_docs/06 册 §3.4 与本文件行注释；VRS 截距项 ω 仅在
 *          rts=vrs 时出现（自由变量）。效率语义与包络模型严格对偶（PR-06）。
 */
#include "multiplier_model.h"

#include <cmath>
#include <iomanip>
#include <sstream>

#include "core/registry/engine_error.h"

namespace opensuperdea::radial
{
namespace
{
using opensuperdea::registry::EngineException;
using opensuperdea::registry::SolveContext;
using opensuperdea::solver::LpProblem;
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

LpProblem MultiplierModel::build_problem(const SolveContext &context)
{
    const auto &data = *context.data;
    const std::size_t m = data.input_count();
    const std::size_t s = data.output_count();
    const std::size_t n = data.dmu_count();
    const bool is_input = context.options.orientation == registry::Orientation::INPUT;
    const bool is_vrs = context.options.rts == registry::Rts::VRS;

    LpProblem problem;
    // 变量布局：v(m) + u(s) + ω(仅 VRS，自由)
    problem.variable_count = m + s + (is_vrs ? 1U : 0U);
    const std::size_t omega_index = m + s;

    const auto &x0_row = data.inputs.row(as_index(context.dmu_index));
    const auto &y0_row = data.outputs.row(as_index(context.dmu_index));

    problem.is_minimization = !is_input;
    problem.objective.assign(problem.variable_count, 0.0);
    if (is_input)
    {
        // max u·y0 + ω（投入导向；ω 项仅 VRS）
        for (std::size_t r = 0; r < s; ++r)
        {
            problem.objective[m + r] = y0_row(as_index(r));
        }
        if (is_vrs)
        {
            problem.objective[omega_index] = 1.0;
        }
    }
    else
    {
        // min v·x0 − ω（输出导向；最优值 = φ*，效率 = 1/最优值）
        for (std::size_t i = 0; i < m; ++i)
        {
            problem.objective[i] = x0_row(as_index(i));
        }
        if (is_vrs)
        {
            problem.objective[omega_index] = -1.0;
        }
    }

    // 归一化行：投入导向 v·x0 = 1；产出导向 u·y0 = 1
    {
        const std::size_t row = problem.constraint_count++;
        problem.row_lower_bounds.push_back(1.0);
        problem.row_upper_bounds.push_back(1.0);
        for (std::size_t i = 0; i < m; ++i)
        {
            if (is_input)
            {
                problem.matrix.push_back({.row = row, .column = i, .value = x0_row(as_index(i))});
            }
        }
        for (std::size_t r = 0; r < s; ++r)
        {
            if (!is_input)
            {
                problem.matrix.push_back(
                    {.row = row, .column = m + r, .value = y0_row(as_index(r))});
            }
        }
    }

    // 逐 DMU 行：u·Y_j − v·X_j + ω ≤ 0（VRS）；两导向同一约束形式（推导见文件头）
    for (std::size_t j = 0; j < n; ++j)
    {
        const std::size_t row = problem.constraint_count++;
        problem.row_lower_bounds.push_back(NEGATIVE_INFINITY);
        problem.row_upper_bounds.push_back(0.0);
        for (std::size_t r = 0; r < s; ++r)
        {
            problem.matrix.push_back(
                {.row = row, .column = m + r, .value = data.outputs(as_index(j), as_index(r))});
        }
        for (std::size_t i = 0; i < m; ++i)
        {
            problem.matrix.push_back(
                {.row = row, .column = i, .value = -data.inputs(as_index(j), as_index(i))});
        }
        if (is_vrs)
        {
            problem.matrix.push_back({.row = row, .column = omega_index, .value = 1.0});
        }
    }

    // 变量界：v,u ≥ 0；ω 自由
    problem.column_lower_bounds.assign(problem.variable_count, 0.0);
    problem.column_upper_bounds.assign(problem.variable_count, POSITIVE_INFINITY);
    if (is_vrs)
    {
        problem.column_lower_bounds[omega_index] = NEGATIVE_INFINITY;
        problem.column_upper_bounds[omega_index] = POSITIVE_INFINITY;
    }
    return problem;
}

result::DmuResult MultiplierModel::assemble_result(const SolveContext &context,
                                                   const SolveOutcome &outcome)
{
    const auto &data = *context.data;
    const std::size_t m = data.input_count();
    const std::size_t s = data.output_count();
    const bool is_input = context.options.orientation == registry::Orientation::INPUT;
    const bool is_vrs = context.options.rts == registry::Rts::VRS;
    const std::size_t omega_index = m + s;

    result::DmuResult out;
    out.dmu_index = context.dmu_index;

    if (outcome.status != SolveStatus::OPTIMAL)
    {
        // 常规（非超效率）乘子形式恒可行（u 或 v 取零向量加截距吸收），不可行属内部错误
        throw EngineException(outcome.status == SolveStatus::INFEASIBLE ? 4 : 3, "solve",
                              "乘子 LP 求解失败（状态码 " +
                                  std::to_string(static_cast<int>(outcome.status)) + "）");
    }

    const double objective = outcome.objective_value;
    out.score = is_input ? objective : 1.0 / objective;
    if (is_input && out.score > 1.0 + K_UNIT_TOLERANCE)
    {
        // 对偶可行域上界为 1（包络 θ ≤ 1 的对偶），越界说明数值异常
        throw EngineException(4, "internal", "乘子效率 > 1（对偶一致性被破坏）");
    }
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

    // 最优乘子提取（FR-OUT-005）：v = 投入乘子，u = 产出乘子
    out.input_multipliers.assign(outcome.primal_solution.begin(),
                                 outcome.primal_solution.begin() + static_cast<std::ptrdiff_t>(m));
    out.output_multipliers.assign(outcome.primal_solution.begin() + static_cast<std::ptrdiff_t>(m),
                                  outcome.primal_solution.begin() +
                                      static_cast<std::ptrdiff_t>(m + s));
    if (is_vrs)
    {
        // 17 位有效数字（max_digits10）保证标注往返无损
        std::ostringstream stream;
        stream << std::setprecision(17) << outcome.primal_solution[omega_index];
        out.annotations.emplace("vrs_intercept", std::move(stream).str());
    }
    return out;
}
} // namespace opensuperdea::radial
