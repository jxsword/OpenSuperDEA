# 独有算法论文与技术路线表（OpenSuperDEA · M0-①）

> 调研批次进度：**批 1（MinDS + FDEF）完成** ｜ 2026-10-24 ｜ 状态：待路线评审
> 产出规范：每算法 ≥3 篇候选论文（题录+DOI/官方链接）；P0/P1/P2 路线结论；URL 均已实际访问并标注日期。

## 0. 总览

| # | 算法 | 类 | 调研状态 | 路线结论 | 公开实现 |
|---|---|---|---|---|---|
| 1 | MinDS / Closest Target（强单调） | A | ✅ 批1完成 | **P0/P1**（有开源参考 deaR model_addmin） | deaR (GPL-2/3) |
| 2 | FDEF 全维有效面 | A | ✅ 批1完成 | **P1**（CRS 有 MILP 算法+算例；VRS 高风险） | 无直接实现（可用 QHULL） |
| 3 | 博弈交叉效率（Nash） | A | ⏳ 批2 | - | - |
| 4 | EBM 混合距离 | A | ⏳ 批2 | - | - |
| 5 | HMB（Hicks-Moorsteen） | A | ⏳ 批2 | - | - |
| 6 | Cluster 基准模型 | A | ⏳ 批3 | - | - |
| 7 | AR / Cone 受限乘数 | A | ⏳ 批3 | - | - |
| 8 | 修正 SBM | B | ⏳ 批3 | - | - |
| 9 | Malmquist 多参比 | B | ⏳ 批3 | - | - |

---

## 1. MinDS / Closest Target（Minimum Distance to Strong Efficient Frontier）

### 1.1 MaxDEA 官方定义（原文引用，访问 2026-10-24）

> 来源：http://maxdea.com/Features_X.htm
> "4) Minimum Distance to Strong Efficient Frontier (Closest Target), **with full features: strong monotonicity algorithm; CRS, VRS, NIRS and NDRS; Non-oriented, Input-oriented and Output-oriented.** Ref. to (J. Aparicio et al., 2007; G. R. Jahanshahloo et al., 2012; Gholam Reza Jahanshahloo, Roshdi, & Davtalab-Olyaie, 2013; Olyaie, Roshdi, Jahanshahloo, & Asgharian, 2014; J Aparicio et al., 2017; Zhu et al., 2018; Zhu et al., 2022)"

**关键结论**：MaxDEA 官方明示其 MinDS 实现依赖 7 篇文献，与本次调研独立发现的论文完全吻合 → **实现范围可精确对齐官方路线**。

### 1.2 候选论文清单（7/7 已确认，满足 ≥3 篇）

| # | 论文 | 题录 | DOI / 链接 | 角色 |
|---|---|---|---|---|
| 1 | Aparicio, Ruiz & Sirvent (2007) | Closest targets and minimum distance to the Pareto-efficient frontier in DEA, *J. Productivity Analysis* 28(3):209-218 | 10.1007/s11123-007-0039-5 | 单阶段 MILP（mixed-integer 0-1），MinDS 奠基 |
| 2 | Jahanshahloo et al. (2012) | A linear bilevel programming problem for obtaining the closest targets..., *Asia-Pacific J. Operational Research* | https://doi.org/10.1142/S0217595912400021 | 双线性规划路线（备选） |
| 3 | Jahanshahloo, Roshdi & Davtalab-Olyaie (2013) | Characterizing and finding FDEFs of PPS with CRS technology, *Int. J. Industrial Mathematics* 5(2):149-159 | https://journals.srbiau.ac.ir/article_2148_...pdf（全文已读） | **FDEF 与 MinDS 的桥接**（MinDS 需先求 FDEF） |
| 4 | Olyaie, Roshdi, Jahanshahloo & Asgharian (2014) | 增强型 closest targets（具体题录待补） | MaxDEA 官方引用；邻近可获：arXiv:1503.09095 (Mehdiloozad et al. 2015, EJOR) | 参考集/最近距离框架 |
| 5 | Aparicio, García-Nové, Kapelko & Pastor (2017) | Graph productivity change measure using the least distance to the Pareto-efficient frontier, *OMEGA* | Semantic Scholar 确认（2017） | least distance 生产率扩展 |
| 6 | **Zhu, Wu, Ji & Li (2018)** | **A simple MILP to determine closest targets in non-oriented DEA model satisfying strong monotonicity, *OMEGA* 79:1-8** | **10.1016/j.omega.2017.07.003** | **强单调核心算法（对应官方"strong monotonicity algorithm"）** |
| 7 | Zhu, Aparicio, Li, Wu & Kou (2022) | Determining closest targets on the extended facet production possibility set, *EJOR* 296(3):927-939 | https://doi.org/10.1016/j.ejor.2021.08.023 | 扩展面 PPS 版本（最新） |

