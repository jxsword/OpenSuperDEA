# OpenSuperDEA M0 预研报告：全量实现 MaxDEA 的可行性论证与实施规划

> 版本：v1.0 ｜ 2026-10-24 ｜ 状态：M0 预研交付
> 依据材料：《MaxDEA 软件详解与开源替代方案》《在线 DEA 平台调研与 MaxDEA 功能覆盖评估》《独有算法论文与技术路线表 v4.0》（本报告为三者整合）
> 所有外部来源均标注访问日期 2026-10-24

## 执行摘要（核心结论）

- **核心问题**：借助 AI Agent 工具（Zcode）+ 自研工程能力，能否完全实现商业软件 MaxDEA X 的全部功能（包括所有在线平台和开源工具均未实现的功能）？
- **结论：可行**。MaxDEA X 的功能差距集中在 10 项独有算法（在线平台/开源工具均缺失 8 项），本预研确认 **10 项全部有明确论文路线，其中 8 项有可直接对拍的开源实现或完整公式+算例，无一需要"从零发明"**。
- **关键发现 1**：MaxDEA 全部功能可按"常规模型族（径向/非径向/成本收益/超效率/交叉效率/窗口/TFP）+ 10 项独有算法"分解；前者有成熟开源生态，后者是本项目核心工作量。
- **关键发现 2**：独有算法中优先级 P0（有开源对拍）4 项、P1（论文+算例自行实现）6 项；最高风险集中在 FDEF 的 NDRS/NIRS、Weak EF 投影不唯一性、big-M 数值稳定性 3 处，均有缓解方案。
- **关键发现 3**：验证体系完整——deaR / DEAPack / R productivity / PyDEA 四套开源对拍 + 5 组黄金论文算例（32 打印机、32 机场、902 学校、Airlines 28、GRIPS EBM）+ 数学性质检验，覆盖全部 10 项。
- **关键发现 4**：性能目标（常规 10 万 DMU、径向 100 万 DMU）有官方基准（MaxDEA X 官方实测 10 万 DMU 3×3 BCC = 25.32 秒）与论文加速算法（Zhu-Wu-Song 2018 大 DEA 加速）作为参照。
- **影响**：实施路线为 CLI 先行（M1-M5）→ GUI 后置（M6）→ 平台扩展（M7-M8），C++20 技术栈（HiGHS 求解器）支撑全部算法族；预计全部功能可实现，唯一不可复现的是 MaxDEA 官方未公开的个人定价与商业授权条款（不影响功能实现）。
- **边界**：本报告结论基于官方功能清单（Features_X.htm）与公开文献/开源代码核验；Cluster 8 基准的"官方逐条定义"无公开文献完全对齐，按 Seiford & Zhu 2003 分层模型+参比组合推断并以黄金算例验证。

## 一图看懂：10 项独有算法实现路线总览

<!-- 此处插入 html5-block：算法路线矩阵（见 widget_algorithm_matrix.html） -->

**图读法**：矩阵按实现优先级（P0 有开源对拍 → P1 论文+算例 → P2 高风险延后）分组；每项算法给出主论文路线、开源对拍对象、黄金算例与实现依赖。10 项算法全部有"论文 + 验证"双闭环，是"可行"结论的直接证据。

---

## 1. 核心问题与可行性总判断

### 1.1 项目目标

以 AI Agent（Zcode）驱动，C++20 全量实现 MaxDEA X 全部功能，项目代号 OpenSuperDEA。开发环境 WSL Ubuntu（ubuntu2604），交付 CLI + GUI 双形态，支持 Windows/Ubuntu，macOS 作为后期扩展平台。

### 1.2 可行性总判断

**判断：可行，且无需"从零发明"任何算法。** 依据有三：

