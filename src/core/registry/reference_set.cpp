/**
 * @file reference_set.cpp
 * @brief 参考集选择器实现（all / exclude_self 先行，其余规则随窗口与 M5 启用）。
 */
#include "reference_set.h"

#include "engine_error.h"

namespace opensuperdea::registry
{
IndexList select_reference_set(ReferenceSetKind kind, const ReferenceSetParams &params,
                               std::size_t dmu_count, std::size_t excluded_index)
{
    IndexList indices;
    switch (kind)
    {
    case ReferenceSetKind::ALL:
        indices.reserve(dmu_count);
        for (std::size_t i = 0; i < dmu_count; ++i)
        {
            indices.push_back(i);
        }
        return indices;
    case ReferenceSetKind::EXCLUDE_SELF:
        indices.reserve(dmu_count > 0 ? dmu_count - 1 : 0);
        for (std::size_t i = 0; i < dmu_count; ++i)
        {
            if (i != excluded_index)
            {
                indices.push_back(i);
            }
        }
        return indices;
    case ReferenceSetKind::WINDOW:
    case ReferenceSetKind::LAYER:
    case ReferenceSetKind::GROUP:
    case ReferenceSetKind::FIXED:
    case ReferenceSetKind::PERIOD_PAIR:
        // 面板类参考集依赖 PanelData 上下文，分支 3（窗口）与 M5（Cluster/TFP）启用
        static_cast<void>(params);
        throw EngineException(1, "args",
                              "参考集规则当前不可用（窗口属 M2 分支 3，"
                              "分层/群组/固定/跨期属 M5）",
                              "使用 all 或 exclude_self");
    }
    throw EngineException(4, "internal", "未知参考集规则");
}
} // namespace opensuperdea::registry
