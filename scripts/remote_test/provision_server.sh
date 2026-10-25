#!/usr/bin/env bash
# provision_server.sh —— juzdata 云服务器一次性环境安装（幂等；DR-009）。
# 分两段：sudo 段（交互输一次密码）+ 用户级段（免 root，自动放 tmux 并轮询）。
# 用法：bash scripts/remote_test/provision_server.sh
set -euo pipefail

HOST=juzdata
VCPKG_COMMIT=a1cae005c3   # 与本机一致（git -C ~/vcpkg rev-parse --short=10 HEAD）
R_LIB_DIR=/data/Rlib
SWAP_FILE=/data/swapfile

echo "=== [1/2] sudo 段（需要输入一次密码）==="
ssh -t "$HOST" bash -s <<'SUDO_EOF'
set -uo pipefail
echo "--- 磁盘预检 ---"
df -h /data | tail -1
if [ ! -w /data/home-sf ]; then
    echo "/data/home-sf 不可写，需要 root 创建："
    sudo mkdir -p /data/opensuperdea && sudo chown "$(whoami)" /data/opensuperdea
    echo "已创建 /data/opensuperdea"
fi
echo "--- R 栈（对齐 tests/parity/refimpl/versions.lock 的 R 4.5.x）---"
if ! command -v Rscript >/dev/null; then
    # CRAN apt 仓库优先（发行版 R 版本偏旧时退回并记录）
    sudo apt-get update -qq
    if ! sudo apt-get install -y --no-install-recommends r-base r-base-dev software-properties-common dirmngr >/dev/null 2>&1; then
        echo "警告：R 安装失败（后续 parity 套件不可用）"
    fi
    R_V=$(Rscript -e 'cat(as.character(getRversion()))' 2>/dev/null || echo "未装")
    echo "R 版本: $R_V（非 4.5.x 时记录入 env.txt，对拍结果标注差异）"
else
    echo "R 已安装：$(Rscript -e 'cat(as.character(getRversion()))')"
fi
sudo apt-get install -y --no-install-recommends r-base-dev gfortran libblas3 liblapack3 python3-pip >/dev/null
echo "--- swapfile（内存 3.8G 无 swap，兜底编译峰值；推荐创建）---"
if [ ! -f /data/swapfile ]; then
    read -r -p "创建 4G swapfile 于 ${SWAP_FILE}? [Y/n] " answer
    if [ "${answer:-Y}" = "Y" ]; then
        sudo fallocate -l 4G "$SWAP_FILE" && sudo chmod 600 "$SWAP_FILE" &&
            sudo mkswap "$SWAP_FILE" >/dev/null && sudo swapon "$SWAP_FILE" &&
            echo "$SWAP_FILE none swap sw 0 0" | sudo tee -a /etc/fstab >/dev/null &&
            echo "swap 已启用"
    fi
else
    swapon --show=NAME | grep -q "$SWAP_FILE" && echo "swap 已存在" || sudo swapon "$SWAP_FILE"
fi
echo "sudo 段完成"
SUDO_EOF

echo "=== [2/2] 用户级段（免 root，tmux 后台 + 轮询）==="
ssh "$HOST" bash -s -- "$VCPKG_COMMIT" "$R_LIB_DIR" <<'USER_EOF'
set -uo pipefail
VCPKG_COMMIT=$1
R_LIB_DIR=$2
BASE=/data/home-sf/opensuperdea
[ -d /data/home-sf ] || BASE=/data/opensuperdea
RUNNER=/data/home-sf/provision_user.sh
cat > "$RUNNER" <<RUNNER_EOF
#!/usr/bin/env bash
set -uo pipefail
{
echo "--- vcpkg（构建 -j2，约 10-20 分钟）---"
if [ ! -d /data/vcpkg ]; then
    git clone https://github.com/microsoft/vcpkg /data/vcpkg
fi
git -C /data/vcpkg fetch origin >/dev/null 2>&1 || true
git -C /data/vcpkg checkout "$VCPKG_COMMIT"
/data/vcpkg/bootstrap-vcpkg.sh -disableMetrics
echo "--- R 用户库（R_LIBS_USER=$R_LIB_DIR）---"
mkdir -p "$R_LIB_DIR"
export R_LIBS_USER="$R_LIB_DIR"
Rscript -e 'install.packages(c("deaR","productivity"), repos="https://cloud.r-project.org", lib=Sys.getenv("R_LIBS_USER"), quiet=TRUE)'
# 实际加载验证（环境册 §4 注 4 教训：装了 ≠ 能加载）
Rscript -e 'library(deaR); library(productivity); cat("R 包加载验证 OK\n")' || echo "警告：R 包加载失败"
echo "--- gcovr（PEP 668）---"
pip3 install --break-system-packages --quiet gcovr || pip3 install --break-system-packages gcovr
echo "--- env.txt 硬件/版本档案（P-014）---"
mkdir -p "$BASE"
{
    uname -a
    lscpu | grep -E "Model name|^CPU\(s\)|MHz" || true
    free -h
    df -h /data | tail -1
    gcc --version | head -1
    cmake --version | head -1
    ninja --version
    Rscript -e 'cat("R", as.character(getRversion()), "\n")'
    Rscript -e 'cat("deaR", as.character(packageVersion("deaR")), "\n")' 2>/dev/null
    Rscript -e 'cat("productivity", as.character(packageVersion("productivity")), "\n")' 2>/dev/null
    /data/vcpkg/vcpkg version 2>/dev/null | head -1
    python3 -m gcovr --version 2>/dev/null | head -1
} > "$BASE/env.txt" 2>&1
echo "provision 完成"
} > "\$HOME/provision.log" 2>&1
echo DONE > "\$HOME/provision.status"
RUNNER_EOF
chmod +x "$RUNNER"
ssh "$HOST" "tmux kill-session -t osd-provision 2>/dev/null; tmux new-session -d -s osd-provision 'bash $RUNNER'"
echo "provision 已在服务器 tmux 会话 osd-provision 后台运行，轮询中（约 15-30 分钟）..."
while true; do
    status=$(ssh "$HOST" 'cat /home/sf/provision.status 2>/dev/null || echo RUNNING')
    if [ "$status" = "DONE" ]; then
        echo "=== provision 完成 ==="
        ssh "$HOST" 'tail -5 /home/sf/provision.log; echo "--- env.txt ---"; cat /data/home-sf/opensuperdea/env.txt 2>/dev/null'
        break
    fi
    sleep 30
done
USER_EOF
echo "=== provision_server.sh 全部完成 ==="
