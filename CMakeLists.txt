# OpenSuperDEA 顶层构建脚本
# 命名规范见仓库根 naming_convention.md §四；
# 项目级变量/选项采用 OPENSUPERDEA_ 前缀（§四 的 CPP_STANDARD_ 为其示例项目前缀，按意图适配）。
cmake_minimum_required(VERSION 3.16)

project(OpenSuperDEA VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(OPENSUPERDEA_ENABLE_WARNINGS ON
    CACHE BOOL "启用编译警告（-Wall -Wextra）")

if(OPENSUPERDEA_ENABLE_WARNINGS)
    add_compile_options(-Wall -Wextra)
endif()

# GUI 骨架已移入 tmp/gui_skeleton/（CLI 先行规划，2026-10-24）。
# M6 恢复 GUI 时引入以下配置：
#   find_package(Qt6 REQUIRED COMPONENTS Core Widgets)
#   set(CMAKE_AUTOMOC ON)
#   set(CMAKE_AUTOUIC ON)
#   set(CMAKE_AUTORCC ON)

add_subdirectory(src)
