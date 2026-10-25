/**
 * @file highs_adapter.cpp
 * @brief HiGHS 求解调用与状态映射实现。
 * @details 构造流程：三元组按行聚合为 CSR（有序输入 O(nnz) 直接填充，乱序回退
 *          map+sort，两路输出逐项一致）→ HighsModel → passModel → run；
 *          kUnboundedOrInfeasible 状态下以零目标二次探测区分不可行与无界
 *          （design_docs/03 册 §2.3 状态映射约定）。
 *          FR-PERF-004（07 册 §3）：SolveSession 缓存上次模型全量状态，结构指纹
 *          一致时仅对差异项做增量修改（changeCoeff/changeRowsBounds/changeColsCost/
 *          changeColsBounds/changeObjectiveSense）并 clearSolver() 冷启动——与新建
 *          实例路径单纯形起点一致，逐项同参；指纹不一致回退 passModel 全量。
 *          solve_lp/solve_milp 经 thread_local 会话透明复用（worker 线程天然
 *          worker-local，池销毁即析构）。
 */
#include "highs_adapter.h"

#include <Highs.h>

#include <algorithm>
#include <map>
#include <memory>
#include <tuple>
#include <utility>
#include <vector>

namespace
{
using opensuperdea::solver::LpProblem;
using opensuperdea::solver::SolveOutcome;
using opensuperdea::solver::SolveStatus;

/**
 * @brief 将稀疏三元组按行聚合为 CSR（start 为前缀和，行内按列排序保证确定性）。
 * @details 快速路径：三元组已按 (row, column, value) 字典序非降时单遍线性填充
 *          （模型构造全部按行主序 push，天然命中）；否则回退 map 分桶 + 逐行排序。
 *          两路输出逐项一致（含等值重复条目与越界行的丢弃行为）。
 */
void build_rowwise_matrix(const LpProblem &problem, std::vector<HighsInt> &starts,
                          std::vector<HighsInt> &indices, std::vector<double> &values)
{
    bool is_sorted = true;
    for (std::size_t i = 1; i < problem.matrix.size(); ++i)
    {
        const auto &previous = problem.matrix[i - 1];
        const auto &current = problem.matrix[i];
        if (std::tie(previous.row, previous.column, previous.value) >
            std::tie(current.row, current.column, current.value))
        {
            is_sorted = false;
            break;
        }
    }
    starts.assign(problem.constraint_count + 1, 0);
    indices.clear();
    values.clear();
    if (is_sorted)
    {
        indices.reserve(problem.matrix.size());
        values.reserve(problem.matrix.size());
        HighsInt total = 0;
        std::size_t next = 0;
        for (std::size_t row = 0; row < problem.constraint_count; ++row)
        {
            while (next < problem.matrix.size() && problem.matrix[next].row == row)
            {
                indices.push_back(static_cast<HighsInt>(problem.matrix[next].column));
                values.push_back(problem.matrix[next].value);
                ++total;
                ++next;
            }
            starts[row + 1] = total;
        }
        return;
    }
    std::map<std::size_t, std::vector<std::pair<std::size_t, double>>> by_row;
    for (const auto &triplet : problem.matrix)
    {
        by_row[triplet.row].emplace_back(triplet.column, triplet.value);
    }
    HighsInt total = 0;
    for (std::size_t row = 0; row < problem.constraint_count; ++row)
    {
        auto it = by_row.find(row);
        if (it == by_row.end())
        {
            starts[row + 1] = total;
            continue;
        }
        auto &row_entries = it->second;
        std::sort(row_entries.begin(), row_entries.end());
        for (const auto &[column, value] : row_entries)
        {
            indices.push_back(static_cast<HighsInt>(column));
            values.push_back(value);
            ++total;
        }
        starts[row + 1] = total;
    }
}

/** @brief HiGHS 模型状态 → 引擎统一状态枚举。 */
SolveStatus map_model_status(HighsModelStatus model_status)
{
    switch (model_status)
    {
    case HighsModelStatus::kOptimal:
        return SolveStatus::OPTIMAL;
    case HighsModelStatus::kInfeasible:
        return SolveStatus::INFEASIBLE;
    case HighsModelStatus::kUnbounded:
        return SolveStatus::UNBOUNDED;
    case HighsModelStatus::kIterationLimit:
    case HighsModelStatus::kTimeLimit:
    case HighsModelStatus::kObjectiveBound:
        return SolveStatus::LIMIT_HIT;
    default:
        return SolveStatus::SOLVER_ERROR;
    }
}

/** @brief 执行求解并组装结果；含无界/不可行歧义的零目标二次探测。 */
SolveOutcome run_highs(Highs &highs)
{
    SolveOutcome outcome;
    if (highs.run() != HighsStatus::kOk)
    {
        // run 失败时模型状态不可信，统一按内部错误处理（默认即 SOLVER_ERROR）
        return outcome;
    }
    const HighsModelStatus model_status = highs.getModelStatus();
    if (model_status == HighsModelStatus::kUnboundedOrInfeasible)
    {
        // 零目标下重解：仍不可行 → 不可行；变为有界可行 → 原问题无界
        const HighsInt column_count = highs.getNumCol();
        std::vector<HighsInt> columns(static_cast<std::size_t>(column_count));
        std::vector<double> zero_costs(static_cast<std::size_t>(column_count), 0.0);
        for (HighsInt column = 0; column < column_count; ++column)
        {
            columns[static_cast<std::size_t>(column)] = column;
        }
        highs.changeColsCost(column_count, columns.data(), zero_costs.data());
        static_cast<void>(highs.run());
        outcome.status = (highs.getModelStatus() == HighsModelStatus::kInfeasible)
                             ? SolveStatus::INFEASIBLE
                             : SolveStatus::UNBOUNDED;
        return outcome;
    }
    outcome.status = map_model_status(model_status);
    if (outcome.status == SolveStatus::OPTIMAL)
    {
        const HighsSolution &solution = highs.getSolution();
        outcome.primal_solution = solution.col_value;
        outcome.objective_value = highs.getObjectiveValue();
        outcome.iteration_count = static_cast<std::size_t>(highs.getInfo().simplex_iteration_count);
    }
    return outcome;
}

/** @brief 系数差异回退阈值：超过则放弃增量、整模重传（07 册 §3.2）。 */
std::size_t value_diff_threshold(std::size_t nnz)
{
    return std::max<std::size_t>(32, nnz / 20);
}
} // namespace

