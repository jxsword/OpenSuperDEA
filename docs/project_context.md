# OpenSuperDEA 项目上下文（会话知识固化）

> 本文档固化历史会话的全部关键决策、调研结论与开发约束，供后续 zcode 连接 WSL 开发时直接读取。
> 生成日期：2026-10-24 ｜ 来源：历史会话（已不可回读）+ research_docs 全部调研交付物
> 配套索引：docs/deliverables_index.md（全部交付物与 URL 清单）

## 1. 项目定位

- **目标**：用 AI Agent（Zcode）+ C++ 全量实现商业软件 MaxDEA X 全部功能，项目代号 OpenSuperDEA。
- **覆盖范围**：常规模型族 + 10 项独有算法（在线平台与开源工具均未实现）+ 万级 DMU 极速求解。
- **形态**：CLI 先行（M1-M5）→ GUI 后置（M6，Qt6）；支持 Ubuntu（WSL）/ Windows，macOS 后期扩展。
- **当前状态**：M0 预研完成；需求工程完成（SRS V1.1）；M1 完成验收（v0.1.0）；**M2 常规引擎开发完成（2026-10-25，release/v0.2.0 待验收：ctest 102/102 + L1 对拍 68/68 + 覆盖率 85%，测试报告 design_docs/reports/m2_test_report.md）**；M3 待启动。

## 2. 技术栈决策（已论证，勿推翻）

| 层 | 选型 | 备注 |
|---|---|---|
| 语言 | **C++20**（DR-007，2026-10-24 用户确认，M2 首分支执行；M1 v0.1.0 为 C++17 基线） | naming_convention/.clang-tidy 评估无需修改 |
| 构建 | CMake（当前 3.16+，target `opensuperdea`） | 项目级变量/选项前缀 `OPENSUPERDEA_` |
| GUI | Qt 6（Widgets，骨架已移入 tmp/gui_skeleton/，M6 恢复；恢复步骤见该目录 README.md） | AUTOMOC/AUTOUIC/AUTORCC 已开 |
| 依赖管理 | vcpkg（manifest 模式，M1 引入） | |
| 线性代数 | Eigen 3.4（M1 引入） | |
| 求解器 | **HiGHS**（LP/MILP 官方 C++ API，M1 引入） | Apache-2.0，无许可冲突 |
| 计算几何 | QHULL（FDEF VRS 路线，M5 引入） | |
| CLI | CLI11（M1 引入） | |
| CI | GitHub Actions 三段式 | WSL 本地 → Actions Linux → Actions Windows |
| 文档 | AsciiDoc（已确认可用）/ Markdown | |

**C++ vs Rust 论证结论（C++ 胜出）**：
- HiGHS 官方 C++ API 语义零损耗（Rust 需 FFI 包装）
- 论文/MATLAB 算法语义与 Eigen 直译接近（Rust 借用检查器增加移植摩擦）
- Qt6/CMake/vcpkg 生态成熟（Rust cxx-qt 相对薄弱）
- Rust 仅"安全/工具链"优势，不减少算法移植工作量

**许可边界**：HiGHS（Apache-2.0）、Eigen（MPL2）、Qt（LGPL）、QHULL 直接依赖无冲突；deaR（GPL-2|3）、DEAPack（GPL-3）、R productivity（GPL-3）**只对拍不移植**；PyDEA（MIT）可参考。

## 3. 功能差距与独有算法路线（调研结论，勿重复调研）

### 3.1 差距全集（8 项 = 算法 7 + 性能 1）

MinDS、FDEF、博弈交叉效率（Nash）、EBM、HMB、Cluster 8 基准、AR/Cone + 万级 DMU 极速求解。
（在线平台 DEAOS/deaR-Shiny/Onlineoutput 与托管环境 Colab/Kaggle 均无法覆盖，见 02_online_dea_platform_survey.md）

### 3.2 10 项独有算法路线摘要（完整版见 unique_algorithms_survey.md v4.0）

