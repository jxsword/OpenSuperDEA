# MaxDEA 软件详解与开源替代方案

> 从飞书文档导出归档（2026-10-24），原始文档: https://my.feishu.cn/docx/IhJCdJVFioMyU5xfmxyc2Qpfn7f

# MaxDEA 软件详解与开源替代方案

> 调研日期：2026-10-24  
> 结论速览：**没有任何单一开源软件能 100% 复现 MaxDEA X 的全部功能**，但通过 R 生态（Benchmarking + deaR + rDEA 等）的组合，可以实现绝大多数常规研究乃至大部分高级功能；MaxDEA 独有/领先的是 MinDS（最小距离强前沿）、FDEF（全维有效面）输出、博弈交叉效率、以及超大数据极速求解。

---

## 一、MaxDEA 软件详解

### 1. 基本信息

| 项目 | 内容 |
|-|-|
| 全称 | MaxDEA（数据包络分析软件） |
| 作者 | 成刚（北京大学中国卫生发展研究中心） |
| 配套教材 | 《数据包络分析方法与MaxDEA软件》（成刚著，第2版书稿可免费下载） |
| 版本体系 | MaxDEA X（当前主力，v12.5）/ MaxDEA Basic（免费）/ MaxDEA Lite（X 的免费版）/ 历史版本 MaxDEA Pro / Ultra / 6-9 |
| 平台 | MaxDEA X 原生跨平台：Windows、macOS、Linux；历史版本为 Excel 插件（需 64 位 Office 专业版） |
| 授权 | 商业软件；Basic / Lite 免费（Lite 仅限径向、SBM、成本、收益等基础模型，数据上限 1000 行） |
| 特色 | 模型组合超 3 万个；大数据极速求解（10000 个 DMU 的 CCR 模型约 2 秒级） |

### 2. 核心功能（MaxDEA X 官方模型清单）

**模型类型**：Envelopment（包络）、Multiplier（乘数/前沿生产函数）、FDH（自由处置包络）。

**效率测度距离**：

- 径向（Radial）：CCR（1978）、BCC（1984）
- 最大距离：ERM（增强 Russell 测度）、SBM（基于松弛的测度，Tone 2001）
- 最小距离到弱有效前沿
- 最小距离到强有效前沿（**MinDS / Closest Target**）：支持强单调性算法、CRS/VRS/NIRS/NDRS、非导向/投入/产出导向
- 方向距离函数（DDF）：支持多种方向向量（DMU 自身、均值、单位向量、RDM、自定义）
- 加性模型族：简单加性、归一化加权加性（Lovell & Pastor）、RAM（范围调整测度）、BAM（有界调整测度）、DSBM（方向 SBM）等
- 混合距离：EBM（基于 ε 的测度，Tone & Tsutsui 2010）
- 成本模型、收益模型

**导向**：投入导向 / 产出导向 / 非导向。

**规模报酬（RTS）**：CRS、VRS、NIRS、NDRS。

**TFP 指数**：

- Malmquist 指数：相邻 / 固定 / 全局 / 序贯 / 窗口（相邻、固定）/ 全局+序贯 等多种参比方式
- Hicks-Moorsteen 指数（HMB）
- 分解：效率变化（追赶）、技术变化（前沿移动）、规模效率变化、有偏技术变化（TC = OBTC × IBTC × MATC，Färe et al. 1997）

**其他模型**：

- Window DEA（窗口分析）
- Cluster 模型（自基准、交叉基准、向上/向下/相邻/窗口/固定基准）
- 超效率模型（Super-efficiency）
- 修正 SBM（Sharp et al. 2007；Lin et al. 2019）
- 交叉效率（Cross efficiency，多阶段）：Blanket Benevolent / Aggressive（Doyle & Green 1995 两类）、Ruiz (2013) 两类、自定义虚拟 DMU 的仁慈/攻击型
- **博弈交叉效率**：Nash 均衡模型（Liang et al. 2008；Wu et al. 2009）
- 非期望产出、期望投入（Undesirable outputs/inputs）
- 非自由处置（Nondiscretionary）模型
- 偏好（加权）模型
- 受限乘数模型（Cone Ratio DEA、Restricted Cone DEA、Assurance Region DEA）

**特有输出**：可输出所有 **FDEF（全维有效面）** 的顶点与超平面方程（分段前沿生产函数）。

