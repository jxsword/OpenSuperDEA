# ccr_classic：CCR 1978 经典小例（L2；FR-RAD-001）
#
# 数据：4 DMU × (2 投入 + 1 产出)（同 toy_cross）。
# 期望值：手算推导（CCR 1978 包络形式，CRS 投入导向）：
#   A: λC=1/2 → θ = max(2/2·0.5?) 推导见下；B: 3/4；C: 1（前沿）；D: 14/15。
#   A 的基准：λC = 1/2 时 Xλ = (2, 1.5) ≥ ... 精确推导：min θ s.t. Xλ ≤ θx0,
#   Yλ ≥ y0 → A = 1/2（对偶乘子 v = (1/4, 1/2), u = 1/6·? 手算记录见 tests/unit/m2/radial_model_test.cpp）
#   双重验证：deaR 1.5.4 model_basic 同值（tests/parity/ 批）。
# 容差：绝对 1e-6（L2 标准）。
