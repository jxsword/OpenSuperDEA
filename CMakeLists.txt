# OpenSuperDEA 顶层构建脚本
# 命名规范见仓库根 naming_convention.md §四；
# 项目级变量/选项采用 OPENSUPERDEA_ 前缀（§四 的 CPP_STANDARD_ 为其示例项目前缀，按意图适配）。
cmake_minimum_required(VERSION 3.16)

project(OpenSuperDEA VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(OPENSUPERDEA_ENABLE_WARNINGS ON
    CACHE BOOL "启用编译警告（-Wall -Wextra）")
option(OPENSUPERDEA_BUILD_TESTS "构建单元测试（GoogleTest + ctest）" ON)
option(OPENSUPERDEA_ENABLE_COVERAGE
       "启用覆盖率统计（--coverage，供 llvm-cov/gcovr，SRS P-009）" OFF)

if(OPENSUPERDEA_ENABLE_WARNINGS)
    if(MSVC)
        # /W4 对齐 GCC -Wall -Wextra 强度；/utf-8 保证中文注释/字符串按 UTF-8 解析
        add_compile_options(/W4 /utf-8)
    else()
        add_compile_options(-Wall -Wextra)
    endif()
endif()

if(OPENSUPERDEA_ENABLE_COVERAGE)
    add_compile_options(--coverage)
    add_link_options(--coverage)
endif()

# ---- 第三方依赖（vcpkg manifest 模式；许可边界见 SRS NFR-SEC-001/002）----
# Eigen 为引擎公共数据底座，随 core 头文件暴露 → PUBLIC 链接在 src/CMakeLists 中声明
find_package(Eigen3 CONFIG REQUIRED)
find_package(HIGHS CONFIG REQUIRED)
find_package(CLI11 CONFIG REQUIRED)

# GUI 骨架已移入 tmp/gui_skeleton/（CLI 先行规划，2026-10-24）。
# M6 恢复 GUI 时引入以下配置：
#   find_package(Qt6 REQUIRED COMPONENTS Core Widgets)
#   set(CMAKE_AUTOMOC ON)
#   set(CMAKE_AUTOUIC ON)
#   set(CMAKE_AUTORCC ON)

add_subdirectory(src)

if(OPENSUPERDEA_BUILD_TESTS)
    find_package(GTest CONFIG REQUIRED)
    enable_testing()
    add_subdirectory(tests)
endif()
