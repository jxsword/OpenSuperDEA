# 独有算法论文与技术路线表（OpenSuperDEA · M0-①）

> 版本：**v2.0（第 1 轮复审修正版）**｜ 2026-10-24 ｜ 状态：待第 2 轮复审
> 调研进度：批 1 + 批 2 + 批 3 完成；**第 1 轮复审新增第 10 项（Weak EF）并修正 8 处题录**
> 产出规范：每算法 ≥3 篇候选论文；P0/P1/P2 路线结论；URL 均已实际访问并标注日期。

## 0. 总览（10/10 项）

> **口径说明**：在线平台调研报告的"8 项独有能力全缺失" = 算法 7 项 + 性能 1 项（万级 DMU 极速求解）；本表按调研大纲 A/B 分类 = 9 项算法；**第 1 轮复审对照官方清单新增"最小距离弱有效前沿"→ 现为 10 项算法**。性能目标（10 万/100 万 DMU）不属算法调研，归 M3/M4 性能里程碑。

| # | 算法 | 类 | 状态 | 路线结论 | 公开实现 |
|---|---|---|---|---|---|
| 1 | MinDS / Closest Target（强单调） | A | ✅ | **P0/P1** | deaR model_addmin (GPL) |
| 2 | FDEF 全维有效面 | A | ✅ | **P1**（CRS MILP；VRS QHULL；NDRS/NIRS=P2） | 无（QHULL 可作基础） |
| 3 | **最小距离弱有效前沿 Weak EF** | A | **新增** | **P1**（两阶段 LP；弱投影不唯一性=P2） | 无 |
| 4 | 博弈交叉效率（Nash） | A | ✅ | **P0/P1** | DEAPack (GPL-3, HiGHS) |
| 5 | EBM 混合距离 | A | ✅ | **P1**（GRIPS 公式完整） | DEAPack declared |
| 6 | HMB（Hicks-Moorsteen） | A | ✅ | **P0/P1** | R productivity + DEAPack |
| 7 | Cluster 基准模型（8 基准） | A | ✅ | **P1**（= Context-dependent DEA 组合） | 无直接实现 |
| 8 | AR / Cone 受限乘数 | A | ✅ | **P0**（双开源参考） | **DEAPack cone_ratio + PyDEA AR** |
| 9 | 修正 SBM | B | ✅ | **P1**（负数据两阶段法） | 无直接实现 |
| 10 | Malmquist 多参比 | B | ✅ | **P0/P1** | DEAPack + deaR + R productivity |

---

## 1. MinDS / Closest Target（Minimum Distance to Strong Efficient Frontier）

### 1.1 MaxDEA 官方定义（原文引用，访问 2026-10-24）

> 来源：http://maxdea.com/Features_X.htm
> "4) Minimum Distance to Strong Efficient Frontier (Closest Target), with full features: strong monotonicity algorithm; CRS, VRS, NIRS and NDRS; Non-oriented, Input-oriented and Output-oriented. Ref. to (J. Aparicio et al., 2007; G. R. Jahanshahloo et al., 2012; Gholam Reza Jahanshahloo, Roshdi, & Davtalab-Olyaie, 2013; Olyaie, Roshdi, Jahanshahloo, & Asgharian, 2014; J Aparicio et al., 2017; Zhu et al., 2018; Zhu et al., 2022)"

### 1.2 候选论文（8/8，官方 7 篇中 6 篇题录锁定 + 2 篇机制补充）

