/**
 * @file highs_adapter.cpp
 * @brief HiGHS 求解调用与状态映射实现。
 * @details 构造流程：三元组按行聚合为 CSR → HighsModel → passModel → run；
 *          kUnboundedOrInfeasible 状态下以零目标二次探测区分不可行与无界
 *          （design_docs/03 册 §2.3 状态映射约定）。
 */
#include "highs_adapter.h"

#include <Highs.h>

#include <algorithm>
#include <map>
#include <utility>
#include <vector>

namespace
{
using opensuperdea::solver::LpProblem;
using opensuperdea::solver::SolveOutcome;
using opensuperdea::solver::SolveStatus;

/** @brief 将稀疏三元组按行聚合为 CSR（start 为前缀和，同行内按列排序保证确定性）。 */
void build_rowwise_matrix(const LpProblem& problem,
                          std::vector<HighsInt>& starts,
                          std::vector<HighsInt>& indices,
                          std::vector<double>& values)
{
    std::map<std::size_t, std::vector<std::pair<std::size_t, double>>> by_row;
    for (const auto& triplet : problem.matrix)
    {
        by_row[triplet.row].emplace_back(triplet.column, triplet.value);
    }
    starts.assign(problem.constraint_count + 1, 0);
    indices.clear();
    values.clear();
    HighsInt total = 0;
    for (std::size_t row = 0; row < problem.constraint_count; ++row)
    {
        auto it = by_row.find(row);
        if (it == by_row.end())
        {
            starts[row + 1] = total;
            continue;
        }
        auto& row_entries = it->second;
        std::sort(row_entries.begin(), row_entries.end());
        for (const auto& [column, value] : row_entries)
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
SolveOutcome run_highs(Highs& highs)
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
        const HighsSolution& solution = highs.getSolution();
        outcome.primal_solution = solution.col_value;
        outcome.objective_value = highs.getObjectiveValue();
        outcome.iteration_count =
            static_cast<std::size_t>(highs.getInfo().simplex_iteration_count);
    }
    return outcome;
}

/** @brief 组装 HighsModel 并求解；integrality 非空时按 MILP 传入。 */
SolveOutcome solve_with_integrality(const LpProblem& problem,
                                    const std::vector<HighsVarType>* integrality)
{
    Highs highs;
    highs.setOptionValue("output_flag", false);
    // 固定随机种子，保证同输入同输出（SRS 可复现性约束）
    highs.setOptionValue("random_seed", 0);

    HighsModel model;
    HighsLp& lp = model.lp_;
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
    build_rowwise_matrix(problem, lp.a_matrix_.start_, lp.a_matrix_.index_, lp.a_matrix_.value_);
    if (integrality != nullptr)
    {
        lp.integrality_ = *integrality;
    }
    if (highs.passModel(model) != HighsStatus::kOk)
    {
        return SolveOutcome{};
    }
    return run_highs(highs);
}
} // namespace

namespace opensuperdea::adapters
{
solver::SolveOutcome solve_lp(const solver::LpProblem& problem)
{
    return solve_with_integrality(problem, nullptr);
}

solver::SolveOutcome solve_milp(const solver::MilpProblem& problem)
{
    // 引擎侧 0/1 标记直接对应 HighsVarType::kContinuous/kInteger（HConst.h 枚举值约定）
    std::vector<HighsVarType> integrality;
    integrality.reserve(problem.integrality.size());
    for (const int flag : problem.integrality)
    {
        integrality.push_back(flag != 0 ? HighsVarType::kInteger : HighsVarType::kContinuous);
    }
    return solve_with_integrality(problem, &integrality);
}
} // namespace opensuperdea::adapters
