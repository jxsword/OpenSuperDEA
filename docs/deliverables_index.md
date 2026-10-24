# OpenSuperDEA 交付物索引

> 全部历史交付物清单（本地路径 + 飞书 URL + 状态），供后续开发定位使用。
> 更新日期：2026-10-24

## 1. 调研文档（research_docs/，WSL 已落盘）

| 文件 | 说明 | 状态 |
|---|---|---|
| research_docs/README.md | 调研目录说明 + 交付物索引 | ✅ |
| research_docs/research_outline.md | 独有算法调研大纲（v2.0，两轮复审） | ✅ |
| research_docs/unique_algorithms_survey.md | **独有算法论文与技术路线表 v4.0**（核心调研产物，332 行） | ✅ |
| research_docs/m0_presearch_report.md | **M0 预研报告**（可行性论证 + 实施规划，与飞书版内容一致） | ✅ |
| research_docs/survey_log.md | 调研过程日志（批 1/2/3 + 两轮复审 + 追加调研 A/B/C） | ✅ |
| research_docs/01_maxdea_software_overview.md | 早期调研①《MaxDEA 软件详解与开源替代方案》（飞书导出归档） | ✅ |
| research_docs/02_online_dea_platform_survey.md | 早期调研②《在线DEA平台调研与MaxDEA功能覆盖评估》（飞书导出归档） | ✅ |
| research_docs/02_matrix.png | 在线调研功能覆盖矩阵图（13 功能组 × 11 平台） | ✅ |
| research_docs/02_matrix_widget.html | 在线调研矩阵交互组件 | ✅ |
| research_docs/widget_algorithm_matrix.html | M0 报告可视化：10 项算法路线矩阵 | ✅ |
| research_docs/widget_milestones.html | M0 报告可视化：里程碑时间线 | ✅ |
| research_docs/check6_crossref_oly_vakili.py | 追加调研脚本（Crossref 定向检索） | ✅ |
| research_docs/archive/survey_drafts/ | 旧版草稿归档（b1/b2/final/v2/v3 + outline_v2 + check 文件） | ✅ |
| research_docs/archive/scripts/ | 过程脚本归档（check2-6.py、deaR_scan.py） | ✅ |
| research_docs/archive/lark_drafts/ | 飞书创建草稿归档（draft.xml = M0 版） | ✅ |

## 2. 飞书文档（在线交付物）

| 文档 | URL | 状态 |
|---|---|---|
| OpenSuperDEA M0 预研报告 | https://my.feishu.cn/docx/M98GdjDt2oJWETxuDsVcdmSHnMc | ✅ 可访问（有本地 Markdown 版） |
| MaxDEA 软件详解与开源替代方案 | https://my.feishu.cn/docx/IhJCdJVFioMyU5xfmxyc2Qpfn7f | ✅ 可访问（已导出本地 01_*.md） |
| 在线DEA平台调研与MaxDEA功能覆盖评估 | https://feishu.doubao.com/docx/N729dOZDPoDZMDx7yswcAvJangh | ✅ 可访问（已导出本地 02_*.md） |

## 3. 项目工程文件（项目根，已存在）

| 文件 | 说明 |
|---|---|
| README.md | 项目门面文档（2026-10-24 新建） |
| AGENTS.md | AI Agent 协作约束（tmp 禁区 + 代码规范速记） |
| naming_convention.md | 命名与注释规范（单一事实源） |
| CMakeLists.txt / src/CMakeLists.txt | 构建脚本（CLI 占位骨架，target `opensuperdea`；Qt6 恢复点已注释） |
| src/main.cpp | CLI 占位入口（M1 替换为 CLI11 骨架） |
| src/adapters/ | 求解器/数据适配层（规划，M1 填充） |
| tmp/gui_skeleton/ | Qt GUI 骨架归档（原 main.cpp / main_window.{cpp,h}，M6 恢复） |
| .clang-format / .clang-tidy / .editorconfig / .gitattributes / .gitignore | 工程规范与 Git 配置 |
| docs/project_context.md | **会话知识固化**（后续开发必读） |
| docs/deliverables_index.md | 本索引 |

## 4. 关键外部来源（调研引用，2026-10-24 核验可访问）

| 来源 | URL |
|---|---|
| MaxDEA 官方功能清单 | https://maxdea.com/Features_X.htm |
| MaxDEA 下载/免费版 | https://maxdea.com/Download.htm |
| MaxDEA 性能基准 | https://maxdea.com/SpeedCompare_CN.htm |
| Seiford-Zhu 2003 全文（Cluster 精读） | https://www.deafrontier.net/papers/OMEGAContext.pdf |
| GRIPS DP09-21（EBM） | https://grips.repo.nii.ac.jp/record/1021/files/DP09-21.pdf |
| ORSJ 开放全文（Wu-Liang-Zha 2009） | https://orsj.org/wp-content/or-archives50/pdf/e_mag/52-2-105-111.pdf |
| IJIM 开放全文（Jahanshahloo 2013） | https://journals.srbiau.ac.ir/article_2148_59fc5d8bc1164ec7224d8f1cee1b68c6.pdf |
| Lins 2025（JPA，VRS FDEF） | https://link.springer.com/article/10.1007/s11123-025-00767-1 |
| arXiv:2312.15422（Sekitani-Zhao 2023） | https://arxiv.org/abs/2312.15422 |
| arXiv:1503.09095（Mehdiloozad-Ahmadi 2015） | https://arxiv.org/abs/1503.09095 |

> 完整论文题录与 DOI 见 research_docs/unique_algorithms_survey.md（v4.0）。
