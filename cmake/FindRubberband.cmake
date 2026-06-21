# Rubber Band: Always use FetchContent for simplicity

include(FetchContent)

fetchcontent_declare(
    rubberband
    GIT_REPOSITORY https://github.com/breakfastquay/rubberband.git
    GIT_TAG v4.0.0
    GIT_SHALLOW TRUE
)

if(NOT TARGET SonarPractice_Rubberband)
    fetchcontent_getproperties(rubberband)
    if(NOT rubberband_POPULATED)
        fetchcontent_makeavailable(rubberband)
    endif()

    add_library(SonarPractice_Rubberband SHARED "${rubberband_SOURCE_DIR}/single/RubberBandSingle.cpp"
    )
    target_include_directories(SonarPractice_Rubberband PUBLIC "${rubberband_SOURCE_DIR}")
    target_compile_definitions(SonarPractice_Rubberband PRIVATE NOMINMAX)
    set_target_properties(SonarPractice_Rubberband PROPERTIES
        CXX_STANDARD 14
        CXX_STANDARD_REQUIRED ON
        POSITION_INDEPENDENT_CODE ON
    )
    if(MSVC)
        target_compile_options(SonarPractice_Rubberband PRIVATE /bigobj)
    endif()
endif()

if(NOT TARGET SonarPractice::Rubberband)
    add_library(SonarPractice::Rubberband ALIAS SonarPractice_Rubberband)
endif()
