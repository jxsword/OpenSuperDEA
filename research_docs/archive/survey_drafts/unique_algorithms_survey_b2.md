# 独有算法论文与技术路线表（OpenSuperDEA · M0-①）

> 调研批次进度：**批 1（MinDS + FDEF）+ 批 2（博弈交叉效率 + EBM + HMB）完成** ｜ 2026-10-24 ｜ 状态：批 1 已确认，批 2 待路线评审
> 产出规范：每算法 ≥3 篇候选论文（题录+DOI/官方链接）；P0/P1/P2 路线结论；URL 均已实际访问并标注日期。

## 0. 总览

| # | 算法 | 类 | 调研状态 | 路线结论 | 公开实现 |
|---|---|---|---|---|---|
| 1 | MinDS / Closest Target（强单调） | A | ✅ 批1 | **P0/P1** | deaR model_addmin (GPL) |
| 2 | FDEF 全维有效面 | A | ✅ 批1 | **P1**（CRS MILP；VRS QHULL；NDRS/NIRS=P2） | 无直接实现 |
| 3 | 博弈交叉效率（Nash） | A | ✅ 批2 | **P0/P1** | DEAPack game_cross_efficiency (GPL-3) |
| 4 | EBM 混合距离 | A | ✅ 批2 | **P1**（GRIPS 论文公式完整） | DEAPack（declared 状态）；deaR 无 |
| 5 | HMB（Hicks-Moorsteen） | A | ✅ 批2 | **P0/P1** | R productivity (GPL-3) + DEAPack |
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

### 1.2 候选论文清单（7/7 已确认）

| # | 论文 | 题录 | DOI / 链接 | 角色 |
|---|---|---|---|---|
| 1 | Aparicio, Ruiz & Sirvent (2007) | Closest targets and minimum distance to the Pareto-efficient frontier in DEA, *JPA* 28(3):209-218 | 10.1007/s11123-007-0039-5 | 单阶段 MILP（0-1），奠基 |
| 2 | Jahanshahloo et al. (2012) | Linear bilevel programming for closest targets..., *APJOR* | doi.org/10.1142/S0217595912400021 | 双线性规划（备选） |
| 3 | Jahanshahloo, Roshdi & Davtalab-Olyaie (2013) | Characterizing and finding FDEFs of PPS with CRS, *IJIM* 5(2):149-159 | journals.srbiau.ac.ir/article_2148_...（全文已读） | **FDEF 与 MinDS 桥接** |
| 4 | Olyaie, Roshdi, Jahanshahloo & Asgharian (2014) | 增强型 closest targets（具体题录待补） | 邻近可获 arXiv:1503.09095 | 最近距离参考集框架 |
| 5 | Aparicio, García-Nové, Kapelko & Pastor (2017) | Graph productivity change measure using least distance, *OMEGA* | Semantic Scholar 确认 | least distance 生产率扩展 |
| 6 | **Zhu, Wu, Ji & Li (2018)** | **A simple MILP for closest targets...strong monotonicity, *OMEGA* 79:1-8** | **10.1016/j.omega.2017.07.003** | **强单调核心算法** |
| 7 | Zhu, Aparicio, Li, Wu & Kou (2022) | Determining closest targets on the extended facet PPS, *EJOR* 296(3):927-939 | doi.org/10.1016/j.ejor.2021.08.023 | 扩展面版本（最新） |

### 1.3 公开实现盘点

- ✅ **deaR R 包 `model_addmin`**（CRAN 1.5.4，**GPL-2|GPL-3**）：Aparicio 2007 + Zhu 2018 修改；`mf`（maximal friends）/ `milp` 两法；CRS/VRS/NIRS/NDRS；**内置 Airlines 28 航空公司论文算例**。文档：rdocumentation.org/packages/deaR/.../model_addmin ｜ CRAN：cran.r-project.org/web/packages/deaR/
- 其余 6 篇无公开代码；deaR 未实现扩展面（Zhu 2022）

### 1.4 实现路线建议（批 1 已确认 ✅）

P0（deaR 对拍参考）+ P1（Zhu 2018 简单 MILP → Zhu 2022 扩展面）；依赖 §2 FDEF；实现顺序 FDEF(CRS) → MinDS(Zhu 2018) → 扩展面(Zhu 2022)。MILP 用 HiGHS；注意 big-M 数值敏感（deaR 文档明示 catastrophic cancellations 风险，需 M 自动估计）。

### 1.5 验证方案

deaR 对拍（4 种 RTS）；Airlines 数据；Zhu 2018 算例；Sekitani & Zhao 2023 (arXiv:2312.15422, CC BY 4.0) 数值表；强单调性数学性质检验。

