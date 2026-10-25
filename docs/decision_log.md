# 决策记录（Decision Records）

## DR-001 2026-10-24 clang-format 以 Microsoft 模板为基座，格式规范以 naming_convention.md 为准  [状态: 生效]
- 背景: 绿地 C++/Qt 项目需建 .clang-format；用户最初要求"基于 Google C++ style 定制"，但仓库根 naming_convention.md 要求 4 空格缩进、Allman 大括号、100 列行宽，与 Google 模板（2 空格、Attached、80 列）三大格式主轴全部冲突。
- 选项与权衡: 方案A BasedOn: Google(需覆写缩进/大括号/列宽三主轴，基座名存实亡，残留断行细节不可控，弃用)；方案B BasedOn: WebKit(4 空格+Allman 匹配，但 ColumnLimit: 0 不限宽与文档 100 列约束方向相反，覆写代价更大，弃用)；方案C BasedOn: Microsoft(实测继承 4 空格+Allman+禁止短语句合并，仅列宽 120→100 一处覆写，采用)。
- 结论: .clang-format 采用 `BasedOnStyle: Microsoft` + `ColumnLimit: 100` + Qt 适配（ForEachMacros/StatementMacros/IncludeCategories）；命名与注释规则不受 clang-format 模板影响，全部以 naming_convention.md 为准（函数 snake_case、常量 ALL_CAPS 等与 Google 默认不同处以文档为准）。
- 理由: 格式基座与规范文档在核心维度一致时覆写最小、行为最可预期；Google 基座在本文档约束下无意义。
- 影响: .clang-format、.clang-tidy、scripts/README.md（评审清单）
- 记录时间 / 会话: 2026-10-24 / ZCode 工具链搭建会话

## DR-002 2026-10-24 提交前检查采用纯 git hook 脚本  [状态: 生效]
- 背景: 需在提交前自动执行 clang-format / clang-tidy / @file 注释检查，不通过则修复并阻止提交。
- 选项与权衡: 方案A pre-commit 框架(声明式配置、增量缓存，但新增 Python 依赖 pip install pre-commit，Qt/C++ 定制检查仍需自写 local hook，弃用)；方案B 纯 git pre-commit 脚本(零新依赖、逻辑透明、脚本随仓库入库，采用)。
- 结论: `scripts/git-hooks/pre-commit`（bash）+ `scripts/setup-hooks.sh` 设置 `git config core.hooksPath scripts/git-hooks`；工具按 clang-*-23/22/21 顺序探测。
- 理由: 团队规模小、检查链固定，框架收益不抵依赖成本；钩子入库后克隆即用（执行一次 setup-hooks.sh）。
- 影响: scripts/git-hooks/pre-commit、scripts/setup-hooks.sh、scripts/README.md
- 记录时间 / 会话: 2026-10-24 / ZCode 工具链搭建会话

## DR-003 2026-10-24 建最小 CMake 骨架以支撑 clang-tidy  [状态: 生效]

- 背景: 仓库无构建系统；clang-tidy 须有 compile_commands.json 才能正确解析 Qt 头文件。
- 选项与权衡: 方案A 暂不建构建系统，钩子无编译数据库时跳过 tidy(改动最小，但 Qt 代码的命名/静态检查长期缺位，弃用)；方案B 建最小 CMake 骨架(CMakeLists + src 占位 + CMAKE_EXPORT_COMPILE_COMMANDS ON，采用)。
- 结论: 顶层 CMakeLists.txt（C++17、Qt6 REQUIRED、AUTOMOC）+ src/ 最小可运行 Qt Widgets 占位（兼作风格示范）；构建系统供后续开发直接沿用。命名规范 §四 的 `CPP_STANDARD_` 前缀为该文档示例项目前缀，本项目按其意图采用 `OPENSUPERDEA_`。
- 理由: 构建系统是 C++ 项目迟早要建的，顺手建立即可让 clang-tidy 完整生效，占位成本一次性。
- 影响: CMakeLists.txt、src/CMakeLists.txt、src/main.cpp、src/main_window.h/.cpp、.gitignore
- 记录时间 / 会话: 2026-10-24 / ZCode 工具链搭建会话

## DR-004 2026-10-24 第三方 API 直接调用代码集中至 src/adapters/ 并做目录级豁免  [状态: 生效]
- 背景: 第三方库 API 不遵守本仓库命名规范且不逐一封装时，直接调用会与 readability-identifier-naming 冲突（第三方宏在项目文件内展开生成符号时必然告警）。
- 选项与权衡: 方案A 逐个封装适配类(工作量大，用户明确不需要，弃用)；方案B 到处打行内 NOLINT(散落难审计，弃用)；方案C 集中适配目录 src/adapters/ + 目录级 .clang-tidy 豁免命名检查 + pre-commit 钩子路径排除(采用)。
- 结论: 新建 src/adapters/（.gitkeep 占位 + 子级 .clang-tidy：InheritParentConfig true、仅关闭 readability-identifier-naming，其余检查继承生效）；钩子排除正则加入 adapters/；约定业务代码不得放入该目录。
- 理由: 隔离范围以目录为界、可审计；对比 '-*' 全关，保留 analyzer/performance 检查仍能发现适配层真实缺陷。
- 影响: src/adapters/.clang-tidy、src/adapters/.gitkeep、scripts/git-hooks/pre-commit、scripts/README.md
- 记录时间 / 会话: 2026-10-24 / ZCode 工具链搭建会话

