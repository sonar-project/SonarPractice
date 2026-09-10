# cmake/SonarPracticeOpenSSL.cmake
# Resolve OpenSSL for Windows (MinGW / LLVM-MinGW / MSVC), Linux, and macOS.
# Nested projects (libgp_parser) call find_package(OpenSSL REQUIRED); we provide
# a CONFIG package + a FindOpenSSL.cmake shim so that succeeds.

function(sonarp_clear_openssl_notfound)
    foreach(_sonarp_ssl_var IN ITEMS
        OPENSSL_ROOT_DIR OPENSSL_INCLUDE_DIR
        OPENSSL_CRYPTO_LIBRARY OPENSSL_SSL_LIBRARY
        OPENSSL_CRYPTO_LIBRARY_DEBUG OPENSSL_SSL_LIBRARY_DEBUG
        LIB_EAY SSL_EAY OpenSSL_DIR
    )
        if(DEFINED CACHE{${_sonarp_ssl_var}})
            if("${${_sonarp_ssl_var}}" MATCHES "-NOTFOUND$" OR "${${_sonarp_ssl_var}}" STREQUAL "")
                unset(${_sonarp_ssl_var} CACHE)
            endif()
        endif()
    endforeach()
    if(DEFINED CACHE{OpenSSL_DIR} AND NOT EXISTS "${OpenSSL_DIR}/OpenSSLConfig.cmake")
        unset(OpenSSL_DIR CACHE)
    endif()
endfunction()

function(sonarp_locate_openssl_root out_var)
    set(_sonarp_ssl_candidates
        "$ENV{OPENSSL_ROOT_DIR}"
        "$ENV{OPENSSL_ROOT}"
        "${OPENSSL_ROOT_DIR}"
        "$ENV{ProgramFiles}/OpenSSL-Win64"
        "$ENV{ProgramFiles}/OpenSSL"
        "C:/Program Files/OpenSSL-Win64"
        "C:/Program Files/OpenSSL"
        "C:/Program Files (x86)/OpenSSL-Win64"
        "C:/OpenSSL-Win64"
        # MSYS2 / Clang/MinGW sysroots (optional)
        "C:/msys64/mingw64"
        "C:/msys64/ucrt64"
        "C:/msys64/clang64"
        "$ENV{MSYSTEM_PREFIX}"
        "/opt/homebrew/opt/openssl@3"
        "/usr/local/opt/openssl@3"
        "/opt/homebrew/opt/openssl"
        "/usr/local/opt/openssl"
    )
    set(_sonarp_ssl_found "")
    foreach(_sonarp_ssl_root IN LISTS _sonarp_ssl_candidates)
        if(_sonarp_ssl_root AND EXISTS "${_sonarp_ssl_root}/include/openssl/ssl.h")
            set(_sonarp_ssl_found "${_sonarp_ssl_root}")
            break()
        endif()
    endforeach()
    set(${out_var} "${_sonarp_ssl_found}" PARENT_SCOPE)
endfunction()

function(sonarp_openssl_pick_msvc_libs root out_crypto out_ssl)
    set(_sonarp_ssl_runtimes MD MDd MT MTd)
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        set(_sonarp_ssl_runtimes MDd MD MTd MT)
    endif()
    set(_sonarp_crypto "")
    set(_sonarp_ssl "")
    foreach(_sonarp_rt IN LISTS _sonarp_ssl_runtimes)
        set(_sonarp_dir "${root}/lib/VC/x64/${_sonarp_rt}")
        if(EXISTS "${_sonarp_dir}/libcrypto.lib" AND EXISTS "${_sonarp_dir}/libssl.lib")
            set(_sonarp_crypto "${_sonarp_dir}/libcrypto.lib")
            set(_sonarp_ssl "${_sonarp_dir}/libssl.lib")
            break()
        endif()
    endforeach()
    if(NOT _sonarp_crypto)
        foreach(_sonarp_dir IN ITEMS "${root}/lib" "${root}/lib64")
            if(EXISTS "${_sonarp_dir}/libcrypto.dll.a")
                set(_sonarp_crypto "${_sonarp_dir}/libcrypto.dll.a")
            elseif(EXISTS "${_sonarp_dir}/libcrypto.lib")
                set(_sonarp_crypto "${_sonarp_dir}/libcrypto.lib")
            endif()
            if(EXISTS "${_sonarp_dir}/libssl.dll.a")
                set(_sonarp_ssl "${_sonarp_dir}/libssl.dll.a")
            elseif(EXISTS "${_sonarp_dir}/libssl.lib")
                set(_sonarp_ssl "${_sonarp_dir}/libssl.lib")
            endif()
        endforeach()
    endif()
    set(${out_crypto} "${_sonarp_crypto}" PARENT_SCOPE)
    set(${out_ssl} "${_sonarp_ssl}" PARENT_SCOPE)