| # | 算法 | 优先级 | 主论文（年份） | 开源对拍 | 黄金算例 |
|---|---|---|---|---|---|
| 1 | MinDS（强单调） | P0/P1 | Zhu 2018 → Zhu 2022；Aparicio-Pastor 2014；Fukuyama 2014；Aparicio-Cordero-Pastor（oriented） | deaR `model_addmin`（4 RTS） | Airlines 28；Zhu 2018；Sekitani-Zhao 2023（arXiv:2312.15422） |
| 2 | FDEF | P1 | Jahanshahloo 2013（CRS MILP）；QHULL（VRS）；Rafati-Maleki 2018 | 无（双路线互证） | IJIM 2013；Lins 2025 |
| 3 | Weak EF | P1 | Charnes-Rousseau-Semple 1996；Mehdiloo-Podinovski 2021 | 无 | JPA 1996 |
| 4 | Nash 博弈交叉效率 | P0/P1 | Liang 2008；Wu-Liang-Chen 2009；Wu-Liang-Zha 2009（ORSJ 开放） | DEAPack `game_cross_efficiency` | Liang 2008 |
| 5 | EBM | P1 | Tone-Tsutsui 2010；GRIPS DP09-21（开放全文） | DEAPack（declared，后转 executable 对拍） | GRIPS ρ=1.471/ε=0.529 |
| 6 | HMB | P0/P1 | Bjurek 1996；O'Donnell 2012 | R productivity + DEAPack | 面板双实现互证 |
| 7 | Cluster 8 基准 | P1 | **Seiford-Zhu 2003（已精读）**；Morita 2005（SBM 版）；Lim 2011（交叉效率版）；Tulkens 1995（Fixed） | 无（参考集组合参数化） | **32 打印机黄金数据**（E¹-E⁴ + Table 3-6 全数值） |
| 8 | AR/Cone | P0 | Charnes 1989/1990；Thompson 1990；Wong-Beasley 1990 | DEAPack `cone_ratio` + PyDEA AR | Kansas 农场 1990 |
| 9 | 修正 SBM | P1 | Sharp-Liu-Meng 2007；Lin-Yang-Huang 2019（C&IE 135:39-52） | 无 | Sharp 2007 |
| 10 | Malmquist 7 参比 | P0/P1 | Caves 1982 / Färe 1992/94；Global（Pastor-Lovell 2005）；Sequential（Shestalova 2003）；Window（Asmild 2004） | DEAPack + deaR + productivity 三实现 | 面板互证 |

### 3.3 关键调研结论（固化，避免重复检索）

- **Olyaie et al. 2014 终判**：公开索引缺失（Crossref 全领域检索 + 学术检索均无 DEA 领域命中），不依赖该篇；MinDS 主实现闭环 4 篇不受影响。
- **Vakili 期刊/DOI 未锁定**：Semantic Scholar PDF 可用（30 欧洲机场算例），实现期以 PDF 为准；同方向更强参考 Aparicio-Cordero-Pastor（CORE 开放，PISA 902 校）。
- **Cluster 本质**：8 基准 = 参考集组合规则参数化（self/cross 群参比、down/up 层参比、adjacent、window/fixed）；核心算法 = Seiford-Zhu 2003 分层剥离 + attractiveness/progress + VJ 版（权重 = AR 下界，与 AR/Cone 打通）。**原始 DEA 增删无效 DMU 不改变效率，Cluster 下会改变——单元测试关键断言**。
- **性能参考**：Zhu-Wu-Song 2018（COR）大 DEA 加速（参考点采样/强有效集分化）→ M3/M4 参考；官方基准 10 万 DMU 3×3 BCC = 25.32s（SpeedCompare_CN.htm）。
- **实现顺序（M5 内部）**：FDEF(CRS) → MinDS(Zhu 2018) → 扩展面(Zhu 2022) → WeakEF → Nash/AR-Cone/修正SBM → Cluster → Malmquist 7+HMB。
- **P2 高风险 2 处**：FDEF 的 NDRS/NIRS（文献稀缺）；Weak EF 弱投影不唯一（取第一可行解 + 文档化）。big-M 稳定性（MinDS/Nash/WeakEF 共用）→ 统一 big-M 自动估计器。

## 4. 验证体系（四层，M2-M5 执行标准）