## DR-005 2026-10-24 需求工程采用瀑布·阶段门禁，设计文档 Markdown 落库 design_docs/，依赖全量预装（含 Qt6）  [状态: 生效]
- 背景: 用户指令在正式开发前插入需求工程阶段（SRS → 详细设计 → 依赖预装 → Git Flow 开发），经 AskUserQuestion 确认三项流程决策。
- 选项与权衡: 开发模型：瀑布·阶段门禁(与 M0-M8 里程碑、每阶段对拍全绿验收天然一致，采用)；敏捷 Sprint(需改造既定里程碑路线，弃用)；混合(流程过重，弃用)。文档载体：Markdown 落库(可版本化评审，符合 naming_convention，采用)；MD+DOCX 双份(每轮双写，弃用)；MD+飞书同步(阶段同步维护成本，弃用)。依赖预装：全量含 Qt6(一次装齐，M6 临近可能需升级重装，用户选择)；M1-M5 全量(Qt6 延后，弃用)；仅 M1+M2(需多次授权，弃用)。
- 结论: 瀑布·阶段门禁推进（SRS 定稿→总体设计→M1-M8 逐里程碑设计-开发-测试，全绿并经用户验收才进下一阶段）；SRS 与设计文档以 Markdown 存 design_docs/（mermaid 优先内嵌）；正式开发前经用户授权统一预装 M1-M6 全部依赖（vcpkg+Eigen/HiGHS/CLI11/GoogleTest/QHULL、R: deaR+productivity、Python: DEAPack+PyDEA、Qt6），测试框架推荐 GoogleTest（M1 任务 6，SRS/设计期定稿）。
- 理由: 与既定里程碑路线和「测试通过才进下一步」的阶段验收要求完全一致；Markdown 入库使需求-设计-代码-测试同链追溯。
- 影响: design_docs/（新建目录）、docs/decision_log.md、SRS 与设计文档流程、阶段三依赖预装清单
- 记录时间 / 会话: 2026-10-24 / ZCode 需求工程会话（plan 获批后首步写入）

## DR-006 2026-10-24 SRS V1.0 定稿，14 项待确认事项全部按建议裁决  [状态: 生效]
- 背景: 阶段一产出 SRS V0.3（23 模块/116 FR/21 NFR）附 14 项 pending 待裁决（P-001~P-014），呈报用户后用户确认「全部按照建议执行」。
- 选项与权衡: 各事项候选方案与权衡详见 SRS 附录 8.2 裁决表与 00a §5.3 原始记录；关键裁决——P-002 性能达标线取「≤ 官方 × 1.2 同量级」(严格 ≤ 官方值对硬件差异过于刚性，弃用)；P-005 C++20 升级定在 M2 启动评估(更早无必要、更晚影响引擎实现，弃用)；P-008 非期望产出/非自由处置归扩展功能(M2 清单未含，强行塞入 M2 挤占对拍验收，弃用)；P-013 Windows 用 MSVC(MinGW 与 vcpkg/HiGHS 端口兼容性差，弃用)。
- 结论: SRS 升 V1.0 作为需求基线；assumption A-001~A-008（WeakEF 取第一可行解、FDEF NDRS/NIRS 延后、Cluster 参数化路线、GoogleTest、版本号映射、对拍外部进程、CLI 英文消息、big-M 估计器前置）一并定稿认可；后续需求变更走 SRS 变更流程。
- 理由: 14 项建议均与既有里程碑路线、四层验证体系、许可边界一致，无新增不可逆风险。
- 影响: design_docs/00_srs.md（V1.0）、00a、00b；阶段二详细设计（01-05 分册）依据
- 记录时间 / 会话: 2026-10-24 / ZCode 需求工程会话

