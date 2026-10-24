/**
 * @file dataset_builder.h
 * @brief CSV 表 → 数据集构造（角色识别 + CLI 覆写，SRS FR-IO-001/002/004）。
 * @details 角色约定（design_docs/02 册 §2）：投入 in_*、产出 out_*、方向 di_* 与 do_*；
 *          CLI 显式列名（--inputs/--outputs）优先于前缀推断；面板数据按 period
 *          列分组构成长表 PanelData。
 */
#pragma once

#include <optional>
#include <string>
#include <vector>

#include "csv_parser.h"
#include "dataset.h"
#include "validator.h"

namespace opensuperdea::dataset
{
/** @brief 列角色识别结果：各类列名清单（保持表头顺序）。 */
struct ColumnRoles
{
    std::vector<std::string> dmu_column;            // 0 或 1 个元素
    std::optional<std::string> period_column;       // 面板数据时存在
    std::vector<std::string> input_columns;         // in_*
    std::vector<std::string> output_columns;        // out_*
    std::vector<std::string> direction_in_columns;  // di_*
    std::vector<std::string> direction_out_columns; // do_*
};

/** @brief 构造选项：CLI 显式列名覆写（02 册 §2.4）。 */
struct BuildOptions
{
    std::vector<std::string> explicit_inputs;  // --inputs c1,c2（空 = 前缀推断）
    std::vector<std::string> explicit_outputs; // --outputs c1,c2
};

/**
 * @brief 从表头识别列角色（前缀规则 + CLI 覆写）。
 * @param table 已解析 CSV
 * @param options 构造选项
 * @param file_path 报告用文件路径
 * @param errors 追加 V-003（角色缺失）/覆盖列不存在错误
 * @return 角色识别结果
 */
ColumnRoles identify_roles(const CsvTable &table, const BuildOptions &options,
                           const std::string &file_path, std::vector<ValidationError> &errors);

/**
 * @brief 由 CSV 表构造截面数据集（含方向列，若有）。
 * @param table 已解析 CSV（调用前应先 validate_table 通过）
 * @param roles 列角色
 * @param file_path 报告用文件路径
 * @param errors 追加构造期错误（理论上是校验遗漏，防御性）
 * @return 数据集；错误时返回空 Dataset
 */
Dataset build_dataset(const CsvTable &table, const ColumnRoles &roles, const std::string &file_path,
                      std::vector<ValidationError> &errors);

/**
 * @brief 由长表 CSV 构造面板数据（period 列分组；不平衡面板允许并收集警告）。
 * @param table 已解析 CSV（需含 period 列）
 * @param roles 列角色
 * @param file_path 报告用文件路径
 * @param errors 追加构造期错误（period 缺失等走 V-003 防御）
 * @return 面板数据；错误时返回空 PanelData
 */
PanelData build_panel(const CsvTable &table, const ColumnRoles &roles, const std::string &file_path,
                      std::vector<ValidationError> &errors);
} // namespace opensuperdea::dataset
