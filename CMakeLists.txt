cmake_minimum_required(VERSION 3.20)

project(lcui VERSION 3.0.0 LANGUAGES C)

include(CTest)
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)
find_package(Threads REQUIRED)
find_package(PkgConfig QUIET)

set(FONTCONFIG_FOUND OFF)
set(LIBYAML_FOUND OFF)
set(LIBX11_FOUND OFF)
set(CAIRO_FOUND OFF)
set(Freetype_FOUND OFF)
set(PNG_FOUND OFF)
set(JPEG_FOUND OFF)
set(LibXml2_FOUND OFF)
set(OpenMP_C_FOUND OFF)
set(X11_FOUND OFF)
set(WAYLAND_FOUND OFF)
set(XKBCOMMON_FOUND OFF)
set(LIBDECOR_FOUND OFF)
set(WAYLAND_XDG_SHELL_XML "")
set(LCUI_HAS_FONTCONFIG OFF)
set(LCUI_HAS_LIBYAML OFF)
set(LCUI_YAML_USE_PKGCONFIG OFF)

option(BUILD_SHARED_LIBS "Build LCUI libraries as shared libraries" OFF)
option(LCUI_BUILD_TESTS "Build LCUI test programs" ${BUILD_TESTING})
option(LCUI_BUILD_EXAMPLES "Build LCUI examples" OFF)
option(LCUI_BUILD_DOC_VIEWER "Build the generated doc-viewer example" OFF)
option(LCUI_WITH_FONT "Enable FreeType and fontconfig integration when available" ON)
option(LCUI_WITH_IMAGE "Enable PNG and JPEG image formats when available" ON)
option(LCUI_WITH_OPENMP "Use OpenMP when it is available" ON)
option(LCUI_ENABLE_COVERAGE "Enable compiler coverage instrumentation" OFF)
option(LCUI_ENABLE_MEMCHECK "Run CTest programs under valgrind or Dr. Memory" OFF)
option(LCUI_WARNINGS_AS_ERRORS "Treat compiler warnings as errors" ON)
option(LCUI_CI_ENV "Define CI_ENV for CI builds" OFF)
option(PTK_TOUCH_ENABLED "Enable touch input support" ON)
option(LCUI_WITH_WAYLAND "Enable the native Wayland backend when available" ON)
option(LCUI_WITH_X11 "Enable the X11 backend when available" ON)

if(APPLE)
    enable_language(OBJC)
endif()

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)

if(NOT BUILD_SHARED_LIBS)
    set(LCUI_STATIC_BUILD ON)
else()
    set(LCUI_STATIC_BUILD OFF)
endif()

set(VERSION "${PROJECT_VERSION}-a")
set(VERSION_MAJOR "${PROJECT_VERSION_MAJOR}")
set(VERSION_MINOR "${PROJECT_VERSION_MINOR}")
set(VERSION_ALTER "${PROJECT_VERSION_PATCH}")
execute_process(
    COMMAND git rev-parse --short HEAD
    WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
    OUTPUT_VARIABLE GIT_COMMIT
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET
)
if(NOT GIT_COMMIT)
    set(GIT_COMMIT "unknown")
endif()

set(LCUI_GENERATED_INCLUDE_DIR "${PROJECT_BINARY_DIR}/generated/include")
file(MAKE_DIRECTORY "${LCUI_GENERATED_INCLUDE_DIR}")

function(lcui_configure_header input output)
    get_filename_component(output_dir "${output}" DIRECTORY)
    file(MAKE_DIRECTORY "${output_dir}")
    configure_file("${input}" "${output}" @ONLY)
endfunction()

if(PkgConfig_FOUND)
    pkg_check_modules(FONTCONFIG QUIET IMPORTED_TARGET fontconfig)
    pkg_check_modules(LIBYAML QUIET IMPORTED_TARGET yaml-0.1)
    if(LCUI_WITH_X11)
        pkg_check_modules(LIBX11 QUIET IMPORTED_TARGET x11)
    endif()
    pkg_check_modules(CAIRO QUIET IMPORTED_TARGET cairo)
    if(LCUI_WITH_WAYLAND AND NOT WIN32 AND NOT APPLE)
        pkg_check_modules(WAYLAND QUIET IMPORTED_TARGET wayland-client)
        pkg_check_modules(XKBCOMMON QUIET IMPORTED_TARGET xkbcommon)
        pkg_check_modules(LIBDECOR QUIET IMPORTED_TARGET libdecor-0)
    endif()
