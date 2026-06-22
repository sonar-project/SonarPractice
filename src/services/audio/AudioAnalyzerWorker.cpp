#include "AudioAnalyzerWorker.h"

#include "AudioAnalyzer.h"

AudioAnalyzerWorker::AudioAnalyzerWorker(QObject *parent) : QObject(parent) {
    qRegisterMetaType<PitchAnalysisRequest>("PitchAnalysisRequest");
}

void AudioAnalyzerWorker::analyze(const PitchAnalysisRequest &request) {
    AudioAnalyzer::AnalysisOptions options;
    options.useRegion = request.useRegion;
    options.regionStartMs = request.regionStartMs;
    options.regionEndMs = request.regionEndMs;

    const AudioAnalyzer analyzer;
    const AudioAnalyzer::SaveResult saveResult =
        analyzer.analyzeAndSave(request.filePath, options);

    emit finished(saveResult.success, saveResult.errorMessage, saveResult.jsonPath,
                  saveResult.noteCount);
}
