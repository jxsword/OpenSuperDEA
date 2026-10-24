# 独有算法论文与技术路线表（OpenSuperDEA · M0-①）

> 版本：**v4.0（调研 A/B/C 执行完成）**｜ 2026-10-24 ｜ 状态：待用户确认 + M0 报告交付
> 调研进度：批 1 + 批 2 + 批 3 完成；两轮复审完成（新增 Weak EF、题录修正 10 处、AR/Cone 升 P0）；追加调研完成（Olyaie 终判、Cluster 全文精读+对拍数据、性能参考补充）
> 产出规范：每算法 ≥3 篇候选论文；P0/P1/P2 路线结论；URL 均已实际访问（第 2 轮 HEAD 校验 7 个关键链接全部 HTTP 200）并标注日期 2026-10-24。

## 0. 总览（10/10 项）

> **口径说明**：在线平台调研报告的"8 项独有能力全缺失" = 算法 7 项 + 性能 1 项（万级 DMU 极速求解）；本表按调研大纲 A/B 分类 = 9 项算法；**复审对照官方清单新增"最小距离弱有效前沿"→ 现为 10 项算法**。性能目标（10 万/100 万 DMU）不属算法调研，归 M3/M4 性能里程碑。

| # | 算法 | 类 | 状态 | 路线结论 | 公开实现 |
|---|---|---|---|---|---|
| 1 | MinDS / Closest Target（强单调） | A | ✅ | **P0/P1** | deaR model_addmin (GPL) |
| 2 | FDEF 全维有效面 | A | ✅ | **P1**（CRS MILP；VRS QHULL；NDRS/NIRS=P2） | 无（QHULL 可作基础） |
| 3 | 最小距离弱有效前沿 Weak EF | A | 复审新增 | **P1**（两阶段 LP；弱投影不唯一性=P2） | 无 |
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

### 1.2 候选论文（11 篇）

| # | 论文 | 题录 | DOI/链接 |
|---|---|---|---|
| 1 | Aparicio, Ruiz & Sirvent (2007) | Closest targets..., *JPA* 28:209-218 | 10.1007/s11123-007-0039-5 |
| 2 | Jahanshahloo, Vakili & Zarepisheh (2012) | Linear bilevel programming for closest targets, *APJOR* | doi.org/10.1142/S0217595912400021 |
| 3 | Jahanshahloo, Roshdi & Davtalab-Olyaie (2013) | Characterizing FDEFs of PPS with CRS, *IJIM* 5(2):149-159 | ✅ 全文 journals.srbiau.ac.ir |
| 4 | Olyaie, Roshdi, Jahanshahloo & Asgharian (2014) | **终判：公开索引缺失**（见 §1.4） | — |
| 5 | Aparicio, García-Nové et al. (2017) | Graph productivity change (least distance), *OMEGA* | 10.1016/j.omega.2016.09.008 系（Omega 71:1-10 另见 #10） |
| 6 | **Zhu, Wu, Ji & Li (2018)** | **Simple MILP ... strong monotonicity, *OMEGA* 79:1-8** | **10.1016/j.omega.2017.07.003** |
| 7 | Zhu, Aparicio, Li, Wu & Kou (2022) | Closest targets on extended facet PPS, *EJOR* 296:927-939 | 10.1016/j.ejor.2021.08.023 |
| 8 | **Aparicio & Pastor (2014)** | **Closest targets and strong monotonicity on the strongly efficient frontier, *Omega* 44:51-57** | **10.1016/j.omega.2013.10.001**（强单调机制：Hölder 距离为何失效 + 输出导向解） |
| 9 | **Fukuyama, Maeda, Sekitani & Shi (2014)** | **Input–output substitutability and strongly monotonic p-norm least distance DEA measures, *EJOR* 237(3):997-1007** | **10.1016/j.ejor.2014.02.033**（p-norm 强单调 + tradeoff set——强单调第 2 方法） |
| 10 | Aparicio et al. (2017) | The determination of the least distance to the strongly efficient frontier, *Omega* 71:1-10 | 10.1016/j.omega.2016.09.008（graph measure） |
| 11 | **Aparicio, Cordero & Pastor** | **The determination of the least distance to the SEF in DEA oriented models: Modelling and computational aspects** | ✅ CORE 开放 PDF（**面向模型 least distance 关键实现参考，PISA 902 校算例**） |

