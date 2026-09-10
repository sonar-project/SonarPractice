# cmake/Dependencies.cmake
# Cross-platform external dependencies for SonarPractice (Windows / Linux / macOS).

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}")

include(AqtSqlPluginWorkaround)
include(FetchContent)

# ---------------------------------------------------------------------------
# Qt 6 (required)
# ---------------------------------------------------------------------------
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
    Svg
)
find_package(Qt6 REQUIRED COMPONENTS LinguistTools)

# Optional: AlphaTab player via Qt WebEngine (ASCII preview remains the fallback).
# Note: Official Qt Windows builds ship WebEngine only for MSVC — not MinGW/llvm-mingw.
set(SONARPRACTICE_HAS_WEBENGINE OFF)
find_package(Qt6 QUIET COMPONENTS WebEngineCore WebEngineQuick)
if(TARGET Qt6::WebEngineQuick AND TARGET Qt6::WebEngineCore)
    set(SONARPRACTICE_HAS_WEBENGINE ON)
    message(STATUS "Qt WebEngineQuick found — Guitar Pro AlphaTab player enabled")
else()
    message(STATUS "Qt WebEngineQuick not found — Guitar Pro ASCII preview only")
    if(WIN32 AND NOT MSVC)
        message(STATUS
            "  On Windows, Qt WebEngine is only provided for MSVC kits. "
            "MinGW/LLVM kits cannot enable the AlphaTab player (Chromium is not built for them).")
    endif()
endif()

# OpenSSL for libgp_parser (MinGW/LLVM: auto-generate .dll.a from Chocolatey DLLs)
include(SonarPracticeOpenSSL)

# ---------------------------------------------------------------------------
# Rubber Band + FFmpeg
# ---------------------------------------------------------------------------
include(FindRubberband)
include(FindFFmpeg)

# ---------------------------------------------------------------------------
# libgp_parser: local tree → find_package → FetchContent
# ---------------------------------------------------------------------------
set(LIBGP_PARSER_SOURCE_DIR "" CACHE PATH
    "Use local libgp_parser sources instead of find_package/FetchContent")
if(NOT LIBGP_PARSER_SOURCE_DIR AND EXISTS "${CMAKE_SOURCE_DIR}/../libgp_parser/CMakeLists.txt")
    set(LIBGP_PARSER_SOURCE_DIR "${CMAKE_SOURCE_DIR}/../libgp_parser")
endif()

if(LIBGP_PARSER_SOURCE_DIR)
    get_filename_component(LIBGP_PARSER_SOURCE_DIR "${LIBGP_PARSER_SOURCE_DIR}" ABSOLUTE)
    if(NOT EXISTS "${LIBGP_PARSER_SOURCE_DIR}/CMakeLists.txt")
        message(FATAL_ERROR
            "LIBGP_PARSER_SOURCE_DIR does not contain CMakeLists.txt: ${LIBGP_PARSER_SOURCE_DIR}")
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
        if(NOT OpenSSL_FOUND)
            message(FATAL_ERROR
                "libgp_parser FetchContent requires OpenSSL. "
                "Install OpenSSL and/or set OPENSSL_ROOT_DIR, or point "
                "LIBGP_PARSER_SOURCE_DIR at a local checkout "
                "(sibling ../libgp_parser is auto-detected when present).")
        endif()
        message(STATUS "libgp_parser not found — FetchContent v0.2.2-r3")
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
