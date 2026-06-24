#include "AudioProcessorWorker.h"

#include "AudioConstants.h"
#include "AudioPeakCache.h"
#include "AudioPeakExtractor.h"
#include "ParametricEq.h"
#include "RubberBandPipeline.h"

#include <QLoggingCategory>
#include <QtMath>

#include <atomic>

Q_LOGGING_CATEGORY(lcWorker, "sonarp.audio.worker")

namespace {

    void upmixMonoToStereo(std::vector<float> &interleaved, int &channelCount) {
        if (channelCount != 1) {
            return;
        }

        const auto frameCount = interleaved.size();
        if (frameCount == 0) {
            return;
        }

        std::vector<float> stereo(frameCount * 2);

        for (std::size_t frame = 0; frame < frameCount; ++frame) {
            const auto sample = interleaved[frame];
            stereo[frame * 2] = sample;
            stereo[frame * 2 + 1] = sample;
        }

        interleaved = std::move(stereo);
        channelCount = AudioConstants::kChannelCount;
    }

    [[nodiscard]] bool isCancelled(const std::atomic_bool *cancelRequested) {
        return cancelRequested != nullptr && cancelRequested->load(std::memory_order_acquire);
    }

    [[nodiscard]] AudioBuildResult cancelledResult(const AudioPlaybackEngine::BuildParameters &params) {
        AudioBuildResult result;
        result.cancelled = true;
        result.buildGeneration = params.buildGeneration;
        result.tempoPercent = params.tempoPercent;
        result.errorMessage = QObject::tr("Processing cancelled.");
        return result;
    }

    [[nodiscard]] qint64 durationMsForFrames(std::size_t frameCount, int sampleRateHz) {
        if (frameCount == 0 || sampleRateHz <= 0) {
            return 0;
        }
        return (static_cast<qint64>(frameCount) * 1000LL) / static_cast<qint64>(sampleRateHz);
    }

} // namespace

AudioProcessorWorker::AudioProcessorWorker(QObject *parent) : QObject(parent) {}

void AudioProcessorWorker::process(const AudioPlaybackEngine::BuildParameters& params) {
    const std::atomic_bool *const cancelRequested = params.cancelRequested;
    const auto cancelled = [&]() { return isCancelled(cancelRequested); };

    if (cancelled()) {
        emit resultReady(cancelledResult(params));
        return;
    }

    if (params.mode == AudioPlaybackEngine::BuildParameters::Mode::MetadataAndPeaks) {
        processMetadataAndPeaks(params);
        return;
    }

    processPlaybackSegment(params);
}

void AudioProcessorWorker::processMetadataAndPeaks(const AudioPlaybackEngine::BuildParameters& params) {
    const std::atomic_bool *const cancelRequested = params.cancelRequested;
    const auto cancelled = [&]() { return isCancelled(cancelRequested); };

    AudioBuildResult result;
    result.buildGeneration = params.buildGeneration;
    result.tempoPercent = params.tempoPercent;
    result.metadataOnly = true;

    if (params.sourceFilePath.isEmpty()) {
        result.errorMessage = QObject::tr("No audio file path provided.");
        emit resultReady(result);
        return;
    }

    AudioPeakCacheEntry cacheEntry;
    if (AudioPeakCache::read(params.sourceFilePath, cacheEntry)) {
        emit progressUpdated(QObject::tr("Loading waveform cache…"), 100);
        result.peaks = cacheEntry.peaks;
        result.durationMs = cacheEntry.durationMs;
        result.sampleRateHz = cacheEntry.sampleRateHz;
        result.channelCount = cacheEntry.channelCount;
        result.success = true;
        emit resultReady(result);
        return;
    }

    emit progressUpdated(QObject::tr("Analyzing audio file…"), -1);

    SampleRequest decodeRequest;
    QString decodeError;
    const std::vector<float> sourceInterleaved = AudioPlaybackEngine::decodeFileToFloat(
        params.sourceFilePath, decodeRequest, decodeError, cancelRequested);

    if (cancelled() || decodeError == QStringLiteral("Cancelled.")) {
        emit resultReady(cancelledResult(params));
        return;
    }

    if (sourceInterleaved.empty()) {
        result.errorMessage = decodeError.isEmpty() ? QObject::tr("Decoder returned no audio data.")
                                                    : decodeError;
        emit resultReady(result);
        return;
    }

    int channelCount = decodeRequest.channelCount;
    int sampleRateHz = decodeRequest.sampleRateHz;
    if (channelCount <= 0) {
        channelCount = AudioConstants::kChannelCount;
    }
    if (sampleRateHz <= 0) {
        sampleRateHz = AudioConstants::kDefaultSampleRateHz;
    }

    std::vector<float> normalized = sourceInterleaved;
    upmixMonoToStereo(normalized, channelCount);

    result.peaks = AudioPeakExtractor::computePeaks(normalized, channelCount,
                                                    AudioConstants::kPeakBucketCount);
    result.channelCount = channelCount;
    result.sampleRateHz = sampleRateHz;

    const auto channelCountSize = static_cast<std::size_t>(channelCount);
    const auto frameCount = normalized.size() / channelCountSize;
    result.durationMs = durationMsForFrames(frameCount, sampleRateHz);
    result.success = true;

    AudioPeakCacheEntry cacheWrite;
    cacheWrite.peaks = result.peaks;
    cacheWrite.durationMs = result.durationMs;
    cacheWrite.sampleRateHz = result.sampleRateHz;
    cacheWrite.channelCount = result.channelCount;
    if (!AudioPeakCache::write(params.sourceFilePath, cacheWrite)) {
        qCDebug(lcWorker) << "Could not write peak cache for" << params.sourceFilePath;
    }

    emit progressUpdated(QObject::tr("Waveform ready"), 100);
    emit resultReady(result);
}

