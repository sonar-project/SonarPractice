# FFmpeg (libavformat, libavutil) for container stream probing — optional on all platforms.
# Linux: apt install libavformat-dev libavutil-dev
# macOS: brew install ffmpeg
# Windows: optional; set FFMPEG_ROOT to an FFmpeg SDK, or rely on extension-based probe.

find_package(PkgConfig QUIET)

set(SONARPRACTICE_HAS_FFMPEG OFF)

if(PkgConfig_FOUND)
    pkg_check_modules(FFMPEG IMPORTED_TARGET GLOBAL libavformat libavutil)
    if(FFMPEG_FOUND)
        set(SONARPRACTICE_HAS_FFMPEG ON)
        if(NOT TARGET SonarPractice::FFmpeg)
            add_library(SonarPractice::FFmpeg ALIAS PkgConfig::FFMPEG)
        endif()
        message(STATUS "FFmpeg found via pkg-config")
    endif()
endif()

if(NOT SONARPRACTICE_HAS_FFMPEG)
    set(FFMPEG_ROOT "" CACHE PATH "FFmpeg install prefix (include/ + lib/)")
    set(_sonarp_ffmpeg_roots)
    if(FFMPEG_ROOT)
        list(APPEND _sonarp_ffmpeg_roots "${FFMPEG_ROOT}")
    endif()
    if(DEFINED ENV{FFMPEG_ROOT})
        list(APPEND _sonarp_ffmpeg_roots "$ENV{FFMPEG_ROOT}")
    endif()
    if(APPLE)
        list(APPEND _sonarp_ffmpeg_roots
            "/opt/homebrew"
            "/usr/local"
        )
    endif()

    find_path(FFMPEG_AVFORMAT_INCLUDE_DIR
        NAMES libavformat/avformat.h
        HINTS ${_sonarp_ffmpeg_roots}
        PATH_SUFFIXES include
    )
    find_library(FFMPEG_AVFORMAT_LIBRARY
        NAMES avformat
        HINTS ${_sonarp_ffmpeg_roots}
        PATH_SUFFIXES lib lib64
    )
    find_library(FFMPEG_AVUTIL_LIBRARY
        NAMES avutil
        HINTS ${_sonarp_ffmpeg_roots}
        PATH_SUFFIXES lib lib64
    )

    if(FFMPEG_AVFORMAT_INCLUDE_DIR AND FFMPEG_AVFORMAT_LIBRARY AND FFMPEG_AVUTIL_LIBRARY)
        set(SONARPRACTICE_HAS_FFMPEG ON)
        if(NOT TARGET SonarPractice::FFmpeg)
            add_library(SonarPractice::FFmpeg INTERFACE IMPORTED GLOBAL)
            target_include_directories(SonarPractice::FFmpeg INTERFACE
                "${FFMPEG_AVFORMAT_INCLUDE_DIR}")
            target_link_libraries(SonarPractice::FFmpeg INTERFACE
                "${FFMPEG_AVFORMAT_LIBRARY}"
                "${FFMPEG_AVUTIL_LIBRARY}")
        endif()
        message(STATUS "FFmpeg found via find_library (${FFMPEG_AVFORMAT_LIBRARY})")
    endif()
endif()

if(NOT SONARPRACTICE_HAS_FFMPEG)
    message(STATUS "FFmpeg (libavformat) not found — media stream probe falls back to extension defaults")
endif()
