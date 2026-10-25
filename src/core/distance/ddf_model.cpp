/**
 * @file ddf_model.cpp
 * @brief DDF 实现：方向构造（5 来源）与 max β LP（FR-DDF-001~003）。
 * @details RDM（Silva Portela 范围方向测度）：投入侧 g = x0 − min_j x（向最小观测收缩）、
 *          产出侧 g = max_j y − y0（向最大观测扩张）；零范围分量以 1 兜底避免除零。
 *          超效率（--super，exclude_self）下有效 DMU 可得 β < 0（记 super_efficient）。
 */
#include "ddf_model.h"

#include <cmath>

#include "core/registry/engine_error.h"

namespace opensuperdea::distance
{
namespace
{
using opensuperdea::registry::EngineException;
using opensuperdea::registry::Orientation;
using opensuperdea::registry::Rts;
using opensuperdea::registry::SolveContext;
using opensuperdea::solver::LpProblem;
using opensuperdea::solver::NEGATIVE_INFINITY;
using opensuperdea::solver::POSITIVE_INFINITY;
using opensuperdea::solver::SolveOutcome;
using opensuperdea::solver::SolveStatus;

/** @brief β ≈ 0（前沿上）的判定容差。 */
constexpr double K_ZERO_TOLERANCE = 1e-9;

/** @brief size_t → Eigen 索引（显式转换，避免实现定义收窄）。 */
Eigen::Index as_index(std::size_t value)
{
    return static_cast<Eigen::Index>(value);
}

/** @brief 方向来源解析（默认 own）。 */
std::string read_direction_kind(const SolveContext &context)
{
    const auto it = context.options.model_options.find("direction");
    if (it == context.options.model_options.end() || it->second.empty())
    {
        return "own";
    }
    const std::string kind = it->second;
    if (kind != "own" && kind != "mean" && kind != "unit" && kind != "custom" && kind != "rdm")
    {
        throw EngineException(1, "args", "非法方向来源 \"" + kind + "\"",
                              "可选 own|mean|unit|custom|rdm");
    }
    return kind;
}

/** @brief 构造方向向量 g = (g_x, g_y)（按定向裁剪：in 仅 g_x，out 仅 g_y）。 */
void build_directions(const SolveContext &context, const std::string &kind, Eigen::VectorXd &gx,
                      Eigen::VectorXd &gy)
{
    const auto &data = *context.data;
    const std::size_t m = data.input_count();
    const std::size_t s = data.output_count();
    const auto &x0 = data.inputs.row(as_index(context.dmu_index));
    const auto &y0 = data.outputs.row(as_index(context.dmu_index));

    gx.resize(as_index(m));
    gy.resize(as_index(s));
    if (kind == "own")
    {
        for (std::size_t i = 0; i < m; ++i)
        {
            gx(as_index(i)) = x0(as_index(i));
        }
        for (std::size_t r = 0; r < s; ++r)
        {
            gy(as_index(r)) = y0(as_index(r));
        }
    }
    else if (kind == "mean")
    {
        const Eigen::VectorXd mean_x = data.inputs.colwise().mean();
        const Eigen::VectorXd mean_y = data.outputs.colwise().mean();
        for (std::size_t i = 0; i < m; ++i)
        {
            gx(as_index(i)) = mean_x(as_index(i));
        }
        for (std::size_t r = 0; r < s; ++r)
        {
            gy(as_index(r)) = mean_y(as_index(r));
        }
    }
    else if (kind == "unit")
    {
        gx.setOnes();
        gy.setOnes();
    }
    else if (kind == "rdm")
    {
        // 观测范围方向（Silva Portela）：投入向最小观测收缩、产出向最大观测扩张
        const Eigen::VectorXd min_x = data.inputs.colwise().minCoeff();
        const Eigen::VectorXd max_y = data.outputs.colwise().maxCoeff();
        for (std::size_t i = 0; i < m; ++i)
        {
            gx(as_index(i)) = x0(as_index(i)) - min_x(as_index(i));
        }
        for (std::size_t r = 0; r < s; ++r)
        {
            gy(as_index(r)) = max_y(as_index(r)) - y0(as_index(r));
        }
    }
    else // custom
    {
        if (!data.directions.has_value())
        {
            throw EngineException(2, "data", "自定义方向需要 di_/do_ 方向列",
                                  "数据文件补充 di_*/do_* 前缀列（FR-IO-002）");
        }
        const auto &directions = *data.directions;
        for (std::size_t i = 0; i < m; ++i)
        {
            gx(as_index(i)) = directions.input_directions(as_index(context.dmu_index), as_index(i));
        }
        for (std::size_t r = 0; r < s; ++r)
        {
            gy(as_index(r)) =
                directions.output_directions(as_index(context.dmu_index), as_index(r));
        }
    }

    // 定向裁剪：in 仅投入侧、out 仅产出侧；非正方向分量以 1 兜底（避免零方向退化）
    const Orientation orientation = context.options.orientation;
    for (std::size_t i = 0; i < m; ++i)
    {
        if (orientation == Orientation::OUTPUT)
        {
            gx(as_index(i)) = 0.0;
        }
        else if (gx(as_index(i)) <= 0.0)
        {
            gx(as_index(i)) = 1.0; // 非正方向分量兜底（避免零方向退化）
        }
    }
    for (std::size_t r = 0; r < s; ++r)
    {
        if (orientation == Orientation::INPUT)
        {
            gy(as_index(r)) = 0.0;
        }
        else if (gy(as_index(r)) <= 0.0)
        {
            gy(as_index(r)) = 1.0;
        }
    }
}
} // namespace

LpProblem DdfModel::build_problem(const SolveContext &context)
{
    const auto &data = *context.data;
    const std::size_t m = data.input_count();
    const std::size_t s = data.output_count();
    const std::size_t k = context.reference_indices.size();
    const bool is_vrs = context.options.rts == Rts::VRS;

    Eigen::VectorXd gx;
    Eigen::VectorXd gy;
    build_directions(context, read_direction_kind(context), gx, gy);
    const auto &x0 = data.inputs.row(as_index(context.dmu_index));
    const auto &y0 = data.outputs.row(as_index(context.dmu_index));

    // 变量布局：[β, λ_k]；max β s.t. Xλ + β·g_x ≤ x0; Yλ − β·g_y ≥ y0
    LpProblem problem;
    problem.variable_count = 1 + k;
    problem.is_minimization = false;
    problem.objective.assign(problem.variable_count, 0.0);
    problem.objective[0] = 1.0;

    for (std::size_t i = 0; i < m; ++i)
    {
        const std::size_t row = problem.constraint_count++;
        problem.row_lower_bounds.push_back(NEGATIVE_INFINITY);
        problem.row_upper_bounds.push_back(x0(as_index(i)));
        for (std::size_t j = 0; j < k; ++j)
        {
            problem.matrix.push_back(
                {.row = row,
                 .column = 1 + j,
                 .value = data.inputs(as_index(context.reference_indices[j]), as_index(i))});
        }
        if (gx(as_index(i)) != 0.0)
        {
            problem.matrix.push_back({.row = row, .column = 0, .value = gx(as_index(i))});
        }
    }
    for (std::size_t r = 0; r < s; ++r)
    {
        const std::size_t row = problem.constraint_count++;
        problem.row_lower_bounds.push_back(y0(as_index(r)));
        problem.row_upper_bounds.push_back(POSITIVE_INFINITY);
        for (std::size_t j = 0; j < k; ++j)
        {
            problem.matrix.push_back(
                {.row = row,
                 .column = 1 + j,
                 .value = data.outputs(as_index(context.reference_indices[j]), as_index(r))});
        }
        if (gy(as_index(r)) != 0.0)
        {
            problem.matrix.push_back({.row = row, .column = 0, .value = -gy(as_index(r))});
        }
    }
    if (is_vrs)
    {
        const std::size_t row = problem.constraint_count++;
        problem.row_lower_bounds.push_back(1.0);
        problem.row_upper_bounds.push_back(1.0);
        for (std::size_t j = 0; j < k; ++j)
        {
            problem.matrix.push_back({.row = row, .column = 1 + j, .value = 1.0});
        }
    }

    // β 自由（超效率下可为负），λ ≥ 0
    problem.column_lower_bounds = {NEGATIVE_INFINITY};
    problem.column_lower_bounds.resize(problem.variable_count, 0.0);
    problem.column_upper_bounds.assign(problem.variable_count, POSITIVE_INFINITY);
    return problem;
}

result::DmuResult DdfModel::assemble_result(const SolveContext &context,
                                            const SolveOutcome &outcome)
{
    const auto &data = *context.data;
    const std::size_t k = context.reference_indices.size();

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
                              "DDF LP 求解失败（状态码 " +
                                  std::to_string(static_cast<int>(outcome.status)) + "）");
    }

    out.score = outcome.objective_value; // β
    if (out.score > K_ZERO_TOLERANCE)
    {
        out.status = result::DmuStatus::INEFFICIENT;
    }
    else if (out.score >= -K_ZERO_TOLERANCE)
    {
        out.status = result::DmuStatus::EFFICIENT;
    }
    else
    {
        // β < 0：被评 DMU 在 exclude_self 参考集前沿内部（超效率语义）
        out.status = result::DmuStatus::SUPER_EFFICIENT;
    }

    // λ 非零项（β 为变量 0，其后为 λ）
    for (std::size_t j = 0; j < k; ++j)
    {
        if (std::abs(outcome.primal_solution[1 + j]) > 1e-9)
        {
            out.lambdas.emplace_back(context.reference_indices[j], outcome.primal_solution[1 + j]);
        }
    }
    static_cast<void>(data);
    return out;
}
} // namespace opensuperdea::distance
