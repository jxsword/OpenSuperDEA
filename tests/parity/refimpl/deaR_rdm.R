#!/usr/bin/env Rscript
# 对拍参考实现：deaR RDM（FR-DDF-002；Silva Portela 范围方向测度）。
# 口径：deaR model_rdm 内部固定 VRS + maxslack 语义；用 maxslack=FALSE 取纯 β。
suppressPackageStartupMessages(library(deaR))
args <- commandArgs(trailingOnly = TRUE)
df <- read.csv(args[1], stringsAsFactors = FALSE)
ni <- sum(startsWith(names(df), "in_"))
no <- sum(startsWith(names(df), "out_"))
de <- make_deadata(df, ni = ni, no = no)
res <- model_rdm(de, maxslack = FALSE)
beta <- vapply(res$DMU, function(dmu) {
  v <- dmu$beta
  if (is.null(v)) NA_real_ else v
}, numeric(1))
cat("dmu\tbeta\n")
for (i in seq_along(beta)) {
  text <- if (is.na(beta[i])) "na" else sprintf("%.15g", beta[i])
  cat(sprintf("%s\t%s\n", names(beta)[i], text))
}
