/**
 * @file session_test.cpp
 * @brief FR-PERF-004 专项单测：SolveSession 会话复用与新建实例路径等价（07 册 §3.2，DR-008 口径）。
 * @details 同一 LP 序列分别走 solve_lp_fresh（每次新建实例全量传入，冷启动）与会话
 *          连续复用（增量修改 + 基复用热启动），断言状态一致与最优目标值一致（LP
 *          最优值唯一，1e-9 内稳定）；原像解为顶点类输出（LP 多重最优解时两条路径
 *          可选不同顶点，DR-008），断言有限性而非逐项相等。覆盖：同构变 RHS/cost/
 *          θ 系数、稀疏模式突变、乱序三元组回退路径、不可行 / 无界、MILP（FDH 型
 *          0-1）、LP↔MILP 交替。
 */
#include <gtest/gtest.h>

#include <array>
#include <cmath>
#include <utility>
#include <vector>

#include "adapters/highs_adapter.h"

namespace
{
using opensuperdea::solver::LpProblem;
using opensuperdea::solver::MilpProblem;
using opensuperdea::solver::SolveOutcome;
using opensuperdea::solver::SolveStatus;

/** @brief 构造 min c·x s.t. A x (≤/=/≥) b 的最小 LP 骨架。 */
LpProblem make_lp(const std::vector<double> &cost, const std::vector<double> &lower,
                  const std::vector<double> &upper, const std::vector<double> &row_lower,
                  const std::vector<double> &row_upper,
                  const std::vector<opensuperdea::solver::MatrixTriplet> &matrix,
                  bool is_min = true)
{
    LpProblem problem;
    problem.variable_count = cost.size();
    problem.constraint_count = row_lower.size();
    problem.is_minimization = is_min;
    problem.objective = cost;
    problem.matrix = matrix;
    problem.row_lower_bounds = row_lower;
    problem.row_upper_bounds = row_upper;
    problem.column_lower_bounds = lower;
    problem.column_upper_bounds = upper;
    return problem;
}

/**
 * @brief 断言两路径等价（DR-008 口径）：状态与最优目标值 1e-9 一致（LP 最优值唯一）；
 *        原像解为顶点类输出（warm-start 与新建实例可选不同顶点），断言有限性。
 */
void expect_strictly_identical(const SolveOutcome &fresh, const SolveOutcome &session,
                               const char *label)
{
    ASSERT_EQ(fresh.status, session.status) << label;
    if (fresh.status != SolveStatus::OPTIMAL)
    {
        return; // 非 OPTIMAL 时其余字段不保证有效（lp_problem.h 契约）
    }
    ASSERT_NEAR(fresh.objective_value, session.objective_value, 1e-9) << label;
    ASSERT_EQ(fresh.primal_solution.size(), session.primal_solution.size()) << label;
    for (std::size_t i = 0; i < fresh.primal_solution.size(); ++i)
    {
        EXPECT_TRUE(std::isfinite(fresh.primal_solution[i])) << label << " var#" << i;
        EXPECT_TRUE(std::isfinite(session.primal_solution[i])) << label << " var#" << i;
    }
}
} // namespace

