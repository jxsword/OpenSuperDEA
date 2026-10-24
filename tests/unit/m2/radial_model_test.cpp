/**
 * @file radial_model_test.cpp
 * @brief 径向族/乘子/规模效率单元测试：手算小例 + L3 性质断言（PR-06/TE=PTE×SE）。
 * @details 手算期望值与 deaR 对拍（tests/parity/parity_manifest.json 径向批）
 *          双重确认；性质断言为 04 册 L3 层固定用例。
 */
#include "core/registry/model_runner.h"

#include <gtest/gtest.h>

#include <cmath>
#include <string>

#include "dataset_fixtures.h"

namespace
{
using opensuperdea::dataset::Dataset;
using opensuperdea::registry::Orientation;
using opensuperdea::registry::Rts;
using opensuperdea::registry::RunConfig;
using opensuperdea::registry::SolveOptions;
using opensuperdea::result::DmuStatus;
using opensuperdea::result::ModelResult;

/** @brief 以指定组合运行模型（toy_cross 固定数据）。 */
ModelResult run_on_toy(const std::string &model_id, Orientation orientation, Rts rts,
                       bool super = false)
{
    const Dataset data = test_fixtures::make_toy_cross();
    SolveOptions options;
    options.orientation = orientation;
    options.rts = rts;
    RunConfig config;
    config.super_efficiency = super;
    config.version = "test";
    return opensuperdea::registry::run_model(model_id, data, options, config);
}

/** @brief 双精度近等断言（对拍容差量级）。 */
void expect_near(double actual, double expected)
{
    EXPECT_NEAR(actual, expected, 1e-9) << "actual=" << actual << " expected=" << expected;
}

/**
 * @brief 手算基准（deaR 同值，见 parity manifest）：
 *        CCR-I: A=1/2 B=3/4 C=1 D=14/15；VRS-I: A=1 B=14/15 C=1 D=1；
 *        NIRS-I: A=1/2 B=3/4 C=1 D=1；NDRS-I: A=1 B=14/15 C=1 D=14/15。
 */
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
} // namespace

TEST(RadialFamily, FR_RAD_001_CcrInputHandComputed)
{
    const auto scores = scores_of(run_on_toy("ccr", Orientation::INPUT, Rts::CRS));
    ASSERT_EQ(scores.size(), 4U);
    expect_near(scores[0], 0.5);
    expect_near(scores[1], 0.75);
    expect_near(scores[2], 1.0);
    expect_near(scores[3], 14.0 / 15.0);
}

TEST(RadialFamily, FR_RAD_002_BccInputHandComputed)
{
    const auto scores = scores_of(run_on_toy("ccr", Orientation::INPUT, Rts::VRS));
    ASSERT_EQ(scores.size(), 4U);
    expect_near(scores[0], 1.0);
    expect_near(scores[1], 14.0 / 15.0);
    expect_near(scores[2], 1.0);
    expect_near(scores[3], 1.0);
}

TEST(RadialFamily, FR_RAD_003_NirsAndNdrsInputHandComputed)
{
    const auto nirs = scores_of(run_on_toy("ccr", Orientation::INPUT, Rts::NIRS));
    expect_near(nirs[0], 0.5);
    expect_near(nirs[1], 0.75);
    expect_near(nirs[2], 1.0);
    expect_near(nirs[3], 1.0);

    const auto ndrs = scores_of(run_on_toy("ccr", Orientation::INPUT, Rts::NDRS));
    expect_near(ndrs[0], 1.0);
    expect_near(ndrs[1], 14.0 / 15.0);
    expect_near(ndrs[2], 1.0);
    expect_near(ndrs[3], 14.0 / 15.0);
}

