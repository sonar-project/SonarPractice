# src/core/filehandle/CMakeLists.txt

find_package(Qt6 REQUIRED COMPONENTS Core)

add_library(filehandle STATIC fnv1a.h)

target_link_libraries(filehandle PUBLIC Qt6::Core)

target_include_directories(filehandle PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
