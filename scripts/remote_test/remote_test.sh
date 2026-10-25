#!/usr/bin/env bash
# remote_test.sh —— juzdata 云服务器远程测试单入口（DR-009）。
# 用法：
#   scripts/remote_test/remote_test.sh sync              # 同步源码到服务器
#   scripts/remote_test/remote_test.sh run ctest parity  # 异步执行套件（tmux + 轮询 + 自动 fetch）
#   scripts/remote_test/remote_test.sh run all           # ctest + parity
#   scripts/remote_test/remote_test.sh status [run-id]   # 查看运行状态
#   scripts/remote_test/remote_test.sh fetch [run-id]    # 回传结果
# 套件：ctest | parity | coverage | bench | smoke | all(=ctest+parity)
set -euo pipefail

HOST=juzdata
# 基目录固定 /data/home-sf/opensuperdea（sf 属主，可写）；provision 的 sudo 段
# 仅在其不可用时才提示 root 建 /data/opensuperdea
REMOTE_ROOT=/data/home-sf/opensuperdea
REMOTE_RUNS="$(dirname "$REMOTE_ROOT")/opensuperdea-remote"
LOCAL_RESULTS=build-remote-results

detect_run_id() {
    local run_id=${2:-}
    if [ -z "$run_id" ]; then
        run_id=$(ssh "$HOST" "cat $REMOTE_RUNS/latest 2>/dev/null") || {
            echo "错误：未找到运行记录（先 run）" >&2
            exit 1
        }
    fi
    echo "$run_id"
}

cmd=${1:-}
[ -n "$cmd" ] || {
    sed -n '2,9p' "$0"
    exit 1
}
shift

case "$cmd" in
sync)
    echo "=== 同步源码 → $HOST:$REMOTE_ROOT/src ==="
    ssh "$HOST" "mkdir -p $REMOTE_ROOT/src"
    rsync -az --delete \
        --exclude 'build*/' --exclude 'tmp/' --exclude '.git/' \
        --exclude 'vcpkg_installed/' --exclude 'datasets/generated/*.csv' \
        --exclude 'build-remote-results/' \
        ./ "$HOST:$REMOTE_ROOT/src/"
    echo "同步完成"
    ;;
run)
    [ $# -ge 1 ] || {
        echo "用法：run <suite>...（ctest|parity|coverage|bench|smoke|all）" >&2
        exit 1
    }
    suites=("$@")
    if [ "${suites[0]}" = "all" ]; then
        suites=(ctest parity)
    fi
    run_id="$(date +%Y%m%d_%H%M%S)"

    echo "=== 内存预检 ==="
    ssh "$HOST" bash -s <<PRECHECK_EOF
available=\$(awk '/MemAvailable/ {print int(\$2/1024)}' /proc/meminfo)
echo "服务器可用内存: \${available}M"
if [ "\$available" -lt 2500 ]; then
    echo "警告：可用内存 <2.5G，构建/ctest 可能 OOM（swap 兜底）"
fi
exit 0
PRECHECK_EOF

    echo "=== 同步源码 ==="
    "$0" sync

    echo "=== 生成服务器端 runner（suites: ${suites[*]}）==="
    ssh "$HOST" "mkdir -p $REMOTE_RUNS/$run_id/results $REMOTE_ROOT"
    {
        echo "set -uo pipefail"
        echo "cd $REMOTE_ROOT/src"
        echo "LOG=$REMOTE_RUNS/$run_id/progress.log"
        echo "RESULTS=$REMOTE_RUNS/$run_id/results"
        echo "echo '=== 远程测试 run_id=$run_id suites: ${suites[*]} ===' | tee -a \$LOG"
        echo "overall=0"
        for suite in "${suites[@]}"; do
            case "$suite" in
            ctest)
                cat <<'S_CTEST'
echo "--- [ctest] Release 构建 + ctest -j2 ---" | tee -a $LOG
export VCPKG_FORCE_SYSTEM_BINARIES=1
/data/home-sf/vcpkg/vcpkg install --x-manifest-root=. --x-install-root=$REMOTE_ROOT/vcpkg_installed --triplet x64-linux >> $LOG 2>&1 || { echo overall=1 > $REMOTE_RUNS/LATEST_EXIT; exit 1; }
cmake -B $REMOTE_ROOT/build -S . -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=/data/home-sf/vcpkg/scripts/buildsystems/vcpkg.cmake -G Ninja >> $LOG 2>&1
cmake --build $REMOTE_ROOT/build -j2 >> $LOG 2>&1
ctest --test-dir $REMOTE_ROOT/build -j2 --output-on-failure | tee -a $LOG
S_CTEST
                ;;
            parity)
                cat <<'S_PARITY'