namespace opensuperdea::adapters
{
/** @brief 会话缓存：与上次传入 HiGHS 的模型逐项对应，供指纹比对与差异更新。 */
struct SolveSession::Impl
{
    Highs highs;
    bool has_model = false;   // 已 passModel 至少一次
    bool model_dirty = false; // 求解过程篡改过模型（探测路径），需全量重传

    std::size_t variable_count = 0;
    std::size_t constraint_count = 0;
    bool is_minimization = true;
    bool integrality_present = false;
    std::vector<double> col_cost;
    std::vector<double> col_lower;
    std::vector<double> col_upper;
    std::vector<double> row_lower;
    std::vector<double> row_upper;
    std::vector<HighsInt> starts;
    std::vector<HighsInt> indices;
    std::vector<double> values;
    std::vector<HighsVarType> integrality;

    Impl()
    {
        highs.setOptionValue("output_flag", false);
        // 固定随机种子，保证同输入同输出（SRS 可复现性约束）
        highs.setOptionValue("random_seed", 0);
        // 固定 HiGHS 内层单线程（01 册 §4.4）：HiGHS 默认 threads=0（auto）会启用
        // 全局 HighsTaskExecutor，其 ExecutorHandle 的创建/销毁在多实例并发下存在
        // 数据竞争（TSAN 实证），且并行只应发生在外层 DMU 级（FR-PERF-001）
        highs.setOptionValue("threads", 1);
    }

    /** @brief 全量路径：构造 HighsModel 整模传入并刷新缓存（与新建实例同参）。 */
    SolveOutcome solve_full(const LpProblem &problem,
                            const std::vector<HighsVarType> *integrality_in)
    {
        HighsModel model;
        HighsLp &lp = model.lp_;
        lp.num_col_ = static_cast<HighsInt>(problem.variable_count);
        lp.num_row_ = static_cast<HighsInt>(problem.constraint_count);
        lp.col_cost_ = problem.objective;
        lp.col_lower_ = problem.column_lower_bounds;
        lp.col_upper_ = problem.column_upper_bounds;
        lp.row_lower_ = problem.row_lower_bounds;
        lp.row_upper_ = problem.row_upper_bounds;
        lp.sense_ = problem.is_minimization ? ObjSense::kMinimize : ObjSense::kMaximize;
        lp.offset_ = 0.0;
        lp.a_matrix_.format_ = MatrixFormat::kRowwise;
        build_rowwise_matrix(problem, lp.a_matrix_.start_, lp.a_matrix_.index_,
                             lp.a_matrix_.value_);
        if (integrality_in != nullptr)
        {
            lp.integrality_ = *integrality_in;
        }
        if (highs.passModel(model) != HighsStatus::kOk)
        {
            return SolveOutcome{};
        }
        variable_count = problem.variable_count;
        constraint_count = problem.constraint_count;
        is_minimization = problem.is_minimization;
        col_cost = problem.objective;
        col_lower = problem.column_lower_bounds;
        col_upper = problem.column_upper_bounds;
        row_lower = problem.row_lower_bounds;
        row_upper = problem.row_upper_bounds;
        starts = lp.a_matrix_.start_;
        indices = lp.a_matrix_.index_;
        values = lp.a_matrix_.value_;
        integrality_present = integrality_in != nullptr;
        if (integrality_in != nullptr)
        {
            integrality = *integrality_in;
        }
        else
        {
            integrality.clear();
        }
        has_model = true;
        model_dirty = false;
        return finish_and_run();
    }

