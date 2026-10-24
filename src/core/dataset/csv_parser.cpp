/**
 * @file csv_parser.cpp
 * @brief CSV 词法解析实现（逐字符状态机，与平台换行符无关）。
 */
#include "csv_parser.h"

#include <array>
#include <fstream>
#include <sstream>

namespace opensuperdea::dataset
{
namespace
{
/** @brief 跳过 UTF-8 BOM（EF BB BF）；starts_with 为 C++20 API，此处手工比较。 */
std::string_view strip_bom(std::string_view text)
{
    constexpr std::string_view BOM = "\xEF\xBB\xBF";
    if (text.size() >= BOM.size() && text.substr(0, BOM.size()) == BOM)
    {
        text.remove_prefix(BOM.size());
    }
    return text;
}

/** @brief 单行内逗号切分（无引号语义；连续逗号产生空字段，由校验层报非数值）。 */
std::vector<std::string> split_line(std::string_view line)
{
    std::vector<std::string> fields;
    std::size_t start = 0;
    while (true)
    {
        const std::size_t comma = line.find(',', start);
        if (comma == std::string_view::npos)
        {
            fields.emplace_back(line.substr(start));
            break;
        }
        fields.emplace_back(line.substr(start, comma - start));
        start = comma + 1;
    }
    return fields;
}
} // namespace

CsvTable parse_csv_text(std::string_view text, CsvParseError *parse_error)
{
    CsvTable table;
    text = strip_bom(text);
    bool has_header = false;
    std::size_t physical_line = 0;
    std::size_t field_count = 0;

    std::size_t start = 0;
    while (start <= text.size())
    {
        const std::size_t newline = text.find('\n', start);
        std::string_view line = (newline == std::string_view::npos)
                                    ? text.substr(start)
                                    : text.substr(start, newline - start);
        ++physical_line;
        start = (newline == std::string_view::npos) ? text.size() + 1 : newline + 1;

        // 统一处理 \r\n / \r / \n 三种换行残迹
        while (!line.empty() && (line.back() == '\r' || line.back() == '\n'))
        {
            line.remove_suffix(1);
        }
        if (line.empty() || line.front() == '#')
        {
            continue; // 空行与注释行跳过（02 册 §2.1）
        }

        auto fields = split_line(line);
        if (!has_header)
        {
            table.headers = std::move(fields);
            field_count = table.headers.size();
            has_header = true;
            continue;
        }
        if (fields.size() != field_count)
        {
            if (parse_error != nullptr)
            {
                parse_error->line_number = physical_line;
                parse_error->message = "列数与表头不一致（期望 " + std::to_string(field_count) +
                                       " 列，实际 " + std::to_string(fields.size()) + " 列）";
            }
            return CsvTable{};
        }
        table.rows.push_back(std::move(fields));
    }
    return table;
}

CsvTable read_csv_file(const std::string &path, CsvParseError *parse_error)
{
    std::ifstream stream(path, std::ios::binary);
    if (!stream.is_open())
    {
        if (parse_error != nullptr)
        {
            parse_error->line_number = 0;
            parse_error->message = "文件不存在或不可读: " + path;
        }
        return CsvTable{};
    }
    std::ostringstream buffer;
    buffer << stream.rdbuf();
    return parse_csv_text(buffer.str(), parse_error);
}
} // namespace opensuperdea::dataset