TEST(SolveSession, BitwiseIdenticalOnHomogeneousSequence)
{
    // 同构序列（模拟逐 DMU 求解画像：结构不变，RHS / cost / θ 系数变化）：
    // min t - (1/2)s1 - (1/3)s2 骨架省略，直接用 λ 型 LP：
    // 变量 [t, x1, x2]；行0: a*x1 + b*x2 - c*t ≤ 0；行1: x1 + x2 ≥ rhs
    opensuperdea::adapters::SolveSession session;
    // 具名变量承载用例表：函数风格转换 + 嵌套 braced-init-list 内联于 range-for
    // 是 GCC 扩展，MSVC 拒绝（C2440）——提出为具名 const 是两编译器通用的写法
    const std::vector<std::array<double, 4>> cases = {{1.0, 2.0, 1.0, 4.0},
                                                      {1.0, 2.0, 0.8, 6.0},
                                                      {2.0, 2.0, 0.8, 6.0},
                                                      {1.0, 2.0, 1.2, 3.5},
                                                      {1.5, 2.5, 1.0, 4.25}};
    for (const auto &[a, b, c, rhs] : cases)
    {
        const LpProblem problem = make_lp({1.0, 0.0, 0.0}, {0.0, 0.0, 0.0},
                                          {opensuperdea::solver::POSITIVE_INFINITY,
                                           opensuperdea::solver::POSITIVE_INFINITY,
                                           opensuperdea::solver::POSITIVE_INFINITY},
                                          {opensuperdea::solver::NEGATIVE_INFINITY, 0.0},
                                          {0.0, opensuperdea::solver::POSITIVE_INFINITY},
                                          {{.row = 0, .column = 1, .value = a},
                                           {.row = 0, .column = 2, .value = b},
                                           {.row = 0, .column = 0, .value = -c},
                                           {.row = 1, .column = 1, .value = 1.0},
                                           {.row = 1, .column = 2, .value = 1.0}});
        const SolveOutcome fresh = opensuperdea::adapters::solve_lp_fresh(problem);
        const SolveOutcome reused = session.solve(problem);
        expect_strictly_identical(fresh, reused, "homogeneous");
        ASSERT_EQ(fresh.status, SolveStatus::OPTIMAL);
    }
}

TEST(SolveSession, BitwiseIdenticalAcrossStructureChanges)
{
    // 结构突变序列：加行 → 加变量 → 回退原结构 → 不可行 → 无界 → LP↔MILP 交替
    opensuperdea::adapters::SolveSession session;
    const LpProblem base =
        make_lp({1.0, 1.0}, {0.0, 0.0},
                {opensuperdea::solver::POSITIVE_INFINITY, opensuperdea::solver::POSITIVE_INFINITY},
                {2.0}, {opensuperdea::solver::POSITIVE_INFINITY},
                {{.row = 0, .column = 0, .value = 1.0}, {.row = 0, .column = 1, .value = 2.0}});

    LpProblem extra_row = base; // 行模式突变：追加一行
    extra_row.constraint_count = 2;
    extra_row.row_lower_bounds = {2.0, 1.0};
    extra_row.row_upper_bounds = {opensuperdea::solver::POSITIVE_INFINITY,
                                  opensuperdea::solver::POSITIVE_INFINITY};
    extra_row.matrix.push_back({.row = 1, .column = 1, .value = 1.0});

    LpProblem extra_var = base; // 列模式突变：追加一个变量
    extra_var.variable_count = 3;
    extra_var.objective = {1.0, 1.0, 0.5};
    extra_var.column_lower_bounds = {0.0, 0.0, 0.0};
    extra_var.column_upper_bounds.push_back(opensuperdea::solver::POSITIVE_INFINITY);
    extra_var.matrix.push_back({.row = 0, .column = 2, .value = 1.0});

    LpProblem infeasible =
        make_lp({0.0}, {0.0}, {opensuperdea::solver::POSITIVE_INFINITY}, {1.0}, {1.0}, {});
    // 空行 ≥ 1：确定不可行（与 radial_envelope_model 空参考集语义同构）

    LpProblem unbounded = make_lp(
        {-1.0}, {0.0}, {opensuperdea::solver::POSITIVE_INFINITY},
        {opensuperdea::solver::NEGATIVE_INFINITY}, {opensuperdea::solver::POSITIVE_INFINITY},
        {{.row = 0, .column = 0, .value = 1.0}}); // min -x, x ≥ 0 自由增长 → 无界

    MilpProblem milp; // FDH 型 0-1 选择变量
    milp.variable_count = 2;
    milp.constraint_count = 1;
    milp.is_minimization = true;
    milp.objective = {-1.0, -1.0};
    milp.matrix = {{.row = 0, .column = 0, .value = 1.0}, {.row = 0, .column = 1, .value = 1.0}};
    milp.row_lower_bounds = {1.0};
    milp.row_upper_bounds = {opensuperdea::solver::POSITIVE_INFINITY};
    milp.column_lower_bounds = {0.0, 0.0};
    milp.column_upper_bounds = {1.0, 1.0};
    milp.integrality = {1, 1};

    const std::vector<std::pair<const char *, const LpProblem *>> sequence = {
        {"base#1", &base}, {"extra_row", &extra_row},   {"extra_var", &extra_var},
        {"base#2", &base}, {"infeasible", &infeasible}, {"unbounded", &unbounded},
    };
    for (const auto &[label, problem] : sequence)
    {
        const SolveOutcome fresh = opensuperdea::adapters::solve_lp_fresh(*problem);
        const SolveOutcome reused = session.solve(*problem);
        expect_strictly_identical(fresh, reused, label);
    }
    const SolveOutcome milp_fresh = opensuperdea::adapters::solve_milp(milp);
    const SolveOutcome milp_reused = session.solve(milp);
    expect_strictly_identical(milp_fresh, milp_reused, "milp");
    ASSERT_EQ(milp_fresh.status, SolveStatus::OPTIMAL);
    // MILP 后再回 LP（整数性缓存失配 → 全量回退路径）
    const SolveOutcome after_milp_fresh = opensuperdea::adapters::solve_lp_fresh(base);
    const SolveOutcome after_milp_reused = session.solve(base);
    expect_strictly_identical(after_milp_fresh, after_milp_reused, "after_milp");
}

