#!/usr/bin/env Rscript
# 对拍参考实现：deaR 加权 SBM（FR-NRAD-004 #6-8；权重取 w_in_/w_out_ 列）。
suppressPackageStartupMessages(library(deaR))
args <- commandArgs(trailingOnly = TRUE)
df <- read.csv(args[1], stringsAsFactors = FALSE)
ni <- sum(startsWith(names(df), "in_"))
no <- sum(startsWith(names(df), "out_"))
de <- make_deadata(df, ni = ni, no = no)
wi <- as.numeric(df[1, grep("^w_in_", names(df))])
wo <- as.numeric(df[1, grep("^w_out_", names(df))])
res <- model_sbmeff(de, rts = args[2], orientation = args[3], weight_input = wi, weight_output = wo)
eff <- vapply(res$DMU, function(dmu) {
  v <- dmu$efficiency
  if (is.null(v)) NA_real_ else v
}, numeric(1))
cat("dmu\tefficiency\n")
for (i in seq_along(eff)) {
  text <- if (is.na(eff[i])) "na" else sprintf("%.15g", eff[i])
  cat(sprintf("%s\t%s\n", names(eff)[i], text))
}
