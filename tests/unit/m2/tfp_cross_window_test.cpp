/**
 * @file tfp_cross_window_test.cpp
 * @brief 分支 3 测试：交叉效率手算值、窗口分析 L3、Malmquist PR-05 恒等式、虚拟 DMU。
 * @details 交叉效率期望值经 deaR cross_efficiency 对拍确认（parity 批）；
 *          Malmquist 期望值经 R productivity malm() 对拍确认（Färe 1994 惯例）。
 */
#include "core/cross/cross_efficiency.h"
#include "core/registry/model_runner.h"
#include "core/tfp/malmquist.h"
#include "core/window/window_analysis.h"

#include <gtest/gtest.h>

#include <cmath>
#include <string>

#include "core/dataset/csv_parser.h"
#include "core/dataset/dataset_builder.h"
#include "dataset_fixtures.h"

namespace
{
using opensuperdea::cross::CrossMethod;
using opensuperdea::cross::CrossRunOptions;
using opensuperdea::dataset::Dataset;
using opensuperdea::registry::Orientation;
using opensuperdea::registry::Rts;
using opensuperdea::registry::RunConfig;
using opensuperdea::registry::SolveOptions;
using opensuperdea::result::ModelResult;

RunConfig make_config()
{
    RunConfig config;
    config.version = "test";
    return config;
}

void expect_near(double actual, double expected)
{
    EXPECT_NEAR(actual, expected, 1e-9) << "actual=" << actual << " expected=" << expected;
}

/** @brief 读 toy_panel 长表为面板数据。 */
opensuperdea::dataset::PanelData load_toy_panel()
{
    const std::string path =
        std::string(OPENSUPERDEA_SOURCE_DIR) + "/tests/golden/toy_panel/data.csv";
    opensuperdea::dataset::CsvParseError parse_error{};
    const auto table = opensuperdea::dataset::read_csv_file(path, &parse_error);
    opensuperdea::dataset::BuildOptions options;
    std::vector<opensuperdea::dataset::ValidationError> errors;
    const auto roles = opensuperdea::dataset::identify_roles(table, options, path, errors);
    return opensuperdea::dataset::build_panel(table, roles, path, errors);
}
} // namespace

// ---- 交叉效率（06 册 §8；deaR M2/M3 同值）----

TEST(CrossEfficiency, FR_CROSS_001_DoyleGreenAggressiveAndBenevolent)
{
    const Dataset data = test_fixtures::make_toy_cross();
    CrossRunOptions options;
    options.method = CrossMethod::DG_AGGRESSIVE;
    const ModelResult aggressive =
        opensuperdea::cross::run_cross_efficiency(data, options, make_config());
    expect_near(aggressive.dmu_results[0].score, 0.3888888889);
    expect_near(aggressive.dmu_results[1].score, 0.6944444444);
    expect_near(aggressive.dmu_results[2].score, 1.0);
    expect_near(aggressive.dmu_results[3].score, 0.8944444444);
    // 矩阵对角 = 自评（CCR-I 效率）
    ASSERT_TRUE(aggressive.cross.has_value());
    const opensuperdea::result::CrossMatrix aggressive_matrix =
        aggressive.cross.value_or(opensuperdea::result::CrossMatrix{});
    EXPECT_NEAR(aggressive_matrix.values[2][2], 1.0, 1e-9);
    options.method = CrossMethod::DG_BENEVOLENT;
    const ModelResult benevolent =
        opensuperdea::cross::run_cross_efficiency(data, options, make_config());
    expect_near(benevolent.dmu_results[0].score, 0.4444444444);
    expect_near(benevolent.dmu_results[3].score, 0.875);
}

TEST(CrossEfficiency, FR_CROSS_002_M3CorrectionMatchesDear)
{
    const Dataset data = test_fixtures::make_toy_cross();
    CrossRunOptions options;
    options.method = CrossMethod::M3_AGGRESSIVE;
    const ModelResult m3 = opensuperdea::cross::run_cross_efficiency(data, options, make_config());
    expect_near(m3.dmu_results[0].score, 0.3888888889); // deaR M3_agg 同值
    options.method = CrossMethod::M3_BENEVOLENT;
    const ModelResult m3b = opensuperdea::cross::run_cross_efficiency(data, options, make_config());
    expect_near(m3b.dmu_results[3].score, 0.875);
}

TEST(CrossEfficiency, FR_CROSS_003_VirtualDmuSecondStage)
{
    // 虚拟 DMU（x_V=(3,2), y_V=(3)）：L3 断言矩阵元素 ∈ [0,1] 且 C 收到均值 1
    const Dataset data = test_fixtures::make_toy_cross();
    CrossRunOptions options;
    options.method = CrossMethod::VIRTUAL_DMU;
    options.virtual_inputs = {3.0, 2.0};
    options.virtual_outputs = {3.0};
    const ModelResult result =
        opensuperdea::cross::run_cross_efficiency(data, options, make_config());
    const opensuperdea::result::CrossMatrix matrix =
        result.cross.value_or(opensuperdea::result::CrossMatrix{});
    for (const auto &row : matrix.values)
    {
        for (const double value : row)
        {
            EXPECT_GE(value, -1e-9);
            EXPECT_LE(value, 1.0 + 1e-9);
        }
    }
    EXPECT_NEAR(result.dmu_results[2].score, 1.0, 1e-9);
}

