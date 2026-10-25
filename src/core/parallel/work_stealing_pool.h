/**
 * @file work_stealing_pool.h
 * @brief DMU 级并行执行入口（FR-PERF-001，07 册 §2.2）。
 * @details 工作窃取线程池的唯一公共接口为 parallel_for：按索引并行执行任务体，
 *          任务体异常在全部任务汇合后以首个为准重抛（EngineException 语义透传）。
 *          thread_count=1 时退化为串行 for（PR-09 的对照路径与保守回退开关）；
 *          任务体须线程安全（共享只读 + 各自独立写入，SolveContext 全只读契约）。
 */
#pragma once

#include <cstddef>
#include <functional>

namespace opensuperdea::parallel
{
/**
 * @brief 按索引区间并行执行任务体。
 * @param count 任务总数，对每个 i ∈ [0, count) 执行 body(i)
 * @param thread_count 线程数：0 = 自动（hardware_concurrency，异常时取 1）；1 = 串行退化
 * @param body 任务体（不得捕获会被并发修改的状态）
 * @throws body 抛出的首个异常（其余任务快速跳过后在本函数内重抛）
 */
void parallel_for(std::size_t count, unsigned int thread_count,
                  const std::function<void(std::size_t)> &body);
} // namespace opensuperdea::parallel
