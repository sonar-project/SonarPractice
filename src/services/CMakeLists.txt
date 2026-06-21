# src/services/CMakeLists.txt

find_package(Qt6 REQUIRED COMPONENTS Core Gui Concurrent Qml)

qt_add_library(ServicesLib STATIC
    config/AppSettings.h
    config/AppSettings.cpp
    config/SettingsConfigProvider.h
    config/SettingsConfigProvider.cpp
    import/ImportTypes.h
    import/ManagedFileStorage.h
    import/ManagedFileStorage.cpp
    import/FileImportUtils.h
    import/FileImportUtils.cpp
    import/IImportService.h
    import/ImportService.h
    import/ImportService.cpp
    media/MediaStreamProbe.h
    media/MediaStreamProbe.cpp
    path/IPathResolver.h
    path/PathResolver.h
    path/PathResolver.cpp
    launcher/ILauncher.h
    launcher/DesktopLauncher.h
    launcher/DesktopLauncher.cpp
    logging/ApplicationErrorLog.h
    logging/ApplicationErrorLog.cpp
    practice/PracticeTrackerService.h
    practice/PracticeTrackerService.cpp
    link/LinkGroupService.h
    link/LinkGroupService.cpp
)

add_subdirectory(audio)

target_link_libraries(ServicesLib
    PUBLIC
        DatabaseMangerLib
        LibraryManager
        filehandle
        CfgLib
        AudioLib
        Qt6::Core
        Qt6::Gui
        Qt6::Concurrent
        Qt6::Qml
)

if(SONARPRACTICE_HAS_FFMPEG)
    target_compile_definitions(ServicesLib PUBLIC SONARPRACTICE_HAS_FFMPEG)
    target_link_libraries(ServicesLib PUBLIC SonarPractice::FFmpeg)
endif()

target_include_directories(ServicesLib
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/import
        ${CMAKE_CURRENT_SOURCE_DIR}/media
        ${CMAKE_CURRENT_SOURCE_DIR}/config
        ${CMAKE_CURRENT_SOURCE_DIR}/path
        ${CMAKE_CURRENT_SOURCE_DIR}/launcher
        ${CMAKE_CURRENT_SOURCE_DIR}/logging
        ${CMAKE_CURRENT_SOURCE_DIR}/practice
        ${CMAKE_CURRENT_SOURCE_DIR}/link
)
