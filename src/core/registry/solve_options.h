/**
 * @file solve_options.h
 * @brief 求解选项公共类型：定向 / 规模报酬 / 选项容器（design_docs/02 册 §1 SolveOptions 契约）。
 * @details 全引擎共用（CLI 解析 → 注册表校验 → 模型消费）；字符串互转供 CLI 与
 *          结果导出使用，非法串返回 false 由调用方报 EXIT_ARG。
 */
#pragma once

#include <cstdint>
#include <map>
#include <string>

namespace opensuperdea::registry
{
/** @brief 定向：投入 / 产出 / 非导向（FR-CLI-003 选项 in|out|non）。 */
enum class Orientation : std::uint8_t
{
    INPUT,
    OUTPUT,
    NON_DIRECTIONAL
};

/** @brief 规模报酬假定：CRS/VRS/NIRS/NDRS（FR-RAD-001~003）。 */
enum class Rts : std::uint8_t
{
    CRS,
    VRS,
    NIRS,
    NDRS
};

/** @brief 一次求解的选项集合（模型只读消费）。 */
struct SolveOptions
{
    Orientation orientation = Orientation::INPUT;
    Rts rts = Rts::CRS;
    std::map<std::string, std::string> model_options; // 模型特有键值（M2 各模型按需读取）
};

/** @brief 定向字符串 → 枚举；仅接受 "in"/"out"/"non"。 */
[[nodiscard]] bool orientation_from_string(const std::string &text, Orientation &out);

/** @brief 枚举 → 定向字符串（"in"/"out"/"non"，导出与回显用）。 */
[[nodiscard]] std::string to_string(Orientation orientation);

/** @brief 规模报酬字符串 → 枚举；仅接受 "crs"/"vrs"/"nirs"/"ndrs"。 */
[[nodiscard]] bool rts_from_string(const std::string &text, Rts &out);

/** @brief 枚举 → 规模报酬字符串（"crs"/"vrs"/"nirs"/"ndrs"）。 */
[[nodiscard]] std::string to_string(Rts rts);
} // namespace opensuperdea::registry
