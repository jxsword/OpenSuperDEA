#!/usr/bin/env Rscript
# 对拍参考实现：deaR 基础径向模型（FR-TST-001/002；许可边界 C-02：只对拍不移植）。
# 用法: Rscript deaR_radial.R <data.csv> <rts: crs|vrs|nirs|ndrs> <orientation: io|oo>
# 输出: TSV（表头 dmu\tefficiency），供 parity_runner.py 双阈值比对（P-010）。
# 版本锁定见 refimpl/versions.lock；deaR 升级必须全量回归（04 册 §2）。

args <- commandArgs(trailingOnly = TRUE)
if (length(args) != 3) {
  stop("用法: deaR_radial.R <data.csv> <rts> <io|oo>")
}
suppressPackageStartupMessages(library(deaR))

df <- read.csv(args[1], stringsAsFactors = FALSE)
ni <- sum(startsWith(names(df), "in_"))
no <- sum(startsWith(names(df), "out_"))
if (ni == 0 || no == 0) {
  stop("数据缺少 in_/out_ 前缀列（02 册 schema）")
}
de <- make_deadata(df, ni = ni, no = no)
res <- model_basic(de, rts = args[2], orientation = args[3])

eff <- vapply(res$DMU, function(dmu) dmu$efficiency, numeric(1))
cat("dmu\tefficiency\n")
for (i in seq_along(eff)) {
  cat(sprintf("%s\t%.15g\n", names(eff)[i], eff[i]))
}
