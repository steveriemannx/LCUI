set(LCUI_PUBLIC_INCLUDE_DIRS
    "${PROJECT_SOURCE_DIR}/include"
    "${PROJECT_SOURCE_DIR}/lib/ctest/include"
    "${PROJECT_SOURCE_DIR}/lib/yutil/include"
    "${PROJECT_SOURCE_DIR}/lib/pandagl/include"
    "${PROJECT_SOURCE_DIR}/lib/thread/include"
    "${PROJECT_SOURCE_DIR}/lib/css/include"
    "${PROJECT_SOURCE_DIR}/lib/i18n/include"
    "${PROJECT_SOURCE_DIR}/lib/ptk/include"
    "${PROJECT_SOURCE_DIR}/lib/worker/include"
    "${PROJECT_SOURCE_DIR}/lib/router/include"
    "${PROJECT_SOURCE_DIR}/lib/ui/include"
    "${PROJECT_SOURCE_DIR}/lib/ui-server/include"
    "${PROJECT_SOURCE_DIR}/lib/ui-cursor/include"
    "${PROJECT_SOURCE_DIR}/lib/ui-xml/include"
    "${PROJECT_SOURCE_DIR}/lib/ui-router/include"
)

function(lcui_add_library target source_dir recursive)
    if(recursive)
        file(GLOB_RECURSE sources CONFIGURE_DEPENDS "${source_dir}/src/*.c")
    else()
        file(GLOB sources CONFIGURE_DEPENDS "${source_dir}/src/*.c")
    endif()
    add_library(${target} ${sources})
    target_compile_definitions(${target} PRIVATE UNICODE _CRT_SECURE_NO_WARNINGS)
    target_include_directories(${target} BEFORE PUBLIC
        "$<BUILD_INTERFACE:${LCUI_GENERATED_INCLUDE_DIR}>"
    )
    foreach(include_dir ${LCUI_PUBLIC_INCLUDE_DIRS})
        target_include_directories(${target} PUBLIC "$<BUILD_INTERFACE:${include_dir}>")
    endforeach()
    target_include_directories(${target} PUBLIC "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>")
    if(LCUI_WARNINGS_AS_ERRORS)
        if(MSVC)
            target_compile_options(${target} PRIVATE /W4 /WX)
        else()
            target_compile_options(${target} PRIVATE -Wall -Werror)
        endif()
    endif()
    if(NOT MSVC)
        set_target_properties(${target} PROPERTIES POSITION_INDEPENDENT_CODE ON)
    endif()
endfunction()

lcui_add_library(yutil lib/yutil FALSE)
target_compile_definitions(yutil PRIVATE YUTIL_EXPORTS _UNICODE)
if(WIN32)
    target_compile_definitions(yutil PRIVATE _CRT_SECURE_NO_WARNINGS)
else()
    target_compile_definitions(yutil PRIVATE _GNU_SOURCE=1)
endif()
lcui_add_library(pandagl lib/pandagl FALSE)
target_link_libraries(pandagl PUBLIC yutil)
if(BUILD_SHARED_LIBS AND WIN32)
    target_compile_definitions(pandagl PRIVATE PANDAGL_DLL_EXPORT)
endif()
file(GLOB_RECURSE font_sources CONFIGURE_DEPENDS "lib/pandagl/src/font/*.c")
target_sources(pandagl PRIVATE ${font_sources})
if(LCUI_WITH_FONT AND Freetype_FOUND)
    target_link_libraries(pandagl PUBLIC Freetype::Freetype)
endif()
file(GLOB_RECURSE text_sources CONFIGURE_DEPENDS "lib/pandagl/src/text/*.c")
target_sources(pandagl PRIVATE ${text_sources})
if(LCUI_WITH_FONT AND FONTCONFIG_FOUND)
    target_link_libraries(pandagl PUBLIC PkgConfig::FONTCONFIG)
endif()
file(GLOB_RECURSE image_sources CONFIGURE_DEPENDS "lib/pandagl/src/image/*.c")
target_sources(pandagl PRIVATE ${image_sources})
if(LCUI_WITH_IMAGE AND PNG_FOUND)
    target_link_libraries(pandagl PUBLIC PNG::PNG)
endif()
if(LCUI_WITH_IMAGE AND JPEG_FOUND)
    target_link_libraries(pandagl PUBLIC JPEG::JPEG)
endif()
lcui_add_library(libthread lib/thread FALSE)
if(BUILD_SHARED_LIBS AND WIN32)
    target_compile_definitions(libthread PRIVATE LIBTHREAD_DLL_EXPORT)
