/**
 * @file nonradial_test.cpp
 * @brief 分支 2 模型族单元测试：SBM/DDF/FDH/价格族/加性族/ERM/DSBM 手算值 + L3。
 * @details 手算期望值均经 deaR 对拍确认（tests/parity/parity_manifest.json 对应批）；
 *          DSBM/BAM 无开源对照项，按 P-006 以 L2 构造值 + L3 性质兜底。
 */
#include "core/registry/model_runner.h"

#include <gtest/gtest.h>

#include <cmath>
#include <string>

#include "dataset_fixtures.h"

#include "core/dataset/csv_parser.h"
#include "core/dataset/dataset_builder.h"

namespace
{
using opensuperdea::dataset::Dataset;
using opensuperdea::registry::Orientation;
using opensuperdea::registry::Rts;
using opensuperdea::registry::RunConfig;
using opensuperdea::registry::SolveOptions;
using opensuperdea::result::DmuStatus;
using opensuperdea::result::ModelResult;

/** @brief 以指定组合运行模型（可选方向/数据集覆写）。 */
ModelResult run_model(const std::string &model_id, Orientation orientation, Rts rts,
                      const std::string &direction = "", const Dataset *data_override = nullptr,
                      bool super = false)
{
    const Dataset data =
        data_override != nullptr ? *data_override : test_fixtures::make_toy_cross();
    SolveOptions options;
    options.orientation = orientation;
    options.rts = rts;
    if (!direction.empty())
    {
        options.model_options["direction"] = direction;
    }
    RunConfig config;
    config.super_efficiency = super;
    config.version = "test";
    return opensuperdea::registry::run_model(model_id, data, options, config);
}

std::vector<double> scores_of(const ModelResult &result)
{
    std::vector<double> scores;
    scores.reserve(result.dmu_results.size());
    for (const auto &dmu : result.dmu_results)
    {
        scores.push_back(dmu.score);
    }
    return scores;
}

void expect_near(double actual, double expected)
{
    EXPECT_NEAR(actual, expected, 1e-9) << "actual=" << actual << " expected=" << expected;
}
} // namespace

// ---- SBM（deaR 对拍同值；06 册 §4.1）----

TEST(NonradialFamily, FR_NRAD_001_SbmHandComputedAllOrientations)
{
    const auto crs = scores_of(run_model("sbm", Orientation::NON_DIRECTIONAL, Rts::CRS));
    expect_near(crs[0], 0.4166666667);
    expect_near(crs[1], 0.7083333333);
    expect_near(crs[2], 1.0);
    expect_near(crs[3], 0.9041666667);
    const auto out_crs = scores_of(run_model("sbm", Orientation::OUTPUT, Rts::CRS));
    expect_near(out_crs[0], 0.5);
    const auto in_vrs = scores_of(run_model("sbm", Orientation::INPUT, Rts::VRS));
    expect_near(in_vrs[1], 0.9166666667);
}

TEST(NonradialFamily, FR_NRAD_002_SbmSuperEfficiencyHandComputed)
{
    // 非导向超效率（Tone 2002）：无效 DMU 退化 1，C = 31/28（deaR delta 同值）
    const ModelResult result =
        run_model("sbm", Orientation::NON_DIRECTIONAL, Rts::CRS, "", nullptr, true);
    expect_near(result.dmu_results[2].score, 1.1071428571);
    EXPECT_EQ(result.dmu_results[2].status, DmuStatus::SUPER_EFFICIENT);
    EXPECT_EQ(result.dmu_results[0].status, DmuStatus::INEFFICIENT);
}

TEST(NonradialFamily, FR_NRAD_004_SbmWeightedHandComputed)
{
    // 加权 SBM 用 toy_weight 数据集（权重 0.7/0.3/1）
    const Dataset data = test_fixtures::make_toy_weight();
    const auto weighted =
        scores_of(run_model("sbm_weighted", Orientation::NON_DIRECTIONAL, Rts::CRS, "", &data));
    expect_near(weighted[0], 0.3833333333); // deaR 同值（parity manifest toy_weight_sbm_weighted）
}

// ---- DDF（06 册 §5）----

TEST(DistanceFamily, FR_DDF_001_DdfOwnAndUnitDirectionsHandComputed)
{
    const auto own = scores_of(run_model("ddf", Orientation::INPUT, Rts::CRS, "own"));
    expect_near(own[0], 0.5);
    expect_near(own[1], 0.25);
    expect_near(own[2], 0.0);
    expect_near(own[3], 1.0 / 15.0);
    const auto unit = scores_of(run_model("ddf", Orientation::INPUT, Rts::CRS, "unit"));
    expect_near(unit[1], 0.5);
    expect_near(unit[3], 1.0 / 3.0);
}

