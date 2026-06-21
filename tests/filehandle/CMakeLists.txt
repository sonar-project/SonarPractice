# tests/filehandle/CMakeLists.txt

find_package(Qt6 REQUIRED COMPONENTS Test)

# Test Filehandle
qt_add_executable(filehandleTest
    tst_filehandleTest.cpp
    tst_filehandleTest.h
)

set_target_properties(filehandleTest PROPERTIES
    MACOSX_BUNDLE FALSE
    WIN32_EXECUTABLE FALSE
)

target_link_libraries(filehandleTest PRIVATE
    Qt6::Test
    filehandle
)

enable_testing()
add_test(NAME filehandle_test COMMAND filehandleTest)
