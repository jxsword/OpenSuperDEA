默认禁止读取 tmp 目录下的任何文件，除非显式指定读取。

# 代码规范

- 编写或修改 C++、CMake、Markdown 文件前，**必须先读仓库根 naming_convention.md**
  并严格遵循其命名与注释规范（单一事实源，本文件不复制其内容）。
- 机器强制：格式由 .clang-format、命名与静态检查由 .clang-tidy 约束，
  提交时由 pre-commit 钩子（scripts/git-hooks/）自动执行；
  无法机器强制的评审级约束清单见 scripts/README.md。
- 速记（细节以 naming_convention.md 为准）：4 空格缩进 + Allman 大括号 + 100 列；
  函数/方法 snake_case、类/枚举类型 PascalCase、常量/枚举值/宏 ALL_CAPS、
  成员 snake_case（`_` 后缀可选）；文件头 Doxygen 注释必含 @file；
  覆写 Qt 虚函数保留 Qt 原名。
