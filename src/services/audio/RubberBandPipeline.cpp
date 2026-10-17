#include "RubberBandPipeline.h"

#include "AudioBufferConverter.h"

#include <algorithm>
#include <cmath>
#include <rubberband/RubberBandStretcher.h>

namespace {

    constexpr std::size_t kRubberBandBlockSize = 1024;

} // namespace

/**
 * @brief Stretches audio tempo without changing its pitch using the RubberBand library.
 *
 * Deinterleaves the source, runs RubberBand study/process/retrieve on planar float
 * channel pointers, then re-interleaves the result.
 */
bool RubberBandPipeline::stretch(const std::vector<float> &sourceInterleaved, int channelCount,
                                 int sampleRateHz, double tempoRatio,
                                 std::vector<float> &outputInterleaved, QString &errorMessage,
                                 const ProgressCallback &onProgress,
                                 const std::function<bool()> &isCancelled) { // TODO: Another simple option is to add a struct for parameters.

    const auto cancelled = [&]() { return isCancelled && isCancelled(); };

    if (sourceInterleaved.empty() || channelCount <= 0 || sampleRateHz <= 0) {
        errorMessage = tr("No audio data to stretch.");
        return false;
    }

    if (tempoRatio <= 0.0) {
        errorMessage = tr("Invalid tempo ratio.");
        return false;
    }

    const auto channelCountSize = static_cast<std::size_t>(channelCount);
    const auto frameCount = sourceInterleaved.size() / channelCountSize;

    if (frameCount == 0) {
        errorMessage = tr("Invalid frame count.");
        return false;
    }

    const auto reportProgress = [&](int completedUnits, int totalUnits) {
        if (!onProgress || totalUnits <= 0) {
            return;
        }
        const int percent = qBound(0, (completedUnits * 100) / totalUnits, 100);
        onProgress(percent);
    };

    if (qFuzzyCompare(tempoRatio, 1.0)) {
        outputInterleaved = sourceInterleaved;
        reportProgress(100, 100);
        return true;
    }

    constexpr int kStudyWeight = 35;
    constexpr int kProcessWeight = 45;
    constexpr int kRetrieveWeight = 20;
    const int totalWeight = kStudyWeight + kProcessWeight + kRetrieveWeight;

    using RubberBand::RubberBandStretcher;

    RubberBandStretcher stretcher(sampleRateHz, channelCountSize,
                                  RubberBandStretcher::OptionProcessOffline |
                                      RubberBandStretcher::OptionEngineFiner);

    stretcher.setTimeRatio(tempoRatio);
    stretcher.setPitchScale(1.0);

    const DeinterleavedFloatBuffer inputChannels =
        DeinterleavedFloatBuffer::fromInterleaved(sourceInterleaved, channelCountSize);

    if (inputChannels.frameCount() != frameCount ||
        inputChannels.channelCount() != channelCountSize) {
        errorMessage = tr("Failed to prepare input channels.");
        return false;
    }

    std::vector<const float *> studyPointers(channelCountSize);
    std::vector<const float *> processPointers(channelCountSize);
    std::vector<float *> retrievePointers(channelCountSize);

    for (int channel = 0; channel < channelCount; ++channel) {
        const auto channelSpan = inputChannels.channel(static_cast<std::size_t>(channel));
        studyPointers[static_cast<std::size_t>(channel)] = channelSpan.data();
        processPointers[static_cast<std::size_t>(channel)] = channelSpan.data();
    }

    std::size_t inputOffset = 0;
    while (inputOffset < frameCount) {
        if (cancelled()) {
            errorMessage = tr("Cancelled.");
            return false;
        }

        const auto blockFrames = std::min(kRubberBandBlockSize, frameCount - inputOffset);

        for (int channel = 0; channel < channelCount; ++channel) {
            studyPointers[static_cast<std::size_t>(channel)] =
                inputChannels.channel(static_cast<std::size_t>(channel)).data() + inputOffset;
        }

        stretcher.study(studyPointers.data(), blockFrames,
                        inputOffset + blockFrames < frameCount ? 0 : 1);

        inputOffset += blockFrames;

        reportProgress(static_cast<int>((inputOffset * kStudyWeight) / frameCount), totalWeight);
    }

    inputOffset = 0;
    while (inputOffset < frameCount) {
        if (cancelled()) {
            errorMessage = tr("Cancelled.");
            return false;
        }

        const auto blockFrames = std::min(kRubberBandBlockSize, frameCount - inputOffset);

        for (int channel = 0; channel < channelCount; ++channel) {
            processPointers[static_cast<std::size_t>(channel)] =
                inputChannels.channel(static_cast<std::size_t>(channel)).data() + inputOffset;
        }

        stretcher.process(processPointers.data(), blockFrames,
                          inputOffset + blockFrames < frameCount ? 0 : 1);

        inputOffset += blockFrames;

        reportProgress(kStudyWeight + static_cast<int>((inputOffset * kProcessWeight) / frameCount),
                       totalWeight);
    }

    stretcher.process(nullptr, 0, 1);

    const auto expectedOutputFrames =
        static_cast<std::size_t>(std::ceil(static_cast<double>(frameCount) * tempoRatio));

    DeinterleavedFloatBuffer::ConvertParameter param{.frameCount = expectedOutputFrames,
                                                     .channelCount = channelCountSize};

    DeinterleavedFloatBuffer outputChannels = DeinterleavedFloatBuffer::createEmpty(param);

    if (outputChannels.frameCount() == 0 || outputChannels.channelCount() != channelCountSize) {
        errorMessage = tr("Failed to allocate output buffer.");
        return false;
    }

    for (int channel = 0; channel < channelCount; ++channel) {
        retrievePointers[static_cast<std::size_t>(channel)] =
            outputChannels.channel(static_cast<std::size_t>(channel)).data();
    }

    std::size_t writtenFrames = 0;
    while (writtenFrames < expectedOutputFrames) {
        if (cancelled()) {
            errorMessage = tr("Cancelled.");
            return false;
        }

        for (int channel = 0; channel < channelCount; ++channel) {
            retrievePointers[static_cast<std::size_t>(channel)] =
                outputChannels.channel(static_cast<std::size_t>(channel)).data() + writtenFrames;
        }

        const auto retrieved =
            stretcher.retrieve(retrievePointers.data(), expectedOutputFrames - writtenFrames);

        if (retrieved <= 0) {
            break;
        }

        writtenFrames += static_cast<std::size_t>(retrieved);

        reportProgress(
            kStudyWeight + kProcessWeight +
                static_cast<int>((writtenFrames * kRetrieveWeight) / expectedOutputFrames),
            totalWeight);
    }

    const auto outputFrames = std::min(writtenFrames, expectedOutputFrames);

    std::vector<std::span<const float>> trimmedChannels;
    trimmedChannels.reserve(channelCountSize);

    for (int channel = 0; channel < channelCount; ++channel) {
        trimmedChannels.emplace_back(
            outputChannels.channel(static_cast<std::size_t>(channel)).first(outputFrames));
    }
    DeinterleavedFloatBuffer::interleaveInto(trimmedChannels, outputInterleaved);

    if (outputInterleaved.empty()) {
        errorMessage = tr("Stretch produced no output samples.");
        return false;
    }

    return true;
}
