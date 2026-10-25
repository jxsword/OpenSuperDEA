/**
 * @file model_runner.h
 * @brief 模型运行器：注册表查询 → 组合校验 → 逐 DMU 求解 → 结果聚合（06 册 §1 流水线）。
 * @details run 子命令与未来 GUI 共用本入口（01 册「单一引擎源」）；超效率经
 *          RunConfig 切换参考集（exclude_self，06 册 §3.2）。
 */
#pragma once

#include <string>
#include <vector>

#include "core/dataset/dataset.h"
#include "core/result/model_result.h"
#include "engine_error.h"
#include "solve_options.h"

namespace opensuperdea::registry
{
/** @brief 一次运行的附加配置（不进入模型 LP 的元信息）。 */
struct RunConfig
{
    bool super_efficiency = false; // 参考集切为 exclude_self（06 册 §3.2）
    std::string data_sha1;         // 数据文件指纹（02 册 §4.2；可空）
    std::string version;           // 引擎版本回显（FR-CLI-001 同源）
    unsigned int thread_count = 0; // 并行度（FR-PERF-001，07 册 §2.5）：0=自动；1=串行退化
};

/**
 * @brief 运行一个已注册模型，产出全部 DMU 的结果与汇总。
 * @param model_id 模型标识
 * @param data 截面数据集
 * @param options 定向/规模报酬/模型选项
 * @param config 附加配置
 * @return 聚合结果（dmu_results 与数据集顺序一致）
 * @throws EngineException 未知模型/非法组合（EXIT_ARG）或求解失败（EXIT_SOLVE）
 */
[[nodiscard]] result::ModelResult run_model(const std::string &model_id,
                                            const dataset::Dataset &data,
                                            const SolveOptions &options, const RunConfig &config);
} // namespace opensuperdea::registry
