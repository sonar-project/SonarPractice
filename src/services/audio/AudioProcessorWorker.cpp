#include "AudioProcessorWorker.h"

#include "AudioConstants.h"
#include "AudioPeakExtractor.h"
#include "ParametricEq.h"
#include "RubberBandPipeline.h"

#include <QLoggingCategory>
#include <QtMath>

#include <atomic>

Q_LOGGING_CATEGORY(lcWorker, "sonarp.audio.worker")

namespace {

     /**
     * @brief Upmixes mono interleaved audio to stereo.
     * If the input has a single channel, each sample is duplicated to the left and right channels.
     * The function operates in‑place: the supplied vector is replaced with stereo data and the
     * channel count is updated to 2.
     * @param[in,out] interleaved Audio samples in interleaved format; will be replaced with stereo data.
     * @param[in,out] channelCount Number of channels; set to 2 on success, unchanged otherwise.
     */
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

     /**
     * @brief Checks whether processing has been cancelled.
     * Returns true if a non‑null cancellation flag is present and set.
     * @param[in] cancelRequested Pointer to an atomic boolean indicating cancellation request.
     * @return true if cancelled, false otherwise or if the pointer is null.
     */
    [[nodiscard]] bool isCancelled(const std::atomic_bool *cancelRequested) {
        return cancelRequested != nullptr && cancelRequested->load(std::memory_order_acquire);
    }

} // namespace

AudioProcessorWorker::AudioProcessorWorker(QObject *parent) : QObject(parent) {}

/**
* @brief Processes an audio build request.
* Performs file decoding (if a file path is provided), optional tempo stretching,
* equalisation, peak extraction and conversion to 16‑bit PCM. Progress and the final
* result are reported via Qt signals.
* @param[in] params Build parameters containing source data, file path, tempo,
*                   EQ preset, cancellation flag and generation/tempo identifiers.
* @note The function may emit `progressUpdated` and `resultReady` signals.
*       It does not return a value; errors or cancellation are communicated through
*       the `AudioBuildResult` passed to `resultReady`.
*/
void AudioProcessorWorker::process(AudioPlaybackEngine::BuildParameters params) {
    const std::atomic_bool *const cancelRequested = params.cancelRequested;
    const auto cancelled = [&]() { return isCancelled(cancelRequested); };
    const auto cancelledResult = [&params]() {
        AudioBuildResult result;
        result.cancelled = true;
        result.buildGeneration = params.buildGeneration;
        result.tempoPercent = params.tempoPercent;
        result.errorMessage = QObject::tr("Processing cancelled.");
        return result;
    };

    if (cancelled()) {
        emit resultReady(cancelledResult());
        return;
    }

    AudioBuildResult result;
    result.buildGeneration = params.buildGeneration;
    result.tempoPercent = params.tempoPercent;
    std::vector<float> sourceInterleaved = std::move(params.sourceInterleaved);
    int channelCount = params.channelCount;
    int sampleRateHz = params.sampleRateHz;

    if (!params.sourceFilePath.isEmpty()) {
        emit progressUpdated(QObject::tr("Reading audio file…"), -1);

        SampleRequest decodeRequest;
        QString decodeError;
        sourceInterleaved = AudioPlaybackEngine::decodeFileToFloat(
            params.sourceFilePath, decodeRequest, decodeError, cancelRequested);
        channelCount = decodeRequest.channelCount;
        sampleRateHz = decodeRequest.sampleRateHz;

        if (cancelled() || decodeError == QStringLiteral("Cancelled.")) {
            emit resultReady(cancelledResult());
            return;
        }

        if (sourceInterleaved.empty()) {
            result.errorMessage = decodeError.isEmpty()
                                      ? QObject::tr("Decoder returned no audio data.")
                                      : decodeError;
            emit resultReady(result);
            return;
        }

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
        result.decodedInterleaved = sourceInterleaved;
        emit progressUpdated(QObject::tr("Audio file read"), 25);
    }

    if (cancelled()) {
        emit resultReady(cancelledResult());
        return;
    }

    if (sourceInterleaved.empty() || channelCount <= 0 || sampleRateHz <= 0) {
        qCWarning(lcWorker) << "Invalid parameters in processing request";
        result.errorMessage = QObject::tr("No audio data to process.");
        emit resultReady(result);
        return;
    }

    result.sampleRateHz = sampleRateHz;
    result.channelCount = channelCount;

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
            emit resultReady(cancelledResult());
            return;
        }
        emit resultReady(result);
        return;
    }

    if (cancelled()) {
        emit resultReady(cancelledResult());
        return;
    }

    const auto channelCountSize = static_cast<std::size_t>(result.channelCount);
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
        emit resultReady(cancelledResult());
        return;
    }

    result.pcmInt16 = AudioPlaybackEngine::floatToInt16(stretched);
    result.peaks = AudioPeakExtractor::computePeaks(stretched, result.channelCount,
                                                    AudioConstants::kPeakBucketCount);

    const auto frameCount = stretched.size() / static_cast<std::size_t>(result.channelCount);
    result.durationMs =
        (static_cast<qint64>(frameCount) * 1000LL) / static_cast<qint64>(sampleRateHz);
    result.success = true;

    emit progressUpdated(QObject::tr("Done"), 100);
    emit resultReady(result);
}
