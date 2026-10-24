/**
 * @file dataset.h
 * @brief 核心数据模型：截面数据集 / 面板数据 / 价格与权重附属数据（SRS FR-ENG-004）。
 * @details 结构契约见 design_docs/02_data_design.md §1；矩阵以 Eigen 双精度承载，
 *          M3/M4 稀疏化在 solver 层进行，本层接口不变（SRS FR-PERF-004）。
 */
#pragma once

#include <map>
#include <optional>
#include <string>
#include <vector>

#include <Eigen/Dense>

namespace opensuperdea::dataset
{
/** @brief 价格附属数据：成本（投入价格）/收益（产出价格）模型消费（FR-IO-003）。 */
struct PriceData
{
    Eigen::MatrixXd input_prices;  // n×m（pi_ 列）
    Eigen::MatrixXd output_prices; // n×s（po_ 列）
};

/** @brief 偏好固定权重（FR-NEG-001，w_in_/w_out_ 列）。 */
struct WeightData
{
    Eigen::VectorXd input_weights;  // m
    Eigen::VectorXd output_weights; // s
};

/** @brief 自定义方向向量（FR-IO-002，di_/do_ 列；DDF 自定义方向消费）。 */
struct DirectionData
{
    Eigen::MatrixXd input_directions;  // n×m
    Eigen::MatrixXd output_directions; // n×s
};

/** @brief 截面数据集：n 个 DMU × (m 投入 + s 产出)。 */
struct Dataset
{
    std::vector<std::string> dmu_names;
    Eigen::MatrixXd inputs;  // n×m
    Eigen::MatrixXd outputs; // n×s

    std::optional<DirectionData> directions;
    std::optional<PriceData> prices;
    std::optional<WeightData> weights;

    // 列名（in_*/out_* 或 CLI 覆写名；02 册 §4.2 JSON 键与 CSV 列名来源；
    // 程序内直接构造的测试数据可留空，导出器回退 in_N/out_N）
    std::vector<std::string> input_names;
    std::vector<std::string> output_names;

    /** @brief DMU 数量。 */
    [[nodiscard]] std::size_t dmu_count() const;
    /** @brief 投入维数。 */
    [[nodiscard]] std::size_t input_count() const;
    /** @brief 产出维数。 */
    [[nodiscard]] std::size_t output_count() const;
};

/** @brief 面板数据：期次 → 截面（FR-IO-004 长表 schema，P-007 裁决）。 */
struct PanelData
{
    std::vector<std::string> periods;        // 按出现顺序排列的期次标签
    std::map<std::string, Dataset> sections; // 期次 → 截面
    std::vector<std::string> dmu_names;      // 全体 DMU 并集（保序）
    /** @brief 是否平衡面板（每期 DMU 集合一致；不平衡允许并告警）。 */
    [[nodiscard]] bool is_balanced() const;
};
} // namespace opensuperdea::dataset
