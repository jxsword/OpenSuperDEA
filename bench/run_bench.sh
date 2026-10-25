#!/usr/bin/env bash
# run_bench.sh —— L4 性能基准 harness（FR-PERF-005，07 册 §4.4；本机为门禁基线）。
# 用法：
#   bash bench/run_bench.sh                     # 门禁矩阵（10 万 3×3 BCC，10 seeds，threads {1,2,4,8,auto}）
#   bash bench/run_bench.sh quick               # 快速参考（seed1 一组 + 1 万 CCR/BCC + 10 万 CCR）
#   bash bench/run_bench.sh matrix10k           # 1 万全规格参考矩阵
# 产出：bench/results/<时间戳>/bench_report.md（含硬件档案 P-014），可整段贴入
#       design_docs/reports/m3_test_report.md。
# 口径（对齐官方 SpeedCompare + DR-008）：每规格 10 组随机数据取均值 + (min-max)；
# M3 门禁为十万级实测报告，30.4s 达标判定移 M4（与百万级同关）。
set -euo pipefail

cd "$(dirname "$0")/.."
BINARY=build/src/opensuperdea
GEN=datasets/generated/gen_bench_data.py
OUT=bench/results/$(date +%Y%m%d_%H%M%S)
MODE=${1:-full}

[ -x "$BINARY" ] || {
    echo "错误：$BINARY 不存在，先构建（cmake --preset default && cmake --build build）" >&2
    exit 1
}

mkdir -p "$OUT"
REPORT="$OUT/bench_report.md"

# ---- 硬件与环境档案（P-014：强制记录，随报告入库）----
{
    echo "# L4 性能基准报告（FR-PERF-005）"
    echo
    echo "- 日期：$(date '+%Y-%m-%d %H:%M:%S')"
    echo "- CPU：$(LC_ALL=C lscpu | grep 'Model name' | sed 's/Model name:*//' | xargs)"
    echo "- 核数/线程：$(nproc) 逻辑核（并行度上限）"
    echo "- 内存：$(LC_ALL=C free -h | awk '/Mem:/ {print $2}')"
    echo "- 系统：$(uname -sr)（$(grep PRETTY_NAME /etc/os-release | cut -d'"' -f2)）"
    echo "- 编译器：$(gcc --version | head -1)；求解器：HiGHS（vcpkg manifest）"
    echo "- 版本：$($BINARY --version 2>/dev/null | head -1 || echo dev)"
    echo "- 口径：每规格 10 seeds 取均值 + (min-max)；官方对照 MaxDEA X 10 万 3×3 BCC = 25.32s (24.09-26.42)，i9-9900K 固频"
    echo
} > "$REPORT"

ensure_data() { # ensure_data <dmu> <io>
    local dmu=$1 io=$2
    for seed in $(seq 1 10); do
        local f="datasets/generated/bench_${dmu}_${io}_seed${seed}.csv"
        [ -f "$f" ] || python3 "$GEN" --dmu "$dmu" --io "$io" --seed "$seed" --eff-ratio 0.10 >/dev/null
    done
}

run_case() { # run_case <label> <csv> <model> <rts> <threads> → 追加一行原始数据
    local label=$1 csv=$2 model=$3 rts=$4 threads=$5
    local out_dir
    out_dir=$(mktemp -d)
    /usr/bin/time -f "%e %M" -o "$OUT/.time" \
        "$BINARY" run --model "$model" --orientation in --rts "$rts" --data "$csv" \
        --threads "$threads" --format csv -o "$out_dir" >/dev/null 2>&1
    read -r wall rss < "$OUT/.time"
    rm -rf "$out_dir"
    echo -e "${label}\t${threads}\t${wall}\t${rss}" >> "$OUT/raw.tsv"
}

