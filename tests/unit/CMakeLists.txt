# 单元测试可执行（按里程碑分目录组织用例源文件，ctest 标签 unit;<milestone>）
add_executable(opensuperdea_unit_tests
    m1/highs_adapter_test.cpp
)

target_link_libraries(opensuperdea_unit_tests PRIVATE
    opensuperdea_core
    GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(opensuperdea_unit_tests PROPERTIES LABELS "unit;m1")
