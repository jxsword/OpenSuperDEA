/**
 * @file property_test.cpp
 * @brief M3 性质断言：PR-09 并行与串行结果逐值一致（04 册 §4，FR-PERF-001 验证手段）。
 * @details 程序内生成构造算例（不依赖外部文件），对代表模型族以 thread_count=1 与
 *          thread_count=4 双跑，断言全部输出字段逐值一致（容差 1e-9，实现目标 bit 级）。
 *          覆盖：注册表主循环（径向/SBM/DDF/价格）+ 交叉效率两段 + 窗口 + Malmquist
 *          的全部并行化路径（07 册 §2.3 四处循环）。
 */
#include <gtest/gtest.h>

#include <cmath>
#include <random>
#include <string>
#include <tuple>
#include <vector>

#include "core/cross/cross_efficiency.h"
#include "core/dataset/dataset.h"
#include "core/registry/model_runner.h"
#include "core/tfp/malmquist.h"
#include "core/window/window_analysis.h"

namespace
{
using opensuperdea::dataset::Dataset;
using opensuperdea::registry::Orientation;
using opensuperdea::registry::RunConfig;
using opensuperdea::registry::SolveOptions;
using opensuperdea::result::ModelResult;

/** @brief PR-09 容差（04 册 §4 口径；实现目标为 bit 级一致）。 */
constexpr double K_PR09_TOLERANCE = 1e-9;

/** @brief PR-09 对比线程数（CI runner 4 核安全，本机 8 核亦覆盖窃取路径）。 */
constexpr unsigned int K_PR09_THREADS = 4;

/** @brief 程序内生成 500 DMU × 3×3 对数正态算例（固定 seed，可复现）。 */
Dataset make_random_dataset(std::size_t dmu_count = 500)
{
    Dataset data;
    data.dmu_names.reserve(dmu_count);
    data.input_names = {"in_x1", "in_x2", "in_x3"};
    data.output_names = {"out_y1", "out_y2", "out_y3"};
    data.inputs.resize(static_cast<Eigen::Index>(dmu_count), 3);
    data.outputs.resize(static_cast<Eigen::Index>(dmu_count), 3);
    std::mt19937 rng(42); // NOLINT(bugprone-random-generator-seed) 固定种子：对拍自身可复现
    std::lognormal_distribution<double> lognormal(0.0, 0.5);
    for (std::size_t d = 0; d < dmu_count; ++d)
    {
        data.dmu_names.push_back("DMU" + std::to_string(d + 1));
        for (int i = 0; i < 3; ++i)
        {
            data.inputs(static_cast<Eigen::Index>(d), i) = lognormal(rng);
            data.outputs(static_cast<Eigen::Index>(d), i) = lognormal(rng);
        }
    }
    return data;
}

RunConfig make_config(unsigned int thread_count)
{
    RunConfig config;
    config.version = "test";
    config.thread_count = thread_count;
    return config;
}

/** @brief 逐字段断言两份运行结果一致（score/状态/松弛/投影/汇总/标注）。 */
void expect_pr09_identical(const ModelResult &serial, const ModelResult &parallel)
{
    ASSERT_EQ(serial.dmu_results.size(), parallel.dmu_results.size());
    for (std::size_t d = 0; d < serial.dmu_results.size(); ++d)
    {
        const auto &a = serial.dmu_results[d];
        const auto &b = parallel.dmu_results[d];
        EXPECT_EQ(a.dmu_index, b.dmu_index) << "DMU#" << d;
        EXPECT_EQ(a.status, b.status) << "DMU#" << d;
        EXPECT_NEAR(a.score, b.score, K_PR09_TOLERANCE) << "DMU#" << d;
        // 松弛/投影按模型填充情况比较（非径向/价格族不填投影 targets，尺寸可分别为空）
        ASSERT_EQ(a.input_slacks.size(), b.input_slacks.size()) << "DMU#" << d;
        ASSERT_EQ(a.input_targets.size(), b.input_targets.size()) << "DMU#" << d;
        for (std::size_t i = 0; i < a.input_slacks.size(); ++i)
        {
            EXPECT_NEAR(a.input_slacks[i], b.input_slacks[i], K_PR09_TOLERANCE) << "DMU#" << d;
        }
        for (std::size_t i = 0; i < a.input_targets.size(); ++i)
        {
            EXPECT_NEAR(a.input_targets[i], b.input_targets[i], K_PR09_TOLERANCE) << "DMU#" << d;
        }
        ASSERT_EQ(a.output_slacks.size(), b.output_slacks.size()) << "DMU#" << d;
        ASSERT_EQ(a.output_targets.size(), b.output_targets.size()) << "DMU#" << d;
        for (std::size_t r = 0; r < a.output_slacks.size(); ++r)
        {
            EXPECT_NEAR(a.output_slacks[r], b.output_slacks[r], K_PR09_TOLERANCE) << "DMU#" << d;
        }
        for (std::size_t r = 0; r < a.output_targets.size(); ++r)
        {
            EXPECT_NEAR(a.output_targets[r], b.output_targets[r], K_PR09_TOLERANCE) << "DMU#" << d;
        }
        EXPECT_EQ(a.annotations, b.annotations) << "DMU#" << d;
    }
    ASSERT_EQ(serial.summary.rank.size(), parallel.summary.rank.size());
    for (std::size_t d = 0; d < serial.summary.rank.size(); ++d)
    {
        EXPECT_EQ(serial.summary.rank[d], parallel.summary.rank[d]) << "rank DMU#" << d;
    }
    EXPECT_NEAR(serial.summary.mean_score, parallel.summary.mean_score, K_PR09_TOLERANCE);
    EXPECT_NEAR(serial.summary.median_score, parallel.summary.median_score, K_PR09_TOLERANCE);
    EXPECT_EQ(serial.summary.efficient_count, parallel.summary.efficient_count);
}
} // namespace

