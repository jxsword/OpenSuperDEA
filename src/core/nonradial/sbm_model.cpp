/**
 * @file sbm_model.cpp
 * @brief SBM 模型族实现：CC 变换 LP / 双定向线性形式 / 加权 / 超效率（Tone 2001/2002）。
 * @details 数学形式（06 册 §4.1-§4.2，推导注记）：
 *          - 非导向 SBM：ρ = (1 − (1/m)Σs⁻/x0)/(1 + (1/s)Σs⁺/y0)，CC 变换
 *            （t = 1/(1 + (1/s)Σs⁺/y0)，λ'=tλ）后：
 *            min t − (1/m)Σs⁻'/x0 s.t. Xλ'+s⁻'=t·x0; Yλ'−s⁺'=t·y0; t+(1/s)Σs⁺'/y0=1。
 *          - 投入导向：ρ_in = 1 − (1/m)Σs⁻/x0（分母为 1，无需 CC）；
 *            产出导向：ρ_out = 1/(1 + (1/s)Σs⁺/y0)，CC 后 min t（t 即 ρ）。
 *          - 超效率（Tone 2002，非导向）：τ = t + (1/m)Σs⁻'/x0 s.t.
 *            Xλ'−s⁻' ≤ t·x0（投影在 DMU 外侧）; Yλ'+s⁺' ≥ t·y0; t−(1/s)Σs⁺'/y0 = 1。
 */
#include "sbm_model.h"

#include <cmath>

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

/** @brief 变体选项抽取。 */
struct SbmVariant
{
    bool is_super = false;
    bool is_weighted = false;
};

SbmVariant read_variant(const SolveContext &context)
{
    SbmVariant variant;
    const auto super_it = context.options.model_options.find("super");
    variant.is_super = super_it != context.options.model_options.end() && super_it->second == "1";
    const auto weight_it = context.options.model_options.find("weight");
    variant.is_weighted =
        weight_it != context.options.model_options.end() && weight_it->second == "1";
    return variant;
}

/** @brief 取加权 SBM 的侧权重（缺失权重数据报 EXIT_DATA）。 */
const Eigen::VectorXd &weight_side(const SolveContext &context, bool input_side)
{
    if (!context.data->weights.has_value())
    {
        throw EngineException(2, "data", "加权 SBM 需要 w_in_/w_out_ 权重列",
                              "数据文件补充 w_in_*/w_out_* 前缀列（和为 1）");
    }
    return input_side ? context.data->weights->input_weights
                      : context.data->weights->output_weights;
}
} // namespace

