#!/usr/bin/env Rscript
# 对拍参考实现：R productivity 相邻 Malmquist（FR-TFP-001；Färe 1994 经典实现）。
# 用法: Rscript productivity_malm.R <data.csv> <crs|vrs>
# 数据要求：dmu,period,in_*,out_* 长表（toy 面板规模）；输出 malmquist/effch/tch。
suppressPackageStartupMessages(library(productivity))
args <- commandArgs(trailingOnly = TRUE)
df <- read.csv(args[1], stringsAsFactors = FALSE)
d <- data.frame(id = df$dmu, year = as.integer(df$period))
x_cols <- grep("^in_", names(df), value = TRUE)
y_cols <- grep("^out_", names(df), value = TRUE)
for (col in x_cols) d[[col]] <- df[[col]]
for (col in y_cols) d[[col]] <- df[[col]]
res <- malm(d, id.var = "id", time.var = "year", x.vars = x_cols, y.vars = y_cols,
            rts = args[2], orientation = "out")
changes <- res$Changes
cat("dmu\tmalmquist\teffch\ttch\n")
for (i in seq_len(nrow(changes))) {
  cat(sprintf("%s\t%.15g\t%.15g\t%.15g\n", changes$id[i], changes$malmquist[i],
              changes$effch[i], changes$tch[i]))
}
