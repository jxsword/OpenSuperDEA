/**
 * @file highs_adapter_test.cpp
 * @brief HiGHS 适配层单元测试（SRS FR-ENG-002）。
 * @details 覆盖：3 个已知解 LP + 1 个 MILP 手算比对 + 不可行/无界状态映射；
 *           用例名遵循 FR 追溯约定（design_docs/04 册 §7）。
 */
#include "adapters/highs_adapter.h"

#include <gtest/gtest.h>

#include <utility>
#include <vector>

namespace
{
using opensuperdea::adapters::solve_lp;
using opensuperdea::adapters::solve_milp;
using opensuperdea::solver::LpProblem;
using opensuperdea::solver::MatrixTriplet;
using opensuperdea::solver::MilpProblem;
using opensuperdea::solver::NEGATIVE_INFINITY;
using opensuperdea::solver::POSITIVE_INFINITY;
using opensuperdea::solver::SolveStatus;

/** @brief 通用构造器：列变量默认 [0, +inf)，行界与矩阵由调用方给定。 */
LpProblem make_problem(std::vector<double> objective, bool is_minimization,
                       std::vector<MatrixTriplet> matrix, std::vector<double> row_lower,
                       std::vector<double> row_upper)
{
    LpProblem problem;
    problem.variable_count = objective.size();
    problem.constraint_count = row_lower.size();
    problem.is_minimization = is_minimization;
    problem.objective = std::move(objective);
    problem.matrix = std::move(matrix);
    problem.row_lower_bounds = std::move(row_lower);
    problem.row_upper_bounds = std::move(row_upper);
    problem.column_lower_bounds.assign(problem.variable_count, 0.0);
    problem.column_upper_bounds.assign(problem.variable_count, POSITIVE_INFINITY);
    return problem;
}
} // namespace

/** @brief 最小化 LP 唯一最优解：min x+2y, x+y>=1, 2x+y>=1 → (1,0), obj=1。 */
TEST(HighsAdapter, FR_ENG_002_LpMinUniqueOptimum)
{
    const auto problem = make_problem({1.0, 2.0}, true,
                                      {{.row = 0, .column = 0, .value = 1.0},
                                       {.row = 0, .column = 1, .value = 1.0},
                                       {.row = 1, .column = 0, .value = 2.0},
                                       {.row = 1, .column = 1, .value = 1.0}},
                                      {1.0, 1.0}, {POSITIVE_INFINITY, POSITIVE_INFINITY});
    const auto outcome = solve_lp(problem);
    ASSERT_EQ(outcome.status, SolveStatus::OPTIMAL);
    EXPECT_DOUBLE_EQ(outcome.objective_value, 1.0);
    ASSERT_EQ(outcome.primal_solution.size(), 2U);
    EXPECT_NEAR(outcome.primal_solution[0], 1.0, 1e-9);
    EXPECT_NEAR(outcome.primal_solution[1], 0.0, 1e-9);
}

/** @brief 最大化有界 LP：max 2x+3y, x+y<=4, x+3y<=6 → (3,1), obj=9。 */
TEST(HighsAdapter, FR_ENG_002_LpMaxBounded)
{
    const auto problem = make_problem({2.0, 3.0}, false,
                                      {{.row = 0, .column = 0, .value = 1.0},
                                       {.row = 0, .column = 1, .value = 1.0},
                                       {.row = 1, .column = 0, .value = 1.0},
                                       {.row = 1, .column = 1, .value = 3.0}},
                                      {NEGATIVE_INFINITY, NEGATIVE_INFINITY}, {4.0, 6.0});
    const auto outcome = solve_lp(problem);
    ASSERT_EQ(outcome.status, SolveStatus::OPTIMAL);
    EXPECT_DOUBLE_EQ(outcome.objective_value, 9.0);
    ASSERT_EQ(outcome.primal_solution.size(), 2U);
    EXPECT_NEAR(outcome.primal_solution[0], 3.0, 1e-9);
    EXPECT_NEAR(outcome.primal_solution[1], 1.0, 1e-9);
}

/** @brief 不可行 LP：x>=2 且 x<=1 → INFEASIBLE。 */
TEST(HighsAdapter, FR_ENG_002_LpInfeasible)
{
    const auto problem =
        make_problem({1.0, 1.0}, true,
                     {{.row = 0, .column = 0, .value = 1.0}, {.row = 1, .column = 0, .value = 1.0}},
                     {2.0, NEGATIVE_INFINITY}, {POSITIVE_INFINITY, 1.0});
    const auto outcome = solve_lp(problem);
    EXPECT_EQ(outcome.status, SolveStatus::INFEASIBLE);
}

/** @brief 无界 LP：max x, x+y>=1, x-y<=1, x,y>=0 → UNBOUNDED（零目标二次探测路径）。 */
TEST(HighsAdapter, FR_ENG_002_LpUnbounded)
{
    const auto problem = make_problem({1.0, 0.0}, false,
                                      {{.row = 0, .column = 0, .value = 1.0},
                                       {.row = 0, .column = 1, .value = 1.0},
                                       {.row = 1, .column = 0, .value = 1.0},
                                       {.row = 1, .column = 1, .value = -1.0}},
                                      {1.0, NEGATIVE_INFINITY}, {POSITIVE_INFINITY, 1.0});
    const auto outcome = solve_lp(problem);
    EXPECT_EQ(outcome.status, SolveStatus::UNBOUNDED);
}

/** @brief MILP 整数生效：max x, 2x<=3, x 整数 → x=1（LP 松弛为 1.5）。 */
TEST(HighsAdapter, FR_ENG_002_MilpIntegerRounding)
{
    MilpProblem problem;
    problem.variable_count = 2;
    problem.constraint_count = 2;
    problem.is_minimization = false;
    problem.objective = {1.0, 0.0};
    problem.matrix = {{.row = 0, .column = 0, .value = 2.0},
                      {.row = 1, .column = 0, .value = 1.0},
                      {.row = 1, .column = 1, .value = 1.0}};
    problem.row_lower_bounds = {NEGATIVE_INFINITY, NEGATIVE_INFINITY};
    problem.row_upper_bounds = {3.0, 10.0};
    problem.column_lower_bounds = {0.0, 0.0};
    problem.column_upper_bounds = {POSITIVE_INFINITY, POSITIVE_INFINITY};
    problem.integrality = {1, 0};

    const auto outcome = solve_milp(problem);
    ASSERT_EQ(outcome.status, SolveStatus::OPTIMAL);
    EXPECT_DOUBLE_EQ(outcome.objective_value, 1.0);
    ASSERT_EQ(outcome.primal_solution.size(), 2U);
    EXPECT_NEAR(outcome.primal_solution[0], 1.0, 1e-9);
}
