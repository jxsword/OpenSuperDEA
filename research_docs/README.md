# research_docs — 调研文档目录

本目录存放 OpenSuperDEA 项目全部调研相关的文档与过程文件（M0 预研阶段全部产物）。

## 目录约定

| 文件 | 用途 |
|---|---|
| README.md | 本说明（交付物索引总表见 docs/deliverables_index.md） |
| research_outline.md | 调研大纲（v2.0，先确认后执行，两轮复审） |
| unique_algorithms_survey.md | **独有算法论文与技术路线表 v4.0**（核心调研产物） |
| m0_presearch_report.md | **M0 预研报告**（可行性论证 + 实施规划，与飞书版一致） |
| survey_log.md | 检索日志 / 调研过程记录 |
| 01_maxdea_software_overview.md | 早期调研①：MaxDEA 软件详解与开源替代方案（飞书导出） |
| 02_online_dea_platform_survey.md | 早期调研②：在线 DEA 平台调研与功能覆盖评估（飞书导出） |
| 02_matrix.png / 02_matrix_widget.html | 在线调研功能覆盖矩阵（13 功能组 × 11 平台） |
| widget_algorithm_matrix.html / widget_milestones.html | M0 报告可视化组件（算法路线矩阵 / 里程碑时间线） |
| check6_crossref_oly_vakili.py | 追加调研脚本（Crossref 定向检索） |
| archive/ | 旧版草稿、过程脚本、飞书创建草稿（审计链保留） |

## 维护规则

- 每份调研报告必须附：官方/论文原文 URL、检索日期、作者与出处；
- 过程临时文件（检索日志、草稿）保留在 survey_log.md 与 archive/，不删除；
- 调研结论与最终技术路线不一致时，在报告中标注原因；
- 全部交付物索引（含飞书 URL）见项目根 docs/deliverables_index.md；
- 后续开发必读：docs/project_context.md（会话知识固化）。

## 调研结论速览（详细见各报告）

- **可行**：10 项独有算法全部有论文路线 + 验证闭环，无一需从零发明（M0 预研报告）。
- **8 项差距**：MinDS、FDEF、Nash、EBM、HMB、Cluster、AR/Cone + 万级 DMU 极速求解。
- **Cluster 黄金数据**：Seiford-Zhu 2003 32 打印机算例（E¹-E⁴ 分层 + Table 3-6 全数值）已精读提取。
- **Olyaie 2014**：公开索引缺失终判，不依赖；**Vakili**：PDF 可用、期刊未锁定（不阻塞）。
