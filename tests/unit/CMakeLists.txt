# 单元测试可执行（按里程碑分目录 + 分目标组织用例源文件，ctest 标签 unit;<milestone>）
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

# M2 引擎测试（注册表/径向族/导出/CLI 全链路；独立目标便于 ctest -L m2 过滤）
add_executable(opensuperdea_unit_tests_m2
    m2/nonradial_test.cpp
    m2/golden_m2_test.cpp
    m2/registry_test.cpp
    m2/reference_set_test.cpp
    m2/radial_model_test.cpp
    m2/result_exporter_test.cpp
    m2/sha1_test.cpp
    m2/tfp_cross_window_test.cpp
    m2/validator_m2_test.cpp
    m2/cli_m2_test.cpp
)

target_link_libraries(opensuperdea_unit_tests_m2 PRIVATE
    opensuperdea_core
    GTest::gtest_main
)

# 黄金算例路径注入（测试经 OPENSUPERDEA_SOURCE_DIR 定位 tests/golden/）
target_compile_definitions(opensuperdea_unit_tests PRIVATE
    OPENSUPERDEA_SOURCE_DIR="${CMAKE_SOURCE_DIR}"
)
target_compile_definitions(opensuperdea_unit_tests_m2 PRIVATE
    OPENSUPERDEA_SOURCE_DIR="${CMAKE_SOURCE_DIR}"
)

include(GoogleTest)
gtest_discover_tests(opensuperdea_unit_tests PROPERTIES LABELS "unit;m1")
gtest_discover_tests(opensuperdea_unit_tests_m2 PROPERTIES LABELS "unit;m2")
