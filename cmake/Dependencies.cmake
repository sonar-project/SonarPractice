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

find_package(libgp_parser 0.2 QUIET)
if(libgp_parser_FOUND)
    message(STATUS "Using system libgp_parser")
else()
    message(STATUS "libgp_parser not found — FetchContent v0.2.2-r1")
    set(LIBGP_PARSER_BUILD_EXAMPLE ON CACHE BOOL "Disable libgp_parser Example" FORCE)
    set(LIBGP_PARSER_BUILD_TESTS ON CACHE BOOL "Disable libgp_parser tests" FORCE)
    fetchcontent_declare(
        libgp_parser
        GIT_REPOSITORY https://github.com/sonar-project/libgp_parser.git
        GIT_TAG v0.2.2-r2
    )
    fetchcontent_makeavailable(libgp_parser)
endif()
