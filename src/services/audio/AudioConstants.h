#ifndef AUDIOCONSTANTS_H
#define AUDIOCONSTANTS_H

namespace AudioConstants {

    constexpr int kDefaultSampleRateHz{44100};
    constexpr int kChannelCount{2};
    constexpr int kBytesPerSample{2};

    constexpr int kMinTempoPercent{50};
    constexpr int kMaxTempoPercent{100};
    constexpr int kDefaultTempoPercent{100};

    constexpr int kTempoDebounceMs{250};
    constexpr int kPositionPollIntervalMs{50};

    constexpr int kPrebufferMs{400};
    constexpr int kRingBufferMs{800};

    constexpr int kPeakBucketCount{512};
    constexpr int kDefaultRegionStartMs{};
    constexpr int kMaxRegionUndoSteps{32};

    constexpr int kPercentScale{100};

    // Rubber Band time ratio = output_duration / input_duration (2.0 at 50 % playback speed).
    [[nodiscard]] inline double rubberBandTimeRatioFromTempoPercent(int tempoPercent) {
        return static_cast<double>(kPercentScale) / static_cast<double>(tempoPercent);
    }

} // namespace AudioConstants

#endif // AUDIOCONSTANTS_H