LpProblem SbmModel::build_problem(const SolveContext &context)
{
    const auto &data = *context.data;
    const std::size_t m = data.input_count();
    const std::size_t s = data.output_count();
    const std::size_t k = context.reference_indices.size();
    const Orientation orientation = context.options.orientation;
    const bool is_input = orientation == Orientation::INPUT;
    const bool is_output = orientation == Orientation::OUTPUT;
    const bool is_vrs = context.options.rts == Rts::VRS;
    const SbmVariant variant = read_variant(context);

    // 加权系数：加权 SBM 用数据集权重（和为 1）；标准 SBM 用均匀权重 1/m、1/s
    Eigen::VectorXd weight_in =
        Eigen::VectorXd::Constant(as_index(m), 1.0 / static_cast<double>(m));
    Eigen::VectorXd weight_out =
        Eigen::VectorXd::Constant(as_index(s), 1.0 / static_cast<double>(s));
    if (variant.is_weighted)
    {
        weight_in = weight_side(context, true);
        weight_out = weight_side(context, false);
    }

    // 变量布局统一为 [t, λ_k, s⁻_m, s⁺_s]；无 t 的变体：常规投入导向 / 定向超效率
    const std::size_t t_count = (is_input || (variant.is_super && is_output)) ? 0U : 1U;
    const std::size_t lambda_offset = t_count;
    const std::size_t slack_in_offset = lambda_offset + k;
    const std::size_t slack_out_offset = slack_in_offset + m;
    const std::size_t t_index = 0;

    const auto &x0_row = data.inputs.row(as_index(context.dmu_index));
    const auto &y0_row = data.outputs.row(as_index(context.dmu_index));

    LpProblem problem;
    problem.variable_count = t_count + k + m + s;

    if (is_input && !variant.is_super)
    {
        // 投入导向（含超效率外的线性形式）：min −Σ w_i·s⁻/x0（score = 1 + 最优值）
        problem.is_minimization = true;
        problem.objective.assign(problem.variable_count, 0.0);
        for (std::size_t i = 0; i < m; ++i)
        {
            problem.objective[slack_in_offset + i] = -weight_in(as_index(i)) / x0_row(as_index(i));
        }
        // Σλ X + s⁻ = x0（m 行等式）
        for (std::size_t i = 0; i < m; ++i)
        {
            const std::size_t row = problem.constraint_count++;
            problem.row_lower_bounds.push_back(x0_row(as_index(i)));
            problem.row_upper_bounds.push_back(x0_row(as_index(i)));
            for (std::size_t j = 0; j < k; ++j)
            {
                problem.matrix.push_back(
                    {.row = row,
                     .column = lambda_offset + j,
                     .value = data.inputs(as_index(context.reference_indices[j]), as_index(i))});
            }
            problem.matrix.push_back({.row = row, .column = slack_in_offset + i, .value = 1.0});
        }
        // Σλ Y ≥ y0（s 行）
        for (std::size_t r = 0; r < s; ++r)
        {
            const std::size_t row = problem.constraint_count++;
            problem.row_lower_bounds.push_back(y0_row(as_index(r)));
            problem.row_upper_bounds.push_back(POSITIVE_INFINITY);
            for (std::size_t j = 0; j < k; ++j)
            {
                problem.matrix.push_back(
                    {.row = row,
                     .column = lambda_offset + j,
                     .value = data.outputs(as_index(context.reference_indices[j]), as_index(r))});
            }
        }
    }
    else if (is_output && !variant.is_super)
    {
        // 产出导向：min t（t 即 ρ_out）
        problem.is_minimization = true;
        problem.objective.assign(problem.variable_count, 0.0);
        problem.objective[t_index] = 1.0;
        // Σλ X − t·x0 ≤ 0（m 行）
        for (std::size_t i = 0; i < m; ++i)
        {
            const std::size_t row = problem.constraint_count++;
            problem.row_lower_bounds.push_back(NEGATIVE_INFINITY);
            problem.row_upper_bounds.push_back(0.0);
            for (std::size_t j = 0; j < k; ++j)
            {
                problem.matrix.push_back(
                    {.row = row,
                     .column = lambda_offset + j,
                     .value = data.inputs(as_index(context.reference_indices[j]), as_index(i))});
            }
            problem.matrix.push_back(
                {.row = row, .column = t_index, .value = -x0_row(as_index(i))});
        }
        // Σλ Y − s⁺ = t·y0（s 行等式）
        for (std::size_t r = 0; r < s; ++r)
        {
            const std::size_t row = problem.constraint_count++;
            problem.row_lower_bounds.push_back(0.0);
            problem.row_upper_bounds.push_back(0.0);
            for (std::size_t j = 0; j < k; ++j)
            {
                problem.matrix.push_back(
                    {.row = row,
                     .column = lambda_offset + j,
                     .value = data.outputs(as_index(context.reference_indices[j]), as_index(r))});
            }
            problem.matrix.push_back({.row = row, .column = slack_out_offset + r, .value = -1.0});
            problem.matrix.push_back(
                {.row = row, .column = t_index, .value = -y0_row(as_index(r))});
        }
        // t + Σ w_r·s⁺/y0 = 1
        {
            const std::size_t row = problem.constraint_count++;
            problem.row_lower_bounds.push_back(1.0);
            problem.row_upper_bounds.push_back(1.0);
            problem.matrix.push_back({.row = row, .column = t_index, .value = 1.0});
            for (std::size_t r = 0; r < s; ++r)
            {
                problem.matrix.push_back({.row = row,
                                          .column = slack_out_offset + r,
                                          .value = weight_out(as_index(r)) / y0_row(as_index(r))});
            }
        }
    }
    else if (!variant.is_super)
    {
        // 非导向（标准图形测度）：min t − Σ w_i·s⁻/x0
        problem.is_minimization = true;
        problem.objective.assign(problem.variable_count, 0.0);
        problem.objective[t_index] = 1.0;
        for (std::size_t i = 0; i < m; ++i)
        {
            problem.objective[slack_in_offset + i] = -weight_in(as_index(i)) / x0_row(as_index(i));
        }
        // Σλ X + s⁻ = t·x0
        for (std::size_t i = 0; i < m; ++i)
        {
            const std::size_t row = problem.constraint_count++;
            problem.row_lower_bounds.push_back(0.0);
            problem.row_upper_bounds.push_back(0.0);
            for (std::size_t j = 0; j < k; ++j)
            {
                problem.matrix.push_back(
                    {.row = row,
                     .column = lambda_offset + j,
                     .value = data.inputs(as_index(context.reference_indices[j]), as_index(i))});
            }
            problem.matrix.push_back({.row = row, .column = slack_in_offset + i, .value = 1.0});
            problem.matrix.push_back(
                {.row = row, .column = t_index, .value = -x0_row(as_index(i))});
        }
        // Σλ Y − s⁺ = t·y0
        for (std::size_t r = 0; r < s; ++r)
        {
            const std::size_t row = problem.constraint_count++;
            problem.row_lower_bounds.push_back(0.0);
            problem.row_upper_bounds.push_back(0.0);
            for (std::size_t j = 0; j < k; ++j)
            {
                problem.matrix.push_back(
                    {.row = row,
                     .column = lambda_offset + j,
                     .value = data.outputs(as_index(context.reference_indices[j]), as_index(r))});
            }
            problem.matrix.push_back({.row = row, .column = slack_out_offset + r, .value = -1.0});
            problem.matrix.push_back(
                {.row = row, .column = t_index, .value = -y0_row(as_index(r))});
        }
        // t + Σ w_r·s⁺/y0 = 1
        {
            const std::size_t row = problem.constraint_count++;
            problem.row_lower_bounds.push_back(1.0);
            problem.row_upper_bounds.push_back(1.0);
            problem.matrix.push_back({.row = row, .column = t_index, .value = 1.0});
            for (std::size_t r = 0; r < s; ++r)
            {
                problem.matrix.push_back({.row = row,
                                          .column = slack_out_offset + r,
                                          .value = weight_out(as_index(r)) / y0_row(as_index(r))});
            }
        }
    }
    else if (variant.is_super && is_input)
    {
        // 投入导向超效率：min Σ w_i·s⁻/x0（score = 1 + 最优值；Tone 2002 定向形式）
        // Xλ − s⁻ ≤ x0（投入外侧扩张），Yλ ≥ y0（产出保持）
        problem.is_minimization = true;
        problem.objective.assign(problem.variable_count, 0.0);
        for (std::size_t i = 0; i < m; ++i)
        {
            problem.objective[slack_in_offset + i] = weight_in(as_index(i)) / x0_row(as_index(i));
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
                     .column = lambda_offset + j,
                     .value = data.inputs(as_index(context.reference_indices[j]), as_index(i))});
            }
            problem.matrix.push_back({.row = row, .column = slack_in_offset + i, .value = -1.0});
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
                     .column = lambda_offset + j,
                     .value = data.outputs(as_index(context.reference_indices[j]), as_index(r))});
            }
        }
    }
    else if (variant.is_super && is_output)
    {
        // 产出导向超效率：min Σ w_r·s⁺/y0（score = 1/(1 − 最优值)）
        // Yλ + s⁺ ≥ y0（产出内侧收缩），Xλ ≤ x0（投入保持）
        problem.is_minimization = true;
        problem.objective.assign(problem.variable_count, 0.0);
        for (std::size_t r = 0; r < s; ++r)
        {
            problem.objective[slack_out_offset + r] = weight_out(as_index(r)) / y0_row(as_index(r));
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
                     .column = lambda_offset + j,
                     .value = data.inputs(as_index(context.reference_indices[j]), as_index(i))});
            }
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
                     .column = lambda_offset + j,
                     .value = data.outputs(as_index(context.reference_indices[j]), as_index(r))});
            }
            problem.matrix.push_back({.row = row, .column = slack_out_offset + r, .value = 1.0});
        }
    }
    else
    {
        // 非导向超效率（Tone 2002）：τ = t + Σ w_i·s⁻/x0 s.t. Xλ−s⁻ ≤ t·x0;
        // Yλ+s⁺ ≥ t·y0; t − Σ w_r·s⁺/y0 = 1（deaR model_sbmsupereff no 口径同值）
        problem.is_minimization = true;
        problem.objective.assign(problem.variable_count, 0.0);
        problem.objective[t_index] = 1.0;
        for (std::size_t i = 0; i < m; ++i)
        {
            problem.objective[slack_in_offset + i] = weight_in(as_index(i)) / x0_row(as_index(i));
        }
        // Xλ − s⁻ ≤ t·x0（投入外侧扩张）
        for (std::size_t i = 0; i < m; ++i)
        {
            const std::size_t row = problem.constraint_count++;
            problem.row_lower_bounds.push_back(NEGATIVE_INFINITY);
            problem.row_upper_bounds.push_back(0.0);
            for (std::size_t j = 0; j < k; ++j)
            {
                problem.matrix.push_back(
                    {.row = row,
                     .column = lambda_offset + j,
                     .value = data.inputs(as_index(context.reference_indices[j]), as_index(i))});
            }
            problem.matrix.push_back({.row = row, .column = slack_in_offset + i, .value = -1.0});
            problem.matrix.push_back(
                {.row = row, .column = t_index, .value = -x0_row(as_index(i))});
        }
        // Yλ + s⁺ ≥ t·y0（产出内侧收缩）
        for (std::size_t r = 0; r < s; ++r)
        {
            const std::size_t row = problem.constraint_count++;
            problem.row_lower_bounds.push_back(0.0);
            problem.row_upper_bounds.push_back(POSITIVE_INFINITY);
            for (std::size_t j = 0; j < k; ++j)
            {
                problem.matrix.push_back(
                    {.row = row,
                     .column = lambda_offset + j,
                     .value = data.outputs(as_index(context.reference_indices[j]), as_index(r))});
            }
            problem.matrix.push_back({.row = row, .column = slack_out_offset + r, .value = 1.0});
            problem.matrix.push_back(
                {.row = row, .column = t_index, .value = -y0_row(as_index(r))});
        }
        if (!is_input)
        {
            // t − Σ w_r·s⁺/y0 = 1（产出侧参与归一）
            const std::size_t row = problem.constraint_count++;
            problem.row_lower_bounds.push_back(1.0);
            problem.row_upper_bounds.push_back(1.0);
            problem.matrix.push_back({.row = row, .column = t_index, .value = 1.0});
            for (std::size_t r = 0; r < s; ++r)
            {
                problem.matrix.push_back({.row = row,
                                          .column = slack_out_offset + r,
                                          .value = -weight_out(as_index(r)) / y0_row(as_index(r))});
            }
        }
    }

    // VRS：Σλ = t（无 t 的线性形式为 Σλ = 1）
    if (is_vrs)
    {
        const std::size_t row = problem.constraint_count++;
        const double bound = t_count == 1 ? 0.0 : 1.0;
        problem.row_lower_bounds.push_back(bound);
        problem.row_upper_bounds.push_back(bound);
        for (std::size_t j = 0; j < k; ++j)
        {
            problem.matrix.push_back({.row = row, .column = lambda_offset + j, .value = 1.0});
        }
        if (t_count == 1)
        {
            problem.matrix.push_back({.row = row, .column = t_index, .value = -1.0});
        }
    }

    // 变量界：t ≥ 0（CC 缩放变量），λ/s ≥ 0
    problem.column_lower_bounds.assign(problem.variable_count, 0.0);
    problem.column_upper_bounds.assign(problem.variable_count, POSITIVE_INFINITY);
    return problem;
}