### 1.3 公开实现 / 路线 / 验证

- ✅ **deaR `model_addmin`**（CRAN 1.5.4，GPL-2|3）：Aparicio 2007 + Zhu 2018；mf/milp 两法；**rts = c("crs","vrs","nirs","ndrs") 源码确认 4 种 RTS**；内置 Airlines 28 算例
- **路线（已确认）**：P0 对拍 + P1 自行实现（Zhu 2018 → Zhu 2022 扩展面）；实现顺序 FDEF → MinDS → 扩展面；MILP 用 HiGHS；big-M 需自动估计
- **验证**：deaR 对拍（4 RTS）、Airlines 数据、Zhu 2018 算例、Sekitani & Zhao 2023（arXiv:2312.15422, CC BY 4.0）数值表、强单调性检验

### 1.4 Olyaie 2014 题录终判（调研 A 完成，2026-10-24）

- **检索动作**：Crossref 作者级检索 "Olyaie"（2013-2016 全领域）仅 6 篇——全为压电微电机/水文预测/生物力学等**非 DEA 领域**论文；"Olyaie+Roshdi" 组合检索无命中；scholar_search 3 轮不同关键词（含 "closest target"、"strong efficient frontier"）均无 DEA 领域命中；同源期刊系（IJIM）站内无此篇。
- **终判**：该文献在公开索引（Crossref / Scholar / 出版商站内）中**无核验题录**，极可能发表于未入主流数据库的期刊/会议/工作论文，或官方引用有误。**按大纲兜底处置：如实标注 + 不依赖该篇**。
- **功能影响：无**。MinDS 主实现闭环 = Zhu-Wu-Ji-Li 2018（MILP 强单调）+ Aparicio-Pastor 2014 / Fukuyama-Maeda-Sekitani-Shi 2014（强单调机制）+ Aparicio-Cordero-Pastor（面向模型版，CORE 开放，PISA 902 校算例）+ deaR `model_addmin` 对拍。
- **等价替代（保留）**：Mehdiloozad & Ahmadi (2015) arXiv:1503.09095（least distance 参考集 + Closest RTS）；Mehdiloo & Podinovski (2021) EJOR 294(1):295-311（强/弱/Farrell 前沿概念关系，figshare 开放 PDF）。

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
| 5 | **Lins, do Nascimento, Dulá, Torres & Lins (2025)** | Public bidding ... ranking proposals using FDEFs of DEA frontier, *JPA* | ✅ link.springer.com/article/10.1007/s11123-025-00767-1 | **VRS FDEF + ERGM 最近目标两阶段法（P1 路线应用佐证）** |
| 6 | **Rafati-Maleki et al. (2018)** | Finding the strong defining hyperplanes of PPS with CRS using the linear independent vectors, *Cogent Math & Stat* | ✅ Taylor & Francis | CRS 强定义超平面替代方法 |
| 7 | （MOO 工作论文） | An MOO approach to compute the efficient frontier in DEA（dual MOO, CORE 开放） | ✅ CORE | 计算几何替代路线佐证 |

### 2.3 路线（已确认）
P1：路线 A = Jahanshahloo 2013 MILP（CRS）；路线 B = QHULL 计算几何（VRS，SDU WP + Lins 2025 佐证）双路线互证；NDRS/NIRS = P2 最高风险。

---

## 3. 最小距离弱有效前沿（Minimum Distance to Weak Efficient Frontier）【复审新增】

### 3.1 官方定义
> "3) Minimum Distance to Weak Efficient Frontier (Charnes, Rousse, and Semple 1996)"（Features_X.htm；官方拼写 "Rousse" 实为 Rousseau）