1. **功能差距已量化**：对照官方功能清单（9 大模型维度），在线平台与开源工具无法覆盖的部分收敛为 8 项——算法侧 7 项（MinDS、FDEF、博弈交叉效率 Nash、EBM、HMB、Cluster 基准、AR/Cone）+ 性能侧 1 项（万级 DMU 极速求解）。这 8 项即本项目与 MaxDEA 的差距全集。
2. **独有算法全部有解**：10 项独有算法（含复审新增的 Weak EF）逐项锁定论文路线——每项 ≥3 篇、题录全部可核验（唯一例外 Olyaie 2014 终判为公开索引缺失，已确认不影响实现）。
3. **工程基础成熟**：常规模型族（径向 CCR/BCC、SBM、超效率、成本/收益、交叉效率、窗口分析、TFP 指数）在开源生态（deaR/PyDEA/DEAPack/R productivity）中全部有实现，可作语义与数值双重参考；求解器 HiGHS（Apache-2.0）提供官方 C++ API，无许可冲突。

## 2. 功能全景与差距分析

### 2.1 MaxDEA X 官方功能清单（9 大维度）

| 维度 | 内容 | 缺口判定 |
|---|---|---|
| 模型类型 | 包络模型 / 乘子模型 / FDH | ✅ 常规 |
| 距离度量 | 径向（CCR/BCC）、ERM/SBM、**最小距离弱有效前沿**、**MinDS（Closest Target）**、方向距离（DDF/RDM/自定义）、加权加性族（9 种子模型）、**EBM**、成本、收益 | ⚠️ 含独有算法 |
| 定向 | 输入 / 输出 / 非定向 | ✅ 常规 |
| RTS | CRS / VRS / NIRS / NDRS + 规模效率分解 | ✅ 常规 |
| TFP 指数 | **Malmquist 7 种参比 + HMB** + 四类分解（EC/TC/SEC/bTC） | ⚠️ 含独有算法 |
| 窗口分析 | Window DEA | ✅ 常规 |
| **Cluster 模型** | **8 种基准：Self/Cross/Downward/Upward/Lower-adjacent/Upper-adjacent/Window/Fixed** | ❌ 独有 |
| 其他模型 | 超效率、修正 SBM×2（Sharp 2007 / Lin 2019）、交叉效率（Doyle-Green I/II + Ruiz + 自定义）、**博弈交叉效率（Nash）**、非期望产出、非自由处置、偏好加权、**受限乘数（Cone Ratio / Restricted Cone / AR）** | ⚠️ 含独有算法 |
| 组合能力 | 上述模型的近全组合 | ✅ 架构问题 |

### 2.2 在线平台与开源工具覆盖 vs 差距（8 项）

| 未覆盖项 | 在线平台（DEAOS/deaR-Shiny/Onlineoutput） | 开源库（deaR/DEAPack/PyDEA/productivity） |
|---|---|---|
| MinDS 最小距离强前沿 | ❌ 全部缺失 | ❌ 仅 deaR `model_addmin`（P0 对拍可用） |
| FDEF 全维有效面输出 | ❌ | ❌ |
| 博弈交叉效率（Nash 均衡） | ❌（仅 deaR 系多阶段交叉效率） | ⚠️ DEAPack `game_cross_efficiency` ✅ |
| EBM 混合距离 | ❌ | ❌（DEAPack declared，未 executable） |
| HMB 指数 | ❌（在线仅 Malmquist 系） | ⚠️ R productivity + DEAPack ✅ |
| Cluster 8 基准 | ❌（仅 deaR 基本窗口分析） | ❌ |
| 受限乘数 AR/Cone | ⚠️ 部分近似 | ⚠️ DEAPack cone_ratio + PyDEA AR ✅ |
| 万级 DMU 极速求解 | ❌（DEAOS 付费最高 500 DMU） | ⚠️ 托管环境受 12h/内存限制 |

**结论**：8 项差距中 7 项为算法能力、1 项为性能能力；开源库可为其中 5 项提供"数值对拍基准"，为 2 项提供"部分实现参考"，Cluster 无任何公开实现（以论文算例验证）。

## 3. 独有算法实现路线（核心章节）

### 3.1 总览表

