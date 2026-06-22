# src/services/audio/CMakeLists.txt

qt_add_library(AudioLib STATIC
    AudioBufferConverter.h
    AudioBufferConverter.cpp
    AudioConstants.h
    BiquadFilter.h
    BiquadFilter.cpp
    ParametricEq.h
    ParametricEq.cpp
    AudioPeakExtractor.h
    AudioPeakExtractor.cpp
    RubberBandPipeline.h
    RubberBandPipeline.cpp
    PcmPlaybackIODevice.h
    PcmPlaybackIODevice.cpp
    AudioPlaybackEngine.h
    AudioPlaybackEngine.cpp
    AudioProcessorWorker.h
    AudioProcessorWorker.cpp
    AudioAnalyzer.h
    AudioAnalyzer.cpp
    AudioAnalyzerWorker.h
    AudioAnalyzerWorker.cpp
    GuitarTabLayout.h
    GuitarTabLayout.cpp
)

target_link_libraries(AudioLib
    PUBLIC
        Qt6::Core
        Qt6::Concurrent
        Qt6::Multimedia
        SonarPractice::Rubberband
        SonarPractice::Aubio
)

target_include_directories(AudioLib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