### 3.2 候选论文（4 篇，题录全部锁定）

| # | 论文 | 题录 | DOI/链接 | 角色 |
|---|---|---|---|---|
| 1 | Charnes, Cooper & Rhodes (1978) | Measuring the efficiency of DMUs, *EJOR* 2:429-444 | 10.1016/0377-2217(78)90138-8 | 弱效率概念定义 |
| 2 | **Charnes, Rousseau & Semple (1996)** | **Sensitivity and stability of efficiency classifications in DEA, *JPA* 7(1):5-18** | **10.1007/BF00158473**（官方引用文献，110 引） | 官方依据（l1/l∞ 分类半径） |
| 3 | **Vakili, Amirmoshiri & Mirnia** | Novel models for obtaining the closest weak and strong efficient projections in DEA | ✅ Semantic Scholar PDF（期刊待补） | **closest weak projection 直接算法（QCP，30 欧洲机场算例）** |
| 4 | **Mehdiloo & Podinovski (2021)** | Strong, weak and Farrell efficient frontiers..., *EJOR* 294(1):295-311 | 10.1016/j.ejor.2021.01.022（✅ figshare 开放） | 弱前沿概念边界（理论基础） |

### 3.3 实现/路线
- **P1 自行实现**：弱有效投影两阶段法（先求弱效率投影 → 再最小化距离范数）；l1/l∞/l2 范数选择；**风险 P2**：弱前沿投影点不唯一（需借用 MinDS 的 strong monotonicity 成果，见 §1 第 8/9 篇）
- **公开实现**：GitHub 无；deaR/DEAPack/PyDEA 均无（DEAPack weak_disposal.py 是"弱可处置性"环境模型，≠ 弱前沿最近距离，勿混淆）
- **验证**：Vakili 等 30 欧洲机场数据（论文算例）、CCR 经典算例；与 MinDS（§1）共用投影内核

---

## 4. 博弈交叉效率（Game Cross Efficiency, Nash）

### 4.1 官方定义
> "5) Game Cross Efficiency model: Nash Equilibrium model (Liang, et al 2008; Wu, et al 2009)"（Features_X.htm）

### 4.2 候选论文（4/4，题录全部锁定）

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
- Downward/Upward/Lower-adjacent/Upper-adjacent = Context-dependent DEA 的 **progress / attractiveness** 分层评估（Seiford & Zhu 2003，见 §7.2 精读）
- Self/Cross = 群内/群间参考集选择（聚类 + 参比组合；Cross 亦有 context-dependent 交叉效率版支撑——Lim 2011）
- Window/Fixed = 窗口分析 / 固定基准参比（技术集概念见 Tulkens & Vanden Eeckaut 1995）

### 7.2 候选论文（7 篇，文献链闭环）

| # | 论文 | 题录 | 可访问性 | 角色 |
|---|---|---|---|---|
| 1 | **Seiford & Zhu (2003)** | Context-dependent DEA—Measuring attractiveness and progress, *OMEGA* 31:397-408, 10.1016/S0305-0483(03)00080-X | ✅ **DEAFrontier 开放全文，已完整精读**（见 §7.3） | **核心算法 + 黄金对拍算例** |
| 2 | Zhu (2001) | Super-efficiency and DEA sensitivity analysis, *EJOR* 129:443-455 | 10.1016/S0377-2217(00)00157-0 | 分层基础 |
| 3 | **Morita, Hirokawa & Zhu (2005)** | A slack-based measure of efficiency in context-dependent DEA, *Omega* 33(4):357-362 | ✅ RePEc 确认（32 引） | **SBM 版 context-dependent——非径向分层基准** |
| 4 | **Lim (2011)** | Context-dependent DEA with cross-efficiency evaluation, *JORS* | ✅ 摘要/PDF 可访问 | **Cross-benchmarking 的交叉效率版支撑** |
| 5 | **Tulkens & Vanden Eeckaut (1995)** | Non-parametric efficiency, progress and regress measures for panel data, *EJOR* 80:474-499 | ✅ ScienceDirect | **contemporaneous/sequential/intertemporal 技术集 + window + benchmark 概念（Fixed-benchmarking 理论基础）** |
| 6 | Xu & Ouenniche (2012) | SBM-based context-dependent DEA（super-efficiency 变体） | ✅ 爱丁堡开放 PDF | 变体佐证 |
| 7 | 郑雅穗 (2005) | Improved slack-based context-dependent DEA — Taiwan tourist hotels | ✅ CORE 开放（34 酒店 5 层应用） | 应用佐证 |

