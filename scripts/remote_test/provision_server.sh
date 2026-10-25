#!/usr/bin/env bash
# provision_server.sh —— juzdata 云服务器一次性环境安装（幂等；DR-009）。
# 【重要】本机以普通用户运行，不要 sudo：远端 sudo 密码在 ssh -t 分配的终端上交互输入。
# 分两段：sudo 段（scp 脚本到远端后 ssh -t 执行，密码交互一次）+ 用户级段（免 root，tmux 后台 + 本机轮询）。
# 用法：bash scripts/remote_test/provision_server.sh
set -euo pipefail

HOST=juzdata
VCPKG_COMMIT=a1cae005c3   # 与本机一致（git -C ~/vcpkg rev-parse --short=10 HEAD）
R_LIB_DIR=/data/Rlib
SWAP_FILE=/data/swapfile

# ---------- [1/2] sudo 段：脚本落远端文件后 ssh -t 执行（交互可靠） ----------
SUDO_PART=$(mktemp)
cat > "$SUDO_PART" <<'SUDO_EOF'
set -uo pipefail
echo "--- 磁盘预检 ---"
df -h /data | tail -1
if [ ! -w /data/home-sf ]; then
    echo "/data/home-sf 不可写，需要 root 创建："
    sudo mkdir -p /data/opensuperdea && sudo chown "$(whoami)" /data/opensuperdea
    echo "已创建 /data/opensuperdea"
fi
echo "--- R 栈（对齐 tests/parity/refimpl/versions.lock 的 R 4.5.x；apt 输出可见防假死）---"
export DEBIAN_FRONTEND=noninteractive
if ! command -v Rscript >/dev/null; then
    # CRAN 官方仓库优先（R 4.5.x）；密钥/仓库任一步失败自动退回发行版 R 并记录
    sudo apt-get install -y --no-install-recommends wget ca-certificates gnupg
    if wget -qO- https://cloud.r-project.org/bin/linux/ubuntu/marutter_pubkey.asc |
        sudo tee /etc/apt/trusted.gpg.d/cran_ubuntu.asc >/dev/null; then
        UB_CODENAME=$(. /etc/os-release && echo "$VERSION_CODENAME")
        echo "deb https://cloud.r-project.org/bin/linux/ubuntu ${UB_CODENAME}-cran40/" |
            sudo tee /etc/apt/sources.list.d/cran-r.list
        sudo apt-get update 2>&1 | tail -3
        echo "--- 安装 R（CRAN 源，数分钟，输出已可见）---"
        sudo apt-get install -y --no-install-recommends r-base r-base-dev && R_SRC="cran" || true
    else
        echo "CRAN 密钥获取失败，退回发行版 R"
    fi
    if ! command -v Rscript >/dev/null; then
        echo "--- 退回发行版 R ---"
        sudo apt-get install -y --no-install-recommends r-base r-base-dev && R_SRC="distro" ||
            echo "警告：R 安装失败（parity 套件将不可用，其余不受影响）"
    fi
    R_V=$(Rscript -e 'cat(as.character(getRversion()))' 2>/dev/null || echo "未装")
    echo "R 版本: $R_V（来源 ${R_SRC:-未知}；非 4.5.x 时对拍结果标注差异）"
else
    echo "R 已安装：$(Rscript -e 'cat(as.character(getRversion()))')"
fi
echo "--- 依赖（r-base-dev/gfortran/blas/lapack/pip）---"
sudo apt-get install -y --no-install-recommends r-base-dev gfortran libblas3 liblapack3 python3-pip
echo "--- swapfile（内存 3.8G 无 swap，兜底编译峰值；推荐创建）---"
if [ ! -f /data/swapfile ]; then
    read -r -p "创建 4G swapfile 于 /data/swapfile? [Y/n] " answer
    if [ "${answer:-Y}" = "Y" ]; then
        sudo fallocate -l 4G /data/swapfile && sudo chmod 600 /data/swapfile &&
            sudo mkswap /data/swapfile >/dev/null && sudo swapon /data/swapfile &&
            echo "/data/swapfile none swap sw 0 0" | sudo tee -a /etc/fstab >/dev/null &&
            echo "swap 已启用"
    fi
