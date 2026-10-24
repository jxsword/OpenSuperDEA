/**
 * @file window_analysis.cpp
 * @brief 窗口分析实现：滑窗合并截面 + 底层模型逐单元求解（FR-WIN-001）。
 */
#include "window_analysis.h"

#include <algorithm>
#include <chrono>
#include <stdexcept>

#include "core/registry/engine_error.h"
#include "core/registry/model_registry.h"

namespace opensuperdea::window
{
namespace
{
using opensuperdea::registry::EngineException;

/** @brief 合并窗口内各期截面为单一数据集（行名 "dmu@period"；行序 = 期次 × 期内序）。 */
dataset::Dataset merge_window(const dataset::PanelData &panel, std::size_t start, std::size_t width)
{
    dataset::Dataset merged;
    merged.input_names = panel.sections.begin()->second.input_names;
    merged.output_names = panel.sections.begin()->second.output_names;
    const std::size_t m = merged.input_names.size();
    const std::size_t s = merged.output_names.size();
    std::size_t total = 0;
    for (std::size_t p = start; p < start + width; ++p)
    {
        total += panel.sections.at(panel.periods[p]).dmu_count();
    }
    merged.inputs.resize(static_cast<Eigen::Index>(total), static_cast<Eigen::Index>(m));
    merged.outputs.resize(static_cast<Eigen::Index>(total), static_cast<Eigen::Index>(s));
    std::size_t row = 0;
    for (std::size_t p = start; p < start + width; ++p)
    {
        const dataset::Dataset &section = panel.sections.at(panel.periods[p]);
        for (std::size_t d = 0; d < section.dmu_count(); ++d)
        {
            merged.dmu_names.push_back(section.dmu_names[d] + "@" + panel.periods[p]);
            merged.inputs.row(static_cast<Eigen::Index>(row)) =
                section.inputs.row(static_cast<Eigen::Index>(d));
            merged.outputs.row(static_cast<Eigen::Index>(row)) =
                section.outputs.row(static_cast<Eigen::Index>(d));
            ++row;
        }
    }
    return merged;
}
} // namespace

result::ModelResult run_window_analysis(const dataset::PanelData &panel, std::size_t window_width,
                                        const std::string &base_model_id,
                                        const registry::SolveOptions &options,
                                        const registry::RunConfig &config)
{
    if (window_width == 0 || window_width > panel.periods.size())
    {
        throw EngineException(1, "args",
                              "窗口宽度非法（须 1 ≤ w ≤ 期次数 " +
                                  std::to_string(panel.periods.size()) + "）");
    }
    const registry::ModelDescriptor *descriptor =
        registry::ModelRegistry::instance().find(base_model_id);
    if (descriptor == nullptr)
    {
        throw EngineException(1, "args", "未知底层模型 \"" + base_model_id + "\"");
    }

    const auto start_time = std::chrono::steady_clock::now();
    result::ModelResult out;
    out.meta.model_id = "window_" + base_model_id;
    out.meta.orientation = options.orientation;
    out.meta.rts = options.rts;
    out.meta.version = config.version;
    out.meta.data_sha1 = config.data_sha1;
    out.meta.input_count = panel.sections.begin()->second.input_count();
    out.meta.output_count = panel.sections.begin()->second.output_count();

    // 滑窗：起点 0..T-w（首尾期窗口数按定义减少）
    for (std::size_t start = 0; start + window_width <= panel.periods.size(); ++start)
    {
        const dataset::Dataset merged = merge_window(panel, start, window_width);
        for (std::size_t unit = 0; unit < merged.dmu_count(); ++unit)
        {
            registry::SolveContext context;
            context.data = &merged;
            context.dmu_index = unit;
            for (std::size_t j = 0; j < merged.dmu_count(); ++j)
            {
                context.reference_indices.push_back(j); // 窗口内全体单元构成参考集
            }
            context.options = options;
            result::DmuResult dmu = descriptor->solve(context);
            dmu.dmu_index = out.dmu_results.size();
            dmu.annotations.emplace("window_start", panel.periods[start]);
            out.dmu_results.push_back(std::move(dmu));
        }
        // 行名以合并期名记录（导出层用 annotations 重建 "dmu@period"）
    }
    // 行名：dmu_results 顺序与各窗口合并序一致；导出需行名 → 记入 annotations
    {
        std::size_t idx = 0;
        for (std::size_t start = 0; start + window_width <= panel.periods.size(); ++start)
        {
            const dataset::Dataset merged = merge_window(panel, start, window_width);
            for (std::size_t unit = 0; unit < merged.dmu_count(); ++unit)
            {
                out.dmu_results[idx].annotations.emplace("unit_name", merged.dmu_names[unit]);
                ++idx;
            }
        }
    }
    out.meta.dmu_count = out.dmu_results.size();
    out.summary = [&] {
        const registry::RunConfig &unused = config;
        static_cast<void>(unused);
        // 复用 runner 的汇总逻辑成本高于内联：窗口分析直接统计
        result::ResultSummary summary;
        summary.rank.assign(out.dmu_results.size(), 0);
        double sum = 0.0;
        std::size_t efficient = 0;
        std::vector<std::pair<double, std::size_t>> ordered;
        for (std::size_t i = 0; i < out.dmu_results.size(); ++i)
        {
            sum += out.dmu_results[i].score;
            if (out.dmu_results[i].status == result::DmuStatus::EFFICIENT)
            {
                ++efficient;
            }
            ordered.emplace_back(out.dmu_results[i].score, i);
        }
        std::ranges::sort(ordered, [](const auto &a, const auto &b) { return a.first > b.first; });
        for (std::size_t pos = 0; pos < ordered.size(); ++pos)
        {
            summary.rank[ordered[pos].second] = pos + 1;
        }
        if (!ordered.empty())
        {
            summary.mean_score = sum / static_cast<double>(ordered.size());
            const std::size_t mid = ordered.size() / 2;
            summary.median_score = ordered.size() % 2 == 1
                                       ? ordered[mid].first
                                       : (ordered[mid - 1].first + ordered[mid].first) / 2.0;
        }
        summary.efficient_count = efficient;
        return summary;
    }();

    const auto elapsed = std::chrono::steady_clock::now() - start_time;
    out.meta.elapsed_ms =
        static_cast<double>(
            std::chrono::duration_cast<std::chrono::microseconds>(elapsed).count()) /
        1000.0;
    out.meta.warnings.emplace_back("窗口分析结果行 = (dmu, period) 单元；行名见 unit_name 标注");
    return out;
}
} // namespace opensuperdea::window
