# FFmpeg (libavformat, libavutil) for container stream probing — optional on all platforms.
# install on Linux Mint apt install libvulkan-dev libavformat-dev libavutil-dev 

find_package(PkgConfig QUIET)

set(SONARPRACTICE_HAS_FFMPEG OFF)

if(PkgConfig_FOUND)
    pkg_check_modules(FFMPEG IMPORTED_TARGET GLOBAL libavformat libavutil)
    if(FFMPEG_FOUND)
        set(SONARPRACTICE_HAS_FFMPEG ON)
        if(NOT TARGET SonarPractice::FFmpeg)
            add_library(SonarPractice::FFmpeg ALIAS PkgConfig::FFMPEG)
        endif()
    endif()
endif()

if(NOT SONARPRACTICE_HAS_FFMPEG)
    message(STATUS "FFmpeg (libavformat) not found — media stream probe falls back to extension defaults")
endif()