### 7.3 Seiford & Zhu 2003 全文精读成果（调研 C 完成，2026-10-24）

- **算法完整**（输入/输出导向 + 价值判断版全覆盖）：
  - 模型 (1)：输出导向 CCR 分层剥离（Step 1-4 + 停止规则；逐步求 E¹→剔除→E²→…→E^L）
  - 模型 (2)/(3)：attractiveness（对更差层 E^(l₀+d)）/ progress（对更优层 E^(l₀-g)）；性质 A*(d)>1 且递减、P*(g)>1 且递增
  - 模型 (4)-(7)：VJ 版（用户指定输出权重 u_r，Σu=1；含输出特定 attractiveness、preferred global/local efficient target 定理）
  - **§3.3 关键打通：VJ 权重 = 乘子下界 = assurance region 概念 → 与 §8 AR/Cone 直接复用**（D_r = u_r/y_{rq} 作为 lower output multiplier bounds）
  - 附录 A：输入导向版（模型 A.1-A.4，H/G/M 度量，VJ 版权重 w_i）
- **黄金对拍算例**（Doyle & Green 1991 的 32 打印机，7 变量）：完整数据表 + 分层 E¹={1,2,3,5,19,20,26}、E²={4,7,10,11,12,15,31}、E³={6,8,9,13,22,27,30}、E⁴={14,16,…32 其余 11 个} + Table 3-6（attractiveness/progress/VJ 分数 + 参考集 + λ*）全部提取 → **可构造 OpenSuperDEA Cluster 单元回归测试基准**
- **核心差异点**（§5 结论）：原始 DEA 增删无效 DMU 不改变效率；context-dependent DEA 下**增删无效 DMU 改变现有 DMU 效率** → 这是 Cluster 与一般 DEA 的本质区别，也是单元测试的关键断言。

### 7.4 实现/路线（升级：文献+算法+数据全覆盖）
- **P1 自行实现**：分层剥离算法（模型 1）→ attractiveness/progress（模型 2/3，参考集 = E^(l₀±d) 子集）→ VJ 版（模型 4-7，权重→AR 下界复用）；8 种参比 = 参考集组合规则（参数化：self/cross = 本群/他群 DMU 集；down/up = 更差/更优层；adjacent = d=g=1；window/fixed = 时段参考集）
- **技术要点**：核心是参考集（reference set）选择逻辑 + 分层剥离，非新数学；与窗口分析（Malmquist 窗口参比）复用
- **验证（升级）**：Seiford & Zhu 2003 32 打印机算例（数据 + E¹-E⁴ + Table 3-6 全数值对拍，误差 <1e-6）；性质断言（A*(d)>1 递减 / P*(g)>1 递增 / 增删无效 DMU 改变效率）；SBM 版（Morita 2005）与 VJ 版交叉验证。**MaxDEA Lite 免费版仅限基础径向/SBM/成本/收益模型，不含 Cluster（官方 Download.htm 确认），不可作为 Cluster 对拍基线**
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

### 8.3 公开实现/路线（P0）
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
| 2 | **Lin, Yang & Huang (2019)** | A modified slacks-based super-efficiency measure in the presence of negative data, *Computers & Industrial Engineering* 135:39-52 | **10.1016/j.cie.2019.05.030**（ORCID 验证） |
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

## 11. DEAPack 专项盘点（跨算法最高价值参考资产）