endfunction()

# Build a GNU/LLVM import library (.dll.a) from a Windows DLL via gendef + dlltool.
function(sonarp_mingw_implib_from_dll dll out_implib)
    if(NOT EXISTS "${dll}")
        set(${out_implib} "" PARENT_SCOPE)
        return()
    endif()

    get_filename_component(_sonarp_cxx_bin "${CMAKE_CXX_COMPILER}" DIRECTORY)
    find_program(SONARP_GENDEF NAMES gendef gendef.exe
        HINTS "${_sonarp_cxx_bin}"
    )
    # Prefer llvm-dlltool when using Clang; never use --opt=VALUE (breaks llvm-dlltool).
    find_program(SONARP_DLLTOOL NAMES llvm-dlltool llvm-dlltool.exe dlltool dlltool.exe
        HINTS "${_sonarp_cxx_bin}"
    )
    if(NOT SONARP_GENDEF OR NOT SONARP_DLLTOOL)
        message(WARNING
            "gendef/dlltool not found next to ${CMAKE_CXX_COMPILER}. "
            "Cannot generate MinGW import libs for ${dll}.")
        set(${out_implib} "" PARENT_SCOPE)
        return()
    endif()

    get_filename_component(_sonarp_dll_name "${dll}" NAME)
    get_filename_component(_sonarp_dll_we "${dll}" NAME_WE)
    set(_sonarp_out_dir "${CMAKE_BINARY_DIR}/sonarp_openssl_implib")
    file(MAKE_DIRECTORY "${_sonarp_out_dir}")
    # Copy DLL into the build dir so gendef/dlltool never see "Program Files" paths.
    set(_sonarp_dll_local "${_sonarp_out_dir}/${_sonarp_dll_name}")
    if(NOT EXISTS "${_sonarp_dll_local}" OR "${dll}" IS_NEWER_THAN "${_sonarp_dll_local}")
        file(COPY "${dll}" DESTINATION "${_sonarp_out_dir}")
    endif()
    set(_sonarp_def "${_sonarp_out_dir}/${_sonarp_dll_we}.def")
    set(_sonarp_a "${_sonarp_out_dir}/${_sonarp_dll_we}.dll.a")

    if(NOT EXISTS "${_sonarp_a}" OR "${dll}" IS_NEWER_THAN "${_sonarp_a}")
        file(REMOVE "${_sonarp_def}")
        execute_process(
            COMMAND "${SONARP_GENDEF}" -a "${_sonarp_dll_name}"
            WORKING_DIRECTORY "${_sonarp_out_dir}"
            RESULT_VARIABLE _sonarp_gendef_rc
            OUTPUT_VARIABLE _sonarp_gendef_out
            ERROR_VARIABLE _sonarp_gendef_err
        )
        if(NOT EXISTS "${_sonarp_def}")
            file(GLOB _sonarp_defs "${_sonarp_out_dir}/${_sonarp_dll_we}*.def")
            if(_sonarp_defs)
                list(GET _sonarp_defs 0 _sonarp_def_found)
                file(RENAME "${_sonarp_def_found}" "${_sonarp_def}")
            endif()
        endif()
        # gendef often prints to stderr and may return non-zero even on success.
        if(NOT EXISTS "${_sonarp_def}")
            message(WARNING
                "gendef failed for ${dll} (rc=${_sonarp_gendef_rc}): ${_sonarp_gendef_err}")
            set(${out_implib} "" PARENT_SCOPE)
            return()
        endif()

        get_filename_component(_sonarp_dlltool_name "${SONARP_DLLTOOL}" NAME)
        set(_sonarp_dlltool_cmd "${SONARP_DLLTOOL}")
        if(_sonarp_dlltool_name MATCHES "llvm-dlltool")
            # Separate argv only — no --foo=bar (llvm-dlltool treats "=C:/..." as a path).
            execute_process(
                COMMAND "${_sonarp_dlltool_cmd}"
                        -m i386:x86-64
                        -d "${_sonarp_def}"
                        -l "${_sonarp_a}"
                        -D "${_sonarp_dll_name}"
                WORKING_DIRECTORY "${_sonarp_out_dir}"
                RESULT_VARIABLE _sonarp_dlltool_rc
                ERROR_VARIABLE _sonarp_dlltool_err
            )
        else()
            execute_process(
                COMMAND "${_sonarp_dlltool_cmd}"
                        -d "${_sonarp_def}"
                        -l "${_sonarp_a}"
                        -D "${_sonarp_dll_name}"
                WORKING_DIRECTORY "${_sonarp_out_dir}"
                RESULT_VARIABLE _sonarp_dlltool_rc
                ERROR_VARIABLE _sonarp_dlltool_err
            )
        endif()
        if(_sonarp_dlltool_rc GREATER 0 OR NOT EXISTS "${_sonarp_a}")
            message(WARNING
                "dlltool failed for ${dll} (rc=${_sonarp_dlltool_rc}): ${_sonarp_dlltool_err}\n"
                "  dlltool=${SONARP_DLLTOOL}\n"
                "  def=${_sonarp_def}")
            set(${out_implib} "" PARENT_SCOPE)
            return()
        endif()
        message(STATUS "Generated MinGW/LLVM import lib ${_sonarp_a}")
    endif()

    set(${out_implib} "${_sonarp_a}" PARENT_SCOPE)
