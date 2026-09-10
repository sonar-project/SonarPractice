# Rubber Band: prefer system library (pkg-config); FetchContent fallback for Windows/CI/macOS.

find_package(PkgConfig QUIET)

if(NOT TARGET SonarPractice::Rubberband)
    if(PkgConfig_FOUND)
        pkg_check_modules(RUBBERBAND IMPORTED_TARGET GLOBAL rubberband)
    endif()

    if(RUBBERBAND_FOUND)
        # Distro packages should ship a shared librubberband; reject static-only installs.
        set(_sonarp_rubberband_is_static FALSE)
        set(_sonarp_rubberband_has_shared FALSE)
        foreach(_lib IN LISTS RUBBERBAND_LIBRARIES RUBBERBAND_LINK_LIBRARIES)
            if(_lib MATCHES "\\.(a|lib)$")
                set(_sonarp_rubberband_is_static TRUE)
            endif()
            if(_lib MATCHES "\\.(so|dylib|dll)(\\.[0-9]+)*$")
                set(_sonarp_rubberband_has_shared TRUE)
            endif()
        endforeach()
        if(_sonarp_rubberband_is_static AND NOT _sonarp_rubberband_has_shared
           AND NOT RUBBERBAND_LIBRARIES MATCHES "\\.(so|dylib|dll)")
            message(FATAL_ERROR
                "Rubber Band must be linked dynamically (found static-only via pkg-config). "
                "Install the shared library (e.g. media-libs/rubberband on Gentoo, "
                "brew install rubberband on macOS).")
        endif()
        add_library(SonarPractice::Rubberband ALIAS PkgConfig::RUBBERBAND)
        message(STATUS "Using system Rubber Band ${RUBBERBAND_VERSION}")
    else()
        message(STATUS "Rubber Band not found via pkg-config — FetchContent v4.0.0")
        include(FetchContent)
        fetchcontent_declare(
            rubberband
            GIT_REPOSITORY https://github.com/breakfastquay/rubberband.git
            GIT_TAG v4.0.0
            GIT_SHALLOW TRUE
        )
        if(NOT TARGET SonarPractice_Rubberband)
            # Upstream is Meson-only (no top-level CMakeLists.txt). MakeAvailable
            # still populates the sources; add_subdirectory is skipped when absent.
            fetchcontent_makeavailable(rubberband)
            add_library(SonarPractice_Rubberband SHARED
                "${rubberband_SOURCE_DIR}/single/RubberBandSingle.cpp"
            )
            target_include_directories(SonarPractice_Rubberband PUBLIC
                "${rubberband_SOURCE_DIR}"
            )
            target_compile_definitions(SonarPractice_Rubberband PRIVATE NOMINMAX)
            set_target_properties(SonarPractice_Rubberband PROPERTIES
                CXX_STANDARD 14
                CXX_STANDARD_REQUIRED ON
                POSITION_INDEPENDENT_CODE ON
            )
            # Windows: ship as SonarPractice_Rubberband.dll (no lib- prefix).
            if(WIN32)
                set_target_properties(SonarPractice_Rubberband PROPERTIES PREFIX "")
            endif()
            if(MSVC)
                target_compile_options(SonarPractice_Rubberband PRIVATE /bigobj)
            endif()
        endif()
        add_library(SonarPractice::Rubberband ALIAS SonarPractice_Rubberband)
    endif()
endif()
