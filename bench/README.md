# bench —— L4 性能基准 harness（FR-PERF-005，07 册 §4.4）

本机为**门禁基线**（P-002/P-014）；云服务器（juzdata）bench 仅作观察/回归，计时不可与本机比对（见 `scripts/remote_test/README.md`）。

## 用法

```bash
bash bench/run_bench.sh          # 门禁矩阵：10 万 3×3 BCC × 10 seeds × threads {1,2,4,8,auto}
bash bench/run_bench.sh quick    # 快速参考：seed1 单组（1 万 CCR/BCC + 10 万 CCR/BCC × threads 1/8）
```

- 算例来自 `datasets/generated/`（缺失时自动按固定 seed 生成，见 `datasets/README.md` §1）；
- 产出 `bench/results/<时间戳>/bench_report.md`（含硬件档案 P-014 + 「均值 (min-max)」表），
  整段贴入 `design_docs/reports/m3_test_report.md`；
- 口径对齐官方 SpeedCompare：每规格 10 组随机数据取均值并附区间。

## 门禁口径（DR-008）

| 项 | 说明 |
|---|---|
| M3 门禁 | 十万级全量完成，实测数据入报告（分钟级）；threads 加速矩阵 |
| 30.4s 达标判定 | 移至 M4 门禁（与百万级 NFR-PERF-004 同关，SRS V1.2） |
| 硬件记录 | 强制（P-014）：报告头部自动采集 |

## 结果目录

`bench/results/`（`.gitignore` 覆盖，不入库；报告正文贴入 design_docs/reports/ 才入库）。
