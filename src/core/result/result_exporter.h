/**
 * @file result_exporter.h
 * @brief 结果导出：CSV（默认，P-011）/ JSON（--format json，P-011），02 册 §4 契约。
 * @details 列集由 ModelDescriptor::ResultColumnSpec 声明、按数据集维度展开
 *          （01 册 §4.1）；数值 10 位有效数字（02 册 §4.1，对拍容差余量）；
 *          不可行 DMU 不输出数值（空单元格 / JSON null）。
 */
#pragma once

#include <string>
#include <vector>

#include "core/dataset/dataset.h"
#include "core/registry/model_descriptor.h"
#include "model_result.h"

namespace opensuperdea::result
{
/** @brief 导出所需的名字集（数据集行名/列名；窗口分析等合成行场景独立构造）。 */
struct ExportNames
{
    std::vector<std::string> row_names;   // 结果行名（与 dmu_results 对齐）
    std::vector<std::string> input_names; // 投入列名（空则回退 in_N）
    std::vector<std::string> output_names;

    /** @brief 由截面数据集构造（常规路径）。 */
    [[nodiscard]] static ExportNames from_dataset(const dataset::Dataset &data)
    {
        return {.row_names = data.dmu_names,
                .input_names = data.input_names,
                .output_names = data.output_names};
    }
};

/**
 * @brief 结果文件名（02 册 §4.1：`<model>_<orientation>_<rts>_result.<ext>`；
 *        不消费 RTS 的模型省略 rts 段）。
 */
[[nodiscard]] std::string result_filename(const ResultMeta &meta, bool uses_rts,
                                          std::string_view extension);

/**
 * @brief 写 CSV 结果文件。
 * @param path 目标路径（父目录须已存在）
 * @param result 运行结果
 * @param names 行名/列名来源
 * @param spec 模型列集声明
 * @throws registry::EngineException 文件不可写（EXIT_INTERNAL）
 */
void write_result_csv(const std::string &path, const ModelResult &result, const ExportNames &names,
                      const registry::ResultColumnSpec &spec);

/**
 * @brief 写 JSON 结果文件（02 册 §4.2 schema；对拍 harness 的产品侧输入）。
 * @param path 目标路径（父目录须已存在）
 * @param result 运行结果
 * @param names 行名/列名来源
 * @param spec 模型列集声明
 * @throws registry::EngineException 文件不可写（EXIT_INTERNAL）
 */
void write_result_json(const std::string &path, const ModelResult &result, const ExportNames &names,
                       const registry::ResultColumnSpec &spec);
} // namespace opensuperdea::result
