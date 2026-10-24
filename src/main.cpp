/**
 * @file main.cpp
 * @brief OpenSuperDEA 命令行入口（CLI 骨架占位）。
 * @details 当前仅输出版本信息并退出，M1 由 CLI11 框架替换为子命令解析。
 * @note GUI 骨架已移入 tmp/gui_skeleton/，M6 恢复（恢复步骤见该目录 README.md）。
 */
#include <iostream>

namespace
{
constexpr int EXIT_OK = 0;
constexpr const char *VERSION = "0.1.0";
} // namespace

int main()
{
    std::cout << "OpenSuperDEA CLI skeleton v" << VERSION << '\n';
    return EXIT_OK;
}