// ---- 注册表主循环并行路径（07 册 §2.3 表第 1 行）----

TEST(PropertyPR09, SerialParallelIdenticalRegistryModels)
{
    const Dataset data = make_random_dataset();
    // 模型 × 合法定向 × 合法 RTS（组合合法性由 m2 套件覆盖，此处只关心并行一致性）
    const std::vector<std::tuple<std::string, Orientation, opensuperdea::registry::Rts>> combos = {
        {"ccr", Orientation::INPUT, opensuperdea::registry::Rts::CRS},
        {"ccr", Orientation::OUTPUT, opensuperdea::registry::Rts::CRS},
        {"bcc", Orientation::INPUT, opensuperdea::registry::Rts::VRS},
        {"sbm", Orientation::INPUT, opensuperdea::registry::Rts::VRS},
        {"multiplier", Orientation::INPUT, opensuperdea::registry::Rts::CRS},
        {"multiplier", Orientation::OUTPUT, opensuperdea::registry::Rts::VRS},
        {"ddf", Orientation::INPUT, opensuperdea::registry::Rts::CRS},
        {"ddf", Orientation::NON_DIRECTIONAL, opensuperdea::registry::Rts::VRS},
        {"fdh", Orientation::INPUT, opensuperdea::registry::Rts::CRS}, // MILP 并行路径
    };
    for (const auto &[model_id, orientation, rts] : combos)
    {
        SolveOptions options;
        options.orientation = orientation;
        options.rts = rts;
        if (model_id == "ddf")
        {
            options.model_options["direction"] = "own";
        }
        const ModelResult serial =
            opensuperdea::registry::run_model(model_id, data, options, make_config(1));
        const ModelResult parallel =
            opensuperdea::registry::run_model(model_id, data, options, make_config(K_PR09_THREADS));
        expect_pr09_identical(serial, parallel);
    }
}

// ---- 交叉效率两段并行路径（stage1/stage2 段间隐式同步）----

TEST(PropertyPR09, SerialParallelIdenticalCrossEfficiency)
{
    const Dataset data = make_random_dataset(60); // 交叉效率 n² 组装，缩小规模控制时长
    for (const auto method : {opensuperdea::cross::CrossMethod::DG_AGGRESSIVE,
                              opensuperdea::cross::CrossMethod::DG_BENEVOLENT})
    {
        opensuperdea::cross::CrossRunOptions options;
        options.rts = opensuperdea::registry::Rts::CRS;
        options.method = method;
        const ModelResult serial =
            opensuperdea::cross::run_cross_efficiency(data, options, make_config(1));
        const ModelResult parallel =
            opensuperdea::cross::run_cross_efficiency(data, options, make_config(K_PR09_THREADS));
        expect_pr09_identical(serial, parallel);
        // 交叉矩阵与行/列均值逐值一致（cross 为 optional，两路径应同有同无）
        if (!serial.cross.has_value() || !parallel.cross.has_value())
        {
            FAIL() << "交叉矩阵缺失";
            return;
        }
        const auto &cross_serial = serial.cross.value();
        const auto &cross_parallel = parallel.cross.value();
        for (std::size_t i = 0; i < cross_serial.values.size(); ++i)
        {
            for (std::size_t j = 0; j < cross_serial.values[i].size(); ++j)
            {
                EXPECT_NEAR(cross_serial.values[i][j], cross_parallel.values[i][j],
                            K_PR09_TOLERANCE);
            }
            EXPECT_NEAR(cross_serial.mean_given[i], cross_parallel.mean_given[i], K_PR09_TOLERANCE);
            EXPECT_NEAR(cross_serial.mean_received[i], cross_parallel.mean_received[i],
                        K_PR09_TOLERANCE);
        }
    }
}

// ---- 窗口分析（窗口内单元并行）与 Malmquist（期内 DMU 对并行）----

TEST(PropertyPR09, SerialParallelIdenticalWindowAndMalmquist)
{
    const Dataset data = make_random_dataset(40);
    opensuperdea::dataset::PanelData panel;
    panel.periods = {"2018", "2019", "2020"};
    panel.sections.emplace("2018", data);
    panel.sections.emplace("2019", data);
    panel.sections.emplace("2020", data);

    SolveOptions options;
    options.orientation = Orientation::INPUT;
    options.rts = opensuperdea::registry::Rts::CRS;
    const ModelResult window_serial =
        opensuperdea::window::run_window_analysis(panel, 2, "ccr", options, make_config(1));
    const ModelResult window_parallel = opensuperdea::window::run_window_analysis(
        panel, 2, "ccr", options, make_config(K_PR09_THREADS));
    expect_pr09_identical(window_serial, window_parallel);

    const ModelResult mi_serial = opensuperdea::tfp::run_malmquist(panel, options, make_config(1));
    const ModelResult mi_parallel =
        opensuperdea::tfp::run_malmquist(panel, options, make_config(K_PR09_THREADS));
    expect_pr09_identical(mi_serial, mi_parallel);
}

// ---- auto 线程数冒烟（不比对耗时，仅验证可运行且结果一致）----

TEST(PropertyPR09, AutoThreadCountSmoke)
{
    const Dataset data = make_random_dataset(80);
    SolveOptions options;
    options.orientation = Orientation::INPUT;
    options.rts = opensuperdea::registry::Rts::VRS;
    const ModelResult serial =
        opensuperdea::registry::run_model("bcc", data, options, make_config(1));
    const ModelResult automatic =
        opensuperdea::registry::run_model("bcc", data, options, make_config(0));
    expect_pr09_identical(serial, automatic);
}
