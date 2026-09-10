# cmake/FindOpenSSL.cmake
# Prefer SonarPractice's generated OpenSSL CONFIG (Shining Light / Chocolatey on
# Windows) so nested projects like libgp_parser succeed under MSVC and MinGW.
# Falls back to CMake's stock FindOpenSSL module.

if(DEFINED OpenSSL_DIR AND EXISTS "${OpenSSL_DIR}/OpenSSLConfig.cmake")
    include("${OpenSSL_DIR}/OpenSSLConfig.cmake")
    include(FindPackageHandleStandardArgs)
    find_package_handle_standard_args(OpenSSL
        REQUIRED_VARS OPENSSL_CRYPTO_LIBRARY OPENSSL_INCLUDE_DIR
        VERSION_VAR OPENSSL_VERSION
    )
    return()
endif()

if(EXISTS "${CMAKE_BINARY_DIR}/sonarp_cmake/OpenSSL/OpenSSLConfig.cmake")
    set(OpenSSL_DIR "${CMAKE_BINARY_DIR}/sonarp_cmake/OpenSSL")
    include("${OpenSSL_DIR}/OpenSSLConfig.cmake")
    include(FindPackageHandleStandardArgs)
    find_package_handle_standard_args(OpenSSL
        REQUIRED_VARS OPENSSL_CRYPTO_LIBRARY OPENSSL_INCLUDE_DIR
        VERSION_VAR OPENSSL_VERSION
    )
    return()
endif()

# Stock CMake module (avoid recursion into this file).
set(_SONARP_STOCK_FIND_OPENSSL "${CMAKE_ROOT}/Modules/FindOpenSSL.cmake")
if(EXISTS "${_SONARP_STOCK_FIND_OPENSSL}")
    include("${_SONARP_STOCK_FIND_OPENSSL}")
else()
    message(FATAL_ERROR "CMake stock FindOpenSSL.cmake not found at ${_SONARP_STOCK_FIND_OPENSSL}")
endif()
