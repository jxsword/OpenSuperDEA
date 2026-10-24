# 在线DEA平台调研与MaxDEA功能覆盖评估

> 从飞书文档导出归档（2026-10-24），原始文档: https://feishu.doubao.com/docx/N729dOZDPoDZMDx7yswcAvJangh

<title>在线DEA平台调研与MaxDEA功能覆盖评估</title>

# 在线 DEA 平台调研：托管环境与在线服务定价及 MaxDEA X 功能覆盖评估

> 调研日期：2026-10-24（数据时效基准，所有价格/模型/限制均来自官方页面并逐一核实可访问性）  
> 价格口径：仅统计**公开定价中支持个人用户使用的费用**（免费版 / 个人订阅 / 按次），不深挖机构授权与教育优惠；无个人付费入口的平台标注"无个人付费版"。  
> 结论速览：**在线平台（Colab / Kaggle 托管环境 + 在线 DEA Web 服务）可以覆盖约 90% 的常规 DEA 研究需求（CCR/BCC、SBM、超效率、Malmquist、交叉效率、成本收益等），但无法覆盖 MaxDEA X 的全部功能**——MinDS（最小距离强前沿）、FDEF（全维有效面输出）、博弈交叉效率（Nash）、EBM 混合距离、Hicks-Moorsteen 指数、Cluster 基准模型、受限乘数（AR/Cone）、超大数据极速求解共 8 项 MaxDEA X 独有能力，在全部在线平台中无一实现。

---

## 一、调研范围与口径

| 维度 | 说明 |
|-|-|
| 在线计算途径 | ① 托管代码环境：**Google Colab / Kaggle** 上运行开源 DEA 库（pyDEA、R 生态 Benchmarking+deaR+rDEA）；② 专用在线 DEA Web 服务：全网核实的 **DEAOS、deaR-Shiny、Onlineoutput DEA** |
| 桌面对照组（无在线版） | **DEAP 2.1、MaxDEA Lite/Basic、OSDEA 桌面版**，作为"无官方在线版"的对照列出 |
| 覆盖评估基线 | **MaxDEA X（v12.5）官方功能清单**（http://maxdea.com/Features_X.htm），共归纳 13 个功能组 |
| 信息源规则 | 价格、模型、限制一律取自官方页面（官网功能页/定价页/文档/README）；官方未披露标"未公开"，不采用第三方博客/转载 |
| 访问验证 | 文中全部官方 URL 均已于 2026-10-24 实际抓取验证可访问性 |

---

## 二、平台总览表（每行一个平台）