endif()

if(LCUI_WITH_WAYLAND AND WAYLAND_FOUND AND XKBCOMMON_FOUND)
    find_program(WAYLAND_SCANNER wayland-scanner)
    if(PkgConfig_FOUND)
        pkg_get_variable(_wayland_protocols_dir wayland-protocols pkgdatadir)
    endif()
    if(_wayland_protocols_dir)
        set(_xdg_shell_xml
            "${_wayland_protocols_dir}/stable/xdg-shell/xdg-shell.xml")
        if(EXISTS "${_xdg_shell_xml}")
            set(WAYLAND_XDG_SHELL_XML "${_xdg_shell_xml}")
        endif()
    endif()
endif()

find_package(Freetype QUIET)
find_package(PNG QUIET)
find_package(JPEG QUIET)
find_package(LibXml2 QUIET)
find_package(OpenMP QUIET COMPONENTS C)
if(LCUI_WITH_X11)
    find_package(X11 QUIET)
endif()

if(NOT LIBYAML_FOUND)
    find_path(LIBYAML_INCLUDE_DIR yaml.h)
    find_library(LIBYAML_LIBRARY NAMES yaml yaml-0)
    if(LIBYAML_INCLUDE_DIR AND LIBYAML_LIBRARY)
        add_library(PkgConfig::LIBYAML UNKNOWN IMPORTED GLOBAL)
        set_target_properties(PkgConfig::LIBYAML PROPERTIES
            IMPORTED_LOCATION "${LIBYAML_LIBRARY}"
            INTERFACE_INCLUDE_DIRECTORIES "${LIBYAML_INCLUDE_DIR}"
        )
        set(LIBYAML_FOUND ON)
        set(LCUI_YAML_USE_PKGCONFIG OFF)
    endif()
else()
    set(LCUI_YAML_USE_PKGCONFIG ON)
endif()

if(LCUI_WITH_FONT AND Freetype_FOUND)
    set(PANDAGL_HAS_FREETYPE ON)
endif()
if(LCUI_WITH_FONT AND FONTCONFIG_FOUND)
    set(PANDAGL_HAS_FONTCONFIG ON)
    set(LCUI_HAS_FONTCONFIG ON)
endif()
if(LCUI_WITH_IMAGE AND PNG_FOUND)
    set(PANDAGL_HAS_LIBPNG ON)
endif()
if(LCUI_WITH_IMAGE AND JPEG_FOUND)
    set(PANDAGL_HAS_LIBJPEG ON)
endif()
if(LIBYAML_FOUND)
    set(LIBI18N_HAS_YAML ON)
    set(LCUI_HAS_LIBYAML ON)
endif()
if(LibXml2_FOUND)
    set(LIBUI_XML_HAS_LIBXML2 ON)
endif()
if(LCUI_WITH_OPENMP AND OpenMP_C_FOUND)
    set(LIBUI_SERVER_HAS_OPENMP ON)
endif()
set(PANDAGL_HAS_FONT ON)
set(PANDAGL_HAS_TEXT ON)
set(PANDAGL_HAS_IMAGE ON)

if(NOT BUILD_SHARED_LIBS)
    foreach(static_macro YUTIL_STATIC_BUILD PANDAGL_STATIC_BUILD PTK_STATIC_BUILD
                         LIBTHREAD_STATIC_BUILD LIBCSS_STATIC_BUILD LIBI18N_STATIC_BUILD
                         LIBWORKER_STATIC_BUILD LIBUI_STATIC_BUILD
                         LIBUI_SERVER_STATIC_BUILD LIBUI_CURSOR_STATIC_BUILD
                         LIBUI_XML_STATIC_BUILD LIBUI_ROUTER_STATIC_BUILD
                         LIBROUTER_STATIC_BUILD LCUI_STATIC_BUILD)
        set(${static_macro} ON)
    endforeach()
endif()

if(LCUI_WITH_X11 AND NOT WIN32 AND NOT APPLE AND
   (LIBX11_FOUND OR X11_FOUND))
    set(PTK_HAS_LIBX11 ON)
endif()

# Wayland 后端需要 wayland-client、xkbcommon、wayland-scanner 和 xdg-shell.xml
if(WAYLAND_FOUND AND XKBCOMMON_FOUND AND WAYLAND_SCANNER
   AND WAYLAND_XDG_SHELL_XML)
    set(PTK_HAS_LIBWAYLAND ON)
    if(LIBDECOR_FOUND)
        set(PTK_HAS_LIBDECOR ON)
    endif()
