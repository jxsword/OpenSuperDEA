/**
 * @file csv_parser_test.cpp
 * @brief CSV 词法解析测试（FR-IO-001；BOM/CRLF/注释/空行/科学计数/列数不一致）。
 */
#include "core/dataset/csv_parser.h"

#include <gtest/gtest.h>

namespace
{
using opensuperdea::dataset::parse_csv_text;
} // namespace

TEST(CsvParser, FR_IO_001_BasicParse)
{
    const auto table = parse_csv_text("a,b\n1,2\n3,4\n", nullptr);
    ASSERT_EQ(table.headers.size(), 2U);
    ASSERT_EQ(table.rows.size(), 2U);
    EXPECT_EQ(table.rows[0][0], "1");
    EXPECT_EQ(table.rows[1][1], "4");
}

TEST(CsvParser, FR_IO_001_BomAndCrlfTolerated)
{
    const auto table = parse_csv_text("\xEF\xBB\xBFh1,h2\r\n1,2\r\n", nullptr);
    ASSERT_EQ(table.headers.size(), 2U);
    EXPECT_EQ(table.headers[0], "h1");
    ASSERT_EQ(table.rows.size(), 1U);
}

TEST(CsvParser, FR_IO_001_CommentAndEmptyLinesSkipped)
{
    const auto table = parse_csv_text("# 注释\n\nh1,h2\n\n# 说明\n1,2\n", nullptr);
    ASSERT_EQ(table.headers.size(), 2U);
    ASSERT_EQ(table.rows.size(), 1U);
}

TEST(CsvParser, FR_IO_001_ScientificNotationKeptAsText)
{
    // 词法层不解释数值，科学计数原样保留（数值语义在 builder/std::stod）
    const auto table = parse_csv_text("v\n1.5e-3\n", nullptr);
    ASSERT_EQ(table.rows.size(), 1U);
    EXPECT_EQ(table.rows[0][0], "1.5e-3");
}

TEST(CsvParser, FR_IO_001_ColumnCountMismatchReported)
{
    opensuperdea::dataset::CsvParseError error{};
    const auto table = parse_csv_text("a,b\n1,2\n3\n", &error);
    EXPECT_TRUE(table.rows.empty());
    EXPECT_EQ(error.line_number, 3U);
    EXPECT_FALSE(error.message.empty());
}