| # | 算法 | 优先级 | 主论文路线 | 开源对拍 | 黄金算例 |
|---|---|---|---|---|---|
| 1 | MinDS（Closest Target，强单调） | P0/P1 | Zhu-Wu-Ji-Li 2018（MILP 强单调，Omega 79:1-8）→ Zhu 2022 扩展面；强单调机制：Aparicio-Pastor 2014（Omega 44:51-57）+ Fukuyama-Maeda-Sekitani-Shi 2014（EJOR 237:997-1007）；面向模型版：Aparicio-Cordero-Pastor（CORE 开放，902 校算例） | deaR `model_addmin`（4 RTS，mf/milp） | Airlines 28；Zhu 2018 算例；Sekitani-Zhao 2023 数值表 |
| 2 | FDEF（全维有效面） | P1 | CRS：Jahanshahloo-Roshdi-Davtalab-Olyaie 2013（IJIM 5(2):149-159，开放全文）MILP 枚举；VRS：QHULL 计算几何（Lins 2025 JPA 佐证）；超平面替代：Rafati-Maleki 2018 | 无（双路线互证） | IJIM 2013 算例；Lins 2025 公共招标 |
| 3 | 最小距离弱有效前沿（Weak EF） | P1 | Charnes-Rousseau-Semple 1996（JPA 7:5-18）主模型；概念关系：Mehdiloo-Podinovski 2021（EJOR 294:295-311，开放 PDF） | 无 | JPA 1996 算例 |
| 4 | 博弈交叉效率（Nash 均衡） | P0/P1 | Liang-Wu-Cook-Zhu 2008（OR 56:1278-1288）；VRS 版 Wu-Liang-Chen 2009；Wu-Liang-Zha 2009（JORSJ 52(2):105-111，ORSJ 开放全文） | DEAPack `game_cross_efficiency`（HiGHS） | Liang 2008 算例 |
| 5 | EBM 混合距离 | P1 | Tone-Tsutsui 2010（EJOR 207:1554-1563）+ GRIPS DP09-21 开放全文（完整公式 + ρ=1.471/ε=0.529 算例） | DEAPack（declared，实现期转可执行后对拍） | GRIPS DP09-21 算例 |
| 6 | HMB（Hicks-Moorsteen） | P0/P1 | Bjurek 1996 奠基；O'Donnell 2012 分解 | R productivity（CRAN 1.1.2）+ DEAPack `hicks_moorsteen` | 面板算例双实现对拍 |
| 7 | Cluster 8 基准 | P1 | Seiford-Zhu 2003（Omega 31:397-408，开放全文**已精读**）分层模型 + attractiveness/progress + VJ 版（权重=AR 下界）；SBM 版：Morita-Hirokawa-Zhu 2005（Omega 33:357-362）；交叉效率版：Lim 2011（JORS）；Fixed 基准：Tulkens-VandenEeckaut 1995（EJOR 80:474-499） | 无（8 基准 = 参考集组合规则参数化） | **32 打印机黄金数据**（Doyle-Green 1991：E¹-E⁴ 分层 + Table 3-6 全数值） |
| 8 | AR / Cone 受限乘数 | P0 | Cone Ratio：Charnes-Cooper-Wei-Huang 1989/1990；AR：Thompson 1990 + Wong-Beasley 1990；Closest 关联：Mehdiloo-Podinovski 2021 | DEAPack `cone_ratio`（polyhedral_crs）+ PyDEA assurance region | Kansas 农场 1990 算例 |
| 9 | 修正 SBM | P1 | Sharp-Liu-Meng 2007（JORS，自然负数据两阶段法）；Lin-Yang-Huang 2019（C&IE 135:39-52，10.1016/j.cie.2019.05.030） | 无 | Sharp 2007 算例 |
| 10 | Malmquist 多参比（7 种） | P0/P1 | Caves 1982 / Färe 1992/94；Global：Pastor-Lovell 2005；Sequential：Shestalova 2003；Window：Asmild 2004；技术集：Tulkens-VandenEeckaut 1995 | DEAPack（global/biennial/pooled）+ deaR（global/sequential）+ R productivity 三实现对拍 | 面板数据三实现互证 |

