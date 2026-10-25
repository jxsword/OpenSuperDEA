/**
 * @file model_runner.cpp
 * @brief 模型运行器实现：逐 DMU 参考集选择-求解-组装与汇总统计（FR-OUT-006）。
 */
#include "model_runner.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <numeric>

#include "model_registry.h"
#include "reference_set.h"
#include "solvable_model.h"

namespace opensuperdea::registry
{
namespace
{
/** @brief 汇总统计：竞争排名（并列同名）+ 均值/中位数/有效计数（FR-OUT-006）。 */
result::ResultSummary summarize(const std::vector<result::DmuResult> &results)
{
    result::ResultSummary summary;
    summary.rank.assign(results.size(), 0); // 0 = 未参与排名（不可行）

    std::vector<std::size_t> order;
    order.reserve(results.size());
    for (std::size_t i = 0; i < results.size(); ++i)
    {
        if (results[i].has_score())
        {
            order.push_back(i);
        }
    }
    std::ranges::sort(order, [&results](std::size_t a, std::size_t b) {
        return results[a].score > results[b].score;
    });

    // 竞争排名：同值同名，名次 = 前方人数 + 1
    for (std::size_t pos = 0; pos < order.size(); ++pos)
    {
        std::size_t rank = pos + 1;
        if (pos > 0 && std::abs(results[order[pos]].score - results[order[pos - 1]].score) <= 1e-12)
        {
            rank = summary.rank[order[pos - 1]];
        }
        summary.rank[order[pos]] = rank;
    }

    std::vector<double> scores;
    scores.reserve(order.size());
    for (const std::size_t i : order)
    {
        scores.push_back(results[i].score);
        if (results[i].status == result::DmuStatus::EFFICIENT ||
            results[i].status == result::DmuStatus::SUPER_EFFICIENT)
        {
            ++summary.efficient_count;
        }
    }
    if (!scores.empty())
    {
        const double sum = std::accumulate(scores.begin(), scores.end(), 0.0);
        summary.mean_score = sum / static_cast<double>(scores.size());
        const std::size_t mid = scores.size() / 2;
        summary.median_score =
            scores.size() % 2 == 1 ? scores[mid] : (scores[mid - 1] + scores[mid]) / 2.0;
    }
    return summary;
}
} // namespace

result::ModelResult run_model(const std::string &model_id, const dataset::Dataset &data,
                              const SolveOptions &options, const RunConfig &config)
{
    const ModelRegistry &registry = ModelRegistry::instance();
    const ModelDescriptor *descriptor = registry.find(model_id);
    if (descriptor == nullptr)
    {
        std::string ids;
        for (const std::string &id : registry.model_ids())
        {
            ids += ids.empty() ? id : "/" + id;
        }
        throw EngineException(1, "args", "未知模型 \"" + model_id + "\"", "可用模型: " + ids);
    }
    std::string reason;
    if (!registry.validate_combo(model_id, options.orientation, options.rts,
                                 config.super_efficiency, &reason))
    {
        throw EngineException(1, "args", reason, "查看模型能力矩阵（--help）");
    }

    const auto start_time = std::chrono::steady_clock::now();
    const ReferenceSetKind kind =
        config.super_efficiency ? ReferenceSetKind::EXCLUDE_SELF : ReferenceSetKind::ALL;
    const ReferenceSetParams params{};

    result::ModelResult out;
    out.meta.model_id = model_id;
    out.meta.orientation = options.orientation;
    out.meta.rts = options.rts;
    out.meta.super_efficiency = config.super_efficiency;
    out.meta.dmu_count = data.dmu_count();
    out.meta.input_count = data.input_count();
    out.meta.output_count = data.output_count();
    out.meta.version = config.version;
    out.meta.data_sha1 = config.data_sha1;

    out.dmu_results.reserve(data.dmu_count());
    for (std::size_t i = 0; i < data.dmu_count(); ++i)
    {
        SolveContext context;
        context.data = &data;
        context.dmu_index = i;
        context.reference_indices = select_reference_set(kind, params, data.dmu_count(), i);
        context.options = options;
        if (config.super_efficiency)
        {
            // 超效率标志经选项传给模型（SBM 族据此切换 Tone 2002 LP；06 册 §4.2）
            context.options.model_options["super"] = "1";
        }
        out.dmu_results.push_back(descriptor->solve(context));
    }
    out.summary = summarize(out.dmu_results);

    const auto elapsed = std::chrono::steady_clock::now() - start_time;
    out.meta.elapsed_ms =
        static_cast<double>(
            std::chrono::duration_cast<std::chrono::microseconds>(elapsed).count()) /
        1000.0;
    return out;
}
} // namespace opensuperdea::registry
