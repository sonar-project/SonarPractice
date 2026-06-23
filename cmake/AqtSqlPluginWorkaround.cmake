# Partial Qt installs (e.g. aqt on CI) ship Sql driver CMake package files without every
# plugin binary. When only SQLite is present, drop the other driver CMake configs.

function(sonarp_remove_missing_qt_sql_driver_cmake)
    set(_prefix_candidates)
    if(DEFINED ENV{QT_ROOT_DIR})
        list(APPEND _prefix_candidates "$ENV{QT_ROOT_DIR}")
    endif()
    if(DEFINED CMAKE_PREFIX_PATH)
        foreach(_entry IN LISTS CMAKE_PREFIX_PATH)
            list(APPEND _prefix_candidates "${_entry}")
        endforeach()
    endif()

    if(WIN32)
        set(_plugin_ext ".dll")
    else()
        set(_plugin_ext ".so")
    endif()

    foreach(_prefix IN LISTS _prefix_candidates)
        set(_sql_cmake_dir "${_prefix}/lib/cmake/Qt6Sql")
        set(_drivers_dir "${_prefix}/plugins/sqldrivers")
        if(NOT IS_DIRECTORY "${_sql_cmake_dir}" OR NOT IS_DIRECTORY "${_drivers_dir}")
            continue()
        endif()

        set(_sqlite_plugin "${_drivers_dir}/libqsqlite${_plugin_ext}")
        set(_mysql_plugin "${_drivers_dir}/libqsqlmysql${_plugin_ext}")
        if(NOT EXISTS "${_sqlite_plugin}" OR EXISTS "${_mysql_plugin}")
            return()
        endif()

        # SQLite-only layout (typical aqt qtbase): remove stale CMake for other drivers.
        file(GLOB _plugin_cmake_files "${_sql_cmake_dir}/Qt6Q*DriverPlugin*.cmake")
        foreach(_cfg IN LISTS _plugin_cmake_files)
            if(_cfg MATCHES "QSQLite")
                continue()
            endif()
            file(REMOVE "${_cfg}")
        endforeach()

        return()
    endforeach()
endfunction()

sonarp_remove_missing_qt_sql_driver_cmake()