### 3.2 关键结论

- **P0 共 4 项**（MinDS、Nash、HMB、AR/Cone）：有直接开源对拍，实现风险最低，建议作为 M5 先行批。
- **P1 共 6 项**（FDEF、Weak EF、EBM、Cluster、修正 SBM、Malmquist 7 参比）：有完整公式论文 + 算例，自行实现 + 论文数值复现。
- **P2 高风险管理 2 处**：FDEF 的 NDRS/NIRS（官方宣称支持但公开文献稀缺）；Weak EF 弱投影不唯一性（实现取第一可行解并文档化）。
- **共性风险**：MinDS/Nash/Weak EF 共用 big-M 数值稳定性问题——实现期统一 big-M 自动估计器（基于数据范围 + 稀疏化）。

## 4. 验证与对拍体系

### 4.1 四层验证

| 层级 | 手段 | 覆盖 |
|---|---|---|
| L1 开源对拍 | deaR（MinDS/交叉效率/Malmquist）、DEAPack（Nash/AR-Cone/HMB/EBM）、PyDEA（AR）、R productivity（HMB/Malmquist） | 10 项中 7 项 |
| L2 论文算例复现 | 32 打印机（Cluster）、32 机场（Weak EF）、902 学校（MinDS 面向模型）、Airlines 28（MinDS）、GRIPS EBM（ρ/ε）、Liang 2008（Nash） | 全部 10 项 |
| L3 数学性质检验 | 强单调性、A*(d)>1 递减 / P*(g)>1 递增、增删无效 DMU 改变效率（Cluster 本质差异）、Malmquist 分解恒等式 | 独有算法核心性质 |
| L4 性能基准 | 官方 SpeedCompare（10 万 DMU 3×3 BCC=25.32s）为参照；自建基准：10 万常规 / 100 万径向 | 性能里程碑 M3/M4 |

### 4.2 许可边界

- **HiGHS（Apache-2.0）**：直接依赖，无冲突 ✅
- **deaR（GPL-2|3）/ DEAPack（GPL-3）/ R productivity（GPL-3）**：只作语义参考与数值对拍，**不移植代码**（运行期作为外部验证工具，非产品依赖）
- **PyDEA（MIT）**：可参考算法思路
- **Eigen（MPL2）/ Qt6（LGPL）**：无冲突 ✅

## 5. 性能目标与优化路径

| 里程碑 | 目标 | 参照 | 技术路径 |
|---|---|---|---|
| M3 | 常规 10 万 DMU | MaxDEA 官方 10 万 DMU 3×3 BCC=25.32s | HiGHS 并行求解、参考点采样 |
| M4 | 径向 100 万 DMU | 官方百万级基准 | 强有效集分化（Zhu-Wu-Song 2018 COR 大 DEA 加速：参考点采样/强有效集分化）、稀疏矩阵、分块求解 |

**技术参考**：Zhu, Wu & Song (2018) "Efficiency evaluation based on DEA in the big data context"（Computers & Operations Research，iranarze 开放 PDF）——参考点采样 + 强有效集分化，直接对应 M3/M4 性能目标。

## 6. 技术栈与工程架构

### 6.1 选型结论（C++ vs Rust，已论证）

| 维度 | C++20（选定） | Rust |
|---|---|---|
| 求解器集成 | HiGHS 官方 C++ API，语义零损耗 | 需 FFI 包装（cxx/bindgen），增加一层 |
| 算法移植 | 论文/MATLAB 语义（矩阵/向量运算）接近，Eigen 直译 | 借用检查器增加移植摩擦 |
| GUI | Qt6 + CMake 生态成熟 | Qt 绑定（cxx-qt）相对薄弱 |
| 工程工具链 | vcpkg + CMake + GitHub Actions 成熟 | 工具链较新但生态在追赶 |
| 安全 | 需纪律（RAII/智能指针） | 语言级保证 |

