/**
 * @file dataset_builder_test.cpp
 * @brief 角色识别与数据集/面板构造测试（FR-IO-001/002/004 + 黄金算例读入）。
 */
#include "core/dataset/dataset_builder.h"

#include <gtest/gtest.h>

#include <string>

namespace
{
using opensuperdea::dataset::BuildOptions;
using opensuperdea::dataset::CsvTable;
using opensuperdea::dataset::identify_roles;

/** @brief 黄金算例目录（CMake 注入仓库根路径）。 */
std::string golden_path(const std::string &relative)
{
    return std::string(OPENSUPERDEA_SOURCE_DIR) + "/tests/golden/" + relative;
}
} // namespace

TEST(DatasetBuilder, FR_IO_001_PrefixRoleDetection)
{
    CsvTable table;
    table.headers = {"dmu", "in_x", "in_y", "out_z"};
    table.rows = {{"A", "1", "2", "3"}};
    std::vector<opensuperdea::dataset::ValidationError> errors;
    const auto roles = identify_roles(table, BuildOptions{}, "<memory>", errors);
    EXPECT_TRUE(errors.empty());
    ASSERT_EQ(roles.input_columns.size(), 2U);
    EXPECT_EQ(roles.input_columns[0], "in_x");
    ASSERT_EQ(roles.output_columns.size(), 1U);
    EXPECT_EQ(roles.output_columns[0], "out_z");
}

TEST(DatasetBuilder, FR_IO_001_ExplicitColumnsOverridePrefix)
{
    CsvTable table;
    table.headers = {"dmu", "in_x", "in_y", "out_z"};
    table.rows = {{"A", "1", "2", "3"}};
    BuildOptions options;
    options.explicit_inputs = {"in_y"};
    options.explicit_outputs = {"out_z"};
    std::vector<opensuperdea::dataset::ValidationError> errors;
    const auto roles = identify_roles(table, options, "<memory>", errors);
    EXPECT_TRUE(errors.empty());
    ASSERT_EQ(roles.input_columns.size(), 1U);
    EXPECT_EQ(roles.input_columns[0], "in_y");
}

TEST(DatasetBuilder, FR_IO_001_UnknownExplicitColumnReported)
{
    CsvTable table;
    table.headers = {"dmu", "in_x"};
    table.rows = {{"A", "1"}};
    BuildOptions options;
    options.explicit_inputs = {"missing"};
    std::vector<opensuperdea::dataset::ValidationError> errors;
    static_cast<void>(identify_roles(table, options, "<memory>", errors));
    ASSERT_EQ(errors.size(), 1U);
    EXPECT_EQ(errors[0].rule_id, "V-003");
}

TEST(DatasetBuilder, FR_IO_001_GoldenCrossCaseLoaded)
{
    opensuperdea::dataset::CsvParseError parse_error{};
    const auto table =
        opensuperdea::dataset::read_csv_file(golden_path("toy_cross/data.csv"), &parse_error);
    ASSERT_TRUE(parse_error.message.empty()) << parse_error.message;
    std::vector<opensuperdea::dataset::ValidationError> errors;
    const auto roles =
        identify_roles(table, BuildOptions{}, golden_path("toy_cross/data.csv"), errors);
    auto data = opensuperdea::dataset::build_dataset(table, roles, "toy_cross", errors);
    EXPECT_TRUE(errors.empty());
    EXPECT_EQ(data.dmu_count(), 4U);
    EXPECT_EQ(data.input_count(), 2U);
    EXPECT_EQ(data.output_count(), 1U);
    EXPECT_DOUBLE_EQ(data.inputs(0, 0), 2.0);
    EXPECT_DOUBLE_EQ(data.outputs(3, 0), 7.0);
}

TEST(DatasetBuilder, FR_IO_002_DirectionColumnsAttached)
{
    opensuperdea::dataset::CsvParseError parse_error{};
    const auto table =
        opensuperdea::dataset::read_csv_file(golden_path("toy_cross/data.csv"), &parse_error);
    ASSERT_TRUE(parse_error.message.empty());
    // 无方向列的算例：directions 为空
    std::vector<opensuperdea::dataset::ValidationError> errors;
    const auto roles = identify_roles(table, BuildOptions{}, "<memory>", errors);
    auto data = opensuperdea::dataset::build_dataset(table, roles, "toy_cross", errors);
    EXPECT_FALSE(data.directions.has_value());
}

TEST(DatasetBuilder, FR_IO_004_PanelLongFormatGroupedByPeriod)
{
    opensuperdea::dataset::CsvParseError parse_error{};
    const auto table =
        opensuperdea::dataset::read_csv_file(golden_path("toy_panel/data.csv"), &parse_error);
    ASSERT_TRUE(parse_error.message.empty()) << parse_error.message;
    std::vector<opensuperdea::dataset::ValidationError> errors;
    const auto roles = identify_roles(table, BuildOptions{}, "<memory>", errors);
    auto panel = opensuperdea::dataset::build_panel(table, roles, "toy_panel", errors);
    EXPECT_TRUE(errors.empty());
    ASSERT_EQ(panel.periods.size(), 2U);
    EXPECT_EQ(panel.periods[0], "2019"); // 按首现顺序，非字典序
    EXPECT_EQ(panel.periods[1], "2020");
    EXPECT_TRUE(panel.is_balanced());
    ASSERT_EQ(panel.sections.at("2019").dmu_count(), 3U);
    EXPECT_DOUBLE_EQ(panel.sections.at("2020").inputs(0, 0), 1.5);
}