TEST(RadialFamily, FR_RAD_001_CcrOutputNormalizedMatchesInputUnderCrs)
{
    // CRS 下投入/产出导向效率一致（对偶镜像）；VRS 下允许不同
    const auto input = scores_of(run_on_toy("ccr", Orientation::INPUT, Rts::CRS));
    const auto output = scores_of(run_on_toy("ccr", Orientation::OUTPUT, Rts::CRS));
    ASSERT_EQ(input.size(), output.size());
    for (std::size_t i = 0; i < input.size(); ++i)
    {
        expect_near(output[i], input[i]);
    }
}

TEST(RadialFamily, FR_RAD_001_ProjectionAndSlackStageTwoHandComputed)
{
    // A：θ*=1/2，前沿基准 λC=1/6，s⁻_labor = 1 − 4/6 = 1/3（deaR 同值）
    const ModelResult result = run_on_toy("ccr", Orientation::INPUT, Rts::CRS);
    const auto &a = result.dmu_results[0];
    expect_near(a.score, 0.5);
    ASSERT_EQ(a.lambdas.size(), 1U);
    EXPECT_EQ(a.lambdas[0].first, 2U); // C 的数据集索引
    expect_near(a.lambdas[0].second, 1.0 / 6.0);
    ASSERT_EQ(a.input_slacks.size(), 2U);
    expect_near(a.input_slacks[0], 1.0 / 3.0);
    expect_near(a.input_slacks[1], 0.0);
    // 投影 x* = θ*x0 − s⁻ = (2/3, 1/2)
    ASSERT_EQ(a.input_targets.size(), 2U);
    expect_near(a.input_targets[0], 2.0 / 3.0);
    expect_near(a.input_targets[1], 0.5);
}

TEST(RadialFamily, L3_FR_RAD_003_RtsOrderingIdentity)
{
    // 全 DMU 恒成立：θ_CRS ≤ θ_NIRS ≤ θ_VRS 且 θ_CRS ≤ θ_NDRS ≤ θ_VRS
    for (const auto &[model_orientation, label] :
         {std::pair{Orientation::INPUT, "in"}, {Orientation::OUTPUT, "out"}})
    {
        const auto crs = scores_of(run_on_toy("ccr", model_orientation, Rts::CRS));
        const auto nirs = scores_of(run_on_toy("ccr", model_orientation, Rts::NIRS));
        const auto ndrs = scores_of(run_on_toy("ccr", model_orientation, Rts::NDRS));
        const auto vrs = scores_of(run_on_toy("ccr", model_orientation, Rts::VRS));
        for (std::size_t i = 0; i < crs.size(); ++i)
        {
            EXPECT_LE(crs[i], nirs[i] + 1e-9) << label << " DMU#" << i;
            EXPECT_LE(nirs[i], vrs[i] + 1e-9) << label << " DMU#" << i;
            EXPECT_LE(crs[i], ndrs[i] + 1e-9) << label << " DMU#" << i;
            EXPECT_LE(ndrs[i], vrs[i] + 1e-9) << label << " DMU#" << i;
        }
    }
}

TEST(RadialFamily, L3_FR_RAD_004_ScaleEfficiencyIdentityAndRegion)
{
    // TE = PTE × SE（恒等式）+ 区间判定：SE≈1 → crs；θ_NIRS=θ_VRS → drs；否则 irs
    const ModelResult result = run_on_toy("scale_efficiency", Orientation::INPUT, Rts::CRS);
    ASSERT_EQ(result.dmu_results.size(), 4U);
    for (const auto &dmu : result.dmu_results)
    {
        const double te = std::stod(dmu.annotations.at("te"));
        const double pte = std::stod(dmu.annotations.at("pte"));
        const double se = std::stod(dmu.annotations.at("se"));
        expect_near(te, pte * se);
        if (std::abs(se - 1.0) <= 1e-9)
        {
            EXPECT_EQ(dmu.annotations.at("rts_region"), "crs");
        }
    }
    // 手算抽查：A 的 TE=1/2、PTE=1 → SE=1/2，θ_NIRS=1/2 < θ_VRS=1 → irs
    const auto &a = result.dmu_results[0];
    expect_near(std::stod(a.annotations.at("te")), 0.5);
    expect_near(std::stod(a.annotations.at("pte")), 1.0);
    EXPECT_EQ(a.annotations.at("rts_region"), "irs");
}

