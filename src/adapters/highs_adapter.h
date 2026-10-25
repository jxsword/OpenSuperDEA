/**
 * @file highs_adapter.h
 * @brief HiGHS LP/MILP 求解适配层（SRS FR-ENG-002）。
 * @details 本目录为第三方 API 直调豁免区（DR-004）：HiGHS 头文件仅出现在本目录
 *          的 .cpp 中；对外只暴露 core/solver 的中立类型，业务代码与求解器解耦。
 *          求解失败不抛异常，统一经 SolveOutcome.status 表达。
 *          FR-PERF-004（07 册 §3）：solve_lp/solve_milp 经 thread_local 会话复用
 *          HiGHS 实例，结构指纹一致时增量修改差异项并冷启动求解，与新建实例
 *          路径逐项同参（bit 级一致由专项单测锁定）。
 */
#pragma once

#include <memory>

#include "core/solver/lp_problem.h"

namespace opensuperdea::adapters
{
/**
 * @brief 求解 LP 问题（thread_local 会话复用，FR-PERF-004）。
 * @param problem 已构造完成的线性规划问题（维度与数组长度需自洽）
 * @return 求解结果（状态 + 原像解 + 目标值）
 */
solver::SolveOutcome solve_lp(const solver::LpProblem &problem);

/**
 * @brief 求解 MILP 问题（变量整数性来自 problem.integrality）。
 * @param problem 含整数性标记的混合整数线性规划问题
 * @return 求解结果
 */
solver::SolveOutcome solve_milp(const solver::MilpProblem &problem);

/**
 * @brief 求解参考路径：每次调用新建 HiGHS 实例全量传入（07 册 §3.2 对照组）。
 * @details 供会话复用 bit 级一致专项单测与降级开关使用；业务代码请走 solve_lp/solve_milp。
 */
solver::SolveOutcome solve_lp_fresh(const solver::LpProblem &problem);

/**
 * @brief 会话式求解器：跨多次 solve 复用 HiGHS 实例与模型缓存（FR-PERF-004，07 册 §3.2）。
 * @details 结构指纹（维度 + 稀疏模式 + 整数性）一致时仅增量修改差异项
 *          （changeCoeff/changeRowsBounds/changeColsCost/changeColsBounds）并
 *          clearSolver() 冷启动；不一致时回退 passModel 全量（与新建实例同参）。
 *          不可拷贝（持有 HiGHS 实例）；线程安全策略 = 每线程一个实例。
 */
class SolveSession
{
  public:
    SolveSession();
    ~SolveSession();
    SolveSession(const SolveSession &) = delete;
    SolveSession &operator=(const SolveSession &) = delete;
    SolveSession(SolveSession &&) noexcept;
    SolveSession &operator=(SolveSession &&) noexcept;

    /**
     * @brief 求解一个 LP（与 solve_lp 同参同果，区别仅在实例复用）。
     */
    solver::SolveOutcome solve(const solver::LpProblem &problem);

    /**
     * @brief 求解一个 MILP（integrality 与会话缓存不一致时回退全量）。
     */
    solver::SolveOutcome solve(const solver::MilpProblem &problem);

  private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
} // namespace opensuperdea::adapters
