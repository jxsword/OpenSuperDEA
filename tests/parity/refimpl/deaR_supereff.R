#!/usr/bin/env Rscript
# 对拍参考实现：deaR 超效率模型（FR-RAD-005；Anderson-Petersen 口径）。
# 用法: Rscript deaR_supereff.R <data.csv> <rts: crs|vrs|nirs|ndrs> <orientation: io|oo>
# 口径说明（04 册 §2 显式标注）：
#   io 输出 efficiency 即产品侧 score（无效 DMU 保持原值，有效 DMU >1，不可行 NA）；
#   oo 输出原始 phi，产品侧为归一 1/phi（manifest 用 reciprocal 映射）。

args <- commandArgs(trailingOnly = TRUE)
if (length(args) != 3) {
  stop("用法: deaR_supereff.R <data.csv> <rts> <io|oo>")
}
suppressPackageStartupMessages(library(deaR))

df <- read.csv(args[1], stringsAsFactors = FALSE)
ni <- sum(startsWith(names(df), "in_"))
no <- sum(startsWith(names(df), "out_"))
de <- make_deadata(df, ni = ni, no = no)
res <- model_supereff(de, rts = args[2], orientation = args[3])

eff <- vapply(res$DMU, function(dmu) dmu$efficiency, numeric(1))
cat("dmu\tefficiency\n")
for (i in seq_along(eff)) {
  value <- eff[i]
  text <- if (is.na(value)) "na" else sprintf("%.15g", value)
  cat(sprintf("%s\t%s\n", names(eff)[i], text))
}