| 层级 | 手段 |
|---|---|
| L1 开源对拍 | deaR（MinDS/交叉效率/Malmquist）、DEAPack（Nash/AR-Cone/HMB/EBM）、PyDEA（AR）、R productivity（HMB/Malmquist）——运行期外部进程调用，独立于产品源码 |
| L2 论文算例复现 | 32 打印机、32 机场、902 学校、Airlines 28、GRIPS EBM、Liang 2008——误差 < 1e-6 |
| L3 数学性质检验 | 强单调性；A*(d)>1 递减 / P*(g)>1 递增；增删无效 DMU 改变效率；Malmquist 分解恒等式 |
| L4 性能基准 | 10 万常规 / 100 万径向（对照官方 SpeedCompare） |

## 5. 里程碑路线图（M0-M8）

| 里程碑 | 内容 | 关键验收 | 状态 |
|---|---|---|---|
| M0 | 预研 | 可行性 + 算法路线 + 验证体系 | ✅ 完成（2026-10-24） |
| M1 | 工程底座 | CMake/vcpkg/HiGHS/Eigen 集成、CLI 框架、数据 IO、三段式 CI 跑通 | ⏳ 待启动 |
| M2 | 常规引擎 | CCR/BCC/SBM/超效率/成本收益/FDH/方向距离/加性族/Malmquist 基础——与 deaR/PyDEA 全量对拍 | 待启动 |
| M3 | 性能优化 | 10 万 DMU 常规模型（对照官方 25.32s） | |
| M4 | 极速求解 | 100 万 DMU 径向（参考点采样/强有效集分化） | |
| M5 | 独有算法 | 10 项逐项黄金算例对拍（顺序见 §3.3） | |
| M6 | GUI（Qt6） | 对标 MaxDEA 界面（模型选择/结果表/投影/FDEF 超平面/Cluster 分层图） | |
| M7 | 平台扩展 | Windows 原生打包、macOS（后期）、安装器 | |
| M8 | 发布 | 文档（AsciiDoc）、许可审计、发布验证、性能报告 | |

## 6. 开发环境与流程

- **开发环境**：WSL `ubuntu2604`（用户 ssy），项目 `/home/ssy/OpenSuperDEA`；Windows 宿主草稿中转（`C:\Users\ysm\Doubao\chats\2026-10-24\new-chat`）。
- **CI 三段式**：WSL 本地全量测试通过 → GitHub Actions（Linux）→ GitHub Actions（Windows）。仓库已关联 GitHub（gh 凭据已配置，账号 jxsword）。
- **Git 规范**：按里程碑分步提交；**未经用户明确确认不得 push 远程**（本地 git 提交可做）。
- **代码规范**：先读 AGENTS.md（tmp 禁区 + 规范速记）与 naming_convention.md（单一事实源：4 空格 + Allman + 100 列；snake_case 函数/ALL_CAPS 常量/PascalCase 类；文件头 Doxygen @file；Markdown 中英文间加空格）；.clang-format/.clang-tidy/pre-commit 机器强制。
- **测试策略**：单元测试 + 对拍集成测试（外部进程调用 R/Python）+ 性质测试 + 性能基准。

## 6.5 既有工程决策记录（ZCode 工具链搭建会话，DR-001~DR-004）

完整记录见 docs/decision_log.md，摘要固化如下（勿推翻）：

| 编号 | 决策 | 要点 |
|---|---|---|
| DR-001 | .clang-format 基座 | `BasedOnStyle: Microsoft` + `ColumnLimit: 100` + Qt 适配（ForEachMacros 等）。用户最初要求 Google 风格，但与 naming_convention（4 空格/Allman/100 列）三主轴冲突，实测弃用 Google/WebKit，选 Microsoft 覆写最小 |
| DR-002 | 提交前检查 | 纯 git pre-commit 钩子（scripts/git-hooks/），零 Python 依赖；`git config core.hooksPath scripts/git-hooks` 生效；clang-format/clang-tidy/@file 注释检查，不通过阻止提交 |
| DR-003 | CMake 骨架 | 最小 CMake（C++17 + Qt6 REQUIRED + AUTOMOC + `CMAKE_EXPORT_COMPILE_COMMANDS ON`）支撑 clang-tidy 解析；target `opensuperdea`；变量前缀 `OPENSUPERDEA_`（naming_convention §四的 CPP_STANDARD_ 为其示例项目前缀，按意图适配） |
| DR-004 | 第三方 API 适配 | 直接调用集中 `src/adapters/` + 目录级 .clang-tidy 豁免（仅关 readability-identifier-naming，其余检查继承）；业务代码禁止入该目录；钩子排除路径 |

