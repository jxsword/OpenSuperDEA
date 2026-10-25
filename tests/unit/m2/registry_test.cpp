/**
 * @file registry_test.cpp
 * @brief 模型注册表测试（FR-CLI-008 能力矩阵 + 06 册 §1 注册/查找契约）。
 */
#include "core/registry/model_registry.h"

#include <gtest/gtest.h>

#include <string>

namespace
{
using opensuperdea::registry::ModelRegistry;
using opensuperdea::registry::Orientation;
using opensuperdea::registry::Rts;
} // namespace

TEST(ModelRegistry, FR_CLI_008_BuiltinModelsRegistered)
{
    const ModelRegistry &registry = ModelRegistry::instance();
    EXPECT_NE(registry.find("ccr"), nullptr);
    EXPECT_NE(registry.find("bcc"), nullptr);
    EXPECT_NE(registry.find("multiplier"), nullptr);
    EXPECT_NE(registry.find("scale_efficiency"), nullptr);
    EXPECT_EQ(registry.find("bogus"), nullptr);
}

TEST(ModelRegistry, FR_CLI_008_CcrSupportsAllRtsAndOrientations)
{
    const ModelRegistry &registry = ModelRegistry::instance();
    std::string reason;
    for (const Orientation orientation : {Orientation::INPUT, Orientation::OUTPUT})
    {
        for (const Rts rts : {Rts::CRS, Rts::VRS, Rts::NIRS, Rts::NDRS})
        {
            EXPECT_TRUE(registry.validate_combo("ccr", orientation, rts, false, &reason)) << reason;
        }
    }
    // 非导向径向包络不在分支 1 能力矩阵内
    EXPECT_FALSE(
        registry.validate_combo("ccr", Orientation::NON_DIRECTIONAL, Rts::CRS, false, &reason));
}

TEST(ModelRegistry, FR_CLI_008_BccStrictVrsAlias)
{
    const ModelRegistry &registry = ModelRegistry::instance();
    std::string reason;
    EXPECT_TRUE(registry.validate_combo("bcc", Orientation::INPUT, Rts::VRS, false, &reason));
    EXPECT_FALSE(registry.validate_combo("bcc", Orientation::INPUT, Rts::CRS, false, &reason));
    EXPECT_FALSE(reason.empty()); // 非法组合必须给出原因（FR-CLI-008）
}

TEST(ModelRegistry, FR_CLI_008_SuperSupportOnlyOnCapableModels)
{
    const ModelRegistry &registry = ModelRegistry::instance();
    std::string reason;
    EXPECT_TRUE(registry.validate_combo("ccr", Orientation::INPUT, Rts::VRS, true, &reason));
    EXPECT_FALSE(
        registry.validate_combo("multiplier", Orientation::INPUT, Rts::CRS, true, &reason));
    EXPECT_FALSE(reason.empty());
}

TEST(ModelRegistry, FR_CLI_008_UnknownModelReasonMentionsId)
{
    const ModelRegistry &registry = ModelRegistry::instance();
    std::string reason;
    EXPECT_FALSE(registry.validate_combo("nope", Orientation::INPUT, Rts::CRS, false, &reason));
    EXPECT_NE(reason.find("nope"), std::string::npos);
}
