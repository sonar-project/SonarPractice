#include "AudioAnalyzerCommon.h"

#include <aubio.h>

#include <algorithm>
#include <cmath>

namespace AudioAnalyzerInternal {

    constexpr uint_t kOnsetBufferSize = 512;
    constexpr uint_t kOnsetHopSize = 256;
    constexpr uint_t kPitchBufferSize = 8192;
    constexpr uint_t kPitchHopSize = 256;
    constexpr smpl_t kOnsetSilenceDb = -72.0;
    constexpr smpl_t kOnsetThresholdHfc = 0.15;
    constexpr smpl_t kOnsetThresholdComplex = 0.12;
    constexpr smpl_t kPitchToleranceMcomb = 0.85;
    constexpr smpl_t kPitchToleranceYinFft = 0.92;
    constexpr double kOnsetMinioiMs = 22.0;
    constexpr double kOnsetMergeMs = 14.0;
    constexpr double kOnsetHighPassHz = 70.0;
    constexpr double kMinPitchAnalysisMs = 35.0;

    /**
     * @brief Returns the middle value of a sorted list of numbers.
     */
    double medianOf(std::vector<double> values) {
        if (values.empty()) {
            return 0.0;
        }
        const auto mid = values.size() / 2;
        std::nth_element(values.begin(), values.begin() + static_cast<std::ptrdiff_t>(mid),
                         values.end());
        if (values.size() % 2 == 1) {
            return values[mid];
        }
        const double lowerMax = *std::max_element(values.begin(),
                                                  values.begin() + static_cast<std::ptrdiff_t>(mid));
        return (lowerMax + values[mid]) / 2.0;
    }

    /**
     * @brief Returns the pitch distance between two frequencies in cents.
     */
    double centsBetween(double frequencyAHz, double frequencyBHz) {
        if (frequencyAHz <= 0.0 || frequencyBHz <= 0.0) {
            return 0.0;
        }
        return 1200.0 * std::log2(frequencyBHz / frequencyAHz);
    }

    /**
     * @brief Returns a stable median pitch, ignoring obvious outliers.
     */
    double robustMedianFrequency(std::vector<double> samples) {
        if (samples.empty()) {
            return 0.0;
        }
        if (samples.size() < 3) {
            return medianOf(std::move(samples));
        }

        const double roughMedian = medianOf(samples);
        std::vector<double> inliers;
        inliers.reserve(samples.size());
        for (const double frequencyHz : samples) {
            if (std::abs(centsBetween(roughMedian, frequencyHz)) <= 50.0) {
                inliers.push_back(frequencyHz);
            }
        }
        if (inliers.empty()) {
            return roughMedian;
        }
        return medianOf(std::move(inliers));
    }