elseif(LCUI_WITH_WAYLAND AND NOT WIN32 AND NOT APPLE)
    message(STATUS "Wayland backend disabled: "
        "wayland-client=${WAYLAND_FOUND} xkbcommon=${XKBCOMMON_FOUND} "
        "scanner=${WAYLAND_SCANNER} xdg-shell.xml=${WAYLAND_XDG_SHELL_XML}")
endif()

lcui_configure_header(src/config.h.in "${LCUI_GENERATED_INCLUDE_DIR}/LCUI/config.h")
lcui_configure_header(lib/yutil/include/yutil/config.h.in "${LCUI_GENERATED_INCLUDE_DIR}/yutil/config.h")
lcui_configure_header(lib/thread/src/thread.h.in "${LCUI_GENERATED_INCLUDE_DIR}/thread.h")
lcui_configure_header(lib/pandagl/src/config.h.in "${LCUI_GENERATED_INCLUDE_DIR}/pandagl/config.h")
lcui_configure_header(lib/ptk/src/config.h.in "${LCUI_GENERATED_INCLUDE_DIR}/ptk/config.h")
lcui_configure_header(lib/css/src/config.h.in "${LCUI_GENERATED_INCLUDE_DIR}/css/config.h")
lcui_configure_header(lib/i18n/src/i18n.h.in "${LCUI_GENERATED_INCLUDE_DIR}/i18n.h")
lcui_configure_header(lib/worker/src/config.h.in "${LCUI_GENERATED_INCLUDE_DIR}/worker/config.h")
lcui_configure_header(lib/ui/src/config.h.in "${LCUI_GENERATED_INCLUDE_DIR}/ui/config.h")
lcui_configure_header(lib/ui-server/src/config.h.in "${LCUI_GENERATED_INCLUDE_DIR}/ui_server/config.h")
lcui_configure_header(lib/ui-cursor/src/config.h.in "${LCUI_GENERATED_INCLUDE_DIR}/ui_cursor/config.h")
lcui_configure_header(lib/ui-xml/src/config.h.in "${LCUI_GENERATED_INCLUDE_DIR}/ui_xml/config.h")
lcui_configure_header(lib/ui-router/src/config.h.in "${LCUI_GENERATED_INCLUDE_DIR}/ui_router/config.h")
lcui_configure_header(lib/router/src/version.h.in "${LCUI_GENERATED_INCLUDE_DIR}/router/version.h")

include(cmake/LCUITargets.cmake)
include(cmake/LCUIPreviews.cmake)

if(LCUI_BUILD_TESTS)
    enable_testing()
    include(cmake/LCUITests.cmake)
endif()

if(LCUI_BUILD_EXAMPLES)
    include(cmake/LCUIExamples.cmake)
endif()

set(LCUI_INSTALL_TARGETS
    yutil pandagl libthread libptk libcss libi18n libworker libui libui-cursor
    libui-server librouter libui-router libui-xml lcui
)
install(TARGETS ${LCUI_INSTALL_TARGETS}
    EXPORT LCUITargets
    ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
    INCLUDES DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
)
install(DIRECTORY include/ DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
    FILES_MATCHING PATTERN "*.h"
)
foreach(lib yutil pandagl thread ptk css i18n worker ui ui-server ui-cursor ui-xml ui-router router ctest)
    if(EXISTS "${PROJECT_SOURCE_DIR}/lib/${lib}/include")
        install(DIRECTORY "lib/${lib}/include/" DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
            FILES_MATCHING PATTERN "*.h"
        )
    endif()
endforeach()
install(DIRECTORY "${LCUI_GENERATED_INCLUDE_DIR}/" DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
    FILES_MATCHING PATTERN "*.h"
)

install(EXPORT LCUITargets
    FILE LCUITargets.cmake
    NAMESPACE LCUI::
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/LCUI"
)
configure_package_config_file(
    cmake/LCUIConfig.cmake.in
    "${PROJECT_BINARY_DIR}/LCUIConfig.cmake"
    INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/LCUI"
)
write_basic_package_version_file(
    "${PROJECT_BINARY_DIR}/LCUIConfigVersion.cmake"
    VERSION "${PROJECT_VERSION}"
    COMPATIBILITY SameMajorVersion
)
install(FILES
    "${PROJECT_BINARY_DIR}/LCUIConfig.cmake"
    "${PROJECT_BINARY_DIR}/LCUIConfigVersion.cmake"
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/LCUI"
)

include(CPack)