| # | 论文 | 题录 | DOI/链接 |
|---|---|---|---|
| 1 | Aparicio, Ruiz & Sirvent (2007) | Closest targets..., *JPA* 28:209-218 | 10.1007/s11123-007-0039-5 |
| 2 | Jahanshahloo, Vakili & Zarepisheh (2012) | Linear bilevel programming for closest targets, *APJOR* | doi.org/10.1142/S0217595912400021 |
| 3 | Jahanshahloo, Roshdi & Davtalab-Olyaie (2013) | Characterizing FDEFs of PPS with CRS, *IJIM* 5(2):149-159 | journals.srbiau.ac.ir（全文已读） |
| 4 | Olyaie, Roshdi, Jahanshahloo & Asgharian (2014) | **题录未锁定**（官方引用格式如上；检索两轮未命中） | 见 §1.4 替代参考 |
| 5 | Aparicio, García-Nové et al. (2017) | Graph productivity change (least distance), *OMEGA* | Semantic Scholar 确认 |
| 6 | **Zhu, Wu, Ji & Li (2018)** | **Simple MILP ... strong monotonicity, *OMEGA* 79:1-8** | **10.1016/j.omega.2017.07.003** |
| 7 | Zhu, Aparicio, Li, Wu & Kou (2022) | Closest targets on extended facet PPS, *EJOR* 296:927-939 | 10.1016/j.ejor.2021.08.023 |
| 8 | **Aparicio & Pastor (2013)** | **Closest targets and strong monotonicity on the strongly efficient frontier, *Omega*** | S0305048313000972（强单调机制：Hölder 距离为何失效 + 输出导向解） |

### 1.3 公开实现 / 路线 / 验证

- ✅ **deaR `model_addmin`**（CRAN 1.5.4，GPL-2|3）：Aparicio 2007 + Zhu 2018；mf/milp 两法；**rts = c("crs","vrs","nirs","ndrs") 源码确认 4 种 RTS**；内置 Airlines 28 算例
- **路线（已确认）**：P0 对拍 + P1 自行实现（Zhu 2018 → Zhu 2022 扩展面）；实现顺序 FDEF → MinDS → 扩展面；MILP 用 HiGHS；big-M 需自动估计
- **验证**：deaR 对拍（4 RTS）、Airlines 数据、Zhu 2018 算例、Sekitani & Zhao 2023（arXiv:2312.15422, CC BY 4.0）数值表、强单调性检验

### 1.4 补充参考（替代 Olyaie 2014 的题录缺口）

- **Mehdiloozad & Ahmadi (2015)** "Identification of reference set and measurement of RTS in DEA: A least distance based framework", **arXiv:1503.09095**（开放）——least distance 参考集识别 + Closest RTS（CRTS），独立论文，实现参考价值高
- **Mehdiloo & Podinovski** "Strong, weak and Farrell efficient frontiers of technologies satisfying different production assumptions", *EJOR*（figshare 开放 PDF）——强/弱前沿概念关系，Weak EF 与 MinDS 的共同理论基础

---

## 2. FDEF（Full Dimensional Efficient Facet）

### 2.1 定义
输出**所有 FDEF 顶点与超平面方程**（分段前沿生产函数）；MinDS 强单调的基础。

### 2.2 候选论文（7 篇）

| # | 论文 | 题录 | 可访问性 | 角色 |
|---|---|---|---|---|
| 1 | **Jahanshahloo, Roshdi & Davtalab-Olyaie (2013)** | FDEFs of PPS with CRS, *IJIM* 5(2):149-159 | ✅ 全文（含算例） | **CRS 全 FDEF 枚举 MILP——P1 核心** |
| 2 | Olesen & Petersen (2003) | Identification and use of efficient faces and facets, *JPA* 20:323-360 | 订阅 | VRS 奠基（E_j 算法有缺陷） |
| 3 | Akbarian (2014) | Weak defining hyperplanes in DEA with CRS, arXiv:1409.7546 | ✅ 开放 | weak facet 刻画 |
| 4 | Huang et al. (2025) | DEA based on robust and closest targets, arXiv:2505.06487 | ✅ CC BY-NC-ND | robust targets = FDEF 交集 |
| 5 | **Lins, do Nascimento, Dulá, Torres & Lins (2025)** | Public bidding ... ranking proposals using FDEFs of DEA frontier, *JPA* | ✅ SpringerLink 开放 | **VRS FDEF + ERGM 最近目标两阶段法（P1 路线 A 应用佐证）** |
| 6 | **Rafati-Maleki et al. (2018)** | Finding the strong defining hyperplanes of PPS with CRS using the linear independent vectors, *Cogent Math & Stat* | ✅ Taylor & Francis | CRS 强定义超平面替代方法 |
| 7 | （2025 工作论文） | 用 MOO 计算 VRS 高效面（dual MOO, CORE 开放） | ✅ CORE | 计算几何替代路线佐证 |

