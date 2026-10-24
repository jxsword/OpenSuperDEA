/**
 * @file result_exporter_test.cpp
 * @brief 结果导出测试（02 册 §4：CSV 列集/10 位有效数字/负零归一 + JSON schema）。
 */
#include "core/registry/model_runner.h"
#include "core/result/result_exporter.h"

#include <gtest/gtest.h>

#include <fstream>
#include <sstream>
#include <string>

#include "dataset_fixtures.h"

namespace
{
using opensuperdea::dataset::Dataset;
using opensuperdea::registry::Orientation;
using opensuperdea::registry::ResultColumnSpec;
using opensuperdea::registry::Rts;
using opensuperdea::registry::run_model;
using opensuperdea::registry::RunConfig;
using opensuperdea::registry::SolveOptions;
using opensuperdea::result::ModelResult;

/** @brief 在临时目录运行 ccr 并导出，返回文件内容与路径。 */
struct ExportFixture
{
    ModelResult model_result;
    std::string csv_text;
    std::string json_text;
    std::string csv_path;
    std::string json_path;
};

ExportFixture export_toy(Orientation orientation, Rts rts, bool super = false)
{
    ExportFixture fixture;
    const Dataset data = test_fixtures::make_toy_cross();
    SolveOptions options;
    options.orientation = orientation;
    options.rts = rts;
    RunConfig config;
    config.super_efficiency = super;
    config.version = "0.0.0-test";
    config.data_sha1 = "deadbeef";
    fixture.model_result = run_model("ccr", data, options, config);

    const ResultColumnSpec spec = [] {
        ResultColumnSpec s;
        s.include_projections = true;
        s.include_slacks = true;
        s.include_lambdas = true;
        return s;
    }();

    const std::string dir = testing::TempDir();
    fixture.csv_path = dir + "export_test_result.csv";
    fixture.json_path = dir + "export_test_result.json";
    opensuperdea::result::write_result_csv(fixture.csv_path, fixture.model_result, data, spec);
    opensuperdea::result::write_result_json(fixture.json_path, fixture.model_result, data, spec);

    std::ifstream csv_in(fixture.csv_path);
    std::stringstream csv_buffer;
    csv_buffer << csv_in.rdbuf();
    fixture.csv_text = csv_buffer.str();
    std::ifstream json_in(fixture.json_path);
    std::stringstream json_buffer;
    json_buffer << json_in.rdbuf();
    fixture.json_text = json_buffer.str();
    return fixture;
}
} // namespace

TEST(ResultExporter, FR_OUT_001_CsvColumnLayoutMatchesContract)
{
    const ExportFixture fixture = export_toy(Orientation::INPUT, Rts::CRS);
    // 公共列 dmu,score,status + projection_* → slack_* → lambda_*（02 册 §4.1 分块）
    EXPECT_NE(fixture.csv_text.find("dmu,score,status,"), std::string::npos);
    EXPECT_NE(fixture.csv_text.find("projection_in_in_labor"), std::string::npos);
    EXPECT_NE(fixture.csv_text.find("slack_out_out_revenue"), std::string::npos);
    EXPECT_NE(fixture.csv_text.find("lambda_C"), std::string::npos);
    // 10 位有效数字（14/15）
    EXPECT_NE(fixture.csv_text.find("0.9333333333"), std::string::npos);
    // 负零归一（不允许 "-0" 单元格）
    EXPECT_EQ(fixture.csv_text.find(",-0,"), std::string::npos);
    EXPECT_EQ(fixture.csv_text.find(",-0\n"), std::string::npos);
}

TEST(ResultExporter, FR_OUT_002_JsonMetaAndResultsSchema)
{
    const ExportFixture fixture = export_toy(Orientation::INPUT, Rts::CRS);
    EXPECT_NE(fixture.json_text.find("\"model\": \"ccr\""), std::string::npos);
    EXPECT_NE(fixture.json_text.find("\"orientation\": \"in\""), std::string::npos);
    EXPECT_NE(fixture.json_text.find("\"rts\": \"crs\""), std::string::npos);
    EXPECT_NE(fixture.json_text.find("\"data_sha1\": \"deadbeef\""), std::string::npos);
    EXPECT_NE(fixture.json_text.find("\"timestamp\""), std::string::npos);
    EXPECT_NE(fixture.json_text.find("\"lambdas\": {\"C\": 0.1666666667}"), std::string::npos);
    EXPECT_NE(fixture.json_text.find("\"summary\""), std::string::npos);
    EXPECT_NE(fixture.json_text.find("\"rank\""), std::string::npos); // FR-OUT-006 排名
}

TEST(ResultExporter, FR_RAD_005_InfeasibleDmuExportsNullAndEmptyCells)
{
    // VRS 超效率 D 不可行：CSV score 空单元格（行宽与表头一致）、JSON score null
    const ExportFixture fixture = export_toy(Orientation::INPUT, Rts::VRS, true);
    EXPECT_NE(fixture.json_text.find("\"score\": null"), std::string::npos);
    EXPECT_NE(fixture.csv_text.find("D,,infeasible,"), std::string::npos);
}

TEST(ResultExporter, FR_OUT_001_ResultFilenameContract)
{
    opensuperdea::result::ResultMeta meta;
    meta.model_id = "ccr";
    meta.orientation = Orientation::INPUT;
    meta.rts = Rts::VRS;
    EXPECT_EQ(opensuperdea::result::result_filename(meta, true, "csv"), "ccr_in_vrs_result.csv");
    // 不消费 RTS 的模型省略 rts 段（scale_efficiency）
    EXPECT_EQ(opensuperdea::result::result_filename(meta, false, "json"), "ccr_in_result.json");
}
