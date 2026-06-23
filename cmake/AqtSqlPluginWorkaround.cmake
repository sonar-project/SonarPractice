# Partial Qt installs (e.g. aqt on CI) ship Sql driver CMake package files without every
# plugin binary. Remove configs only when the referenced plugin library is missing.

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
        if(NOT IS_DIRECTORY "${_prefix}/lib/cmake/Qt6Sql")
            continue()
        endif()

        set(
            _optional_drivers
            "QMYSQL|libqsqlmysql"
            "QPSQL|libqsqlpsql"
            "QOCI|libqsqloci"
            "QMIMER|libqsqlmimer"
            "IBASE|libqsqlibase"
        )

        foreach(_pair IN LISTS _optional_drivers)
            string(REPLACE "|" ";" _pair_list "${_pair}")
            list(GET _pair_list 0 _cmake_name)
            list(GET _pair_list 1 _library_base)

            set(_plugin_path "${_prefix}/plugins/sqldrivers/${_library_base}${_plugin_ext}")
            if(EXISTS "${_plugin_path}")
                continue()
            endif()

            file(GLOB _configs "${_prefix}/lib/cmake/Qt6Sql/Qt6${_cmake_name}DriverPlugin*.cmake")
            foreach(_cfg IN LISTS _configs)
                file(REMOVE "${_cfg}")
            endforeach()
        endforeach()

        return()
    endforeach()
endfunction()

sonarp_remove_missing_qt_sql_driver_cmake()
