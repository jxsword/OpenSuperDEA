#!/usr/bin/env Rscript
# 对拍参考实现：deaR 加性族（FR-NRAD-004 #1/2/3/9；score = 加权松弛和 objval）。
# 用法: Rscript deaR_additive.R <data.csv> <rts> <raw|mean|range|user>
suppressPackageStartupMessages(library(deaR))
args <- commandArgs(trailingOnly = TRUE)
df <- read.csv(args[1], stringsAsFactors = FALSE)
ni <- sum(startsWith(names(df), "in_"))
no <- sum(startsWith(names(df), "out_"))
de <- make_deadata(df, ni = ni, no = no)
X <- as.matrix(df[, startsWith(names(df), "in_"), drop = FALSE])
Y <- as.matrix(df[, startsWith(names(df), "out_"), drop = FALSE])
mode <- args[3]
if (mode == "raw") {
  wi <- 1; wo <- 1
} else if (mode == "mean") {
  wi <- 1 / colMeans(X); wo <- 1 / colMeans(Y)
} else if (mode == "range") {
  wi <- 1 / (apply(X, 2, max) - apply(X, 2, min))
  wo <- 1 / (apply(Y, 2, max) - apply(Y, 2, min))
} else {
  wi <- as.numeric(df[1, grep("^w_in_", names(df))])
  wo <- as.numeric(df[1, grep("^w_out_", names(df))])
}
res <- model_additive(de, rts = args[2], orientation = "no", weight_slack_i = wi, weight_slack_o = wo)
v <- vapply(res$DMU, function(dmu) {
  val <- dmu$objval
  if (is.null(val)) NA_real_ else val
}, numeric(1))
cat("dmu\tobjval\n")
for (i in seq_along(v)) {
  text <- if (is.na(v[i])) "na" else sprintf("%.15g", v[i])
  cat(sprintf("%s\t%s\n", names(v)[i], text))
}
