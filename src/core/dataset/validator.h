/**
 * @file validator.h
 * @brief 数据校验规则表（FR-IO-005；规则编号对应 design_docs/02 册 §3）。
 * @details 错误聚合输出：单次校验报告全部错误而非首个；错误携带
 *          文件:行号:列名:原因:修复提示 五元组，映射 EXIT_DATA（FR-CLI-007）。
 */
#pragma once

#include <string>
#include <vector>

#include "csv_parser.h"
#include "dataset.h"

namespace opensuperdea::dataset
{
/** @brief 校验错误记录（对应 02 册规则表 V-001~V-010）。 */
struct ValidationError
{
    std::string rule_id; // 如 "V-004"
    std::string file;    // 来源文件（可为 "<memory>"）
    std::size_t line;    // 1 基物理行号；文件级错误为 0
    std::string column;  // 列名；文件级错误为空
    std::string message; // 原因
    std::string hint;    // 修复提示
};

/**
 * @brief 校验规则表条目：规则函数签名（输入 CsvTable 与角色列索引，追加错误）。
 */
using ValidationRule = void (*)(const CsvTable &, const std::vector<std::string> &,
                                const std::vector<std::string> &, std::vector<ValidationError> &);

/**
 * @brief 对 CSV 表执行全部 M1 生效规则（V-001~V-005、V-009），错误聚合返回。
 * @param table 已解析 CSV 表
 * @param input_columns 投入列名（前缀识别或 CLI 覆写后）
 * @param output_columns 产出列名
 * @param file_path 报告用文件路径
 * @param is_panel 面板长表模式：V-005 跳过 DMU 名重复检查（跨期多行合法，
 *                 唯一性由 V-009 的 (dmu, period) 组合保证），空名仍报
 * @return 全部校验错误（空 = 通过）
 */
std::vector<ValidationError> validate_table(const CsvTable &table,
                                            const std::vector<std::string> &input_columns,
                                            const std::vector<std::string> &output_columns,
                                            const std::string &file_path, bool is_panel = false);
} // namespace opensuperdea::dataset
