# 独有算法全网调研大纲（OpenSuperDEA · M0-①）

> 版本：v2.0（两轮复审后）｜ 日期：2026-10-24 ｜ 状态：待用户确认
> 复审记录：
> - v1.0 → v1.1（第 1 轮复审）：移除对「联系作者」的一切依赖；补充检索时间范围、GitHub 搜索语法与过滤、代码可运行性筛选、完成判据、算法与 MaxDEA 官方功能条目对照。
> - v1.1 → v2.0（第 2 轮复审）：算法分 A/B 类管理；补充输出物验收标准与兜底处置；限定源码审计许可证范围；P2 处置路径闭合。

## 1. 调研目标

1. 对 MaxDEA X 的 9 项独有/领先能力（见 §2），逐一找到**最先进、可实现的**学术算法路线（论文级依据）；
2. 全网盘点**第三方公开实现**（开源代码 / 论文补充材料 / 公开算例数据），判定：可直接复用 / 需自行实现 / 存在障碍；**不采用「联系作者索取」途径（判定不可行，一律仅用公开可访问资源）**；
3. 输出《独有算法论文与技术路线表》+ 可行性结论，作为 M5 实现与验证基线。

## 2. 调研对象（A/B 分类 + MaxDEA 官方条目对照）

**A 类（完全缺失，MaxDEA 真独有）**：MinDS、FDEF、博弈交叉效率（Nash）、EBM、HMB、Cluster 基准、AR/Cone —— 优先调研，决定项目可行性上限。
**B 类（部分缺失 / 变体缺失）**：修正 SBM（Sharp 2007 / Lin 2019）、Malmquist 7 种参比 —— 常规路线即可覆盖主体，仅补变体。

| # | 类 | 算法 | MaxDEA 官方功能条目对照（http://maxdea.com/Features_X.htm） | 已知基线论文（待验证/扩充） |
|---|---|---|---|---|
| 1 | A | MinDS / Closest Target（强单调算法） | Minimum Distance to Strong Efficient Frontier (Closest Target) | Aparicio 2007; Jahanshahloo 2012/2013; Olyaie 2014; Aparicio 2017; Zhu 2018; Zhu 2022 |
| 2 | A | FDEF（顶点+超平面方程输出） | 可输出所有 FDEF 顶点与超平面方程（分段前沿生产函数） | Olesen & Petersen 1996; Jahanshahloo 2005/2009 及后续 |
| 3 | A | 博弈交叉效率（Nash） | Game Cross Efficiency model: Nash Equilibrium model | Liang, Wu, Cook & Zhu 2008; Wu, Liang & Yang 2009 |
| 4 | A | EBM 混合距离 | Hybrid Distance (EBM, Tone & Tsutsui 2010) | Tone & Tsutsui 2010 |
| 5 | A | HMB（Hicks-Moorsteen） | TFP Index: Hicks-Moorsteen Index (HMB) | O'Donnell 2010/2012 |
| 6 | A | Cluster 基准模型（8 种基准） | Cluster model（自/交叉/上下/相邻/窗口/固定基准） | MaxDEA 特有体系，对应 Benchmarking 文献 |
| 7 | A | AR / Cone 受限乘数 | Restricted multiplier model (Cone Ratio / Assurance Region) | Thompson 1990 (AR); Charnes 1990 (Cone) |
| 8 | B | 修正 SBM | Modified SBM (Sharp et al. 2007; Lin et al. 2019) | Sharp, Meng & Liu 2007; Lin et al. 2019 |
| 9 | B | Malmquist 多参比 | Malmquist：Adjacent/Fixed/Global/Sequential/Window/Global+Sequential | Caves 1982; Färe 1992/1994; Pastor & Lovell 2005 等 |

## 3. 检索渠道（8 路并行 + 规则）

