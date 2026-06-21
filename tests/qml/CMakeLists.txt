qt_add_executable(qmlTests
    main.cpp
    qmltestsetup.cpp
    qmltestsetup.h
)

set_target_properties(qmlTests PROPERTIES
    MACOSX_BUNDLE FALSE
    WIN32_EXECUTABLE FALSE
)

target_link_libraries(qmlTests
    PRIVATE
        Qt6::QuickTest
        Qt6::Quick
        Qt6::Qml
        ServicesLib
)

target_include_directories(qmlTests PRIVATE ${CMAKE_SOURCE_DIR}/tests)

add_dependencies(qmlTests SonarPractice)

target_compile_definitions(qmlTests
    PRIVATE
        SONAR_QML_SOURCE_DIR="${CMAKE_SOURCE_DIR}"
        QUICK_TEST_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
)

enable_testing()
add_test(NAME qmlTests COMMAND qmlTests)
set_tests_properties(qmlTests PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
)
