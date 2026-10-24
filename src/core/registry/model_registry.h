/**
 * @file model_registry.h
 * @brief 模型注册表：model_id → 描述符（design_docs/01 册 §4.1 + 06 册 §1）。
 * @details 全部模型经注册表统一暴露，CLI/GUI/测试零特判（01 册设计目标「模型即插件」）。
 *          内建模型在首次访问实例时注册；后续里程碑（GUI 动态注册）追加 register_model。
 */
#pragma once

#include <map>
#include <string>
#include <vector>

#include "engine_error.h"
#include "model_descriptor.h"
#include "solve_options.h"

namespace opensuperdea::registry
{
/** @brief 模型注册表（进程级单例）。 */
class ModelRegistry
{
  public:
    /** @brief 进程级实例（首次访问时注册全部内建模型）。 */
    [[nodiscard]] static const ModelRegistry &instance();

    /** @brief 注册一个模型描述符（model_id 重复抛 EngineException）。 */
    void register_model(ModelDescriptor descriptor);

    /** @brief 按 model_id 查找；未注册返回 nullptr。 */
    [[nodiscard]] const ModelDescriptor *find(const std::string &model_id) const;

    /**
     * @brief 参数组合合法性校验（FR-CLI-008：模型 × 定向 × RTS × 超效率）。
     * @param model_id 模型标识（未注册时返回 false）
     * @param orientation 定向
     * @param rts 规模报酬
     * @param super 是否请求超效率参考集
     * @param reason 不合法时填写人类可读原因（可为 nullptr）
     * @return 组合是否合法
     */
    [[nodiscard]] bool validate_combo(const std::string &model_id, Orientation orientation, Rts rts,
                                      bool super, std::string *reason) const;

    /** @brief 已注册模型 id 清单（注册顺序；CLI 错误提示用）。 */
    [[nodiscard]] std::vector<std::string> model_ids() const;

  private:
    ModelRegistry() = default;
    void register_builtin_models();

    std::map<std::string, ModelDescriptor> models; // 按 model_id 有序
};
} // namespace opensuperdea::registry
