# tests/database/CMakeLists.txt

find_package(Qt6 REQUIRED COMPONENTS Test)

set(DATABASE_TEST_FIXTURE_SOURCES DatabaseTestFixture.cpp DatabaseTestFixture.h)

function(add_database_test target_name)
    qt_add_executable(${target_name} ${ARGN} ${DATABASE_TEST_FIXTURE_SOURCES})

    set_target_properties(${target_name} PROPERTIES
        MACOSX_BUNDLE FALSE
        WIN32_EXECUTABLE FALSE
    )

    target_link_libraries(${target_name}
        PRIVATE
            DatabaseMangerLib
            Qt6::Test
    )

    target_include_directories(${target_name} PRIVATE
        ${CMAKE_SOURCE_DIR}/tests
        ${CMAKE_CURRENT_SOURCE_DIR}
    )

    add_test(NAME ${target_name} COMMAND ${target_name})
endfunction()

add_database_test(connectionSchemaTest
    tst_connectionSchemaTest.cpp
    tst_connectionSchemaTest.h
)

add_database_test(userRepositoryTest
    tst_userRepositoryTest.cpp
    tst_userRepositoryTest.h
)

add_database_test(artistRepositoryTest
    tst_artistRepositoryTest.cpp
    tst_artistRepositoryTest.h
)

add_database_test(tuningRepositoryTest
    tst_tuningRepositoryTest.cpp
    tst_tuningRepositoryTest.h
)

add_database_test(songRepositoryTest
    tst_songRepositoryTest.cpp
    tst_songRepositoryTest.h
)

add_database_test(mediaFileRepositoryTest
    tst_mediaFileRepositoryTest.cpp
    tst_mediaFileRepositoryTest.h
)

add_database_test(practiceJournalTest
    tst_practiceJournalTest.cpp
    tst_practiceJournalTest.h
)

add_database_test(reminderRepoTest
    tst_reminderRepositoryTest.cpp
    tst_reminderRepositoryTest.h
)

add_database_test(audioPresetRepoTest
    tst_audioPresetRepositoryTest.cpp
    tst_audioPresetRepositoryTest.h
)

add_database_test(practiceAssetRepositoryTest
    tst_practiceAssetRepositoryTest.cpp
    tst_practiceAssetRepositoryTest.h
)

enable_testing()
