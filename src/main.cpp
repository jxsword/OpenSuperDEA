/**
 * @file main.cpp
 * @brief OpenSuperDEA 命令行入口：仅装配 run_cli（业务在 cli/ 与 core/）。
 * @note GUI 骨架已移入 tmp/gui_skeleton/，M6 恢复（恢复步骤见该目录 README.md）。
 */
#include "cli/cli_app.h"
#include "cli/exit_codes.h"

#include <iostream>

int main(int argc, char **argv)
{
    return opensuperdea::cli::run_cli(argc, argv, std::cout, std::cerr);
}
