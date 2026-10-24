# 调研日志（survey_log）

> 调研批次：M0-① 独有算法全网调研 ｜ 启动 2026-10-24
> 批次安排：批1 = MinDS + FDEF（先行）→ 批2 = 博弈交叉效率(Nash) + EBM + HMB → 批3 = Cluster + AR/Cone + 修正SBM + Malmquist多参比

## 批1 · MinDS / FDEF（2026-10-24）
（进行中）

## 批1 · MinDS / FDEF（2026-10-24）✅ 完成

### MinDS 检索记录
- GitHub 代码搜索（gh CLI，账号 jxsword）：'minimum distance strong efficient frontier'/'closest target DEA'/'closest targets DEA'/'minDS'/'strong efficient frontier' 均无 DEA 相关实现（仅噪声）
- 学术：MaxDEA 官方 Features_X.htm 原文引用了 7 篇 MinDS 文献 → 全部定位
- 关键发现：deaR R 包 model_addmin 实现 Aparicio 2007 + Zhu 2018（mf/milp 两法、4 种 RTS、Airlines 数据集），GPL-2/3
- 论文：Aparicio 2007 (JPA 28:209, DOI 10.1007/s11123-007-0039-5)；Zhu 2018 (Omega 79:1-8, DOI 10.1016/j.omega.2017.07.003)；Zhu 2022 (EJOR 296:927-939)；Sekitani&Zhao 2023 (arXiv:2312.15422, CC BY 4.0)
- 结论：P0/P1（deaR 参考 + Zhu 2018/2022 自行实现）

### FDEF 检索记录
- GitHub：'full dimensional efficient facets'/'fdef'/'efficient facets' 无实现
- 学术：Jahanshahloo 2013 (IJIM 5(2):149-159) CRS 全 FDEF 枚举 MILP 算法 + 算例（全文已读）；Olesen&Petersen 1996/2003 奠基；Akbarian 2014 (arXiv:1409.7546)；Huang 2025 (arXiv:2505.06487, CC BY-NC-ND)
- 技术路线：路线A=Jahanshahloo 2013 MILP（CRS）；路线B=QHULL 计算几何枚举（VRS，SDU WP 佐证）
- 结论：P1（CRS 完整可实现）；VRS 中风险；NDRS/NIRS 为 P2

## 批2 · 博弈交叉效率 / EBM / HMB（2026-10-24）✅ 完成

### 博弈交叉效率（Nash）
- GitHub：无独立实现；DEAPack src/deapack/evaluation/game_cross_efficiency.py 完整实现 Liang 2008（HiGHS）
- 论文：Liang-Wu-Cook-Zhu 2008 (Operations Research 56:1278-1288, DOI 10.1287/opre.1070.0487)；Wu-Liang-Chen 2009 (Omega 37:909-918, VRS)；Wu-Liang-Zha 2009 (JORSJ 52:105-111)
- 结论：P0/P1（DEAPack 参考 + 自行实现）

### EBM
- GitHub：无独立实现；DEAPack 为 declared 状态（benchmark_ebm.py + M13_DECLARED_EBM_IC.md + oracle）；deaR 无 EBM（确认）
- 论文：Tone & Tsutsui 2010 (EJOR 207:1554-1563, DOI 10.1016/j.ejor.2010.07.014)；GRIPS DP09-21 开放全文（affinity index + PCA + EBM-I-C/O-C 完整公式 + 算例 ρx=1.471/εx=0.529）
- 结论：P1（公式完整可复现，Eigen 谱分解 + HiGHS LP）

### HMB
- GitHub/CRAN：R productivity 包（CRAN 1.1.2, GPL-3, Färe-Primont/HMB/Fisher/Malmquist 完整族）；DEAPack hicks_moorsteen.py（Bjurek adjacent-period, HiGHS）
- 论文：Bjurek 1996 (Scand J Econ 98:303-313)；O'Donnell 2012 (JPA 38:255-272)；Kerstens 2014；Mizobuchi 2017
- 结论：P0/P1（双开源参考；与 Malmquist 合并实现）

### DEAPack 专项
- 覆盖 game cross/EBM(declared)/HMB/global-biennial-pooled Malmquist/metafrontier/Luenberger/动态网络SBM；用 HiGHS；GPL-3 → 参考不直接移植

## 批3 · Cluster / AR-Cone / 修正SBM / Malmquist多参比（2026-10-24）✅ 完成

### Cluster（8基准）
- 判定：= Context-dependent DEA（Seiford & Zhu 2003, OMEGA, DEAFrontier 开放全文）+ 聚类参比 + 窗口/固定参比的组合体系；非单一新算法
- 实现：P1（分层剥离 L1..Lh + attractiveness/progress + 参考集参数化）；无开源实现
### AR/Cone
- 论文：Thompson 1986/1990（AR）；Charnes-Cooper 1989/1990（Cone Ratio）
- 实现：✅ PyDEA（MIT）assurance region 已实现；deaR 无（源码确认）
- 结论：P0/P1
### 修正SBM
- 论文：Sharp-Liu-Meng 2007（JORS, natural negative data）；Lin et al. 2019（C&IE 135:39-52, 超效率负数据）；Wei 2019（Expert Systems）；Matin 2015
- 实现：无开源；P1（两阶段法，负数据变换）
### Malmquist多参比
- 论文：Caves 1982；Färe 1992/94；Pastor & Lovell 2005（Global）；Shestalova 2003（Sequential）；Asmild 2004（Window）
- 实现：DEAPack global/biennial/pooled；deaR malmquist（global/sequential）；R productivity；Stata malmq2/gtfpch
- 结论：P0/P1，与 HMB 合并实现