result::DmuResult SbmModel::assemble_result(const SolveContext &context,
                                            const SolveOutcome &outcome)
{
    const auto &data = *context.data;
    const std::size_t m = data.input_count();
    const std::size_t s = data.output_count();
    const std::size_t k = context.reference_indices.size();
    const Orientation orientation = context.options.orientation;
    const SbmVariant variant = read_variant(context);
    const std::size_t t_count = (orientation == Orientation::INPUT ||
                                 (variant.is_super && orientation == Orientation::OUTPUT))
                                    ? 0U
                                    : 1U;
    const std::size_t lambda_offset = t_count;
    const std::size_t slack_in_offset = t_count + k;
    const std::size_t slack_out_offset = slack_in_offset + m;

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
                              "SBM LP 求解失败（状态码 " +
                                  std::to_string(static_cast<int>(outcome.status)) + "）");
    }

    const auto &solution = outcome.primal_solution;
    const double objective = outcome.objective_value;
    // 常规投入导向与投入导向超效率同式：score = 1 ± Σ w·s⁻/x0（合并分支，消除克隆）
    if (orientation == Orientation::INPUT)
    {
        out.score = 1.0 + objective;
    }
    else if (variant.is_super && orientation == Orientation::OUTPUT)
    {
        // min Σ w·s⁺/y0 → score = 1/(1 − Σ w·s⁺/y0)；分母趋零视为不可行
        if (objective >= 1.0 - K_UNIT_TOLERANCE)
        {
            out.score = std::numeric_limits<double>::quiet_NaN();
            out.status = result::DmuStatus::INFEASIBLE;
            return out;
        }
        out.score = 1.0 / (1.0 - objective);
    }
    else
    {
        out.score = objective; // t（产出/非导向）或 τ（非导向超效率）
    }
    if (!variant.is_super)
    {
        // 常规 SBM：score ∈ [0,1]；>1 属数值异常
        if (out.score > 1.0 + K_UNIT_TOLERANCE)
        {
            throw EngineException(4, "internal", "SBM 效率 > 1（数值异常）");
        }
        out.status = out.score >= 1.0 - K_UNIT_TOLERANCE ? result::DmuStatus::EFFICIENT
                                                         : result::DmuStatus::INEFFICIENT;
    }
    else
    {
        out.status = out.score > 1.0 + K_UNIT_TOLERANCE ? result::DmuStatus::SUPER_EFFICIENT
                                                        : result::DmuStatus::INEFFICIENT;
    }

    // λ 与松弛提取（列布局同 build_problem）
    for (std::size_t j = 0; j < k; ++j)
    {
        if (std::abs(solution[lambda_offset + j]) > 1e-9)
        {
            out.lambdas.emplace_back(context.reference_indices[j], solution[lambda_offset + j]);
        }
    }
    if (t_count + k + m + s == solution.size())
    {
        out.input_slacks.resize(m);
        out.output_slacks.resize(s);
        for (std::size_t i = 0; i < m; ++i)
        {
            out.input_slacks[i] = solution[slack_in_offset + i];
        }
        for (std::size_t r = 0; r < s; ++r)
        {
            out.output_slacks[r] = solution[slack_out_offset + r];
        }
    }
    return out;
}
} // namespace opensuperdea::nonradial
