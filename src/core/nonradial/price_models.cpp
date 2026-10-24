/**
 * @file price_models.cpp
 * @brief 价格族模型实现：成本/收益/利润 LP 与效率比值（06 册 §6）。
 * @details 共用骨架：最优解点（x* / y*）作为投影输出；RTS 约束经
 *          append_rts_row 语义（成本/收益/利润 LP 中以 Σλ 界实现）。
 */
#include "price_models.h"

#include <cmath>

#include "core/radial/radial_envelope_model.h"
#include "core/registry/engine_error.h"

namespace opensuperdea::nonradial
{
namespace
{
using opensuperdea::registry::EngineException;
using opensuperdea::registry::Orientation;
using opensuperdea::registry::Rts;
using opensuperdea::registry::SolveContext;
using opensuperdea::solver::LpProblem;
using opensuperdea::solver::MatrixTriplet;
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

/** @brief 取被评 DMU 的价格行（缺失价格数据报 EXIT_DATA）。 */
Eigen::VectorXd price_row(const SolveContext &context, bool input_side)
{
    const auto &data = *context.data;
    if (!data.prices.has_value())
    {
        throw EngineException(2, "data", "价格族模型需要 pi_/po_ 价格列",
                              "数据文件补充 pi_*/po_* 前缀列（FR-IO-003）");
    }
    const Eigen::VectorXd row =
        input_side ? Eigen::VectorXd(data.prices->input_prices.row(as_index(context.dmu_index)))
                   : Eigen::VectorXd(data.prices->output_prices.row(as_index(context.dmu_index)));
    if (row.size() == 0)
    {
        throw EngineException(2, "data",
                              input_side ? "缺少 pi_ 投入价格列" : "缺少 po_ 产出价格列");
    }
    return row;
}

/** @brief RTS 约束行（VRS: Σλ=1；NIRS: ≤1；NDRS: ≥1；CRS: 无）。 */
void append_rts_row(Rts rts, std::size_t lambda_offset, std::size_t lambda_count,
                    LpProblem &problem)
{
    if (rts == Rts::CRS)
    {
        return;
    }
    const std::size_t row = problem.constraint_count++;
    const bool lower_one = rts == Rts::VRS || rts == Rts::NDRS;
    const bool upper_one = rts == Rts::VRS || rts == Rts::NIRS;
    problem.row_lower_bounds.push_back(lower_one ? 1.0 : NEGATIVE_INFINITY);
    problem.row_upper_bounds.push_back(upper_one ? 1.0 : POSITIVE_INFINITY);
    for (std::size_t j = 0; j < lambda_count; ++j)
    {
        problem.matrix.push_back({.row = row, .column = lambda_offset + j, .value = 1.0});
    }
}
} // namespace

LpProblem CostModel::build_problem(const SolveContext &context)
{
    const auto &data = *context.data;
    const std::size_t m = data.input_count();
    const std::size_t s = data.output_count();
    const std::size_t k = context.reference_indices.size();
    const Eigen::VectorXd w = price_row(context, true);
    const auto &y0_row = data.outputs.row(as_index(context.dmu_index));

    // 变量布局：[x_m, λ_k]；min w·x s.t. Yλ ≥ y0; Xλ − x ≤ 0; x,λ ≥ 0 (+RTS)
    LpProblem problem;
    problem.variable_count = m + k;
    problem.is_minimization = true;
    problem.objective.assign(problem.variable_count, 0.0);
    for (std::size_t i = 0; i < m; ++i)
    {
        problem.objective[i] = w(as_index(i));
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
                 .column = m + j,
                 .value = data.outputs(as_index(context.reference_indices[j]), as_index(r))});
        }
    }
    for (std::size_t i = 0; i < m; ++i)
    {
        const std::size_t row = problem.constraint_count++;
        problem.row_lower_bounds.push_back(NEGATIVE_INFINITY);
        problem.row_upper_bounds.push_back(0.0);
        for (std::size_t j = 0; j < k; ++j)
        {
            problem.matrix.push_back(
                {.row = row,
                 .column = m + j,
                 .value = data.inputs(as_index(context.reference_indices[j]), as_index(i))});
        }
        problem.matrix.push_back({.row = row, .column = i, .value = -1.0});
    }
    append_rts_row(context.options.rts, m, k, problem);
    problem.column_lower_bounds.assign(problem.variable_count, 0.0);
    problem.column_upper_bounds.assign(problem.variable_count, POSITIVE_INFINITY);
    return problem;
}