    /** @brief 求解收尾：不可行/无界结果可能伴随零目标探测篡改 cost → 置脏下次全量。 */
    SolveOutcome finish_and_run()
    {
        const SolveOutcome outcome = run_highs(highs);
        if (outcome.status == SolveStatus::INFEASIBLE || outcome.status == SolveStatus::UNBOUNDED)
        {
            // 零目标二次探测会把 cost 清零（03 册 §2.3 歧义路径），缓存自此不可信
            model_dirty = true;
        }
        return outcome;
    }

    /** @brief 会话求解主流程：指纹比对 → 增量修改或全量重传 → 冷启动求解。 */
    SolveOutcome solve(const LpProblem &problem, const std::vector<HighsVarType> *integrality_in)
    {
        std::vector<HighsInt> new_starts;
        std::vector<HighsInt> new_indices;
        std::vector<double> new_values;
        build_rowwise_matrix(problem, new_starts, new_indices, new_values);

        const bool has_integrality_request = integrality_in != nullptr;
        const bool integrality_changed =
            has_integrality_request != integrality_present ||
            (has_integrality_request && integrality != *integrality_in);
        const bool structure_same = has_model && !model_dirty && integrality_changed == false &&
                                    variable_count == problem.variable_count &&
                                    constraint_count == problem.constraint_count &&
                                    new_starts == starts && new_indices == indices;

        if (!structure_same)
        {
            return solve_full(problem, integrality_in);
        }

        // ---- 增量路径：逐项比对差异，仅更新变化部分 ----
        if (problem.is_minimization != is_minimization)
        {
            if (highs.changeObjectiveSense(problem.is_minimization
                                               ? ObjSense::kMinimize
                                               : ObjSense::kMaximize) != HighsStatus::kOk)
            {
                return solve_full(problem, integrality_in);
            }
            is_minimization = problem.is_minimization;
        }

        const std::size_t n = problem.variable_count;
        const std::size_t m = problem.constraint_count;
        std::vector<HighsInt> diff_set;
        std::vector<double> diff_lower;
        std::vector<double> diff_upper;
        std::vector<double> diff_value;

        // 目标系数（仅差集）
        diff_set.clear();
        diff_value.clear();
        for (std::size_t j = 0; j < n; ++j)
        {
            if (problem.objective[j] != col_cost[j])
            {
                diff_set.push_back(static_cast<HighsInt>(j));
                diff_value.push_back(problem.objective[j]);
            }
        }
        if (!diff_set.empty())
        {
            if (highs.changeColsCost(static_cast<HighsInt>(diff_set.size()), diff_set.data(),
                                     diff_value.data()) != HighsStatus::kOk)
            {
                return solve_full(problem, integrality_in);
            }
            col_cost = problem.objective;
        }

        // 列界（仅差集）
        diff_set.clear();
        diff_lower.clear();
        diff_upper.clear();
        for (std::size_t j = 0; j < n; ++j)
        {
            if (problem.column_lower_bounds[j] != col_lower[j] ||
                problem.column_upper_bounds[j] != col_upper[j])
            {
                diff_set.push_back(static_cast<HighsInt>(j));
                diff_lower.push_back(problem.column_lower_bounds[j]);
                diff_upper.push_back(problem.column_upper_bounds[j]);
            }
        }
        if (!diff_set.empty())
        {
            if (highs.changeColsBounds(static_cast<HighsInt>(diff_set.size()), diff_set.data(),
                                       diff_lower.data(), diff_upper.data()) != HighsStatus::kOk)
            {
                return solve_full(problem, integrality_in);
            }
            col_lower = problem.column_lower_bounds;
            col_upper = problem.column_upper_bounds;
        }

        // 行界（RHS，仅差集）
        diff_set.clear();
        diff_lower.clear();
        diff_upper.clear();
        for (std::size_t i = 0; i < m; ++i)
        {
            if (problem.row_lower_bounds[i] != row_lower[i] ||
                problem.row_upper_bounds[i] != row_upper[i])
            {
                diff_set.push_back(static_cast<HighsInt>(i));
                diff_lower.push_back(problem.row_lower_bounds[i]);
                diff_upper.push_back(problem.row_upper_bounds[i]);
            }
        }
        if (!diff_set.empty())
        {
            if (highs.changeRowsBounds(static_cast<HighsInt>(diff_set.size()), diff_set.data(),
                                       diff_lower.data(), diff_upper.data()) != HighsStatus::kOk)
            {
                return solve_full(problem, integrality_in);
            }
            row_lower = problem.row_lower_bounds;
            row_upper = problem.row_upper_bounds;
        }

        // 系数值差异（稀疏模式一致，仅值变；通常为 θ 列少数条目）
        std::size_t value_diff_count = 0;
        std::vector<std::pair<HighsInt, HighsInt>> diff_positions;
        diff_value.clear(); // 复用前清空（前段为 cost 差集，残留会使 changeCoeff 取错值）
        for (std::size_t k = 0; k < new_values.size(); ++k)
        {
            if (new_values[k] != values[k])
            {
                ++value_diff_count;
                diff_positions.emplace_back(row_of_entry(new_starts, k),
                                            static_cast<HighsInt>(new_indices[k]));
                diff_value.push_back(new_values[k]);
            }
        }
        if (value_diff_count > value_diff_threshold(new_values.size()))
        {
            return solve_full(problem, integrality_in);
        }
        for (std::size_t k = 0; k < diff_positions.size(); ++k)
        {
            if (highs.changeCoeff(diff_positions[k].first, diff_positions[k].second,
                                  diff_value[k]) != HighsStatus::kOk)
            {
                return solve_full(problem, integrality_in);
            }
        }
        values = new_values;

        // 冷启动：清除基与求解器状态，保证与新建实例同起点（07 册 §3.2）
        if (highs.clearSolver() != HighsStatus::kOk)
        {
            return solve_full(problem, integrality_in);
        }
        return finish_and_run();
    }

