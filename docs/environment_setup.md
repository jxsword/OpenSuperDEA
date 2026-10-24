# OpenSuperDEA 开发与验证环境基线（Environment Setup）

> 版本：V1.0 ｜ 2026-10-24 ｜ 阶段三依赖预装交付物（DR-005 授权范围：全量含 Qt6）
> 用途：记录当前机器环境审计结果、本次安装清单、验证证据与环境注意事项；供 CI 对照与未来机器复现（Windows 侧见 CI 工作流）。

## 1. 环境审计基线（安装前已就绪，无需安装）

| 类别 | 项 | 版本/状态 |
|---|---|---|
| 构建 | CMake / Ninja / Make | 4.4.3 / 1.13.2 / 4.4.1 |
| 编译器 | GCC（gcc/g++） | 16.0.1（experimental trunk） |
| 版本控制 | git / gh | 2.53.0 / 已登录（jxsword，https） |
| 代码质量 | clang-format / clang-tidy + pre-commit 钩子 | 23/22/21 探测链，core.hooksPath 已配置 |
| Python | python3 + pip（pyenv） | 3.13.13 / pip 26.0.1 |
| Node | node / npm（nvm，用户级） | v24.15.0 / 11.12.1 |
| Qt6 | qt6-base-dev / qt6-base-dev-tools / wayland / svg / qmake6 | 已装（dpkg 确认） |
| vcpkg | ~/vcpkg（已 bootstrap + 二进制缓存目录） | 2026-07-27 版 |
| 硬件 | 磁盘 / 内存 / 核数 | 752G 可用 / 31GiB / 8 核 |

## 2. 本次安装清单（缺失项补装）

| # | 项 | 安装方式 | 结果/版本 | 备注 |
|---|---|---|---|---|
| 1 | R 工具链：r-base、r-base-dev、gfortran | `zcode-run sudo -n apt-get install`（sudoers 白名单内） | R 4.5.2 / gfortran 15.2.0 | |
| 2 | 数值库：libblas-dev、liblapack-dev | apt（同上） | 已装 | |
| 3 | libgfortran-15-dev（链接用 .so） | apt | 已装 | 见 §4 注 1 |
| 4 | R 包编译依赖：libuv1-dev、libglu1-mesa-dev、libx11-dev、libxt-dev、libcurl4-openssl-dev、libssl-dev、libxml2-dev、libpng-dev、libjpeg-dev、libtiff-dev | apt（一次装齐） | 9+1 项已装 | fs/rgl/plotly 链依赖 |
| 5 | R 包：deaR、productivity（含依赖共 100+ 包） | `install.packages` → 用户库 `~/R/x86_64-pc-linux-gnu-library/4.5` | deaR 1.5.4 / productivity 1.1.2 | 见 §4 注 2 |
| 6 | Python：DEAPack（PyPI） | pip | 2.0.1（import 验证通过） | |
| 7 | Python：PyDEA（MIT，非 PyPI） | `git clone` → `~/dea_tools/pyDEA` + pip 装 requirements（pulp、openpyxl） | import 验证通过（PYTHONPATH 方式使用） | GPL/MIT 对拍工具置于产品仓库外 |
| 8 | vcpkg manifest 预热：eigen3 / highs / cli11 / gtest / qhull | `tmp/vcpkg_warmup/vcpkg.json` + `vcpkg install` | **全部成功，3.8 分钟**；HiGHS 1.15.1、Qhull 8.0.2 等已入二进制缓存 | M1 配置时秒级复用 |
| 9 | mermaid CLI（@mermaid-js/mermaid-cli） | npm -g（需镜像源，见 §4 注 3） | mmdc 12.0.0，中文图渲染验证通过 | 可选项 |

## 3. 验证证据（冒烟测试）