**结论**：C++20 胜出——HiGHS 官方 API、MATLAB 移植语义接近、Qt6/CMake 既有经验；Rust 仅"安全/工具链"优势，不减少算法移植工作量。平台：WSL Ubuntu 开发 → Windows 原生 → macOS（后期）。

### 6.2 技术栈清单

| 层 | 选型 |
|---|---|
| 语言 | C++20（+ CMake 3.2x） |
| 依赖管理 | vcpkg（manifest 模式） |
| 线性代数 | Eigen 3.4 |
| 求解器 | HiGHS（LP/MILP，官方 C++ API） |
| 计算几何 | QHULL（FDEF VRS 路线） |
| CLI | CLI11 / 自研参数解析（对标 MaxDEA 菜单项） |
| GUI（M6） | Qt 6（Widgets + Charts） |
| 数据 IO | CSV / Excel 导入导出（xlsx 库或自研） |
| CI | GitHub Actions：WSL Ubuntu → Actions Linux → Actions Windows 三段式 |
| 文档 | AsciiDoc（用户已确认可用） |

### 6.3 架构分层

```
OpenSuperDEA
├── core/          # 引擎：数据模型、参考集、LP/MILP 构造、求解封装
│   ├── radial/    # CCR/BCC/NIRS/NDRS + 定向 + 规模分解
│   ├── nonradial/ # SBM/ERM/加性族/修正 SBM/EBM
│   ├── distance/  # DDF/RDM/弱前沿/强前沿(MinDS)/FDEF
│   ├── cross/     # 交叉效率/博弈 Nash/AR-Cone
│   ├── cluster/   # 分层/8 基准
│   └── tfp/       # Malmquist 7 参比/HMB/分解
├── cli/           # 命令行前端（M1 先行）
├── gui/           # Qt6 前端（M6）
├── tests/         # 对拍脚本（deaR/DEAPack/productivity/PyDEA）+ 黄金算例 + 性质检验
└── bench/         # 性能基准（10 万/100 万 DMU）
```

## 7. 里程碑规划（M0-M8）

| 里程碑 | 内容 | 关键交付/验收 |
|---|---|---|
| **M0** | 预研（本报告） | 可行性论证 + 算法路线 + 验证体系 ✅ |
| **M1** | 工程底座 | CMake+vcpkg+HiGHS/Eigen 集成、CLI 框架、数据 IO、三段式 CI 跑通 |
| **M2** | 常规引擎 | CCR/BCC/SBM/超效率/成本收益/FDH/方向距离/加性族/Malmquist 基础——与 deaR/PyDEA 全量对拍 |
| **M3** | 性能优化 | 10 万 DMU 常规模型达标（对照官方 25.32s） |
| **M4** | 极速求解 | 100 万 DMU 径向；参考点采样/强有效集分化 |
| **M5** | 独有算法 | FDEF(CRS)→MinDS(Zhu 2018)→扩展面(Zhu 2022)→WeakEF→Nash/AR-Cone/修正SBM→Cluster→Malmquist 7 参比+HMB；逐项黄金算例对拍 |
| **M6** | GUI（Qt6） | 对标 MaxDEA 界面：模型选择/结果表/投影/FDEF 超平面输出/Cluster 分层图 |
| **M7** | 平台扩展 | Windows 原生打包、macOS 适配（后期）；安装器 |
| **M8** | 发布 | 文档（AsciiDoc）、许可审计、发布验证、性能报告 |

**执行原则**：CLI 全部功能先行（M1-M5 只做 CLI），GUI 复用 CLI 引擎（M6 只做界面层）；每里程碑以"对拍+算例+性质"三重验证为完成标准。

## 8. 开发流程与 CI