### 2.3 路线（已确认）
P1：路线 A = Jahanshahloo 2013 MILP（CRS）；路线 B = QHULL 计算几何（VRS，SDU WP + Lins 2025 佐证）双路线互证；NDRS/NIRS = P2 最高风险。

---

## 3. 最小距离弱有效前沿（Minimum Distance to Weak Efficient Frontier）【新增】

### 3.1 官方定义
> "3) Minimum Distance to Weak Efficient Frontier (Charnes, Rousse, and Semple 1996)"（Features_X.htm；官方拼写 "Rousse" 实为 Rousseau）

### 3.2 候选论文（4 篇）

| # | 论文 | 题录 | 可访问性 | 角色 |
|---|---|---|---|---|
| 1 | Charnes, Cooper & Rhodes (1978) | Measuring the efficiency of DMUs, *EJOR* 2:429-444 | 订阅 | 弱效率概念定义 |
| 2 | Charnes, Rousseau & Semple | Sensitivity and stability of efficiency classifications in DEA, *JPA* | ✅ SpringerLink 10.1007/BF00158473 | 官方引用文献（110 引）；卷期/年份待核（官方标 1996） |
| 3 | **Vakili, Amirmoshiri & Mirnia** | Novel models for obtaining the closest weak and strong efficient projections in DEA | Semantic Scholar（题录待补全） | **closest weak projection 直接算法（QCP）** |
| 4 | Mehdiloo & Podinovski | Strong, weak and Farrell efficient frontiers..., *EJOR* | ✅ figshare 开放 | 弱前沿概念边界（理论基础） |

### 3.3 实现/路线
- **P1 自行实现**：弱有效投影两阶段法（先求弱效率投影 → 再最小化距离范数）；l1/l∞/l2 范数选择；**风险 P2**：弱前沿投影点不唯一（需强单调/唯一性处理，可借用 MinDS 的 strong monotonicity 成果）
- **公开实现**：GitHub 无；deaR/DEAPack/PyDEA 均无（DEAPack weak_disposal.py 是"弱可处置性"环境模型，≠ 弱前沿最近距离，勿混淆）
- **验证**：Vakili 等 30 欧洲机场数据（论文算例）、CCR 经典算例；与 MinDS（§1）实现共用投影内核

---

## 4. 博弈交叉效率（Game Cross Efficiency, Nash）

### 4.1 官方定义
> "5) Game Cross Efficiency model: Nash Equilibrium model (Liang, et al 2008; Wu, et al 2009)"（Features_X.htm，访问 2026-10-24）

### 4.2 候选论文（4/4，全部题录锁定）

| # | 论文 | 题录 | DOI/链接 |
|---|---|---|---|
| 1 | **Liang, Wu, Cook & Zhu (2008)** | The DEA Game Cross-Efficiency Model and Its Nash Equilibrium, *Operations Research* 56(5):1278-1288 | **10.1287/opre.1070.0487** |
| 2 | Wu, Liang & Chen (2009) | DEA game cross-efficiency to Olympic rankings, *OMEGA* 37:909-918 | 10.1016/j.omega.2008.07.007（VRS） |
| 3 | **Wu, Liang & Zha (2009)** | Preference voting and ranking using DEA game cross efficiency, *JORSJ* 52(2):105-111 | ✅ **ORSJ 开放全文** orsj.org/wp-content/or-archives50/pdf/e_mag/52-2-105-111.pdf |
| 4 | Liang, Wu, Cook & Zhu (2008b) | Two-stage game approach, *EJOR* 185:418-429 | 10.1016/j.ejor.2006.12.025 |

### 4.3 实现/路线（已确认）
✅ **DEAPack game_cross_efficiency.py**（GPL-3，HiGHS；specs 注册 liang_wu_cook_zhu_2008）→ P0 参考 + P1 自行实现（Liang 2008 迭代算法 = Nash 均衡；Wu 2009 VRS）。验证：DEAPack 对拍 + Liang 2008 算例 + 奥运奖牌数据 + ORSJ 论文算例 + Nash 性质检验。

---

## 5. EBM（Epsilon-Based Measure）

