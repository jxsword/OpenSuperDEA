/**
 * @file additive_models.cpp
 * @brief 加权加性族 / DSBM / ERM 实现（P-006 清单，见 additive_models.h 文件头）。
 */
#include "additive_models.h"

#include <cmath>

#include "core/registry/engine_error.h"
#include "sbm_model.h"

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

/** @brief 效率值 0（加性有效）的判定容差。 */
constexpr double K_ZERO_TOLERANCE = 1e-9;

/** @brief size_t → Eigen 索引（显式转换，避免实现定义收窄）。 */
Eigen::Index as_index(std::size_t value)
{
    return static_cast<Eigen::Index>(value);
}

/** @brief RTS 约束行（Σλ 界）。 */
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

/** @brief 加权加性族的权重向量（模式经 model_options["weight_mode"]，默认 raw）。 */
void additive_weights(const SolveContext &context, Eigen::VectorXd &win, Eigen::VectorXd &wout)
{
    const auto &data = *context.data;
    const std::size_t m = data.input_count();
    const std::size_t s = data.output_count();
    const auto mode_it = context.options.model_options.find("weight_mode");
    const std::string mode =
        mode_it != context.options.model_options.end() && !mode_it->second.empty() ? mode_it->second
                                                                                   : "raw";

    win.resize(as_index(m));
    wout.resize(as_index(s));
    if (mode == "raw")
    {
        win.setOnes();
        wout.setOnes();
        return;
    }
    if (mode == "mean")
    {
        // 归一化加权加性（Lovell-Pastor 1995）：以列均值归一
        const Eigen::VectorXd mean_x = data.inputs.colwise().mean();
        const Eigen::VectorXd mean_y = data.outputs.colwise().mean();
        for (std::size_t i = 0; i < m; ++i)
        {
            win(as_index(i)) = mean_x(as_index(i)) > 0.0 ? 1.0 / mean_x(as_index(i)) : 1.0;
        }
        for (std::size_t r = 0; r < s; ++r)
        {
            wout(as_index(r)) = mean_y(as_index(r)) > 0.0 ? 1.0 / mean_y(as_index(r)) : 1.0;
        }
        return;
    }
    if (mode == "range" || mode == "bam")
    {
        // RAM（Cooper-Pastor-Tone 1997）/ BAM（M2 默认界 = 数据范围；用户界留 M5）
        const Eigen::VectorXd min_x = data.inputs.colwise().minCoeff();
        const Eigen::VectorXd max_x = data.inputs.colwise().maxCoeff();
        const Eigen::VectorXd min_y = data.outputs.colwise().minCoeff();
        const Eigen::VectorXd max_y = data.outputs.colwise().maxCoeff();
        for (std::size_t i = 0; i < m; ++i)
        {
            const double range = max_x(as_index(i)) - min_x(as_index(i));
            win(as_index(i)) = range > 0.0 ? 1.0 / range : 1.0; // 零范围兜底（常数列）
        }
        for (std::size_t r = 0; r < s; ++r)
        {
            const double range = max_y(as_index(r)) - min_y(as_index(r));
            wout(as_index(r)) = range > 0.0 ? 1.0 / range : 1.0;
        }
        return;
    }
    if (mode == "user")
    {
        // wADD（Lovell-Pastor 一般权）：w_in_/w_out_ 列（和为 1，V-010 前置）
        if (!data.weights.has_value())
        {
            throw EngineException(2, "data", "wADD 需要 w_in_/w_out_ 权重列",
                                  "数据文件补充 w_in_*/w_out_* 前缀列（和为 1）");
        }
        win = data.weights->input_weights;
        wout = data.weights->output_weights;
        return;
    }
    throw EngineException(1, "args", "非法加性权重模式 \"" + mode + "\"",
                          "可选 raw|mean|range|bam|user");
}
} // namespace