endfunction()

function(sonarp_write_openssl_config
        root include_dir crypto_lib ssl_lib crypto_dll ssl_dll)
    set(_cfg_dir "${CMAKE_BINARY_DIR}/sonarp_cmake/OpenSSL")
    file(MAKE_DIRECTORY "${_cfg_dir}")

    # Embedded paths must use forward slashes — backslashes are CMake escapes.
    file(TO_CMAKE_PATH "${root}" root)
    file(TO_CMAKE_PATH "${include_dir}" include_dir)
    file(TO_CMAKE_PATH "${crypto_lib}" crypto_lib)
    file(TO_CMAKE_PATH "${ssl_lib}" ssl_lib)
    if(crypto_dll)
        file(TO_CMAKE_PATH "${crypto_dll}" crypto_dll)
    endif()
    if(ssl_dll)
        file(TO_CMAKE_PATH "${ssl_dll}" ssl_dll)
    endif()
    file(TO_CMAKE_PATH "${_cfg_dir}" _cfg_dir)

    # MSVC must never see a .dll as IMPORTED_LOCATION on an UNKNOWN target
    # (LNK1107). Use SHARED + IMPORTED_IMPLIB when a DLL is known; otherwise
    # UNKNOWN with the import/static library path only.
    set(_crypto_shared FALSE)
    set(_ssl_shared FALSE)
    set(_crypto_loc "${crypto_lib}")
    set(_ssl_loc "${ssl_lib}")
    set(_crypto_implib_line "")
    set(_ssl_implib_line "")
    if(crypto_dll AND EXISTS "${crypto_dll}")
        set(_crypto_shared TRUE)
        set(_crypto_loc "${crypto_dll}")
        set(_crypto_implib_line "IMPORTED_IMPLIB \"${crypto_lib}\"")
    endif()
    if(ssl_dll AND EXISTS "${ssl_dll}")
        set(_ssl_shared TRUE)
        set(_ssl_loc "${ssl_dll}")
        set(_ssl_implib_line "IMPORTED_IMPLIB \"${ssl_lib}\"")
    endif()
    if(_crypto_shared)
        set(_crypto_type "SHARED")
    else()
        set(_crypto_type "UNKNOWN")
    endif()
    if(_ssl_shared)
        set(_ssl_type "SHARED")
    else()
        set(_ssl_type "UNKNOWN")
    endif()

    file(WRITE "${_cfg_dir}/OpenSSLConfig.cmake" "
set(OPENSSL_FOUND TRUE)
set(OpenSSL_FOUND TRUE)
set(OPENSSL_VERSION \"windows-resolved\")
set(OPENSSL_INCLUDE_DIR \"${include_dir}\")
set(OPENSSL_INCLUDE_DIRS \"${include_dir}\")
set(OPENSSL_CRYPTO_LIBRARY \"${crypto_lib}\")
set(OPENSSL_SSL_LIBRARY \"${ssl_lib}\")
set(OPENSSL_LIBRARIES \"${ssl_lib};${crypto_lib}\")
set(OPENSSL_ROOT_DIR \"${root}\")

if(NOT TARGET OpenSSL::Crypto)
  add_library(OpenSSL::Crypto ${_crypto_type} IMPORTED GLOBAL)
  set_target_properties(OpenSSL::Crypto PROPERTIES
    IMPORTED_LOCATION \"${_crypto_loc}\"
    ${_crypto_implib_line}
    INTERFACE_INCLUDE_DIRECTORIES \"${include_dir}\"
  )
endif()
if(NOT TARGET OpenSSL::SSL)
  add_library(OpenSSL::SSL ${_ssl_type} IMPORTED GLOBAL)
  set_target_properties(OpenSSL::SSL PROPERTIES
    IMPORTED_LOCATION \"${_ssl_loc}\"
    ${_ssl_implib_line}
    INTERFACE_INCLUDE_DIRECTORIES \"${include_dir}\"
    INTERFACE_LINK_LIBRARIES OpenSSL::Crypto
  )
endif()
")
    file(WRITE "${_cfg_dir}/OpenSSLConfigVersion.cmake" "
set(PACKAGE_VERSION \"3.0.0\")
set(PACKAGE_VERSION_COMPATIBLE TRUE)
set(PACKAGE_VERSION_UNSUITABLE FALSE)
")
    set(OpenSSL_DIR "${_cfg_dir}" CACHE PATH "SonarPractice OpenSSL CONFIG dir" FORCE)
    list(PREPEND CMAKE_PREFIX_PATH "${CMAKE_BINARY_DIR}/sonarp_cmake")
    set(OpenSSL_DIR "${_cfg_dir}" PARENT_SCOPE)
endfunction()

# ---------------------------------------------------------------------------
sonarp_clear_openssl_notfound()
sonarp_locate_openssl_root(_SONARP_OPENSSL_ROOT)

set(_SONARP_CRYPTO "")
set(_SONARP_SSL "")
set(_SONARP_CRYPTO_DLL "")
set(_SONARP_SSL_DLL "")

# GNU/LLVM toolchains on Windows (MinGW, llvm-mingw) — not clang-cl/MSVC.
set(_SONARP_GNU_WIN FALSE)
if(WIN32 AND NOT MSVC)
    set(_SONARP_GNU_WIN TRUE)
endif()

if(_SONARP_OPENSSL_ROOT)
    set(OPENSSL_ROOT_DIR "${_SONARP_OPENSSL_ROOT}" CACHE PATH "OpenSSL root" FORCE)
    set(OPENSSL_INCLUDE_DIR "${_SONARP_OPENSSL_ROOT}/include" CACHE PATH "OpenSSL include directory" FORCE)

    file(GLOB _SONARP_CRYPTO_DLL_CANDIDATES
        "${_SONARP_OPENSSL_ROOT}/bin/libcrypto-*.dll"
        "${_SONARP_OPENSSL_ROOT}/libcrypto-*.dll")
    file(GLOB _SONARP_SSL_DLL_CANDIDATES
        "${_SONARP_OPENSSL_ROOT}/bin/libssl-*.dll"
        "${_SONARP_OPENSSL_ROOT}/libssl-*.dll")
    if(_SONARP_CRYPTO_DLL_CANDIDATES)
        list(GET _SONARP_CRYPTO_DLL_CANDIDATES 0 _SONARP_CRYPTO_DLL)
    endif()
    if(_SONARP_SSL_DLL_CANDIDATES)
        list(GET _SONARP_SSL_DLL_CANDIDATES 0 _SONARP_SSL_DLL)
    endif()

    if(_SONARP_GNU_WIN)
        # Prefer native MinGW import libs; otherwise generate them from the DLLs.
        if(EXISTS "${_SONARP_OPENSSL_ROOT}/lib/libcrypto.dll.a"
           AND EXISTS "${_SONARP_OPENSSL_ROOT}/lib/libssl.dll.a")
            set(_SONARP_CRYPTO "${_SONARP_OPENSSL_ROOT}/lib/libcrypto.dll.a")
            set(_SONARP_SSL "${_SONARP_OPENSSL_ROOT}/lib/libssl.dll.a")
        elseif(_SONARP_CRYPTO_DLL AND _SONARP_SSL_DLL)
            sonarp_mingw_implib_from_dll("${_SONARP_CRYPTO_DLL}" _SONARP_CRYPTO)
            sonarp_mingw_implib_from_dll("${_SONARP_SSL_DLL}" _SONARP_SSL)
        endif()
        if(_SONARP_CRYPTO AND _SONARP_SSL)
            message(STATUS "OpenSSL MinGW/LLVM import libs: ${_SONARP_CRYPTO}")
        endif()
    else()
        sonarp_openssl_pick_msvc_libs("${_SONARP_OPENSSL_ROOT}" _SONARP_CRYPTO _SONARP_SSL)
        if(_SONARP_CRYPTO AND _SONARP_SSL)
            message(STATUS "OpenSSL MSVC import libs: ${_SONARP_CRYPTO}")
        endif()
    endif()
else()
    message(STATUS "OpenSSL root not auto-detected (ProgramFiles=$ENV{ProgramFiles})")
endif()

if(WIN32 AND _SONARP_OPENSSL_ROOT AND _SONARP_CRYPTO AND _SONARP_SSL)
    set(OPENSSL_CRYPTO_LIBRARY "${_SONARP_CRYPTO}" CACHE FILEPATH "OpenSSL crypto library" FORCE)
    set(OPENSSL_SSL_LIBRARY "${_SONARP_SSL}" CACHE FILEPATH "OpenSSL ssl library" FORCE)
    sonarp_write_openssl_config(
        "${_SONARP_OPENSSL_ROOT}"
        "${OPENSSL_INCLUDE_DIR}"
        "${_SONARP_CRYPTO}"
        "${_SONARP_SSL}"
        "${_SONARP_CRYPTO_DLL}"
        "${_SONARP_SSL_DLL}"
    )
    find_package(OpenSSL QUIET)
    if(NOT OpenSSL_FOUND)
        set(OpenSSL_FOUND TRUE)
        set(OPENSSL_FOUND TRUE)
        set(OPENSSL_VERSION "windows-resolved")
    endif()
    message(STATUS "OpenSSL ready for nested find_package (OpenSSL_DIR=${OpenSSL_DIR})")
elseif(_SONARP_OPENSSL_ROOT)
    find_package(OpenSSL QUIET)
else()
    find_package(OpenSSL QUIET)
endif()

if(NOT OpenSSL_FOUND AND OPENSSL_INCLUDE_DIR AND OPENSSL_CRYPTO_LIBRARY
   AND EXISTS "${OPENSSL_CRYPTO_LIBRARY}" AND NOT "${OPENSSL_CRYPTO_LIBRARY}" MATCHES "-NOTFOUND$")
    if(NOT TARGET OpenSSL::Crypto)
        add_library(OpenSSL::Crypto UNKNOWN IMPORTED GLOBAL)
        set_target_properties(OpenSSL::Crypto PROPERTIES
            IMPORTED_LOCATION "${OPENSSL_CRYPTO_LIBRARY}"
            INTERFACE_INCLUDE_DIRECTORIES "${OPENSSL_INCLUDE_DIR}"
        )
    endif()
    if(OPENSSL_SSL_LIBRARY AND EXISTS "${OPENSSL_SSL_LIBRARY}"
       AND NOT "${OPENSSL_SSL_LIBRARY}" MATCHES "-NOTFOUND$" AND NOT TARGET OpenSSL::SSL)
        add_library(OpenSSL::SSL UNKNOWN IMPORTED GLOBAL)
        set_target_properties(OpenSSL::SSL PROPERTIES
            IMPORTED_LOCATION "${OPENSSL_SSL_LIBRARY}"
            INTERFACE_INCLUDE_DIRECTORIES "${OPENSSL_INCLUDE_DIR}"
            INTERFACE_LINK_LIBRARIES OpenSSL::Crypto
        )
    endif()
    set(OpenSSL_FOUND TRUE)
    set(OPENSSL_FOUND TRUE)
    if(NOT OPENSSL_VERSION)
        set(OPENSSL_VERSION "manual")
    endif()
endif()

if(OpenSSL_FOUND)
    message(STATUS "OpenSSL ${OPENSSL_VERSION} found (${OPENSSL_CRYPTO_LIBRARY})")
else()
    message(WARNING
        "OpenSSL not found yet. libgp_parser requires it.\n"
        "  Windows (MinGW / LLVM-MinGW): choco install openssl\n"
        "    CMake generates .dll.a import libs via gendef/dlltool automatically.\n"
        "  Or MSYS2: pacman -S mingw-w64-x86_64-openssl\n"
        "  Then delete CMakeCache.txt and reconfigure.\n"
        "  macOS: brew install openssl@3\n"
        "  Linux: libssl-dev / openssl-devel")
endif()