void AudioProcessorWorker::processPlaybackSegment(const AudioPlaybackEngine::BuildParameters& params) {
    const std::atomic_bool *const cancelRequested = params.cancelRequested;
    const auto cancelled = [&]() { return isCancelled(cancelRequested); };

    AudioBuildResult result;
    result.buildGeneration = params.buildGeneration;
    result.tempoPercent = params.tempoPercent;

    if (params.sourceFilePath.isEmpty()) {
        result.errorMessage = QObject::tr("No audio file path provided.");
        emit resultReady(result);
        return;
    }

    emit progressUpdated(QObject::tr("Reading playback segment…"), -1);

    const int sampleRateForSeek = params.sourceSampleRateHz > 0 ? params.sourceSampleRateHz
                                                                 : AudioConstants::kDefaultSampleRateHz;

    SampleRequest decodeRequest;
    decodeRequest.decodeStartFrame =
        (params.regionStartMs * static_cast<qint64>(sampleRateForSeek)) / 1000LL;
    if (params.regionEndMs > params.regionStartMs) {
        decodeRequest.decodeEndFrame =
            (params.regionEndMs * static_cast<qint64>(sampleRateForSeek)) / 1000LL;
    }

    QString decodeError;
    std::vector<float> sourceInterleaved = AudioPlaybackEngine::decodeFileToFloat(
        params.sourceFilePath, decodeRequest, decodeError, cancelRequested);

    if (cancelled() || decodeError == QStringLiteral("Cancelled.")) {
        emit resultReady(cancelledResult(params));
        return;
    }

    if (sourceInterleaved.empty()) {
        result.errorMessage = decodeError.isEmpty() ? QObject::tr("Decoder returned no audio data.")
                                                    : decodeError;
        emit resultReady(result);
        return;
    }

    int channelCount = decodeRequest.channelCount;
    int sampleRateHz = decodeRequest.sampleRateHz;
    if (channelCount <= 0) {
        channelCount = AudioConstants::kChannelCount;
    }
    if (sampleRateHz <= 0) {
        sampleRateHz = AudioConstants::kDefaultSampleRateHz;
    }

    const auto channelCountSize = static_cast<size_t>(channelCount);
    if (channelCountSize > 0 && sourceInterleaved.size() % channelCountSize != 0) {
        sourceInterleaved.resize(sourceInterleaved.size() -
                                 (sourceInterleaved.size() % channelCountSize));
    }

    upmixMonoToStereo(sourceInterleaved, channelCount);
    result.sampleRateHz = sampleRateHz;
    result.channelCount = channelCount;

    const auto sourceFrames = sourceInterleaved.size() / channelCountSize;
    result.segmentSourceStartMs = params.regionStartMs;
    result.segmentSourceEndMs =
        params.regionStartMs + durationMsForFrames(sourceFrames, sampleRateHz);

    if (cancelled()) {
        emit resultReady(cancelledResult(params));
        return;
    }

    const double tempoRatio =
        AudioConstants::rubberBandTimeRatioFromTempoPercent(params.tempoPercent);

    std::vector<float> stretched;
    RubberBandPipeline pipeline;

    const bool needsStretch = !qFuzzyCompare(tempoRatio, 1.0);
    if (needsStretch) {
        emit progressUpdated(QObject::tr("Adjusting tempo…"), 30);
    }

    const RubberBandPipeline::ProgressCallback stretchProgress = [this, needsStretch](int percent) {
        if (!needsStretch) {
            return;
        }
        const auto mapped = 30 + ((percent * 55) / 100);
        emit progressUpdated(QObject::tr("Adjusting tempo…"), mapped);
    };

    const std::function<bool()> stretchCancelled = [&]() { return cancelled(); };

    if (!pipeline.stretch(sourceInterleaved, channelCount, sampleRateHz, tempoRatio, stretched,
                          result.errorMessage, stretchProgress, stretchCancelled)) {
        if (cancelled() || result.errorMessage == QStringLiteral("Cancelled.")) {
            emit resultReady(cancelledResult(params));
            return;
        }
        emit resultReady(result);
        return;
    }

    if (cancelled()) {
        emit resultReady(cancelledResult(params));
        return;
    }

    if (stretched.empty() || channelCountSize == 0 || stretched.size() % channelCountSize != 0) {
        result.errorMessage = QObject::tr("Tempo processing returned invalid audio data.");
        emit resultReady(result);
        return;
    }

    emit progressUpdated(QObject::tr("Optimizing audio…"), 90);

    ParametricEq equalizer;
    equalizer.setSampleRate(sampleRateHz);
    equalizer.setPresetById(params.eqPresetId);
    equalizer.process(stretched, result.channelCount);

    if (cancelled()) {
        emit resultReady(cancelledResult(params));
        return;
    }

    result.pcmInt16 = AudioPlaybackEngine::floatToInt16(stretched);

    const auto frameCount = stretched.size() / channelCountSize;
    result.durationMs = durationMsForFrames(frameCount, sampleRateHz);
    result.success = true;

    emit progressUpdated(QObject::tr("Done"), 100);
    emit resultReady(result);
}