### 5.1 官方定义
> "7) Hybrid Distance (Radial and SBM Measure): (EBM, Epsilon-based Measure, Tone and Tsutsui 2010)"（Features_X.htm）

### 5.2 候选论文（3/3，题录全部锁定）

| # | 论文 | 题录 | 可访问性 |
|---|---|---|---|
| 1 | **Tone & Tsutsui (2010)** | An epsilon-based measure..., *EJOR* 207(3):1554-1563 | 10.1016/j.ejor.2010.07.014 |
| 2 | Tone & Tsutsui（工作论文版） | EBM revisited（GRIPS DP09-21） | ✅ grips.repo.nii.ac.jp/record/1021/files/DP09-21.pdf（**完整公式+算例**） |
| 3 | MDPI 应用系列（2021/2022） | EBM 公式复述（含 ε/w 计算） | ✅ mdpi.com/1996-1073/14/15/4498 |

### 5.3 实现/路线（已确认）
P1 自行实现：归一化 → affinity index S=1-2D（ln 偏差）→ 亲和矩阵 → **Perron-Frobenius 最大特征值 ρ + 非负特征向量 w**（Eigen 谱分解）→ ε=(m-ρ)/(m-1)、w=w/Σw → EBM-I-C/O-C LP（HiGHS）。**验证算例现成**：ρ=1.471, ε=0.529。deaR 无 EBM（源码确认）；DEAPack declared 状态待转正后可对拍。

---

## 6. HMB（Hicks-Moorsteen TFP Index）

### 6.1 官方定义
> "TFP Index: Malmquist Index and Hicks-Moorsteen Index (also called HMB Index)"（Features_X.htm）；官方含 TFP 分解：EC/TC/SEC/biased TC（TC=OBTC×IBTC×MATC, Färe et al. 1997）

### 6.2 候选论文（5/5，题录全部锁定）

| # | 论文 | 题录 | DOI/链接 |
|---|---|---|---|
| 1 | **Bjurek (1996)** | The Malmquist Total Factor Productivity Index, *Scand. J. Economics* 98:303-313 | 10.2307/3440855 |
| 2 | O'Donnell (2012) | An aggregate quantity framework..., *JPA* 38:255-272 | 10.1007/s11123-011-0256-9 |
| 3 | **Kerstens & Van de Woestyne (2014)** | Comparing Malmquist and Hicks–Moorsteen productivity indices: unbalanced vs. balanced panel, *EJOR* 233(3):749-758 | ✅ kkerstens.github.io 开放全文 |
| 4 | Briec & Kerstens (2011) | The Hicks–Moorsteen productivity index satisfies the determinateness axiom, *Manchester School* | kkerstens.github.io 开放 |
| 5 | Mizobuchi (2017) | Productivity indexes under Hicks neutral technical change | 10.1016/j.ejor.2016.10.043 |

### 6.3 实现/路线（已确认）
✅ **R productivity**（CRAN 1.1.2，GPL-3，Färe-Primont/HMB/Fisher/Malmquist 完整族）+ ✅ **DEAPack hicks_moorsteen.py**（Bjurek，HiGHS）双参考。P0 对拍 + P1 自行实现；与 Malmquist 共享距离函数内核，M5 内合并实现。

---

## 7. Cluster 基准模型（8 种基准）

### 7.1 官方定义
> "**Cluster model**: a) Self-benchmarking b) Cross-benchmarking c) Downward-benchmarking d) Upward-benchmarking e) Lower-adjacent-benchmarking f) Upper-adjacent-benchmarking g) Window-benchmarking h) Fixed-benchmarking"（Features_X.htm）；中文页标注"群组参比模型，广义DEA模型"

**性质判定**：非单一新算法，而是 **Context-dependent DEA（分层基准）+ 聚类参比 + 窗口/固定参比的组合体系**：
- Downward/Upward/Lower-adjacent/Upper-adjacent = Context-dependent DEA 的 **progress / attractiveness** 分层评估（Seiford & Zhu 2003）
- Self/Cross = 群内/群间参考集选择（聚类 + 参比组合）
- Window/Fixed = 窗口分析 / 固定基准参比（技术集概念见 Tulkens & Vanden Eeckaut 1995）

