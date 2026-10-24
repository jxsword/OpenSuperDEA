/**
 * @file golden_m2_test.cpp
 * @brief L2 黄金算例执行（FR-TST-003/FR-IO-008；expected.csv 与 CLI 结果零转换比对）。
 * @details 用例经 run_cli 全链路（数据 → 求解 → 导出）后与 expected.csv 按
 *          dmu/score/status 对齐，容差 = L2 标准（绝对 1e-6，04 册 §3）。
 */
#include "cli/cli_app.h"

#include <gtest/gtest.h>

#include <cmath>
#include <filesystem>
#include <fstream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

namespace
{
struct GoldenExpectation
{
    std::map<std::string, std::pair<double, std::string>> rows; // dmu → (score, status)
};

/** @brief 读 expected.csv（dmu,score[,status,...]，取前两列）。 */
GoldenExpectation read_expected(const std::string &path)
{
    std::ifstream in(path);
    std::string line;
    std::getline(in, line); // 表头
    GoldenExpectation expectation;
    while (std::getline(in, line))
    {
        std::istringstream stream(line);
        std::string dmu;
        std::string score_text;
        std::getline(stream, dmu, ',');
        std::getline(stream, score_text, ',');
        expectation.rows[dmu] = {std::stod(score_text), ""};
    }
    return expectation;
}

/** @brief 读产品结果 CSV（dmu,score,status）。 */
GoldenExpectation read_product(const std::string &path)
{
    std::ifstream in(path);
    std::string line;
    std::getline(in, line);
    GoldenExpectation product;
    while (std::getline(in, line))
    {
        std::istringstream stream(line);
        std::string dmu;
        std::string score_text;
        std::string status;
        std::getline(stream, dmu, ',');
        std::getline(stream, score_text, ',');
        std::getline(stream, status, ',');
        product.rows[dmu] = {score_text.empty() ? std::numeric_limits<double>::quiet_NaN()
                                                : std::stod(score_text),
                             status};
    }
    return product;
}

int run_cli(const std::vector<std::string> &arguments)
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

std::string golden_dir()
{
    return std::string(OPENSUPERDEA_SOURCE_DIR) + "/tests/golden/";
}
} // namespace

TEST(GoldenM2, L2_FR_TST_003_CcrClassicCase)
{
    const std::string case_dir = golden_dir() + "ccr_classic";
    const std::string outdir = testing::TempDir() + "golden_ccr";
    std::filesystem::remove_all(outdir);
    ASSERT_EQ(run_cli({"run", "--model", "ccr", "--data", case_dir + "/data.csv", "-o", outdir}),
              0);
    const GoldenExpectation expected = read_expected(case_dir + "/expected.csv");
    const GoldenExpectation product = read_product(outdir + "/ccr_in_crs_result.csv");
    ASSERT_EQ(expected.rows.size(), product.rows.size());
    for (const auto &[dmu, value_and_status] : expected.rows)
    {
        const auto found = product.rows.find(dmu);
        ASSERT_NE(found, product.rows.end()) << dmu;
        EXPECT_NEAR(found->second.first, value_and_status.first, 1e-6) << dmu;
    }
}

TEST(GoldenM2, L2_FR_TST_003_Fare1994MalmquistCase)
{
    const std::string case_dir = golden_dir() + "fare1994_panel";
    const std::string outdir = testing::TempDir() + "golden_malm";
    std::filesystem::remove_all(outdir);
    ASSERT_EQ(run_cli({"run", "--panel", "--model", "malmquist", "--data", case_dir + "/data.csv",
                       "-o", outdir}),
              0);
    const GoldenExpectation expected = read_expected(case_dir + "/expected.csv");
    const GoldenExpectation product = read_product(outdir + "/malmquist_in_crs_result.csv");
    ASSERT_EQ(expected.rows.size(), product.rows.size());
    for (const auto &[dmu, value_and_status] : expected.rows)
    {
        const auto found = product.rows.find(dmu);
        ASSERT_NE(found, product.rows.end()) << dmu;
        EXPECT_NEAR(found->second.first, value_and_status.first, 1e-6) << dmu;
    }
}