1. **开发环境**：WSL `ubuntu2604`（用户 ssy），项目 `/home/ssy/OpenSuperDEA`；Windows 宿主中转草稿。
2. **CI 三段式**：先在 WSL 本地全量测试通过 → 推送 GitHub Actions（Linux）→ 再启 Actions Windows 测试。仓库已关联 GitHub（gh 已登录），推送需用户确认后执行。
3. **测试策略**：单元测试（模型数值）+ 集成测试（对拍脚本，Python/R 外部进程调用）+ 性质测试（强单调/分层性质）+ 性能基准（bench）。
4. **Git 规范**：按里程碑分步提交，AGENTS.md 约束（Doxygen 中文注释规范）先行建立。

## 9. 风险与缓解

| 风险 | 等级 | 缓解 |
|---|---|---|
| FDEF 的 NDRS/NIRS 公开文献稀缺 | 高（P2） | 以 CRS/VRS 双路线先闭环，NDRS/NIRS 延后；向算法社区验证 |
| Weak EF 弱投影不唯一性 | 高（P2） | 取第一可行解 + 文档化；与 Mehdiloo-Podinovski 2021 概念对照 |
| big-M 数值稳定性（MinDS/Nash/WeakEF） | 中 | 统一 big-M 自动估计器（数据范围+稀疏化）；对拍校验 |
| EBM 谱分解数值误差 | 中 | 用 GRIPS 算例 ρ/ε 精确复现（1e-8 级） |
| GPL 许可边界 | 中 | 只参考不移植；对拍脚本独立于产品源码 |
| Cluster 官方逐条定义未公开对齐 | 中 | Seiford-Zhu 2003 + 参比组合推断；32 打印机黄金算例验证 |
| QHULL 高维（m+s>10）性能 | 中 | 实现期实测；备选 MILP 枚举路线 |
| MaxDEA 官方未公开定价/许可细节 | 低 | 不影响功能实现；仅商业对标参考 |

## 10. 参考来源

[1] MaxDEA 官方功能清单：https://maxdea.com/Features_X.htm（访问 2026-10-24）
[2] MaxDEA 下载/免费版说明：https://maxdea.com/Download.htm（访问 2026-10-24）
[3] MaxDEA 性能基准：https://maxdea.com/SpeedCompare_CN.htm（访问 2026-10-24）
[4] 在线 DEA 平台（DEAOS：https://www.deaos.com/ ；deaR-Shiny：https://rbensua.shinyapps.io/deaR/ ；Onlineoutput：http://www.onlineoutput.com/dea-software/ ；Colab：https://colab.research.google.com/signup ；Kaggle：https://www.kaggle.com/docs/notebooks）（访问 2026-10-24）
[5] DEA Frontier 开放文库（Seiford-Zhu 2003 全文）：https://www.deafrontier.net/papers/OMEGAContext.pdf（访问 2026-10-24）
[6] GRIPS EBM 工作论文：https://grips.repo.nii.ac.jp/record/1021/files/DP09-21.pdf（访问 2026-10-24）
[7] ORSJ 开放全文（Wu-Liang-Zha 2009）：https://orsj.org/wp-content/or-archives50/pdf/e_mag/52-2-105-111.pdf（访问 2026-10-24）
[8] IJIM 开放全文（Jahanshahloo 2013）：https://journals.srbiau.ac.ir/article_2148_59fc5d8bc1164ec7224d8f1cee1b68c6.pdf（访问 2026-10-24）
[9] Zhu-Wu-Song 2018 大 DEA 加速（iranarze 开放 PDF）（访问 2026-10-24）
[10] arXiv 开放资源：arXiv:1503.09095（Mehdiloozad-Ahmadi 2015）、arXiv:2312.15422（Sekitani-Zhao 2023）、arXiv:1409.7546（Akbarian 2014）（访问 2026-10-24）
[11] 完整论文题录与 DOI：见《独有算法论文与技术路线表 v4.0》（research_docs/unique_algorithms_survey.md）

---

*本报告为 OpenSuperDEA 项目 M0 预研交付物。全部算法结论可追溯至 v4.0 调研报告；所有外部 URL 于 2026-10-24 核验可访问。*
