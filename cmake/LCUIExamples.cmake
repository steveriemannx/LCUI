function(lcui_add_example target source_dir)
    file(GLOB_RECURSE sources CONFIGURE_DEPENDS "${source_dir}/*.c")
    if(NOT sources)
        return()
    endif()
    add_executable(${target} ${sources})
    target_compile_definitions(${target} PRIVATE UNICODE _CRT_SECURE_NO_WARNINGS)
    target_link_libraries(${target} PRIVATE ${ARGN})
    set_target_properties(${target} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/examples/${target}"
    )
    install(TARGETS ${target} RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}/examples/${target}" COMPONENT Examples)
    set_property(GLOBAL APPEND PROPERTY LCUI_EXAMPLE_TARGETS ${target})
endfunction()

foreach(example fill-rect fill-rect-with-rgba paint-background paint-border paint-boxshadow
                mix-rect-with-opacity pixel-manipulation image-scaling-bench char-render
                string-render font-mixed-baseline)
    lcui_add_example(${example} "lib/pandagl/examples/${example}" pandagl)
endforeach()
if(CAIRO_FOUND)
    lcui_add_example(pandagl-cairo-example lib/pandagl/examples/cairo pandagl)
    target_link_libraries(pandagl-cairo-example PRIVATE PkgConfig::CAIRO)
else()
    message(STATUS "Cairo was not found; the pandagl Cairo example is omitted")
endif()

foreach(example hello todolist)
    lcui_add_example(${example} "examples/${example}" lcui)
    if(EXISTS "${PROJECT_SOURCE_DIR}/examples/${example}/app")
        install(DIRECTORY "${PROJECT_SOURCE_DIR}/examples/${example}/app/"
            DESTINATION "${CMAKE_INSTALL_BINDIR}/examples/${example}/app"
            COMPONENT Examples
        )
    endif()
endforeach()

lcui_add_example(fabric examples/fabric lcui)
if(TARGET fabric AND CAIRO_FOUND)
    target_compile_definitions(fabric PRIVATE HAS_CAIRO)
    target_link_libraries(fabric PRIVATE PkgConfig::CAIRO)
endif()

if(LCUI_BUILD_DOC_VIEWER)
    file(GLOB_RECURSE doc_viewer_generated_headers CONFIGURE_DEPENDS
        "${PROJECT_SOURCE_DIR}/examples/doc-viewer/app/*.tsx.h"
    )
    if(NOT doc_viewer_generated_headers)
        message(FATAL_ERROR "Generate the doc-viewer app headers with lcui build before enabling LCUI_BUILD_DOC_VIEWER")
    endif()
    lcui_add_example(doc-viewer examples/doc-viewer/app lcui)
    set_target_properties(doc-viewer PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/examples/doc-viewer/dist")
    if(EXISTS "${PROJECT_SOURCE_DIR}/examples/doc-viewer/dist/assets")
        install(DIRECTORY "${PROJECT_SOURCE_DIR}/examples/doc-viewer/dist/assets/"
            DESTINATION "${CMAKE_INSTALL_BINDIR}/examples/doc-viewer/assets"
            COMPONENT Examples
        )
    endif()
endif()

get_property(LCUI_EXAMPLE_TARGETS GLOBAL PROPERTY LCUI_EXAMPLE_TARGETS)
if(LCUI_EXAMPLE_TARGETS)
    add_custom_target(lcui-examples DEPENDS ${LCUI_EXAMPLE_TARGETS})
endif()
