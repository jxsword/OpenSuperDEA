# M1 阶段测试报告（Milestone 1 Test Report）

> 日期：2026-10-24 ｜ 分支/标签：main @ v0.1.0 ｜ 环境：WSL ubuntu2604，GCC 16.0.1，CMake 4.4.3，vcpkg（HiGHS 1.15.1 / Eigen 5.0.1 / CLI11 2.7.2 / gtest 1.18.0）
> 门禁依据：design_docs/04_verification_test_plan.md §5（M1 行）、SRS V1.0、启动提示词 §6

## 1. 六任务验收对照

| # | 任务（提示词 §6） | 验收证据 | 结果 |
|---|---|---|---|
| 1 | 首次本地 git 提交 | `git log`：3813e9c（工程规范基线，11 文件）→ f805a2b（骨架+文档，45 文件）；**未 push**（origin 无任何推送） | ✅ |
| 2 | vcpkg + 依赖集成 | vcpkg.json manifest + CMakePresets；`rm -rf build && cmake --preset default` 全新配置成功；四库头文件/链接可用（HiGHS 适配层编译通过即证明） | ✅ |
| 3 | CLI 框架（CLI11） | `version`/`--version` 输出 v0.1.0；`check`/`run` 子命令解析正确；退出码规范 0/1/2（FR-CLI-007 测试断言） | ✅ |
| 4 | CSV 数据 IO | toy_cross 读入 `dataset: 4 DMU, 2 投入, 1 产出`；toy_panel `panel: 2 期次, 3 DMU`；缺列/非数值/空数据三类错误带 `文件:行:列 规则 提示`（V-002~V-005 用例） | ✅ |
| 5 | 核心数据结构 | Dataset/PanelData/Direction/Price/Weight（Eigen）+ ModelResult 契约（02 册）；存取/序列化往返测试通过 | ✅ |
| 6 | 三段式 CI + 测试框架 | GoogleTest 33 用例全绿（本地 WSL 段）；ci.yml（Linux→Windows needs 门禁 + vcpkg 缓存）入库；**Actions 双平台全绿**（run 40107184884：https://github.com/jxsword/OpenSuperDEA/actions/runs/40107184884） | ✅ |

## 2. 测试统计

- 单元测试：**33/33 通过**（`ctest --test-dir build`，标签 unit;m1），0.1s
  - HiGHS 适配层 5（3 LP 手算 + MILP 整数化 + 不可行/无界映射，含零目标二次探测路径）
  - CSV 解析 5（BOM/CRLF/注释/科学计数/列数不一致）
  - 数据构造 6（前缀角色/CLI 覆写/黄金算例/方向列/面板分组/期次序）
  - 数据模型 3（维度存取/矩阵往返/平衡面板）
  - 校验规则 7（V-002/003/004/005/009 逐规则 + 聚合性 + 面板跳过跨期重名）
  - CLI 7（version/--version/check/panel 摘要/退出码三类）
- 构建告警：0（-Wall -Wextra + clang-tidy 23 全清单，经 pre-commit 钩子逐次强制）
- 黄金算例：toy_cross / toy_panel 入库（L2 正式算例库自 M2 起）

## 3. 质量门禁执行记录

- pre-commit 钩子（clang-format / clang-tidy-23 / @file 检查）全部提交逐次通过；
- 过程中依规关闭 2 项与项目约定冲突/版本不一致的检查并注明理由（.clang-tidy 内注释）：
  readability-identifier-length（DEA 数学记号 m/s/n）、readability-trailing-comma（LLVM 21/23 行为不一致）；
- Git Flow：2 个 feature 分支（vcpkg-deps / cli-csv）no-ff 合入 develop → main，结构见 `git log --graph`。

## 4. 已知边界（M1 范围内，非缺陷）

- `run --model ccr` 为占位（数据链路全通，模型执行提示 M2 提供）——03 册 §3.1 设计行为；
- 价格/权重列（pi_/po_/w_）读入与校验规则 V-008/V-010 属 M2 交付；
- ~~Actions 两段未执行~~ 已补证：经 3 轮修复（workflow env context 非法引用、PowerShell bootstrap 语法、MSVC /W4 /utf-8）后双平台全绿。

## 5. 结论

M1 工程底座六任务全部达成，本地门禁全绿，具备进入 M2（常规引擎）的条件。
用户已验收并授权 push（2026-10-24）：三段式 CI 全绿闭环（WSL 本地 ✅ / Actions Linux ✅ / Actions Windows ✅），tag v0.1.0 指向 CI 全绿 commit。
