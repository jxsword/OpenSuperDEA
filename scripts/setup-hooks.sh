#!/usr/bin/env bash
# 启用提交前检查钩子：让 git 使用 scripts/git-hooks/ 下的钩子（随仓库入库、团队共享）。
set -euo pipefail

git config core.hooksPath scripts/git-hooks
chmod +x "$(dirname "$0")/git-hooks/pre-commit"
echo "已设置 core.hooksPath=scripts/git-hooks，pre-commit 检查钩子生效。"
