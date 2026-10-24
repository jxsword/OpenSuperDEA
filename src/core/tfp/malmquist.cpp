/**
 * @file malmquist.cpp
 * @brief 相邻 Malmquist 实现：距离函数内核（4 LP/DMU 对）与 MI/EC/TC 组装（06 册 §10）。
 */
#include "malmquist.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <iomanip>
#include <sstream>
#include <utility>

#include "adapters/highs_adapter.h"
#include "core/registry/engine_error.h"
#include "core/solver/lp_problem.h"

namespace opensuperdea::tfp
{
namespace
{
using opensuperdea::registry::EngineException;
using opensuperdea::registry::Rts;
using opensuperdea::registry::RunConfig;
using opensuperdea::registry::SolveOptions;
using opensuperdea::solver::LpProblem;
using opensuperdea::solver::NEGATIVE_INFINITY;
using opensuperdea::solver::POSITIVE_INFINITY;
using opensuperdea::solver::SolveOutcome;
using opensuperdea::solver::SolveStatus;

/** @brief size_t → Eigen 索引。 */
Eigen::Index as_index(std::size_t value)
{
    return static_cast<Eigen::Index>(value);
}

/** @brief 标注数值文本：17 位有效数字（max_digits10，往返无损，保 PR-05 恒等式）。 */
std::string format_annotation(double value)
{
    std::ostringstream stream;
    stream << std::setprecision(17) << value;
    return std::move(stream).str();
}

/**
 * @brief 距离函数 d^s(x0,y0)：对期 s 技术的产出导向径向包络（φ ≥ 1 前沿内；<1 在前沿外）。
 * @return φ*；VRS 跨期不可行时为 quiet NaN（调用方按 02 册 §2.3 跳过该 DMU 对）
 */
double distance_value(const Eigen::MatrixXd &tech_x, const Eigen::MatrixXd &tech_y,
                      const Eigen::RowVectorXd &x0, const Eigen::RowVectorXd &y0, Rts rts)
{
    const auto k = static_cast<std::size_t>(tech_x.rows());
    if (k == 0)
    {
        return std::numeric_limits<double>::quiet_NaN();
    }
    // max φ s.t. Xλ ≤ x0; Yλ ≥ φ·y0; (+Σλ=1 VRS)
    LpProblem problem;
    problem.variable_count = 1 + k;
    problem.is_minimization = false;
    problem.objective.assign(problem.variable_count, 0.0);
    problem.objective[0] = 1.0;
    for (std::size_t i = 0; std::cmp_less(i, x0.size()); ++i)
    {
        const std::size_t row = problem.constraint_count++;
        problem.row_lower_bounds.push_back(NEGATIVE_INFINITY);
        problem.row_upper_bounds.push_back(x0(as_index(i)));
        for (std::size_t j = 0; j < k; ++j)
        {
            problem.matrix.push_back(
                {.row = row, .column = 1 + j, .value = tech_x(as_index(j), as_index(i))});
        }
    }
    for (std::size_t r = 0; std::cmp_less(r, y0.size()); ++r)
    {
        const std::size_t row = problem.constraint_count++;
        problem.row_lower_bounds.push_back(0.0);
        problem.row_upper_bounds.push_back(POSITIVE_INFINITY);
        for (std::size_t j = 0; j < k; ++j)
        {
            problem.matrix.push_back(
                {.row = row, .column = 1 + j, .value = tech_y(as_index(j), as_index(r))});
        }
        problem.matrix.push_back({.row = row, .column = 0, .value = -y0(as_index(r))});
    }
    if (rts == Rts::VRS)
    {
        const std::size_t row = problem.constraint_count++;
        problem.row_lower_bounds.push_back(1.0);
        problem.row_upper_bounds.push_back(1.0);
        for (std::size_t j = 0; j < k; ++j)
        {
            problem.matrix.push_back({.row = row, .column = 1 + j, .value = 1.0});
        }
    }
    problem.column_lower_bounds = {NEGATIVE_INFINITY};
    problem.column_lower_bounds.resize(problem.variable_count, 0.0);
    problem.column_upper_bounds.assign(problem.variable_count, POSITIVE_INFINITY);

    const SolveOutcome outcome = opensuperdea::adapters::solve_lp(problem);
    if (outcome.status == SolveStatus::INFEASIBLE)
    {
        return std::numeric_limits<double>::quiet_NaN();
    }
    if (outcome.status != SolveStatus::OPTIMAL)
    {
        throw EngineException(3, "solve", "Malmquist 距离函数 LP 求解失败");
    }
    const double phi = outcome.objective_value;
    if (phi <= 1e-12)
    {
        throw EngineException(3, "solve", "距离函数 φ* 非正（数据退化）");
    }
    return phi;
}
} // namespace

result::ModelResult run_malmquist(const dataset::PanelData &panel, const SolveOptions &options,
                                  const RunConfig &config)
{
    if (panel.periods.size() < 2)
    {
        throw EngineException(1, "args", "Malmquist 需要至少 2 期面板数据");
    }
    const auto start_time = std::chrono::steady_clock::now();
    result::ModelResult out;
    out.meta.model_id = "malmquist";
    out.meta.orientation = options.orientation;
    out.meta.rts = options.rts;
    out.meta.version = config.version;
    out.meta.data_sha1 = config.data_sha1;
    out.meta.input_count = panel.sections.begin()->second.input_count();
    out.meta.output_count = panel.sections.begin()->second.output_count();

    std::vector<std::string> row_names;

    for (std::size_t t = 0; t + 1 < panel.periods.size(); ++t)
    {
        const std::string &period_t = panel.periods[t];
        const std::string &period_t1 = panel.periods[t + 1];
        const dataset::Dataset &data_t = panel.sections.at(period_t);
        const dataset::Dataset &data_t1 = panel.sections.at(period_t1);

        // 相邻期 DMU 交集（不平衡面板：缺失即跳过并记录，02 册 §2.3）
        std::vector<std::string> common;
        for (const auto &name : data_t.dmu_names)
        {
            if (std::ranges::find(data_t1.dmu_names, name) != data_t1.dmu_names.end())
            {
                common.push_back(name);
            }
        }
        const std::size_t skipped = data_t.dmu_names.size() - common.size();
        if (skipped > 0)
        {
            std::string warning = "期对 ";
            warning += period_t;
            warning += '-';
            warning += period_t1;
            warning += "：跳过 ";
            warning += std::to_string(skipped);
            warning += " 个仅单期存在的 DMU";
            out.meta.warnings.push_back(std::move(warning));
        }

        for (const auto &name : common)
        {
            const auto index_t = static_cast<Eigen::Index>(
                std::ranges::find(data_t.dmu_names, name) - data_t.dmu_names.begin());
            const auto index_t1 = static_cast<Eigen::Index>(
                std::ranges::find(data_t1.dmu_names, name) - data_t1.dmu_names.begin());
            const Eigen::RowVectorXd x_t = data_t.inputs.row(index_t);
            const Eigen::RowVectorXd y_t = data_t.outputs.row(index_t);
            const Eigen::RowVectorXd x_t1 = data_t1.inputs.row(index_t1);
            const Eigen::RowVectorXd y_t1 = data_t1.outputs.row(index_t1);

            const double d_t_t =
                distance_value(data_t.inputs, data_t.outputs, x_t, y_t, options.rts);
            const double d_t_t1 =
                distance_value(data_t.inputs, data_t.outputs, x_t1, y_t1, options.rts);
            const double d_t1_t =
                distance_value(data_t1.inputs, data_t1.outputs, x_t, y_t, options.rts);
            const double d_t1_t1 =
                distance_value(data_t1.inputs, data_t1.outputs, x_t1, y_t1, options.rts);
            if (std::isnan(d_t_t) || std::isnan(d_t_t1) || std::isnan(d_t1_t) ||
                std::isnan(d_t1_t1))
            {
                // VRS 跨期不可行：跳过该 DMU 对并记录（02 册 §2.3 同规则）
                std::string warning = "期对 ";
                warning += period_t;
                warning += '-';
                warning += period_t1;
                warning += "：DMU ";
                warning += name;
                warning += " 距离函数不可行（VRS 跨期），已跳过";
                out.meta.warnings.push_back(std::move(warning));
                continue;
            }

            // Färe 1994/productivity 惯例：距离取倒数比（MI > 1 = 进步）
            // EC = D_{t+1}(t+1)/D_t(t)、MI = √[D_t(t)/D_t(t+1) × D_{t+1}(t)/D_{t+1}(t+1)]，
            // 其中 D = 1/φ（Shephard 距离）；TC = MI/EC
            const double ec = d_t_t / d_t1_t1;
            const double mi = std::sqrt((d_t_t / d_t_t1) * (d_t1_t / d_t1_t1));
            const double tc = mi / ec;

            result::DmuResult dmu;
            dmu.dmu_index = out.dmu_results.size();
            dmu.score = mi;
            dmu.status =
                mi >= 1.0 - 1e-9 ? result::DmuStatus::EFFICIENT : result::DmuStatus::INEFFICIENT;
            dmu.annotations.emplace("ec", format_annotation(ec));
            dmu.annotations.emplace("tc", format_annotation(tc));
            out.dmu_results.push_back(std::move(dmu));
            std::string row_name = name;
            row_name += '@';
            row_name += period_t;
            row_name += '-';
            row_name += period_t1;
            row_names.push_back(std::move(row_name));
        }
    }

    if (out.dmu_results.empty())
    {
        throw EngineException(2, "data", "无有效相邻期对（面板交集为空或全部不可行）");
    }

    // 行名注入 annotations（导出层从 unit_name 读取）
    for (std::size_t i = 0; i < out.dmu_results.size(); ++i)
    {
        out.dmu_results[i].annotations.emplace("unit_name", row_names[i]);
    }
    out.meta.dmu_count = out.dmu_results.size();

    // 汇总（MI 降序）
    out.summary.rank.assign(out.dmu_results.size(), 0);
    std::vector<std::pair<double, std::size_t>> ordered;
    double sum = 0.0;
    for (std::size_t i = 0; i < out.dmu_results.size(); ++i)
    {
        sum += out.dmu_results[i].score;
        ordered.emplace_back(out.dmu_results[i].score, i);
    }
    std::ranges::sort(ordered, [](const auto &a, const auto &b) { return a.first > b.first; });
    for (std::size_t pos = 0; pos < ordered.size(); ++pos)
    {
        out.summary.rank[ordered[pos].second] = pos + 1;
    }
    out.summary.mean_score = sum / static_cast<double>(out.dmu_results.size());
    const std::size_t mid = ordered.size() / 2;
    out.summary.median_score = ordered.size() % 2 == 1
                                   ? ordered[mid].first
                                   : (ordered[mid - 1].first + ordered[mid].first) / 2.0;

    const auto elapsed = std::chrono::steady_clock::now() - start_time;
    out.meta.elapsed_ms =
        static_cast<double>(
            std::chrono::duration_cast<std::chrono::microseconds>(elapsed).count()) /
        1000.0;
    return out;
}
} // namespace opensuperdea::tfp
