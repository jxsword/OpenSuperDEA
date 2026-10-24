/**
 * @file reference_set_test.cpp
 * @brief 参考集选择器测试（06 册 §2：all/exclude_self 落地 + 未启用规则报错）。
 */
#include "core/registry/engine_error.h"
#include "core/registry/reference_set.h"

#include <gtest/gtest.h>

#include <algorithm>
#include <stdexcept>
#include <vector>

namespace
{
using opensuperdea::registry::ReferenceSetKind;
using opensuperdea::registry::ReferenceSetParams;
using opensuperdea::registry::select_reference_set;
} // namespace

TEST(ReferenceSet, FR_RAD_001_AllReturnsAscendingFullRange)
{
    const ReferenceSetParams params;
    const auto indices = select_reference_set(ReferenceSetKind::ALL, params, 4, 0);
    EXPECT_EQ(indices, (std::vector<std::size_t>{0, 1, 2, 3}));
}

TEST(ReferenceSet, FR_RAD_005_ExcludeSelfRemovesEvaluatedDmu)
{
    const ReferenceSetParams params;
    const auto indices = select_reference_set(ReferenceSetKind::EXCLUDE_SELF, params, 4, 2);
    EXPECT_EQ(indices, (std::vector<std::size_t>{0, 1, 3}));
}

TEST(ReferenceSet, FR_RAD_005_ExcludeSelfOnSingleDmuYieldsEmpty)
{
    const ReferenceSetParams params;
    const auto indices = select_reference_set(ReferenceSetKind::EXCLUDE_SELF, params, 1, 0);
    EXPECT_TRUE(indices.empty());
}

TEST(ReferenceSet, FR_RAD_001_PanelKindsRejectedUntilBranch3)
{
    const ReferenceSetParams params;
    EXPECT_THROW(static_cast<void>(select_reference_set(ReferenceSetKind::WINDOW, params, 4, 0)),
                 opensuperdea::registry::EngineException);
}
