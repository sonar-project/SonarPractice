# cmake/Dependencies.cmake
# External dependencies for SonarPractice.

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}")

find_package(Qt6 6.11 REQUIRED COMPONENTS
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

include(FindRubberband)
include(FindAubio)
include(FindFFmpeg)

include(FetchContent)

fetchcontent_declare(
    libgp_parser
    GIT_REPOSITORY https://github.com/sonar-project/libgp_parser.git
    GIT_TAG v0.1.1
)
fetchcontent_makeavailable(libgp_parser)