1. **GitHub 代码搜索**：关键词 + topics（dea / closest-target / minDS / cross-efficiency / fdef）；限定语法 `in:name` / `in:description`；按 stars、license 过滤；含组织级搜索
2. **Google Scholar / Semantic Scholar**：引用网络、代码附件、补充材料
3. **arXiv**：预印本，重点追 MinDS 最新进展
4. **期刊补充材料**：EJOR / OMEGA / JORS / Applied Mathematical Modelling 等官网 supplementary（仅公开可下载）
5. **CRAN**：R 包 deaR、Benchmarking、rDEA、closesttargets 及同类新包
6. **Julia 生态**：DataEnvelopmentAnalysis.jl 及衍生包
7. **MATLAB File Exchange / 论文作者公开主页**：仅限公开可访问的静态资源，不向作者索取
8. **开源软件源码审计**：仅限许可证明确的开源项目（GPL/MIT/BSD 等）；Dearun 等未确认开源者只记录状态、不审计源码

**检索时间范围**：2000–2026；MinDS / FDEF 重点追踪 2015 年后最新方法（强单调算法）。

## 4. 检索词策略（每算法中英双语，至少 3 组组合词）

英文：minimum distance strong efficient frontier / closest target / strong monotonicity DEA / full dimensional efficient facets / FDEF vertices hyperplanes / game cross efficiency Nash equilibrium / epsilon-based measure EBM / Hicks-Moorsteen / cone ratio DEA / assurance region DEA / modified SBM Sharp Lin / global sequential window Malmquist
中文：最小距离 强有效前沿 / 强单调性 / 全维有效面 / 博弈交叉效率 纳什均衡 / 混合距离 EBM / 共同前沿 指数 / 锥比例 DEA / 保障区间 DEA / 修正SBM / 全局 序贯 窗口 Malmquist
规则：每算法至少 3 组关键词组合交叉检索；同一关键词跨渠道去重。

## 5. 评估维度（每个候选实现）

| 维度 | 说明 |
|---|---|
| 先进性 | 是否最新方法（MinDS 强单调 vs 早期两阶段法） |
| 可实现性 | 公式/伪代码完整度；所需求解器类型（LP/MILP/NLP） |
| 公开代码 | 有/无；语言；许可证；链接；可否直接运行（README/活跃度核验） |
| 验证数据 | 论文算例数据可获得性（正文表/补充材料/公开数据集，不含向作者索取） |
| 风险 | 计算复杂度（如 FDEF 顶点枚举指数级）、算法未完整公开等 |

## 6. 输出物与验收标准

1. **《独有算法论文与技术路线表》**：功能 / 推荐论文（题录+DOI+官方链接）/ 公开代码情况 / 实现路线建议 / 验证数据来源 / 风险备注
2. **GitHub 代码搜索结果清单**：仓库名+链接+星标+语言+许可证
3. **检索日志**（survey_log.md：关键词+渠道+日期）

**验收标准**：
- 每算法至少 3 篇候选论文（题录+DOI/官方链接）；不足 3 篇须如实标注「文献稀少」；
- 每算法给出 P0/P1/P2 路线结论与依据；
- 全部引用 URL 实际可访问并标注检索日期（2026-10-24 起）。

## 7. 优先级规则（P2 处置闭合，不含联系作者）

- **P0**：有完整开源实现 → 直接对拍验证 + 参考移植
- **P1**：有论文 + 完整算例数据 + 算法描述完整 → 自行实现 + 论文数值复现验证
- **P2**：算法描述不完整 / 无数据 → 标记为最高风险项，降级为「近似方法或与用户协商功能范围调整」；**不联系作者、不静默放弃**

## 8. 完成判据与兜底

- **完成判据**：8 路渠道均完成一轮检索且无新发现，或每算法达到候选论文下限；
- **兜底处置**：某算法全网无任何公开资料 → 如实报告为最高风险项，由用户决定功能范围调整（不编造来源、不静默降级）。

## 9. 衔接

- 调研结论 → 确认/调整 M5 实现范围；A 类任一项确认无法实现时，提前与用户协商降级方案；
- 调研报告并入 M0 预研报告交付（Markdown + 飞书文档双载体）。

## 10. 产出文件

- research_docs/unique_algorithms_survey.md（调研报告）
- research_docs/survey_log.md（检索日志，过程临时文件）