    /**
     * @brief Re-measures pitch inside each note segment using aubio pitch.
     */
    void refineNoteFrequencies(const std::vector<float> &monoSamples, uint_t sampleRateHz,
                               double timeOffsetSec, QVector<AudioAnalyzer::Note> &notes,
                               PitchAlgorithm algorithm) {
        if (notes.isEmpty() || monoSamples.empty() || sampleRateHz == 0) {
            return;
        }

        const char *pitchMethod = algorithm == PitchAlgorithm::YinFft ? "yinfft" : "mcomb";
        const smpl_t tolerance =
            algorithm == PitchAlgorithm::YinFft ? kPitchToleranceYinFft : kPitchToleranceMcomb;

        aubio_pitch_t *pitchDetector =
            new_aubio_pitch(pitchMethod, kPitchBufferSize, kPitchHopSize, sampleRateHz);
        if (pitchDetector == nullptr) {
            return;
        }

        aubio_pitch_set_silence(pitchDetector, -90.0);
        aubio_pitch_set_tolerance(pitchDetector, tolerance);
        aubio_pitch_set_unit(pitchDetector, "Hz");

        fvec_t *input = new_fvec(kPitchHopSize);
        fvec_t *pitchOutput = new_fvec(1);
        if (input == nullptr || pitchOutput == nullptr) {
            del_fvec(input);
            del_fvec(pitchOutput);
            del_aubio_pitch(pitchDetector);
            return;
        }

        for (AudioAnalyzer::Note &note : notes) {
            const double relativeStartSec = note.startSec - timeOffsetSec;
            const double relativeEndSec = note.endSec - timeOffsetSec;
            if (relativeEndSec <= relativeStartSec) {
                continue;
            }

            const auto startFrame = static_cast<std::size_t>(
                std::max(0.0, relativeStartSec) * static_cast<double>(sampleRateHz));
            auto endFrame = static_cast<std::size_t>(
                std::min(relativeEndSec * static_cast<double>(sampleRateHz),
                         static_cast<double>(monoSamples.size())));

            const auto minAnalysisFrames = static_cast<std::size_t>(
                (kMinPitchAnalysisMs * static_cast<double>(sampleRateHz)) / 1000.0);
            if (endFrame > startFrame && endFrame - startFrame < minAnalysisFrames) {
                endFrame = std::min(startFrame + minAnalysisFrames, monoSamples.size());
            }

            std::vector<double> pitchSamples;
            for (std::size_t offset = startFrame; offset + kPitchHopSize <= endFrame;
                 offset += kPitchHopSize) {
                for (uint_t index = 0; index < kPitchHopSize; ++index) {
                    input->data[index] = monoSamples[offset + index];
                }

                aubio_pitch_do(pitchDetector, input, pitchOutput);
                const double pitchHz = pitchOutput->data[0];
                if (pitchHz > 0.0) {
                    pitchSamples.push_back(pitchHz);
                }
            }

            if (!pitchSamples.empty()) {
                note.frequencyHz = robustMedianFrequency(pitchSamples);
            }
        }

        del_fvec(input);
        del_fvec(pitchOutput);
        del_aubio_pitch(pitchDetector);
    }

    /**
     * @brief Copies pitch from the nearest neighbor when a note has no frequency.
     */
    void fillMissingPitchFromNeighbors(QVector<AudioAnalyzer::Note> &notes) {
        double lastValidHz = 0.0;
        for (AudioAnalyzer::Note &note : notes) {
            if (note.frequencyHz > 0.0) {
                lastValidHz = note.frequencyHz;
                continue;
            }
            if (lastValidHz > 0.0) {
                note.frequencyHz = lastValidHz;
            }
        }

        double nextValidHz = 0.0;
        for (int index = notes.size() - 1; index >= 0; --index) {
            AudioAnalyzer::Note &note = notes[index];
            if (note.frequencyHz > 0.0) {
                nextValidHz = note.frequencyHz;
                continue;
            }
            if (nextValidHz > 0.0) {
                note.frequencyHz = nextValidHz;
            }
        }
    }

    /**
     * @brief Removes notes that have no valid pitch or duration.
     */
    void removeNotesWithoutPitch(QVector<AudioAnalyzer::Note> &notes) {
        QVector<AudioAnalyzer::Note> filtered;
        filtered.reserve(notes.size());
        for (const AudioAnalyzer::Note &note : notes) {
            if (note.endSec > note.startSec && note.frequencyHz > 0.0) {
                filtered.push_back(note);
            }
        }
        notes = std::move(filtered);
    }

    /**
     * @brief Applies a simple high-pass filter before onset detection.
     */
    std::vector<float> highPassForOnsets(const std::vector<float> &monoSamples,
                                       uint_t sampleRateHz) {
        if (monoSamples.empty() || sampleRateHz == 0) {
            return monoSamples;
        }

        const float sampleRate = static_cast<float>(sampleRateHz);
        const float cutoffHz = static_cast<float>(kOnsetHighPassHz);
        const float pi = 3.14159265358979323846F;
        const float angularFrequency = 2.0F * pi * cutoffHz;
        const float alpha = angularFrequency / (angularFrequency + sampleRate);

        std::vector<float> filtered(monoSamples.size());
        float previousInput = 0.0F;
        float previousOutput = 0.0F;
        for (std::size_t index = 0; index < monoSamples.size(); ++index) {
            const float input = monoSamples[index];
            const float output = alpha * (previousOutput + input - previousInput);
            filtered[index] = output;
            previousInput = input;
            previousOutput = output;
        }
        return filtered;
    }

