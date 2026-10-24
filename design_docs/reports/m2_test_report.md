# M2 阶段测试报告（Milestone 2 Test Report）

> 日期：2026-10-25 ｜ 分支：release/v0.2.0（develop @ b5c930f 冻结）｜ 环境：WSL ubuntu2604，GCC 16.0.1，CMake 4.4.3，vcpkg（HiGHS 1.15.1 / Eigen / CLI11 / gtest 1.18.0）
> 门禁依据：design_docs/04_verification_test_plan.md §5（M2 行）、06 册 V1.0、SRS V1.1、启动提示词 §6-§7

## 1. 交付范围与门禁对照

| 分支 | 交付 | 合入门禁（提示词 §6） | 结果 |
|---|---|---|---|
| feature/m2-registry-radial | C++20 模型注册表（concept+工厂）+ 参考集选择器 + CCR/BCC/NIRS/NDRS + 超效率 + 规模效率分解 + 乘子模型 + run 接线 + CSV/JSON 导出 | ctest + deaR 径向族对拍全绿 | ✅ ctest 68/68 + 对拍 16/16 |
| feature/m2-nonradial-ddf | SBM（3 定向+超效率）/ERM/加权加性族（ADD/归一化/RAM/BAM/wADD/DSBM）+ DDF 五方向/RDM + 成本收益利润 + FDH(MILP) + V-006/V-008/V-010 | ctest + 对拍 + L3 全绿 | ✅ ctest 92/92 + 对拍 59/59 |
| feature/m2-cross-window-tfp | 交叉效率（DG-I/II/M3/虚拟 DMU）/窗口分析/偏好权重载体/相邻 Malmquist + 对拍 harness + 黄金算例 + 覆盖率门禁 | ctest + L1/L2/L3 + 覆盖率 ≥80% | ✅ ctest 102/102 + 对拍 68/68 + 覆盖率 85.0% |

## 2. 测试统计

- 单元测试：**102/102 通过**（ctest，m1 回归 33 + m2 69；0 warnings，clang-tidy-23 全清单 + pre-commit 钩子逐提交强制）
  - 注册表/参考集 9（FR-CLI-008 能力矩阵边界、exclude_self）
  - 径向族 11（手算基准 + L3：RTS 序恒等 θ_CRS≤θ_NIRS≤θ_VRS、TE=PTE×SE、PR-06 乘子-包络对偶全扫描）
  - 非径向/距离/价格/加性 24（SBM 全定向、DDF 五方向、FDH MILP、成本/收益/利润、加性族、ERM、DSBM、BAM=RAM 同构断言）
  - 交叉/窗口/TFP 8（PR-05 MI=EC×TC 全扫描、窗口 w=1 退化一致性、虚拟 DMU、矩阵均值契约）
  - 校验规则 M2 增量 5（V-006/V-008/V-010 逐规则）
  - 导出/CLI/黄金算例 12（02 册列集契约、10 位有效数字、负零归一、infeasible 空值、L2 黄金算例全链路）
- L1 开源对拍：**68/68 通过**（tests/parity/parity_runner.py，双阈值 P-010：绝对 1e-6 或相对 1e-9）
  - deaR 1.5.4：径向 8 + 径向超效率 4 + 乘子 4 + SBM 6 + SBM 超效率 3 + DDF custom 6 + DDF own/mean/unit 9 + RDM 1 + FDH 2 + 加性 7 + ERM 4 + 交叉效率 8 + 加权 SBM/加性 2（列映射差异均在 manifest 显式标注：输出导向 φ 倒数、M3 目标形式）
  - R productivity 1.1.2：Malmquist 1（Färe 1994 惯例仲裁：MI/EC/TC 全对齐）
  - 覆盖模型：径向族/SBM 族/超效率（径向+SBM）/乘子/DDF（5 方向）/FDH/成本/收益/加性族/ERM/交叉效率（4 方法）/Malmquist
- L2 黄金算例：ccr_classic（手算溯源 + deaR 双确认）、fare1994_panel（productivity 仲裁 + 手算复核）——`tests/golden/<case>/{data,expected,README}`，容差 1e-6
- L3 数学性质：PR-05（MI=EC×TC）、PR-06（乘子=包络，全数据集扫描）、TE=PTE×SE、RTS 序恒等、窗口退化一致性——全部 gtest 固定用例
- 覆盖率（P-009）：**core+adapters 行覆盖 85.0% ≥ 80%**（gcovr 8.6 + gcov-16；`build-coverage` 目录构建 --coverage 后 ctest 全量驱动）
- 环境注意（复现）：覆盖率构建需 `cmake -DCMAKE_TOOLCHAIN_FILE=$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake -DCMAKE_CXX_FLAGS=--coverage -DCMAKE_EXE_LINKER_FLAGS=--coverage`；gcovr 需 `--gcov-executable gcov-16`（系统默认 gcov 为 15，与 GCC 16 的 gcno 版本不匹配）

## 3. 设计偏差记录（实现期核定）

| 事项 | 06 册原稿 | 实现定案 | 依据 |
|---|---|---|---|
| 成本效率公式（§6） | CE = w·x0 / w·x*（分子分母反序，得值 ≥1） | CE = 最小成本/实际成本 ≤ 1 | 教科书定义 + deaR 同口径（L1 对拍为准） |
| SBM 超效率定向（§4.2） | 未细分定向 | non 用 Tone 2002 CC 形式；io/oo 定向超效率为单侧变体；deaR 三口径全部对齐 | L1 对拍 |
| VRS 加性族 | 全族适用 | VRS 下每 DMU 可自参考 → A 类极点 DMU score=0（数学性质，测试固化） | L3 |
| BAM 上下界 | "上下界参数化" | M2 默认界 = 数据范围（数值与 RAM 同构，测试断言恒等）；用户界参数化留 M5 | P-006 兜底条款 |
| DSBM/ERM non | 方向 SBM/图测度 | DSBM = 方向归一化 CC-SBM（L2+L3 兜底）；ERM non = SBM（Tone 2001 恒等，测试断言） | P-006 兜底条款 |
| 利润效率 CRS | "可为负" | CRS 无界时记 infeasible + note（VRS 有界正常） | 教科书已知边界 |
| 加权加性 #5-#9 命名 | P-006 清单 | 与本地 MaxDEA 存档（research_docs/01）核对：前 5 种名称一致，#6-8 加权 SBM 经 deaR weight 参数对拍 | P-006 |

## 4. 已知边界（M2 范围内，非缺陷）

- 交叉效率仅投入导向（io）口径（MaxDEA/deaR 惯例）；虚拟 DMU 经 `--method virtual --virtual-inputs/--virtual-outputs` 提供。
- 窗口分析底层模型限截面注册表模型（径向/SBM 等），行名合成（`dmu@period`）；M5 参比族复用同参考集机制。
- Malmquist 相邻参比为 M2 基础版；7 参比全集属 M5（内核 `distance_value` 不变）。
- deaR 的 malmquist 实现与教科书/productivity 口径不一致（跨期距离定义），L1 以 productivity 为准（manifest 注记）。
- 面板类参考集选择器（layer/group/fixed/period_pair）已留接口，M5 启用。

## 5. 结论

M2 常规引擎三分支全部交付，四层门禁（ctest 102/102 + L1 对拍 68/68 + L2 黄金算例 + L3 性质 + 覆盖率 85%）全绿。
具备进入用户验收 → tag v0.2.0 → push 授权（触发三段式 CI）的条件。