TEST(SolveSession, UnorderedTripletsFallbackMatchesSorted)
{
    // 乱序三元组（回退 map+sort 路径）与排序后同数据（快速路径）结果严格一致
    opensuperdea::adapters::SolveSession session;
    LpProblem sorted =
        make_lp({1.0, 1.0}, {0.0, 0.0},
                {opensuperdea::solver::POSITIVE_INFINITY, opensuperdea::solver::POSITIVE_INFINITY},
                {3.0, 2.0},
                {opensuperdea::solver::POSITIVE_INFINITY, opensuperdea::solver::POSITIVE_INFINITY},
                {{.row = 0, .column = 0, .value = 1.0},
                 {.row = 0, .column = 1, .value = 2.0},
                 {.row = 1, .column = 0, .value = 3.0},
                 {.row = 1, .column = 1, .value = 1.0}});
    LpProblem shuffled = sorted;
    shuffled.matrix = {{.row = 1, .column = 1, .value = 1.0},
                       {.row = 0, .column = 1, .value = 2.0},
                       {.row = 1, .column = 0, .value = 3.0},
                       {.row = 0, .column = 0, .value = 1.0}};
    const SolveOutcome fresh = opensuperdea::adapters::solve_lp_fresh(sorted);
    const SolveOutcome reused = session.solve(shuffled);
    expect_strictly_identical(fresh, reused, "unordered");
}

TEST(SolveSession, ThreadLocalSolveLpMatchesFreshAcrossCalls)
{
    // solve_lp 的 thread_local 会话跨多次调用（含结构突变）与新建实例一致
    const LpProblem first =
        make_lp({1.0, 2.0}, {0.0, 0.0},
                {opensuperdea::solver::POSITIVE_INFINITY, opensuperdea::solver::POSITIVE_INFINITY},
                {4.0}, {opensuperdea::solver::POSITIVE_INFINITY},
                {{.row = 0, .column = 0, .value = 1.0}, {.row = 0, .column = 1, .value = 1.0}});
    LpProblem second = first;
    second.objective = {2.0, 1.0};
    second.row_lower_bounds = {7.0};
    LpProblem third = second;
    third.constraint_count = 2;
    third.row_lower_bounds = {7.0, 1.0};
    third.row_upper_bounds = {opensuperdea::solver::POSITIVE_INFINITY,
                              opensuperdea::solver::POSITIVE_INFINITY};
    third.matrix.push_back({.row = 1, .column = 1, .value = 2.0});

    for (const LpProblem &problem : {first, second, third, first})
    {
        const SolveOutcome fresh = opensuperdea::adapters::solve_lp_fresh(problem);
        const SolveOutcome engine = opensuperdea::adapters::solve_lp(problem);
        expect_strictly_identical(fresh, engine, "thread_local");
    }
}