---

## 2. FDEF（Full Dimensional Efficient Facet）

### 2.1 功能定义

MaxDEA X 独有能力：输出**所有 FDEF 的顶点与超平面方程**（分段前沿生产函数），是 MinDS 强单调、敏感度分析的基础。官方 Features 页未单列 FDEF，但 Jahanshahloo 2013 被官方列为 MinDS 实现文献。

### 2.2 候选论文清单（5 篇）

| # | 论文 | 题录 | 可访问性 | 角色 |
|---|---|---|---|---|
| 1 | **Jahanshahloo, Roshdi & Davtalab-Olyaie (2013)** | FDEFs of PPS with CRS, *IJIM* 5(2):149-159 | ✅ 全文已读（含算例） | **CRS 全 FDEF 枚举 MILP——P1 核心** |
| 2 | Olesen & Petersen (2003) | Identification and use of efficient faces and facets in DEA, *JPA* 20:323-360 | 订阅 | VRS 理论奠基（E_j 算法有缺陷被 2013 批判） |
| 3 | Akbarian (2014) | Characterizing weak defining hyperplanes in DEA with CRS, arXiv:1409.7546 | ✅ 开放 | weak facet 刻画（GAMS 可实现） |
| 4 | Huang, Wang, Zhang, Xu (2025) | DEA based on robust and closest targets, arXiv:2505.06487 | ✅ 开放（CC BY-NC-ND） | robust targets = FDEF 交集（最新） |
| 5 | （2025） | Public bidding using FDEFs of VRS frontier, *JPA* | RePEc 确认 | VRS FDEF 应用 |

### 2.3 公开实现盘点

无直接开源实现。两条可行技术路线：**路线 A** = Jahanshahloo 2013 MILP 枚举（CRS，模型 2.4 求 extreme CCR-efficient 集 E → 模型 3.5 找 SDH）；**路线 B** = QHULL 计算几何枚举凸包面 + 强有效面筛选（VRS，SDU WP 佐证）。

### 2.4 实现路线建议（批 1 已确认 ✅）

P1（CRS = Jahanshahloo MILP；VRS = QHULL 双路线互证）；NDRS/NIRS 标 P2 最高风险。

---

## 3. 博弈交叉效率（Game Cross Efficiency, Nash Equilibrium）

### 3.1 MaxDEA 官方定义（原文引用，访问 2026-10-24）

> 来源：http://maxdea.com/Features_X.htm
> "5) Game Cross Efficiency model: **Nash Equilibrium model** (Liang, et al 2008; Wu, et al 2009)"

### 3.2 候选论文清单（4 篇，≥3 满足）

| # | 论文 | 题录 | DOI / 链接 | 角色 |
|---|---|---|---|---|
| 1 | **Liang, Wu, Cook & Zhu (2008)** | The DEA Game Cross-Efficiency Model and Its Nash Equilibrium, *Operations Research* 56(5):1278-1288 | **10.1287/opre.1070.0487** | **核心算法：迭代求 best game scores = Nash 均衡（CRS）** |
| 2 | Wu, Liang & Chen (2009) | DEA game cross-efficiency approach to Olympic rankings, *OMEGA* 37(4):909-918 | 10.1016/j.omega.2008.07.007 | **VRS 情形博弈交叉效率**（对应 MaxDEA 多 RTS） |
| 3 | Wu, Liang & Zha (2009) | Preference voting and ranking using DEA game cross efficiency model, *JORSJ* 52(2):105-111 | Scopus 确认 | 偏好投票应用 |
| 4 | Liang, Wu, Cook & Zhu (2008b) | DEA models for two-stage processes: Game approach..., *EJOR* 185(1):418-429 | 10.1016/j.ejor.2006.12.025 | 两阶段扩展（可选项） |

### 3.3 公开实现盘点（重要发现）

- ✅ **DEAPack `game_cross_efficiency.py`**（daopingw/DEAPack，**GPL-3.0**，Python）：完整实现 Liang-Wu-Cook-Zhu 2008（方法 ID `evaluation.cross.game_nash.liang_wu_cook_zhu_2008`），**使用 SciPyHiGHSSolver（HiGHS）——与 OpenSuperDEA 技术栈同款求解器**；含 benchmark（benchmark_cross_appraisal.py）
- DEA-Frontier（Excel 商业插件）含 game cross efficiency（第三方博客实证），非开源
- deaR 无 game 版本（仅普通交叉效率 model_cross_efficiency）

### 3.4 实现路线建议

