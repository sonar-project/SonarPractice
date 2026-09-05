# cmake/Dependencies.cmake
# External dependencies for SonarPractice.

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}")

include(AqtSqlPluginWorkaround)

find_package(Qt6 6.8 REQUIRED COMPONENTS
    Core
    Concurrent
    Multimedia
    Sql
    Test
    Quick
    QuickControls2
    QuickDialogs2
    Qml
    QuickTest
    Widgets
)

# Optional: AlphaTab player via Qt WebEngine (ASCII preview remains the fallback).
set(SONARPRACTICE_HAS_WEBENGINE OFF)
find_package(Qt6 QUIET COMPONENTS WebEngineQuick)
if(TARGET Qt6::WebEngineQuick)
    set(SONARPRACTICE_HAS_WEBENGINE ON)
    message(STATUS "Qt WebEngineQuick found — Guitar Pro AlphaTab player enabled")
else()
    message(STATUS "Qt WebEngineQuick not found — Guitar Pro ASCII preview only")
endif()

include(FindRubberband)
include(FindFFmpeg)

include(FetchContent)

# Prefer a local checkout for Qt Creator / in-tree dev builds:
# 1) -DLIBGP_PARSER_SOURCE_DIR=...  2) sibling ../libgp_parser  3) find_package  4) FetchContent
set(LIBGP_PARSER_SOURCE_DIR "" CACHE PATH
    "Use local libgp_parser sources instead of find_package/FetchContent")
if(NOT LIBGP_PARSER_SOURCE_DIR AND EXISTS "${CMAKE_SOURCE_DIR}/../libgp_parser/CMakeLists.txt")
    set(LIBGP_PARSER_SOURCE_DIR "${CMAKE_SOURCE_DIR}/../libgp_parser")
endif()

if(LIBGP_PARSER_SOURCE_DIR)
    get_filename_component(LIBGP_PARSER_SOURCE_DIR "${LIBGP_PARSER_SOURCE_DIR}" ABSOLUTE)
    if(NOT EXISTS "${LIBGP_PARSER_SOURCE_DIR}/CMakeLists.txt")
        message(FATAL_ERROR "LIBGP_PARSER_SOURCE_DIR does not contain CMakeLists.txt: ${LIBGP_PARSER_SOURCE_DIR}")
    endif()
    message(STATUS "Using local libgp_parser from ${LIBGP_PARSER_SOURCE_DIR}")
    set(LIBGP_PARSER_BUILD_EXAMPLE OFF CACHE BOOL "Disable libgp_parser Example" FORCE)
    set(LIBGP_PARSER_BUILD_TESTS OFF CACHE BOOL "Disable libgp_parser tests" FORCE)
    add_subdirectory("${LIBGP_PARSER_SOURCE_DIR}" "${CMAKE_BINARY_DIR}/_deps/libgp_parser-build")
else()
    find_package(libgp_parser 0.2 QUIET)
    if(libgp_parser_FOUND)
        message(STATUS "Using system libgp_parser")
    else()
        message(STATUS "libgp_parser not found — FetchContent v0.2.2-r1")
        set(LIBGP_PARSER_BUILD_EXAMPLE OFF CACHE BOOL "Disable libgp_parser Example" FORCE)
        set(LIBGP_PARSER_BUILD_TESTS OFF CACHE BOOL "Disable libgp_parser tests" FORCE)
        fetchcontent_declare(
            libgp_parser
            GIT_REPOSITORY https://github.com/sonar-project/libgp_parser.git
            GIT_TAG v0.2.2-r3
        )
        fetchcontent_makeavailable(libgp_parser)
    endif()
endif()
