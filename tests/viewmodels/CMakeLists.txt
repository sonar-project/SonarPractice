find_package(Qt6 REQUIRED COMPONENTS Test)

qt_add_executable(viewModelsTest
    tst_viewModelsTest.cpp
    tst_viewModelsTest.h
)

set_target_properties(viewModelsTest PROPERTIES
    MACOSX_BUNDLE FALSE
    WIN32_EXECUTABLE FALSE
)

target_link_libraries(viewModelsTest
    PRIVATE
        ServicesLib
        ViewModelsLib
        Qt6::Test
)

target_include_directories(viewModelsTest PRIVATE ${CMAKE_SOURCE_DIR}/tests)

enable_testing()
add_test(NAME viewModelsTest COMMAND viewModelsTest)

set_tests_properties(viewModelsTest PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
)