### 7.2 候选论文（5 篇）

| # | 论文 | 题录 | 可访问性 |
|---|---|---|---|
| 1 | **Seiford & Zhu (2003)** | Context-dependent DEA—Measuring attractiveness and progress, *OMEGA* 31:397-408 | ✅ **DEAFrontier 开放全文**（deafrontier.net/papers/OMEGAContext.pdf） |
| 2 | Zhu (2001) | Super-efficiency and DEA sensitivity analysis, *EJOR* 129:443-455 | 10.1016/S0377-2217(00)00157-0（分层基础） |
| 3 | Xu & Ouenniche (2011/2012) | SBM-based context-dependent DEA | 爱丁堡开放 PDF |
| 4 | **Tulkens & Vanden Eeckaut (1995)** | Non-parametric efficiency, progress and regress measures for panel data, *EJOR* 80:474-499 | ✅ ScienceDirect | **contemporaneous/sequential/intertemporal 技术集 + window + benchmark 概念（Fixed-benchmarking 理论基础）** |
| 5 | 应用类（JARIE 等） | context-dependent + 聚类 | 佐证 |

### 7.3 实现/路线
- **P1 自行实现**：分层算法（level 剥离：反复求 BCC 效率剔除 efficient DMU 得 L1..Lh 层）→ attractiveness（对更优层）/ progress（对更差层）评估模型；8 种参比 = 参考集组合规则（参数化）
- **技术要点**：核心是参考集（reference set）选择逻辑，非新数学；与窗口分析（Malmquist 窗口参比）复用
- **验证（修正）**：Seiford & Zhu 2003 算例（打印机数据，Doyle & Green 原始数据）+ Tulkens-VandenEeckaut 面板算例 + 分层结构性质检验。**MaxDEA Lite 免费版仅限基础径向/SBM/成本/收益模型，不含 Cluster（官方 Download.htm 确认），不可作为 Cluster 对拍基线**
- 公开实现：无直接开源（GitHub/CRAN 均无）；deaR 无 context-dependent（源码确认）

---

## 8. AR / Cone 受限乘数

### 8.1 官方定义
> "9) Restricted multiplier model (Cone Ratio DEA, Restricted Cone DEA, Assurance Region DEA)"（Features_X.htm）

### 8.2 候选论文（4/4）

| # | 论文 | 题录 | 角色 |
|---|---|---|---|
| 1 | Thompson, Singleton, Thrall & Smith (1986) | Comparative site evaluations...（AR 起源） | AR 定义 |
| 2 | Thompson et al. (1990) | The role of multiplier bounds...（Kansas farming） | AR 应用 |
| 3 | **Charnes, Cooper, Wei & Huang (1989/1990)** | Polyhedral cone-ratio DEA models（banks） | **Cone Ratio 定义** |
| 4 | Wong & Beasley (1990) | Weight restriction by bounds on proportions | 权重限制扩展 |

### 8.3 公开实现/路线（升级为 P0）
- ✅ **DEAPack**：`valuation/weight_restriction/cone_ratio/polyhedral_crs.py`（**Charnes et al. 1990 锥比实现**）+ `source_protocols/assurance_region.md` + 注册表 `charnes_etal_1990.json`；普通 `cross_efficiency.py` 亦在
- ✅ **PyDEA**（MIT）：assurance region 已实现（教程 06_assurance_region.py）
- deaR 无 AR/Cone（源码确认）
- **P0（双开源参考）**：DEAPack cone_ratio + PyDEA AR 对拍；自行实现——AR = 乘子空间界限约束（α ≤ u_r/u_s ≤ β）；Cone = 乘子限定在生成锥（LP 可直接实现，HiGHS）
- **验证**：DEAPack/PyDEA 对拍 + Thompson Kansas farming 算例 + Charnes banks 算例

---

## 9. 修正 SBM（Modified SBM）

### 9.1 官方定义
> "2) Modified SBM (Sharp et al 2007) 3) Modified SBM (Lin et al 2019)"（Features_X.htm）

### 9.2 候选论文（4/4，题录全部锁定）