**daopingw/DEAPack**（Python，GPL-3.0，PyPI + readthedocs + CI）：
- 独有算法相关：game_cross_efficiency.py ✅ / **cone_ratio (polyhedral_crs, Charnes 1990)** ✅ / assurance_region.md ✅ / cross_efficiency.py ✅ / hicks_moorsteen.py ✅ / EBM (declared) / global-biennial-pooled Malmquist ✅ / metafrontier / Luenberger / 动态网络 SBM / scale elasticity
- **区分**：`weak_disposal.py` = 弱可处置性环境模型（undesirable outputs），**≠ 弱有效前沿最近距离（§3）**
- 求解器用 SciPyHiGHS（与 OpenSuperDEA 同款）；GPL-3 限参考不直接移植

## 12. 总结论（10/10 全部有解）

1. **10 项独有算法全部有明确论文路线**（每项 ≥3 篇；Olyaie 2014 经追加调研**终判为公开索引缺失**，等价替代 3 篇齐备，功能影响无）
2. **A 类 8 项无一需要"从零发明"**：MinDS/Nash/HMB/AR 有开源参考（deaR/DEAPack/R productivity/PyDEA）；EBM/FDEF/Cluster/WeakEF 有完整公式论文 + 算例
3. **Cluster 已升级为"文献+算法+数据全覆盖"**：Seiford-Zhu 2003 全文精读完成，32 打印机黄金对拍数据（E¹-E⁴ 分层 + Table 3-6 分数 + λ*）落库；SBM 版（Morita 2005）、交叉效率版（Lim 2011）补链
4. **最高风险点**：FDEF 的 NDRS/NIRS（P2）；Weak EF 投影不唯一性（P2）；big-M 数值稳定性（MinDS/Nash/WeakEF 共用）
5. **实现顺序建议（M5 内部）**：M2 常规引擎 → FDEF(CRS) → MinDS(Zhu 2018) → 扩展面(Zhu 2022) → WeakEF → Nash/AR/Cone/修正SBM → Cluster → Malmquist 7 参比+HMB 合并
6. **验证体系**：开源实现对拍（deaR/DEAPack/productivity/PyDEA）+ 论文算例复现（含 32 打印机/32 机场/902 学校）+ 数学性质检验，覆盖全部 10 项
7. **性能参考（附加）**：Zhu, Wu & Song (2018, COR, "Efficiency evaluation based on DEA in the big data context", iranarze 开放 PDF) 提出大规模 DEA 加速（参考点采样/强有效集分化）→ **M3/M4 万级/百万级 DMU 性能目标的算法参考**

## 13. 已知缺口（剩余 2 项 + 实现期任务）

- **Vakili, Amirmoshiri & Mirnia（closest weak & strong projections）期刊/DOI 待补全**（Semantic Scholar PDF 可访问、30 欧洲机场算例可用；Crossref bibliographic 两次检索受限/未收录——实现期可仅以 PDF 为准）
- ~~Olyaie et al. (2014) 精确题录~~ **已闭环**（终判公开索引缺失，见 §1.4；不影响实现）
- ~~Cluster 官方逐条定义~~ **已闭环**（Seiford-Zhu 2003 精读 + Morita/Lim 补链；8 基准 = 参考集组合规则参数化，验证依赖黄金算例）
- DEAPack EBM 从 declared 转 executable 后需追加对拍（实现期任务）
- FDEF VRS 的 QHULL 筛选在极高维（m+s>10）下的性能需实测（实现期任务）
- Preference (weighted) model（官方条目 8）未列入本表：固定权重效率计算为低成本功能（deaR model_basic 的 weights 为 slack 权重，需另实现固定乘子权重），归 M2 常规功能

## 14. 复审记录