echo "--- [parity] 68 例对拍 ---" | tee -a $LOG
Rscript -e 'library(deaR); library(productivity)' >/dev/null 2>&1 || { echo "R 包不可用，parity 跳过" | tee -a $LOG; exit 1; }
python3 tests/parity/parity_runner.py --binary $REMOTE_ROOT/build/src/opensuperdea | tee -a $LOG
S_PARITY
                ;;
            coverage)
                cat <<'S_COV'
echo "--- [coverage] build-coverage + gcovr ---" | tee -a $LOG
cmake -B $REMOTE_ROOT/build-coverage -S . -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_FLAGS="--coverage" -DCMAKE_EXE_LINKER_FLAGS="--coverage" -DCMAKE_TOOLCHAIN_FILE=/data/home-sf/vcpkg/scripts/buildsystems/vcpkg.cmake -G Ninja >> $LOG 2>&1
cmake --build $REMOTE_ROOT/build-coverage -j2 >> $LOG 2>&1
ctest --test-dir $REMOTE_ROOT/build-coverage >> $LOG 2>&1
GCOV=$(command -v gcov-13 || command -v gcov)
python3 -m gcovr --root . --gcov-executable "$GCOV" --print-summary | tee -a $LOG
S_COV
                ;;
            bench)
                cat <<'S_BENCH'
echo "--- [bench] 占位（M3 harness 落地后接入；threads {1,2,auto}）---" | tee -a $LOG
S_BENCH
                ;;
            smoke)
                cat <<'S_SMOKE'
echo "--- [smoke] 链路桩测试 ---" | tee -a $LOG
echo "smoke step1: sync 链路 OK" | tee -a $LOG
sleep 5
echo "smoke step2: tmux/轮询 OK" | tee -a $LOG
cp $LOG "$RESULTS/smoke_progress_copy.log"
S_SMOKE
                ;;
            *)
                echo "未知套件: $suite" >&2
                exit 1
                ;;
            esac
            echo "[ \"\$?\" -ne 0 ] && overall=1"
        done
        echo "echo \"overall_exit=\$overall\" > $REMOTE_RUNS/$run_id/results/summary.txt"
        echo "echo DONE > $REMOTE_RUNS/$run_id/status"
    } | ssh "$HOST" "cat > $REMOTE_RUNS/$run_id/runner.sh"
    ssh "$HOST" "echo $run_id > $REMOTE_RUNS/latest; chmod +x $REMOTE_RUNS/$run_id/runner.sh"

    echo "=== 启动 tmux 会话 osd-$run_id ==="
    ssh "$HOST" "tmux kill-session -t osd-$run_id 2>/dev/null; tmux new-session -d -s osd-$run_id 'bash $REMOTE_RUNS/$run_id/runner.sh; exit'"
    echo "run_id=$run_id（Ctrl-C 仅退出轮询，远端继续；恢复：remote_test.sh status|fetch $run_id）"

    echo "=== 轮询（每 30s）==="
    while true; do
        status=$(ssh "$HOST" "cat $REMOTE_RUNS/$run_id/status 2>/dev/null || echo RUNNING")
        if [ "$status" = "DONE" ]; then
            echo "=== 远端 DONE，回传结果 ==="
            "$0" fetch "$run_id"
            break
        fi
        ssh "$HOST" "tail -2 $REMOTE_RUNS/$run_id/progress.log 2>/dev/null" | sed 's/^/  | /'
        sleep 30
    done
    ;;
status)
    run_id=$(detect_run_id "$@")
    echo "=== run_id=$run_id 状态 ==="
    ssh "$HOST" "echo status: \$(cat $REMOTE_RUNS/$run_id/status 2>/dev/null || echo RUNNING); tail -8 $REMOTE_RUNS/$run_id/progress.log 2>/dev/null; tmux ls 2>/dev/null | grep osd || true"
    ;;
fetch)
    run_id=$(detect_run_id "$@")
    echo "=== 回传 run_id=$run_id → $LOCAL_RESULTS/$run_id/ ==="
    mkdir -p "$LOCAL_RESULTS/$run_id"
    rsync -az "$HOST:$REMOTE_RUNS/$run_id/" "$LOCAL_RESULTS/$run_id/"
    ssh "$HOST" "cp $REMOTE_ROOT/src/tests/parity/*.log $REMOTE_RUNS/$run_id/results/ 2>/dev/null" || true
    rsync -az "$HOST:$REMOTE_RUNS/$run_id/results/" "$LOCAL_RESULTS/$run_id/results/" 2>/dev/null || true
    cat "$LOCAL_RESULTS/$run_id/results/summary.txt" 2>/dev/null || echo "（无 summary，运行未结束）"
    echo "回传完成"
    ;;
*)
    echo "未知子命令: $cmd（sync|run|status|fetch）" >&2
    exit 1
    ;;
esac
