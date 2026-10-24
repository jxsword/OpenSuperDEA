/**
 * @file cli_app.cpp
 * @brief CLI 子命令实现：version / check / run（数据链路全通，模型执行占位）。
 * @details 错误策略（FR-CLI-007/03 册 §3.2）：所有错误走 stderr 且带 [E<code>] 前缀；
 *          数据错误聚合输出（单次报告全部）。M1 run 的模型白名单仅含 ccr 占位，
 *          返回统一提示（EXIT_ARG），M2 由模型注册表替换（03 册 §3.1）。
 *          回调仅收集意图，parse 后统一执行——避免回调内 std::exit 跳过返回路径。
 */
#include "cli_app.h"

#include <CLI/CLI.hpp>

#include "core/dataset/csv_parser.h"
#include "core/dataset/dataset_builder.h"
#include "core/dataset/validator.h"
#include "exit_codes.h"

#include <algorithm>
#include <exception>
#include <iostream>
#include <string>
#include <vector>

namespace opensuperdea::cli
{
namespace
{
constexpr const char *VERSION_TEXT = "0.1.0";
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

/** @brief 数据载入链路：读文件 → 角色识别 → 校验 → 构造（截面或面板）。 */
int load_and_validate(const std::string &data_path, bool as_panel,
                      const std::vector<std::string> &explicit_inputs,
                      const std::vector<std::string> &explicit_outputs, std::ostream &err)
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
    auto validation_errors = dataset::validate_table(table, roles.input_columns,
                                                     roles.output_columns, data_path, as_panel);
    errors.insert(errors.end(), validation_errors.begin(), validation_errors.end());

    if (as_panel)
    {
        auto panel = dataset::build_panel(table, roles, data_path, errors);
        static_cast<void>(panel);
    }
    else
    {
        auto data = dataset::build_dataset(table, roles, data_path, errors);
        static_cast<void>(data);
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
    const int load_status =
        load_and_validate(data_path, as_panel, explicit_inputs, explicit_outputs, err);
    if (load_status != EXIT_OK)
    {
        return load_status;
    }
    // 摘要：重新构造一次拿到计数（check 场景文件小，二次构造成本可忽略）
    dataset::CsvParseError parse_error{};
    auto table = dataset::read_csv_file(data_path, &parse_error);
    dataset::BuildOptions options;
    options.explicit_inputs = explicit_inputs;
    options.explicit_outputs = explicit_outputs;
    std::vector<dataset::ValidationError> errors;
    const auto roles = dataset::identify_roles(table, options, data_path, errors);
    if (as_panel)
    {
        auto panel = dataset::build_panel(table, roles, data_path, errors);
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
        auto data = dataset::build_dataset(table, roles, data_path, errors);
        out << "dataset: " << data.dmu_count() << " DMU, " << data.input_count() << " 投入, "
            << data.output_count() << " 产出" << '\n';
    }
    return EXIT_OK;
}

/** @brief M1 模型能力白名单（03 册 §3.1：注册表占位，M2 替换）。 */
bool model_id_known(const std::string &model_id)
{
    return model_id == "ccr";
}

/** @brief run 子命令（FR-CLI-003 骨架：解析 + 数据链路 + 模型占位）。 */
int run_solve(const std::string &model_id, const std::string &data_path, bool as_panel,
              const std::vector<std::string> &explicit_inputs,
              const std::vector<std::string> &explicit_outputs, std::ostream &err)
{
    if (!model_id_known(model_id))
    {
        err << "[E1] args: 未知模型 \"" << model_id << "\" (hint: M1 仅提供 ccr 占位，"
            << "常规模型 M2 起可用)\n";
        return EXIT_ARG;
    }
    // M1 骨架：数据链路先行走通（错误提前暴露），模型执行留待 M2 注册表
    const int load_status =
        load_and_validate(data_path, as_panel, explicit_inputs, explicit_outputs, err);
    if (load_status != EXIT_OK)
    {
        return load_status;
    }
    err << "[E1] args: 模型 \"" << model_id << "\" 尚未实现 (hint: 首个模型 CCR 属 M2)\n";
    return EXIT_ARG;
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
    bool run_solve_cmd = false;
    auto *solve_cmd = app.add_subcommand("run", "求解模型（M1 骨架：参数与数据链路）");
    solve_cmd->add_option("--model", solve_model, "模型标识（如 ccr）")->required();
    solve_cmd->add_option("--data", solve_data, "数据 CSV 路径")->required();
    solve_cmd->add_flag("--panel", solve_panel, "按面板长表解析（period 列）");
    solve_cmd->add_option("--inputs", solve_inputs, "投入列名覆写（可多个）");
    solve_cmd->add_option("--outputs", solve_outputs, "产出列名覆写（可多个）");
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
        return run_solve(solve_model, solve_data, solve_panel, solve_inputs, solve_outputs, err);
    }
    return EXIT_OK;
}
} // namespace opensuperdea::cli
