function(lcui_add_preview target)
    add_executable(${target} EXCLUDE_FROM_ALL ${ARGN})
    target_include_directories(${target} PRIVATE "${PROJECT_SOURCE_DIR}/tests/helpers")
    target_compile_definitions(${target} PRIVATE UNICODE _CRT_SECURE_NO_WARNINGS)
    target_link_libraries(${target} PRIVATE lcui)
    set_target_properties(${target} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/previews"
    )
endfunction()

lcui_add_preview(preview-flex-layout
    tests/previews/preview_flex_layout.c tests/scenes/flex_layout_scene.c
)
lcui_add_preview(preview-block-layout
    tests/previews/preview_block_layout.c tests/scenes/block_layout_scene.c
)
lcui_add_preview(preview-widget-opacity
    tests/previews/preview_widget_opacity.c tests/scenes/widget_opacity_scene.c
)
lcui_add_preview(preview-scrollbar
    tests/previews/preview_scrollbar.c tests/scenes/scrollbar_scene.c
)
lcui_add_preview(preview-flex-layout-percentage-wrap
    tests/previews/preview_flex_layout_percentage_wrap.c
    tests/integration/test_flex_layout_percentage_wrap.c
)
target_compile_definitions(preview-flex-layout-percentage-wrap PRIVATE
    LCUI_FLEX_LAYOUT_PERCENTAGE_WRAP_PREVIEW
)

add_custom_target(lcui-previews DEPENDS
    preview-flex-layout preview-block-layout preview-widget-opacity preview-scrollbar
    preview-flex-layout-percentage-wrap
)
