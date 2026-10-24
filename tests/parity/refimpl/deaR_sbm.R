#!/usr/bin/env Rscript
# 对拍参考实现：deaR SBM 族（FR-NRAD-001/002/004；Tone 2001/2002）。
# 用法: Rscript deaR_sbm.R <data.csv> <rts: crs|vrs> <orientation: no|io|oo> [super]
# 口径：io/oo 的 efficiency 即产品侧 score（无需变换，已实测一致）；
#       super 模式用 model_sbmsupereff（效率字段 delta）。

args <- commandArgs(trailingOnly = TRUE)
if (length(args) < 3) {
  stop("用法: deaR_sbm.R <data.csv> <rts> <no|io|oo> [super]")
}
suppressPackageStartupMessages(library(deaR))

df <- read.csv(args[1], stringsAsFactors = FALSE)
ni <- sum(startsWith(names(df), "in_"))
no <- sum(startsWith(names(df), "out_"))
de <- make_deadata(df, ni = ni, no = no)

is_super <- length(args) >= 4 && args[4] == "super"
if (is_super) {
  res <- model_sbmsupereff(de, rts = args[2], orientation = args[3])
  values <- vapply(res$DMU, function(dmu) {
    v <- dmu$delta
    if (is.null(v)) NA_real_ else v
  }, numeric(1))
} else {
  res <- model_sbmeff(de, rts = args[2], orientation = args[3])
  values <- vapply(res$DMU, function(dmu) dmu$efficiency, numeric(1))
}
cat("dmu\tefficiency\n")
for (i in seq_along(values)) {
  text <- if (is.na(values[i])) "na" else sprintf("%.15g", values[i])
  cat(sprintf("%s\t%s\n", names(values)[i], text))
}