## DR-007 2026-10-24 C++17 → C++20 升级于 M2 首分支执行  [状态: 生效]
- 背景: SRS C-01（P-005 裁决）约定 C++20 升级时点在 M2 启动评估并经用户确认；升级评估（tmp/cpp20_upgrade_assessment.md）呈报优势与风险后用户确认升级。
- 选项与权衡: M2 首分支即切 C++20(锁定 concepts/指定初始化器/span/jthread 收益，避免 M2 接口返工，采用)；M4 性能期再切(模型注册表用 SFINAE 写一遍后返工，弃用)；保持 C++17(放弃 M2-M4 核心收益，弃用)。
- 结论: CMAKE_CXX_STANDARD 17→20（一行）；naming_convention.md 与 .clang-tidy 评估结论为无需修改（无标准版本绑定）；确定性成本为 modernize-use-designated-initializers 对 M1 约 10 处位置初始化的一次性批量修复（随升级分支完成）；SRS C-01 升 V1.1。
- 理由: 三编译器矩阵（GCC 16/13、MSVC 19.4x）C++20 完整支持；vcpkg 四库均兼容；净风险半小时批量修复 vs M2 架构核心（concepts 注册表）与 M3/M4 前置（span/jthread）收益。
- 影响: CMakeLists.txt、SRS V1.1（C-01）、M2 设计册 06（接口按 C++20 设计）、.clang-tidy（新增 designated-initializers 修复）
- 记录时间 / 会话: 2026-10-24 / ZCode M1-M2 会话

## DR-008 2026-10-25 热启动路线立项与 NFR-PERF-003 里程碑归属调整  [状态: 生效]
- 背景: F-005/F-006 实测冷启动 10 万 DMU BCC 8 线程 ≈8.1h，与 NFR-PERF-003（≤30.4s）差约 1000×；C 实验证实热启动 7.8× + presolve off 5.3×（合计 9×，score 全配置 bit 级一致），A 路线全做预估 2-5 分钟，最后 4-6× 依赖 M4 算法（FR-PERF-002/003 强有效集分化/参考点采样）。
- 选项与权衡: A 热启动+PR-09 口径细化(技术正道、实测支撑，采用)；B 维持 bit 级冷启动(M4 百万级同样不可达，问题仅被推迟，弃用)；C 先实验量化(已完成，作为 A 的零风险前置并入)。里程碑处理: α 30.4s 移 M4 门禁(采用)/β M3 内继续压(已知差 4-6×，结局大概率仍不达标，弃用)/γ M3 带伤关闭(验收记录失真，弃用)。
- 结论: 热启动立项：SolveSession 增量修改后不 clearSolver（保留最优基）+ presolve off/solver=simplex + 模型层 Δ 声明（省 O(nnz) 指纹全扫），配置开关可回退；PR-09 口径细化为「效率值/状态 1e-9 容差一致，顶点类输出（slack/λ/投影，LP 多重最优解）不跨配置 bit 对齐」；NFR-PERF-003 的达标判定由 M3 门禁移至 M4（与百万级同关门），M3 门禁改为「十万级分钟级完成，实测入报告」（SRS V1.1→V1.2 变更）。
- 理由: 实测数据完备（score 100/100 bit 级一致、二阶段同构序列热启动同样适用）；30.4s 实为 M3 架构并行 + M4 算法加速的合力目标（SRS 原切分对 M3 两开关的能力假设被实测证伪）；热启动开关可回退、冷启动路径保留为对照。
- 影响: src/adapters/highs_adapter.cpp（热启动/presolve 选项/探测后缓存对齐）、tests/unit/m3（PR-09 断言口径、session_test 语义）、design_docs/07 册 §3.2/§5/§8、04 册 §4 PR-09 行、SRS V1.2（§4.1 NFR-PERF-003 版本列、§8.1 追踪矩阵）、M3 测试报告门禁行
- 记录时间 / 会话: 2026-10-25 / ZCode M3 会话（用户裁决 α + A）

## DR-009 2026-10-25 云服务器远程测试工作流（juzdata）  [状态: 生效]
- 背景: 68 例对拍、覆盖率全量、L4/M4 基准等 >20 分钟任务占用本机 8 核，与开发迭代互相阻塞；juzdata（2 核 3.8G、glibc 2.39、单核与本机打平且支持 AVX-512）具备承接条件。
- 选项与权衡: 服务器本地构建-每轮只同步源码(本机 glibc 2.43 > 服务器 2.39 二进制不可直传，采用)；二进制直传(glibc 差异否决，弃用)；服务器跑性能门禁(2 核 {4,8} 线程无意义且环境不可控，弃用——门禁保持本机基线 P-002/P-014，服务器 bench 仅作观察/回归)。
- 结论: scripts/remote_test/ 三件套（provision_server.sh 一次性装 R 栈/vcpkg@ a1cae005c3/swapfile+env.txt 档案；remote_test.sh 子命令 sync/run/status/fetch，tmux 会话 + 30s 轮询 + 结果回传 build-remote-results/；README 含差异与排障）。服务器目录全落 /data，构建/ctest 限 -j2 + 内存预检；bench suite 占位（threads {1,2,auto}），M3 harness 落地后接入。
- 理由: 链路冒烟已验证（sync→tmux→轮询→fetch overall_exit=0）；异步化使本机开发与长跑任务解耦，M4 百万级依赖此基建。
- 影响: scripts/remote_test/（新建）、docs/decision_log.md、M4 测试执行方式；不 push（P-003）
- 记录时间 / 会话: 2026-10-25 / ZCode M3 会话（用户裁决执行顺序：A 之后、bench 之前）
