/**
 * @file validator.cpp
 * @brief 校验规则实现（V-001 文件级在 builder 入口报，此处覆盖表级 V-002~V-005、V-009）。
 * @details 规则与提示文案以 design_docs/02_data_design.md §3 规则表为准；
 *          错误聚合（单次全量报告），顺序按规则编号保证输出稳定。
 */
#include "validator.h"

#include <algorithm>
#include <ranges>
#include <set>
#include <unordered_set>

namespace opensuperdea::dataset
{
namespace
{
/** @brief 数值解析：接受整数/小数/科学计数；拒绝空串与千分位。 */
bool parse_number(const std::string &text, double &value)
{
    if (text.empty())
    {
        return false;
    }
    std::size_t consumed = 0;
    try
    {
        value = std::stod(text, &consumed);
    }
    catch (const std::exception &)
    {
        return false;
    }
    // 全串消费（拒绝 "12abc" 类尾缀）
    return consumed == text.size();
}

/** @brief V-002：空文件/无表头/无数据行。 */
void check_v002(const CsvTable &table, const std::vector<std::string> &input_columns,
                const std::vector<std::string> &output_columns,
                std::vector<ValidationError> &errors, const std::string &file_path)
{
    static_cast<void>(input_columns);
    static_cast<void>(output_columns);
    if (table.headers.empty())
    {
        errors.push_back({.rule_id = "V-002",
                          .file = file_path,
                          .line = 0,
                          .column = "",
                          .message = "空文件或无表头",
                          .hint = "提供含表头的 CSV"});
        return;
    }
    if (table.rows.empty())
    {
        errors.push_back({.rule_id = "V-002",
                          .file = file_path,
                          .line = 0,
                          .column = "",
                          .message = "无数据行",
                          .hint = "表头下至少需要一行数据"});
    }
}

/** @brief V-003：缺少 dmu 列或无任何投入/产出列。 */
void check_v003(const CsvTable &table, const std::vector<std::string> &input_columns,
                const std::vector<std::string> &output_columns,
                std::vector<ValidationError> &errors, const std::string &file_path)
{
    if (table.headers.empty())
    {
        return; // V-002 已报
    }
    const bool has_dmu = std::ranges::find(table.headers, "dmu") != table.headers.end();
    if (!has_dmu)
    {
        errors.push_back({.rule_id = "V-003",
                          .file = file_path,
                          .line = 0,
                          .column = "dmu",
                          .message = "缺少 dmu 名称列",
                          .hint = "首列命名为 dmu"});
    }
    if (input_columns.empty() && output_columns.empty())
    {
        errors.push_back(
            {.rule_id = "V-003",
             .file = file_path,
             .line = 0,
             .column = "",
             .message = "无任何 in_/out_ 角色列",
             .hint = "投入列以 in_ 前缀、产出列以 out_ 前缀命名，或用 --inputs/--outputs 指定"});
    }
}

/** @brief V-004：非数值单元格（聚合全部行报告，前 10 条后截断）。 */
void check_v004(const CsvTable &table, const std::vector<std::string> &input_columns,
                const std::vector<std::string> &output_columns,
                std::vector<ValidationError> &errors, const std::string &file_path)
{
    std::set<std::size_t> numeric_columns; // 表头索引集合
    for (const auto &column_name : input_columns)
    {
        auto it = std::ranges::find(table.headers, column_name);
        if (it != table.headers.end())
        {
            numeric_columns.insert(static_cast<std::size_t>(it - table.headers.begin()));
        }
    }
    for (const auto &column_name : output_columns)
    {
        auto it = std::ranges::find(table.headers, column_name);
        if (it != table.headers.end())
        {
            numeric_columns.insert(static_cast<std::size_t>(it - table.headers.begin()));
        }
    }
    constexpr std::size_t MAX_REPORTED = 10;
    std::size_t reported = 0;
    for (std::size_t row_index = 0; row_index < table.rows.size(); ++row_index)
    {
        for (const std::size_t column_index : numeric_columns)
        {
            double unused_value = 0.0;
            if (!parse_number(table.rows[row_index][column_index], unused_value))
            {
                if (reported >= MAX_REPORTED)
                {
                    errors.push_back({.rule_id = "V-004",
                                      .file = file_path,
                                      .line = 0,
                                      .column = "",
                                      .message = "非数值单元格过多（其余略）",
                                      .hint = "修正后重新校验"});
                    return;
                }
                ++reported;
                errors.push_back(
                    {.rule_id = "V-004",
                     .file = file_path,
                     .line = row_index + 2, // +2：表头 1 行 + 1 基行号
                     .column = table.headers[column_index],
                     .message = "非数值: \"" + table.rows[row_index][column_index] + "\"",
                     .hint = "填写数值（支持整数/小数/科学计数）"});
            }
        }
    }
}

/** @brief V-005：DMU 名重复或为空（面板模式跳过重复检查，见 validate_table 契约）。 */
void check_v005(const CsvTable &table, const std::vector<std::string> &input_columns,
                const std::vector<std::string> &output_columns,
                std::vector<ValidationError> &errors, const std::string &file_path, bool is_panel)
{
    static_cast<void>(input_columns);
    static_cast<void>(output_columns);
    auto dmu_it = std::ranges::find(table.headers, "dmu");
    if (dmu_it == table.headers.end())
    {
        return; // V-003 已报
    }
    const std::size_t dmu_index = static_cast<std::size_t>(dmu_it - table.headers.begin());
    std::unordered_set<std::string> seen;
    std::vector<std::string> duplicated;
    for (std::size_t row_index = 0; row_index < table.rows.size(); ++row_index)
    {
        const std::string &name = table.rows[row_index][dmu_index];
        if (name.empty())
        {
            errors.push_back({.rule_id = "V-005",
                              .file = file_path,
                              .line = row_index + 2,
                              .column = "dmu",
                              .message = "DMU 名为空",
                              .hint = "每行需唯一 DMU 名称"});
            continue;
        }
        if (is_panel)
        {
            continue; // 面板长表跨期重复合法（V-009 管 (dmu, period) 唯一）
        }
        // 重复名聚合（上限 10 条防刷屏）
        if (!seen.insert(name).second && duplicated.size() < 10)
        {
            duplicated.push_back(name);
        }
    }
    if (!duplicated.empty())
    {
        std::string joined;
        for (const auto &name : duplicated)
        {
            joined += (joined.empty() ? "" : ", ") + name;
        }
        errors.push_back({.rule_id = "V-005",
                          .file = file_path,
                          .line = 0,
                          .column = "dmu",
                          .message = "重复 DMU 名: " + joined,
                          .hint = "DMU 名称必须唯一"});
    }
}

/** @brief V-009：面板 (dmu, period) 重复。 */
void check_v009(const CsvTable &table, const std::vector<std::string> &input_columns,
                const std::vector<std::string> &output_columns,
                std::vector<ValidationError> &errors, const std::string &file_path)
{
    static_cast<void>(input_columns);
    static_cast<void>(output_columns);
    auto period_it = std::ranges::find(table.headers, "period");
    auto dmu_it = std::ranges::find(table.headers, "dmu");
    if (period_it == table.headers.end() || dmu_it == table.headers.end())
    {
        return; // 非面板数据或 V-003 已报
    }
    const std::size_t period_index = static_cast<std::size_t>(period_it - table.headers.begin());
    const std::size_t dmu_index = static_cast<std::size_t>(dmu_it - table.headers.begin());
    std::set<std::pair<std::string, std::string>> seen;
    for (std::size_t row_index = 0; row_index < table.rows.size(); ++row_index)
    {
        const auto key =
            std::make_pair(table.rows[row_index][dmu_index], table.rows[row_index][period_index]);
        if (!seen.insert(key).second)
        {
            errors.push_back({.rule_id = "V-009",
                              .file = file_path,
                              .line = row_index + 2,
                              .column = "dmu,period",
                              .message = "组合重复: (" + key.first + "," + key.second + ")",
                              .hint = "每 (dmu, period) 组合仅允许一行"});
        }
    }
}
} // namespace

std::vector<ValidationError> validate_table(const CsvTable &table,
                                            const std::vector<std::string> &input_columns,
                                            const std::vector<std::string> &output_columns,
                                            const std::string &file_path, bool is_panel)
{
    std::vector<ValidationError> errors;
    // 规则顺序即输出顺序（02 册规则表顺序）
    check_v002(table, input_columns, output_columns, errors, file_path);
    check_v003(table, input_columns, output_columns, errors, file_path);
    check_v004(table, input_columns, output_columns, errors, file_path);
    check_v005(table, input_columns, output_columns, errors, file_path, is_panel);
    check_v009(table, input_columns, output_columns, errors, file_path);
    return errors;
}
} // namespace opensuperdea::dataset
