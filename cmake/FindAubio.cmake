# Aubio: bundled static build via FetchContent (cross-platform, no optional I/O deps).

include(FetchContent)

if(NOT CMAKE_C_COMPILER_LOADED)
    enable_language(C)
endif()

fetchcontent_declare(
    aubio
    GIT_REPOSITORY https://github.com/aubio/aubio.git
    GIT_TAG 0.4.9
    GIT_SHALLOW TRUE
)

if(NOT TARGET SonarPractice_Aubio)
    fetchcontent_getproperties(aubio)
    if(NOT aubio_POPULATED)
        fetchcontent_populate(aubio)
    endif()

    set(_aubio_sources
        "${aubio_SOURCE_DIR}/src/cvec.c"
        "${aubio_SOURCE_DIR}/src/fmat.c"
        "${aubio_SOURCE_DIR}/src/fvec.c"
        "${aubio_SOURCE_DIR}/src/lvec.c"
        "${aubio_SOURCE_DIR}/src/mathutils.c"
        "${aubio_SOURCE_DIR}/src/musicutils.c"
        "${aubio_SOURCE_DIR}/src/vecutils.c"
        "${aubio_SOURCE_DIR}/src/notes/notes.c"
        "${aubio_SOURCE_DIR}/src/onset/onset.c"
        "${aubio_SOURCE_DIR}/src/onset/peakpicker.c"
        "${aubio_SOURCE_DIR}/src/pitch/pitch.c"
        "${aubio_SOURCE_DIR}/src/pitch/pitchfcomb.c"
        "${aubio_SOURCE_DIR}/src/pitch/pitchmcomb.c"
        "${aubio_SOURCE_DIR}/src/pitch/pitchschmitt.c"
        "${aubio_SOURCE_DIR}/src/pitch/pitchspecacf.c"
        "${aubio_SOURCE_DIR}/src/pitch/pitchyin.c"
        "${aubio_SOURCE_DIR}/src/pitch/pitchyinfast.c"
        "${aubio_SOURCE_DIR}/src/pitch/pitchyinfft.c"
        "${aubio_SOURCE_DIR}/src/spectral/awhitening.c"
        "${aubio_SOURCE_DIR}/src/spectral/dct.c"
        "${aubio_SOURCE_DIR}/src/spectral/dct_ooura.c"
        "${aubio_SOURCE_DIR}/src/spectral/dct_plain.c"
        "${aubio_SOURCE_DIR}/src/spectral/fft.c"
        "${aubio_SOURCE_DIR}/src/spectral/filterbank.c"
        "${aubio_SOURCE_DIR}/src/spectral/filterbank_mel.c"
        "${aubio_SOURCE_DIR}/src/spectral/mfcc.c"
        "${aubio_SOURCE_DIR}/src/spectral/ooura_fft8g.c"
        "${aubio_SOURCE_DIR}/src/spectral/phasevoc.c"
        "${aubio_SOURCE_DIR}/src/spectral/specdesc.c"
        "${aubio_SOURCE_DIR}/src/spectral/statistics.c"
        "${aubio_SOURCE_DIR}/src/spectral/tss.c"
        "${aubio_SOURCE_DIR}/src/temporal/a_weighting.c"
        "${aubio_SOURCE_DIR}/src/temporal/biquad.c"
        "${aubio_SOURCE_DIR}/src/temporal/c_weighting.c"
        "${aubio_SOURCE_DIR}/src/temporal/filter.c"
        "${aubio_SOURCE_DIR}/src/utils/hist.c"
        "${aubio_SOURCE_DIR}/src/utils/log.c"
        "${aubio_SOURCE_DIR}/src/utils/parameter.c"
        "${aubio_SOURCE_DIR}/src/utils/scale.c"
        "${aubio_SOURCE_DIR}/src/utils/windll.c"
    )

    add_library(SonarPractice_Aubio STATIC ${_aubio_sources})
    target_include_directories(SonarPractice_Aubio
        PUBLIC
            "${aubio_SOURCE_DIR}/src"
        PRIVATE
            "${CMAKE_CURRENT_LIST_DIR}/aubio_config"
    )
    target_compile_definitions(SonarPractice_Aubio PRIVATE HAVE_CONFIG_H)
    set_target_properties(SonarPractice_Aubio PROPERTIES
        C_STANDARD 99
        C_STANDARD_REQUIRED ON
        POSITION_INDEPENDENT_CODE ON
    )
    if(UNIX AND NOT APPLE)
        target_link_libraries(SonarPractice_Aubio PUBLIC m)
    endif()
    if(MSVC)
        target_compile_definitions(SonarPractice_Aubio PRIVATE _USE_MATH_DEFINES)
    endif()
endif()

if(NOT TARGET SonarPractice::Aubio)
    add_library(SonarPractice::Aubio ALIAS SonarPractice_Aubio)
endif()
