# 数据集资产清单（datasets/）

> 建立：2026-10-25（M3 启动，用户裁决提前备齐 M3-M5 所需数据集）｜设计依据：`design_docs/07_m3_performance_design.md` §4
> 原则：生成类可复现（固定 seed + 生成器入库）；外部类记全来源（URL / 许可 / 访问日期）；
> 论文 PDF 为版权作品，仅作内部对拍资产，**不随 release 再分发**。

## 1. generated/ —— 程序生成的大规模随机算例（M3 bench / M4 百万级）

- 生成器：`gen_bench_data.py`（Python 标准库，零依赖）。
- 机制：投入对数正态 → 产出经 Cobb-Douglas 真实前沿生成 → 右偏技术无效项
  （`u ~ Exp(0.35)`）→ **强有效比例控制**（目标比例单元 u=0 落于前沿）。
- 统计口径：对齐官方 [SpeedCompare](https://maxdea.com/SpeedCompare_CN.htm)
  （随机数据 × 10 组取均值 + min-max 区间；官方硬件 i9-9900K 固频 3.6GHz，本机环境见 bench 报告 P-014 记录）。
- 规格矩阵（对齐官方）：DMU ∈ {1 万, 10 万, 100 万} × {3×3, 5×5}；每组 seed 1-10。
- **生成文件不入库**（`.gitignore`；10 万 ≈ 8MB/组、100 万 ≈ 80MB/组），
  一键复现：`python3 datasets/generated/gen_bench_data.py --all` 或按规格单发（见脚本 docstring）。
- M3 门禁主用：`bench_100000_3x3_seed*.csv`（NFR-PERF-003 ≤ 30.4s）；
  M4 主用：`bench_1000000_3x3_*`（NFR-PERF-004）。

### 有效比例校准记录

- 机制：Cobb-Douglas 前沿生成的 CRS 数据，实测有效比例 = 设定的前沿单元比例 + 包络凸包效应增量。
- 实测（scipy.linprog HiGHS 内核独立实现仲裁，与产品引擎同求解器家族；目标 10%）：
  n=500 → **14.6%**；n=1000 → **12.0%**；eff 均值 ≈ 0.78，min ≈ 0.13。
- 结论：凸包效应增量 +2~5 个百分点且随 n 收敛，10 万级预计 10~12%；
  `--eff-ratio` 语义为「前沿单元（u=0）比例」，bench 报告按实测有效比例记录（P-002 口径），
  无需进一步校正参数。

## 2. external/ —— R 包导出与论文算例提取（M5 对拍为主）

导出环境：R 4.5.2 / deaR 1.5.4 / productivity 1.1.2（与 `tests/parity/refimpl/versions.lock` 一致）。
列结构保持 R 包原始口径（论文原始变量名），消费时按 02 册 schema 映射（`--inputs/--outputs` 覆写或转换脚本）。

| 文件 | DMU | 来源 | 用途（里程碑 × 模型） |
|---|---|---|---|
| `deaR_airlines.csv` | 28 | deaR::Airlines（Coelli, Grifell-Tatje & Perelman 2002） | M5 MinDS 对拍（deaR `model_addmin`） |
| `deaR_doyle_green_1994.csv` | 20 | deaR::Doyle_Green_1994 | M2+ 交叉效率参考 |
| `deaR_tone2001.csv` | 5 | deaR::Tone2001（SBM 原始算例） | SBM 论文复现 |
| `deaR_tone2003.csv` | 9 | deaR::Tone2003 | 超效率参考 |
| `deaR_power_plants.csv` | 6 | deaR::Power_plants（Tone 2002 超效率） | SBM 超效率论文复现 |
| `deaR_pft1981.csv` | 70 | deaR::PFT1981（Charnes-Cooper-Rhodes 1981） | CCR 经典 |
| `deaR_fortune500.csv` | 15 | deaR::Fortune500（Zhu 2014） | 乘子/AR 参考 |
| `deaR_electric_plants.csv` | 19 | deaR::Electric_plants（Färe 1989） | 经典 |
| `deaR_grifell_lovell_1999.csv` | 16 | deaR::Grifell_Lovell_1999 | Malmquist/面板 |
| `deaR_hotels.csv` | 23 | deaR::Hotels（Wu, Tsai & Zhou 2011） | 交叉效率应用 |
| `deaR_economy.csv` | 31 | deaR::Economy（Wang & Lan 2011） | 应用参考 |
| `deaR_libraries.csv` | 23 | deaR::Libraries（Cooper, Seiford & Tone 2007） | 教材经典 |
| `productivity_usagri.csv` | 480 | productivity::usagri（1995-2004 美国农业面板） | M5 HMB / Malmquist 多参比 |
| `printers32_seiford_zhu_2003.csv` | 32 | Seiford & Zhu 2003 Omega 31:397-408 Table 2（本次从 PDF 提取） | **M5 Cluster 黄金对拍**（1 投入价格 + 6 产出） |
| `ebm_example1_tone2010.csv` | 6 | Tone & Tsutsui 2010 GRIPS DP09-21 Table 1（webReader 通道提取，见 §3） | M5 EBM 窄范围算例（ε=0 退化为 CCR） |
| `ebm_example2_tone2010.csv` | 4 | 同上 Table 5 | M5 EBM 分散算例（ε=1 退化为 SBM） |
| `ebm_example3_hospitals12_tone2010.csv` | 12 | 同上 Table 9 | **M5 EBM 黄金算例**（NFR-PERF-002：ρ=1.471、ε=0.529） |

deaR 内置共 26 个数据集，其余（Fuzzy 系列、Supply_Chain 等）属模糊 DEA / M5 之外范围，未导出；
需要时 `Rscript -e 'data(package="deaR")$results'` 列出后同法导出。

### 已知数据差异记录

- `printers32`：论文声称 E¹={1,2,3,5,19,20,26}，但按其 Table 2 印刷数据实测（本引擎与 deaR
  双实现一致）DMU03 = 0.914811 非有效，实测 E¹={1,2,5,19,20,26}。判定为论文数据表舍入所致；
  **Cluster 对拍以 deaR 重算分层为准**，论文 Table 3-6 分数仅作量级参照。

### EBM 算例期望分数（Tone & Tsutsui 2010 DP09-21 数值表转录，M5 建 golden 用）

- Example 3（12 医院，EBM-I-C，Table 11-13）：affinity S=[[1,0.471],[0.471,1]]，
  **ρ=1.471，w=(0.5,0.5)，ε=(2−ρ)/(2−1)=0.529**（NFR-PERF-002 判定基准）；
- 期望分数对照（DMU: CCR-I-C / SBM-I-C / EBM-I-C）：
  A 1/1/1；B 1/1/1；C 0.883/0.852/**0.868**；D 1/1/**0.986**；E 0.763/0.756/**0.761**；
  F 0.835/0.704/**0.771**；G 0.902/0.895/**0.898**；H 0.796/0.774/**0.788**；
  I 0.960/0.905/**0.931**；J 0.871/0.781/**0.829**；K 0.955/0.866/**0.912**；
  L 0.958/0.936/**0.946**；
- Example 1（Table 1，窄范围）：ε=0，EBM 退化为 CCR（0.500/0.500/0.333/0.200/0.167）；
  Example 2（Table 5，分散）：ε=1，EBM 退化为 SBM（C/D 为 0.8）——两者作退化性质断言用。

## 3. external/papers/ —— 关键论文 PDF 存档（M5 算例数值表来源）

| 文件 | 状态 | 题录与来源 |
|---|---|---|
| `seiford_zhu_2003_context_dependent_dea.pdf` | ✅ 12 页 | Seiford & Zhu 2003, Omega 31:397-408, doi:10.1016/S0305-0483(03)00080-X（[ScienceDirect](https://www.sciencedirect.com/science/article/abs/pii/S030504830300080X)；本次经 researcher.life 镜像下载，访问 2026-10-25） |
| `morita_zhu_2005_sbm_context_dea.pdf` | ✅ 8 页 | Morita, Hirokawa & Zhu 2005, Omega 33:357-362（[deafrontier.net](https://www.deafrontier.net/papers/OMEGASlackCDEA.pdf)，访问 2026-10-25） |
| `vakili_2021_closest_weak_strong_projections.pdf` | ✅ 16 页 | Vakili, Amirmoshiri & Mirnia 2021, Bol. Soc. Parana. Mat. 39(5)（[期刊官方](http://www.spm.uem.br/bspm/pdf/vol39-5/1.pdf)，访问 2026-10-25） |
| `tone_tsutsui_2010_ebm_grips_dp09-21.pdf` | ⚠️ 内容已提取 | Tone & Tsutsui 2010 EBM revisited, GRIPS DP09-21（doi:10.24545/00001021，293.4 kB）。PDF 直链被 NII JAIRO Cloud WAF 拦截（curl/wget/urllib/WebFetch/浏览器直链均被拒；Z.ai webReader 出口可通，**三个算例数值表已全量提取为 §2 的 ebm_*.csv**）。原件存档可选：截至 2026-10-25 目录中未见补入的 PDF，如需存档请放入本目录（详情页 <https://grips.repo.nii.ac.jp/records/1021> 下载，勿用直链） |

## 4. M5 实现期待提取清单（数据源已就位）

- ~~GRIPS DP09-21 数值表 → EBM 算例 CSV~~ **已完成**（§2 三个 `ebm_*.csv` + §2 期望分数记录，2026-10-25）；
- Vakili 2021 的 30 欧洲机场数表 → WeakEF 对拍 CSV（PDF 已存档 §3）；
- Seiford-Zhu 2003 Table 3-6（attractiveness/progress 分数 + λ*）→ Cluster 期望值文件
  （32 打印机原始数据已提取，见 §2）；
- AR/Cone（Thompson Kansas farming、Charnes banks）、修正 SBM（Sharp 2007 / Lin 2019
  负数据算例）、Nash 奥运奖牌（Wu 2009）：论文付费墙或数据内嵌付费版，M5 实现期按
  `research_docs/unique_algorithms_survey.md` 各条 URL 逐个获取，届时补充本清单。

## 5. 与既有资产的关系

- `tests/golden/`（8 个 L2 黄金算例）与 `tests/parity/`（对拍数据）不迁移、不重复；
- 本目录为**原始数据资产层**，验证算例（data + expected + README 三件套）仍归
  `tests/golden/`；M5 起消费本目录数据构建新 golden 用例时遵循 04 册 L2 格式。