summarize() { # summarize <label> <threads> → 均值 + (min-max)（单组则直报）
    local label=$1 threads=$2
    local walls=()
    while IFS=$'\t' read -r l t w _rss; do
        case "$l" in
        "$label seed"*) [ "$t" = "$threads" ] && walls+=("$w") ;;
        esac
    done < "$OUT/raw.tsv"
    [ ${#walls[@]} -eq 0 ] && return 0
    if [ ${#walls[@]} -eq 1 ]; then
        echo "| $label | $threads | ${walls[0]}（seed1 单组） |" >> "$OUT/table.md"
        return 0
    fi
    local mean min max
    mean=$(python3 -c "vs=[${walls[*]// /,}]; print(f'{sum(vs)/len(vs):.2f}')")
    min=$(python3 -c "vs=[${walls[*]// /,}]; print(f'{min(vs):.2f}')")
    max=$(python3 -c "vs=[${walls[*]// /,}]; print(f'{max(vs):.2f}')")
    echo "| $label | $threads | ${mean} (${min}-${max}) |" >> "$OUT/table.md"
}

collect_threads() { # collect_threads <label> <model> <rts> <dmu> <io>
    local label=$1 model=$2 rts=$3 dmu=$4 io=$5
    ensure_data "$dmu" "$io"
    for threads in 1 2 4 8 auto; do
        # threads=1 仅 seed1：十万级串行单 case 30-60 分钟，10 seeds 全跑不可行；
        # 串行参考点取一个，细粒度加速比见 2000 DMU 序列与探针数据（m3 报告 §3）
        if [ "$threads" = "1" ]; then
            run_case "$label seed1" "datasets/generated/bench_${dmu}_${io}_seed1.csv" \
                "$model" "$rts" "$threads"
        else
            for seed in $(seq 1 10); do
                run_case "$label seed$seed" "datasets/generated/bench_${dmu}_${io}_seed${seed}.csv" \
                    "$model" "$rts" "$threads"
            done
        fi
    done
    for threads in 1 2 4 8 auto; do
        summarize "$label" "$threads"
    done
}

case "$MODE" in
slim) # 核心档（M3 报告主用，~1h）：1 万全矩阵；10 万 BCC threads=8 实测（5h08m）由
    # 2026-10-26 全量长跑提供（design_docs/reports/m3_test_report.md §3），不重跑
    echo "## 核心档（M3 实测，DR-008；10 万 BCC 单 case 引用长跑实测 5h08m）" > "$OUT/table.md"
    echo "| 规格 | threads | 耗时（秒） |" >> "$OUT/table.md"
    echo "|---|---|---|" >> "$OUT/table.md"
    ensure_data 10000 3x3
    for threads in 1 2 4 8 auto; do
        run_case "1万 3×3 BCC seed1" datasets/generated/bench_10000_3x3_seed1.csv bcc vrs "$threads"
    done
    awk -F'\t' '{printf "| %s | %s | %s |\n", $1, $2, $3}' "$OUT/raw.tsv" >> "$OUT/table.md"
    ;;
full) # 夜间档（官方 10-seeds 口径矩阵，~10h；M4 达标后主用）
    echo "## 门禁矩阵：10 万 DMU 3×3 BCC（10 seeds × threads 矩阵）" > "$OUT/table.md"
    echo "| 规格 | threads | 耗时均值 (min-max) 秒 |" >> "$OUT/table.md"
    echo "|---|---|---|" >> "$OUT/table.md"
    echo "采集 10 万 3×3 BCC × 10 seeds × 5 threads（每 case 独立计时，总时长较长）..."
    collect_threads "10万 3×3 BCC" bcc vrs 100000 3x3
    ;;
quick) # 快速参考：seed1 一组
    echo "## 快速参考（seed1 单组）" > "$OUT/table.md"
    echo "| 规格 | threads | 耗时 秒 |" >> "$OUT/table.md"
    echo "|---|---|---|" >> "$OUT/table.md"
    ensure_data 10000 3x3; ensure_data 100000 3x3
    for threads in 1 8; do
        run_case "1万 3×3 CCR seed1" datasets/generated/bench_10000_3x3_seed1.csv ccr crs "$threads"
        run_case "1万 3×3 BCC seed1" datasets/generated/bench_10000_3x3_seed1.csv bcc vrs "$threads"
        run_case "10万 3×3 CCR seed1" datasets/generated/bench_100000_3x3_seed1.csv ccr crs "$threads"
        run_case "10万 3×3 BCC seed1" datasets/generated/bench_100000_3x3_seed1.csv bcc vrs "$threads"
    done
    awk -F'\t' '{printf "| %s | %s | %s |\n", $1, $2, $3}' "$OUT/raw.tsv" >> "$OUT/table.md"
    ;;
*)
    echo "用法：run_bench.sh [slim|full|quick]" >&2
    exit 1
    ;;
esac

cat "$OUT/table.md" >> "$REPORT"
echo "原始数据：$OUT/raw.tsv（列：规格 seed / threads / wall 秒 / RSS KB）" >> "$REPORT"
echo
echo "=== 报告已生成：$REPORT ==="
cat "$REPORT"
