/**
 * @file validator_m2_test.cpp
 * @brief M2 校验规则测试（V-006 扩展列数 / V-008 负值 / V-010 价格与权重前置）。
 */
#include "core/dataset/dataset_builder.h"
#include "core/dataset/validator.h"

#include <gtest/gtest.h>

#include <string>
#include <vector>

namespace
{
using opensuperdea::dataset::ColumnRoles;
using opensuperdea::dataset::CsvTable;
using opensuperdea::dataset::validate_table;

/** @brief 构造带扩展列的最小表。 */
CsvTable make_table(const std::vector<std::string> &headers,
                    const std::vector<std::vector<std::string>> &rows)
{
    CsvTable table;
    table.headers = headers;
    table.rows = rows;
    return table;
}

/** @brief 基础角色（in_×2, out_×1）。 */
ColumnRoles base_roles()
{
    ColumnRoles roles;
    roles.dmu_column = {"dmu"};
    roles.input_columns = {"in_x", "in_y"};
    roles.output_columns = {"out_z"};
    return roles;
}

/** @brief 表头中找指定前缀列数量。 */
std::vector<std::string> prefix_columns(const std::vector<std::string> &headers,
                                        std::string_view prefix)
{
    std::vector<std::string> matched;
    for (const auto &header : headers)
    {
        if (header.starts_with(prefix))
        {
            matched.push_back(header);
        }
    }
    return matched;
}
} // namespace

TEST(ValidatorM2, FR_IO_005_V006_ExtensionColumnCountMismatch)
{
    const auto table =
        make_table({"dmu", "in_x", "in_y", "out_z", "pi_x"}, {{"A", "1", "1", "1", "1"}});
    auto roles = base_roles();
    roles.price_in_columns = prefix_columns(table.headers, "pi_"); // 1 列 ≠ m=2
    const auto errors = validate_table(table, roles, "t.csv");
    bool has_v006 = false;
    for (const auto &error : errors)
    {
        has_v006 = has_v006 || error.rule_id == "V-006";
    }
    EXPECT_TRUE(has_v006);
}

TEST(ValidatorM2, FR_IO_005_V008_NegativeValueInInputsRejected)
{
    const auto table = make_table({"dmu", "in_x", "in_y", "out_z"}, {{"A", "1", "-0.5", "1"}});
    const auto errors = validate_table(table, base_roles(), "t.csv");
    ASSERT_FALSE(errors.empty());
    EXPECT_EQ(errors[0].rule_id, "V-008");
    EXPECT_NE(errors[0].message.find("投入"), std::string::npos);
}

TEST(ValidatorM2, FR_IO_005_V010_NonpositivePriceRejected)
{
    const auto table = make_table({"dmu", "in_x", "in_y", "out_z", "pi_x", "pi_y"},
                                  {{"A", "1", "1", "1", "0", "2"}});
    auto roles = base_roles();
    roles.price_in_columns = {"pi_x", "pi_y"};
    const auto errors = validate_table(table, roles, "t.csv");
    ASSERT_FALSE(errors.empty());
    EXPECT_EQ(errors[0].rule_id, "V-010");
    EXPECT_NE(errors[0].message.find("价格须为正"), std::string::npos);
}

TEST(ValidatorM2, FR_IO_005_V010_WeightSumNotOneRejected)
{
    const auto table = make_table({"dmu", "in_x", "in_y", "out_z", "w_in_x", "w_in_y"},
                                  {{"A", "1", "1", "1", "0.5", "0.4"}});
    auto roles = base_roles();
    roles.weight_in_columns = {"w_in_x", "w_in_y"};
    const auto errors = validate_table(table, roles, "t.csv");
    ASSERT_FALSE(errors.empty());
    EXPECT_EQ(errors[0].rule_id, "V-010");
    EXPECT_NE(errors[0].message.find("权重和"), std::string::npos);
}

TEST(ValidatorM2, FR_IO_005_V010_WeightSumOneAccepted)
{
    const auto table = make_table({"dmu", "in_x", "in_y", "out_z", "w_in_x", "w_in_y"},
                                  {{"A", "1", "1", "1", "0.7", "0.3"}});
    auto roles = base_roles();
    roles.weight_in_columns = {"w_in_x", "w_in_y"};
    const auto errors = validate_table(table, roles, "t.csv");
    EXPECT_TRUE(errors.empty());
}