endif()
if(NOT WIN32)
    target_link_libraries(libthread PUBLIC Threads::Threads ${CMAKE_DL_LIBS})
endif()
target_link_libraries(libthread PUBLIC yutil)

lcui_add_library(libptk lib/ptk FALSE)
target_link_libraries(libptk PUBLIC yutil pandagl)
# Wayland 后端位于 lib/ptk/src/*.c，需要让平台层能按名字包含内部头文件
target_include_directories(libptk PRIVATE "${PROJECT_SOURCE_DIR}/lib/ptk/src")
if(PTK_HAS_LIBWAYLAND)
    set(WAYLAND_GEN_DIR "${PROJECT_BINARY_DIR}/generated/wayland")
    file(MAKE_DIRECTORY "${WAYLAND_GEN_DIR}")
    set(WAYLAND_XDG_SHELL_H "${WAYLAND_GEN_DIR}/xdg-shell-client-protocol.h")
    set(WAYLAND_XDG_SHELL_C "${WAYLAND_GEN_DIR}/xdg-shell-client-protocol.c")
    add_custom_command(
        OUTPUT "${WAYLAND_XDG_SHELL_H}" "${WAYLAND_XDG_SHELL_C}"
        COMMAND "${WAYLAND_SCANNER}" client-header
                "${WAYLAND_XDG_SHELL_XML}" "${WAYLAND_XDG_SHELL_H}"
        COMMAND "${WAYLAND_SCANNER}" private-code
                "${WAYLAND_XDG_SHELL_XML}" "${WAYLAND_XDG_SHELL_C}"
        DEPENDS "${WAYLAND_XDG_SHELL_XML}"
        VERBATIM
    )
    target_sources(libptk PRIVATE "${WAYLAND_XDG_SHELL_C}")
    target_include_directories(libptk PRIVATE "${WAYLAND_GEN_DIR}")
    target_link_libraries(libptk PUBLIC PkgConfig::WAYLAND PkgConfig::XKBCOMMON)
    if(PTK_HAS_LIBDECOR)
        target_link_libraries(libptk PUBLIC PkgConfig::LIBDECOR)
    endif()
endif()
if(BUILD_SHARED_LIBS AND WIN32)
    target_compile_definitions(libptk PRIVATE PTK_DLL_EXPORT)
endif()
if(WIN32)
    file(GLOB_RECURSE ptk_platform_sources CONFIGURE_DEPENDS "lib/ptk/src/windows/*.c")
    target_sources(libptk PRIVATE ${ptk_platform_sources})
    target_compile_definitions(libptk PRIVATE _CRT_SECURE_NO_WARNINGS)
    target_link_libraries(libptk PUBLIC Shell32)
elseif(APPLE)
    file(GLOB_RECURSE ptk_platform_sources CONFIGURE_DEPENDS
        "lib/ptk/src/macos/*.c" "lib/ptk/src/macos/*.m"
    )
    target_sources(libptk PRIVATE ${ptk_platform_sources})
    target_link_libraries(libptk PUBLIC "-framework AppKit")
else()
    file(GLOB_RECURSE ptk_platform_sources CONFIGURE_DEPENDS "lib/ptk/src/linux/*.c")
    target_sources(libptk PRIVATE ${ptk_platform_sources})
    target_link_libraries(libptk PUBLIC Threads::Threads ${CMAKE_DL_LIBS})
    if(LIBX11_FOUND)
        target_link_libraries(libptk PUBLIC PkgConfig::LIBX11)
    elseif(X11_FOUND)
        target_include_directories(libptk PRIVATE ${X11_INCLUDE_DIR})
        target_link_libraries(libptk PUBLIC ${X11_LIBRARIES})
    endif()
endif()
lcui_add_library(libcss lib/css TRUE)
target_link_libraries(libcss PUBLIC yutil)
if(BUILD_SHARED_LIBS AND WIN32)
    target_compile_definitions(libcss PRIVATE LIBCSS_DLL_EXPORT)
endif()

lcui_add_library(libi18n lib/i18n FALSE)
target_link_libraries(libi18n PUBLIC yutil)
if(BUILD_SHARED_LIBS AND WIN32)
    target_compile_definitions(libi18n PRIVATE LIBI18N_DLL_EXPORT)
endif()
if(LIBYAML_FOUND)
    target_link_libraries(libi18n PUBLIC PkgConfig::LIBYAML)
endif()

lcui_add_library(libworker lib/worker TRUE)
target_link_libraries(libworker PUBLIC yutil libthread)
if(BUILD_SHARED_LIBS AND WIN32)
    target_compile_definitions(libworker PRIVATE LIBWORKER_DLL_EXPORT)
