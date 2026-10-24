# scripts/ 说明

## 提交前检查钩子

`git-hooks/pre-commit` 在每次 `git commit` 前对**暂存区**的 C/C++ 文件执行三道检查，任一未通过则阻止提交：

| 检查 | 工具 | 未通过时的行为 |
|---|---|---|
| 代码格式 | clang-format | 工作区与暂存区一致时自动修复并重新暂存；否则报告 |
| 静态分析 | clang-tidy（依赖 `build*/compile_commands.json`，缺失则跳过） | 输出告警清单与修改建议 |
| 文件头注释 | grep 检查 `@file` | 输出补写模板 |

工具探测顺序：`clang-format-23/22/21`、`clang-format`（clang-tidy 同理）。

### 启用方式（克隆后执行一次）

```bash
./scripts/setup-hooks.sh
```

前提：先 `cmake -S . -B build` 生成编译数据库，clang-tidy 才会生效（否则自动跳过、不阻塞提交）。

### 应急通道

确认告警可接受时：`git commit --no-verify`。

### 第三方 API 适配目录（src/adapters/）

不合规命名的第三方库不逐一封装时，把**直接调用第三方 API 的胶水代码**集中放进 `src/adapters/`，该目录享受双重豁免：

- **clang-tidy**：`src/adapters/.clang-tidy`（`InheritParentConfig: true` + 关闭 `readability-identifier-naming`）——第三方宏展开生成的符号不再触发命名告警，其余检查（clang-analyzer/performance 等）仍然生效
- **clang-format / 整体检查**：pre-commit 钩子的排除正则含 `adapters/`，本目录不参与提交前检查

注意：适配层之外的业务代码保持全量检查，不要把普通业务文件放进 `src/adapters/`（Code Review 把关）。若需在目录内临时关闭格式化，用 `// clang-format off` / `// clang-format on` 区域标记。

## 评审级约束（无法机器强制，Code Review 时人工把关）

以下条目来自仓库根 `naming_convention.md`，clang-tidy 无法可靠检查：

- **布尔变量/函数**以 `is_` / `has_` / `can_` / `should_` 开头（§1.2 / §1.3）
- **回调/事件函数**以 `on_` 开头（§1.3）
- **抽象基类**可加 `I` 前缀或 `Base` 后缀；异常类以 `Exception` / `Error` 结尾（§1.4）
- **全局变量**尽量避免（仅命名上强制 `g_` 前缀）（§1.1）
- **宏**加项目前缀（如 `OPENSUPERDEA_`）避免冲突（§1.5）
- **覆写 Qt 虚函数保留 Qt 原名**（`paintEvent` 等为 Qt 定死的 lowerCamelCase，不受本仓库 snake_case 函数名规则约束），行前加：

  ```cpp
  // NOLINTNEXTLINE(readability-identifier-naming) Qt 虚函数，保留 Qt 原名
  void paintEvent(QPaintEvent* event) override;
  ```

- **函数/类/枚举的 Doxygen 中文注释**（§2.2–§2.5）：注释"为什么"而非"是什么"
- **文件名/目录名 snake_case**、CMake 目标/变量命名（§四）、Markdown 规范（§五）
- naming_convention.md §四 中 `CPP_STANDARD_` 前缀为该文档示例项目的前缀，本项目按其意图采用 `OPENSUPERDEA_` 前缀