### 第 1 轮（v1.0 → v2.0）
- 新增第 10 项：Minimum Distance to Weak Efficient Frontier（官方条目 3，复审前遗漏）
- Lin et al. 2019 DOI 修正：10.1016/j.cie.2019.07.011（错）→ **10.1016/j.cie.2019.05.030**（对，ORCID 验证）+ 作者补全 Lin, Yang & Huang
- arXiv:1503.09095 定性修正：Mehdiloozad & Ahmadi 2015 独立论文，非"Olyaie 邻近"
- 补充：Aparicio & Pastor（强单调机制）、Kerstens-Van de Woestyne 2014、Lins 等 2025（JPA）、Rafati-Maleki 2018、Tulkens-VandenEeckaut 1995、Wu-Liang-Zha 2009（ORSJ 开放全文）、Briec-Kerstens 2011
- AR/Cone 升级 P0（DEAPack cone_ratio 确认）；Cluster 验证手段修正（MaxDEA Lite 不含 Cluster）
- 口径说明：在线调研 8 项 = 算法 7 项 + 性能 1 项；本表 10 项 = 大纲 9 项 + WeakEF

### 第 2 轮（v2.0 → v3.0）
- **Charnes, Rousseau & Semple (1996) 题录锁定**：JPA 7(1):5-18, 10.1007/BF00158473（官方年份 1996 正确）
- **Mehdiloo & Podinovski 题录锁定**：EJOR 294(1):295-311, 2021, 10.1016/j.ejor.2021.01.022
- **Aparicio & Pastor 发表年修正**：Omega 44:51-57, **2014**（online 2013），10.1016/j.omega.2013.10.001
- **新增 Fukuyama, Maeda, Sekitani & Shi (2014)**：EJOR 237(3):997-1007, 10.1016/j.ejor.2014.02.033（p-norm 强单调）
- 链接可用性校验：ORSJ / DEAFrontier / GRIPS / arXiv:2312.15422 / Vakili PDF / IJIM 2013 / Springer JPA 2025 —— **7 个关键链接全部 HTTP 200**

### 追加调研（v3.0 → v4.0，用户确认执行的 A/B/C 三项 + 性能参考）

**调研 A — Olyaie et al. 2014 题录定位（终判）**：
- Crossref 作者级全领域检索（2013-2016）：仅 6 篇且全为非 DEA 领域（压电微电机/水文/生物医学）；"Olyaie+Roshdi" 组合无命中；scholar_search 3 轮（不同关键词）无命中；IJIM 同源期刊站内无此篇
- **终判**：公开索引缺失 → 如实标注 + 不依赖该篇；主实现闭环 4 篇（Zhu 2018 / Aparicio-Pastor 2014 / Fukuyama 2014 / Aparicio-Cordero-Pastor）+ deaR 对拍，功能影响无

**调研 B — Vakili 期刊/DOI（部分闭环）**：
- Semantic Scholar 开放 PDF（30 欧洲机场算例）确认可访问；Crossref bibliographic 两次尝试均受限/无收录 → 期刊名仍待补全，实现期可仅以 PDF 为准；新增 Aparicio-Cordero-Pastor（least distance oriented models，CORE 开放，PISA 902 校）作为同方向更强参考

**调研 C — Seiford & Zhu 2003 全文精读（闭环）**：
- 完整算法（模型 1-7 + 附录 A.1-A.4）全部提取；32 打印机黄金对拍数据（Table 2 全表 + E¹-E⁴ 分层 + Table 3-6 分数 + λ*）落库；§3.3 确认 VJ 权重 = 乘子下界 = AR 概念（与 §8 打通）；§5 确认"增删无效 DMU 改变效率"为 Cluster 本质差异
- 文献链补全：Morita-Hirokawa-Zhu 2005（SBM 版，Omega 33(4):357-362）、Lim 2011（JORS 交叉效率版）、郑雅穗 2005（Taiwan hotels 应用）

**性能参考（附加发现）**：Zhu, Wu & Song (2018, COR, "Efficiency evaluation based on DEA in the big data context"，iranarze 开放 PDF) — 大 DEA 加速（参考点采样/强有效集分化）→ M3/M4 性能目标算法参考