LpProblem AdditiveSlacksModel::build_problem(const SolveContext &context)
{
    const auto &data = *context.data;
    const std::size_t m = data.input_count();
    const std::size_t s = data.output_count();
    const std::size_t k = context.reference_indices.size();
    Eigen::VectorXd win;
    Eigen::VectorXd wout;
    additive_weights(context, win, wout);
    const auto &x0_row = data.inputs.row(as_index(context.dmu_index));
    const auto &y0_row = data.outputs.row(as_index(context.dmu_index));

    // 变量布局：[λ_k, s⁻_m, s⁺_s]；max Σw·s⁻ + Σw·s⁺ s.t. Xλ+s⁻=x0; Yλ−s⁺=y0 (+RTS)
    LpProblem problem;
    problem.variable_count = k + m + s;
    problem.is_minimization = false;
    problem.objective.assign(problem.variable_count, 0.0);
    for (std::size_t i = 0; i < m; ++i)
    {
        problem.objective[k + i] = win(as_index(i));
    }
    for (std::size_t r = 0; r < s; ++r)
    {
        problem.objective[k + m + r] = wout(as_index(r));
    }
    for (std::size_t i = 0; i < m; ++i)
    {
        const std::size_t row = problem.constraint_count++;
        problem.row_lower_bounds.push_back(x0_row(as_index(i)));
        problem.row_upper_bounds.push_back(x0_row(as_index(i)));
        for (std::size_t j = 0; j < k; ++j)
        {
            problem.matrix.push_back(
                {.row = row,
                 .column = j,
                 .value = data.inputs(as_index(context.reference_indices[j]), as_index(i))});
        }
        problem.matrix.push_back({.row = row, .column = k + i, .value = 1.0});
    }
    for (std::size_t r = 0; r < s; ++r)
    {
        const std::size_t row = problem.constraint_count++;
        problem.row_lower_bounds.push_back(y0_row(as_index(r)));
        problem.row_upper_bounds.push_back(y0_row(as_index(r)));
        for (std::size_t j = 0; j < k; ++j)
        {
            problem.matrix.push_back(
                {.row = row,
                 .column = j,
                 .value = data.outputs(as_index(context.reference_indices[j]), as_index(r))});
        }
        problem.matrix.push_back({.row = row, .column = k + m + r, .value = -1.0});
    }
    append_rts_row(context.options.rts, 0, k, problem);
    problem.column_lower_bounds.assign(problem.variable_count, 0.0);
    problem.column_upper_bounds.assign(problem.variable_count, POSITIVE_INFINITY);
    return problem;
}

result::DmuResult AdditiveSlacksModel::assemble_result(const SolveContext &context,
                                                       const SolveOutcome &outcome)
{
    const auto &data = *context.data;
    const std::size_t m = data.input_count();
    const std::size_t s = data.output_count();
    const std::size_t k = context.reference_indices.size();

    result::DmuResult out;
    out.dmu_index = context.dmu_index;
    if (outcome.status != SolveStatus::OPTIMAL)
    {
        // 加性 LP 恒可行（λ=自身, s=0）；非最优属求解器异常
        throw EngineException(3, "solve",
                              "加性 LP 求解失败（状态码 " +
                                  std::to_string(static_cast<int>(outcome.status)) + "）");
    }
    out.score = outcome.objective_value; // 加权和目标（有效 DMU 为 0；与径向排序相反）
    out.status = out.score <= K_ZERO_TOLERANCE ? result::DmuStatus::EFFICIENT
                                               : result::DmuStatus::INEFFICIENT;
    const auto &solution = outcome.primal_solution;
    for (std::size_t j = 0; j < k; ++j)
    {
        if (std::abs(solution[j]) > 1e-9)
        {
            out.lambdas.emplace_back(context.reference_indices[j], solution[j]);
        }
    }
    out.input_slacks.resize(m);
    out.output_slacks.resize(s);
    for (std::size_t i = 0; i < m; ++i)
    {
        out.input_slacks[i] = solution[k + i];
    }
    for (std::size_t r = 0; r < s; ++r)
    {
        out.output_slacks[r] = solution[k + m + r];
    }
    // 投影 = x0 − s⁻ / y0 + s⁺
    out.input_targets.resize(m);
    out.output_targets.resize(s);
    for (std::size_t i = 0; i < m; ++i)
    {
        out.input_targets[i] =
            data.inputs.row(as_index(context.dmu_index))(as_index(i)) - out.input_slacks[i];
    }
    for (std::size_t r = 0; r < s; ++r)
    {
        out.output_targets[r] =
            data.outputs.row(as_index(context.dmu_index))(as_index(r)) + out.output_slacks[r];
    }
    return out;
}

