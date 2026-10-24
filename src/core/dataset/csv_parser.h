/**
 * @file csv_parser.h
 * @brief CSV 词法解析：UTF-8 文本 → 表头 + 数据行（不涉及语义角色，见 dataset_builder）。
 * @details 容忍规则（design_docs/02 册 §2）：UTF-8 BOM 跳过；CRLF/LF/CR 换行；
 *          `#` 起始行与空行跳过；不支持引号包裹（DEA 数据为纯数值场景）。
 */
#pragma once

#include <string>
#include <string_view>
#include <vector>

namespace opensuperdea::dataset
{
/** @brief 解析后的 CSV 表：headers[0] 行为表头，其后每行为数据行。 */
struct CsvTable
{
    std::vector<std::string> headers;
    std::vector<std::vector<std::string>> rows; // 与表头同列数
};

/** @brief CSV 解析失败原因（词法层：列数不一致等）。 */
struct CsvParseError
{
    std::size_t line_number; // 1 基，物理行号
    std::string message;
};

/**
 * @brief 解析 CSV 文本。
 * @param text UTF-8 文本（可含 BOM/CRLF）
 * @param parse_error 非空时接收首个词法错误；返回空表
 * @return 解析结果；词法错误时返回空 CsvTable
 */
CsvTable parse_csv_text(std::string_view text, CsvParseError *parse_error);

/**
 * @brief 读取并解析 CSV 文件（文件不存在/不可读时报错）。
 * @param path 文件路径
 * @param parse_error 非空时接收错误；返回空表
 * @return 解析结果
 */
CsvTable read_csv_file(const std::string &path, CsvParseError *parse_error);
} // namespace opensuperdea::dataset
