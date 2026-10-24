# fare1994_panel：Färe 1994 面板 Malmquist 小例（L2；FR-TFP-001）
#
# 数据：3 DMU × 2 期 × (1 投入 + 1 产出) 长表。
# 期望值：Färe-Prosser-... 1994 相邻 Malmquist（CRS 产出导向）——
#   R productivity 1.1.2 malm() 输出（malmquist 列）为基准：
#   A: 1.6, B: 1.32, C: 1.219048；EC/TC 与 MI 满足 PR-05 恒等式（MI = EC × TC）。
#   手算复核：A 的 4 个距离函数 φ = (3, 1.875, 3.657, 2.2857) → MI = √(3/1.875 × 3.657/2.2857) = 1.6。
# 容差：绝对 1e-6（L2 标准）。
