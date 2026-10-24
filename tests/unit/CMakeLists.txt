# 单元测试可执行（按里程碑分目录组织用例源文件，ctest 标签 unit;<milestone>）
add_executable(opensuperdea_unit_tests
    m1/highs_adapter_test.cpp
    m1/csv_parser_test.cpp
    m1/dataset_builder_test.cpp
    m1/dataset_test.cpp
    m1/validator_test.cpp
    m1/cli_test.cpp
)

target_link_libraries(opensuperdea_unit_tests PRIVATE
    opensuperdea_core
    GTest::gtest_main
)

# 黄金算例路径注入（测试经 OPENSUPERDEA_SOURCE_DIR 定位 tests/golden/）
target_compile_definitions(opensuperdea_unit_tests PRIVATE
    OPENSUPERDEA_SOURCE_DIR="${CMAKE_SOURCE_DIR}"
)

include(GoogleTest)
gtest_discover_tests(opensuperdea_unit_tests PROPERTIES LABELS "unit;m1")
