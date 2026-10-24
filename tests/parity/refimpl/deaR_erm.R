#!/usr/bin/env Rscript
# 对拍参考实现：deaR ERM/Russell（FR-NRAD-003；io 取 mean_efficiency，oo 取 1/efficiency）。
# 用法: Rscript deaR_erm.R <data.csv> <rts> <io|oo>
suppressPackageStartupMessages(library(deaR))
args <- commandArgs(trailingOnly = TRUE)
df <- read.csv(args[1], stringsAsFactors = FALSE)
ni <- sum(startsWith(names(df), "in_"))
no <- sum(startsWith(names(df), "out_"))
de <- make_deadata(df, ni = ni, no = no)
res <- model_nonradial(de, rts = args[2], orientation = args[3])
values <- vapply(res$DMU, function(dmu) {
  v <- if (args[3] == "io") dmu$mean_efficiency else dmu$efficiency
  if (is.null(v)) NA_real_ else if (length(v) > 1) mean(v) else v
}, numeric(1))
cat("dmu\tefficiency\n")
for (i in seq_along(values)) {
  text <- if (is.na(values[i])) "na" else sprintf("%.15g", values[i])
  cat(sprintf("%s\t%s\n", names(values)[i], text))
}