TEST(CrossEfficiency, FR_CROSS_004_MatrixMeansConsistency)
{
    const Dataset data = test_fixtures::make_toy_cross();
    CrossRunOptions options;
    const ModelResult result =
        opensuperdea::cross::run_cross_efficiency(data, options, make_config());
    const opensuperdea::result::CrossMatrix matrix =
        result.cross.value_or(opensuperdea::result::CrossMatrix{});
    // 列均值（剔除自评）= dmu_results score（FR-CROSS-004 契约）
    for (std::size_t j = 0; j < data.dmu_count(); ++j)
    {
        double sum = 0.0;
        for (std::size_t i = 0; i < data.dmu_count(); ++i)
        {
            if (i != j)
            {
                sum += matrix.values[i][j];
            }
        }
        expect_near(matrix.mean_received[j], sum / static_cast<double>(data.dmu_count() - 1));
        expect_near(result.dmu_results[j].score, matrix.mean_received[j]);
    }
}

// ---- 窗口分析（06 册 §9）----

TEST(WindowAnalysis, FR_WIN_001_WindowWidthOneEqualsPeriodDea)
{
    // L3：w=1 时各窗口效率 = 单期截面 DEA（参考集退化一致性）
    const opensuperdea::dataset::PanelData panel = load_toy_panel();
    SolveOptions options;
    options.orientation = Orientation::INPUT;
    options.rts = Rts::CRS;
    const ModelResult window1 =
        opensuperdea::window::run_window_analysis(panel, 1, "ccr", options, make_config());
    for (const auto &period : panel.periods)
    {
        const ModelResult cross_section = opensuperdea::registry::run_model(
            "ccr", panel.sections.at(period), options, make_config());
        for (std::size_t d = 0; d < cross_section.dmu_results.size(); ++d)
        {
            const std::string unit_name = panel.sections.at(period).dmu_names[d] + "@" + period;
            bool found = false;
            for (const auto &row : window1.dmu_results)
            {
                if (row.annotations.at("unit_name") == unit_name)
                {
                    expect_near(row.score, cross_section.dmu_results[d].score);
                    found = true;
                    break;
                }
            }
            EXPECT_TRUE(found) << unit_name;
        }
    }
}

TEST(WindowAnalysis, FR_WIN_001_WindowWidthValidation)
{
    const opensuperdea::dataset::PanelData panel = load_toy_panel();
    SolveOptions options;
    const RunConfig config = make_config();
    EXPECT_THROW(static_cast<void>(
                     opensuperdea::window::run_window_analysis(panel, 0, "ccr", options, config)),
                 opensuperdea::registry::EngineException);
    EXPECT_THROW(static_cast<void>(
                     opensuperdea::window::run_window_analysis(panel, 3, "ccr", options, config)),
                 opensuperdea::registry::EngineException); // > 期次数 2
}

// ---- 相邻 Malmquist（06 册 §10；Färe 1994 惯例）----

TEST(MalmquistFamily, FR_TFP_001_MalmquistMatchesProductivityConvention)
{
    const opensuperdea::dataset::PanelData panel = load_toy_panel();
    SolveOptions options;
    options.rts = Rts::CRS;
    const ModelResult result = opensuperdea::tfp::run_malmquist(panel, options, make_config());
    // R productivity 1.1.2 malm() 同值（Färe 1994 惯例，MI > 1 = 进步）
    expect_near(result.dmu_results[0].score, 1.6);
    expect_near(result.dmu_results[1].score, 1.32);
    expect_near(result.dmu_results[2].score, 1.219047619);
}

TEST(MalmquistFamily, L3_FR_TFP_001_MalmquistIdentityECxTC)
{
    // PR-05：MI = EC × TC（全 DMU × 全相邻期对，浮点容差内）
    const opensuperdea::dataset::PanelData panel = load_toy_panel();
    for (const Rts rts : {Rts::CRS, Rts::VRS})
    {
        SolveOptions options;
        options.rts = rts;
        const ModelResult result = opensuperdea::tfp::run_malmquist(panel, options, make_config());
        for (const auto &dmu : result.dmu_results)
        {
            const double ec = std::stod(dmu.annotations.at("ec"));
            const double tc = std::stod(dmu.annotations.at("tc"));
            EXPECT_NEAR(dmu.score, ec * tc, 1e-9) << "rts=" << static_cast<int>(rts);
        }
    }
}
