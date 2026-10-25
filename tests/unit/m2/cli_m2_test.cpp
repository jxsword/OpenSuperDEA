/**
 * @file cli_m2_test.cpp
 * @brief CLI run 全链路测试（FR-CLI-003/008：注册表接线、导出落盘、退出码）。
 */
#include "cli/cli_app.h"

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

namespace
{
struct CliOutcome
{
    int code = 0;
    std::string out;
    std::string err;
};

/** @brief 以参数列表执行 run_cli 并捕获双流。 */
CliOutcome run_with(const std::vector<std::string> &arguments)
{
    std::vector<char *> argv;
    argv.push_back(const_cast<char *>("opensuperdea"));
    for (const auto &argument : arguments)
    {
        argv.push_back(const_cast<char *>(argument.c_str()));
    }
    std::ostringstream out;
    std::ostringstream err;
    CliOutcome outcome;
    outcome.code = opensuperdea::cli::run_cli(static_cast<int>(argv.size()), argv.data(), out, err);
    outcome.out = out.str();
    outcome.err = err.str();
    return outcome;
}

/** @brief golden 算例路径（CMake 注入仓库根）。 */
std::string golden_path(const std::string &relative)
{
    return std::string(OPENSUPERDEA_SOURCE_DIR) + "/tests/golden/" + relative;
}

/** @brief 每用例独立的临时输出目录（testing::TempDir() + 用例名由框架隔离）。 */
class CliM2 : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        outdir = testing::TempDir() + "cli_m2_out";
        std::filesystem::remove_all(outdir);
    }
    void TearDown() override
    {
        std::filesystem::remove_all(outdir);
    }
    std::string outdir;
};
} // namespace

TEST_F(CliM2, FR_CLI_003_RunCcrCsvWritesResultFileAndSummary)
{
    const CliOutcome outcome = run_with(
        {"run", "--model", "ccr", "--data", golden_path("toy_cross/data.csv"), "-o", outdir});
    ASSERT_EQ(outcome.code, 0) << outcome.err;
    EXPECT_NE(outcome.out.find("A 0.5 inefficient"), std::string::npos);
    EXPECT_NE(outcome.out.find("C 1 efficient"), std::string::npos);
    EXPECT_NE(outcome.out.find("summary: mean="), std::string::npos);
    const std::string result_file = outdir + "/ccr_in_crs_result.csv";
    ASSERT_TRUE(std::filesystem::exists(result_file));
    std::ifstream in(result_file);
    std::stringstream buffer;
    buffer << in.rdbuf();
    EXPECT_NE(buffer.str().find("dmu,score,status"), std::string::npos);
}

TEST_F(CliM2, FR_CLI_003_RunJsonFormatWritesJsonFile)
{
    const CliOutcome outcome =
        run_with({"run", "--model", "ccr", "--rts", "vrs", "--orientation", "out", "--format",
                  "json", "--data", golden_path("toy_cross/data.csv"), "-o", outdir});
    ASSERT_EQ(outcome.code, 0) << outcome.err;
    ASSERT_TRUE(std::filesystem::exists(outdir + "/ccr_out_vrs_result.json"));
}

TEST_F(CliM2, FR_CLI_003_RunSuperEfficiencyReportsInfeasibleForD)
{
    const CliOutcome outcome =
        run_with({"run", "--model", "ccr", "--rts", "vrs", "--super", "--data",
                  golden_path("toy_cross/data.csv"), "-o", outdir});
    ASSERT_EQ(outcome.code, 0) << outcome.err;
    EXPECT_NE(outcome.out.find("D - infeasible"), std::string::npos);
}

TEST_F(CliM2, FR_CLI_008_UnknownModelListsAvailableIds)
{
    const CliOutcome outcome = run_with(
        {"run", "--model", "bogus", "--data", golden_path("toy_cross/data.csv"), "-o", outdir});
    EXPECT_EQ(outcome.code, 1);
    EXPECT_NE(outcome.err.find("可用模型"), std::string::npos);
    // 清单含全部内建模型（std::map 有序，不假设排列）
    EXPECT_NE(outcome.err.find("ccr"), std::string::npos);
    EXPECT_NE(outcome.err.find("bcc"), std::string::npos);
    EXPECT_NE(outcome.err.find("multiplier"), std::string::npos);
    EXPECT_NE(outcome.err.find("scale_efficiency"), std::string::npos);
}

TEST_F(CliM2, FR_CLI_008_InvalidOrientationAndRtsRejected)
{
    const std::string data = golden_path("toy_cross/data.csv");
    EXPECT_EQ(run_with({"run", "--model", "ccr", "--orientation", "sideways", "--data", data, "-o",
                        outdir})
                  .code,
              1);
    EXPECT_EQ(
        run_with({"run", "--model", "ccr", "--rts", "weird", "--data", data, "-o", outdir}).code,
        1);
    // bcc 严格 VRS：crs 组合非法（能力矩阵拒绝）
    EXPECT_EQ(
        run_with({"run", "--model", "bcc", "--rts", "crs", "--data", data, "-o", outdir}).code, 1);
}

TEST_F(CliM2, FR_CLI_008_ScaleEfficiencyRejectsExplicitRts)
{
    const CliOutcome outcome =
        run_with({"run", "--model", "scale_efficiency", "--rts", "vrs", "--data",
                  golden_path("toy_cross/data.csv"), "-o", outdir});
    EXPECT_EQ(outcome.code, 1);
    EXPECT_NE(outcome.err.find("不使用 --rts"), std::string::npos);
}

TEST_F(CliM2, FR_CLI_003_RunScaleEfficiencyCsvHasAnnotationColumns)
{
    const CliOutcome outcome = run_with({"run", "--model", "scale_efficiency", "--data",
                                         golden_path("toy_cross/data.csv"), "-o", outdir});
    ASSERT_EQ(outcome.code, 0) << outcome.err;
    const std::string result_file = outdir + "/scale_efficiency_in_result.csv";
    ASSERT_TRUE(std::filesystem::exists(result_file)); // 文件名无 rts 段
    std::ifstream in(result_file);
    std::stringstream buffer;
    buffer << in.rdbuf();
    EXPECT_NE(buffer.str().find(",te,pte,se,rts_region"), std::string::npos);
    EXPECT_NE(buffer.str().find("crs"), std::string::npos); // C 规模有效
}

TEST_F(CliM2, FR_CLI_007_RunOnInvalidDataExitsDataCode)
{
    const CliOutcome outcome =
        run_with({"run", "--model", "ccr", "--data", "no_such_file.csv", "-o", outdir});
    EXPECT_EQ(outcome.code, 2);
}

TEST_F(CliM2, FR_CLI_003_RunPanelWithoutWindowOrMalmquistRejected)
{
    // 分支 3 起 --panel 支持 malmquist 与 --window；普通截面模型 + --panel 仍拒绝
    const CliOutcome outcome = run_with({"run", "--model", "ccr", "--panel", "--data",
                                         golden_path("toy_panel/data.csv"), "-o", outdir});
    EXPECT_EQ(outcome.code, 1);
    EXPECT_NE(outcome.err.find("面板"), std::string::npos);
}
