#!/usr/bin/env Rscript
# 对拍参考实现：deaR FDH（FR-FDH-001；自由处置支配 MILP）。
# 用法: Rscript deaR_fdh.R <data.csv> <io|oo>
suppressPackageStartupMessages(library(deaR))
args <- commandArgs(trailingOnly = TRUE)
df <- read.csv(args[1], stringsAsFactors = FALSE)
ni <- sum(startsWith(names(df), "in_"))
no <- sum(startsWith(names(df), "out_"))
de <- make_deadata(df, ni = ni, no = no)
res <- model_fdh(de, orientation = args[2])
eff <- vapply(res$DMU, function(dmu) {
  v <- dmu$efficiency
  if (is.null(v)) NA_real_ else v
}, numeric(1))
cat("dmu\tefficiency\n")
for (i in seq_along(eff)) {
  text <- if (is.na(eff[i])) "na" else sprintf("%.15g", eff[i])
  cat(sprintf("%s\t%s\n", names(eff)[i], text))
}
