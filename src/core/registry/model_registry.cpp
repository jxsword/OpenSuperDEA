/**
 * @file model_registry.cpp
 * @brief 模型注册表实现：内建模型注册与组合校验（FR-CLI-008）。
 * @details 注册的工厂 lambda 捕获具体模型类型（06 册 §1）；M2 分支 1 注册径向族
 *          （ccr/bcc/超效率经 run 级开关）+ 乘子 + 规模效率，后续分支增量注册。
 */
#include "model_registry.h"

#include <utility>

#include "core/radial/multiplier_model.h"
#include "core/radial/radial_envelope_model.h"
#include "core/radial/scale_efficiency_model.h"

namespace opensuperdea::registry
{
namespace
{
using opensuperdea::radial::MultiplierModel;
using opensuperdea::radial::RadialEnvelopeModel;
using opensuperdea::radial::ScaleEfficiencyModel;

/** @brief scale_efficiency 的手工工厂：三 LP 分解流程封装为求解函数。 */
SolveFunction make_scale_efficiency_function()
{
    return [](const SolveContext &context) -> opensuperdea::result::DmuResult {
        ScaleEfficiencyModel model;
        return model.evaluate(context);
    };
}
} // namespace

const std::set<Orientation> &all_orientations()
{
    static const std::set<Orientation> k_set{Orientation::INPUT, Orientation::OUTPUT,
                                             Orientation::NON_DIRECTIONAL};
    return k_set;
}

const std::set<Rts> &all_rts()
{
    static const std::set<Rts> k_set{Rts::CRS, Rts::VRS, Rts::NIRS, Rts::NDRS};
    return k_set;
}

const ModelRegistry &ModelRegistry::instance()
{
    // 静态局部：内建模型仅在首次访问时注册一次（register_model 对重复注册抛错）
    static ModelRegistry registry = [] {
        ModelRegistry r;
        r.register_builtin_models();
        return r;
    }();
    return registry;
}

void ModelRegistry::register_model(ModelDescriptor descriptor)
{
    const std::string id = descriptor.model_id;
    if (id.empty())
    {
        throw EngineException(4, "internal", "注册模型缺少 model_id");
    }
    if (!models.emplace(id, std::move(descriptor)).second)
    {
        throw EngineException(4, "internal", "模型重复注册: " + id);
    }
}

const ModelDescriptor *ModelRegistry::find(const std::string &model_id) const
{
    const auto it = models.find(model_id);
    return it == models.end() ? nullptr : &it->second;
}

bool ModelRegistry::validate_combo(const std::string &model_id, Orientation orientation, Rts rts,
                                   bool super, std::string *reason) const
{
    const auto set_reason = [reason](const std::string &text) {
        if (reason != nullptr)
        {
            *reason = text;
        }
    };

    const ModelDescriptor *descriptor = find(model_id);
    if (descriptor == nullptr)
    {
        set_reason("未知模型 \"" + model_id + "\"");
        return false;
    }
    if (!descriptor->orientations.contains(orientation))
    {
        set_reason("模型 \"" + model_id + "\" 不支持该定向");
        return false;
    }
    if (descriptor->uses_rts && !descriptor->rts.contains(rts))
    {
        set_reason("模型 \"" + model_id + "\" 不支持该规模报酬假定");
        return false;
    }
    if (super && !descriptor->supports_super)
    {
        set_reason("模型 \"" + model_id + "\" 不支持超效率（--super）");
        return false;
    }
    return true;
}

std::vector<std::string> ModelRegistry::model_ids() const
{
    std::vector<std::string> ids;
    ids.reserve(models.size());
    for (const auto &[id, descriptor] : models)
    {
        ids.push_back(id);
    }
    return ids;
}

void ModelRegistry::register_builtin_models()
{
    // ---- ccr：径向包络族（crs/vrs/nirs/ndrs 全 RTS；vrs 即 BCC，06 册 §3.1）----
    {
        ModelDescriptor d;
        d.model_id = "ccr";
        d.display_name = "CCR 径向包络（RTS 经 --rts 选择，vrs 即 BCC）";
        d.orientations = {Orientation::INPUT, Orientation::OUTPUT};
        d.rts = all_rts();
        d.supports_super = true;
        d.columns.include_projections = true;
        d.columns.include_slacks = true;
        d.columns.include_lambdas = true;
        d.solve = make_solve_function<RadialEnvelopeModel>();
        register_model(std::move(d));
    }
    // ---- bcc：径向包络 VRS 严格别名（MaxDEA 命名习惯）----
    {
        ModelDescriptor d;
        d.model_id = "bcc";
        d.display_name = "BCC 径向包络（VRS）";
        d.orientations = {Orientation::INPUT, Orientation::OUTPUT};
        d.rts = {Rts::VRS};
        d.supports_super = true;
        d.columns.include_projections = true;
        d.columns.include_slacks = true;
        d.columns.include_lambdas = true;
        d.solve = make_solve_function<RadialEnvelopeModel>();
        register_model(std::move(d));
    }
    // ---- multiplier：CCR/BCC 乘子（对偶）模型（06 册 §3.4）----
    {
        ModelDescriptor d;
        d.model_id = "multiplier";
        d.display_name = "径向乘子模型（CCR/BCC 对偶形式，输出最优乘子）";
        d.orientations = {Orientation::INPUT, Orientation::OUTPUT};
        d.rts = {Rts::CRS, Rts::VRS};
        d.columns.include_multipliers = true;
        d.solve = make_solve_function<MultiplierModel>();
        register_model(std::move(d));
    }
    // ---- scale_efficiency：TE = PTE × SE 分解（06 册 §3.3；不消费 --rts）----
    {
        ModelDescriptor d;
        d.model_id = "scale_efficiency";
        d.display_name = "规模效率分解（TE/PTE/SE + 规模报酬区间）";
        d.orientations = {Orientation::INPUT, Orientation::OUTPUT};
        d.uses_rts = false;
        d.columns.extra_columns = {"te", "pte", "se", "rts_region"};
        d.solve = make_scale_efficiency_function();
        register_model(std::move(d));
    }
}
} // namespace opensuperdea::registry