else
    swapon --show=NAME | grep -q /data/swapfile && echo "swap 已存在并启用" || sudo swapon /data/swapfile
fi
echo "sudo 段完成"
rm -f /tmp/osd_provision_sudo.sh
SUDO_EOF
scp -q "$SUDO_PART" "$HOST:/tmp/osd_provision_sudo.sh"
rm -f "$SUDO_PART"

echo "=== [1/2] sudo 段（远端提示输密码，交互一次）==="
ssh -t "$HOST" "bash /tmp/osd_provision_sudo.sh"

# ---------- [2/2] 用户级段：runner 落远端，tmux 后台执行，本机轮询 ----------
RUNNER_PART=$(mktemp)
cat > "$RUNNER_PART" <<RUNNER_EOF
#!/usr/bin/env bash
set -uo pipefail
VCPKG_COMMIT='$VCPKG_COMMIT'
R_LIB_DIR='$R_LIB_DIR'
BASE=/data/home-sf/opensuperdea
[ -d /data/home-sf ] || BASE=/data/opensuperdea
LOG=/home/sf/provision.log
STATUS=/home/sf/provision.status
rm -f "\$STATUS"
{
echo "--- vcpkg（构建 -j2，约 10-20 分钟）---"
if [ ! -d /data/vcpkg ]; then
    git clone https://github.com/microsoft/vcpkg /data/vcpkg
fi
git -C /data/vcpkg fetch origin >/dev/null 2>&1 || true
git -C /data/vcpkg checkout "\$VCPKG_COMMIT"
/data/vcpkg/bootstrap-vcpkg.sh -disableMetrics
echo "--- R 用户库（R_LIBS_USER=\$R_LIB_DIR）---"
mkdir -p "\$R_LIB_DIR"
export R_LIBS_USER="\$R_LIB_DIR"
Rscript -e 'install.packages(c("deaR","productivity"), repos="https://cloud.r-project.org", lib=Sys.getenv("R_LIBS_USER"), quiet=TRUE)'
# 实际加载验证（环境册 §4 注 4 教训：装了 ≠ 能加载）
Rscript -e 'library(deaR); library(productivity); cat("R 包加载验证 OK\n")' || echo "警告：R 包加载失败"
echo "--- gcovr（PEP 668）---"
pip3 install --break-system-packages --quiet gcovr || pip3 install --break-system-packages gcovr
echo "--- env.txt 硬件/版本档案（P-014）---"
mkdir -p "\$BASE"
{
    uname -a
    LC_ALL=C lscpu | grep -E 'Model name|^CPU\(s\)|MHz' || true
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
} > "\$BASE/env.txt" 2>&1
echo "provision 完成"
} >> "\$LOG" 2>&1
echo DONE > "\$STATUS"
RUNNER_EOF
scp -q "$RUNNER_PART" "$HOST:/tmp/osd_provision_user.sh"
rm -f "$RUNNER_PART"

echo "=== [2/2] 用户级段（免 root，tmux 后台 + 本机轮询）==="
ssh "$HOST" "tmux kill-session -t osd-provision 2>/dev/null; rm -f /home/sf/provision.status; tmux new-session -d -s osd-provision 'bash /tmp/osd_provision_user.sh'"
echo "provision 已在服务器 tmux 会话 osd-provision 后台运行，轮询中（约 15-30 分钟）..."
while true; do
    status=$(ssh "$HOST" 'cat /home/sf/provision.status 2>/dev/null || echo RUNNING')
    if [ "$status" = "DONE" ]; then
        echo "=== provision 完成 ==="
        ssh "$HOST" 'tail -6 /home/sf/provision.log; echo "--- env.txt ---"; cat /data/home-sf/opensuperdea/env.txt 2>/dev/null'
        break
    fi
    sleep 30
done
echo "=== provision_server.sh 全部完成 ==="
