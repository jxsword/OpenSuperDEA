/**
 * @file dataset_builder.cpp
 * @brief 角色识别与数据集构造实现。
 * @details 数值经 parse 统一走校验器已通过的路径；这里只做矩阵装配与维度
 *          对齐（防御性：校验遗漏时收集错误而不是抛异常）。
 */
#include "dataset_builder.h"

#include <Eigen/Dense>

#include <algorithm>
#include <map>
#include <ranges>
#include <set>

namespace opensuperdea::dataset
{
namespace
{
/** @brief 表头中查找列名，返回索引；不存在返回 npos。 */
std::optional<std::size_t> find_column(const CsvTable &table, const std::string &name)
{
    const auto found = std::ranges::find(table.headers, name);
    if (found == table.headers.end())
    {
        return std::nullopt;
    }
    return static_cast<std::size_t>(found - table.headers.begin());
}

/** @brief 按前缀收集列名（保持表头顺序）。 */
std::vector<std::string> columns_with_prefix(const CsvTable &table, std::string_view prefix)
{
    std::vector<std::string> matched;
    for (const auto &header : table.headers)
    {
        if (header.starts_with(prefix))
        {
            matched.emplace_back(header);
        }
    }
    return matched;
}

/** @brief CLI 覆写列存在性检查。 */
void check_explicit_columns(const CsvTable &table, const std::vector<std::string> &explicit_columns,
                            const std::string &role_label, std::vector<ValidationError> &errors,
                            const std::string &file_path)
{
    for (const auto &name : explicit_columns)
    {
        if (!find_column(table, name).has_value())
        {
            std::string message;
            message.append(role_label).append("覆写列不存在: ").append(name);
            errors.push_back({.rule_id = "V-003",
                              .file = file_path,
                              .line = 0,
                              .column = name,
                              .message = std::move(message),
                              .hint = "检查 --inputs/--outputs 列名拼写"});
        }
    }
}
} // namespace

ColumnRoles identify_roles(const CsvTable &table, const BuildOptions &options,
                           const std::string &file_path, std::vector<ValidationError> &errors)
{
    ColumnRoles roles;
    // dmu 列与 period 列固定命名（02 册 §2.1/§2.3）
    if (find_column(table, "dmu").has_value())
    {
        roles.dmu_column.emplace_back("dmu");
    }
    if (find_column(table, "period").has_value())
    {
        roles.period_column = "period";
    }
    // 投入/产出：CLI 覆写优先（02 册 §2.4），否则前缀推断
    if (!options.explicit_inputs.empty() || !options.explicit_outputs.empty())
    {
        roles.input_columns = options.explicit_inputs;
        roles.output_columns = options.explicit_outputs;
        check_explicit_columns(table, roles.input_columns, "投入", errors, file_path);
        check_explicit_columns(table, roles.output_columns, "产出", errors, file_path);
    }
    else
    {
        roles.input_columns = columns_with_prefix(table, "in_");
        roles.output_columns = columns_with_prefix(table, "out_");
    }
    roles.direction_in_columns = columns_with_prefix(table, "di_");
    roles.direction_out_columns = columns_with_prefix(table, "do_");
    return roles;
}

Dataset build_dataset(const CsvTable &table, const ColumnRoles &roles, const std::string &file_path,
                      std::vector<ValidationError> &errors)
{
    if (roles.dmu_column.empty() || (roles.input_columns.empty() && roles.output_columns.empty()))
    {
        // V-003 已由 validate_table 报告；防御返回空集
        return Dataset{};
    }
    const auto dmu_index = find_column(table, "dmu");
    if (!dmu_index.has_value())
    {
        errors.push_back({.rule_id = "V-003",
                          .file = file_path,
                          .line = 0,
                          .column = "dmu",
                          .message = "dmu 列缺失于表头",
                          .hint = "首列命名为 dmu"});
        return Dataset{};
    }
    const std::size_t dmu_count = table.rows.size();
    const std::size_t m = roles.input_columns.size();
    const std::size_t s = roles.output_columns.size();

    Dataset dataset;
    dataset.input_names = roles.input_columns;
    dataset.output_names = roles.output_columns;
    dataset.inputs.resize(static_cast<Eigen::Index>(dmu_count), static_cast<Eigen::Index>(m));
    dataset.outputs.resize(static_cast<Eigen::Index>(dmu_count), static_cast<Eigen::Index>(s));

    // 列索引解析（含防御：校验层已保证存在，此处缺失则聚合报错）
    std::vector<std::size_t> input_indices;
    input_indices.reserve(m);
    for (const auto &name : roles.input_columns)
    {
        const auto column_index = find_column(table, name);
        if (!column_index.has_value())
        {
            errors.push_back({.rule_id = "V-003",
                              .file = file_path,
                              .line = 0,
                              .column = name,
                              .message = "投入列缺失于表头",
                              .hint = "检查列名拼写与 --inputs 覆写"});
            return Dataset{};
        }
        input_indices.push_back(*column_index);
    }
    std::vector<std::size_t> output_indices;
    output_indices.reserve(s);
    for (const auto &name : roles.output_columns)
    {
        const auto column_index = find_column(table, name);
        if (!column_index.has_value())
        {
            errors.push_back({.rule_id = "V-003",
                              .file = file_path,
                              .line = 0,
                              .column = name,
                              .message = "产出列缺失于表头",
                              .hint = "检查列名拼写与 --outputs 覆写"});
            return Dataset{};
        }
        output_indices.push_back(*column_index);
    }

    for (std::size_t row = 0; row < dmu_count; ++row)
    {
        dataset.dmu_names.push_back(table.rows[row][*dmu_index]);
        for (std::size_t j = 0; j < m; ++j)
        {
            dataset.inputs(static_cast<Eigen::Index>(row), static_cast<Eigen::Index>(j)) =
                std::stod(table.rows[row][input_indices[j]]);
        }
        for (std::size_t j = 0; j < s; ++j)
        {
            dataset.outputs(static_cast<Eigen::Index>(row), static_cast<Eigen::Index>(j)) =
                std::stod(table.rows[row][output_indices[j]]);
        }
    }

    // 方向列装配（FR-IO-002；维度与 m/s 不匹配已由校验保证，此处防御）
    if (!roles.direction_in_columns.empty() || !roles.direction_out_columns.empty())
    {
        // 方向列索引预解析（同投入/产出：缺失即聚合报错）
        std::vector<std::size_t> direction_in_indices;
        direction_in_indices.reserve(roles.direction_in_columns.size());
        for (const auto &name : roles.direction_in_columns)
        {
            const auto column_index = find_column(table, name);
            if (!column_index.has_value())
            {
                errors.push_back({.rule_id = "V-003",
                                  .file = file_path,
                                  .line = 0,
                                  .column = name,
                                  .message = "方向输入列缺失于表头",
                                  .hint = "检查 di_ 前缀列名"});
                return Dataset{};
            }
            direction_in_indices.push_back(*column_index);
        }
        std::vector<std::size_t> direction_out_indices;
        direction_out_indices.reserve(roles.direction_out_columns.size());
        for (const auto &name : roles.direction_out_columns)
        {
            const auto column_index = find_column(table, name);
            if (!column_index.has_value())
            {
                errors.push_back({.rule_id = "V-003",
                                  .file = file_path,
                                  .line = 0,
                                  .column = name,
                                  .message = "方向产出列缺失于表头",
                                  .hint = "检查 do_ 前缀列名"});
                return Dataset{};
            }
            direction_out_indices.push_back(*column_index);
        }

        DirectionData directions;
        directions.input_directions.resize(
            static_cast<Eigen::Index>(dmu_count),
            static_cast<Eigen::Index>(roles.direction_in_columns.size()));
        directions.output_directions.resize(
            static_cast<Eigen::Index>(dmu_count),
            static_cast<Eigen::Index>(roles.direction_out_columns.size()));
        for (std::size_t row = 0; row < dmu_count; ++row)
        {
            for (std::size_t j = 0; j < roles.direction_in_columns.size(); ++j)
            {
                directions.input_directions(static_cast<Eigen::Index>(row),
                                            static_cast<Eigen::Index>(j)) =
                    std::stod(table.rows[row][direction_in_indices[j]]);
            }
            for (std::size_t j = 0; j < roles.direction_out_columns.size(); ++j)
            {
                directions.output_directions(static_cast<Eigen::Index>(row),
                                             static_cast<Eigen::Index>(j)) =
                    std::stod(table.rows[row][direction_out_indices[j]]);
            }
        }
        dataset.directions = std::move(directions);
    }
    static_cast<void>(errors);
    return dataset;
}

PanelData build_panel(const CsvTable &table, const ColumnRoles &roles, const std::string &file_path,
                      std::vector<ValidationError> &errors)
{
    if (!roles.period_column.has_value() || roles.dmu_column.empty())
    {
        errors.push_back({.rule_id = "V-003",
                          .file = file_path,
                          .line = 0,
                          .column = "period",
                          .message = "面板数据需同时存在 dmu 与 period 列",
                          .hint = "长表 schema 见 design_docs/02 册 §2.3"});
        return PanelData{};
    }
    const auto period_index = find_column(table, *roles.period_column);
    if (!period_index.has_value())
    {
        errors.push_back({.rule_id = "V-003",
                          .file = file_path,
                          .line = 0,
                          .column = "period",
                          .message = "period 列缺失于表头",
                          .hint = "长表 schema 见 design_docs/02 册 §2.3"});
        return PanelData{};
    }

    // 期次按首次出现顺序排列（保持时序语义，不字典序）
    std::vector<std::string> period_order;
    std::map<std::string, CsvTable> grouped;
    for (const auto &row : table.rows)
    {
        const std::string &period = row[*period_index];
        auto &section = grouped[period];
        if (section.headers.empty())
        {
            period_order.push_back(period);
            section.headers = table.headers;
        }
        section.rows.push_back(row);
    }

    PanelData panel;
    std::set<std::string> dmu_union;
    for (const auto &period : period_order)
    {
        panel.periods.push_back(period);
        panel.sections[period] = build_dataset(grouped[period], roles, file_path, errors);
        for (const auto &name : panel.sections[period].dmu_names)
        {
            dmu_union.insert(name);
        }
    }
    // 全体 DMU 并集按首现顺序（跨期次）
    for (const auto &period : period_order)
    {
        for (const auto &name : panel.sections[period].dmu_names)
        {
            if (std::ranges::find(panel.dmu_names, name) == panel.dmu_names.end())
            {
                panel.dmu_names.push_back(name);
            }
        }
    }
    static_cast<void>(dmu_union);
    return panel;
}
} // namespace opensuperdea::dataset