TEST(DistanceFamily, FR_DDF_002_RdmMatchesDearRdm)
{
    // RDM（deaR model_rdm 口径：VRS + non；maxslack=FALSE 纯 β）
    const auto scores = scores_of(run_model("ddf", Orientation::NON_DIRECTIONAL, Rts::VRS, "rdm"));
    expect_near(scores[1], 1.0 / 13.0);
}

TEST(DistanceFamily, FR_DDF_003_CustomDirectionConsumesDiDoColumns)
{
    const std::string path =
        std::string(OPENSUPERDEA_SOURCE_DIR) + "/tests/golden/toy_ddf/data.csv";
    opensuperdea::dataset::CsvParseError parse_error{};
    const auto table = opensuperdea::dataset::read_csv_file(path, &parse_error);
    ASSERT_TRUE(parse_error.message.empty());
    opensuperdea::dataset::BuildOptions options;
    std::vector<opensuperdea::dataset::ValidationError> errors;
    const auto roles = opensuperdea::dataset::identify_roles(table, options, path, errors);
    ASSERT_TRUE(errors.empty());
    const auto data = opensuperdea::dataset::build_dataset(table, roles, path, errors);
    ASSERT_TRUE(data.directions.has_value());
    const auto scores = scores_of(run_model("ddf", Orientation::INPUT, Rts::CRS, "custom", &data));
    expect_near(scores[0], 1.0); // g=(1,0.5)：λC=1/6 时 β 受产出侧约束 β≤1（deaR 同值）
}

// ---- FDH（MILP；06 册 §7）----

TEST(FdhFamily, FR_FDH_001_FdhBinaryDominationHandComputed)
{
    const Dataset data = test_fixtures::make_toy_fdh();
    const auto in_scores = scores_of(run_model("fdh", Orientation::INPUT, Rts::CRS, "", &data));
    // A-D 互不支配 → 前沿；E 被 A 支配 → 0.8333（deaR model_fdh 同值）
    expect_near(in_scores[0], 1.0);
    expect_near(in_scores[4], 5.0 / 6.0);
    const auto out_scores = scores_of(run_model("fdh", Orientation::OUTPUT, Rts::CRS, "", &data));
    expect_near(out_scores[4], 1.0); // 产出导向下 E 不被产出支配
}

// ---- 价格族（06 册 §6）----

TEST(PriceFamily, FR_COST_001_CostEfficiencyWithDecomposition)
{
    const Dataset data = test_fixtures::make_toy_price();
    const ModelResult result = run_model("cost", Orientation::NON_DIRECTIONAL, Rts::CRS, "", &data);
    const auto scores = scores_of(result);
    expect_near(scores[0], 0.375);      // deaR 同值（min 3 / actual 8）
    expect_near(scores[1], 9.0 / 13.0); // deaR 同值（min 9 / actual 13）
    expect_near(scores[2], 1.0);
    // 分解标注：TE（径向）× AE = CE
    const auto &a = result.dmu_results[0];
    const double te = std::stod(a.annotations.at("te"));
    const double ae = std::stod(a.annotations.at("ae"));
    expect_near(te * ae, scores[0]);
}

TEST(PriceFamily, FR_COST_002_RevenueEfficiency)
{
    const Dataset data = test_fixtures::make_toy_price();
    const auto scores =
        scores_of(run_model("revenue", Orientation::NON_DIRECTIONAL, Rts::CRS, "", &data));
    expect_near(scores[0], 0.5);
    expect_near(scores[3], 14.0 / 15.0);
}

TEST(PriceFamily, FR_COST_003_ProfitEfficiencyCanBeNegative)
{
    const Dataset data = test_fixtures::make_toy_price();
    const auto scores =
        scores_of(run_model("profit", Orientation::NON_DIRECTIONAL, Rts::VRS, "", &data));
    expect_near(scores[0], -2.0 / 3.0); // 手算（deaR restricted 口径不同，见 manifest 注记）
    expect_near(scores[1], -1.0 / 6.0);
    expect_near(scores[2], 1.0);
    expect_near(scores[3], 5.0 / 6.0);
}

TEST(PriceFamily, FR_COST_003_ProfitCrsUnboundedMarkedInfeasible)
{
    const Dataset data = test_fixtures::make_toy_price();
    const ModelResult result =
        run_model("profit", Orientation::NON_DIRECTIONAL, Rts::CRS, "", &data);
    EXPECT_EQ(result.dmu_results[0].status, DmuStatus::INFEASIBLE);
    EXPECT_FALSE(result.dmu_results[0].has_score());
}

// ---- 加权加性族（06 册 §4.4；score = 加权松弛和，有效为 0）----

TEST(AdditiveFamily, FR_NRAD_004_AdditiveRawHandComputed)
{
    const auto scores = scores_of(run_model("additive", Orientation::NON_DIRECTIONAL, Rts::CRS));
    expect_near(scores[0], 11.0 / 6.0); // deaR objval 同值
    expect_near(scores[1], 1.5);
    expect_near(scores[2], 0.0);
    expect_near(scores[3], 5.0 / 6.0);
    EXPECT_EQ(run_model("additive", Orientation::NON_DIRECTIONAL, Rts::CRS).dmu_results[2].status,
              DmuStatus::EFFICIENT);
}

