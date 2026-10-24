/**
 * @file highs_adapter.h
 * @brief HiGHS LP/MILP 求解适配层（SRS FR-ENG-002）。
 * @details 本目录为第三方 API 直调豁免区（DR-004）：HiGHS 头文件仅出现在本目录
 *          的 .cpp 中；对外只暴露 core/solver 的中立类型，业务代码与求解器解耦。
 *          求解失败不抛异常，统一经 SolveOutcome.status 表达。
 */
#pragma once

#include "core/solver/lp_problem.h"

namespace opensuperdea::adapters
{
/**
 * @brief 求解 LP 问题。
 * @param problem 已构造完成的线性规划问题（维度与数组长度需自洽）
 * @return 求解结果（状态 + 原像解 + 目标值）
 */
solver::SolveOutcome solve_lp(const solver::LpProblem& problem);

/**
 * @brief 求解 MILP 问题（变量整数性来自 problem.integrality）。
 * @param problem 含整数性标记的混合整数线性规划问题
 * @return 求解结果
 */
solver::SolveOutcome solve_milp(const solver::MilpProblem& problem);
} // namespace opensuperdea::adapters
