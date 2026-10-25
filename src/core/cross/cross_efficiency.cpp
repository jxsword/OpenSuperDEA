/**
 * @file cross_efficiency.cpp
 * @brief 交叉效率实现：两阶段 LP 骨架与矩阵组装（deaR M2/M3 严格对齐，见 .h 文件头）。
 */
#include "cross_efficiency.h"

#include <algorithm>
#include <cmath>
#include <string>

#include "core/parallel/work_stealing_pool.h"
#include "core/radial/multiplier_model.h"
#include "core/registry/engine_error.h"

namespace opensuperdea::cross
{
namespace
{
using opensuperdea::registry::EngineException;
using opensuperdea::registry::Orientation;
using opensuperdea::registry::Rts;
using opensuperdea::registry::RunConfig;
using opensuperdea::registry::SolveContext;
using opensuperdea::solver::LpProblem;
using opensuperdea::solver::MatrixTriplet;
using opensuperdea::solver::NEGATIVE_INFINITY;
using opensuperdea::solver::POSITIVE_INFINITY;
using opensuperdea::solver::SolveOutcome;
using opensuperdea::solver::SolveStatus;

/** @brief 效率值 1 判定容差。 */
constexpr double K_UNIT_TOLERANCE = 1e-9;

/** @brief deaR 同款防无界界（第二阶段 Σ变量 ≤ 1e10）。 */
constexpr double K_BOUND = 1e10;

/** @brief size_t → Eigen 索引。 */
Eigen::Index as_index(std::size_t value)
{
    return static_cast<Eigen::Index>(value);
}

/** @brief 第一阶段：逐 DMU CCR/BCC 乘子自评（得 e_ii 与任意乘子）。 */
struct Stage1Result
{
    std::vector<double> efficiency;               // e_ii
    std::vector<std::vector<double>> input_mult;  // v_i（m）
    std::vector<std::vector<double>> output_mult; // u_i（s）
};

Stage1Result run_stage1(const dataset::Dataset &data, Rts rts, unsigned int thread_count)
{
    opensuperdea::radial::MultiplierModel model;
    Stage1Result stage1;
    stage1.efficiency.resize(data.dmu_count());
    stage1.input_mult.resize(data.dmu_count());
    stage1.output_mult.resize(data.dmu_count());
    // 逐 DMU 自评相互独立（FR-PERF-001，07 册 §2.3）：按索引预分配无锁写入
    parallel::parallel_for(
        data.dmu_count(), thread_count, [&data, &model, &stage1, rts](std::size_t i) {
            SolveContext context;
            context.data = &data;
            context.dmu_index = i;
            for (std::size_t j = 0; j < data.dmu_count(); ++j)
            {
                context.reference_indices.push_back(j);
            }
            context.options.orientation = Orientation::INPUT;
            context.options.rts = rts;
            const LpProblem problem = model.build_problem(context);
            const SolveOutcome outcome = opensuperdea::adapters::solve_lp(problem);
            if (outcome.status != SolveStatus::OPTIMAL)
            {
                throw EngineException(3, "solve",
                                      "交叉效率第一阶段自评失败（DMU " + std::to_string(i) + "）");
            }
            const result::DmuResult dmu = model.assemble_result(context, outcome);
            stage1.efficiency[i] = dmu.score;
            stage1.input_mult[i] = dmu.input_multipliers;
            stage1.output_mult[i] = dmu.output_multipliers;
        });
    return stage1;
}

/**
 * @brief 第二阶段 LP（评者 i；M2/M3/虚拟 DMU 同骨架）。
 * @return 求解出的 (v, u, ωL−ωU)（CRS 时 ω = 0）
 */
struct Stage2Solution
{
    std::vector<double> input_mult;
    std::vector<double> output_mult;
    double rts_intercept = 0.0; // ωL − ωU（仅 VRS 非零）
};

Stage2Solution run_stage2(const dataset::Dataset &data, const Stage1Result &stage1,
                          const CrossRunOptions &options, std::size_t evaluator)
{
    const std::size_t m = data.input_count();
    const std::size_t s = data.output_count();
    const std::size_t n = data.dmu_count();
    const bool is_vrs = options.rts == Rts::VRS;
    const std::size_t omega_l = m + s;
    const std::size_t omega_u = omega_l + 1;
    const bool has_omega = is_vrs;
    const std::size_t var_count = m + s + (has_omega ? 2U : 0U);

    // 评者 i 之外的 DMU 集合（j ≠ i）
    std::vector<std::size_t> others;
    for (std::size_t j = 0; j < n; ++j)
    {
        if (j != evaluator)
        {
            others.push_back(j);
        }
    }

    const bool is_benevolent = options.method == CrossMethod::DG_BENEVOLENT ||
                               options.method == CrossMethod::M3_BENEVOLENT;
    const bool is_m3 = options.method == CrossMethod::M3_AGGRESSIVE ||
                       options.method == CrossMethod::M3_BENEVOLENT;
    const bool is_virtual = options.method == CrossMethod::VIRTUAL_DMU;

    LpProblem problem;
    problem.variable_count = var_count;
    problem.is_minimization = !is_benevolent;
    problem.objective.assign(var_count, 0.0);

    // 目标系数（io 口径；06 册 §8 + deaR M2/M3）
    if (is_virtual)
    {
        for (std::size_t i2 = 0; i2 < m; ++i2)
        {
            problem.objective[i2] = -options.virtual_inputs[i2]; // −v·x_V
        }
        for (std::size_t r = 0; r < s; ++r)
        {
            problem.objective[m + r] = options.virtual_outputs[r]; // +u·y_V
        }
    }
    else if (is_m3)
    {
        // M3：min/max Σ_{j≠i} u·y_j（投入侧无目标系数）
        for (std::size_t r = 0; r < s; ++r)
        {
            double sum = 0.0;
            for (const std::size_t j : others)
            {
                sum += data.outputs(as_index(j), as_index(r));
            }
            problem.objective[m + r] = sum;
        }
        if (has_omega)
        {
            problem.objective[omega_l] = static_cast<double>(others.size());
            problem.objective[omega_u] = -static_cast<double>(others.size());
        }
    }
    else
    {
        // M2（Doyle-Green）：min/max Σ_{j≠i}(u·y_j − v·x_j)
        for (std::size_t i2 = 0; i2 < m; ++i2)
        {
            double sum = 0.0;
            for (const std::size_t j : others)
            {
                sum += data.inputs(as_index(j), as_index(i2));
            }
            problem.objective[i2] = -sum;
        }
        for (std::size_t r = 0; r < s; ++r)
        {
            double sum = 0.0;
            for (const std::size_t j : others)
            {
                sum += data.outputs(as_index(j), as_index(r));
            }
            problem.objective[m + r] = sum;
        }
        if (has_omega)
        {
            problem.objective[omega_l] = static_cast<double>(others.size());
            problem.objective[omega_u] = -static_cast<double>(others.size());
        }
    }

    // 约束 (a)：∀j: −v·x_j + u·y_j + ωL − ωU ≤ 0
    for (std::size_t j = 0; j < n; ++j)
    {
        const std::size_t row = problem.constraint_count++;
        problem.row_lower_bounds.push_back(NEGATIVE_INFINITY);
        problem.row_upper_bounds.push_back(0.0);
        for (std::size_t i2 = 0; i2 < m; ++i2)
        {
            problem.matrix.push_back(
                {.row = row, .column = i2, .value = -data.inputs(as_index(j), as_index(i2))});
        }
        for (std::size_t r = 0; r < s; ++r)
        {
            problem.matrix.push_back(
                {.row = row, .column = m + r, .value = data.outputs(as_index(j), as_index(r))});
        }
        if (has_omega)
        {
            problem.matrix.push_back({.row = row, .column = omega_l, .value = 1.0});
            problem.matrix.push_back({.row = row, .column = omega_u, .value = -1.0});
        }
    }

    // 约束 (b)：M2: v·x_i = 1；M3: v·Σ_{j≠i}x_j = 1
    {
        const std::size_t row = problem.constraint_count++;
        problem.row_lower_bounds.push_back(1.0);
        problem.row_upper_bounds.push_back(1.0);
        for (std::size_t i2 = 0; i2 < m; ++i2)
        {
            double coefficient;
            if (is_m3)
            {
                coefficient = 0.0;
                for (const std::size_t j : others)
                {
                    coefficient += data.inputs(as_index(j), as_index(i2));
                }
            }
            else
            {
                coefficient = data.inputs(as_index(evaluator), as_index(i2));
            }
            problem.matrix.push_back({.row = row, .column = i2, .value = coefficient});
        }
    }

    // 约束 (c)：M2: u·y_i + ω = e_ii；M3: u·y_i − e_ii·v·x_i + ω = 0；虚拟 DMU 同 M2
    {
        const std::size_t row = problem.constraint_count++;
        if (is_m3)
        {
            problem.row_lower_bounds.push_back(0.0);
            problem.row_upper_bounds.push_back(0.0);
            for (std::size_t i2 = 0; i2 < m; ++i2)
            {
                problem.matrix.push_back({.row = row,
                                          .column = i2,
                                          .value = -stage1.efficiency[evaluator] *
                                                   data.inputs(as_index(evaluator), as_index(i2))});
            }
        }
        else
        {
            problem.row_lower_bounds.push_back(stage1.efficiency[evaluator]);
            problem.row_upper_bounds.push_back(stage1.efficiency[evaluator]);
        }
        for (std::size_t r = 0; r < s; ++r)
        {
            problem.matrix.push_back({.row = row,
                                      .column = m + r,
                                      .value = data.outputs(as_index(evaluator), as_index(r))});
        }
        if (has_omega)
        {
            problem.matrix.push_back({.row = row, .column = omega_l, .value = 1.0});
            problem.matrix.push_back({.row = row, .column = omega_u, .value = -1.0});
        }
    }

    // 约束 (d)：防无界界 Σ变量 ≤ 1e10（deaR 同款，双方法均加）
    {
        const std::size_t row = problem.constraint_count++;
        problem.row_lower_bounds.push_back(NEGATIVE_INFINITY);
        problem.row_upper_bounds.push_back(K_BOUND);
        for (std::size_t v2 = 0; v2 < var_count; ++v2)
        {
            problem.matrix.push_back({.row = row, .column = v2, .value = 1.0});
        }
    }

    // 变量界：乘子与 ωL ≥ 0，ωU ≤ 0 由 ωU 变量非负 + 目标系数方向实现
    // （deaR 变量全部 ≥ 0，ωU 以负系数进入；此处同构）
    problem.column_lower_bounds.assign(var_count, 0.0);
    problem.column_upper_bounds.assign(var_count, POSITIVE_INFINITY);

    const SolveOutcome outcome = opensuperdea::adapters::solve_lp(problem);
    if (outcome.status != SolveStatus::OPTIMAL)
    {
        throw EngineException(3, "solve",
                              "交叉效率第二阶段 LP 求解失败（评者 " + std::to_string(evaluator) +
                                  "，状态码 " + std::to_string(static_cast<int>(outcome.status)) +
                                  "）");
    }
    Stage2Solution solution;
    solution.input_mult.assign(outcome.primal_solution.begin(),
                               outcome.primal_solution.begin() + static_cast<std::ptrdiff_t>(m));
    solution.output_mult.assign(outcome.primal_solution.begin() + static_cast<std::ptrdiff_t>(m),
                                outcome.primal_solution.begin() +
                                    static_cast<std::ptrdiff_t>(m + s));
    if (has_omega)
    {
        solution.rts_intercept =
            outcome.primal_solution[omega_l] - outcome.primal_solution[omega_u];
    }
    return solution;
}
} // namespace

bool cross_method_from_string(const std::string &text, CrossMethod &out)
{
    if (text == "dg_i")
    {
        out = CrossMethod::DG_AGGRESSIVE;
        return true;
    }
    if (text == "dg_ii")
    {
        out = CrossMethod::DG_BENEVOLENT;
        return true;
    }
    if (text == "m3_i")
    {
        out = CrossMethod::M3_AGGRESSIVE;
        return true;
    }
    if (text == "m3_ii")
    {
        out = CrossMethod::M3_BENEVOLENT;
        return true;
    }
    if (text == "virtual")
    {
        out = CrossMethod::VIRTUAL_DMU;
        return true;
    }
    return false;
}

result::ModelResult run_cross_efficiency(const dataset::Dataset &data,
                                         const CrossRunOptions &options, const RunConfig &config)
{
    const std::size_t n = data.dmu_count();
    const Stage1Result stage1 = run_stage1(data, options.rts, config.thread_count);

    result::ModelResult out;
    out.meta.model_id = "cross_efficiency";
    out.meta.orientation = Orientation::INPUT;
    out.meta.rts = options.rts;
    out.meta.dmu_count = n;
    out.meta.input_count = data.input_count();
    out.meta.output_count = data.output_count();
    out.meta.version = config.version;
    out.meta.data_sha1 = config.data_sha1;

    result::CrossMatrix cross;
    cross.values.assign(n, std::vector<double>(n, 0.0));

    // 逐评者求解相互独立（消费 stage1 全量结果，段间已隐式同步；07 册 §2.3）
    parallel::parallel_for(
        n, config.thread_count, [&data, &stage1, &options, &cross, n](std::size_t i) {
            const Stage2Solution solution = run_stage2(data, stage1, options, i);
            // e_ij = (u·y_j + ω)/(v·x_j)（CRS ω=0）；对角 = 自评 e_ii（deaR 同款）
            for (std::size_t j = 0; j < n; ++j)
            {
                if (j == i)
                {
                    cross.values[i][j] = stage1.efficiency[i];
                    continue;
                }
                double u_y = 0.0;
                for (std::size_t r = 0; r < data.output_count(); ++r)
                {
                    u_y += solution.output_mult[r] * data.outputs(as_index(j), as_index(r));
                }
                double v_x = 0.0;
                for (std::size_t i2 = 0; i2 < data.input_count(); ++i2)
                {
                    v_x += solution.input_mult[i2] * data.inputs(as_index(j), as_index(i2));
                }
                if (v_x <= 0.0)
                {
                    throw EngineException(4, "internal",
                                          "交叉效率分母非正（评者 " + std::to_string(i) + "）");
                }
                cross.values[i][j] = (u_y + solution.rts_intercept) / v_x;
            }
        });

    // 行均值 A_i（剔除自评）与列均值 e_j（剔除自评）
    cross.mean_given.assign(n, 0.0);
    cross.mean_received.assign(n, 0.0);
    for (std::size_t i = 0; i < n; ++i)
    {
        double sum = 0.0;
        for (std::size_t j = 0; j < n; ++j)
        {
            if (j != i)
            {
                sum += cross.values[i][j];
            }
        }
        cross.mean_given[i] = sum / static_cast<double>(n - 1);
    }
    for (std::size_t j = 0; j < n; ++j)
    {
        double sum = 0.0;
        for (std::size_t i = 0; i < n; ++i)
        {
            if (i != j)
            {
                sum += cross.values[i][j];
            }
        }
        cross.mean_received[j] = sum / static_cast<double>(n - 1);
    }

    // 逐 DMU 结果 = 收到的平均交叉效率 e_j（FR-CROSS-004 列均值 + 排名）
    out.dmu_results.resize(n);
    for (std::size_t j = 0; j < n; ++j)
    {
        out.dmu_results[j].dmu_index = j;
        out.dmu_results[j].score = cross.mean_received[j];
        out.dmu_results[j].status = cross.mean_received[j] >= 1.0 - K_UNIT_TOLERANCE
                                        ? result::DmuStatus::EFFICIENT
                                        : result::DmuStatus::INEFFICIENT;
    }
    out.cross = std::move(cross);

    // 汇总（按 e_j 降序排名）
    out.summary.rank.assign(n, 0);
    std::vector<std::size_t> order;
    order.reserve(n);
    for (std::size_t i = 0; i < n; ++i)
    {
        order.push_back(i);
    }
    std::ranges::sort(order, [&](std::size_t a, std::size_t b) {
        return out.dmu_results[a].score > out.dmu_results[b].score;
    });
    for (std::size_t pos = 0; pos < order.size(); ++pos)
    {
        std::size_t rank = pos + 1;
        if (pos > 0 && out.dmu_results[order[pos]].score == out.dmu_results[order[pos - 1]].score)
        {
            rank = out.summary.rank[order[pos - 1]];
        }
        out.summary.rank[order[pos]] = rank;
    }
    double sum = 0.0;
    for (const auto &dmu : out.dmu_results)
    {
        sum += dmu.score;
    }
    out.summary.mean_score = sum / static_cast<double>(n);
    return out;
}
} // namespace opensuperdea::cross