TEST(AdditiveFamily, FR_NRAD_004_NormalizedAndRangeWeightsMatchDear)
{
    const auto normalized =
        scores_of(run_model("additive_normalized", Orientation::NON_DIRECTIONAL, Rts::CRS));
    expect_near(normalized[0], 0.5809523810);
    const auto ram = scores_of(run_model("ram", Orientation::NON_DIRECTIONAL, Rts::CRS));
    expect_near(ram[0], 0.6111111111);
    expect_near(ram[3], 0.2777777778);
}

TEST(AdditiveFamily, FR_NRAD_004_BamEqualsRamWithRangeBounds)
{
    // M2 的 BAM 界 = 数据范围（与 RAM 同权重）；数值恒等即实现正确（L2 构造断言）
    const auto ram = scores_of(run_model("ram", Orientation::NON_DIRECTIONAL, Rts::CRS));
    const auto bam = scores_of(run_model("bam", Orientation::NON_DIRECTIONAL, Rts::CRS));
    ASSERT_EQ(ram.size(), bam.size());
    for (std::size_t i = 0; i < ram.size(); ++i)
    {
        expect_near(bam[i], ram[i]);
    }
}

TEST(AdditiveFamily, FR_NRAD_004_WeightedAdditiveUsesDatasetWeights)
{
    const Dataset data = test_fixtures::make_toy_weight();
    const auto scores = scores_of(
        run_model("additive_weighted", Orientation::NON_DIRECTIONAL, Rts::CRS, "", &data));
    expect_near(scores[0], 1.4666666667); // deaR 同值
}

TEST(AdditiveFamily, L3_FR_NRAD_004_AdditiveEfficientDmuZeroScore)
{
    // L3（构造算例全扫描）：CRS 下有效 DMU 松弛和为 0、无效 > 0；
    // VRS 下 A 为强有效极点（score=0），径向无效的 B 松弛和 > 0
    const auto crs = scores_of(run_model("additive", Orientation::NON_DIRECTIONAL, Rts::CRS));
    EXPECT_LE(crs[2], 1e-9);
    EXPECT_GT(crs[0], 0.0);
    const auto vrs = scores_of(run_model("additive", Orientation::NON_DIRECTIONAL, Rts::VRS));
    EXPECT_LE(vrs[0], 1e-9);
    EXPECT_GT(vrs[1], 0.0);
}

// ---- DSBM（P-006 兜底：L2 构造值 + L3）----

TEST(DsbmFamily, FR_NRAD_004_DsbmSanityAndUnitDirection)
{
    // own 方向 = SBM 同构（归一基 g=(x0,y0)）→ 与 SBM non 同值（L2 构造断言）
    const auto sbm = scores_of(run_model("sbm", Orientation::NON_DIRECTIONAL, Rts::CRS));
    const auto dsbm = scores_of(run_model("dsbm", Orientation::NON_DIRECTIONAL, Rts::CRS, "own"));
    for (std::size_t i = 0; i < sbm.size(); ++i)
    {
        expect_near(dsbm[i], sbm[i]);
    }
    // unit 方向 L3：∈ [0,1]，有效 DMU 为 1
    const auto unit = scores_of(run_model("dsbm", Orientation::NON_DIRECTIONAL, Rts::CRS, "unit"));
    expect_near(unit[2], 1.0);
    for (const double score : unit)
    {
        EXPECT_GE(score, 0.0);
        EXPECT_LE(score, 1.0 + 1e-9);
    }
}

// ---- ERM（06 册 §4.3）----

TEST(ErmFamily, FR_NRAD_003_ErmRussellHandComputed)
{
    const auto in_scores = scores_of(run_model("erm", Orientation::INPUT, Rts::CRS));
    expect_near(in_scores[0], 5.0 / 12.0); // deaR mean_efficiency 同值
    expect_near(in_scores[1], 17.0 / 24.0);
    expect_near(in_scores[2], 1.0);
    const auto out_scores = scores_of(run_model("erm", Orientation::OUTPUT, Rts::CRS));
    expect_near(out_scores[0], 0.5); // 1/φ̄（deaR oo reciprocal 同值）
    expect_near(out_scores[3], 14.0 / 15.0);
}

TEST(ErmFamily, FR_NRAD_003_ErmNonOrientedEqualsSbm)
{
    // Tone 2001 恒等：ERM 图测度比值形 = SBM（L2 构造断言）
    const auto sbm = scores_of(run_model("sbm", Orientation::NON_DIRECTIONAL, Rts::CRS));
    const auto erm = scores_of(run_model("erm", Orientation::NON_DIRECTIONAL, Rts::CRS));
    for (std::size_t i = 0; i < sbm.size(); ++i)
    {
        expect_near(erm[i], sbm[i]);
    }
}