TEST(RadialFamily, L3_FR_MULT_002_MultiplierEnvelopeDualityFullScan)
{
    // PR-06：乘子最优值 = 包络最优值（全数据集扫描 × crs/vrs × in/out）
    for (const Rts rts : {Rts::CRS, Rts::VRS})
    {
        for (const Orientation orientation : {Orientation::INPUT, Orientation::OUTPUT})
        {
            const auto envelope = scores_of(run_on_toy("ccr", orientation, rts));
            const auto multiplier = scores_of(run_on_toy("multiplier", orientation, rts));
            ASSERT_EQ(envelope.size(), multiplier.size());
            for (std::size_t i = 0; i < envelope.size(); ++i)
            {
                // expect_near 自带数值输出；组合上下文由循环序保证（crs/vrs × in/out）
                expect_near(multiplier[i], envelope[i]);
            }
        }
    }
}

TEST(RadialFamily, FR_OUT_005_MultiplierOutputsOptimalMultipliers)
{
    const ModelResult result = run_on_toy("multiplier", Orientation::INPUT, Rts::CRS);
    const auto &c = result.dmu_results[2]; // C 有效：v·x0=1 且 u·y0=1
    ASSERT_EQ(c.input_multipliers.size(), 2U);
    ASSERT_EQ(c.output_multipliers.size(), 1U);
    expect_near(c.score, 1.0);
    // v·x0 = 1 归一化断言
    const double v_dot_x0 = (c.input_multipliers[0] * 4.0) + (c.input_multipliers[1] * 3.0);
    expect_near(v_dot_x0, 1.0);
}

TEST(RadialFamily, FR_RAD_005_SuperEfficiencyEfficientDmuAboveOne)
{
    // CRS：仅 C 有效 → 超效率 >1；无效 DMU 保持原值
    const auto scores = scores_of(run_on_toy("ccr", Orientation::INPUT, Rts::CRS, true));
    expect_near(scores[0], 0.5);
    expect_near(scores[2], 8.0 / 7.0); // deaR model_supereff 同值
    EXPECT_GT(scores[2], 1.0);
}

TEST(RadialFamily, FR_RAD_005_SuperEfficiencyVrsInfeasibleMarked)
{
    // VRS 超效率：D 剔除后无技术可产出 7 → 不可行标注（deaR NA 同判）
    const ModelResult result = run_on_toy("ccr", Orientation::INPUT, Rts::VRS, true);
    EXPECT_EQ(result.dmu_results[3].status, DmuStatus::INFEASIBLE);
    EXPECT_FALSE(result.dmu_results[3].has_score());
    // 单 DMU 数据集：剔除自身后参考集为空 → 不可行
    const Dataset data = test_fixtures::make_single_dmu();
    SolveOptions options;
    options.orientation = Orientation::INPUT;
    options.rts = Rts::VRS;
    RunConfig config;
    config.super_efficiency = true;
    config.version = "test";
    const ModelResult single = opensuperdea::registry::run_model("ccr", data, options, config);
    EXPECT_EQ(single.dmu_results[0].status, DmuStatus::INFEASIBLE);
}

TEST(RadialFamily, FR_RAD_004_ScaleEfficiencyScoreIsTeWithAnnotations)
{
    const ModelResult result = run_on_toy("scale_efficiency", Orientation::INPUT, Rts::CRS);
    const auto &d = result.dmu_results[3]; // D：TE=14/15, PTE=1 → SE=14/15
    expect_near(d.score, 14.0 / 15.0);
    expect_near(std::stod(d.annotations.at("se")), 14.0 / 15.0);
}
