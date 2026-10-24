/**
 * @file scale_efficiency_model.h
 * @brief 规模效率分解：TE = PTE × SE + 规模报酬区间判定（design_docs/06 册 §3.3）。
 * @details 同一 DMU 解 CRS 与 VRS 两次：TE = θ*_CRS、PTE = θ*_VRS、SE = TE/PTE；
 *          NIRS 用于判别规模报酬区间（SE<1 且 θ_NIRS>θ_VRS → IRS；θ_NIRS=θ_VRS → DRS；
 *          SE≈1 → CRS）。三 LP 复用 RadialEnvelopeModel 构造，RTS 逐个覆写。
 */
#pragma once

#include "core/registry/solvable_model.h"

namespace opensuperdea::radial
{
/**
 * @brief 规模效率分解模型（不走 SolvableModel concept：每 DMU 三次求解，
 *        注册表以手工工厂 lambda 封装，见 model_registry.cpp）。
 */
class ScaleEfficiencyModel
{
  public:
    /**
     * @brief 求解单 DMU 的规模效率分解。
     * @param context 求解上下文（options.orientation 生效；options.rts 被忽略并覆写）
     * @return 单 DMU 结果：score=TE，annotations 含 te/pte/se/rts_region
     * @throws registry::EngineException 任一 LP 求解失败（EXIT_SOLVE）
     */
    [[nodiscard]] static result::DmuResult evaluate(const registry::SolveContext &context);
};
} // namespace opensuperdea::radial
