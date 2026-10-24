/**
 * @file scale_efficiency_model.cpp
 * @brief 规模效率分解实现：CRS/VRS/NIRS 三次包络求解与分解恒等式（FR-RAD-004）。
 */
#include "scale_efficiency_model.h"

#include <cmath>
#include <iomanip>
#include <sstream>

#include "core/registry/engine_error.h"
#include "radial_envelope_model.h"

namespace opensuperdea::radial
{
namespace
{
using opensuperdea::registry::EngineException;
using opensuperdea::registry::Rts;
using opensuperdea::registry::SolveContext;
using opensuperdea::solver::SolveStatus;

/** @brief 效率值 1 / 相等判定的工程容差。 */
constexpr double K_UNIT_TOLERANCE = 1e-9;

/** @brief 标注数值文本：17 位有效数字（max_digits10，往返无损，保 L3 恒等式）。 */
std::string format_annotation(double value)
{
    std::ostringstream stream;
    stream << std::setprecision(17) << value;
    return std::move(stream).str();
}

/**
 * @brief 以指定 RTS 求一次径向包络，返回效率测度（投入 θ* / 产出 1/φ*）。
 * @throws EngineException 求解失败
 */
double solve_radial_score(const SolveContext &context, Rts rts)
{
    SolveContext rts_context = context;
    rts_context.options.rts = rts;
    RadialEnvelopeModel model;
    const solver::LpProblem problem = model.build_problem(rts_context);
    const solver::SolveOutcome outcome = opensuperdea::adapters::solve_lp(problem);
    if (outcome.status != SolveStatus::OPTIMAL)
    {
        throw EngineException(3, "solve", "规模效率分解 LP 求解失败（RTS 求解非最优）");
    }
    if (context.options.orientation == registry::Orientation::INPUT)
    {
        return outcome.objective_value;
    }
    if (outcome.objective_value <= K_UNIT_TOLERANCE)
    {
        throw EngineException(3, "solve", "输出导向 φ* 非正（产出数据退化）");
    }
    return 1.0 / outcome.objective_value;
}
} // namespace

result::DmuResult ScaleEfficiencyModel::evaluate(const SolveContext &context)
{
    const double te = solve_radial_score(context, Rts::CRS);  // 技术效率（CRS）
    const double pte = solve_radial_score(context, Rts::VRS); // 纯技术效率（VRS）
    const double nirs = solve_radial_score(context, Rts::NIRS);
    if (pte <= 0.0)
    {
        throw EngineException(4, "internal", "PTE 非正，无法分解规模效率");
    }

    const double se = te / pte;
    // 规模报酬区间判定（06 册 §3.3）：SE≈1 → CRS；θ_NIRS=θ_VRS → DRS；否则 IRS
    const bool is_constant = std::abs(se - 1.0) <= K_UNIT_TOLERANCE;
    const bool nirs_equals_vrs = std::abs(nirs - pte) <= K_UNIT_TOLERANCE;
    const char *region = "irs";
    if (is_constant)
    {
        region = "crs";
    }
    else if (nirs_equals_vrs)
    {
        region = "drs";
    }

    result::DmuResult out;
    out.dmu_index = context.dmu_index;
    out.score = te;
    out.status = is_constant ? result::DmuStatus::EFFICIENT : result::DmuStatus::INEFFICIENT;
    out.annotations.emplace("te", format_annotation(te));
    out.annotations.emplace("pte", format_annotation(pte));
    out.annotations.emplace("se", format_annotation(se));
    out.annotations.emplace("rts_region", region);
    return out;
}
} // namespace opensuperdea::radial
