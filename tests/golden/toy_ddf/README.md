# toy_ddf：DDF 自定义方向算例（FR-DDF-003 + FR-IO-002 联测）
#
# 数据：4 DMU × (2 投入 + 1 产出)，同 toy_cross 的 in/out 值；
#       di_labor/di_capital/do_revenue 为逐 DMU 自定义方向列（FR-IO-002）。
# 用途：ddf --direction custom 的 L1 对拍（deaR model_dir，maxslack=FALSE 取纯 β）
#       与 V-006 方向列数校验联测。
# 出处：本仓库构造算例（数据同 tests/golden/toy_cross，方向为测试设计值）。
# 容差：P-010 双阈值（绝对 1e-6 或相对 1e-9）。
