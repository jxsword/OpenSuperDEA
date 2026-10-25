#!/usr/bin/env Rscript
# 对拍参考实现：deaR 方向距离模型（FR-DDF-001~003；maxslack=FALSE 取纯 β）。
# 用法: Rscript deaR_ddf.R <data.csv> <rts: crs|vrs> <in|out|non> <own|mean|unit|custom>
#   own/mean/unit：方向由数据按与产品侧相同规则计算（own=自身，mean=数据集均值，
#   unit=全 1）；custom：方向取数据文件 di_*/do_* 列（逐 DMU 自定义方向）。
# 口径：beta 字段即产品侧 score（实测一致，无需变换）。

args <- commandArgs(trailingOnly = TRUE)
if (length(args) < 4) {
  stop("用法: deaR_ddf.R <data.csv> <rts> <in|out|non> <own|mean|unit|custom>")
}
suppressPackageStartupMessages(library(deaR))

df <- read.csv(args[1], stringsAsFactors = FALSE)
ni <- sum(startsWith(names(df), "in_"))
no <- sum(startsWith(names(df), "out_"))
de <- make_deadata(df, ni = ni, no = no)
X <- as.matrix(df[, startsWith(names(df), "in_"), drop = FALSE])
Y <- as.matrix(df[, startsWith(names(df), "out_"), drop = FALSE])
n <- nrow(df)

kind <- args[4]
if (kind == "own") {
  di <- X
  do <- Y
} else if (kind == "mean") {
  di <- matrix(rep(colMeans(X), n), nrow = n, byrow = TRUE)
  do <- matrix(rep(colMeans(Y), n), nrow = n, byrow = TRUE)
} else if (kind == "unit") {
  di <- matrix(1, n, ni)
  do <- matrix(1, n, no)
} else {
  di <- as.matrix(df[, startsWith(names(df), "di_"), drop = FALSE])
  do <- as.matrix(df[, startsWith(names(df), "do_"), drop = FALSE])
}
if (args[3] == "in") {
  do[, ] <- 0
} else if (args[3] == "out") {
  di[, ] <- 0
}
res <- model_dir(de, dir_input = t(di), dir_output = t(do), rts = args[2], maxslack = FALSE)

beta <- vapply(res$DMU, function(dmu) {
  v <- dmu$beta
  if (is.null(v)) NA_real_ else v
}, numeric(1))
cat("dmu\tbeta\n")
for (i in seq_along(beta)) {
  text <- if (is.na(beta[i])) "na" else sprintf("%.15g", beta[i])
  cat(sprintf("%s\t%s\n", names(beta)[i], text))
}
