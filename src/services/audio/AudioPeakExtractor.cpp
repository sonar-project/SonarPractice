#include "AudioPeakExtractor.h"

#include <algorithm>
#include <cmath>

/**
 * @brief Computes a normalized list of peak values for audio visualization.
 *
 * This method divides the audio data into a specific number of buckets and
 * finds the maximum amplitude within each. Finally, it normalizes the result
 * so that the highest peak is 1.0.
 *
 * @param interleavedSamples The raw audio samples stored in an interleaved format.
 * @param channelCount The number of audio channels (e.g., 1 for mono, 2 for stereo).
 * @param bucketCount The number of output peaks desired (e.g., for drawing a waveform).
 * @return A QVector containing the normalized peak values for each bucket.
 */
QVector<float> AudioPeakExtractor::computePeaks(const std::vector<float> &interleavedSamples,
                                                int channelCount, int bucketCount) {
    QVector<float> peaks;
    if (bucketCount <= 0 || channelCount <= 0 || interleavedSamples.empty()) {
        return peaks;
    }

    const auto channelCountSize = static_cast<std::size_t>(channelCount);
    const auto frameCount = interleavedSamples.size() / channelCountSize;

    if (frameCount == 0) {
        return peaks;
    }

    peaks.resize(bucketCount);
    peaks.fill(0.0F);

    for (int bucket = 0; bucket < bucketCount; ++bucket) {
        const auto startFrame =
            (static_cast<std::size_t>(bucket) * frameCount) / static_cast<std::size_t>(bucketCount);
        const auto endFrame = std::min((static_cast<std::size_t>(bucket + 1) * frameCount) /
                                           static_cast<std::size_t>(bucketCount),
                                       frameCount);
        float peak = 0.0F;

        for (auto frame = startFrame; frame < endFrame; ++frame) {
            for (int channel = 0; channel < channelCount; ++channel) {
                const auto index = frame * channelCountSize + static_cast<std::size_t>(channel);
                const float sample = std::fabs(interleavedSamples[index]);
                peak = std::max(peak, sample);
            }
        }
        peaks[bucket] = peak;
    }

    const auto maxPeak = std::max_element(peaks.begin(), peaks.end(),
                                          [](float left, float right) { return left < right; });
    const float normalize = (maxPeak != peaks.end() && *maxPeak > 0.0F) ? (1.0F / *maxPeak) : 1.0F;
    for (float &value : peaks) {
        value *= normalize;
    }

    return peaks;
}
