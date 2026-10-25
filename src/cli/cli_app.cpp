/**
 * @file cli_app.cpp
 * @brief CLI 子命令实现：version / check / run（M2 起 run 经模型注册表全量生效）。
 * @details 错误策略（FR-CLI-007/03 册 §3.2）：所有错误走 stderr 且带 [E<code>] 前缀；
 *          数据错误聚合输出（单次报告全部）。run 流程（06 册 §1）：查注册表 →
 *          组合校验（FR-CLI-008）→ 逐 DMU 构造-求解-组装 → 结果导出（02 册 §4）。
 *          回调仅收集意图，parse 后统一执行——避免回调内 std::exit 跳过返回路径。
 */
#include "cli_app.h"

#include <CLI/CLI.hpp>

#include "core/cross/cross_efficiency.h"
#include "core/dataset/csv_parser.h"
#include "core/dataset/dataset_builder.h"
#include "core/dataset/validator.h"
#include "core/registry/engine_error.h"
#include "core/registry/model_descriptor.h"
#include "core/registry/model_registry.h"
#include "core/registry/model_runner.h"
#include "core/registry/solve_options.h"
#include "core/result/result_exporter.h"
#include "core/tfp/malmquist.h"
#include "core/util/sha1.h"
#include "core/window/window_analysis.h"
#include "exit_codes.h"

