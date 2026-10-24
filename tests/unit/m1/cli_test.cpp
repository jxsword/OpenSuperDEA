/**
 * @file cli_test.cpp
 * @brief CLI 行为测试（FR-CLI-001/002/003/006/007；经 run_cli 返回退出码断言）。
 */
#include "cli/cli_app.h"

#include <gtest/gtest.h>

#include <sstream>
#include <string>
#include <vector>

namespace
{
/** @brief golden 算例路径（CMake 注入仓库根）。 */
std::string golden_path(const std::string &relative)
{
    return std::string(OPENSUPERDEA_SOURCE_DIR) + "/tests/golden/" + relative;
}

/** @brief 以参数列表执行 run_cli（程序名自动前置），返回退出码。 */
int run_with(const std::vector<std::string> &arguments)
{
    std::vector<char *> argv;
    argv.push_back(const_cast<char *>("opensuperdea"));
    for (const auto &argument : arguments)
    {
        argv.push_back(const_cast<char *>(argument.c_str()));
    }
    std::ostringstream out;
    std::ostringstream err;
    return opensuperdea::cli::run_cli(static_cast<int>(argv.size()), argv.data(), out, err);
}
} // namespace

TEST(CliApp, FR_CLI_001_VersionSubcommandExitsZero)
{
    const int code = run_with({"version"});
    EXPECT_EQ(code, 0);
}

TEST(CliApp, FR_CLI_006_GlobalVersionFlagWorks)
{
    const int code = run_with({"--version"});
    EXPECT_EQ(code, 0);
}

TEST(CliApp, FR_CLI_002_CheckValidDatasetPrintsSummary)
{
    const std::string path = golden_path("toy_cross/data.csv");
    const int code = run_with({"check", "--data", path});
    EXPECT_EQ(code, 0);
}

TEST(CliApp, FR_CLI_002_CheckPanelPrintsPeriodSummary)
{
    const std::string path = golden_path("toy_panel/data.csv");
    const int code = run_with({"check", "--data", path, "--panel"});
    EXPECT_EQ(code, 0);
}

TEST(CliApp, FR_CLI_007_InvalidModelExitsArgCode)
{
    const std::string path = golden_path("toy_cross/data.csv");
    const int code = run_with({"run", "--model", "bogus", "--data", path});
    EXPECT_EQ(code, 1); // EXIT_ARG
}

TEST(CliApp, FR_CLI_007_RunPlaceholderReportsNotImplemented)
{
    const std::string path = golden_path("toy_cross/data.csv");
    const int code = run_with({"run", "--model", "ccr", "--data", path});
    EXPECT_EQ(code, 1);
}

TEST(CliApp, FR_CLI_007_MissingFileExitsDataCode)
{
    const int code = run_with({"check", "--data", "no_such_file.csv"});
    EXPECT_EQ(code, 2); // EXIT_DATA
}