| # | 论文 | 题录 | DOI/链接 |
|---|---|---|---|
| 1 | **Sharp, Liu & Meng (2007)** | A modified slacks-based measure model ... with 'natural' negative outputs and inputs, *JORS* | Emrouznejad 综述确认 |
| 2 | **Lin, Yang & Huang (2019)** | A modified slacks-based super-efficiency measure in the presence of negative data, *Computers & Industrial Engineering* 135:39-52（2019-09） | **10.1016/j.cie.2019.05.030**（ORCID 验证） |
| 3 | Matin & Ghahfarokhi (2015) | Two-phase modified slacks-based measure for negative data | 佐证 |
| 4 | Wei (2019) | A modified slacks-based ranking method handling negative data, *Expert Systems* 36 | 10.1111/exsy.12329 |

**核心**：处理负数据（natural negative outputs/inputs）的 SBM 变体——两阶段法（Sharp 2007）+ 超效率版（Lin-Yang-Huang 2019 = MaxDEA 引用的 "Lin et al 2019"）。**Lin 2019 关键性质（实现验证要点）**：VRS 下 Super-SBM 可行、允许负值、**单调、单位不变、平移不变**、强 Pareto 有效投影。

### 9.3 实现/路线
- **P1 自行实现**：负数据变换（保持单调性）+ 两阶段 SBM + 超效率修正；无直接开源（GitHub 无）
- **验证**：Sharp 2007 论文算例（含负数据）、Lin 2019 算例 + 平移不变/单位不变性质数值检验
- 与标准 SBM（M2 常规模型）共享内核，实现成本低

---

## 10. Malmquist 多参比（7 种）

### 10.1 官方定义
> "a) Adjacent Malmquist b) Fixed Malmquist c) Global Malmquist d) Sequential Malmquist e) Window-Malmquist (Adjacent) f) Window-Malmquist (Fixed) g) Global with Sequential Malmquist"（Features_X.htm）；官方另含三种指数时间基准：Index(t-1,t) / Index(t-n,t) / Index(t0,t)

### 10.2 候选论文（6 篇）

| # | 论文 | 题录 | 参比 |
|---|---|---|---|
| 1 | Caves, Christensen & Diewert (1982) | The economic theory of index numbers..., *Econometrica* 50:1393-1414 | 原始 Malmquist |
| 2 | Färe et al. (1992/1994) | Productivity growth, technical progress... | Adjacent/Fixed 分解 |
| 3 | **Pastor & Lovell (2005)** | A global Malmquist productivity index, *Economics Letters* 88:266-271 | **Global** |
| 4 | Shestalova (2003) | Sequential Malmquist indices, *EJOR* 149:596-608 | **Sequential** |
| 5 | Asmild et al. (2004) | Window Malmquist, *EJOR* | **Window** |
| 6 | Tulkens & Vanden Eeckaut (1995) | Non-parametric efficiency, progress and regress..., *EJOR* 80:474-499 | 技术集（contemporaneous/sequential/intertemporal）概念基础 |

### 10.3 公开实现/路线
- ✅ **DEAPack**：global_malmquist.py / biennial_malmquist.py / _pooled_malmquist.py（HiGHS）+ source_protocols（Oh 2010、Pastor-Lovell 2005）
- ✅ **deaR**：malmquist_index.R 支持 **global/sequential** 参比（源码确认）
- ✅ **R productivity**：Malmquist + Färe-Primont 分解；Stata：malmq2（Global）、gtfpch（Global ML）
- **P0/P1**：多参考对拍；自行实现 = 距离函数内核 + 参比集规则（Adjacent/Fixed/Global/Sequential/Window/组合）+ 三种指数时间基准；与 HMB（§6）共享内核合并实现
- **验证**：DEAPack/deaR/productivity 三实现对拍 + Färe 1994 经典算例

---

## 11. DEAPack 专项盘点（跨算法最高价值参考资产，第 1 轮复审更新）

