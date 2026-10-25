/**
 * @file dmu_result.h
 * @brief 单个 DMU 的求解结果（design_docs/02 册 §4 契约 + 06 册 §1 DmuResult）。
 * @details 状态枚举跨模型统一（02 册 §4.3）；score 为无量纲效率测度
 *          （径向 θ 或 1/φ、SBM ρ 等），不可行时不写入数值（NaN 哨兵）。
 */
#pragma once

#include <cstddef>
#include <cstdint>
#include <map>
#include <string>
#include <utility>
#include <vector>

namespace opensuperdea::result
{
/** @brief DMU 求解状态（02 册 §4.3，跨模型统一）。 */
enum class DmuStatus : std::uint8_t
{
    EFFICIENT,       /**< 强有效（前沿上且无松弛） */
    INEFFICIENT,     /**< 无效 */
    SUPER_EFFICIENT, /**< 超有效（超效率模型下 score > 1） */
    INFEASIBLE,      /**< 不可行（超效率剔除后无可行解；不输出数值） */
    NON_UNIQUE_NOTE  /**< 解非唯一标注（WeakEF 等，A-001；M2 暂不产生） */
};

/** @brief 状态枚举 → 导出字符串（"efficient"/"inefficient"/…，02 册 §4.3）。 */
[[nodiscard]] std::string to_string(DmuStatus status);

/** @brief 单 DMU 求解结果。 */
struct DmuResult
{
    std::size_t dmu_index = 0; // 数据集内被评 DMU 行索引
    double score = 0.0;        // 效率测度；不可行时为 quiet NaN
    DmuStatus status = DmuStatus::INEFFICIENT;

    // 投影目标值（FR-OUT-002）：长度 m / s；不可行或未产出时为空
    std::vector<double> input_targets;
    std::vector<double> output_targets;

    // 松弛量（FR-OUT-004）：长度 m / s；空 = 模型不产出
    std::vector<double> input_slacks;
    std::vector<double> output_slacks;

    // 强度权重 λ（FR-OUT-003）：(数据集内参考 DMU 索引, 权重) 非零项；空 = 无
    std::vector<std::pair<std::size_t, double>> lambdas;

    // 最优乘子（FR-OUT-005）：投入乘子 v（m）/ 产出乘子 u（s）；空 = 模型不产出
    std::vector<double> input_multipliers;
    std::vector<double> output_multipliers;

    // 模型特有标注列（如 scale_efficiency 的 te/pte/se/rts_region）
    std::map<std::string, std::string> annotations;

    /** @brief score 是否为可导出的有限数值。 */
    [[nodiscard]] bool has_score() const;
};
} // namespace opensuperdea::result
