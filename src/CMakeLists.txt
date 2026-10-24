# 引擎静态库（core + adapters）与 CLI 可执行
# 目标名 snake_case（naming_convention.md §四）；目录职责见 design_docs/01_overall_design.md §5
add_library(opensuperdea_core STATIC
    adapters/highs_adapter.cpp
    cli/cli_app.cpp
    core/dataset/csv_parser.cpp
    core/dataset/dataset.cpp
    core/dataset/dataset_builder.cpp
    core/dataset/validator.cpp
    core/distance/ddf_model.cpp
    core/nonradial/additive_models.cpp
    core/nonradial/fdh_model.cpp
    core/nonradial/price_models.cpp
    core/nonradial/sbm_model.cpp
    core/radial/multiplier_model.cpp
    core/radial/radial_envelope_model.cpp
    core/radial/scale_efficiency_model.cpp
    core/registry/model_registry.cpp
    core/registry/model_runner.cpp
    core/registry/reference_set.cpp
    core/registry/solve_options.cpp
    core/result/dmu_result.cpp
    core/result/result_exporter.cpp
    core/util/sha1.cpp
)

# 引擎头以 src/ 为根（#include "core/..." / "adapters/..."），对测试与未来 GUI 复用开放
target_include_directories(opensuperdea_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# Eigen 随 core 公共头暴露（数据矩阵底座）；HiGHS 仅适配层 .cpp 接触，不外泄（DR-004）
target_link_libraries(opensuperdea_core PUBLIC Eigen3::Eigen)
target_link_libraries(opensuperdea_core PRIVATE highs::highs CLI11::CLI11)

add_executable(opensuperdea main.cpp)
target_link_libraries(opensuperdea PRIVATE opensuperdea_core)