**daopingw/DEAPack**（Python，GPL-3.0，PyPI + readthedocs + CI）：
- 独有算法相关：game_cross_efficiency.py ✅ / **cone_ratio (polyhedral_crs, Charnes 1990)** ✅ / assurance_region.md ✅ / cross_efficiency.py ✅ / hicks_moorsteen.py ✅ / EBM (declared) / global-biennial-pooled Malmquist ✅ / metafrontier / Luenberger / 动态网络 SBM / scale elasticity
- **区分**：`weak_disposal.py` = 弱可处置性环境模型（undesirable outputs），**≠ 弱有效前沿最近距离（§3）**
- 求解器用 SciPyHiGHS（与 OpenSuperDEA 同款）；GPL-3 限参考不直接移植

## 12. 总结论（10/10 全部有解）

1. **10 项独有算法全部有明确论文路线**（每项 ≥3 篇；仅 Olyaie 2014 单篇题录未锁定、有 2 篇等价替代）
2. **A 类 8 项无一需要"从零发明"**：MinDS/Nash/HMB/AR 有开源参考（deaR/DEAPack/R productivity/PyDEA）；EBM/FDEF/Cluster/WeakEF 有完整公式论文 + 算例
3. **最高风险点**：FDEF 的 NDRS/NIRS（P2）；Weak EF 投影不唯一性（P2）；big-M 数值稳定性（MinDS/Nash/WeakEF 共用）
4. **实现顺序建议（M5 内部）**：M2 常规引擎 → FDEF(CRS) → MinDS(Zhu 2018) → 扩展面(Zhu 2022) → WeakEF → Nash/AR/Cone/修正SBM → Cluster → Malmquist 7 参比+HMB 合并
5. **验证体系**：开源实现对拍（deaR/DEAPack/productivity/PyDEA）+ 论文算例复现 + 数学性质检验，覆盖全部 10 项

## 13. 已知缺口

- **Olyaie et al. (2014) 精确题录未锁定**（等价替代：Mehdiloozad & Ahmadi 2015 arXiv:1503.09095；Aparicio & Pastor 2013 Omega）
- **Charnes, Rousseau & Semple 的 JPA 卷期/年份待核**（官方标 1996；SpringerLink 10.1007/BF00158473 实际卷期待确认）
- **Vakili, Amirmoshiri & Mirnia（closest weak & strong projections）完整题录待补全**
- DEAPack EBM 从 declared 转 executable 后需追加对拍
- FDEF VRS 的 QHULL 筛选在极高维（m+s>10）下的性能需实测
- MaxDEA 对 Cluster 8 基准的确切数学定义未见公开文献逐条对齐（以 Seiford & Zhu 2003 分层模型 + 参比组合推断；**MaxDEA Lite 不含 Cluster，不可对拍**——验证依赖论文算例与应用数据）
- **Preference (weighted) model（官方条目 8，Set Input/output Weights）** 未列入本表：固定权重效率计算 = 低成本功能（deaR model_basic 的 weights 参数为 slack 权重，需另实现固定乘子权重），归 M2 常规功能，不作独有算法管理

## 14. 第 1 轮复审记录（v1.0 → v2.0）

- 新增第 10 项：Minimum Distance to Weak Efficient Frontier（官方条目 3，复审前遗漏）
- Lin et al. 2019 DOI 修正：10.1016/j.cie.2019.07.011（错）→ **10.1016/j.cie.2019.05.030**（对，ORCID 验证）+ 作者补全 Lin, Yang & Huang
- arXiv:1503.09095 定性修正：Mehdiloozad & Ahmadi 2015 独立论文（least distance 参考集 + CRTS），非"Olyaie 邻近"
- 补充：Aparicio & Pastor 2013（强单调机制）、Kerstens-Van de Woestyne 2014（EJOR 233(3) + 开放全文）、Lins 等 2025（Public Bidding, JPA）、Rafati-Maleki 2018（CRS 超平面）、Tulkens-VandenEeckaut 1995（技术集/参比）、Wu-Liang-Zha 2009（ORSJ 开放全文）、Briec-Kerstens 2011（HMB determinateness）
- AR/Cone 升级 P0：DEAPack cone_ratio 实现确认
- Cluster 验证手段修正：MaxDEA Lite 不含 Cluster（官方 Download.htm 确认）
- 口径说明：在线调研 8 项 = 算法 7 项 + 性能 1 项；本表 10 项 = 大纲 9 项 + WeakEF
