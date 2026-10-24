#!/usr/bin/env Rscript
# 对拍参考实现：deaR 成本/收益效率（FR-COST-001/002；restricted_optimal=FALSE 标准形式）。
# 用法: Rscript deaR_price.R <data.csv> <rts> <cost|revenue>
suppressPackageStartupMessages(library(deaR))
args <- commandArgs(trailingOnly = TRUE)
df <- read.csv(args[1], stringsAsFactors = FALSE)
ni <- sum(startsWith(names(df), "in_"))
no <- sum(startsWith(names(df), "out_"))
de <- make_deadata(df, ni = ni, no = no)
pi_cols <- names(df)[startsWith(names(df), "pi_")]
po_cols <- names(df)[startsWith(names(df), "po_")]
if (args[3] == "cost") {
  w <- as.numeric(df[1, pi_cols])
  res <- model_profit(de, price_input = w, restricted_optimal = FALSE, rts = args[2])
} else {
  p <- as.numeric(df[1, po_cols])
  res <- model_profit(de, price_output = p, restricted_optimal = FALSE, rts = args[2])
}
eff <- vapply(res$DMU, function(dmu) {
  v <- dmu$efficiency
  if (is.null(v)) NA_real_ else v
}, numeric(1))
cat("dmu\tefficiency\n")
for (i in seq_along(eff)) {
  text <- if (is.na(eff[i])) "na" else sprintf("%.15g", eff[i])
  cat(sprintf("%s\t%s\n", names(eff)[i], text))
}
