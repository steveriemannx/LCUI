if(LCUI_ENABLE_MEMCHECK)
    if(WIN32)
        find_program(LCUI_MEMCHECK_EXECUTABLE drmemory REQUIRED)
        set(LCUI_TEST_PREFIX "${LCUI_MEMCHECK_EXECUTABLE};--")
    else()
        find_program(LCUI_MEMCHECK_EXECUTABLE valgrind REQUIRED)
        set(LCUI_TEST_PREFIX "${LCUI_MEMCHECK_EXECUTABLE};--leak-check=full;--error-exitcode=42;--num-callers=20")
    endif()
endif()

function(lcui_add_test_program target)
    add_executable(${target} ${ARGN})
    target_compile_definitions(${target} PRIVATE UNICODE _CRT_SECURE_NO_WARNINGS)
    if(LCUI_WARNINGS_AS_ERRORS)
        if(MSVC)
            target_compile_options(${target} PRIVATE /W4 /WX)
        else()
            target_compile_options(${target} PRIVATE -Wall -Werror)
        endif()
    endif()
    if(LCUI_ENABLE_COVERAGE AND NOT MSVC)
        target_compile_options(${target} PRIVATE --coverage)
        target_link_options(${target} PRIVATE --coverage)
    endif()
    if(LCUI_ENABLE_MEMCHECK)
        add_test(NAME ${target} COMMAND ${LCUI_TEST_PREFIX} "$<TARGET_FILE:${target}>")
    else()
        add_test(NAME ${target} COMMAND "$<TARGET_FILE:${target}>")
    endif()
endfunction()

lcui_add_test_program(ctest-tests
    lib/ctest/tests/main.c lib/ctest/tests/test_str.c lib/ctest/tests/test_grep.c
)
set_tests_properties(ctest-tests PROPERTIES WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}/lib/ctest/tests")
target_link_libraries(ctest-tests PRIVATE ctest yutil)

file(GLOB yutil_test_sources CONFIGURE_DEPENDS "lib/yutil/tests/*.c")
add_executable(yutil-tests ${yutil_test_sources})
target_compile_definitions(yutil-tests PRIVATE UNICODE _CRT_SECURE_NO_WARNINGS)
target_link_libraries(yutil-tests PRIVATE yutil)
set_target_properties(yutil-tests PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/tests"
)
if(LCUI_WARNINGS_AS_ERRORS)
    if(MSVC)
        target_compile_options(yutil-tests PRIVATE /W4 /WX)
    else()
        target_compile_options(yutil-tests PRIVATE -Wall -Werror)
    endif()
endif()
if(NOT MSVC)
    target_compile_options(yutil-tests PRIVATE -Wno-error=unused-variable -Wno-error=unused-but-set-variable)
endif()

lcui_add_test_program(libthread-tests lib/thread/tests/main.c lib/thread/tests/test_thread.c)
set_tests_properties(libthread-tests PROPERTIES WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}/lib/thread/tests")
target_link_libraries(libthread-tests PRIVATE ctest libthread)

lcui_add_test_program(libptk-tests
    lib/ptk/tests/main.c lib/ptk/tests/test_steptimer.c lib/ptk/tests/test_timer_scheduler.c
)
set_tests_properties(libptk-tests PROPERTIES WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}/lib/ptk/tests")
target_link_libraries(libptk-tests PRIVATE ctest libptk)
target_include_directories(libptk-tests PRIVATE lib/ptk/src)

if(LCUI_WITH_FONT AND LCUI_WITH_IMAGE AND Freetype_FOUND AND PNG_FOUND AND JPEG_FOUND)
    lcui_add_test_program(pandagl-tests
        lib/pandagl/tests/main.c
        lib/pandagl/tests/test_canvas_mix.c
        lib/pandagl/tests/test_image_reader.c
        lib/pandagl/tests/test_font_load.c
        lib/pandagl/tests/test_text_edit.c
        lib/pandagl/tests/test_font_cache.c
        lib/pandagl/tests/test_text_baseline.c
        lib/pandagl/tests/test_text_style_tag.c
    )
    set_tests_properties(pandagl-tests PROPERTIES WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}/lib/pandagl/tests/fixtures")
    target_link_libraries(pandagl-tests PRIVATE ctest pandagl)
endif()

file(GLOB css_test_sources CONFIGURE_DEPENDS "lib/css/tests/*.c")
lcui_add_test_program(libcss-tests ${css_test_sources})
set_tests_properties(libcss-tests PROPERTIES WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}/lib/css/tests")
target_include_directories(libcss-tests PRIVATE lib/css/tests)
target_link_libraries(libcss-tests PRIVATE ctest libcss)

lcui_add_test_program(libi18n-tests lib/i18n/tests/test.c)
set_tests_properties(libi18n-tests PROPERTIES WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}/lib/i18n/tests")
target_link_libraries(libi18n-tests PRIVATE ctest libi18n)

lcui_add_test_program(librouter-tests lib/router/tests/test.c)
set_tests_properties(librouter-tests PROPERTIES WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}/lib/router/tests")
target_link_libraries(librouter-tests PRIVATE ctest librouter)

if(LibXml2_FOUND)
    lcui_add_test_program(libui-xml-tests
        lib/ui-xml/tests/main.c lib/ui-xml/tests/test_xml_parser.c
    )
    set_tests_properties(libui-xml-tests PROPERTIES WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}/lib/ui-xml/tests/fixtures")
    target_link_libraries(libui-xml-tests PRIVATE ctest libui-xml)
endif()

if(LCUI_WITH_FONT AND Freetype_FOUND)
    file(GLOB integration_sources CONFIGURE_DEPENDS
        "tests/integration/*.c" "tests/scenes/*.c"
    )
    lcui_add_test_program(lcui-tests tests/main.c ${integration_sources})
    set_tests_properties(lcui-tests PROPERTIES WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}/tests/fixtures")
    target_include_directories(lcui-tests PRIVATE tests/helpers)
    target_link_libraries(lcui-tests PRIVATE lcui ctest)
else()
    message(STATUS "Skipping font-dependent LCUI integration tests")
endif()

if(LCUI_ENABLE_COVERAGE AND NOT MSVC)
    target_link_options(ctest-tests PRIVATE --coverage)
    target_link_options(yutil-tests PRIVATE --coverage)
endif()
