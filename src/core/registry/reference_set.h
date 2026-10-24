/**
 * @file reference_set.h
 * @brief 参考集选择器（design_docs/01 册 §4.2 + 06 册 §2 落地）。
 * @details 「哪些 DMU 进入约束」的统一抽象：Cluster 8 基准（M5）与 Malmquist
 *          7 参比本质都是参考集组合规则参数化。M2 分支 1 落地 all/exclude_self，
 *          window/layer/group/fixed/period_pair 留接口（分支 3 与 M5 填充）。
 */
#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

namespace opensuperdea::registry
{
/** @brief 进入约束的 DMU 行索引列表（数据集内 0 基）。 */
using IndexList = std::vector<std::size_t>;

/** @brief 参考集组合规则（06 册 §2 全集；M2 渐次启用）。 */
enum class ReferenceSetKind : std::uint8_t
{
    ALL,          /**< 全体 DMU（常规模型默认） */
    EXCLUDE_SELF, /**< 全体 − 被评 DMU（超效率族） */
    WINDOW,       /**< 期次 [t-w+1, t] 合并截面（窗口分析/Window Malmquist） */
    LAYER,        /**< 分层参考集（Cluster，M5） */
    GROUP,        /**< 群组参考集（Cluster self/cross，M5） */
    FIXED,        /**< 固定基准 T*（Cluster fixed，M5） */
    PERIOD_PAIR   /**< 跨期参考集（Malmquist 参比，M2 分支 3 / M5） */
};

/** @brief 选择器参数（按 kind 取用；未用字段忽略）。 */
struct ReferenceSetParams
{
    std::size_t window_width = 1; // WINDOW：窗口宽度 w
    std::size_t layer = 0;        // LAYER：层号
    std::size_t group = 0;        // GROUP：群组号
    IndexList fixed_dmus;         // FIXED：固定基准索引集
    std::size_t period_t = 0;     // PERIOD_PAIR：基期
    std::size_t period_s = 0;     // PERIOD_PAIR：参比期
};

/**
 * @brief 按组合规则选择参考集。
 * @param kind 组合规则
 * @param params 规则参数
 * @param dmu_count 截面 DMU 总数
 * @param excluded_index 需剔除的被评 DMU 索引（仅 EXCLUDE_SELF 消费；其余传任意值）
 * @return 参考集索引（升序）
 * @note WINDOW/LAYER/GROUP/FIXED/PERIOD_PAIR 在 M2 分支 1 尚不可用，抛 EngineException。
 */
[[nodiscard]] IndexList select_reference_set(ReferenceSetKind kind,
                                             const ReferenceSetParams &params,
                                             std::size_t dmu_count, std::size_t excluded_index);
} // namespace opensuperdea::registry
