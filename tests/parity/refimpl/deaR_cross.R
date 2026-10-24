#!/usr/bin/env Rscript
# 对拍参考实现：deaR 交叉效率（FR-CROSS-001/002；M2=Doyle-Green，M3=Ruiz 类修正）。
# 用法: Rscript deaR_cross.R <data.csv> <rts> <dg_i|dg_ii|m3_i|m3_ii>
# 口径：e = 列均值（剔除自评，selfapp=FALSE），即产品侧 score。
suppressPackageStartupMessages(library(deaR))
args <- commandArgs(trailingOnly = TRUE)
df <- read.csv(args[1], stringsAsFactors = FALSE)
ni <- sum(startsWith(names(df), "in_"))
no <- sum(startsWith(names(df), "out_"))
de <- make_deadata(df, ni = ni, no = no)
m2 <- args[3] %in% c("dg_i", "dg_ii")
m3 <- args[3] %in% c("m3_i", "m3_ii")
res <- cross_efficiency(de, rts = args[2], orientation = "io", selfapp = FALSE, M2 = m2, M3 = m3)
obj <- if (m2) res$M2_agg else res$M3_agg
if (args[3] %in% c("dg_ii", "m3_ii")) {
  obj <- if (m2) res$M2_ben else res$M3_ben
}
e <- obj$e
cat("dmu\te\n")
for (i in seq_along(e)) {
  text <- if (is.na(e[i])) "na" else sprintf("%.15g", e[i])
  cat(sprintf("%s\t%s\n", names(e)[i], text))
}