#include <algorithm>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace opensuperdea::cli
{
namespace
{
constexpr const char *VERSION_TEXT = "0.3.0";
constexpr const char *PROGRAM_NAME = "opensuperdea";

/** @brief 输出一条校验错误（五元组 → 单行，NFR-USAB-001）。 */
void print_validation_error(std::ostream &err, const dataset::ValidationError &error)
{
    err << "[E2] data: " << error.file;
    if (error.line != 0)
    {
        err << ":" << error.line;
    }
    if (!error.column.empty())
    {
        err << ":" << error.column;
    }
    err << " " << error.rule_id << " " << error.message;
    if (!error.hint.empty())
    {
        err << " (hint: " << error.hint << ")";
    }
    err << '\n';
}

/** @brief 数据载入链路：读文件 → 角色识别 → 校验 → 构造（截面或面板，二选一捕获）。 */
int load_and_build(const std::string &data_path, bool as_panel,
                   const std::vector<std::string> &explicit_inputs,
                   const std::vector<std::string> &explicit_outputs, dataset::Dataset *cross_out,
                   dataset::PanelData *panel_out, std::ostream &err)
{
    dataset::CsvParseError parse_error{};
    const auto table = dataset::read_csv_file(data_path, &parse_error);
    if (!parse_error.message.empty())
    {
        err << "[E2] data: " << data_path << ":" << parse_error.line_number << " "
            << parse_error.message << '\n';
        return EXIT_DATA;
    }
    dataset::BuildOptions options;
    options.explicit_inputs = explicit_inputs;
    options.explicit_outputs = explicit_outputs;
    std::vector<dataset::ValidationError> errors;
    const auto roles = dataset::identify_roles(table, options, data_path, errors);
    auto validation_errors = dataset::validate_table(table, roles, data_path, as_panel);
    errors.insert(errors.end(), validation_errors.begin(), validation_errors.end());

    if (as_panel)
    {
        if (panel_out != nullptr)
        {
            *panel_out = dataset::build_panel(table, roles, data_path, errors);
        }
    }
    else if (cross_out != nullptr)
    {
        *cross_out = dataset::build_dataset(table, roles, data_path, errors);
    }
    if (!errors.empty())
    {
        for (const auto &error : errors)
        {
            print_validation_error(err, error);
        }
        return EXIT_DATA;
    }
    return EXIT_OK;
}

/** @brief version 子命令（FR-CLI-001）。 */
int run_version(std::ostream &out)
{
    out << PROGRAM_NAME << " v" << VERSION_TEXT << '\n';
    return EXIT_OK;
}

/** @brief check 子命令（FR-CLI-002：完整校验 + 摘要，不求解）。 */
int run_check(const std::string &data_path, bool as_panel,
              const std::vector<std::string> &explicit_inputs,
              const std::vector<std::string> &explicit_outputs, std::ostream &out,
              std::ostream &err)
{
    dataset::Dataset data;
    dataset::PanelData panel;
    const int load_status =
        load_and_build(data_path, as_panel, explicit_inputs, explicit_outputs,
                       as_panel ? nullptr : &data, as_panel ? &panel : nullptr, err);
    if (load_status != EXIT_OK)
    {
        return load_status;
    }
    if (as_panel)
    {
        out << "panel: " << panel.periods.size() << " 期次, " << panel.dmu_names.size() << " DMU, "
            << panel.sections.begin()->second.input_count() << " 投入, "
            << panel.sections.begin()->second.output_count() << " 产出";
        if (!panel.is_balanced())
        {
            out << "（不平衡面板）";
        }
        out << '\n';
    }
    else
    {
        out << "dataset: " << data.dmu_count() << " DMU, " << data.input_count() << " 投入, "
            << data.output_count() << " 产出" << '\n';
    }
    return EXIT_OK;
}

/** @brief 逗号分隔数值列表解析（虚拟 DMU 向量；数量/数值非法即 EXIT_ARG）。 */
std::vector<double> parse_number_list(const std::string &text, std::size_t expected_size,
                                      std::ostream &err)
{
    std::vector<double> values;
    std::string token;
    std::istringstream stream(text);
    while (std::getline(stream, token, ','))
    {
        try
        {
            std::size_t consumed = 0;
            values.push_back(std::stod(token, &consumed));
            if (consumed != token.size())
            {
                throw std::invalid_argument("tail");
            }
        }
        catch (const std::exception &)
        {
            err << "[E1] args: 非法数值列表 \"" << text << "\"\n";
            return {};
        }
    }
    if (values.size() != expected_size)
    {
        err << "[E1] args: 数值列表元素数 " << values.size() << " 与期望 " << expected_size
            << " 不一致\n";
        return {};
    }
    return values;
}

/** @brief Malmquist 结果列集（unit_name + ec/tc）。 */
registry::ResultColumnSpec malmquist_columns()
{
    registry::ResultColumnSpec spec;
    spec.extra_columns = {"unit_name", "ec", "tc"};
    return spec;
}

/** @brief 窗口分析结果列集（unit_name + window_start）。 */
registry::ResultColumnSpec window_columns()
{
    registry::ResultColumnSpec spec;
    spec.extra_columns = {"unit_name", "window_start"};
    return spec;
}

/** @brief 数据文件 SHA-1 指纹（02 册 §4.2 data_sha1；文件已通过数据载入校验）。 */
std::string file_sha1(const std::string &path)
{
    std::ifstream in(path, std::ios::binary);
    if (!in)
    {
        return {};
    }
    std::ostringstream buffer;
    buffer << in.rdbuf();
    return util::sha1_hex(buffer.str());
}

/** @brief 求解摘要打印（FR-CLI-003：逐 DMU 表 + 汇总 + 文件路径）。 */
void print_run_summary(std::ostream &out, const result::ModelResult &model_result,
                       const std::vector<std::string> &row_names, const std::string &result_path)
{
    const result::ResultMeta &meta = model_result.meta;
    out << "model=" << meta.model_id << " orientation=" << registry::to_string(meta.orientation)
        << " rts=" << registry::to_string(meta.rts)
        << (meta.super_efficiency ? " super=on" : " super=off") << " n=" << meta.dmu_count
        << " m=" << meta.input_count << " s=" << meta.output_count << '\n';
    out << std::setprecision(10);
    for (const result::DmuResult &dmu : model_result.dmu_results)
    {
        out << row_names[dmu.dmu_index] << ' ';
        if (dmu.has_score())
        {
            out << dmu.score;
        }
        else
        {
            out << '-';
        }
        out << ' ' << result::to_string(dmu.status) << '\n';
    }
    out << "summary: mean=" << model_result.summary.mean_score
        << " median=" << model_result.summary.median_score
        << " efficient=" << model_result.summary.efficient_count << '/' << meta.dmu_count << '\n';
    out << "result: " << result_path << '\n';
}

/**
 * @brief run 子命令（FR-CLI-003：解析 → 数据链路 → 注册表求解 → 导出）。
 * @details 分支 3 路由：--panel + malmquist → 相邻 Malmquist；--panel + --window w
 *          → 窗口分析（底层模型 = --model）；cross_efficiency → 两阶段交叉效率；
 *          其余截面模型走注册表逐 DMU 流水线。
 */
int run_solve(const std::string &model_id, const std::string &data_path, bool as_panel,
              const std::vector<std::string> &explicit_inputs,
              const std::vector<std::string> &explicit_outputs, const std::string &orientation_text,
              const std::string &rts_text, bool rts_explicit, bool super_efficiency,
              const std::string &direction, const std::string &method,
              const std::string &virtual_inputs_text, const std::string &virtual_outputs_text,
              std::size_t window_width, unsigned int thread_count, const std::string &format,
              const std::string &outdir, std::ostream &out, std::ostream &err)
{
    if (model_id.empty())
    {
        err << "[E1] args: 缺少 --model\n";
        return EXIT_ARG;
    }
    // ---- 面板路由（分支 3）：malmquist / --window w ----
    if (as_panel)
    {
        if (model_id != "malmquist" && window_width == 0)
        {
            err << "[E1] args: 面板数据仅支持 malmquist 或 --window w 窗口分析"
                << " (hint: 逐 DMU 面板模型属 M5 参比族)\n";
            return EXIT_ARG;
        }
        dataset::PanelData panel;
        const int panel_status = load_and_build(data_path, true, explicit_inputs, explicit_outputs,
                                                nullptr, &panel, err);
        if (panel_status != EXIT_OK)
        {
            return panel_status;
        }
        registry::Orientation panel_orientation = registry::Orientation::INPUT;
        if (!registry::orientation_from_string(orientation_text, panel_orientation))
        {
            err << "[E1] args: 非法定向 \"" << orientation_text << "\" (hint: in|out|non)\n";
            return EXIT_ARG;
        }
        registry::Rts panel_rts = registry::Rts::CRS;
        if (!registry::rts_from_string(rts_text, panel_rts))
        {
            err << "[E1] args: 非法规模报酬 \"" << rts_text << "\" (hint: crs|vrs|nirs|ndrs)\n";
            return EXIT_ARG;
        }
        registry::SolveOptions panel_options;
        panel_options.orientation = panel_orientation;
        panel_options.rts = panel_rts;
        registry::RunConfig panel_config;
        panel_config.data_sha1 = file_sha1(data_path);
        panel_config.version = VERSION_TEXT;
        panel_config.thread_count = thread_count;

        result::ModelResult panel_result;
        try
        {
            if (model_id == "malmquist")
            {
                panel_result = tfp::run_malmquist(panel, panel_options, panel_config);
            }
            else
            {
                panel_result = window::run_window_analysis(panel, window_width, model_id,
                                                           panel_options, panel_config);
            }
        }
        catch (const registry::EngineException &exception)
        {
            err << "[E" << exception.exit_code() << "] " << exception.module() << ": "
                << exception.what();
            if (!exception.hint().empty())
            {
                err << " (hint: " << exception.hint() << ")";
            }
            err << '\n';
            return exception.exit_code();
        }

        // 行名取 unit_name 标注（窗口/Malmquist 合成行）；列名取首期截面
        std::vector<std::string> row_names;
        row_names.reserve(panel_result.dmu_results.size());
        for (const auto &dmu : panel_result.dmu_results)
        {
            row_names.push_back(dmu.annotations.at("unit_name"));
        }
        const dataset::Dataset &first_section = panel.sections.begin()->second;
        result::ExportNames names{.row_names = row_names,
                                  .input_names = first_section.input_names,
                                  .output_names = first_section.output_names};

        std::error_code panel_fs_error;
        std::filesystem::create_directories(outdir, panel_fs_error);
        if (panel_fs_error)
        {
            err << "[E1] args: 输出目录不可创建 \"" << outdir
                << "\" (hint: " << panel_fs_error.message() << ")\n";
            return EXIT_ARG;
        }
        const std::string panel_ext = format == "csv" ? "csv" : "json";
        const std::string panel_path = (std::filesystem::path{outdir} /
                                        result::result_filename(panel_result.meta, true, panel_ext))
                                           .generic_string();
        const registry::ResultColumnSpec panel_spec =
            panel_result.meta.model_id == "malmquist" ? malmquist_columns() : window_columns();
        try
        {
            if (format == "csv")
            {
                result::write_result_csv(panel_path, panel_result, names, panel_spec);
            }
            else
            {
                result::write_result_json(panel_path, panel_result, names, panel_spec);
            }
        }
        catch (const registry::EngineException &exception)
        {
            err << "[E" << exception.exit_code() << "] " << exception.module() << ": "
                << exception.what() << '\n';
            return exception.exit_code();
        }
        print_run_summary(out, panel_result, names.row_names, panel_path);
        return EXIT_OK;
    }

    registry::Orientation orientation = registry::Orientation::INPUT;
    if (!registry::orientation_from_string(orientation_text, orientation))
    {
        err << "[E1] args: 非法定向 \"" << orientation_text << "\" (hint: in|out|non)\n";
        return EXIT_ARG;
    }
    registry::Rts rts = registry::Rts::CRS;
    if (!registry::rts_from_string(rts_text, rts))
    {
        err << "[E1] args: 非法规模报酬 \"" << rts_text << "\" (hint: crs|vrs|nirs|ndrs)\n";
        return EXIT_ARG;
    }
    if (format != "csv" && format != "json")
    {
        err << "[E1] args: 非法格式 \"" << format << "\" (hint: csv|json)\n";
        return EXIT_ARG;
    }

    // 面板数据链路窗口/Malmquist 属分支 3；本阶段 run 仅消费截面
    dataset::Dataset data;
    const int load_status =
        load_and_build(data_path, false, explicit_inputs, explicit_outputs, &data, nullptr, err);
    if (load_status != EXIT_OK)
    {
        return load_status;
    }

    const registry::ModelDescriptor *descriptor =
        registry::ModelRegistry::instance().find(model_id);
    if (descriptor == nullptr)
    {
        std::string ids;
        for (const std::string &id : registry::ModelRegistry::instance().model_ids())
        {
            ids += ids.empty() ? id : "/" + id;
        }
        err << "[E1] args: 未知模型 \"" << model_id << "\" (hint: 可用模型 " << ids << ")\n";
        return EXIT_ARG;
    }
    if (rts_explicit && !descriptor->uses_rts)
    {
        err << "[E1] args: 模型 \"" << model_id << "\" 不使用 --rts"
            << " (hint: " << descriptor->display_name << ")\n";
        return EXIT_ARG;
    }

    registry::SolveOptions options;
    options.orientation = orientation;
    options.rts = rts;
    if (!direction.empty())
    {
        // 方向来源仅 DDF 消费（06 册 §5）；显式传给其他模型即参数错误
        if (model_id != "ddf")
        {
            err << "[E1] args: --direction 仅适用于 ddf 模型\n";
            return EXIT_ARG;
        }
        options.model_options["direction"] = direction;
    }
    registry::RunConfig config;
    config.super_efficiency = super_efficiency;
    config.data_sha1 = file_sha1(data_path);
    config.version = VERSION_TEXT;
    config.thread_count = thread_count;

    result::ModelResult model_result;
    try
    {
        if (model_id == "cross_efficiency")
        {
            cross::CrossRunOptions cross_options;
            cross_options.rts = rts;
            const std::string method_text = method.empty() ? "dg_i" : method;
            if (!cross::cross_method_from_string(method_text, cross_options.method))
            {
                err << "[E1] args: 非法交叉效率方法 \"" << method_text
                    << "\" (hint: dg_i|dg_ii|m3_i|m3_ii|virtual)\n";
                return EXIT_ARG;
            }
            if (cross_options.method == cross::CrossMethod::VIRTUAL_DMU)
            {
                cross_options.virtual_inputs =
                    parse_number_list(virtual_inputs_text, data.input_count(), err);
                cross_options.virtual_outputs =
                    parse_number_list(virtual_outputs_text, data.output_count(), err);
                if (cross_options.virtual_inputs.empty() || cross_options.virtual_outputs.empty())
                {
                    return EXIT_ARG;
                }
            }
            model_result = cross::run_cross_efficiency(data, cross_options, config);
        }
        else
        {
            if (!method.empty())
            {
                err << "[E1] args: --method 仅适用于 cross_efficiency 模型\n";
                return EXIT_ARG;
            }
            model_result = registry::run_model(model_id, data, options, config);
        }
    }
    catch (const registry::EngineException &exception)
    {
        err << "[E" << exception.exit_code() << "] " << exception.module() << ": "
            << exception.what();
        if (!exception.hint().empty())
        {
            err << " (hint: " << exception.hint() << ")";
        }
        err << '\n';
        return exception.exit_code();
    }

    // 导出（02 册 §4.1：默认 CSV，--format json 可选）
    std::error_code fs_error;
    std::filesystem::create_directories(outdir, fs_error);
    if (fs_error)
    {
        err << "[E1] args: 输出目录不可创建 \"" << outdir << "\" (hint: " << fs_error.message()
            << ")\n";
        return EXIT_ARG;
    }
    const std::string extension = format == "csv" ? "csv" : "json";
    const std::string result_path =
        (std::filesystem::path{outdir} /
         result::result_filename(model_result.meta, descriptor->uses_rts, extension))
            .generic_string();
    try
    {
        if (format == "csv")
        {
            result::write_result_csv(result_path, model_result,
                                     result::ExportNames::from_dataset(data), descriptor->columns);
        }
        else
        {
            result::write_result_json(result_path, model_result,
                                      result::ExportNames::from_dataset(data), descriptor->columns);
        }
    }
    catch (const registry::EngineException &exception)
    {
        err << "[E" << exception.exit_code() << "] " << exception.module() << ": "
            << exception.what() << '\n';
        return exception.exit_code();
    }

    print_run_summary(out, model_result, data.dmu_names, result_path);
    return EXIT_OK;
}
} // namespace

int run_cli(int argc, char **argv, std::ostream &out, std::ostream &err)
{
    CLI::App app{std::string(PROGRAM_NAME) + " — DEA 效率测算引擎（CLI）"};
    app.require_subcommand(0, 1); // --version 可独立使用，不强制子命令

    bool version_requested = false;
    app.add_flag("--version", version_requested, "输出版本信息");

    bool run_version_cmd = false;
    auto *version_cmd = app.add_subcommand("version", "输出版本信息");
    version_cmd->callback([&] { run_version_cmd = true; });

    std::string check_data;
    bool check_panel = false;
    std::vector<std::string> check_inputs;
    std::vector<std::string> check_outputs;
    bool run_check_cmd = false;
    auto *check_cmd = app.add_subcommand("check", "校验数据文件并输出摘要（不求解）");
    check_cmd->add_option("--data", check_data, "数据 CSV 路径")->required();
    check_cmd->add_flag("--panel", check_panel, "按面板长表解析（period 列）");
    check_cmd->add_option("--inputs", check_inputs, "投入列名覆写（可多个）");
    check_cmd->add_option("--outputs", check_outputs, "产出列名覆写（可多个）");
    check_cmd->callback([&] { run_check_cmd = true; });

    std::string solve_model;
    std::string solve_data;
    bool solve_panel = false;
    std::vector<std::string> solve_inputs;
    std::vector<std::string> solve_outputs;
    std::string solve_orientation = "in";
    std::string solve_rts;       // 空串 = 未显式给出（默认 crs；非 rts 模型显式传入报错）
    std::string solve_direction; // 空串 = 未显式给出（ddf 默认 own）
    std::string solve_method;    // 空串 = 未显式给出（cross 默认 dg_i）
    std::string solve_virtual_inputs;
    std::string solve_virtual_outputs;
    std::string solve_window; // 空串 = 未显式给出（--panel + 底层模型时启用窗口）
    std::string solve_format = "csv";
    std::string solve_outdir = ".";
    bool solve_super = false;
    unsigned int solve_threads = 0; // 并行线程数（FR-PERF-001）：0=自动；1=串行
    bool run_solve_cmd = false;
    auto *solve_cmd = app.add_subcommand("run", "求解模型（M2 注册表：径向族/乘子/规模效率）");
    solve_cmd->add_option("--model", solve_model, "模型标识（ccr/bcc/multiplier/scale_efficiency）")
        ->required();
    solve_cmd->add_option("--data", solve_data, "数据 CSV 路径")->required();
    solve_cmd->add_flag("--panel", solve_panel, "按面板长表解析（分支 3 窗口/Malmquist 启用）");
    solve_cmd->add_option("--inputs", solve_inputs, "投入列名覆写（可多个）");
    solve_cmd->add_option("--outputs", solve_outputs, "产出列名覆写（可多个）");
    solve_cmd->add_option("--orientation", solve_orientation, "定向 in|out|non（默认 in）");
    solve_cmd->add_option("--rts", solve_rts, "规模报酬 crs|vrs|nirs|ndrs（默认 crs）");
    solve_cmd->add_option("--direction", solve_direction,
                          "DDF 方向 own|mean|unit|custom|rdm（默认 own）");
    solve_cmd->add_option("--method", solve_method,
                          "交叉效率方法 dg_i|dg_ii|m3_i|m3_ii|virtual（默认 dg_i）");
    solve_cmd->add_option("--virtual-inputs", solve_virtual_inputs,
                          "虚拟 DMU 投入向量（逗号分隔，--method virtual）");
    solve_cmd->add_option("--virtual-outputs", solve_virtual_outputs,
                          "虚拟 DMU 产出向量（逗号分隔，--method virtual）");
    solve_cmd->add_option("--window", solve_window,
                          "窗口宽度 w（--panel + 底层模型时启用窗口分析）");
    solve_cmd->add_flag("--super", solve_super, "超效率（参考集剔除被评 DMU）");
    solve_cmd->add_option("--threads", solve_threads,
                          "并行线程数（FR-PERF-001）：0=自动（默认）；1=串行");
    solve_cmd->add_option("--format", solve_format, "结果格式 csv|json（默认 csv）");
    solve_cmd->add_option("-o,--outdir", solve_outdir, "结果输出目录（默认当前目录）");
    solve_cmd->callback([&] { run_solve_cmd = true; });

    try
    {
        CLI11_PARSE(app, argc, argv);
    }
    catch (const CLI::ParseError &error)
    {
        err << "[E1] args: " << error.what() << '\n';
        return EXIT_ARG;
    }

    if (version_requested)
    {
        return run_version(out);
    }
    if (run_version_cmd)
    {
        return run_version(out);
    }
    if (run_check_cmd)
    {
        return run_check(check_data, check_panel, check_inputs, check_outputs, out, err);
    }
    if (run_solve_cmd)
    {
        const bool rts_explicit = !solve_rts.empty();
        std::size_t window_width = 0;
        if (!solve_window.empty())
        {
            try
            {
                window_width = static_cast<std::size_t>(std::stoul(solve_window));
            }
            catch (const std::exception &)
            {
                err << "[E1] args: 非法窗口宽度 \"" << solve_window << "\"\n";
                return EXIT_ARG;
            }
        }
        return run_solve(solve_model, solve_data, solve_panel, solve_inputs, solve_outputs,
                         solve_orientation, rts_explicit ? solve_rts : "crs", rts_explicit,
                         solve_super, solve_direction, solve_method, solve_virtual_inputs,
                         solve_virtual_outputs, window_width, solve_threads, solve_format,
                         solve_outdir, out, err);
    }
    return EXIT_OK;
}
} // namespace opensuperdea::cli
