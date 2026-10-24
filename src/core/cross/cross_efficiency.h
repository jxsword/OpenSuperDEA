/**
 * @file cross_efficiency.h
 * @brief 交叉效率：两阶段（CCR 自评 → 二次目标）+ n×n 矩阵（FR-CROSS-001~004；06 册 §8）。
 * @details 第二阶段形式与 deaR cross_efficiency 严格对齐（M2 = Doyle-Green）：
 *          约束 u·y_j − v·x_j + ω ≤ 0（∀j）、v·x_i = 1、u·y_i = e_ii、
 *          Σ变量 ≤ 1e10（deaR 同款防无界界）；目标 min/max Σ_{j≠i}(u·y_j − v·x_j)。
 *          M3（Ruiz 类修正，FR-CROSS-002 映射 deaR M3）：归一化 v·Σ_{j≠i}x_j = 1、
 *          u·y_i = e_ii·v·x_i、目标 min/max Σ_{j≠i} u·y_j。
 *          虚拟 DMU（FR-CROSS-003）：第二阶段目标换为 min/max (u·y_V − v·x_V)，
 *          虚拟 DMU 投入/产出向量经 CLI 提供。
 *          本模块为整表级流水线（非逐 DMU concept）：第一阶段逐 DMU 自评 +
 *          第二阶段逐评者求解，产出 CrossMatrix + 列均值排名。
 */
#pragma once

#include <cstdint>
#include <vector>

#include "core/dataset/dataset.h"
#include "core/registry/model_runner.h"
#include "core/result/model_result.h"

namespace opensuperdea::cross
{
/** @brief 第二阶段方法（FR-CROSS-001~003）。 */
enum class CrossMethod : std::uint8_t
{
    DG_AGGRESSIVE, /**< Doyle-Green 攻击型（deaR M2_agg） */
    DG_BENEVOLENT, /**< Doyle-Green 仁慈型（deaR M2_ben） */
    M3_AGGRESSIVE, /**< M3 修正攻击型（Ruiz 类，deaR M3_agg） */
    M3_BENEVOLENT, /**< M3 修正仁慈型（deaR M3_ben） */
    VIRTUAL_DMU    /**< 自定义虚拟 DMU 目标（FR-CROSS-003） */
};

/** @brief 交叉效率运行选项（仅投入导向 io 口径，MaxDEA/deaR 惯例）。 */
struct CrossRunOptions
{
    CrossMethod method = CrossMethod::DG_AGGRESSIVE;
    registry::Rts rts = registry::Rts::CRS;
    std::vector<double> virtual_inputs;  // 虚拟 DMU 投入向量（VIRTUAL_DMU，长度 m）
    std::vector<double> virtual_outputs; // 虚拟 DMU 产出向量（长度 s）
};

/**
 * @brief 运行交叉效率（全数据集）。
 * @param data 截面数据集
 * @param options 方法与 RTS
 * @param config 元信息（sha1/version）
 * @return 结果：dmu_results[j].score = e_j（收到的平均交叉效率），cross 矩阵齐备
 * @throws registry::EngineException 求解失败（EXIT_SOLVE）
 */
[[nodiscard]] result::ModelResult run_cross_efficiency(const dataset::Dataset &data,
                                                       const CrossRunOptions &options,
                                                       const registry::RunConfig &config);

/** @brief 方法字符串 → 枚举（dg_i|dg_ii|m3_i|m3_ii|virtual）。 */
[[nodiscard]] bool cross_method_from_string(const std::string &text, CrossMethod &out);
} // namespace opensuperdea::cross
