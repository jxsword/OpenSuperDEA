/**
 * @file window_analysis.h
 * @brief 窗口分析（FR-WIN-001；06 册 §9）：面板滑窗 + 底层模型复用。
 * @details 对每窗口 [t-w+1, t]（首尾期窗口数按定义减少）合并各期截面为参考集，
 *          对窗内全部 (dmu, period) 单元求解底层模型（径向/SBM 等任意已注册
 *          截面模型）。结果行 = (dmu, period) 单元，行名 "dmu@period"。
 */
#pragma once

#include <string>

#include "core/dataset/dataset.h"
#include "core/registry/model_runner.h"

namespace opensuperdea::window
{
/**
 * @brief 运行窗口分析。
 * @param panel 面板数据
 * @param window_width 窗口宽度 w（1 ≤ w ≤ 期次数；非法抛 EXIT_ARG）
 * @param base_model_id 底层截面模型 id（如 ccr/sbm）
 * @param options 定向/RTS/模型选项（透传底层模型）
 * @param config 元信息
 * @return 结果：每行一个 (dmu, period) 单元（score 为该窗口内效率值）
 */
[[nodiscard]] result::ModelResult run_window_analysis(const dataset::PanelData &panel,
                                                      std::size_t window_width,
                                                      const std::string &base_model_id,
                                                      const registry::SolveOptions &options,
                                                      const registry::RunConfig &config);
} // namespace opensuperdea::window