### 3. MaxDEA 自身技术栈（按版本演进）

| 版本阶段 | 技术架构 |
|-|-|
| MaxDEA 5 / 6.x（早期） | **Access VBA 平台**：软件本体是 Access 数据库文件（.mdb），不是 exe、也无安装程序，免安装直接打开运行；程序与数据整合在同一个 Access 文件中（成刚教材官方说明） |
| MaxDEA 6.3 起 | 部分模块重新设计；MaxDEA 9 为**单一 64 位 exe 可执行文件**（MaxDEA.exe），免安装、界面友好，数据无需特殊格式 |
| MaxDEA Pro / Ultra（商业版） | **Excel 插件架构（COM Add-in）**：以 64 位 Microsoft Office 专业版（2010 SP1+）为宿主，VBA/COM 调用自研求解逻辑 |
| MaxDEA X（12.x，当前主力） | **原生跨平台独立桌面应用（standalone）**：Windows / macOS / Linux 原生支持；官方未公开底层语言与框架细节，以自研高速算法实现大数据极速求解（10,000 DMU × 3投入 × 3产出 CCR 约 2 秒级，MaxDEA 9 需 5 分 43 秒） |
| InverseDEA（配套逆 DEA 软件） | Windows only，需 64 位 MS Office Professional |

> 小结：MaxDEA 的技术栈经历了 **Access VBA（.mdb）→ 独立 exe → Excel 插件 → 原生跨平台桌面应用** 的演进；当前 X 版已完全脱离 Office 依赖，属于闭源商业软件，底层求解器细节不公开。

---

## 二、开源免费替代：能否实现全部功能？

### 判断

| 结论 | 说明 |
|-|-|
| ❌ 单一开源软件 | 无任何开源软件能完整覆盖 MaxDEA X 的**全部**功能（尤其 MinDS 强单调算法、FDEF 全维有效面输出、博弈交叉效率 Nash、3 万+ 模型组合、超大数据极速求解） |
| ✅ 常规研究（>90% 需求） | 开源组合**可以**实现：CCR/BCC、SBM、超效率、方向距离、Malmquist 及分解、窗口分析、成本/收益、交叉效率、非期望产出、非自由处置等绝大多数期刊论文常用模型 |
| ✅ 前沿扩展 | 部分开源工具提供 MaxDEA 没有或较弱的扩展：模糊 DEA（deaR）、Robust/order-m 效率（rDEA）、SFA 随机前沿（Benchmarking）、bootstrap 统计推断（DEA Toolbox、Benchmarking） |

### 主要开源/免费工具清单

| 工具 | 类型 | 语言/平台 | 许可证 | 覆盖模型亮点 |
|-|-|-|-|-|
| **Benchmarking** | R 包（CRAN，v0.33） | R + lp_solve | 开源（CRAN 分发） | DEA（fdh/vrs/drs/crs/irs/add）+ SFA；投入/产出/双曲/加性/超效/方向测度；成本/收益/利润；价格信息；bootstrap |
| **deaR** | R 包（CRAN，v1.5.2） | R + LP 求解 | 开源（CRAN 分发） | 常规 + 模糊 DEA；超效率、交叉效率、Malmquist、窗口分析、非期望产出；含 Shiny Web 应用（deaR-Shiny） |
| **rDEA** | R 包（CRAN） | R + C++ | 开源（CRAN 分发） | 标准 DEA（投入/产出/成本最小化）+ Robust DEA（order-m，可含环境变量） |
| **DEA / metafrontier / deaviz** | R 包（CRAN） | R | 开源（CRAN 分发） | 径向 CCR/BCC、SBM、加性、方向距离；共同前沿分析；可视化 |
| **DEA Toolbox（Data Envelopment Analysis Toolbox）** | MATLAB 工具包 | MATLAB + Optimization/Statistics Toolbox | **GPL-3** | 径向、方向、加性、超效率、Malmquist、Malmquist-Luenberger、成本/收益/利润、非期望产出、bootstrap |
| **OSDEA（Open Source DEA）** | 独立 GUI + 代码库 | Java | **GPL-3** | 40 个 DEA 模型（CCR/BCC/SBM/IRS/DRS/GRS/NC/ND 等），输出 DMU 分数、投影、参考集、松弛、权重；跨平台 |
| **pyDEA** | Python 库（PyPI/GitHub） | Python + PuLP(CBC) | **MIT** | CCR/BCC/SBM 等基础模型；GUI + CLI + API 三接口；可读 xls/xlsx/csv |
| **DEA Frontier** | Excel 插件（免费版） | Excel VBA + OpenSolver(COIN-OR CBC) | 免费（非开源） | 径向、超效率、Malmquist 等；免费版限 20 DMU |
| **DEAP 2.1** | 命令行程序 | Windows 可执行 | 免费（非开源） | CCR/BCC 输入导向 + Malmquist（Tim Coelli，CEPA） |