result::DmuResult CostModel::assemble_result(const SolveContext &context,
                                             const SolveOutcome &outcome)
{
    const auto &data = *context.data;
    const std::size_t m = data.input_count();
    const Eigen::VectorXd w = price_row(context, true);
    const auto &x0_row = data.inputs.row(as_index(context.dmu_index));

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
        throw EngineException(3, "solve", "成本最小化 LP 求解失败");
    }

    const double actual_cost = w.dot(x0_row);
    const double min_cost = outcome.objective_value;
    if (min_cost <= 0.0)
    {
        throw EngineException(4, "internal", "最小成本非正（价格或数据退化）");
    }
    // 标准成本效率 = 最小成本 / 实际成本 ≤ 1（deaR 同口径；06 册 §6 公式分子分母
    // 顺序为笔误，按教科书定义实现——04 册 L1 对拍以 deaR 为准）
    out.score = min_cost / actual_cost;
    out.status = out.score >= 1.0 - K_UNIT_TOLERANCE ? result::DmuStatus::EFFICIENT
                                                     : result::DmuStatus::INEFFICIENT;

    // 投影 = 最优投入点 x*；标注：最小成本 + 技术/配置分解（TE = 径向 θ，AE = CE/TE）
    out.input_targets.assign(outcome.primal_solution.begin(),
                             outcome.primal_solution.begin() + static_cast<std::ptrdiff_t>(m));
    out.annotations.emplace("min_cost", std::to_string(min_cost));

    // 技术效率（径向包络，同 RTS）→ 配置效率 AE = CE / TE（FR-COST-001 分解）
    opensuperdea::radial::RadialEnvelopeModel radial;
    const solver::LpProblem radial_problem = radial.build_problem(context);
    const solver::SolveOutcome radial_outcome = opensuperdea::adapters::solve_lp(radial_problem);
    if (radial_outcome.status == SolveStatus::OPTIMAL)
    {
        const double te = radial_outcome.objective_value;
        if (te > 0.0)
        {
            out.annotations.emplace("te", std::to_string(te));
            out.annotations.emplace("ae", std::to_string(out.score / te));
        }
    }
    return out;
}

LpProblem RevenueModel::build_problem(const SolveContext &context)
{
    const auto &data = *context.data;
    const std::size_t m = data.input_count();
    const std::size_t s = data.output_count();
    const std::size_t k = context.reference_indices.size();
    const Eigen::VectorXd p = price_row(context, false);
    const auto &x0_row = data.inputs.row(as_index(context.dmu_index));

    // 变量布局：[y_s, λ_k]；max p·y s.t. Xλ ≤ x0; Yλ − y ≤ 0; y,λ ≥ 0 (+RTS)
    LpProblem problem;
    problem.variable_count = s + k;
    problem.is_minimization = false;
    problem.objective.assign(problem.variable_count, 0.0);
    for (std::size_t r = 0; r < s; ++r)
    {
        problem.objective[r] = p(as_index(r));
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
                 .column = s + j,
                 .value = data.inputs(as_index(context.reference_indices[j]), as_index(i))});
        }
    }
    for (std::size_t r = 0; r < s; ++r)
    {
        const std::size_t row = problem.constraint_count++;
        problem.row_lower_bounds.push_back(NEGATIVE_INFINITY);
        problem.row_upper_bounds.push_back(0.0);
        for (std::size_t j = 0; j < k; ++j)
        {
            problem.matrix.push_back(
                {.row = row,
                 .column = s + j,
                 .value = -data.outputs(as_index(context.reference_indices[j]), as_index(r))});
        }
        problem.matrix.push_back({.row = row, .column = r, .value = 1.0});
    }
    append_rts_row(context.options.rts, s, k, problem);
    problem.column_lower_bounds.assign(problem.variable_count, 0.0);
    problem.column_upper_bounds.assign(problem.variable_count, POSITIVE_INFINITY);
    return problem;
}

result::DmuResult RevenueModel::assemble_result(const SolveContext &context,
                                                const SolveOutcome &outcome)
{
    const auto &data = *context.data;
    const std::size_t s = data.output_count();
    const Eigen::VectorXd p = price_row(context, false);
    const auto &y0_row = data.outputs.row(as_index(context.dmu_index));

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
        throw EngineException(3, "solve", "收益最大化 LP 求解失败");
    }

    const double actual_revenue = p.dot(y0_row);
    const double max_revenue = outcome.objective_value;
    if (max_revenue <= 0.0)
    {
        throw EngineException(4, "internal", "最大收益非正（价格或数据退化）");
    }
    out.score = actual_revenue / max_revenue;
    out.status = out.score >= 1.0 - K_UNIT_TOLERANCE ? result::DmuStatus::EFFICIENT
                                                     : result::DmuStatus::INEFFICIENT;
    out.output_targets.assign(outcome.primal_solution.begin(),
                              outcome.primal_solution.begin() + static_cast<std::ptrdiff_t>(s));
    out.annotations.emplace("max_revenue", std::to_string(max_revenue));
    return out;
}

