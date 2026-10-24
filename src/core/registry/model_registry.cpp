/**
 * @file model_registry.cpp
 * @brief 模型注册表实现：内建模型注册与组合校验（FR-CLI-008）。
 * @details 注册的工厂 lambda 捕获具体模型类型（06 册 §1）；分支 1 注册径向族，
 *          分支 2 增量注册 SBM 族，后续分支继续增量。
 */
#include "model_registry.h"

#include <array>
#include <string_view>
#include <utility>

#include "core/distance/ddf_model.h"
#include "core/nonradial/additive_models.h"
#include "core/nonradial/fdh_model.h"
#include "core/nonradial/price_models.h"
#include "core/nonradial/sbm_model.h"
#include "core/radial/multiplier_model.h"
#include "core/radial/radial_envelope_model.h"
#include "core/radial/scale_efficiency_model.h"

namespace opensuperdea::registry
{
namespace
{
using opensuperdea::distance::DdfModel;
using opensuperdea::nonradial::CostModel;
using opensuperdea::nonradial::FdhModel;
using opensuperdea::nonradial::ProfitModel;
using opensuperdea::nonradial::RevenueModel;
using opensuperdea::nonradial::SbmModel;
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

/** @brief 加权 SBM 工厂：注入 weight 选项后委托 SbmModel。 */
SolveFunction make_weighted_sbm_function()
{
    return [](const SolveContext &context) -> opensuperdea::result::DmuResult {
        SolveContext weighted = context;
        weighted.options.model_options["weight"] = "1";
        SbmModel model;
        const solver::LpProblem problem = model.build_problem(weighted);
        const solver::SolveOutcome outcome = adapters::solve_lp(problem);
        return model.assemble_result(weighted, outcome);
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
    // ---- sbm：SBM（Tone 2001；3 定向 × crs/vrs；--super 为 Tone 2002 超效率）----
    {
        ModelDescriptor d;
        d.model_id = "sbm";
        d.display_name = "SBM 非径向测度（in/out/non 定向，--super 超效率）";
        d.orientations = {Orientation::INPUT, Orientation::OUTPUT, Orientation::NON_DIRECTIONAL};
        d.rts = {Rts::CRS, Rts::VRS};
        d.supports_super = true;
        d.columns.include_slacks = true;
        d.columns.include_lambdas = true;
        d.solve = make_solve_function<SbmModel>();
        register_model(std::move(d));
    }
    // ---- sbm_weighted：加权 SBM（06 册 §4.4 #6-8；消费 w_in_/w_out_ 列）----
    {
        ModelDescriptor d;
        d.model_id = "sbm_weighted";
        d.display_name = "加权 SBM（权重来自 w_in_/w_out_ 列）";
        d.orientations = {Orientation::INPUT, Orientation::OUTPUT, Orientation::NON_DIRECTIONAL};
        d.rts = {Rts::CRS, Rts::VRS};
        d.columns.include_slacks = true;
        d.columns.include_lambdas = true;
        d.solve = make_weighted_sbm_function();
        register_model(std::move(d));
    }
    // ---- fdh：自由处置包络（MILP；06 册 §7；rts 不适用）----
    {
        ModelDescriptor d;
        d.model_id = "fdh";
        d.display_name = "FDH 自由处置包络（MILP，0-1 支配；in/out 定向）";
        d.orientations = {Orientation::INPUT, Orientation::OUTPUT};
        d.uses_rts = false;
        d.supports_super = true;
        d.columns.include_lambdas = true;
        d.solve = [](const SolveContext &context) -> opensuperdea::result::DmuResult {
            opensuperdea::nonradial::FdhModel model;
            const solver::MilpProblem problem = model.build_milp_problem(context);
            const solver::SolveOutcome outcome = adapters::solve_milp(problem);
            return model.assemble_result(context, outcome);
        };
        register_model(std::move(d));
    }
    // ---- cost/revenue/profit：价格族（FR-COST-001~003；消费 pi_/po_ 列；定向不适用）----
    {
        ModelDescriptor d;
        d.model_id = "cost";
        d.display_name = "成本效率（min 成本 + 技术/配置分解；需 pi_ 列）";
        d.orientations = all_orientations();
        d.rts = all_rts();
        d.columns.include_projections = true;
        d.columns.extra_columns = {"min_cost", "te", "ae"};
        d.solve = make_solve_function<CostModel>();
        register_model(std::move(d));
    }
    {
        ModelDescriptor d;
        d.model_id = "revenue";
        d.display_name = "收益效率（max 收益；需 po_ 列）";
        d.orientations = all_orientations();
        d.rts = all_rts();
        d.columns.include_projections = true;
        d.columns.extra_columns = {"max_revenue"};
        d.solve = make_solve_function<RevenueModel>();
        register_model(std::move(d));
    }
    {
        ModelDescriptor d;
        d.model_id = "profit";
        d.display_name = "利润效率（max 利润；效率值可为负；CRS 无界记 infeasible；需 pi_/po_ 列）";
        d.orientations = all_orientations();
        d.rts = all_rts();
        d.columns.include_projections = true;
        d.columns.extra_columns = {"max_profit"};
        d.solve = make_solve_function<ProfitModel>();
        register_model(std::move(d));
    }
    // ---- 加权加性族（06 册 §4.4 P-006 清单 #1/2/3/4/9；score = 加权松弛和）----
    const std::array<std::pair<const char *, const char *>, 5> additive_variants = {{
        {"additive", "ADD 简单加性（松弛和；raw）"},
        {"additive_normalized", "归一化加权加性（Lovell-Pastor 1995；mean）"},
        {"ram", "RAM 范围调整测度（Cooper-Pastor-Tone 1997；range）"},
        {"bam", "BAM 有界调整测度（M2 界 = 数据范围；bam）"},
        {"additive_weighted", "wADD 一般权加性（w_in_/w_out_ 列；user）"},
    }};
    for (const auto &[id, name] : additive_variants)
    {
        ModelDescriptor d;
        d.model_id = id;
        d.display_name = name;
        d.orientations = {Orientation::NON_DIRECTIONAL};
        d.rts = all_rts();
        d.columns.include_slacks = true;
        d.columns.include_lambdas = true;
        std::string mode = "user";
        if (std::string_view{id} == "additive")
        {
            mode = "raw";
        }
        else if (std::string_view{id} == "additive_normalized")
        {
            mode = "mean";
        }
        else if (std::string_view{id} == "ram")
        {
            mode = "range";
        }
        else if (std::string_view{id} == "bam")
        {
            mode = "bam";
        }
        d.solve = [mode](const SolveContext &context) -> opensuperdea::result::DmuResult {
            SolveContext with_mode = context;
            with_mode.options.model_options["weight_mode"] = mode;
            opensuperdea::nonradial::AdditiveSlacksModel model;
            const solver::LpProblem problem = model.build_problem(with_mode);
            const solver::SolveOutcome outcome = adapters::solve_lp(problem);
            return model.assemble_result(with_mode, outcome);
        };
        register_model(std::move(d));
    }
    // ---- dsbm：方向 SBM（06 册 §4.4 #5；方向来源经 --direction）----
    {
        ModelDescriptor d;
        d.model_id = "dsbm";
        d.display_name = "方向 SBM（方向 own/mean/unit/custom）";
        d.orientations = {Orientation::NON_DIRECTIONAL};
        d.rts = {Rts::CRS, Rts::VRS};
        d.columns.include_slacks = true;
        d.columns.include_lambdas = true;
        d.solve = make_solve_function<opensuperdea::nonradial::DsbmModel>();
        register_model(std::move(d));
    }
    // ---- erm：增强 Russell 测度（06 册 §4.3；non 图测度 = SBM，委托 SbmModel）----
    {
        ModelDescriptor d;
        d.model_id = "erm";
        d.display_name = "ERM 增强 Russell 测度（io/oo Russell；non = SBM 图测度）";
        d.orientations = {Orientation::INPUT, Orientation::OUTPUT, Orientation::NON_DIRECTIONAL};
        d.rts = all_rts();
        d.columns.include_lambdas = true;
        d.solve = [](const SolveContext &context) -> opensuperdea::result::DmuResult {
            if (context.options.orientation == Orientation::NON_DIRECTIONAL)
            {
                // Tone 2001：ERM 图测度比值形与 SBM 恒等（06 册 §4.3 数值关系注记）
                opensuperdea::nonradial::SbmModel sbm;
                const solver::LpProblem problem = sbm.build_problem(context);
                const solver::SolveOutcome outcome = adapters::solve_lp(problem);
                return sbm.assemble_result(context, outcome);
            }
            opensuperdea::nonradial::ErmModel erm;
            const solver::LpProblem problem = erm.build_problem(context);
            const solver::SolveOutcome outcome = adapters::solve_lp(problem);
            return erm.assemble_result(context, outcome);
        };
        register_model(std::move(d));
    }
    // ---- ddf：方向距离函数（06 册 §5；方向来源经 --direction）----
    {
        ModelDescriptor d;
        d.model_id = "ddf";
        d.display_name = "方向距离函数 DDF（方向 own/mean/unit/custom/rdm）";
        d.orientations = {Orientation::INPUT, Orientation::OUTPUT, Orientation::NON_DIRECTIONAL};
        d.rts = all_rts();
        d.supports_super = true;
        d.columns.include_lambdas = true;
        d.solve = make_solve_function<opensuperdea::distance::DdfModel>();
        register_model(std::move(d));
    }
}
} // namespace opensuperdea::registry
