/**
 * @file engine_error.h
 * @brief 引擎类型化异常（design_docs/01 册 §4.3 错误处理策略）。
 * @details 引擎内部抛 EngineException，CLI/GUI 边界统一捕获转错误码 + 消息；
 *          exit_code 取值与 FR-CLI-007 退出码规范一致。
 */
#pragma once

#include <stdexcept>
#include <string>
#include <utility>

namespace opensuperdea::registry
{
/** @brief 引擎异常：携带退出码 / 模块名 / 消息 / 修复提示四元组。 */
class EngineException : public std::runtime_error
{
  public:
    /**
     * @brief 构造引擎异常。
     * @param exit_code_arg 进程退出码（FR-CLI-007）
     * @param module_arg 模块名（args/data/solve/internal）
     * @param message_arg 错误原因
     * @param hint_arg 修复提示（可为空）
     */
    // module/hint 按值接收以供成员初始化列表移动构造（检查器不分析初始化列表，豁免）
    EngineException(int exit_code_arg, std::string module_arg, const std::string &message_arg,
                    std::string hint_arg = "") // NOLINT(performance-unnecessary-value-param)
        : std::runtime_error(message_arg), exit_code_value(exit_code_arg),
          module_value(std::move(module_arg)), hint_value(std::move(hint_arg))
    {
    }

    /** @brief 进程退出码。 */
    [[nodiscard]] int exit_code() const
    {
        return exit_code_value;
    }

    /** @brief 出错模块名。 */
    [[nodiscard]] const std::string &module() const
    {
        return module_value;
    }

    /** @brief 修复提示（可为空）。 */
    [[nodiscard]] const std::string &hint() const
    {
        return hint_value;
    }

  private:
    int exit_code_value;
    std::string module_value;
    std::string hint_value;
};
} // namespace opensuperdea::registry