- **P0 参考移植 + 对拍**：DEAPack game_cross_efficiency.py（Python/GPL——参考算法逻辑、自行 C++ 实现；求解器同为 HiGHS，移植语义直接）
- **P1 核心算法**：Liang et al. 2008（CRS 迭代算法：每个 DMU 在"其他 DMU 得分不低于当前最优"约束下最大化自身效率 → 迭代至收敛 = Nash 均衡）；Wu et al. 2009（VRS）
- **技术要点**：迭代收敛判据、M 参数；CRS/VRS 乘子模型

### 3.5 验证方案

1. DEAPack 对拍（随机数据 + 论文算例）
2. 论文数值复现：Liang 2008（数值算例）、Wu 2009（奥运奖牌数据）
3. Nash 均衡性质检验（任何单方偏离不再改善 = 均衡点验证）

---

## 4. EBM（Epsilon-Based Measure，混合距离）

### 4.1 MaxDEA 官方定义（原文引用）

> 来源：http://maxdea.com/Features_X.htm
> "7) Hybrid Distance (Radial and SBM Measure): (EBM, Epsilon-based Measure, Tone and Tsutsui 2010)"

### 4.2 候选论文清单（4 篇，≥3 满足）

| # | 论文 | 题录 | DOI / 链接 | 角色 |
|---|---|---|---|---|
| 1 | **Tone & Tsutsui (2010)** | An epsilon-based measure of efficiency in DEA – A third pole of technical efficiency, *EJOR* 207(3):1554-1563 | **10.1016/j.ejor.2010.07.014** | **核心：EBM-I-C / EBM-O-C + affinity index + PCA** |
| 2 | Tone & Tsutsui (2010) 工作论文版 | EBM revisited（GRIPS DP09-21） | ✅ **grips.repo.nii.ac.jp/record/1021/files/DP09-21.pdf（开放全文，完整公式+算例）** | **P1 实现直接依据** |
| 3 | 图量级（2021/2022）应用论文 | MDPI 系列 EBM 应用（公式复述） | mdpi.com/1996-1073/14/15/4498 等 | 公式交叉佐证 |
| 4 | Figshare 补充材料（2026） | EBM-DEA exogenous variable 附录 A1.1（ε/权重计算） | figshare.com/.../34067966 | 实现细节佐证 |

### 4.3 公开实现盘点

- ⚠️ **DEAPack**：`benchmarks/benchmark_ebm.py` + `specs/M13_DECLARED_EBM_IC.md`（**declared 状态**，非 executable 确认）+ oracle（tone_tsutsui_2010_ebm_ic）+ docs/models/ebm.md —— EBM 在 DEAPack 中"已声明/进行中"
- ❌ deaR 无 EBM（确认：CRAN 的 ebm 包为 Explainable Boosting Machines，无关）
- ❌ GitHub 无独立 EBM 实现

### 4.4 实现路线建议

- **P1 自行实现**：GRIPS DP09-21 公式完整可复现——
  1. 归一化数据 → affinity index S(a,b) = 1 - 2D（D 为基于 ln(b_j/a_j) 的 diversity index）
  2. 构建 affinity matrix S（m×m 输入 / s×s 输出）
  3. **Perron-Frobenius 最大特征值 ρ + 非负特征向量 w**（Eigen 库谱分解可直接计算）
  4. ε = (m-ρ)/(m-1)、w- = w/Σw（输出侧同理）
  5. EBM-I-C / EBM-O-C 线性规划（HiGHS 求解）
- **验证数据现成**：GRIPS 论文算例 ρx=1.471, wx=(0.5,0.5), εx=0.529
- 待 DEAPack EBM 转 executable 后可对拍

### 4.5 风险

| 风险 | 级别 | 说明 |
|---|---|---|
| ε/权重计算与 Tone 原版精确一致 | 中 | GRIPS 工作论文与正式版差异需对齐（工作论文即正式版预处理版本，建议以正式版 EJOR 为准、工作论文作公式参考） |
| 特征向量非负性退化 | 低 | Perron-Frobenius 保证非负矩阵主特征向量非负 |
| 非径向（EBM-N）变体 | 低 | 官方主打 EBM-I/O-C；EBM-N 可选 |

---

## 5. HMB（Hicks-Moorsteen TFP Index）

### 5.1 MaxDEA 官方定义（原文引用）

> 来源：http://maxdea.com/Features_X.htm
> "TFP Index: **Malmquist Index and Hicks-Moorsteen Index (also called HMB Index)**"

### 5.2 候选论文清单（4 篇，≥3 满足）