---

<html5-block alt="MaxDEA X 与主要开源 DEA 工具的功能覆盖矩阵，图例：实心圆完整支持、半空心圆部分支持、空心圆不支持" data-ref="html5_1"></html5-block>

## 三、功能覆盖矩阵（MaxDEA X vs 主要开源工具）

图例：● 完整支持 ／ ◐ 部分或有限支持 ／ ○ 不支持

| 功能 | MaxDEA X | Benchmarking | deaR | rDEA | DEA Toolbox | OSDEA | pyDEA |
|-|-|-|-|-|-|-|-|
| CCR / BCC 径向 | ● | ● | ● | ● | ● | ● | ● |
| SBM / 非径向 | ● | ◐ | ● | ○ | ○ | ● | ◐ |
| 超效率 | ● | ● | ● | ○ | ● | ○ | ◐ |
| 方向距离函数 | ● | ● | ◐ | ○ | ● | ○ | ○ |
| 成本 / 收益模型 | ● | ● | ◐ | ● | ● | ○ | ○ |
| 交叉效率 | ● | ○ | ● | ○ | ○ | ○ | ○ |
| Malmquist 指数（分解） | ● | ◐ | ● | ○ | ● | ○ | ○ |
| 窗口 / Cluster 分析 | ● | ○ | ● | ○ | ○ | ○ | ○ |
| 非期望产出 / 非自由处置 | ● | ◐ | ● | ○ | ● | ● | ○ |
| 模糊 / Robust (order-m) DEA | ○ | ○ | ● | ● | ○ | ○ | ○ |
| MinDS / Closest Target | ● | ○ | ○ | ○ | ○ | ○ | ○ |
| 博弈交叉效率（Nash） | ● | ○ | ○ | ○ | ○ | ○ | ○ |
| FDEF 输出 / 超大数据极速 | ● | ○ | ○ | ○ | ○ | ○ | ○ |

> 注：覆盖状态依据各工具官方文档/CRAN 说明整理，◐ 表示部分支持或实现方式有限（例如 Benchmarking 通过加性/方向模型间接覆盖部分 SBM 与含非期望产出的场景）。

---

## 四、开源替代的技术栈

### 推荐组合路线

**路线 A（最完整，科研首选）：R 生态**

- 技术栈：**R + lp_solve（经 lpSolveAPI 接口）+ quadprog/ucminf（二次规划）+ Rcpp（C++ 加速）**
- 组合：Benchmarking（效率测度 + SFA + 成本收益利润）+ deaR（交叉效率/Malmquist/窗口/模糊 DEA）+ rDEA（order-m robust）+ metafrontier（共同前沿）+ deaviz（可视化）
- 优点：模型覆盖最广、可编程可复现、与 Stata/Python 数据互通、免费开源
- 缺点：无 GUI 一键操作（需写 R 脚本）；大数据量求解速度不及 MaxDEA X

**路线 B：MATLAB 路线**

- 技术栈：**MATLAB + Optimization Toolbox（linprog 线性规划）+ Statistics and Machine Learning Toolbox（bootstrap/统计检验）**
- 工具：Data Envelopment Analysis Toolbox（GPL-3，源码可下载）
- 优点：含 Malmquist-Luenberger、bootstrap 统计推断；学术社区常用
- 缺点：需 MATLAB 商业许可（工具本身开源免费）

**路线 C：Python 路线**

- 技术栈：**Python 3 + PuLP（LP/MILP 建模，≥1.6.1）+ CBC（COIN-OR 默认求解器，可换 GLPK）+ openpyxl（Excel 读写）+ wxPython（GUI）**
- 工具：pyDEA（MIT 许可，GUI/CLI/API）
- 优点：可嵌入现有 Python 数据管线；MIT 许可最宽松
- 缺点：维护较早（2016 年至今），模型覆盖面有限，大规模性能一般

