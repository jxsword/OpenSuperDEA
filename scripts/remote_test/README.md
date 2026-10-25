# scripts/remote_test —— 云服务器远程测试工作流（juzdata，DR-009）

把 >20 分钟的测试任务（68 例对拍、覆盖率全量、L4/M4 基准）放到云服务器 `ssh juzdata`
异步执行：tmux 会话运行、进度/结果写服务器文件、本机轮询、完成后自动回传。

## 文件

| 文件 | 用途 |
|---|---|
| `provision_server.sh` | 一次性环境安装（幂等）：sudo 段（R 栈/swapfile/目录）+ 用户级段（vcpkg/R 包/gcovr/env.txt 档案）。**首次使用需在本机执行，交互输一次 sudo 密码** |
| `remote_test.sh` | 日常单入口：`sync` / `run <suite>...` / `status` / `fetch [run-id]`。运行期零 root |

## 快速上手

```bash
# 一次性（需在场输 sudo 密码 + swap 确认）
bash scripts/remote_test/provision_server.sh

# 日常（免交互）
scripts/remote_test/remote_test.sh run all          # ctest + parity
scripts/remote_test/remote_test.sh run smoke        # 链路桩测试（无需 R/vcpkg）
scripts/remote_test/remote_test.sh status           # 查看最近 run
scripts/remote_test/remote_test.sh fetch 20261025_  # 补拉历史结果
```

## 服务器布局（全落 /data，系统盘仅 apt 包）

```text
/data/home-sf/opensuperdea/{src,build,build-coverage,vcpkg_installed}
/data/home-sf/opensuperdea-remote/<run-id>/{runner.sh,status,progress.log,results/}
/data/home-sf/vcpkg（checkout a1cae005c3，与本机一致）  /data/home-sf/Rlib（R_LIBS_USER）  /data/swapfile（4G）
```

## 关键约束与差异（务必知悉）

1. **glibc 2.39 < 本机 2.43**：二进制不可直传，每轮只同步源码（`sync` 用 rsync --delete，
   排除 build*/tmp/.git/vcpkg_installed/生成 CSV）；
2. **性能门禁保持本机基线**：服务器 2 核（本机 8 核）+ 单核性能打平（0.95s vs 0.965s，
   AVX-512），对拍/覆盖率/回归类结果可直接比对；**bench 计时与本机不可比**，服务器
   bench 仅作观察/回归，threads 矩阵缩为 {1, 2, auto}，门禁判定（P-002/P-014）一律
   以本机 bench 报告为准；
3. **内存 3.8G 无大余量**：构建/ctest 限 `-j2`，`run` 前自动预检（available <2.5G 警告），
   swapfile 兜底；M4 百万 DMU 是否上云待内存画像后再定；
4. **R 栈版本**：对齐 versions.lock（R 4.5.x）；CRAN apt 不可达时退回发行版 R 并记录
   于 `env.txt`，对拍结果标注差异；
5. 断线恢复：轮询 Ctrl-C 只退本机，远端 tmux 继续跑；`status` / `fetch` 随时查看回传。

## 结果落点

本机 `build-remote-results/<run-id>/`（`.gitignore` 的 `build-*/` 已覆盖，不入库），
内含 `progress.log`（全程输出）与 `results/summary.txt`（各套件退出码）。

## 故障排查

| 现象 | 处置 |
|---|---|
| `run` 卡在 vcpkg install | 服务器 2 核 + 无缓存的首次安装约 10-20 分钟，看 progress.log |
| parity 报 R 包不可用 | provision 的用户级段未完成或 R_LIBS_USER 未生效；`ssh juzdata 'Rscript -e "library(deaR)"'` 验证 |
| OOM killed | 确认 swapfile 已 swapon；减少并行（脚本已限 -j2） |
| tmux 会话丢失 | `ssh juzdata tmux ls` 查看；runner 独立于会话存续，重跑 `run` 生成新 run-id 即可 |
