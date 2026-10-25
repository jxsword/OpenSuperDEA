#!/usr/bin/env python3
"""DEA 基准算例生成器（datasets/generated/）。

生成机制（design_docs/07 册 §4.2，用户裁决 V1.1，对齐官方 SpeedCompare 口径）：
  1. 投入对数正态：x_ij ~ LogNormal(mu, sigma)；
  2. 产出经真实前沿生成：y_k = A_k * prod_i x_i^beta_ki（Cobb-Douglas，CRS 真实前沿）；
  3. 技术无效项 u ~ Exp(lambda)（右偏分布），按强有效目标比例 eff_ratio 控制的单元 u = 0
     （落于前沿，即真实效率 = 1；实测 CCR 有效比例因包络凸包效应略高，校准记录见
     datasets/README.md）；
  4. 固定 seed 完全可复现；每规格 10 组不同 seed，基准报告取均值 + min-max 区间
     （官方 SpeedCompare 同口径：随机数据 × 10 组统计）。

CSV 输出遵循 02 册 §2 schema：首列 dmu，列头 in_1..in_m / out_1..out_s。

用法示例：
  python3 gen_bench_data.py --dmu 100000 --io 3x3 --seed 1 --eff-ratio 0.10
  python3 gen_bench_data.py --all                # 官方全规格矩阵（1万/10万/100万 × 3x3/5x5 × 10 seeds）
  python3 gen_bench_data.py --dmu 2000 --io 3x3 --seed 99 --eff-ratio 0.10 --out /tmp
"""

import argparse
import csv
import math
import os
import random

# 官方 SpeedCompare 规格矩阵：DMU 数 × 投入产出规格
BENCH_SPECS = [
    (10_000, 3, 3),
    (10_000, 5, 5),
    (100_000, 3, 3),
    (100_000, 5, 5),
    (1_000_000, 3, 3),
    (1_000_000, 5, 5),
]
BENCH_SEEDS = list(range(1, 11))  # 每规格 10 组（官方同口径）

# 分布参数（对数刻度）：投入跨约 e^{±1.5} ≈ 0.22~4.5 的量级，正值、右偏，符合 DEA 模拟惯例
LOG_MEAN = 0.0
LOG_SIGMA = 0.5
INEFF_SCALE = 0.35  # 无效项 Exp 尺度：E[u] = 0.35，真实效率均值约 exp(-0.35) ≈ 0.70


def generate_dataset(dmu_count, input_count, output_count, seed, eff_ratio):
    """生成一组 DEA 算例，返回 (header, rows)。"""
    rng = random.Random(seed)

    # Cobb-Douglas 前沿弹性：每个产出对投入的指数取均匀权重后归一（CRS 真实前沿）
    elasticities = []
    for _ in range(output_count):
        raw = [rng.uniform(0.8, 1.2) for _ in range(input_count)]
        total = sum(raw)
        elasticities.append([w / total for w in raw])
    scales = [rng.uniform(0.9, 1.1) for _ in range(output_count)]

    n_eff = round(dmu_count * eff_ratio)
    is_efficient = [i < n_eff for i in range(dmu_count)]
    rng.shuffle(is_efficient)  # 有效单元位置随机分布

    header = ["dmu"] + [f"in_{j + 1}" for j in range(input_count)] + \
             [f"out_{k + 1}" for k in range(output_count)]
    rows = []
    for i in range(dmu_count):
        inputs = [math.exp(LOG_MEAN + LOG_SIGMA * rng.gauss(0.0, 1.0))
                  for _ in range(input_count)]
        u = 0.0 if is_efficient[i] else rng.expovariate(1.0 / INEFF_SCALE)
        outputs = [scales[k] * math.exp(-u) *
                   math.prod(x ** beta for x, beta in zip(inputs, elasticities[k]))
                   for k in range(output_count)]
        rows.append([f"DMU{i + 1:07d}"] +
                    [f"{v:.10g}" for v in inputs] + [f"{v:.10g}" for v in outputs])
    return header, rows


def write_csv(path, header, rows):
    with open(path, "w", newline="", encoding="utf-8") as handle:
        writer = csv.writer(handle)
        writer.writerow(header)
        writer.writerows(rows)


def build_one(dmu_count, input_count, output_count, seed, eff_ratio, out_dir):
    name = f"bench_{dmu_count}_{input_count}x{output_count}_seed{seed}.csv"
    path = os.path.join(out_dir, name)
    header, rows = generate_dataset(dmu_count, input_count, output_count, seed, eff_ratio)
    write_csv(path, header, rows)
    return path


def main():
    parser = argparse.ArgumentParser(description="DEA 基准算例生成器（07 册 §4.2）")
    parser.add_argument("--dmu", type=int, help="DMU 数量")
    parser.add_argument("--io", choices=["3x3", "5x5"], help="投入×产出规格")
    parser.add_argument("--seed", type=int, default=1, help="随机种子（可复现）")
    parser.add_argument("--eff-ratio", type=float, default=0.10,
                        help="强有效单元目标比例（默认 0.10）")
    parser.add_argument("--out", default=os.path.dirname(os.path.abspath(__file__)),
                        help="输出目录（默认生成器所在目录）")
    parser.add_argument("--all", action="store_true",
                        help="生成官方全规格矩阵（6 规格 × 10 seeds，量级较大，注意磁盘）")
    args = parser.parse_args()

    if args.all:
        count = 0
        for dmu_count, input_count, output_count in BENCH_SPECS:
            for seed in BENCH_SEEDS:
                path = build_one(dmu_count, input_count, output_count, seed,
                                 args.eff_ratio, args.out)
                count += 1
                print(f"[{count}/60] {path}")
        return

    if not args.dmu or not args.io:
        parser.error("需要 --dmu 与 --io，或使用 --all")
    input_count, output_count = (int(v) for v in args.io.split("x"))
    path = build_one(args.dmu, input_count, output_count, args.seed, args.eff_ratio, args.out)
    print(path)


if __name__ == "__main__":
    main()
