# src/core/libraryManager/CMakeLists.txt

add_library(LibraryManager STATIC
    LibraryManager.cpp LibraryManager.h
    instrument.cpp instrument.h
)

target_include_directories(LibraryManager
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(LibraryManager
    PRIVATE
        libgp_parser::libgp_parser
        Qt6::Core
)

target_compile_features(LibraryManager PUBLIC cxx_std_23)
