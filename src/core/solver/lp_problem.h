/**
 * @file lp_problem.h
 * @brief 与求解器无关的 LP/MILP 问题统一描述（引擎侧求解接口）。
 * @details 第三方求解器（HiGHS）仅在 src/adapters/ 内接触（DR-004 隔离约定），
 *          本头文件不包含任何第三方头，保证 CLI/GUI/测试可直接消费引擎类型。
 *          约束矩阵以稀疏三元组描述，M3/M4 大规模路径直接复用该稀疏表示
 *          （SRS FR-PERF-004）。
 * @note 无穷界限使用 std::numeric_limits<double>::infinity() 表达。
 */
#pragma once

#include <cstddef>
#include <cstdint>
#include <limits>
#include <vector>

namespace opensuperdea::solver
{
/** @brief 表示正/负无穷界限的常量（构造问题时使用）。 */
inline constexpr double POSITIVE_INFINITY = std::numeric_limits<double>::infinity();
inline constexpr double NEGATIVE_INFINITY = -std::numeric_limits<double>::infinity();

/** @brief 约束矩阵稀疏三元组（0 基行列索引）。 */
struct MatrixTriplet
{
    std::size_t row;
    std::size_t column;
    double value;
};

/** @brief 线性规划问题（variable_count 个列变量 × constraint_count 个行约束）。 */
struct LpProblem
{
    std::size_t variable_count = 0;
    std::size_t constraint_count = 0;
    bool is_minimization = true;
    std::vector<double> objective;           // 长度 variable_count
    std::vector<MatrixTriplet> matrix;       // 行约束稀疏系数
    std::vector<double> row_lower_bounds;    // 长度 constraint_count
    std::vector<double> row_upper_bounds;    // 长度 constraint_count
    std::vector<double> column_lower_bounds; // 长度 variable_count
    std::vector<double> column_upper_bounds; // 长度 variable_count
};

/** @brief 混合整数线性规划问题：在 LP 之上追加变量整数性标记。 */
struct MilpProblem : public LpProblem
{
    std::vector<int> integrality; // 长度 variable_count；0 连续、1 整数
};

/** @brief 求解状态：适配层将求解器原始状态统一映射到此枚举（design_docs/03 册 §2.3）。 */
enum class SolveStatus : std::uint8_t
{
    OPTIMAL,
    INFEASIBLE,
    UNBOUNDED,
    LIMIT_HIT,
    SOLVER_ERROR
};

/** @brief 求解结果：原像解 + 目标值 + 状态（非 OPTIMAL 时其余字段不保证有效）。 */
struct SolveOutcome
{
    SolveStatus status = SolveStatus::SOLVER_ERROR;
    std::vector<double> primal_solution; // 长度 variable_count
    double objective_value = 0.0;
    std::size_t iteration_count = 0; // 0 表示求解器未报告
};
} // namespace opensuperdea::solver
