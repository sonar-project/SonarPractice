# src/core/config/CMakeLists.txt

find_package(Qt6 REQUIRED COMPONENTS Core)

add_library(CfgLib STATIC
  interfaces/iConfigProvider.h
)

target_link_libraries(CfgLib PUBLIC Qt6::Core)

target_include_directories(CfgLib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})