LpProblem DsbmModel::build_problem(const SolveContext &context)
{
    const auto &data = *context.data;
    const std::size_t m = data.input_count();
    const std::size_t s = data.output_count();
    const std::size_t k = context.reference_indices.size();
    const bool is_vrs = context.options.rts == Rts::VRS;

    // 方向来源与 DDF 同机制（own/mean/unit/custom；经 --direction 传入）
    Eigen::VectorXd gx;
    Eigen::VectorXd gy;
    {
        const auto it = context.options.model_options.find("direction");
        const std::string kind =
            it != context.options.model_options.end() && !it->second.empty() ? it->second : "own";
        gx.resize(as_index(m));
        gy.resize(as_index(s));
        const auto &x0 = data.inputs.row(as_index(context.dmu_index));
        const auto &y0 = data.outputs.row(as_index(context.dmu_index));
        if (kind == "own")
        {
            for (std::size_t i = 0; i < m; ++i)
            {
                gx(as_index(i)) = x0(as_index(i)) > 0.0 ? x0(as_index(i)) : 1.0;
            }
            for (std::size_t r = 0; r < s; ++r)
            {
                gy(as_index(r)) = y0(as_index(r)) > 0.0 ? y0(as_index(r)) : 1.0;
            }
        }
        else if (kind == "unit")
        {
            gx.setOnes();
            gy.setOnes();
        }
        else if (kind == "mean")
        {
            const Eigen::VectorXd mean_x = data.inputs.colwise().mean();
            const Eigen::VectorXd mean_y = data.outputs.colwise().mean();
            for (std::size_t i = 0; i < m; ++i)
            {
                gx(as_index(i)) = mean_x(as_index(i)) > 0.0 ? mean_x(as_index(i)) : 1.0;
            }
            for (std::size_t r = 0; r < s; ++r)
            {
                gy(as_index(r)) = mean_y(as_index(r)) > 0.0 ? mean_y(as_index(r)) : 1.0;
            }
        }
        else if (kind == "custom")
        {
            if (!data.directions.has_value())
            {
                throw EngineException(2, "data", "DSBM 自定义方向需要 di_/do_ 方向列");
            }
            for (std::size_t i = 0; i < m; ++i)
            {
                const double value =
                    data.directions->input_directions(as_index(context.dmu_index), as_index(i));
                gx(as_index(i)) = value > 0.0 ? value : 1.0;
            }
            for (std::size_t r = 0; r < s; ++r)
            {
                const double value =
                    data.directions->output_directions(as_index(context.dmu_index), as_index(r));
                gy(as_index(r)) = value > 0.0 ? value : 1.0;
            }
        }
        else
        {
            throw EngineException(1, "args",
                                  "DSBM 方向来源支持 own|mean|unit|custom（rdm 不适用）");
        }
    }

    const auto &x0_row = data.inputs.row(as_index(context.dmu_index));
    const auto &y0_row = data.outputs.row(as_index(context.dmu_index));

    // 变量布局：[t, λ_k, s⁻_m, s⁺_s]；CC 变换（归一基 = 方向 g，06 册 §4.4 #5）
    LpProblem problem;
    problem.variable_count = 1 + k + m + s;
    problem.is_minimization = true;
    problem.objective.assign(problem.variable_count, 0.0);
    problem.objective[0] = 1.0;
    for (std::size_t i = 0; i < m; ++i)
    {
        problem.objective[1 + k + i] = -1.0 / (static_cast<double>(m) * gx(as_index(i)));
    }
    for (std::size_t i = 0; i < m; ++i)
    {
        const std::size_t row = problem.constraint_count++;
        problem.row_lower_bounds.push_back(0.0);
        problem.row_upper_bounds.push_back(0.0);
        for (std::size_t j = 0; j < k; ++j)
        {
            problem.matrix.push_back(
                {.row = row,
                 .column = 1 + j,
                 .value = data.inputs(as_index(context.reference_indices[j]), as_index(i))});
        }
        problem.matrix.push_back({.row = row, .column = 1 + k + i, .value = 1.0});
        problem.matrix.push_back({.row = row, .column = 0, .value = -x0_row(as_index(i))});
    }
    for (std::size_t r = 0; r < s; ++r)
    {
        const std::size_t row = problem.constraint_count++;
        problem.row_lower_bounds.push_back(0.0);
        problem.row_upper_bounds.push_back(0.0);
        for (std::size_t j = 0; j < k; ++j)
        {
            problem.matrix.push_back(
                {.row = row,
                 .column = 1 + j,
                 .value = data.outputs(as_index(context.reference_indices[j]), as_index(r))});
        }
        problem.matrix.push_back({.row = row, .column = 1 + k + m + r, .value = -1.0});
        problem.matrix.push_back({.row = row, .column = 0, .value = -y0_row(as_index(r))});
    }
    {
        // t + (1/s)Σ s⁺/g_y = 1
        const std::size_t row = problem.constraint_count++;
        problem.row_lower_bounds.push_back(1.0);
        problem.row_upper_bounds.push_back(1.0);
        problem.matrix.push_back({.row = row, .column = 0, .value = 1.0});
        for (std::size_t r = 0; r < s; ++r)
        {
            problem.matrix.push_back({.row = row,
                                      .column = 1 + k + m + r,
                                      .value = 1.0 / (static_cast<double>(s) * gy(as_index(r)))});
        }
    }
    if (is_vrs)
    {
        const std::size_t row = problem.constraint_count++;
        problem.row_lower_bounds.push_back(0.0);
        problem.row_upper_bounds.push_back(0.0);
        for (std::size_t j = 0; j < k; ++j)
        {
            problem.matrix.push_back({.row = row, .column = 1 + j, .value = 1.0});
        }
        problem.matrix.push_back({.row = row, .column = 0, .value = -1.0});
    }
    problem.column_lower_bounds.assign(problem.variable_count, 0.0);
    problem.column_upper_bounds.assign(problem.variable_count, POSITIVE_INFINITY);
    return problem;
}

