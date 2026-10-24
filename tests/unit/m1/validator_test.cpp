/**
 * @file validator_test.cpp
 * @brief 校验规则逐条测试（FR-IO-005；V-002/003/004/005/009 各一用例 + 聚合性）。
 */
#include "core/dataset/validator.h"

#include <gtest/gtest.h>

namespace
{
using opensuperdea::dataset::CsvTable;
using opensuperdea::dataset::validate_table;
using opensuperdea::dataset::ValidationError;

/** @brief 构造最小合法表（2 投入 1 产出）。 */
CsvTable make_valid_table()
{
    CsvTable table;
    table.headers = {"dmu", "in_x", "in_y", "out_z"};
    table.rows = {{"A", "1", "2", "3"}, {"B", "2.5", "1e-1", "4"}};
    return table;
}

/** @brief 按规则编号过滤错误。 */
std::vector<ValidationError> errors_of(const std::vector<ValidationError> &errors,
                                       const std::string &rule_id)
{
    std::vector<ValidationError> matched;
    for (const auto &error : errors)
    {
        if (error.rule_id == rule_id)
        {
            matched.push_back(error);
        }
    }
    return matched;
}
} // namespace

TEST(Validator, FR_IO_005_ValidTablePasses)
{
    const auto errors = validate_table(make_valid_table(), {"in_x", "in_y"}, {"out_z"}, "t.csv");
    EXPECT_TRUE(errors.empty()) << (errors.empty() ? "" : errors[0].message);
}

TEST(Validator, FR_IO_005_V002_EmptyDataReported)
{
    CsvTable empty_table;
    const auto errors = validate_table(empty_table, {}, {}, "t.csv");
    EXPECT_FALSE(errors_of(errors, "V-002").empty());
}

TEST(Validator, FR_IO_005_V003_MissingRoleColumnsReported)
{
    CsvTable table;
    table.headers = {"name", "x"};
    table.rows = {{"A", "1"}};
    const auto errors = validate_table(table, {}, {}, "t.csv");
    const auto v003 = errors_of(errors, "V-003");
    ASSERT_GE(v003.size(), 1U); // 缺 dmu 列 + 无角色列，两条聚合
}

TEST(Validator, FR_IO_005_V004_NonNumericAggregatedWithLocation)
{
    auto table = make_valid_table();
    table.rows[0][3] = "12x"; // A 行 out_z 非数值（尾缀）
    table.rows[1][2] = "abc"; // B 行 in_y 非数值
    const auto errors = validate_table(table, {"in_x", "in_y"}, {"out_z"}, "t.csv");
    const auto v004 = errors_of(errors, "V-004");
    ASSERT_EQ(v004.size(), 2U);  // 错误聚合：一次报全部
    EXPECT_EQ(v004[0].line, 2U); // 表头 +1、行 1 基 → 第 2 物理行
    EXPECT_EQ(v004[0].column, "out_z");
    EXPECT_EQ(v004[1].column, "in_y");
    EXPECT_NE(v004[1].message.find("abc"), std::string::npos);
}

TEST(Validator, FR_IO_005_V005_PanelSkipsCrossPeriodDuplicates)
{
    // 面板长表：同一 DMU 跨期多行合法（V-009 管组合唯一），仅空名报错
    CsvTable table;
    table.headers = {"dmu", "period", "in_x", "out_y"};
    table.rows = {{"A", "2019", "1", "1"}, {"A", "2020", "2", "2"}};
    const auto panel_errors =
        validate_table(table, {"in_x"}, {"out_y"}, "t.csv", /*is_panel=*/true);
    EXPECT_TRUE(errors_of(panel_errors, "V-005").empty());
    const auto cross_errors =
        validate_table(table, {"in_x"}, {"out_y"}, "t.csv", /*is_panel=*/false);
    EXPECT_FALSE(errors_of(cross_errors, "V-005").empty()); // 截面语义下仍报重复
}

TEST(Validator, FR_IO_005_V005_DuplicateAndEmptyDmuNames)
{
    auto table = make_valid_table();
    table.rows.push_back({"A", "1", "1", "1"}); // 与 A 重复
    table.rows.push_back({"", "1", "1", "1"});  // 空名
    const auto errors = validate_table(table, {"in_x", "in_y"}, {"out_z"}, "t.csv");
    const auto v005 = errors_of(errors, "V-005");
    ASSERT_GE(v005.size(), 2U);
    EXPECT_NE(v005[1].message.find("重复"), std::string::npos);
}

TEST(Validator, FR_IO_005_V009_PanelDuplicateKeyReported)
{
    CsvTable table;
    table.headers = {"dmu", "period", "in_x", "out_y"};
    table.rows = {{"A", "2019", "1", "1"}, {"A", "2019", "2", "2"}};
    const auto errors = validate_table(table, {"in_x"}, {"out_y"}, "t.csv");
    const auto v009 = errors_of(errors, "V-009");
    ASSERT_EQ(v009.size(), 1U);
    EXPECT_EQ(v009[0].line, 3U);
}
