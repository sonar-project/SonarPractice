cmake_minimum_required(VERSION 3.20)

project(SonarPractice VERSION 0.0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(cmake/Dependencies.cmake)

qt_standard_project_setup(REQUIRES 6.11)

# QML-Typen generieren
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(BUILD_TESTING OFF CACHE BOOL "Disable Catch2 tests globally" FORCE)
set(CATCH_BUILD_TESTING OFF CACHE BOOL "Disable Catch2 internal tests" FORCE)
set(CATCH_INSTALL_DOCS OFF CACHE BOOL "Disable Catch2 docs" FORCE)

set(LIBGP_PARSER_BUILD_EXAMPLE OFF CACHE BOOL "Disable libgp_parser Example" FORCE)
set(LIBGP_PARSER_BUILD_TESTS OFF CACHE BOOL "Disable libgp_parser tests" FORCE)

add_compile_definitions(APP_VERSION="${PROJECT_VERSION}")
add_compile_definitions(COMPILER_INFO="${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
string(TIMESTAMP BUILD_DATE "%Y-%m-%d %H:%M:%S")
add_compile_definitions(BUILD_DATE="${BUILD_DATE}")

enable_testing()

add_subdirectory(src)

find_package(Qt6 REQUIRED COMPONENTS Quick Core Multimedia Sql Widgets Svg)
find_package(Qt6 REQUIRED COMPONENTS LinguistTools)

file(GLOB SONARPRACTICE_TS_FILES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/SonarPractice_*.ts")

include(cmake/GenerateTranslationCatalog.cmake)
set(SONARPRACTICE_TRANSLATION_CATALOG_HEADER
    "${CMAKE_CURRENT_BINARY_DIR}/generated/TranslationCatalog.generated.h")
sonarp_generate_translation_catalog(
    TS_FILES ${SONARPRACTICE_TS_FILES}
    BASE_NAME SonarPractice
    SOURCE_LANGUAGE en
    OUTPUT_HEADER "${SONARPRACTICE_TRANSLATION_CATALOG_HEADER}"
)

set_source_files_properties(src/ui/Theme.qml PROPERTIES QT_QML_SINGLETON_TYPE TRUE)

qt_add_executable(SonarPractice
    main.cpp
    src/app/ApplicationBootstrap.cpp src/app/ApplicationBootstrap.h
    src/app/StartupController.cpp src/app/StartupController.h
    src/app/StorageDirectoryPicker.cpp src/app/StorageDirectoryPicker.h
    src/app/TranslationManager.cpp src/app/TranslationManager.h
    src/app/RepositoryFactory.h
)

qt_add_translations(SonarPractice TS_FILES ${SONARPRACTICE_TS_FILES})

target_sources(SonarPractice PRIVATE ${SonarPractice})
target_include_directories(SonarPractice PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/generated")

if(WIN32)
    configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/app.rc.in"
        "${CMAKE_CURRENT_BINARY_DIR}/app.rc"
        @ONLY
    )
target_sources(SonarPractice PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/app.rc")
endif()

include(src/viewmodels/ViewModelQmlSources.cmake)

qt_add_qml_module(SonarPractice
    URI com.sonarp.sonarpractice
    SOURCES
    ${VIEWMODEL_QML_SOURCES}
    VERSION 1.0
    QML_FILES
    src/ui/Main.qml
    src/ui/Theme.qml

    src/ui/components/AboutDialog.qml
    src/ui/components/AssetTile.qml
    src/ui/components/AudioWaveformView.qml
    src/ui/components/AudioProcessingOverlay.qml
    src/ui/components/AudioPresetIcon.qml
    src/ui/components/DarkSpinBox.qml
    src/ui/components/DarkTextField.qml
    src/ui/components/DashboardExerciseList.qml
    src/ui/components/DashboardReminderPanel.qml
    src/ui/components/ImportProgressOverlay.qml
    src/ui/components/JournalMarkdownEditor.qml
    src/ui/components/JournalTableView.qml
    src/ui/components/LibraryBusyOverlay.qml
    src/ui/components/LibraryFolderTree.qml
    src/ui/components/MediaKindIcon.qml
    src/ui/components/PracticeCalendarPanel.qml
    src/ui/components/ResizableSidePanel.qml
    src/ui/components/SongCard.qml
    src/ui/components/SongInfoPanel.qml
    src/ui/components/SongReminderPanel.qml
    src/ui/components/StartupOverlay.qml
    src/ui/components/TopBar.qml
    src/ui/components/TrainingPanel.qml
    src/ui/components/TransientNoticePopup.qml

    src/ui/pages/AudioConfigPage.qml
    src/ui/pages/ConfigPage.qml
    src/ui/pages/DashboardPage.qml
    src/ui/pages/ImportSheet.qml
    src/ui/pages/LibraryPage.qml
    src/ui/pages/PracticeHubPage.qml
    RESOURCES
    assets/png/audio.png
    assets/png/doc.png
    assets/png/gp.png
    assets/png/unlinked.png
    assets/png/video.png

    assets/svg/icon.svg
    assets/svg/brackets.svg
    assets/svg/bold.svg
    assets/svg/minus.svg
    assets/svg/heading-1.svg
    assets/svg/heading-2.svg
    assets/svg/heading-3.svg

    src/ui/js/MediaKind.js
    src/ui/js/MarkdownFormat.js
    sonarpractice.desktop
    RESOURCES assets/app.rc.in
)

# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1.
# If you are developing for iOS or macOS you should consider setting an
# explicit, fixed bundle identifier manually though.
set_target_properties(SonarPractice PROPERTIES
    #    MACOSX_BUNDLE_GUI_IDENTIFIER com.sonarp.sonarpractice
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

target_include_directories(SonarPractice PRIVATE
    ${CMAKE_SOURCE_DIR}/src/app
    ${CMAKE_SOURCE_DIR}/src/viewmodels
)

target_compile_definitions(SonarPractice PRIVATE QT_QML_DEBUG_ENABLED)

target_link_libraries(SonarPractice
    PRIVATE
    Qt6::Widgets
    Qt6::Quick
    Qt6::QuickControls2
    Qt6::QuickDialogs2
    Qt6::Qml
    Qt6::Multimedia
    Qt6::Svg
    DatabaseMangerLib
    AudioLib
    ServicesLib
)

install(TARGETS SonarPractice
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

add_subdirectory(tests)
