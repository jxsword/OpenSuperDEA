/**
 * @file exit_codes.h
 * @brief CLI 统一退出码（FR-CLI-007；设计文档 03 册 §3.2）。
 */
#pragma once

#include <cstdint>

namespace opensuperdea::cli
{
/** @brief 进程退出码：0 成功；1 参数；2 数据；3 求解；4 内部。
 *  非 enum class：枚举值需在 cli 层裸用（返回 int 语义）；基类型收敛满足性能检查。 */
enum ExitCode : std::uint8_t
{
    EXIT_OK = 0,
    EXIT_ARG = 1,
    EXIT_DATA = 2,
    EXIT_SOLVE = 3,
    EXIT_INTERNAL = 4,
};
} // namespace opensuperdea::cli