    /**
     * @brief Removes onset times that are closer than minGapSec.
     */
    void mergeOnsetTimes(std::vector<double> &onsetTimesSec, double minGapSec) {
        if (onsetTimesSec.empty()) {
            return;
        }

        std::sort(onsetTimesSec.begin(), onsetTimesSec.end());
        std::vector<double> merged;
        merged.reserve(onsetTimesSec.size());
        merged.push_back(onsetTimesSec.front());
        for (std::size_t index = 1; index < onsetTimesSec.size(); ++index) {
            if (onsetTimesSec[index] - merged.back() >= minGapSec) {
                merged.push_back(onsetTimesSec[index]);
            }
        }
        onsetTimesSec = std::move(merged);
    }

    /**
     * @brief Runs one aubio onset method and returns onset times in seconds.
     */
    std::vector<double>
    detectOnsetTimesSecWithMethod(const std::vector<float> &monoSamples, uint_t sampleRateHz,
                                  const char *method, smpl_t threshold, QString &errorMessage) {
        std::vector<double> onsetTimesSec;

        if (monoSamples.empty() || sampleRateHz == 0) {
            return onsetTimesSec;
        }

        aubio_onset_t *onsetDetector =
            new_aubio_onset(method, kOnsetBufferSize, kOnsetHopSize, sampleRateHz);
        if (onsetDetector == nullptr) {
            errorMessage =
                AudioAnalyzer::tr("Could not initialize aubio onset detector (%1).").arg(method);
            return onsetTimesSec;
        }

        aubio_onset_set_threshold(onsetDetector, threshold);
        aubio_onset_set_silence(onsetDetector, kOnsetSilenceDb);
        aubio_onset_set_minioi_ms(onsetDetector, static_cast<smpl_t>(kOnsetMinioiMs));

        fvec_t *input = new_fvec(kOnsetHopSize);
        fvec_t *onsetOutput = new_fvec(1);
        if (input == nullptr || onsetOutput == nullptr) {
            del_aubio_onset(onsetDetector);
            if (input != nullptr) {
                del_fvec(input);
            }
            if (onsetOutput != nullptr) {
                del_fvec(onsetOutput);
            }
            errorMessage = AudioAnalyzer::tr("Could not allocate aubio buffers.");
            return onsetTimesSec;
        }

        const auto totalFrames = monoSamples.size();
        onsetTimesSec.reserve(totalFrames / kOnsetHopSize);

        for (std::size_t offset = 0; offset + kOnsetHopSize <= totalFrames;
             offset += kOnsetHopSize) {
            for (uint_t index = 0; index < kOnsetHopSize; ++index) {
                input->data[index] = monoSamples[offset + index];
            }

            aubio_onset_do(onsetDetector, input, onsetOutput);
            if (onsetOutput->data[0] != 0.0F) {
                const double onsetSec =
                    static_cast<double>(aubio_onset_get_last_s(onsetDetector));
                if (onsetSec >= 0.0) {
                    onsetTimesSec.push_back(onsetSec);
                }
            }
        }

        del_fvec(input);
        del_fvec(onsetOutput);
        del_aubio_onset(onsetDetector);

        mergeOnsetTimes(onsetTimesSec, kOnsetMergeMs / 1000.0);
        return onsetTimesSec;
    }

    /**
     * @brief Combines HFC and complex onsets for guitar-friendly strike detection.
     */
    std::vector<double> detectOnsetTimesSec(const std::vector<float> &monoSamples,
                                            uint_t sampleRateHz, QString &errorMessage) {
        std::vector<double> onsetTimesSec;

        if (monoSamples.empty() || sampleRateHz == 0) {
            return onsetTimesSec;
        }

        QString localError;
        const std::vector<double> hfcOnsets = detectOnsetTimesSecWithMethod(
            monoSamples, sampleRateHz, "hfc", kOnsetThresholdHfc, localError);
        if (!localError.isEmpty()) {
            errorMessage = localError;
            return onsetTimesSec;
        }

        const std::vector<float> highPassed = highPassForOnsets(monoSamples, sampleRateHz);
        const std::vector<double> complexOnsets = detectOnsetTimesSecWithMethod(
            highPassed, sampleRateHz, "complex", kOnsetThresholdComplex, localError);
        if (!localError.isEmpty()) {
            errorMessage = localError;
            return onsetTimesSec;
        }

        onsetTimesSec.reserve(hfcOnsets.size() + complexOnsets.size());
        onsetTimesSec.insert(onsetTimesSec.end(), hfcOnsets.begin(), hfcOnsets.end());
        onsetTimesSec.insert(onsetTimesSec.end(), complexOnsets.begin(), complexOnsets.end());
        mergeOnsetTimes(onsetTimesSec, kOnsetMergeMs / 1000.0);
        return onsetTimesSec;
    }

