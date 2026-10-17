# tests/libraryManager/CMakeLists.txt

find_package(Qt6 REQUIRED COMPONENTS Test)

# Test libraryManager
qt_add_executable(libmanagerTest
    tst_libmanagerTest.cpp
    tst_libmanagerTest.h
)

set_target_properties(libmanagerTest PROPERTIES
    MACOSX_BUNDLE FALSE
    WIN32_EXECUTABLE FALSE
)

target_link_libraries(libmanagerTest PRIVATE
    Qt6::Test
    LibraryManager
)

target_include_directories(libmanagerTest PRIVATE ${CMAKE_SOURCE_DIR}/tests)

enable_testing()
add_test(NAME LibraryManager COMMAND libmanagerTest)
