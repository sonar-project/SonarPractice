# tests/services/CMakeLists.txt

find_package(Qt6 REQUIRED COMPONENTS Test)

qt_add_executable(importServiceTest
    tst_importServiceTest.cpp
    tst_importServiceTest.h
)

set_target_properties(importServiceTest PROPERTIES
    MACOSX_BUNDLE FALSE
    WIN32_EXECUTABLE FALSE
)

target_compile_definitions(importServiceTest
    PRIVATE
        TESTDATA_DIR="${CMAKE_SOURCE_DIR}/testdata"
)

target_link_libraries(importServiceTest
    PRIVATE
        ServicesLib
        Qt6::Test
)

qt_add_executable(pathResolverTest
    tst_pathResolverTest.cpp
    tst_pathResolverTest.h
)

set_target_properties(pathResolverTest PROPERTIES
    MACOSX_BUNDLE FALSE
    WIN32_EXECUTABLE FALSE
)

target_link_libraries(pathResolverTest
    PRIVATE
        ServicesLib
        Qt6::Test
)

enable_testing()
add_test(NAME importServiceTest COMMAND importServiceTest)
add_test(NAME pathResolverTest COMMAND pathResolverTest)

qt_add_executable(applicationErrorLogTest
    tst_applicationErrorLogTest.cpp
    tst_applicationErrorLogTest.h
)

set_target_properties(applicationErrorLogTest PROPERTIES
    MACOSX_BUNDLE FALSE
    WIN32_EXECUTABLE FALSE
)

target_link_libraries(applicationErrorLogTest
    PRIVATE
        ServicesLib
        Qt6::Test
)

add_test(NAME applicationErrorLogTest COMMAND applicationErrorLogTest)

qt_add_executable(practiceTrackerTest
    tst_practiceTrackerTest.cpp
    tst_practiceTrackerTest.h
)

set_target_properties(practiceTrackerTest PROPERTIES
    MACOSX_BUNDLE FALSE
    WIN32_EXECUTABLE FALSE
)

target_link_libraries(practiceTrackerTest
    PRIVATE
        ServicesLib
        Qt6::Test
)

target_include_directories(practiceTrackerTest PRIVATE ${CMAKE_SOURCE_DIR}/tests)

add_test(NAME practiceTrackerTest COMMAND practiceTrackerTest)

qt_add_executable(appSettingsTest
    tst_appSettingsTest.cpp
    tst_appSettingsTest.h
)

set_target_properties(appSettingsTest PROPERTIES
    MACOSX_BUNDLE FALSE
    WIN32_EXECUTABLE FALSE
)

target_link_libraries(appSettingsTest
    PRIVATE
        ServicesLib
        Qt6::Test
)

add_test(NAME appSettingsTest COMMAND appSettingsTest)

qt_add_executable(managedFileStorageTest
    tst_managedFileStorageTest.cpp
    tst_managedFileStorageTest.h
)

set_target_properties(managedFileStorageTest PROPERTIES
    MACOSX_BUNDLE FALSE
    WIN32_EXECUTABLE FALSE
)

target_link_libraries(managedFileStorageTest
    PRIVATE
        ServicesLib
        Qt6::Test
)

add_test(NAME managedFileStorageTest COMMAND managedFileStorageTest)

qt_add_executable(linkGroupServiceTest
    tst_linkGroupServiceTest.cpp
    tst_linkGroupServiceTest.h
)

set_target_properties(linkGroupServiceTest PROPERTIES
    MACOSX_BUNDLE FALSE
    WIN32_EXECUTABLE FALSE
)

target_link_libraries(linkGroupServiceTest
    PRIVATE
        ServicesLib
        Qt6::Test
)

add_test(NAME linkGroupServiceTest COMMAND linkGroupServiceTest)

qt_add_executable(startupControllerTest
    tst_startupControllerTest.cpp
    tst_startupControllerTest.h
    ${CMAKE_SOURCE_DIR}/src/app/ApplicationBootstrap.cpp
    ${CMAKE_SOURCE_DIR}/src/app/StartupController.cpp
    ${CMAKE_SOURCE_DIR}/src/app/StorageDirectoryPicker.cpp
)

set_target_properties(startupControllerTest PROPERTIES
    MACOSX_BUNDLE FALSE
    WIN32_EXECUTABLE FALSE
)

target_include_directories(startupControllerTest PRIVATE
    ${CMAKE_SOURCE_DIR}/src/app
)

target_link_libraries(startupControllerTest
    PRIVATE
        ViewModelsLib
        ServicesLib
        Qt6::Test
        Qt6::Qml
        Qt6::Widgets
)

add_test(NAME startupControllerTest COMMAND startupControllerTest)

qt_add_executable(mediaStreamClassificationTest
    tst_mediaStreamClassificationTest.cpp
    tst_mediaStreamClassificationTest.h
)

set_target_properties(mediaStreamClassificationTest PROPERTIES
    MACOSX_BUNDLE FALSE
    WIN32_EXECUTABLE FALSE
)

target_link_libraries(mediaStreamClassificationTest
    PRIVATE
        ServicesLib
        Qt6::Test
)

add_test(NAME mediaStreamClassificationTest COMMAND mediaStreamClassificationTest)

set_tests_properties(importServiceTest pathResolverTest applicationErrorLogTest practiceTrackerTest appSettingsTest managedFileStorageTest linkGroupServiceTest startupControllerTest mediaStreamClassificationTest PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
)