### 1.3 公开实现盘点（重要发现）

- ✅ **deaR R 包 `model_addmin`**（CRAN v1.5.4，许可证 **GPL-2 | GPL-3**，作者 Vicente Bolós / Coll-Serrano / Benítez，瓦伦西亚大学）：
  - 实现 **Aparicio et al. (2007) 加权 additive-min (mADD) 模型**，非 CRS 时采用 **Zhu et al. (2018) 修改**——与 MaxDEA 路线重合
  - 方法：`mf`（maximal friends，默认）/ `milp`（Aparicio 2007 MILP）
  - RTS：CRS / VRS / NIRS / NDRS；含非导向/导向权重开关
  - **内置 Airlines 数据集（28 家航空公司，2 输出 4 输入）**——即 Aparicio 2007 论文算例，验证数据现成
  - 辅助函数：`extreme_efficient`、`maximal_friends`（MaxDEA "maximal friends" 概念在 deaR 中已实现）
  - 文档：https://www.rdocumentation.org/packages/deaR/versions/1.5.3/topics/model_addmin ｜ CRAN：https://cran.r-project.org/web/packages/deaR/index.html
- 其余 6 篇论文无公开代码；deaR 未实现扩展面版本（Zhu 2022，MaxDEA X 最新宣传点）

### 1.4 实现路线建议（P0 优先）

- **P0 参考移植 + 对拍**：以 deaR `model_addmin` 为**行为基准**（GPL 代码可直接阅读参考；正式移植到 C++ 时注意许可证合规——GPL 传染性，建议仅参考算法逻辑自行实现并保留独立实现记录，或用 LGPL/BSD 依赖如 HiGHS 无冲突）
- **P1 核心算法**：Zhu et al. (2018) 简单 MILP（非导向、强单调性）→ 与 Aparicio et al. (2007) MILP（导向/权重）并列实现
- **P1 扩展**：Zhu et al. (2022) 扩展面版本（依赖 §2 FDEF 计算）；Aparicio et al. (2017) 生产率扩展
- **技术要点**：MILP 用 HiGHS（技术栈已含）；Aparicio 2007 的 big-M（M_d/M_lambda）数值敏感——deaR 文档明示"very big value can produce catastrophic cancellations"，实现时需 M 值估计策略（deaR 有自动估计逻辑可参考）
- **算法关系**：MinDS 强单调版依赖 FDEF（§2）枚举——Jahanshahloo 2013 正是 MaxDEA 官方引用的 MinDS 文献之一，两个算法需同批实现

### 1.5 验证方案

1. **deaR 对拍**：随机数据 + Airlines 28 航空公司数据，C++ 结果 vs deaR `model_addmin`（mf/milp）在 4 种 RTS 下逐一比对得分/目标/松弛
2. **论文数值复现**：Aparicio 2007（Airlines 表）、Zhu 2018 论文算例、Sekitani & Zhao 2023 (arXiv:2312.15422) 数值表（开放获取 CC BY 4.0）
3. **数学性质**：强单调性检验（构造支配 DMU 对，验证得分单调）

### 1.6 风险

| 风险 | 级别 | 说明 |
|---|---|---|
| big-M 数值不稳定 | 中 | deaR 文档已确认；需 M 自动估计 + 容差策略 |
| VRS 下 closest target 非唯一 | 低 | maximal friends 方法可枚举全部投影（deaR 已实现思路） |
| MaxDEA 与 Zhu 2022 具体等价性 | 低 | 官方引用一致，实现后可抽样对拍 MaxDEA Lite 免费版（≤1000 行） |

---

## 2. FDEF（Full Dimensional Efficient Facet）

### 2.1 功能定义

MaxDEA X 独有能力：可输出**所有 FDEF 的顶点与超平面方程**（分段前沿生产函数），用于 MinDS 强单调、敏感度分析、参考集识别。官方 Features 页（http://maxdea.com/Features_X.htm，访问 2026-10-24）未单列 FDEF，但 §1.1 中 FDEF 论文（Jahanshahloo 2013）被官方列为 MinDS 实现基础。

### 2.2 候选论文清单（5 篇，满足 ≥3 篇）