| # | 论文 | 题录 | DOI / 链接 | 角色 |
|---|---|---|---|---|
| 1 | **Bjurek (1996)** | The Malmquist Total Factor Productivity Index, *Scandinavian J. Economics* 98(2):303-313 | 10.2307/3440855 | **HMB 原始定义（Malmquist 产出指数/投入指数之比）** |
| 2 | O'Donnell (2012) | An aggregate quantity framework for measuring and decomposing productivity change, *JPA* 38(3):255-272 | 10.1007/s11123-011-0256-9 | 分解框架（TFPE=OTE×OSE×RME） |
| 3 | Kerstens et al. (2014) | Comparing Malmquist and Hicks-Moorsteen indices（balanced/unbalanced panel） | Sci-Hub 快照（公式） | HMB 公式/性质佐证 |
| 4 | Mizobuchi (2017) | Productivity indexes under Hicks neutral technical change | 10.1016/j.ejor.2016.10.043 | 理论性质 |

### 5.3 公开实现盘点（重大发现）

- ✅ **R 包 `productivity`**（CRAN 1.1.2，**GPL-3**，Dakpo/Desjeux/Latruffe，INRA）：完整实现 **Färe-Primont、Hicks-Moorsteen、Fisher、Laspeyres、Lowe、Paasche、Malmquist** 指数族 + 分解 + 并行；用 lp_solve。CRAN：cran.r-project.org/web/packages/productivity/
- ✅ **DEAPack `hicks_moorsteen.py`**（Bjurek adjacent-period HMB，GPL-3，HiGHS/scipy）
- GitHub timbulwidodostp/Hicks-Moorsteen（R，DPIN 3.0 用法）

### 5.4 实现路线建议

- **P0 参考 + 对拍**：R productivity（HMB 完整指数族 + 分解逻辑）与 DEAPack 双参考
- **P1 核心**：Bjurek 1996 定义（HMB = 产出数量指数/投入数量指数，基于距离函数）；O'Donnell 2012 分解（可选）
- **技术要点**：HMB 需计算 4 个距离函数（DO、DI 在两期两技术下）；与 Malmquist 共享距离函数计算内核——**M5 内可与 Malmquist 多参比合并实现**
- 注意 HMB 与 Malmquist 差异（Kerstens 2014：HMB 对非平衡面板敏感）

### 5.5 验证方案

R productivity 包对拍（其自带数据 + 论文数据）；Bjurek 1996 数值算例；DEAPack hicks_moorsteen benchmark

---

## 6. DEAPack 专项盘点（跨算法参考资产，批 2 新发现）

**daopingw/DEAPack**（https://github.com/daopingw/DEAPack，Python，**GPL-3.0**，PyPI 发布、readthedocs 文档、测试 CI）—— 覆盖最广的开源 DEA 框架，对 OpenSuperDEA 是**最高价值参考资产**：

| 能力 | DEAPack 状态 | 与 OpenSuperDEA 关联 |
|---|---|---|
| MinDS/closest targets | 方法宇宙规划中（FDEF for strong targets） | 参考其方法文档 |
| FDEF | 规划中（未 executable） | 参考方法映射 |
| Game cross efficiency (Liang 2008) | ✅ executable（HiGHS） | **P0 参考** |
| EBM | declared（进行中）+ benchmark + oracle | 参考公式协议 |
| HMB (Bjurek) | ✅ executable（HiGHS） | **P0 参考** |
| Malmquist 系（global/biennial/pooled） | ✅ 多个 executable | **批 3 参考** |
| 动态/网络 SBM、metafrontier、Luenberger 等 | ✅ | 常规模型参考 |

许可证提示：GPL-3 代码可读参考，正式移植到 C++ 需独立实现（避免 GPL 传染；HiGHS 本身 Apache-2.0 无冲突）。

---

## 7. 批 1 + 批 2 结论（供路线评审）

1. **MinDS**：P0/P1（deaR 参考 + Zhu 2018/2022）✅ 已确认
2. **FDEF**：P1（CRS MILP / VRS QHULL）；NDRS/NIRS=P2 ✅ 已确认
3. **博弈交叉效率**：**P0/P1**（DEAPack 参考 + Liang 2008 迭代算法 + Wu 2009 VRS）
4. **EBM**：**P1**（GRIPS 公式完整；Eigen 谱分解 + HiGHS LP）
5. **HMB**：**P0/P1**（R productivity + DEAPack 双参考；与 Malmquist 合并实现）
6. **共通点**：DEAPack 用 HiGHS（与 OpenSuperDEA 同款求解器），移植语义最接近；验证基线 = 各开源实现对拍 + 论文算例

## 8. 后续批次预告

- 批 3：Cluster（8 基准）+ AR/Cone + 修正 SBM + Malmquist 多参比（含 HMB 合并实现要点）