| 验证 | 命令/方法 | 结果 |
|---|---|---|
| deaR 真算 CCR | 4 DMU 手造数据 `model_basic(io, crs)` | 效率 0.3333/0.6667/1/0.9333，**与手算一致** ✅ |
| productivity Malmquist | 内置 usagri 数据 `malm(crs, in)` | 全量进度跑完，Changes 输出生成 ✅ |
| productivity HMB | `hicksmoorsteen(usagri…)` | 计算完成 ✅ |
| DEAPack | `python3 -c "import deapack"` | 2.0.1 导入 ✅ |
| PyDEA | `PYTHONPATH=~/dea_tools/pyDEA python3 -c "import pyDEA"` | 导入 ✅ |
| vcpkg 五端口 | warmup manifest 安装日志 | All requested installations completed ✅ |
| mmdc 渲染 | 中文标签 flowchart → SVG | 生成成功 ✅ |
| **design_docs 全部 mermaid 图（12 幅）** | mmdc 逐图渲染 | **12/12 PASS**（修复 02 册类图 2 处语法） ✅ |

## 4. 环境注意事项（本机特性，后续会话必读）

1. **sudo 白名单**：`zcode-run` 的免密 sudo 仅放行 `apt-get`（`ln` 等其他命令会报 interactive authentication required）；需要非 apt 的 root 操作时改用免 sudo 替代方案或请用户执行。
2. **GCC 16 + gfortran 15 混合工具链**：默认 `gcc` 为 16（experimental），而 gfortran 开发库在 `/usr/lib/gcc/x86_64-linux-gnu/15/`；**编译 R 包前需 `export LIBRARY_PATH=/usr/lib/gcc/x86_64-linux-gnu/15`**，否则 `-lgfortran` 链接失败（运行时不受影响，仅编译期需要）。R 包装入用户库（系统库不可写），`library()` 自动可见。
3. **npm/puppeteer 网络**：puppeteer 默认 Chromium 下载源不可达，需 `export PUPPETEER_DOWNLOAD_BASE_URL=https://registry.npmmirror.com/-/binary/chrome-for-testing` 后安装（本次 mmdc 9 秒装完）。
4. R `install.packages` 失败时 **Rscript 仍 exit 0**——验证必须用 `library()`/`requireNamespace()` 实际加载，不可信退出码。

## 5. 新机器复现指南（Ubuntu 系）

```bash
# sudo 段（经 zcode-run）
apt-get install -y r-base r-base-dev gfortran libblas-dev liblapack-dev \
  libgfortran-15-dev libuv1-dev libglu1-mesa-dev libx11-dev libxt-dev \
  libcurl4-openssl-dev libssl-dev libxml2-dev libpng-dev libjpeg-dev libtiff-dev \
  python3-pip qt6-base-dev qt6-base-dev-tools
# R 包（用户库）
export LIBRARY_PATH=/usr/lib/gcc/x86_64-linux-gnu/15
Rscript -e 'install.packages(c("deaR","productivity"), lib=Sys.getenv("R_LIBS_USER"), repos="https://cloud.r-project.org", Ncpus=8)'
# Python 对拍工具
pip install DEAPack && git clone https://github.com/araith/pyDEA.git ~/dea_tools/pyDEA && pip install -r ~/dea_tools/pyDEA/requirements.txt
# mermaid（可选）
export PUPPETEER_DOWNLOAD_BASE_URL=https://registry.npmmirror.com/-/binary/chrome-for-testing
npm install -g @mermaid-js/mermaid-cli
```

## 6. 与设计的对应关系

- 04 册验证计划的对拍矩阵（deaR/productivity/DEAPack/PyDEA）全部就位；`refimpl/versions.lock` 在 M2 建 harness 时按本页版本固化。
- 03 册 M1 的 vcpkg 五端口已入二进制缓存（HiGHS 1.15.1 / Qhull 8.0.2 / Eigen / CLI11 / GoogleTest）。
- Windows（MSVC，P-013）侧不在本机安装，由 Actions windows-latest + vcpkg manifest 覆盖（03 册 §6）。
