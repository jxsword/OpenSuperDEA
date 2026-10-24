/**
 * @file solvable_model.h
 * @brief 可求解模型的 C++20 concept 与求解函数类型擦除（design_docs/06 册 §1 核心设计）。
 * @details ISolvableModel 为 concept 而非虚基类：接口遗漏在编译期报错，注册表经
 *          工厂 lambda 捕获具体类型做类型擦除，无虚函数开销。SolveContext 一次性
 *          装配求解所需的全部只读上下文。
 */
#pragma once

#include <concepts>
#include <cstddef>
#include <functional>
#include <vector>

#include "adapters/highs_adapter.h"
#include "core/dataset/dataset.h"
#include "core/result/dmu_result.h"
#include "core/solver/lp_problem.h"
#include "solve_options.h"

namespace opensuperdea::registry
{
/** @brief 一次逐 DMU 求解的只读上下文（06 册 §1：数据 + 被评 DMU + 已选参考集 + 选项）。 */
struct SolveContext
{
    const dataset::Dataset *data = nullptr;     // 数据集（模型只读）
    std::size_t dmu_index = 0;                  // 被评 DMU 行索引
    std::vector<std::size_t> reference_indices; // 参考集选择器已选索引
    SolveOptions options;                       // 定向/规模报酬/模型选项
};

/**
 * @brief 可求解模型 concept（06 册 §1 原文签名）：
 *        build_problem 产出中立 LP，assemble_result 把求解结果组装为 DmuResult。
 * @tparam M 候选模型类型
 */
template <typename M>
concept SolvableModel =
    requires(M model, const SolveContext &context, solver::SolveOutcome &outcome) {
        { model.build_problem(context) } -> std::same_as<solver::LpProblem>;
        { model.assemble_result(context, outcome) } -> std::same_as<result::DmuResult>;
    };

/**
 * @brief 类型擦除的求解函数：给定上下文返回该 DMU 的结果。
 * @details 注册表存储本函数对象；构建-求解-组装的流水线封装在工厂 lambda 内。
 */
using SolveFunction = std::function<result::DmuResult(const SolveContext &)>;

/**
 * @brief 由满足 concept 的具体模型类型生成求解函数（工厂 lambda 捕获具体类型）。
 * @tparam M 满足 SolvableModel 的模型类型
 * @return 可注册进 ModelDescriptor 的求解函数
 */
template <SolvableModel M> SolveFunction make_solve_function()
{
    return [](const SolveContext &context) -> result::DmuResult {
        M model{};
        const solver::LpProblem problem = model.build_problem(context);
        const solver::SolveOutcome outcome = adapters::solve_lp(problem);
        return model.assemble_result(context, outcome);
    };
}
} // namespace opensuperdea::registry
