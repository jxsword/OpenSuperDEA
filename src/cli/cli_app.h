/**
 * @file cli_app.h
 * @brief CLI11 命令行装配：version / check / run 骨架（SRS FR-CLI-001~003/006/007）。
 * @details P-004 裁决：M1 交付 version/check/run 骨架；run 完成参数解析与数据
 *          链路，模型执行走注册表占位（M2 全量生效）。测试经 run_cli 直接调用，
 *          以返回退出码断言（不经子进程）。
 */
#pragma once

#include <iosfwd>

namespace opensuperdea::cli
{
/**
 * @brief CLI 主入口（main 装配点）。
 * @param argc/argv 命令行参数
 * @param out stdout 流（摘要/结果输出）
 * @param err stderr 流（错误输出，格式 [E<code>] <module>: <message> (hint: …)）
 * @return 退出码（ExitCode）
 */
int run_cli(int argc, char **argv, std::ostream &out, std::ostream &err);
} // namespace opensuperdea::cli