LpProblem ProfitModel::build_problem(const SolveContext &context)
{
    const auto &data = *context.data;
    const std::size_t m = data.input_count();
    const std::size_t s = data.output_count();
    const std::size_t k = context.reference_indices.size();
    const Eigen::VectorXd w = price_row(context, true);
    const Eigen::VectorXd p = price_row(context, false);

    // 变量布局：[x_m, y_s, λ_k]；max p·y − w·x s.t. Xλ − x ≤ 0; Yλ − y ≥ 0 (+RTS)
    LpProblem problem;
    problem.variable_count = m + s + k;
    problem.is_minimization = false;
    problem.objective.assign(problem.variable_count, 0.0);
    for (std::size_t i = 0; i < m; ++i)
    {
        problem.objective[i] = -w(as_index(i));
    }
    for (std::size_t r = 0; r < s; ++r)
    {
        problem.objective[m + r] = p(as_index(r));
    }
    for (std::size_t i = 0; i < m; ++i)
    {
        const std::size_t row = problem.constraint_count++;
        problem.row_lower_bounds.push_back(NEGATIVE_INFINITY);
        problem.row_upper_bounds.push_back(0.0);
        for (std::size_t j = 0; j < k; ++j)
        {
            problem.matrix.push_back(
                {.row = row,
                 .column = m + s + j,
                 .value = data.inputs(as_index(context.reference_indices[j]), as_index(i))});
        }
        problem.matrix.push_back({.row = row, .column = i, .value = -1.0});
    }
    for (std::size_t r = 0; r < s; ++r)
    {
        const std::size_t row = problem.constraint_count++;
        problem.row_lower_bounds.push_back(0.0);
        problem.row_upper_bounds.push_back(POSITIVE_INFINITY);
        for (std::size_t j = 0; j < k; ++j)
        {
            problem.matrix.push_back(
                {.row = row,
                 .column = m + s + j,
                 .value = data.outputs(as_index(context.reference_indices[j]), as_index(r))});
        }
        problem.matrix.push_back({.row = row, .column = m + r, .value = -1.0});
    }
    append_rts_row(context.options.rts, m + s, k, problem);
    problem.column_lower_bounds.assign(problem.variable_count, 0.0);
    problem.column_upper_bounds.assign(problem.variable_count, POSITIVE_INFINITY);
    return problem;
}

result::DmuResult ProfitModel::assemble_result(const SolveContext &context,
                                               const SolveOutcome &outcome)
{
    const auto &data = *context.data;
    const std::size_t m = data.input_count();
    const std::size_t s = data.output_count();
    const Eigen::VectorXd w = price_row(context, true);
    const Eigen::VectorXd p = price_row(context, false);
    const auto &x0_row = data.inputs.row(as_index(context.dmu_index));
    const auto &y0_row = data.outputs.row(as_index(context.dmu_index));

    result::DmuResult out;
    out.dmu_index = context.dmu_index;
    if (outcome.status == SolveStatus::UNBOUNDED)
    {
        // CRS 下正单位利润沿射线放大 → 无界（教科书已知边界；建议 VRS）
        out.score = std::numeric_limits<double>::quiet_NaN();
        out.status = result::DmuStatus::INFEASIBLE;
        out.annotations.emplace("note", "profit_unbounded_crs");
        return out;
    }
    if (outcome.status == SolveStatus::INFEASIBLE)
    {
        out.score = std::numeric_limits<double>::quiet_NaN();
        out.status = result::DmuStatus::INFEASIBLE;
        return out;
    }
    if (outcome.status != SolveStatus::OPTIMAL)
    {
        throw EngineException(3, "solve", "利润最大化 LP 求解失败");
    }

    const double actual_profit = p.dot(y0_row) - w.dot(x0_row);
    const double max_profit = outcome.objective_value;
    if (max_profit <= K_UNIT_TOLERANCE)
    {
        // 最大利润非正：PE 分母非正，效率值无教科书意义（02 册状态枚举外情形）
        out.score = std::numeric_limits<double>::quiet_NaN();
        out.status = result::DmuStatus::INFEASIBLE;
        out.annotations.emplace("note", "max_profit_nonpositive");
        return out;
    }
    out.score = actual_profit / max_profit;
    out.status = out.score >= 1.0 - K_UNIT_TOLERANCE ? result::DmuStatus::EFFICIENT
                                                     : result::DmuStatus::INEFFICIENT;
    const auto &solution = outcome.primal_solution;
    out.input_targets.assign(solution.begin(), solution.begin() + static_cast<std::ptrdiff_t>(m));
    out.output_targets.assign(solution.begin() + static_cast<std::ptrdiff_t>(m),
                              solution.begin() + static_cast<std::ptrdiff_t>(m + s));
    out.annotations.emplace("max_profit", std::to_string(max_profit));
    return out;
}
} // namespace opensuperdea::nonradial
