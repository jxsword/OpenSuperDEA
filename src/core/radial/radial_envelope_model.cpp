/**
 * @file radial_envelope_model.cpp
 * @brief 径向包络模型实现：包络 LP 构造、二阶段 max-slack、状态映射与投影合成。
 * @details 数学约定（06 册 §3.1）：X 为 n×m 投入、Y 为 n×s 产出；投入导向
 *          min θ s.t. X_ref^T λ ≤ θ·x0, Y_ref^T λ ≥ y0, RTS 截距约束；
 *          输出导向 max φ（效率 = 1/φ*）。二阶段固定一阶段测度后最大化
 *          Σs⁻+Σs⁺（对偶/二阶段提取 slack，FR-OUT-004）。
 */
#include "radial_envelope_model.h"

#include <algorithm>
#include <cmath>

#include "core/registry/engine_error.h"

namespace opensuperdea::radial
{
namespace
{
using opensuperdea::registry::EngineException;
using opensuperdea::registry::Rts;
using opensuperdea::registry::SolveContext;
using opensuperdea::solver::LpProblem;
using opensuperdea::solver::MatrixTriplet;
using opensuperdea::solver::NEGATIVE_INFINITY;
using opensuperdea::solver::POSITIVE_INFINITY;
using opensuperdea::solver::SolveOutcome;
using opensuperdea::solver::SolveStatus;

/** @brief λ/乘子的数值零阈值（LP 解的数值噪声水平以下视为零）。 */
constexpr double K_LAMBDA_ZERO = 1e-9;

/** @brief 效率值 1 的判定容差（02 册双阈值以下的工程容差）。 */
constexpr double K_UNIT_TOLERANCE = 1e-9;

/**
 * @brief 二阶段松弛变量的下界（F-001 修复）。
 * @details 一阶段单纯形在 HiGHS primal 可行容差（默认 1e-7）内终止，最优解对约束的
 *          违反量可达容差量级；二阶段将不等式改为严格等式后，该违反量使 presolve
 *          判定不可行（规模越大触发概率越高）。将松弛变量下界从 0 定向放宽到
 *          −K_STAGE2_SLACK_FLOOR 同量级：违反量被松弛吸收，二阶段可行性由
 *          「一阶段最优解 + 微负松弛」 witness 保证；最优目标与效率值不受影响，
 *          松弛/投影的数值影响 ≤ 1e-7（在 P-010 对拍阈值 1e-6 内）。
 */
constexpr double K_STAGE2_SLACK_FLOOR = -1e-7;

/**
 * @brief RTS 截距约束行（VRS: Σλ=1；NIRS: Σλ≤1；NDRS: Σλ≥1；CRS: 无）。
 * @param rts 规模报酬假定
 * @param lambda_offset λ 变量在问题中的起始列
 * @param lambda_count 参考集大小
 * @param problem 目标问题（追加一行）
 * @return 有无追加约束行（CRS 返回 false）
 */

/** @brief size_t → Eigen 索引（显式转换，避免实现定义收窄）。 */
Eigen::Index as_index(std::size_t value)
{
    return static_cast<Eigen::Index>(value);
}

bool append_rts_row(Rts rts, std::size_t lambda_offset, std::size_t lambda_count,
                    LpProblem &problem)
{
    if (rts == Rts::CRS)
    {
        return false;
    }
    const std::size_t row = problem.constraint_count++;
    // VRS 为等式 Σλ=1；NIRS 仅上界 ≤1；NDRS 仅下界 ≥1
    const bool lower_one = rts == Rts::VRS || rts == Rts::NDRS;
    const bool upper_one = rts == Rts::VRS || rts == Rts::NIRS;
    problem.row_lower_bounds.push_back(lower_one ? 1.0 : NEGATIVE_INFINITY);
    problem.row_upper_bounds.push_back(upper_one ? 1.0 : POSITIVE_INFINITY);
    for (std::size_t j = 0; j < lambda_count; ++j)
    {
        problem.matrix.push_back({.row = row, .column = lambda_offset + j, .value = 1.0});
    }
    return true;
}
} // namespace

LpProblem RadialEnvelopeModel::build_problem(const SolveContext &context)
{
    const auto &data = *context.data;
    const std::size_t m = data.input_count();
    const std::size_t s = data.output_count();
    const std::size_t k = context.reference_indices.size();
    const bool is_input = context.options.orientation == registry::Orientation::INPUT;

    LpProblem problem;
    // 变量布局：测度（θ 或 φ）+ k 个 λ
    problem.variable_count = 1 + k;

    if (k == 0)
    {
        // 参考集为空（如单 DMU 超效率）：无技术可参照，语义即不可行（06 册 §3.2 態判）。
        // 构造「空行 ≥ 1」的确定不可行问题，交由求解器状态映射统一走 INFEASIBLE 分支。
        problem.is_minimization = true;
        problem.objective = {0.0};
        problem.constraint_count = 1;
        problem.row_lower_bounds = {1.0};
        problem.row_upper_bounds = {POSITIVE_INFINITY};
        problem.column_lower_bounds = {NEGATIVE_INFINITY};
        problem.column_upper_bounds = {POSITIVE_INFINITY};
        return problem;
    }

    const auto &x0_row = data.inputs.row(as_index(context.dmu_index));
    const auto &y0_row = data.outputs.row(as_index(context.dmu_index));

    if (is_input)
    {
        // min θ；θ 自由
        problem.is_minimization = true;
        problem.objective = {1.0};
        problem.objective.resize(problem.variable_count, 0.0);

        // 投入收缩：X_ref^T λ − θ·x0 ≤ 0（m 行）
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
        // 产出不降：Y_ref^T λ ≥ y0（s 行）
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
        // max φ；φ 自由（效率 = 1/φ*）
        problem.is_minimization = false;
        problem.objective = {1.0};
        problem.objective.resize(problem.variable_count, 0.0);

        // 产出扩张：Y_ref^T λ − φ·y0 ≥ 0（s 行）
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
        // 投入不增：X_ref^T λ ≤ x0（m 行）
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

    // RTS 截距约束（BCC/NIRS/NDRS；06 册 §3.1）
    append_rts_row(context.options.rts, 1, k, problem);

    // 变量界：测度自由，λ ≥ 0
    problem.column_lower_bounds = {NEGATIVE_INFINITY};
    problem.column_lower_bounds.resize(problem.variable_count, 0.0);
    problem.column_upper_bounds.assign(problem.variable_count, POSITIVE_INFINITY);
    return problem;
}

result::DmuResult RadialEnvelopeModel::assemble_result(const SolveContext &context,
                                                       const SolveOutcome &outcome)
{
    const auto &data = *context.data;
    const std::size_t m = data.input_count();
    const std::size_t s = data.output_count();
    const std::size_t k = context.reference_indices.size();
    const bool is_input = context.options.orientation == registry::Orientation::INPUT;

    result::DmuResult out;
    out.dmu_index = context.dmu_index;

    if (outcome.status == SolveStatus::INFEASIBLE)
    {
        // 超效率 VRS 不可行 / 参考集为空：标注不输出数值（06 册 §3.2）
        out.score = std::numeric_limits<double>::quiet_NaN();
        out.status = result::DmuStatus::INFEASIBLE;
        return out;
    }
    if (outcome.status != SolveStatus::OPTIMAL)
    {
        throw EngineException(3, "solve",
                              "径向包络 LP 求解失败（状态码 " +
                                  std::to_string(static_cast<int>(outcome.status)) + "）");
    }

    const double theta = outcome.objective_value; // 投入导向即 θ*；输出导向即 φ*
    if (!is_input && theta <= K_UNIT_TOLERANCE)
    {
        throw EngineException(3, "solve", "输出导向 φ* 非正（产出数据退化）",
                              "检查被评 DMU 产出是否全零");
    }
    out.score = is_input ? theta : 1.0 / theta;

    // 状态判定：>1 超有效（超效率模式）；≈1 有效（含弱有效，slack 单独呈现）
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

    // ---- 二阶段：固定一阶段测度，max Σs⁻ + Σs⁺（FR-OUT-004）----
    const auto &x0_row = data.inputs.row(as_index(context.dmu_index));
    const auto &y0_row = data.outputs.row(as_index(context.dmu_index));
    const std::size_t slack_in_offset = k;
    const std::size_t slack_out_offset = k + m;

    LpProblem stage2;
    stage2.variable_count = k + m + s;
    stage2.is_minimization = false; // max 松弛和
    stage2.objective.assign(stage2.variable_count, 0.0);
    for (std::size_t i = 0; i < m; ++i)
    {
        stage2.objective[slack_in_offset + i] = 1.0;
    }
    for (std::size_t r = 0; r < s; ++r)
    {
        stage2.objective[slack_out_offset + r] = 1.0;
    }

    if (is_input)
    {
        // Σλ X + s⁻ = θ*·x0（m 行等式）
        for (std::size_t i = 0; i < m; ++i)
        {
            const std::size_t row = stage2.constraint_count++;
            stage2.row_lower_bounds.push_back(theta * x0_row(as_index(i)));
            stage2.row_upper_bounds.push_back(theta * x0_row(as_index(i)));
            for (std::size_t j = 0; j < k; ++j)
            {
                stage2.matrix.push_back(
                    {.row = row,
                     .column = j,
                     .value = data.inputs(as_index(context.reference_indices[j]), as_index(i))});
            }
            stage2.matrix.push_back({.row = row, .column = slack_in_offset + i, .value = 1.0});
        }
        // Σλ Y − s⁺ = y0（s 行等式）
        for (std::size_t r = 0; r < s; ++r)
        {
            const std::size_t row = stage2.constraint_count++;
            stage2.row_lower_bounds.push_back(y0_row(as_index(r)));
            stage2.row_upper_bounds.push_back(y0_row(as_index(r)));
            for (std::size_t j = 0; j < k; ++j)
            {
                stage2.matrix.push_back(
                    {.row = row,
                     .column = j,
                     .value = data.outputs(as_index(context.reference_indices[j]), as_index(r))});
            }
            stage2.matrix.push_back({.row = row, .column = slack_out_offset + r, .value = -1.0});
        }
    }
    else
    {
        // Σλ X + s⁻ = x0
        for (std::size_t i = 0; i < m; ++i)
        {
            const std::size_t row = stage2.constraint_count++;
            stage2.row_lower_bounds.push_back(x0_row(as_index(i)));
            stage2.row_upper_bounds.push_back(x0_row(as_index(i)));
            for (std::size_t j = 0; j < k; ++j)
            {
                stage2.matrix.push_back(
                    {.row = row,
                     .column = j,
                     .value = data.inputs(as_index(context.reference_indices[j]), as_index(i))});
            }
            stage2.matrix.push_back({.row = row, .column = slack_in_offset + i, .value = 1.0});
        }
        // Σλ Y − s⁺ = φ*·y0
        for (std::size_t r = 0; r < s; ++r)
        {
            const std::size_t row = stage2.constraint_count++;
            stage2.row_lower_bounds.push_back(theta * y0_row(as_index(r)));
            stage2.row_upper_bounds.push_back(theta * y0_row(as_index(r)));
            for (std::size_t j = 0; j < k; ++j)
            {
                stage2.matrix.push_back(
                    {.row = row,
                     .column = j,
                     .value = data.outputs(as_index(context.reference_indices[j]), as_index(r))});
            }
            stage2.matrix.push_back({.row = row, .column = slack_out_offset + r, .value = -1.0});
        }
    }
    append_rts_row(context.options.rts, 0, k, stage2);
    stage2.column_lower_bounds.assign(stage2.variable_count, 0.0);
    stage2.column_upper_bounds.assign(stage2.variable_count, POSITIVE_INFINITY);

    // F-001：一阶段单纯形在 HiGHS primal 容差（1e-7）内终止，最优解对约束的违反量
    // 可达容差量级，严格等式二阶段会被 presolve 判不可行（规模越大概率越高）。
    // 回退策略：首次严格求解（小规模保持精确收敛）；仅失败时将松弛段下界定向放宽到
    // K_STAGE2_SLACK_FLOOR 重解一次（违反量被松弛吸收，witness = 一阶段最优解）。
    // 效率值（一阶段）不受影响；回退路径松弛/投影偏差 ≤ 1e-7，在 P-010 阈值 1e-6 内。
    SolveOutcome stage2_outcome = opensuperdea::adapters::solve_lp(stage2);
    if (stage2_outcome.status != SolveStatus::OPTIMAL)
    {
        for (std::size_t i = 0; i < m + s; ++i)
        {
            stage2.column_lower_bounds[slack_in_offset + i] = K_STAGE2_SLACK_FLOOR;
        }
        stage2_outcome = opensuperdea::adapters::solve_lp(stage2);
    }
    if (stage2_outcome.status != SolveStatus::OPTIMAL)
    {
        // 一阶段最优解可行 ⇒ 二阶段必可行；到不了这里属求解器异常
        throw EngineException(3, "solve", "二阶段松弛 LP 求解失败");
    }
    const auto &solution = stage2_outcome.primal_solution;

    // λ 非零项（FR-OUT-003；参考集内位置 → 数据集全局索引）
    for (std::size_t j = 0; j < k; ++j)
    {
        if (std::abs(solution[j]) > K_LAMBDA_ZERO)
        {
            out.lambdas.emplace_back(context.reference_indices[j], solution[j]);
        }
    }
    out.input_slacks.resize(m);
    out.output_slacks.resize(s);
    for (std::size_t i = 0; i < m; ++i)
    {
        // 下界放宽后松弛可为容差级微负（F-001），输出按非负语义钳制
        out.input_slacks[i] = std::max(0.0, solution[slack_in_offset + i]);
    }
    for (std::size_t r = 0; r < s; ++r)
    {
        out.output_slacks[r] = std::max(0.0, solution[slack_out_offset + r]);
    }

    // 投影（FR-OUT-002）：x* = θ*x0 − s⁻（投入）/ x0 − s⁻；y* = y0 + s⁺（投入）/ φ*y0 + s⁺
    out.input_targets.resize(m);
    out.output_targets.resize(s);
    for (std::size_t i = 0; i < m; ++i)
    {
        const double shrink = is_input ? theta : 1.0;
        out.input_targets[i] = (shrink * x0_row(as_index(i))) - out.input_slacks[i];
    }
    for (std::size_t r = 0; r < s; ++r)
    {
        const double expand = is_input ? 1.0 : theta;
        out.output_targets[r] = (expand * y0_row(as_index(r))) + out.output_slacks[r];
    }
    return out;
}
} // namespace opensuperdea::radial