**路线 D：Java / 独立软件**

- 技术栈：**Java（OSDEA-GUI 桌面应用 + OSDEA-Code 库，提供 Javadoc），GPL-3**
- 优点：无需 MATLAB/Office；GUI 开箱即用；40 模型
- 缺点：缺少 Malmquist、交叉效率、超效率等高级模型

**路线 E：Excel 插件**

- 技术栈：**Excel VBA 插件（.xla/.xlam）+ OpenSolver（开源 Excel 优化器）+ COIN-OR CBC 求解引擎**
- 工具：DEA Frontier 免费版（Joe Zhu 开发）、OpenSolver 版无 DMU 数量限制
- 优点：上手门槛最低，Excel 用户零学习成本
- 缺点：免费版功能与规模受限；核心不开源

---

## 五、MaxDEA 与开源工具优缺点对比

### 横向对比表

| 对比维度 | MaxDEA X | R 生态（Benchmarking+deaR+rDEA） | DEA Toolbox（MATLAB） | OSDEA（Java） | pyDEA（Python） |
|-|-|-|-|-|-|
| 成本 | 商业付费（Basic/Lite 免费但受限） | 免费开源 | 免费开源（GPL-3） | 免费开源（GPL-3） | 免费开源（MIT） |
| 模型覆盖 | 最全：3 万+ 组合，含 MinDS/FDEF/博弈交叉效率 | 广：常规 + 模糊/robust/SFA；缺 MinDS/FDEF | 中：径向/方向/加性/超效/Malmquist/Luenberger | 中：40 模型，缺超效率/Malmquist/交叉效率 | 少：基础 CCR/BCC/SBM |
| 易用性 | GUI 极简，中文教材+演示视频 | 需写 R 代码，学习曲线陡 | 需 MATLAB 操作技能 | GUI 开箱即用 | GUI / CLI / API 三接口 |
| 大数据性能 | 极速：1 万 DMU 约 2 秒 | 一般：lp_solve 求解大模型较慢 | 取决于 MATLAB 与机器 | 一般 | 一般 |
| 可复现 / 自动化 | 差：GUI 手动操作，难脚本化 | 强：脚本 + 参数化，R Markdown 可复现 | 较强：脚本驱动 | 中：GUI / 库调用 | 强：可嵌入 Python 管线 |
| 扩展能力 | 闭源，不可扩展 | 强：开源可改，社区活跃 | 强：GPL 源码可改 | 强：代码库 + Javadoc | 中：项目已基本停止维护 |
| 统计推断 | 无 | 有：bootstrap、SFA、order-m 等 | 有：bootstrap 与统计检验 | 无 | 无 |
| 平台 | Windows / macOS / Linux | 全平台 | Windows / macOS / Linux | 全平台（JVM） | 全平台 |
| 维护状态 | 持续更新（当前 v12.5） | CRAN 活跃维护 | 较活跃 | 不活跃（2016 年前后） | 基本停止（2016） |

### 各自核心优劣势速览

**MaxDEA X**

- 优势：模型数量与前沿程度业内第一；极速求解大数据；GUI 上手门槛极低；配套成刚中文教材与演示视频，中文科研圈事实标准。
- 劣势：商业收费（个人/学生成本高）；闭源黑盒，求解算法不可审查、不可扩展；无法脚本化，批量/复现流程不友好；不含 bootstrap、SFA 等统计推断扩展。

**R 生态（Benchmarking / deaR / rDEA / metafrontier / deaviz）**

- 优势：免费开源、学术主流引用；可编程可复现（论文级严谨性）；独有扩展——模糊 DEA、order-m robust、SFA 随机前沿、bootstrap、共同前沿；跨平台；持续维护。
- 劣势：无图形界面，需掌握 R；多个包拼凑使用，模型体系不如 MaxDEA 统一；大数据场景求解速度明显慢于 MaxDEA X；MinDS/FDEF/博弈交叉效率等前沿模型缺失。

**DEA Toolbox（MATLAB，GPL-3）**