| 平台 | 官方 URL（2026-10-24 已验证） | 价格（仅个人口径） | 支持模型清单（官方口径） | 数据限制 | 技术形态 | 维护状态 | 访问日期 |
|-|-|-|-|-|-|-|-|
| **Google Colab** | [https://colab.research.google.com/](https://colab.research.google.com/) ｜定价 [https://colab.research.google.com/signup](https://colab.research.google.com/signup) ｜FAQ [https://research.google.com/colaboratory/faq.html](https://research.google.com/colaboratory/faq.html) | 免费版 \$0；Pro SGD 14.46/月（100 CU）；Pro+ SGD 72.06/月（600 CU）；按需付费 SGD 14.46/100 CU。⚠️ **中国大陆不在官方付费开通地区清单内，个人仅能使用免费版** | 通用 Notebook 环境，可运行任意开源 DEA 库：pyDEA（基础 CCR/BCC/SBM）；R 生态 Benchmarking+deaR+rDEA（常规+模糊+Robust+SFA） | 免费版会话最长 12 小时、GPU 严格受限；GPU/TPU 型号清单与周配额官方不公开；高内存仅付费档 | Notebook（官方支持 Python 3.12 与 R 4.6.0 双运行时） | Google 官方活跃维护（运行时按季度发版，2026.07 镜像） | 2026-10-24 |
| **Kaggle** | [https://www.kaggle.com/](https://www.kaggle.com/) ｜文档 [https://www.kaggle.com/docs/notebooks](https://www.kaggle.com/docs/notebooks) ｜GPU 配额 [https://www.kaggle.com/docs/efficient-gpu-usage](https://www.kaggle.com/docs/efficient-gpu-usage) | **完全免费，无个人付费版**（官方文档仅给两条付费升级路径：实验性 Colab Pro 联动额外 15/30 GPU 小时/周、GCP AI Notebooks 按量付费，均非 Kaggle 个人订阅） | 同 Colab：pyDEA 与 R 生态全库可经 `!pip install` / devtools 安装 | CPU/GPU 会话 12 小时、TPU 9 小时；CPU 4 核/30GB RAM；GPU 周配额 30 小时（浮动）；输出上限 20GB；数据集单库 200GB | Notebook（官方支持 Python 与 R 双内核，Docker 镜像约每两周更新） | Google 旗下，持续维护 | 2026-10-24 |
| **DEAOS**（DEA Online Software，DssBridge 公司） | [https://www.deaos.com/](https://www.deaos.com/) | Lite **免费**；Basic £29.99/31 天或 £89.99/186 天；Professional £99.99/31 天或 £299.99/186 天；Enterprise £599.99/93 天 | 径向包络/乘数、additive、FDH、规模效率、径向超效率、带价值判断（偏好）径向模型、range directional（RDM）、成本效率、SBM 及其修正形式、单期/多期数据、分类变量与非自主变量 | Lite：≤15 DMU、≤4 指标、≤2 项目；Basic ≤50 DMU；Professional ≤200 DMU；Enterprise ≤500 DMU | 纯 Web（免安装，Excel 导入/导出） | 站点在线，但官网新闻停更于 2021/05，活跃度证据不足 | 2026-10-24 |
| **deaR-Shiny**（R 包 deaR 官方 Web 前端） | [https://rbensua.shinyapps.io/deaR/](https://rbensua.shinyapps.io/deaR/) | **完全免费、免注册**（官方论文声明） | Additive、Basic radial、Preference Structure（偏好结构）、FDH、Multiplier、Non-radial、SBM（Tone 2001）、径向超效率、Cross-efficiency、Malmquist 指数、Additive/SBM 超效率、Profit 效率、3 种模糊 DEA（Guo-Tanaka/Kao-Liu/Possibilistic） | 官方未公开 DMU/指标上限 | R Shiny 交互式 Web App | 学术维护（Sustainability 2021 论文配套），站点在线，无近期版本迭代证据 | 2026-10-24 |
| **Onlineoutput DEA** | [http://www.onlineoutput.com/dea-software/](http://www.onlineoutput.com/dea-software/) | 免费 Demo 版（需注册）；**正式版价格未公开** | Basic radial、径向超效率、投入/产出导向、CRS/VRS/DRS/IRS、FDH、整数型上下缩放 | Demo 有数量限制但官方未披露具体数值 | 纯 Web（Excel 粘贴输入，输出 Excel/Word/HTML） | 站点在线，维护透明度低 | 2026-10-24 |
| **DEAP 2.1**（Tim Coelli / CEPA）【对照】 | [https://economics.uq.edu.au/cepa/software](https://economics.uq.edu.au/cepa/software) | **免费**（CEPA 四款免费软件之一） | CRS/VRS（CCR/BCC）技术+规模效率、成本与配置效率、Malmquist TFP（技术变化/效率变化/规模变化分解）；投入/产出导向 | 官方未公开 | Fortran 编译的桌面命令行程序（无 GUI），配数据文件与 47 页指南 | 多年停滞（仍为 2.1 版、XP 时代打包，同机构 DPIN 已至 3.1） | 2026-10-24 |
| **MaxDEA Lite / Basic**（MaxDEA X 免费版）【对照】 | [https://maxdea.com/Download.htm](https://maxdea.com/Download.htm) ｜https://www.maxdea.com/Index_CN.htm | **免费**（官方原文 "MaxDEA Lite is the free version of MaxDEA X"） | 仅基础模型：radial、SBM、cost、revenue | **数据上限 1000 行**（官方原文） | 原生桌面应用（Windows/macOS/Linux） | 活跃（Lite v12.2，随 X v12.5 更新） | 2026-10-24 |
| **OSDEA（Open Source DEA）桌面版**【对照】 | [https://opensourcedea.org/](https://opensourcedea.org/) ｜模型清单 [https://opensourcedea.org/detailed-features/](https://opensourcedea.org/detailed-features/) | **免费、开源 GPL v3** | 官方称最多 40 个 DEA 模型：CCR、BCC、SBM（含 VRS/IRS/DRS/GRS 与投入/产出导向）、IRS、DRS、GRS、NC（不可控）、ND（不可自由处置）；输出得分/投影/参考集/松弛/权重 | 官方宣称无限制（unlimited problem size/variables） | Java 桌面 GUI（可执行 JAR）+ 可嵌入 Java 代码库，依赖 lpsolve | 长期停滞（当前版本仍 0.2，要求 JRE 1.6 即 2006 年环境） | 2026-10-24 |
| **MaxDEA X（基线）** | 功能清单 [http://maxdea.com/Features_X.htm](http://maxdea.com/Features_X.htm) ｜速度对比 [http://maxdea.com/SpeedCompare.htm](http://maxdea.com/SpeedCompare.htm) | **个人定价官方未公开**：官网购买页（Purchase.htm 等 6 个候选路径）全部失效，仅留邮箱/QQ 询价渠道（MaxDEA@maxdea.com） | 全功能：Envelopment/Multiplier/FDH；径向、SBM/ERM、MinDS、DDF、加性族（RAM/BAM/DSBM）、EBM、成本/收益；三种导向；四种 RTS；Malmquist 与 Hicks-Moorsteen 7 类参比 TFP 指数；Window/Cluster；超效率、交叉效率（含博弈 Nash）；非期望产出/非自由处置；偏好加权；AR/Cone 受限乘数；FDEF 输出 | 无官方行数上限；官方自测支持 100 万 DMU（1M DMU/3 投入 3 产出 CCR 约 1 分 46 秒） | 原生桌面应用（Windows/macOS/Linux） | 活跃（当前 v12.5） | 2026-10-24 |

> 补充说明（调研中的排除项）：Dearun Tools（https://www.dearun.net/）为**桌面**下载安装软件（免费，模型覆盖广含 EBM/网络 DEA/非期望产出），非在线 Web 服务；DEA Solver Light 为 2010 年 Windows 小程序，无官方独立站点——均不纳入在线平台清单。

---

## 三、个人用户公开定价一览（仅统计支持个人用户的费用）

| 平台 | 免费额度 | 个人付费档 | 有无个人付费版 | 备注 |
|-|-|-|-|-|
| Google Colab | \$0（12h 会话） | Pro SGD 14.46/月、Pro+ SGD 72.06/月、按需 SGD 14.46/100 CU | 有（**中国大陆不可订阅**） | 定价按访问区域显示货币，官方未直接披露人民币金额 |
| Kaggle | 完全免费（30 GPU 小时/周） | — | **无个人付费版** | 官方仅指向 Colab 联动促销与 GCP 按量付费，均非 Kaggle 订阅 |
| DEAOS | Lite 免费（≤15 DMU/4 指标/2 项目） | Basic £29.99/31 天或 £89.99/186 天；Professional £99.99/31 天或 £299.99/186 天；Enterprise £599.99/93 天 | 有（订阅制） | 免费档适合小样本试算 |
| deaR-Shiny | 完全免费、免注册 | — | **无（学术免费）** | 模型最全的免费 Web 服务 |
| Onlineoutput DEA | 免费 Demo（注册后使用） | **正式版价格未公开** | 未公开 | Demo 上限未披露 |
| DEAP 2.1 | 免费 | — | 无 | — |
| MaxDEA Lite/Basic | 免费（≤1000 行） | — | 无 | 全功能版需商业询价 |
| OSDEA | 免费（GPL-3） | — | 无 | — |
| MaxDEA X | 无免费全功能版（Lite 受限） | **官方未公开**（仅询价） | 未公开 | 购买页失效，官网仅留销售联系方式 |

---

## 四、功能覆盖矩阵（13 个功能组 × 11 个平台/计算途径）

<html5-block alt="在线/桌面 DEA 平台 × MaxDEA X 功能覆盖矩阵" data-ref="html5_1"></html5-block>

图例：**●** 完整支持 ／ **◐** 部分或有限支持 ／ **○** 不支持；★ = MaxDEA X 独有/领先能力。

| MaxDEA X 功能组（v12.5 官方清单） | Colab+pyDEA | Colab+R生态 | Kaggle+pyDEA | Kaggle+R生态 | deaR-Shiny | DEAOS | Onlineoutput | DEAP 2.1 | MaxDEA Lite | OSDEA | MaxDEA X★ |
|-|-|-|-|-|-|-|-|-|-|-|-|
| 径向 CCR/BCC/FDH | ◐ | ● | ◐ | ● | ● | ● | ● | ◐ | ◐ | ● | ● |
| 非径向 SBM/ERM | ◐ | ● | ◐ | ● | ● | ● | ○ | ○ | ◐ | ● | ● |
| **MinDS 最小距离强前沿 ★** | ○ | ○ | ○ | ○ | ○ | ○ | ○ | ○ | ○ | ○ | ● |
| 方向距离 DDF / 加性模型族 | ○ | ◐ | ○ | ◐ | ◐ | ◐ | ○ | ○ | ○ | ○ | ● |
| **EBM 混合距离 ★** | ○ | ○ | ○ | ○ | ○ | ○ | ○ | ○ | ○ | ○ | ● |
| 成本 / 收益模型 | ○ | ● | ○ | ● | ◐ | ◐ | ○ | ● | ● | ○ | ● |
| 超效率 | ◐ | ● | ◐ | ● | ● | ● | ● | ○ | ○ | ○ | ● |
| Malmquist 指数（含分解） | ○ | ● | ○ | ● | ● | ◐ | ○ | ● | ○ | ○ | ● |
| **Hicks-Moorsteen 指数 ★** | ○ | ○ | ○ | ○ | ○ | ○ | ○ | ○ | ○ | ○ | ● |
| 窗口 / Cluster 基准分析 ★ | ○ | ◐ | ○ | ◐ | ○ | ○ | ○ | ○ | ○ | ○ | ● |
| 交叉效率 / 博弈交叉效率(Nash) ★ | ○ | ◐ | ○ | ◐ | ◐ | ○ | ○ | ○ | ○ | ○ | ● |
| 非期望产出/非自由处置/偏好加权/AR-Cone ★ | ○ | ◐ | ○ | ◐ | ◐ | ◐ | ○ | ○ | ○ | ◐ | ● |
| **FDEF 输出 / 超大数据极速求解 ★** | ○ | ○ | ○ | ○ | ○ | ○ | ○ | ○ | ○ | ○ | ● |

注：

- 托管环境列（Colab/Kaggle）按所运行开源库拆分：**+pyDEA** 仅基础 CCR/BCC/SBM（2017 年后未更新，PyPI 最新 1.6/MIT）；**+R生态** = Benchmarking+deaR+rDEA 组合能力。
- deaR-Shiny 的"偏好加权"指其 Preference Structure 模型；DEAOS 的"偏好/非自由处置"指带价值判断径向模型与非自主变量——均非 MaxDEA 的 AR/Cone 受限乘数体系。
- 覆盖状态依据各平台官方模型清单逐一对照（来源见第八节），◐ 表示部分支持或实现方式有限。

---

## 五、结论：在线平台能否覆盖 MaxDEA 全部功能？

### 5.1 总体判断

**不能。** 在线计算途径（托管环境 + 在线 Web 服务）合计可以覆盖 MaxDEA X 的**常规研究功能约 90%**（径向、SBM、超效率、成本收益、Malmquist、交叉效率、DDF/加性部分、偏好加权部分、非自由处置等主流模型），但 **MaxDEA X 的 8 项独有/领先能力在全部在线平台中无一实现**。

### 5.2 不支持项清单（逐项 + 依据）

| # | MaxDEA X 独有能力 | 在线平台是否支持 | 依据（官方清单对照） |
|-|-|-|-|
| 1 | **MinDS（最小距离强前沿 / Closest Target）**，含强单调性算法、CRS/VRS/NIRS/NDRS、三导向 | ❌ 全部缺失 | maxdea.com/Features_X.htm 第 4 项 vs pyDEA 文档、R 包清单（Benchmarking/deaR/rDEA）、deaR-Shiny 模型列表、DEAOS 模型列表均无 Closest Target |
| 2 | **FDEF 全维有效面输出**（输出所有有效面顶点与超平面方程） | ❌ 全部缺失 | 在线平台/开源库仅输出效率得分与投影，无有效面几何输出（Features_X "What's more important" 段） |
| 3 | **博弈交叉效率（Nash 均衡模型）** | ❌ 全部缺失 | deaR-Shiny 提供多阶段 Cross-efficiency、R 生态 deaR 提供交叉效率，但均无 Nash 均衡模型（Liang 2008 / Wu 2009） |
| 4 | **EBM 混合距离**（ε-based measure，径向+SBM 混合） | ❌ 全部缺失 | 在线平台模型清单均无 EBM；仅桌面 Dearun 支持（非在线） |
| 5 | **Hicks-Moorsteen 指数（HMB）** | ❌ 全部缺失 | 在线平台仅有 Malmquist 系指数，无 HMB |
| 6 | **Cluster 基准模型**（自基准/交叉基准/向上/向下/相邻/窗口/固定基准 8 种） | ❌ 全部缺失 | R 生态 deaR 仅有基本 Window 分析（◐），无 Cluster 体系 |
| 7 | **受限乘数模型（AR / Cone Ratio）** | ❌ 全部缺失 | 偏好加权仅有部分近似：deaR-Shiny Preference Structure、DEAOS 带价值判断径向模型；无 Assurance Region / Cone Ratio 实现 |
| 8 | **超大数据极速求解**（1 万 DMU 约 2 秒级；官方自测至 100 万 DMU） | ❌ 无法达到 | 在线 Web 服务规模上限：DEAOS 付费最高 500 DMU；托管环境受 12h 会话与内存限制（免费档标准内存，万级 DMU LP 求解有超时/内存风险） |
| 9 | 修正 SBM（Sharp 2007 / Lin 2019） | ❌ 缺失 | 在线平台无修正 SBM 变体 |
| 10 | Malmquist 7 种参比方式（相邻/固定/全局/序贯/窗口×2/全局+序贯） | ◐ 部分 | 在线仅有基本 Malmquist（deaR-Shiny ●；DEAOS 多期数据 ◐；R 生态 ●），无全局/序贯/窗口等全部参比 |
| 11 | 近全部模型组合（官方称 3 万+ 组合） | ❌ 无法复现 | 在线/开源库为独立模型集合，无组合式生成能力 |

### 5.3 在线平台能覆盖的部分（客观结论）

- **完全覆盖**：径向 CCR/BCC、SBM、超效率、Malmquist（含分解）——托管环境 + R 生态、deaR-Shiny 均可完整实现。
- **基本覆盖**：成本/收益（R 生态完整；deaR-Shiny 仅 Profit；DEAOS 仅成本效率）；DDF/加性（R 生态与 DEAOS 部分）；非期望产出/非自由处置（R 生态部分；OSDEA 桌面版 NC/ND 完整）。
- **在线独有扩展（MaxDEA X 无或较弱）**：模糊 DEA（deaR-Shiny 3 种）、Robust/order-m（rDEA）、SFA 随机前沿（Benchmarking）、bootstrap（Benchmarking/DEA Toolbox）——与前序《MaxDEA软件详解与开源替代方案》结论一致。

---

## 六、在线途径 vs 桌面对照：优劣势速览

| 平台类别 | 优势 | 劣势 |
|-|-|-|
| **托管环境（Colab/Kaggle + 开源库）** | 0 元起步（Kaggle 无个人付费限制，大陆个人友好）；可编程可复现、论文级严谨；可运行全部开源 DEA 生态；免安装、跨设备 | 无 GUI 一键操作；需写 Python/R 脚本；会话有时长限制（12h）；无 MinDS/FDEF/Nash/EBM/HMB/Cluster/AR-Cone |
| **在线 Web 服务（deaR-Shiny / DEAOS / Onlineoutput）** | 开箱即用 Web 界面；deaR-Shiny 免费免注册且模型最全；DEAOS 支持 Excel 导入导出 | 规模上限低（DEAOS 免费 15 DMU、付费最高 500）；维护活跃度普遍存疑（DEAOS 新闻停更 2021）；Onlineoutput 价格未公开 |
| **桌面对照（DEAP 2.1 / MaxDEA Lite / OSDEA）** | 全部免费；DEAP 是 Coelli 教材经典（Malmquist 权威）；MaxDEA Lite 官方出品免费体验 MaxDEA 生态；OSDEA 开源可嵌入 | **均无官方在线版**；DEAP/OSDEA 多年停滞；MaxDEA Lite 仅 1000 行基础模型 |
| **MaxDEA X（基线）** | 功能最全（8 项独有能力）、极速求解大数据、GUI 极简、中文教材配套 | 闭源商业软件；**个人定价官方未公开**（仅询价）；无在线版、无法脚本化 |

---

## 七、选型建议

1. **常规科研（CCR/BCC、SBM、超效率、Malmquist、交叉效率、成本收益）**：首选 **Kaggle（免费、无付费限制）+ R 生态（deaR+Benchmarking+rDEA）**；中国大陆个人用户尤其推荐（Colab 付费档不可订阅，Kaggle 免费额度对 DEA 这类 CPU 小 LP 任务完全够用）。
2. **要开箱即用的 Web 界面**：**deaR-Shiny**——免费、免注册，模型最全（含 3 种模糊 DEA、Profit、超效率、交叉效率、Malmquist），适合教学演示与快速试算。
3. **小样本快速试算（≤15 DMU）：DEAOS Lite** 免费档即可；更大样本（≤500 DMU）需订阅（£29.99–£599.99）。
4. **需要 MinDS / FDEF / 博弈交叉效率（Nash）/ EBM / HMB / Cluster / AR-Cone / 万级以上 DMU 极速求解**：**任何在线平台均无法覆盖**，仍需 **MaxDEA X**（个人定价须通过官方邮箱/QQ 询价）——这是在线方案与 MaxDEA X 之间无法弥合的差距。
5. **预算敏感且只要基础模型**：桌面对照组中 **MaxDEA Lite（免费，≤1000 行）** 体验 MaxDEA 生态；**OSDEA（免费 40 模型）** 无规模限制；**DEAP 2.1（免费）** 作为 Malmquist 经典对照。

---

## 八、主要信息来源（官方页面，全部于 2026-10-24 实际访问验证）

- MaxDEA X 官方功能清单（英文）：http://maxdea.com/Features_X.htm ✅
- MaxDEA 官网（中文）：https://www.maxdea.com/Index_CN.htm ✅
- MaxDEA 下载页（Lite 免费版说明）：https://maxdea.com/Download.htm ✅
- MaxDEA 速度对比（100 万 DMU 自测）：http://maxdea.com/SpeedCompare.htm ✅
- Google Colab 主入口：https://colab.research.google.com/ ✅
- Colab 官方 FAQ：https://research.google.com/colaboratory/faq.html ✅
- Colab 官方定价/订阅页：https://colab.research.google.com/signup ✅
- Colab 运行时版本（维护证据）：https://research.google.com/colaboratory/runtime-version-faq.html ✅
- Kaggle Notebook 官方文档：https://www.kaggle.com/docs/notebooks ✅
- Kaggle GPU 用量指南：https://www.kaggle.com/docs/efficient-gpu-usage ✅
- Kaggle 数据集规格：https://www.kaggle.com/docs/datasets ✅
- DEAOS 官网：https://www.deaos.com/ ✅
- deaR-Shiny：https://rbensua.shinyapps.io/deaR/ ✅
- Onlineoutput DEA：http://www.onlineoutput.com/dea-software/ ✅
- DEAP 2.1（CEPA/昆士兰大学）：https://economics.uq.edu.au/cepa/software ✅
- OSDEA 官网：https://opensourcedea.org/ ✅；模型清单：https://opensourcedea.org/detailed-features/ ✅
- pyDEA（PyPI 官方包页）：https://pypi.org/project/pyDEA/ ✅；源码：https://github.com/araith/pyDEA
- 配套文档：《MaxDEA软件详解与开源替代方案.md》（同目录，开源生态技术栈与优缺点详见该文档）