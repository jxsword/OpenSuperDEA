/**
 * @file result_exporter.cpp
 * @brief 结果导出实现：CSV/JSON 序列化（02 册 §4.1/§4.2）。
 * @details 列展开顺序：公共列 dmu,score,status → projection_* → slack_* →
 *          lambda_* → mult_* → 模型特有列（分块排列，02 册 §4.1）。
 *          JSON 自包含序列化（无第三方依赖），字符串统一转义。
 */
#include "result_exporter.h"

#include <chrono>
#include <cmath>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <vector>

#include "core/registry/engine_error.h"

namespace opensuperdea::result
{
namespace
{
using opensuperdea::registry::EngineException;

/** @brief 数值文本：10 位有效数字（02 册 §4.1）；非有限值输出空串（不可行不输出数值）。 */
std::string format_number(double value)
{
    if (!std::isfinite(value))
    {
        return {};
    }
    if (value == 0.0)
    {
        return "0"; // 归一负零（LP 求解器可能返回 -0.0）
    }
    std::ostringstream stream;
    stream << std::setprecision(10) << value;
    return std::move(stream).str();
}

/** @brief 数据集投入列名（程序内构造数据回退 in_N）。 */
std::string input_column_name(const ExportNames &names, std::size_t index)
{
    return index < names.input_names.size() ? names.input_names[index]
                                            : "in_" + std::to_string(index + 1);
}

/** @brief 数据集产出列名（程序内构造数据回退 out_N）。 */
std::string output_column_name(const ExportNames &names, std::size_t index)
{
    return index < names.output_names.size() ? names.output_names[index]
                                             : "out_" + std::to_string(index + 1);
}

/** @brief 按列集声明展开全部 CSV 列名（与写值顺序一致）。 */
std::vector<std::string> build_columns(const ExportNames &names,
                                       const registry::ResultColumnSpec &spec)
{
    std::vector<std::string> columns{"dmu", "score", "status"};
    if (spec.include_projections)
    {
        for (std::size_t i = 0; i < names.input_names.size(); ++i)
        {
            columns.push_back("projection_in_" + input_column_name(names, i));
        }
        for (std::size_t r = 0; r < names.output_names.size(); ++r)
        {
            columns.push_back("projection_out_" + output_column_name(names, r));
        }
    }
    if (spec.include_slacks)
    {
        for (std::size_t i = 0; i < names.input_names.size(); ++i)
        {
            columns.push_back("slack_in_" + input_column_name(names, i));
        }
        for (std::size_t r = 0; r < names.output_names.size(); ++r)
        {
            columns.push_back("slack_out_" + output_column_name(names, r));
        }
    }
    if (spec.include_lambdas)
    {
        for (const auto &name : names.row_names)
        {
            columns.push_back("lambda_" + name);
        }
    }
    if (spec.include_multipliers)
    {
        for (std::size_t i = 0; i < names.input_names.size(); ++i)
        {
            columns.push_back("mult_v_" + input_column_name(names, i));
        }
        for (std::size_t r = 0; r < names.output_names.size(); ++r)
        {
            columns.push_back("mult_u_" + output_column_name(names, r));
        }
    }
    columns.insert(columns.end(), spec.extra_columns.begin(), spec.extra_columns.end());
    return columns;
}

/** @brief JSON 字符串转义（含引号、反斜杠与控制字符）。 */
std::string json_escape(std::string_view text)
{
    std::string out;
    out.reserve(text.size() + 2);
    for (const char ch : text)
    {
        switch (ch)
        {
        case '"':
            out += "\\\"";
            break;
        case '\\':
            out += "\\\\";
            break;
        case '\n':
            out += "\\n";
            break;
        case '\r':
            out += "\\r";
            break;
        case '\t':
            out += "\\t";
            break;
        default:
            if (static_cast<unsigned char>(ch) < 0x20U)
            {
                // 其余控制字符按 \u00XX 输出
                static const char *k_hex = "0123456789abcdef";
                const unsigned int code_point = static_cast<unsigned char>(ch);
                out += "\\u00";
                out.push_back(k_hex[(code_point >> 4U) & 0xFU]);
                out.push_back(k_hex[code_point & 0xFU]);
            }
            else
            {
                out.push_back(ch);
            }
            break;
        }
    }
    return out;
}

/** @brief 当前时刻的 UTC ISO-8601 文本（JSON meta.timestamp）。 */
std::string utc_timestamp()
{
    const std::time_t now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    std::tm utc{};
#ifdef _MSC_VER
    gmtime_s(&utc, &now);
#else
    gmtime_r(&now, &utc);
#endif
    std::ostringstream stream;
    stream << std::put_time(&utc, "%Y-%m-%dT%H:%M:%SZ");
    return std::move(stream).str();
}

/** @brief 读取向量第 index 个元素（越界/缺失返回空串——不可行 DMU 输出空单元格）。 */
std::string optional_number(const std::vector<double> &values, std::size_t index)
{
    return index < values.size() ? format_number(values[index]) : std::string{};
}

/** @brief 按展开列写单行 CSV 值（与 build_columns 顺序严格一致；空值保持单元格对位）。 */
std::vector<std::string> build_row_values(const DmuResult &dmu, const ExportNames &names,
                                          const registry::ResultColumnSpec &spec)
{
    std::vector<std::string> values;
    values.push_back(names.row_names[dmu.dmu_index]);
    values.push_back(dmu.has_score() ? format_number(dmu.score) : std::string{});
    values.push_back(to_string(dmu.status));

    if (spec.include_projections)
    {
        for (std::size_t i = 0; i < names.input_names.size(); ++i)
        {
            values.push_back(optional_number(dmu.input_targets, i));
        }
        for (std::size_t r = 0; r < names.output_names.size(); ++r)
        {
            values.push_back(optional_number(dmu.output_targets, r));
        }
    }
    if (spec.include_slacks)
    {
        for (std::size_t i = 0; i < names.input_names.size(); ++i)
        {
            values.push_back(optional_number(dmu.input_slacks, i));
        }
        for (std::size_t r = 0; r < names.output_names.size(); ++r)
        {
            values.push_back(optional_number(dmu.output_slacks, r));
        }
    }
    if (spec.include_lambdas)
    {
        // 稀疏 λ 展开为全 DMU 列（02 册 §4.1 lambda_*；零值留空）
        std::vector<double> lambda_by_dmu(names.row_names.size(), 0.0);
        for (const auto &[index, weight] : dmu.lambdas)
        {
            lambda_by_dmu[index] = weight;
        }
        for (const double weight : lambda_by_dmu)
        {
            values.push_back(weight != 0.0 ? format_number(weight) : std::string{});
        }
    }
    if (spec.include_multipliers)
    {
        for (std::size_t i = 0; i < names.input_names.size(); ++i)
        {
            values.push_back(optional_number(dmu.input_multipliers, i));
        }
        for (std::size_t r = 0; r < names.output_names.size(); ++r)
        {
            values.push_back(optional_number(dmu.output_multipliers, r));
        }
    }
    for (const auto &column : spec.extra_columns)
    {
        const auto found = dmu.annotations.find(column);
        values.push_back(found != dmu.annotations.end() ? found->second : std::string{});
    }
    return values;
}

/** @brief 打开输出文件（失败抛 EXIT_INTERNAL）。 */
std::ofstream open_output(const std::string &path)
{
    std::ofstream stream(path, std::ios::binary | std::ios::trunc);
    if (!stream)
    {
        throw EngineException(4, "internal", "结果文件不可写: " + path, "检查目录权限");
    }
    return stream;
}
} // namespace

std::string result_filename(const ResultMeta &meta, bool uses_rts, std::string_view extension)
{
    std::ostringstream stream;
    stream << meta.model_id << '_' << registry::to_string(meta.orientation);
    if (uses_rts)
    {
        stream << '_' << registry::to_string(meta.rts);
    }
    stream << "_result." << extension;
    return std::move(stream).str();
}

void write_result_csv(const std::string &path, const ModelResult &result, const ExportNames &names,
                      const registry::ResultColumnSpec &spec)
{
    std::ofstream stream = open_output(path);
    const std::vector<std::string> columns = build_columns(names, spec);
    for (std::size_t c = 0; c < columns.size(); ++c)
    {
        if (c != 0)
        {
            stream << ',';
        }
        stream << columns[c];
    }
    stream << '\n';

    for (const DmuResult &dmu : result.dmu_results)
    {
        const std::vector<std::string> values = build_row_values(dmu, names, spec);
        for (std::size_t v = 0; v < values.size(); ++v)
        {
            if (v != 0)
            {
                stream << ',';
            }
            stream << values[v];
        }
        stream << '\n';
    }
    if (!stream)
    {
        throw EngineException(4, "internal", "结果文件写入失败: " + path);
    }
}

void write_result_json(const std::string &path, const ModelResult &result, const ExportNames &names,
                       const registry::ResultColumnSpec &spec)
{
    std::ofstream stream = open_output(path);
    std::ostringstream body;
    body << std::setprecision(10);

    body << "{\n  \"meta\": {";
    body << R"("model": ")" << json_escape(result.meta.model_id) << '"';
    body << R"(, "orientation": ")" << registry::to_string(result.meta.orientation) << '"';
    body << R"(, "rts": ")" << registry::to_string(result.meta.rts) << '"';
    body << ", \"super\": " << (result.meta.super_efficiency ? "true" : "false");
    body << R"(, "version": ")" << json_escape(result.meta.version) << '"';
    body << R"(, "timestamp": ")" << utc_timestamp() << '"';
    body << ", \"elapsed_ms\": " << result.meta.elapsed_ms;
    body << R"(, "data_sha1": ")" << json_escape(result.meta.data_sha1) << '"';
    body << "},\n";

    if (result.cross.has_value())
    {
        // 交叉效率矩阵（FR-CROSS-004）：评者×被评者 + 行/列均值
        const CrossMatrix &cross = *result.cross;
        body << "  \"cross_matrix\": {\n";
        body << "    \"values\": [\n";
        for (std::size_t i = 0; i < cross.values.size(); ++i)
        {
            body << "      [";
            for (std::size_t j = 0; j < cross.values[i].size(); ++j)
            {
                if (j != 0)
                {
                    body << ", ";
                }
                body << format_number(cross.values[i][j]);
            }
            body << ']' << (i + 1 != cross.values.size() ? "," : "") << '\n';
        }
        body << "    ],\n";
        body << "    \"mean_given\": [";
        for (std::size_t i = 0; i < cross.mean_given.size(); ++i)
        {
            if (i != 0)
            {
                body << ", ";
            }
            body << format_number(cross.mean_given[i]);
        }
        body << "],\n";
        body << "    \"mean_received\": [";
        for (std::size_t j = 0; j < cross.mean_received.size(); ++j)
        {
            if (j != 0)
            {
                body << ", ";
            }
            body << format_number(cross.mean_received[j]);
        }
        body << "]\n  },\n";
    }

    body << "  \"results\": [\n";
    for (std::size_t d = 0; d < result.dmu_results.size(); ++d)
    {
        const DmuResult &dmu = result.dmu_results[d];
        body << R"(    {"dmu": ")" << json_escape(names.row_names[dmu.dmu_index]) << '"';
        body << ", \"score\": ";
        body << (dmu.has_score() ? format_number(dmu.score) : std::string("null"));
        body << ", \"rank\": " << result.summary.rank[d];
        body << R"(, "status": ")" << to_string(dmu.status) << '"';
        if (spec.include_projections && (!dmu.input_targets.empty() || !dmu.output_targets.empty()))
        {
            body << ", \"projection\": {";
            bool first = true;
            for (std::size_t i = 0; i < dmu.input_targets.size(); ++i)
            {
                if (!first)
                {
                    body << ", ";
                }
                first = false;
                body << '"' << json_escape(input_column_name(names, i))
                     << "\": " << format_number(dmu.input_targets[i]);
            }
            for (std::size_t r = 0; r < dmu.output_targets.size(); ++r)
            {
                if (!first)
                {
                    body << ", ";
                }
                first = false;
                body << '"' << json_escape(output_column_name(names, r))
                     << "\": " << format_number(dmu.output_targets[r]);
            }
            body << '}';
        }
        if (spec.include_slacks && (!dmu.input_slacks.empty() || !dmu.output_slacks.empty()))
        {
            body << ", \"slacks\": {";
            bool first = true;
            for (std::size_t i = 0; i < dmu.input_slacks.size(); ++i)
            {
                if (!first)
                {
                    body << ", ";
                }
                first = false;
                body << '"' << json_escape(input_column_name(names, i))
                     << "\": " << format_number(dmu.input_slacks[i]);
            }
            for (std::size_t r = 0; r < dmu.output_slacks.size(); ++r)
            {
                if (!first)
                {
                    body << ", ";
                }
                first = false;
                body << '"' << json_escape(output_column_name(names, r))
                     << "\": " << format_number(dmu.output_slacks[r]);
            }
            body << '}';
        }
        if (spec.include_lambdas && !dmu.lambdas.empty())
        {
            body << ", \"lambdas\": {";
            for (std::size_t l = 0; l < dmu.lambdas.size(); ++l)
            {
                if (l != 0)
                {
                    body << ", ";
                }
                body << '"' << json_escape(names.row_names[dmu.lambdas[l].first])
                     << "\": " << format_number(dmu.lambdas[l].second);
            }
            body << '}';
        }
        if (spec.include_multipliers &&
            (!dmu.input_multipliers.empty() || !dmu.output_multipliers.empty()))
        {
            body << R"(, "multipliers": {"v": {)";
            for (std::size_t i = 0; i < dmu.input_multipliers.size(); ++i)
            {
                if (i != 0)
                {
                    body << ", ";
                }
                body << '"' << json_escape(input_column_name(names, i))
                     << "\": " << format_number(dmu.input_multipliers[i]);
            }
            body << "}, \"u\": {";
            for (std::size_t r = 0; r < dmu.output_multipliers.size(); ++r)
            {
                if (r != 0)
                {
                    body << ", ";
                }
                body << '"' << json_escape(output_column_name(names, r))
                     << "\": " << format_number(dmu.output_multipliers[r]);
            }
            body << "}}";
        }
        if (!dmu.annotations.empty())
        {
            body << ", \"annotations\": {";
            bool first = true;
            for (const auto &[key, value] : dmu.annotations)
            {
                if (!first)
                {
                    body << ", ";
                }
                first = false;
                body << '"' << json_escape(key) << "\": \"" << json_escape(value) << '"';
            }
            body << '}';
        }
        body << '}';
        if (d + 1 != result.dmu_results.size())
        {
            body << ',';
        }
        body << '\n';
    }
    body << "  ],\n";

    body << R"(  "summary": {"mean": )" << result.summary.mean_score
         << ", \"median\": " << result.summary.median_score
         << ", \"efficient_count\": " << result.summary.efficient_count << "},\n";

    body << "  \"warnings\": [";
    for (std::size_t w = 0; w < result.meta.warnings.size(); ++w)
    {
        if (w != 0)
        {
            body << ", ";
        }
        body << '"' << json_escape(result.meta.warnings[w]) << '"';
    }
    body << "]\n}\n";

    stream << body.str();
    if (!stream)
    {
        throw EngineException(4, "internal", "结果文件写入失败: " + path);
    }
}
} // namespace opensuperdea::result
