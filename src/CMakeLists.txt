# opensuperdea 可执行目标（目标名 snake_case，见 naming_convention.md §四）
add_executable(opensuperdea
    main.cpp
)
# CLI 占位无外部链接；M1 引入 CLI11 后在此追加 target_link_libraries。
# GUI 骨架（main_window.cpp/h + Qt6::Widgets）已移入 tmp/gui_skeleton/，M6 恢复。
