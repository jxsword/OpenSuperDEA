/**
 * @file model_result.h
 * @brief 一次模型运行的全部结果与元信息（design_docs/02 册 §1 ModelResult 契约）。
 * @details 由 ModelRunner 聚合各 DmuResult 生成；CSV/JSON 导出器与本结构解耦，
 *          列集由 ModelDescriptor 的 ResultColumnSpec 声明（01 册 §4.1）。
 */
#pragma once

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

#include "core/registry/solve_options.h"
#include "dmu_result.h"

namespace opensuperdea::result
{
/** @brief 运行元信息（JSON meta 与导出文件名共用）。 */
struct ResultMeta
{
    std::string model_id;
    registry::Orientation orientation = registry::Orientation::INPUT;
    registry::Rts rts = registry::Rts::CRS;
    bool super_efficiency = false; // 本次运行是否为超效率（参考集 exclude_self）
    std::size_t dmu_count = 0;     // n
    std::size_t input_count = 0;   // m
    std::size_t output_count = 0;  // s
    std::string version;           // 引擎版本（FR-CLI-001 同源）
    std::string data_sha1;         // 数据文件指纹（02 册 §4.2；对拍同源判定）
    double elapsed_ms = 0.0;       // 求解总耗时
    std::vector<std::string> warnings;
};

/** @brief 汇总统计（FR-OUT-006：排名/均值/中位数/有效计数）。 */
struct ResultSummary
{
    std::vector<std::size_t> rank; // 按 score 降序的竞争排名（并列同名），与 DMU 顺序对齐
    double mean_score = 0.0;
    double median_score = 0.0;
    std::size_t efficient_count = 0; // 含 super_efficient
};

/** @brief 交叉效率矩阵（FR-CROSS-004：评者×被评者 + 行/列均值）。 */
struct CrossMatrix
{
    std::vector<std::vector<double>> values; // values[i][j] = 评者 i 对被评 j 的交叉效率
    std::vector<double> mean_given;          // 行均值 A_i（评者 i 给其余 DMU 的平均，剔除自评）
    std::vector<double> mean_received;       // 列均值 e_j（被评 j 收到的平均交叉效率，剔除自评）
};

/** @brief 一次模型运行的结果聚合。 */
struct ModelResult
{
    ResultMeta meta;
    std::vector<DmuResult> dmu_results; // 与数据集 DMU 顺序一致
    ResultSummary summary;
    std::optional<CrossMatrix> cross; // 仅交叉效率模型产出（FR-CROSS-004）
};
} // namespace opensuperdea::result