endif()

lcui_add_library(libui lib/ui TRUE)
target_link_libraries(libui PUBLIC yutil pandagl libcss)
if(BUILD_SHARED_LIBS AND WIN32)
    target_compile_definitions(libui PRIVATE LIBUI_DLL_EXPORT)
endif()

lcui_add_library(libui-cursor lib/ui-cursor TRUE)
target_link_libraries(libui-cursor PUBLIC yutil pandagl libptk)
if(BUILD_SHARED_LIBS AND WIN32)
    target_compile_definitions(libui-cursor PRIVATE LIBUI_CURSOR_DLL_EXPORT)
endif()

lcui_add_library(libui-server lib/ui-server TRUE)
target_link_libraries(libui-server PUBLIC yutil pandagl libptk libui libui-cursor)
if(BUILD_SHARED_LIBS AND WIN32)
    target_compile_definitions(libui-server PRIVATE LIBUI_SERVER_DLL_EXPORT)
endif()
if(LCUI_WITH_OPENMP AND OpenMP_C_FOUND)
    target_link_libraries(libui-server PUBLIC OpenMP::OpenMP_C)
endif()

lcui_add_library(librouter lib/router FALSE)
target_link_libraries(librouter PUBLIC yutil libui)
if(BUILD_SHARED_LIBS AND WIN32)
    target_compile_definitions(librouter PRIVATE LIBROUTER_DLL_EXPORT)
endif()

lcui_add_library(libui-router lib/ui-router TRUE)
target_link_libraries(libui-router PUBLIC yutil libui librouter)
if(BUILD_SHARED_LIBS AND WIN32)
    target_compile_definitions(libui-router PRIVATE LIBUI_ROUTER_DLL_EXPORT)
endif()

lcui_add_library(libui-xml lib/ui-xml TRUE)
target_link_libraries(libui-xml PUBLIC yutil libui libcss pandagl)
if(BUILD_SHARED_LIBS AND WIN32)
    target_compile_definitions(libui-xml PRIVATE LIBUI_XML_DLL_EXPORT)
endif()
if(LibXml2_FOUND)
    target_link_libraries(libui-xml PUBLIC LibXml2::LibXml2)
endif()

add_library(ctest STATIC lib/ctest/src/ctest.c)
target_include_directories(ctest PUBLIC
    "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/lib/ctest/include>"
    "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>"
)
target_compile_definitions(ctest PRIVATE UNICODE _CRT_SECURE_NO_WARNINGS)
target_link_libraries(ctest PUBLIC yutil)
if(LCUI_WARNINGS_AS_ERRORS)
    if(MSVC)
        target_compile_options(ctest PRIVATE /W4 /WX)
    else()
        target_compile_options(ctest PRIVATE -Wall -Werror)
    endif()
endif()

file(GLOB_RECURSE lcui_sources CONFIGURE_DEPENDS "src/*.c")
add_library(lcui ${lcui_sources})
add_library(LCUI::lcui ALIAS lcui)
target_include_directories(lcui BEFORE PUBLIC "$<BUILD_INTERFACE:${LCUI_GENERATED_INCLUDE_DIR}>")
target_include_directories(lcui PUBLIC
    "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>"
    "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>"
)
target_link_libraries(lcui PUBLIC
    yutil libthread libptk libworker pandagl libcss libi18n libui libui-cursor
    libui-server librouter libui-router libui-xml
)
target_compile_definitions(lcui PRIVATE UNICODE _CRT_SECURE_NO_WARNINGS)
if(LCUI_CI_ENV)
    target_compile_definitions(lcui PRIVATE CI_ENV)
endif()
if(BUILD_SHARED_LIBS AND WIN32)
    target_compile_definitions(lcui PRIVATE LCUI_DLL_EXPORT)
endif()
if(LCUI_WITH_FONT AND FONTCONFIG_FOUND)
    target_compile_definitions(lcui PRIVATE HAVE_FONTCONFIG)
endif()
if(LCUI_WARNINGS_AS_ERRORS)
    if(MSVC)
        target_compile_options(lcui PRIVATE /W4 /WX)
    else()
        target_compile_options(lcui PRIVATE -Wall -Werror)
    endif()
endif()
if(NOT MSVC)
    set_target_properties(lcui PROPERTIES POSITION_INDEPENDENT_CODE ON)
endif()

if(LCUI_ENABLE_COVERAGE AND NOT MSVC)
    foreach(target ${LCUI_INSTALL_TARGETS} ctest)
        target_compile_options(${target} PRIVATE --coverage)
        target_link_options(${target} PRIVATE --coverage)
    endforeach()
endif()
