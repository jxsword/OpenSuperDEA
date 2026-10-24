# OpenSuperDEA

**OpenSuperDEA** 是一个以 AI Agent（Zcode）驱动、用 C++ 全量实现商业软件 MaxDEA X 全部功能的开源项目。目标是覆盖所有在线平台与开源工具均未实现的功能（10 项独有算法 + 万级 DMU 极速求解），交付 CLI + GUI 双形态，支持 Ubuntu（WSL）/ Windows，macOS 作为后期扩展平台。

## 项目状态

| 阶段 | 状态 |
|---|---|
| M0 预研（可行性论证 + 算法路线 + 验证体系） | ✅ 完成（2026-10-24） |
| M1 工程底座（CMake + HiGHS/Eigen 集成 + CLI 框架 + 三段式 CI） | 待启动 |

M0 预研报告（双载体交付）：Markdown 见 [research_docs/m0_presearch_report.md](research_docs/m0_presearch_report.md)；飞书文档见项目交付记录。

## 功能范围

对标 MaxDEA X 官方功能清单（[Features_X.htm](https://maxdea.com/Features_X.htm)，访问 2026-10-24），分为两部分：

**常规模型族**（开源生态成熟，M2 实现）：

- 径向：CCR / BCC / NIRS / NDRS + 规模效率分解
- 非径向：SBM / ERM / 加权加性族（9 种子模型）
- 方向距离：DDF / RDM / 自定义方向向量
- 超效率、成本 / 收益、FDH、乘子模型
- 交叉效率（Doyle-Green I/II + Ruiz + 自定义虚拟 DMU）
- 窗口分析（Window DEA）

**独有算法（10 项，全部有论文路线 + 验证闭环，M5 实现）**：

| # | 算法 | 优先级 | 验证手段 |
|---|---|---|---|
| 1 | MinDS（Closest Target，强单调） | P0/P1 | deaR `model_addmin` 对拍 + 3 组算例 |
| 2 | FDEF 全维有效面 | P1 | 论文算例（IJIM 2013 / Lins 2025） |
| 3 | 最小距离弱有效前沿 | P1 | 论文算例（JPA 1996） |
| 4 | 博弈交叉效率（Nash 均衡） | P0/P1 | DEAPack 对拍 + Liang 2008 算例 |
| 5 | EBM 混合距离 | P1 | GRIPS 算例（ρ/ε 精确复现） |
| 6 | HMB 指数 | P0/P1 | R productivity + DEAPack 对拍 |
| 7 | Cluster 8 基准 | P1 | 32 打印机黄金算例（Seiford-Zhu 2003） |
| 8 | AR / Cone 受限乘数 | P0 | DEAPack + PyDEA 对拍 |
| 9 | 修正 SBM（Sharp 2007 / Lin 2019） | P1 | 论文算例 |
| 10 | Malmquist 多参比（7 种）+ TFP 分解 | P0/P1 | 三实现互证（DEAPack/deaR/productivity） |

完整算法路线、论文题录与 DOI 见 [research_docs/unique_algorithms_survey.md](research_docs/unique_algorithms_survey.md)。

## 技术栈

| 层 | 选型 |
|---|---|
| 语言 | C++17（当前骨架）/ C++20（引擎实现期规划，需同步评估 naming_convention.md 与 .clang-tidy 约束） |
| 构建 | CMake（当前 3.16+） |
| GUI | Qt 6（Widgets，骨架已移入 tmp/gui_skeleton/，M6 恢复） |
| 依赖管理 | vcpkg（manifest 模式，M1 引入） |
| 线性代数 | Eigen 3.4（M1 引入） |
| 求解器 | HiGHS（LP/MILP 官方 C++ API，M1 引入） |
| 计算几何 | QHULL（FDEF VRS 路线，M5 引入） |
| CLI | CLI11（M1 引入） |
| CI | GitHub Actions：WSL Ubuntu → Actions Linux → Actions Windows 三段式 |
| 文档 | AsciiDoc / Markdown |

**选型依据**：C++20（规划）vs Rust 的论证结论——HiGHS 官方 C++ API、论文/MATLAB 语义直译、Qt6/CMake 生态成熟；详见 M0 预研报告。

## 目录结构

```text
OpenSuperDEA/
├── CMakeLists.txt          # 顶层构建脚本
├── AGENTS.md               # AI Agent 协作约束（先读）
├── naming_convention.md    # 命名与注释规范（单一事实源）
├── .clang-format           # 代码格式（机器强制）
├── .clang-tidy             # 静态检查（机器强制）
├── src/                    # 源码
│   ├── CMakeLists.txt
│   ├── main.cpp            # 入口（当前为 CLI 占位，M1 替换为 CLI11）
│   └── adapters/           # 求解器/数据适配层（规划，M1 填充）
├── scripts/                # 构建/CI/钩子脚本（规划）
├── docs/                   # 设计文档（规划）
├── research_docs/          # 调研文档（已完成，含 M0 预研）
├── build/                  # 构建输出（不入库）
└── tmp/                    # 临时文件（禁止读取，见 AGENTS.md）
    └── gui_skeleton/       # Qt GUI 骨架归档（CLI 先行规划，M6 恢复）
```

规划中的引擎分层（M1-M5 填充）：

```text
core/
├── radial/      # CCR/BCC/NIRS/NDRS + 定向 + 规模分解
├── nonradial/   # SBM/ERM/加性族/修正 SBM/EBM
├── distance/    # DDF/RDM/弱前沿/强前沿(MinDS)/FDEF
├── cross/       # 交叉效率/博弈 Nash/AR-Cone
├── cluster/     # 分层/8 基准
└── tfp/         # Malmquist 7 参比/HMB/分解
```

## 构建指南

### 环境

- 开发环境：WSL Ubuntu（`ubuntu2604`），用户 `ssy`
- 依赖：CMake ≥ 3.16（当前 CLI 占位无外部依赖）；M1 起增加 vcpkg、Eigen、HiGHS；Qt 6 为 M6 恢复引入

### 构建（当前骨架）

```bash
wsl -d ubuntu2604 -s ssy
cd /home/ssy/OpenSuperDEA
cmake -S . -B build
cmake --build build -j$(nproc)
./build/src/opensuperdea
```

### 测试

```bash
cd /home/ssy/OpenSuperDEA
ctest --test-dir build          # 单元测试（M2 起）
python3 tests/run_parity.py     # 开源实现对拍（deaR/DEAPack/productivity/PyDEA）
```

## 使用示例

CLI 规划草案（M1 落地后生效）：

```bash
opensuperdea run --model ccr --orientation input --rts vrs --data data.csv
opensuperdea run --model mind --rts vrs --data data.csv --target output
opensuperdea cluster --benchmark self --data panel.csv
opensuperdea compare --left run_a.json --right run_b.json
```

## 里程碑路线图

| 里程碑 | 内容 | 关键验收 |
|---|---|---|
| M0 | 预研（完成） | 可行性 + 算法路线 + 验证体系 |
| M1 | 工程底座 | CMake/vcpkg/HiGHS/Eigen、CLI 框架、三段式 CI 跑通 |
| M2 | 常规引擎 | 与 deaR/PyDEA 全量对拍 |
| M3 | 性能优化 | 10 万 DMU 常规模型（对照官方 25.32s） |
| M4 | 极速求解 | 100 万 DMU 径向（参考点采样/强有效集分化） |
| M5 | 独有算法 | 10 项逐项黄金算例对拍 |
| M6 | GUI（Qt6） | 对标 MaxDEA 界面 |
| M7 | 平台扩展 | Windows 原生、macOS（后期） |
| M8 | 发布 | 文档/许可审计/性能报告 |

## 验证体系

| 层级 | 手段 |
|---|---|
| L1 开源对拍 | deaR、DEAPack、R productivity、PyDEA |
| L2 论文算例 | 32 打印机、32 机场、902 学校、Airlines 28、GRIPS EBM |
| L3 数学性质 | 强单调性、分层性质、Malmquist 分解恒等式 |
| L4 性能基准 | 10 万 / 100 万 DMU（对照官方 SpeedCompare） |

## 文档索引

| 文档 | 说明 |
|---|---|
| research_docs/README.md | 调研目录说明 |
| research_docs/research_outline.md | 调研大纲（两轮复审） |
| research_docs/unique_algorithms_survey.md | 独有算法论文与技术路线表（v4.0） |
| research_docs/m0_presearch_report.md | M0 预研报告（本仓库版） |
| research_docs/survey_log.md | 调研过程日志 |
| research_docs/widget_*.html | 报告可视化组件 |

## 开发约定

- **AI Agent 协作**：先读 [AGENTS.md](AGENTS.md)，遵守 `tmp/` 禁区与代码规范
- **命名与注释**：以 [naming_convention.md](naming_convention.md) 为单一事实源；C++/CMake/Markdown 修改前必读
- **机器强制**：.clang-format（格式）、.clang-tidy（命名与静态检查）、pre-commit 钩子（scripts/git-hooks/）
- **Git 流程**：按里程碑分步提交；**未经用户确认不得 push 远程**（仓库已关联 GitHub，gh 凭据已配置）
- **CI 三段式**：WSL 本地全量测试通过 → GitHub Actions（Linux）→ GitHub Actions（Windows）

## 许可与合规

- 产品依赖：HiGHS（Apache-2.0）、Eigen（MPL2）、Qt（LGPL）、QHULL —— 无许可冲突
- 参考实现对拍：deaR（GPL-2|3）、DEAPack（GPL-3）、R productivity（GPL-3）—— **只作语义参考与数值对拍，不移植代码**；PyDEA（MIT）可参考
- 商业对标：功能以 MaxDEA X 为参照（官方清单 [Features_X.htm](https://maxdea.com/Features_X.htm)），本项目为独立实现，与 MaxDEA 无代码关联