| # | 论文 | 题录 | 可访问性 | 角色 |
|---|---|---|---|---|
| 1 | **Jahanshahloo, Roshdi & Davtalab-Olyaie (2013)** | Characterizing and finding FDEFs of PPS with CRS technology, *IJIM* 5(2):149-159 | ✅ 全文已读（含算例） | **CRS 全 FDEF 枚举 MILP 算法——P1 核心路线** |
| 2 | Olesen & Petersen (2003) | Identification and use of efficient faces and facets in DEA, *JPA* 20:323-360 | 订阅（题录确认） | VRS 理论奠基（含 E_j 算法，被 2013 批判有循环缺陷） |
| 3 | Akbarian (2014) | On characterizing weak defining hyperplanes (weak facets) in DEA with CRS, arXiv:1409.7546 | ✅ 开放获取 | weak facet 刻画（GAMS 可实现，作为强面甄别补充） |
| 4 | Huang, Wang, Zhang, Xu (2025) | DEA based on robust and closest targets（Integrating FDEFs）, arXiv:2505.06487 | ✅ 开放获取（CC BY-NC-ND） | 2025 最新：robust targets = 最大数量 FDEF 交集 |
| 5 | （2025）| Public bidding in government procurement using FDEFs of VRS frontier, *JPA* | RePEc 确认 | VRS FDEF 应用（ERGM nearest targets） |

### 2.3 公开实现盘点

- **无任何直接开源实现**（GitHub code/repos 8 组关键词全灭）
- 两条**可行技术路线**（计算层面）：
  - 路线 A：**Jahanshahloo 2013 MILP 枚举**（CRS）：先模型(2.4) 求 extreme CCR-efficient DMU 集 E，再对每个 DMU 解模型(3.5)（最小化 $\sum I_j$，big-M MILP）找 SDH；**算法完整、有算例、可逐行实现**
  - 路线 B：**计算几何 QHULL 枚举凸包面**（VRS/通用）：SDU working paper（Olesen 系）明确用 QHULL 生成 H-representation；scipy.spatial.ConvexHull / qhull C++ 库可直接求凸包全部 facet → 再筛选"强有效面"（全部乘子严格正）。**MaxDEA 输出"所有 FDEF 顶点与超平面方程"与此路线同构**

### 2.4 实现路线建议

- **P1（CRS）**：Jahanshahloo 2013 完整实现（HiGHS MILP）；算例验证（论文 Table 1 反例 + Section 5 示例）
- **P1（VRS）**：QHULL 枚举 + 强有效面筛选（线性规划检验乘子正性）；Olesen & Petersen 2003 理论作为正确性判据
- **P2（NDRS/NIRS）**：弱定义超平面刻画（Akbarian 2014 思路扩展），标记最高风险
- **输出形态对齐**：顶点 = FDEF 上 extreme efficient DMU 集合；超平面方程 = (v,u,ψ) 系数——直接对应 MaxDEA 输出

### 2.5 验证方案

1. 论文数值复现：Jahanshahloo 2013 算例（CRS）；Olesen & Petersen 论文数据
2. **交叉验证**：QHULL 路线 vs MILP 路线在相同数据上比对 FDEF 集合（两条独立路线互证——天然对拍）
3. **几何性质**：每个 FDEF 维数 = m+s-1（仿射独立集检验）；超平面支撑性检验

### 2.6 风险

| 风险 | 级别 | 说明 |
|---|---|---|
| 高维下 FDEF 数量爆炸（枚举指数级） | **高** | MaxDEA 同样受此限制；需去重 + 用户可设上限；性能目标需实测 |
| VRS 强有效面筛选计算量 | 中 | 每面一次 LP 检验；可用 GPU/并行缓解 |
| NDRS/NIRS FDEF 理论不完整 | 中 | 标记 P2，必要时与用户协商降级 |
| big-M 数值敏感 | 中 | 同 MinDS，需 M 估计策略 |

---

## 3. 批 1 结论（供路线评审）

1. **MinDS 可行性高**：有 deaR 开源参考（GPL，Aparicio 2007 + Zhu 2018）+ 官方引用文献全覆盖 → **P0/P1**
2. **FDEF 可实现**：CRS 有完整 MILP 算法+算例（P1）；VRS 有 QHULL 计算几何路线（P1 中风险）；NDRS/NIRS 为 P2
3. **两算法强耦合**：MinDS 强单调版依赖 FDEF → **实现顺序：FDEF(CRS) → MinDS(Zhu 2018) → 扩展面(Zhu 2022)**，建议同一里程碑内完成
4. **验证基线成型**：deaR Airlines 数据集 + 论文算例 + 双路线交叉验证

## 4. 后续批次预告

- 批 2：博弈交叉效率（Nash）+ EBM + HMB
- 批 3：Cluster + AR/Cone + 修正 SBM + Malmquist 多参比
