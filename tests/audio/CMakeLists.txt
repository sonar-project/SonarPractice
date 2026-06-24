# tests/audio/CMakeLists.txt

find_package(Qt6 REQUIRED COMPONENTS Test Multimedia)

function(add_audio_test target_name)
    qt_add_executable(${target_name} ${ARGN})

    set_target_properties(${target_name} PROPERTIES
        MACOSX_BUNDLE FALSE
        WIN32_EXECUTABLE FALSE
    )

    target_link_libraries(${target_name}
        PRIVATE
            AudioLib
            Qt6::Test
    )

    target_include_directories(${target_name} PRIVATE
        ${CMAKE_SOURCE_DIR}/tests
        ${CMAKE_CURRENT_SOURCE_DIR}
    )

    add_test(NAME ${target_name} COMMAND ${target_name})
endfunction()

add_audio_test(audioBufferConverterTest
    tst_audioBufferConverterTest.cpp
    tst_audioBufferConverterTest.h
)

add_audio_test(pcmPlaybackIODeviceTest
    tst_pcmPlaybackIODeviceTest.cpp
    tst_pcmPlaybackIODeviceTest.h
)

add_audio_test(audioFilterTest
    tst_audioFilterTest.cpp
    tst_audioFilterTest.h
)

add_audio_test(audioPeakExtractorTest
    tst_audioPeakExtractorTest.cpp
    tst_audioPeakExtractorTest.h
)

add_audio_test(rubberBandPipelineTest
    tst_rubberBandPipelineTest.cpp
    tst_rubberBandPipelineTest.h
)

add_audio_test(audioPeakCacheTest
    tst_audioPeakCacheTest.cpp
    tst_audioPeakCacheTest.h
)

add_audio_test(audioPlaybackHelpersTest
    tst_audioPlaybackHelpersTest.cpp
    tst_audioPlaybackHelpersTest.h
)

enable_testing()

set_tests_properties(
    audioBufferConverterTest
    pcmPlaybackIODeviceTest
    audioFilterTest
    audioPeakExtractorTest
    audioPeakCacheTest
    rubberBandPipelineTest
    audioPlaybackHelpersTest
    PROPERTIES
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
)
