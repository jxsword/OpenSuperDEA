#!/usr/bin/env Rscript
# 对拍参考实现：deaR 乘子（对偶）模型（FR-MULT-001；PR-06 对偶一致性的独立实现佐证）。
# 用法: Rscript deaR_multiplier.R <data.csv> <rts: crs|vrs> <orientation: io|oo>
# 口径：io efficiency 即产品侧 score；oo 为原始 phi（manifest 用 reciprocal 映射）。

args <- commandArgs(trailingOnly = TRUE)
if (length(args) != 3) {
  stop("用法: deaR_multiplier.R <data.csv> <rts> <io|oo>")
}
suppressPackageStartupMessages(library(deaR))

df <- read.csv(args[1], stringsAsFactors = FALSE)
ni <- sum(startsWith(names(df), "in_"))
no <- sum(startsWith(names(df), "out_"))
de <- make_deadata(df, ni = ni, no = no)
res <- model_multiplier(de, rts = args[2], orientation = args[3])

eff <- vapply(res$DMU, function(dmu) dmu$efficiency, numeric(1))
cat("dmu\tefficiency\n")
for (i in seq_along(eff)) {
  value <- eff[i]
  text <- if (is.na(value)) "na" else sprintf("%.15g", value)
  cat(sprintf("%s\t%s\n", names(eff)[i], text))
}