    /** @brief CSR 条目序号 → 行号（前缀和上界二分）。 */
    static HighsInt row_of_entry(const std::vector<HighsInt> &starts, std::size_t entry)
    {
        const auto it =
            std::upper_bound(starts.begin(), starts.end(), static_cast<HighsInt>(entry));
        return static_cast<HighsInt>(std::distance(starts.begin(), it) - 1);
    }
};

SolveSession::SolveSession() : impl_(std::make_unique<Impl>())
{
}

SolveSession::~SolveSession() = default;

SolveSession::SolveSession(SolveSession &&) noexcept = default;

SolveSession &SolveSession::operator=(SolveSession &&) noexcept = default;

SolveOutcome SolveSession::solve(const LpProblem &problem)
{
    return impl_->solve(problem, nullptr);
}

SolveOutcome SolveSession::solve(const solver::MilpProblem &problem)
{
    // 引擎侧 0/1 标记直接对应 HighsVarType::kContinuous/kInteger（HConst.h 枚举值约定）
    std::vector<HighsVarType> integrality;
    integrality.reserve(problem.integrality.size());
    for (const int flag : problem.integrality)
    {
        integrality.push_back(flag != 0 ? HighsVarType::kInteger : HighsVarType::kContinuous);
    }
    return impl_->solve(problem, &integrality);
}

SolveOutcome solve_lp(const LpProblem &problem)
{
    // thread_local 会话（FR-PERF-004）：worker 线程内跨 DMU 复用，池销毁即析构；
    // 指纹失配/模型篡改时自动回退全量路径，语义与新建实例逐项一致
    thread_local SolveSession session;
    return session.solve(problem);
}

SolveOutcome solve_milp(const solver::MilpProblem &problem)
{
    thread_local SolveSession session;
    return session.solve(problem);
}

SolveOutcome solve_lp_fresh(const LpProblem &problem)
{
    SolveSession session; // 新会话首次 solve 即新建实例全量路径（对照组语义）
    return session.solve(problem);
}
} // namespace opensuperdea::adapters