    /**
     * @brief Builds note segments from onset times; pitch is filled in later.
     */
    QVector<AudioAnalyzer::Note>
    buildNotesFromOnsets(const std::vector<double> &onsetTimesSec, double totalDurationSec,
                         double timeOffsetSec) {
        QVector<AudioAnalyzer::Note> notes;
        if (onsetTimesSec.empty()) {
            return notes;
        }

        notes.reserve(static_cast<int>(onsetTimesSec.size()));

        for (std::size_t index = 0; index < onsetTimesSec.size(); ++index) {
            const double startSec = onsetTimesSec.at(index);
            const double endSec = index + 1 < onsetTimesSec.size() ? onsetTimesSec.at(index + 1)
                                                                   : totalDurationSec;
            if (endSec <= startSec) {
                continue;
            }

            AudioAnalyzer::Note note;
            note.startSec = startSec + timeOffsetSec;
            note.endSec = endSec + timeOffsetSec;
            note.frequencyHz = 0.0;
            notes.push_back(note);
        }

        return notes;
    }

    /**
     * @brief Averages all channels into one mono stream.
     */
    std::vector<float> downmixToMono(const std::vector<float> &interleaved,
                                     std::size_t channelCount) {
        if (channelCount <= 1) {
            return interleaved;
        }

        const auto frameCount = interleaved.size() / channelCount;
        std::vector<float> mono(frameCount, 0.0F);
        for (std::size_t frame = 0; frame < frameCount; ++frame) {
            float sum = 0.0F;
            for (std::size_t channel = 0; channel < channelCount; ++channel) {
                sum += interleaved[frame * channelCount + channel];
            }
            mono[frame] = sum / static_cast<float>(channelCount);
        }
        return mono;
    }

    /**
     * @brief Returns mono audio for the selected region and sets timeOffsetSec.
     */
    std::vector<float> extractRegionMono(const std::vector<float> &monoSamples,
                                         uint_t sampleRateHz,
                                         const AudioAnalyzer::AnalysisOptions &options,
                                         double &timeOffsetSec) {
        if (!options.useRegion) {
            timeOffsetSec = 0.0;
            return monoSamples;
        }

        const auto totalFrames = monoSamples.size();
        const auto startFrame = static_cast<std::size_t>(
            (std::max<qint64>(0, options.regionStartMs) * static_cast<qint64>(sampleRateHz)) /
            1000LL);
        std::size_t endFrame = totalFrames;
        if (options.regionEndMs > 0) {
            endFrame = static_cast<std::size_t>(
                (options.regionEndMs * static_cast<qint64>(sampleRateHz)) / 1000LL);
        }
        endFrame = std::min(endFrame, totalFrames);
        if (endFrame <= startFrame) {
            return {};
        }

        timeOffsetSec = static_cast<double>(startFrame) / static_cast<double>(sampleRateHz);
        return std::vector<float>(monoSamples.begin() + static_cast<std::ptrdiff_t>(startFrame),
                                  monoSamples.begin() + static_cast<std::ptrdiff_t>(endFrame));
    }

    /**
     * @brief Returns how many seconds two note segments overlap.
     */
    double overlapDurationSec(const AudioAnalyzer::Note &a, const AudioAnalyzer::Note &b) {
        const double overlapStart = std::max(a.startSec, b.startSec);
        const double overlapEnd = std::min(a.endSec, b.endSec);
        return std::max(0.0, overlapEnd - overlapStart);
    }

} // namespace AudioAnalyzerInternal