- 优势：开源免费（工具本身）；MATLAB 可视化与编程生态好；含 Malmquist-Luenberger 与 bootstrap 统计检验；有 Excel 版 DEA Frontier 同源。
- 劣势：依赖 MATLAB 商业许可与 Optimization/Statistics Toolbox；模型数量少于 MaxDEA；无 SBM、MinDS 等；受众限于 MATLAB 用户。

**OSDEA（Java，GPL-3）**

- 优势：免费开源；独立 GUI 开箱即用；全平台；40 个模型含 NC/ND 非自由处置；提供代码库可直接嵌入自研程序。
- 劣势：缺超效率、Malmquist、交叉效率等常用高级模型；界面较陈旧；社区与维护不活跃；无统计推断。

**pyDEA（Python，MIT）**

- 优势：纯 Python 技术栈（PuLP + CBC），可嵌入现有 Python 数据管线；GUI/CLI/API 三接口；MIT 许可最宽松。
- 劣势：2016 年后基本停止维护（Python 3.3–3.5 时代产物）；模型仅基础 CCR/BCC/SBM 类；性能与生态均落后。

**DEA Frontier 免费版 / DEAP 2.1**

- 优势：免费；Excel 插件（DEA Frontier）零学习成本；DEAP 2.1 是 Coelli 经典教材配套、结果权威可信。
- 劣势：均非开源；功能与规模受限（DEA Frontier 免费版限 20 DMU，DEAP 仅 CCR/BCC 输入导向 + Malmquist，命令行无界面）。

---

## 六、选型建议

1. **论文复现 / 常规科研（CCR、BCC、SBM、Malmquist、超效率、交叉效率）**：直接用 **R 的 deaR + Benchmarking**，免费、可复现、文献引用率高。
2. **需要成本/收益、含价格信息、SFA 对照、bootstrap**：优先 **Benchmarking**（R）或 **DEA Toolbox**（MATLAB）。
3. **需要模糊 DEA、共同前沿、Robust/order-m**：**deaR + rDEA + metafrontier** 是开源独有强项（MaxDEA 不覆盖）。
4. **需要 MinDS（Closest Target）、FDEF 全维有效面、博弈交叉效率、或 10 万级 DMU 极速求解**：开源目前无替代，仍需 **MaxDEA X**（或 MaxDEA Lite 免费版体验基础功能）。
5. **预算敏感且只要基础模型**：先下载 **MaxDEA Lite**（免费，1000 行内基础模型），再按需迁移到 R。

---

## 主要信息来源

- MaxDEA 官方功能列表（英文）：http://maxdea.com/Features_X.htm
- MaxDEA 官网（中文）：https://www.maxdea.com/Index_CN.htm
- MaxDEA 下载/免费版说明：https://maxdea.com/Download.htm
- MaxDEA 速度对比：http://maxdea.com/SpeedCompare.htm
- 成刚《数据包络分析方法与MaxDEA软件》教材原文（Access VBA 平台 / .mdb 架构）：https://file.lianxh.cn/PDFTW/%E6%95%B0%E6%8D%AE%E5%8C%85%E7%BB%9C%E5%88%86%E6%9E%90%E6%96%B9%E6%B3%95%E4%B8%8EMAXDEA%E8%BD%AF%E4%BB%B6.pdf
- MaxDEA 9 单文件 exe 说明（北京友万）：http://www.uone-tech.cn/MaxDEA.html
- MaxDEA X 12.2 standalone 跨平台说明（学术论文）：https://pdfs.semanticscholar.org/ae20/6671cab2dd555fec5449830e5443c5fccfba.pdf
- Benchmarking 包文档（CRAN）：https://www.stats.bris.ac.uk/R/web/packages/Benchmarking/refman/Benchmarking.html
- deaR 包文档（CRAN）：https://publications.artsci.wustl.edu/web/packages/deaR/refman/deaR.html
- rDEA 包手册：https://cran.r-universe.dev/rDEA/doc/manual.html
- Data Envelopment Analysis Toolbox FAQ：https://www.deatoolbox.com/faq/
- Open Source DEA（OSDEA）：https://opensourcedea.org/ ；模型列表：https://opensourcedea.org/detailed-features/
- pyDEA（GitHub）：https://github.com/araith/pyDEA ；依赖：https://raw.githubusercontent.com/araith/pyDEA/master/requirements.txt
- DEA Frontier / OpenSolver 说明：https://www.deafrontier.net/deasupport/ ；https://opensolver.org/dea-frontier-for-opensolver/