/**
 * @file model_descriptor.h
 * @brief 模型描述符：能力矩阵 + 结果列集 + 求解工厂（design_docs/01 册 §4.1 + 06 册 §1）。
 * @details 能力矩阵即注册表数据（FR-CLI-008）：validate_combo 直接查描述符集合，
 *          CLI/GUI 共用；列集声明驱动 CSV/JSON 导出（02 册 §4.1）。
 */
#pragma once

#include <map>
#include <set>
#include <string>
#include <vector>

#include "solvable_model.h"
#include "solve_options.h"

namespace opensuperdea::registry
{
/** @brief 结果列集声明（导出器按此展开具体列，02 册 §4.1）。 */
struct ResultColumnSpec
{
    bool include_projections = false;       // projection_<in_/out_列名> 分块
    bool include_slacks = false;            // slack_<in_/out_列名> 分块
    bool include_lambdas = false;           // lambda_<DMU 名>（全 DMU 展开，稀疏留空）
    bool include_multipliers = false;       // mult_v_<投入列> / mult_u_<产出列>
    std::vector<std::string> extra_columns; // 模型特有固定列（值取 DmuResult.annotations）
};

/** @brief 模型描述符：注册表的最小注册单元。 */
struct ModelDescriptor
{
    std::string model_id;               // 如 "ccr"、"sbm"
    std::string display_name;           // 人读名（CLI 帮助/摘要用）
    std::set<Orientation> orientations; // 支持的定向集合
    std::set<Rts> rts;                  // 支持的规模报酬集合
    bool uses_rts = true;               // false：--rts 显式传入即非法（如 scale_efficiency）
    bool supports_super = false;        // 是否支持超效率参考集（exclude_self）
    ResultColumnSpec columns;           // 结果列集声明
    SolveFunction solve;                // 类型擦除的求解函数（工厂 lambda）
};

/** @brief 全部合法定向/规模报酬集合（注册校验与 CLI 帮助共用）。 */
[[nodiscard]] const std::set<Orientation> &all_orientations();
[[nodiscard]] const std::set<Rts> &all_rts();
} // namespace opensuperdea::registry