## 7. 用户工作偏好（历史会话固化）

- **先出计划，确认后再执行**（重要任务强制；部分场景要求两轮复审大纲/报告）。
- 文档/报告类交付默认 **Markdown + 飞书文档双载体**；报告需含可视化（doubao-visualization）。
- 文件名仅英文字母/数字/下划线/连字符（禁中文/空格）；调研文档固定放 `research_docs/`。
- **联系作者不可行**——调研只依赖公开可访问资源。
- 复杂技术任务要求：AGENTS.md 全局约束、Git Flow 分步提交、多阶段验收清单、URL 必须核实可访问性并提供官方原文链接。
- 开发决策已锁定项：C++20 规划 / HiGHS / Qt6 / vcpkg / GitHub Actions 三段 CI / CLI 先行。

## 8. 下一步（M2 启动清单）

0. **需求工程阶段完成（2026-10-24，DR-005/006）**：SRS V1.0 定稿于 design_docs/00_srs.md（23 模块/116 FR/21 NFR）；设计分册 01-05 就绪。M2+ 算法设计册（06+）启动时产出。
0.5 **M1 完成（2026-10-24，v0.1.0）**：六任务验收通过；Git Flow 实跑（3 feature 分支 no-ff 合入）；33 单元测试全绿；三段式 CI 全绿（WSL / Actions Linux / Actions Windows，run 40107184884）；远程 github.com/jxsword/OpenSuperDEA 已建 main+develop+v0.1.0。CI 修复沉淀：workflow env 不可引用 runner 系统变量（用官方固定路径）、PowerShell 调用 bat 需 `& "${env:VAR}\..."`、MSVC 需 /W4 /utf-8（中文注释必需）。测试报告 design_docs/reports/m1_test_report.md。
0.7 **C++20 基线 + M2 设计册完成（2026-10-25，DR-007）**：CMAKE_CXX_STANDARD 20（指定初始化器/ranges/starts_with 已现代化，零告警）；M2 实现蓝本 design_docs/06_m2_engine_design.md 已确认。**M2 新会话启动提示词：tmp/zcode_2_m2.md（新会话首条消息粘贴该文件全文，或指示 zcode 显式读取该路径）。**
0.8 **M2 常规引擎完成（2026-10-25）**：三分支 Git Flow 实跑（m2-registry-radial / m2-nonradial-ddf / m2-cross-window-tfp，no-ff 合入 develop）；交付：C++20 模型注册表（concept+工厂）+ 参考集选择器 + 径向族/乘子/规模效率/SBM 族/超效率/DDF 五方向/FDH(MILP)/成本收益利润/加权加性族（ADD/归一化/RAM/BAM/wADD/DSBM）/ERM/交叉效率（DG-I/II/M3/虚拟 DMU）/窗口分析/相邻 Malmquist；对拍 harness（tests/parity/，68 用例，双阈值 P-010）+ L2 黄金算例（ccr_classic/fare1994_panel）+ L3 性质套件（PR-05/PR-06 等）+ 覆盖率门禁 85%（P-009 达标）。设计偏差与口径差异记录于 design_docs/reports/m2_test_report.md §3（含 06 册 §6 成本效率公式笔误修正）。release/v0.2.0 已冻结版本号（CMake project VERSION + VERSION_TEXT = 0.2.0）。
1. 项目根已备好：AGENTS.md、naming_convention.md、.clang-format/.clang-tidy、CMakeLists.txt、CLI 占位骨架（src/main.cpp，已构建验证通过）；Qt6 GUI 骨架已移入 tmp/gui_skeleton/（M6 恢复）。
2. M1 内容：引入 vcpkg（manifest）+ Eigen + HiGHS；CLI 框架（CLI11）；数据 IO（CSV）；三段式 CI（.github/workflows）；把 src/adapters/ 填充为求解器/数据适配层。
3. 依赖现状：当前无外部依赖（CLI 占位）；Eigen/HiGHS/CLI11 未安装（vcpkg 引入）；Qt6 为 M6 恢复引入。
4. 调研全部资料在 research_docs/（索引见 deliverables_index.md），无需重新调研。
