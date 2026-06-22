#include "AudioAnalyzerWorker.h"

#include "AudioAnalyzer.h"

/**
 * @brief Registers PitchAnalysisRequest for queued cross-thread calls.
 */
AudioAnalyzerWorker::AudioAnalyzerWorker(QObject *parent) : QObject(parent) {
    qRegisterMetaType<PitchAnalysisRequest>("PitchAnalysisRequest");
}

/**
 * @brief Runs offline pitch detection and emits finished() with the JSON path.
 */
void AudioAnalyzerWorker::analyze(const PitchAnalysisRequest &request) {
    AudioAnalyzer::AnalysisOptions options;
    options.useRegion = request.useRegion;
    options.regionStartMs = request.regionStartMs;
    options.regionEndMs = request.regionEndMs;
    options.mode = static_cast<AudioAnalyzer::DetectionMode>(
        qBound(0, request.detectionMode, 2));

    const AudioAnalyzer analyzer;
    const AudioAnalyzer::SaveResult saveResult =
        analyzer.analyzeAndSave(request.filePath, options);

    emit finished(saveResult.success, saveResult.errorMessage, saveResult.jsonPath,
                  saveResult.noteCount);
}