result::DmuResult DsbmModel::assemble_result(const SolveContext &context,
                                             const SolveOutcome &outcome)
{
    result::DmuResult out;
    out.dmu_index = context.dmu_index;
    if (outcome.status != SolveStatus::OPTIMAL)
    {
        throw EngineException(3, "solve", "DSBM LP 求解失败");
    }
    out.score = outcome.objective_value; // ρ_g ∈ [0,1]
    out.status = out.score >= 1.0 - K_ZERO_TOLERANCE ? result::DmuStatus::EFFICIENT
                                                     : result::DmuStatus::INEFFICIENT;
    const std::size_t k = context.reference_indices.size();
    const std::size_t m = context.data->input_count();
    for (std::size_t j = 0; j < k; ++j)
    {
        if (std::abs(outcome.primal_solution[1 + j]) > 1e-9)
        {
            out.lambdas.emplace_back(context.reference_indices[j], outcome.primal_solution[1 + j]);
        }
    }
    out.input_slacks.resize(m);
    for (std::size_t i = 0; i < m; ++i)
    {
        out.input_slacks[i] = outcome.primal_solution[1 + k + i];
    }
    return out;
}

LpProblem ErmModel::build_problem(const SolveContext &context)
{
    const auto &data = *context.data;
    const std::size_t m = data.input_count();
    const std::size_t s = data.output_count();
    const std::size_t k = context.reference_indices.size();
    const bool is_input = context.options.orientation == Orientation::INPUT;
    const auto &x0_row = data.inputs.row(as_index(context.dmu_index));
    const auto &y0_row = data.outputs.row(as_index(context.dmu_index));

    if (is_input)
    {
        // 投入 Russell：min (1/m)Σθ_i s.t. Xλ ≤ θ∘x0; Yλ ≥ y0；变量 [θ_m, λ_k]
        LpProblem problem;
        problem.variable_count = m + k;
        problem.is_minimization = true;
        problem.objective.assign(problem.variable_count, 0.0);
        for (std::size_t i = 0; i < m; ++i)
        {
            problem.objective[i] = 1.0 / static_cast<double>(m);
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
            problem.matrix.push_back({.row = row, .column = i, .value = -x0_row(as_index(i))});
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
        append_rts_row(context.options.rts, m, k, problem);
        problem.column_lower_bounds.assign(problem.variable_count, 0.0);
        problem.column_upper_bounds.assign(problem.variable_count, POSITIVE_INFINITY);
        return problem;
    }

    // 产出 Russell：max (1/s)Σφ_r s.t. Yλ ≥ φ∘y0; Xλ ≤ x0；变量 [φ_s, λ_k]
    LpProblem problem;
    problem.variable_count = s + k;
    problem.is_minimization = false;
    problem.objective.assign(problem.variable_count, 0.0);
    for (std::size_t r = 0; r < s; ++r)
    {
        problem.objective[r] = 1.0 / static_cast<double>(s);
    }
    for (std::size_t r = 0; r < s; ++r)
    {
        const std::size_t row = problem.constraint_count++;
        // Yλ − φ·y0 ≥ 0（φ 上界由参考集产出水平给出）
        problem.row_lower_bounds.push_back(0.0);
        problem.row_upper_bounds.push_back(POSITIVE_INFINITY);
        for (std::size_t j = 0; j < k; ++j)
        {
            problem.matrix.push_back(
                {.row = row,
                 .column = s + j,
                 .value = data.outputs(as_index(context.reference_indices[j]), as_index(r))});
        }
        problem.matrix.push_back({.row = row, .column = r, .value = -y0_row(as_index(r))});
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
    append_rts_row(context.options.rts, s, k, problem);
    problem.column_lower_bounds.assign(problem.variable_count, 0.0);
    problem.column_upper_bounds.assign(problem.variable_count, POSITIVE_INFINITY);
    return problem;
}

result::DmuResult ErmModel::assemble_result(const SolveContext &context,
                                            const SolveOutcome &outcome)
{
    const bool is_input = context.options.orientation == Orientation::INPUT;
    const std::size_t n_vars =
        is_input ? context.data->input_count() : context.data->output_count();

    result::DmuResult out;
    out.dmu_index = context.dmu_index;
    if (outcome.status != SolveStatus::OPTIMAL)
    {
        throw EngineException(3, "solve",
                              "ERM LP 求解失败（状态码 " +
                                  std::to_string(static_cast<int>(outcome.status)) + "）");
    }
    const double measure = outcome.objective_value;
    out.score = is_input ? measure : 1.0 / measure; // io：Russell 均值 ≤1；oo：1/φ̄ ≤1
    out.status = out.score >= 1.0 - K_ZERO_TOLERANCE ? result::DmuStatus::EFFICIENT
                                                     : result::DmuStatus::INEFFICIENT;
    for (std::size_t j = 0; j < context.reference_indices.size(); ++j)
    {
        const double weight = outcome.primal_solution[n_vars + j];
        if (std::abs(weight) > 1e-9)
        {
            out.lambdas.emplace_back(context.reference_indices[j], weight);
        }
    }
    return out;
}
} // namespace opensuperdea::nonradial
