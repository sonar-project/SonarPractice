#ifndef AUDIOANALYZERCOMMON_H
#define AUDIOANALYZERCOMMON_H

#include "AudioAnalyzer.h"

#include <aubio.h>

#include <QString>
#include <vector>

/**
 * @brief Shared helpers for all AudioAnalyzer detection modes.
 *
 * Onset detection, pitch refinement, mono downmix, and region slicing live here.
 */
namespace AudioAnalyzerInternal {

    /** Pitch estimator used when refining note frequencies. */
    enum class PitchAlgorithm { Mcomb, YinFft };

    /** Returns the middle value of a list of numbers. */
    [[nodiscard]] double medianOf(std::vector<double> values);
    /** Returns the pitch distance between two frequencies in cents. */
    [[nodiscard]] double centsBetween(double frequencyAHz, double frequencyBHz);
    /** Returns a stable median pitch, ignoring obvious outliers. */
    [[nodiscard]] double robustMedianFrequency(std::vector<double> samples);

    /** Re-measures pitch inside each note segment using the given algorithm. */
    void refineNoteFrequencies(const std::vector<float> &monoSamples, uint_t sampleRateHz,
                               double timeOffsetSec, QVector<AudioAnalyzer::Note> &notes,
                               PitchAlgorithm algorithm);

    /** Copies pitch from the nearest neighbor when a note has no frequency. */
    void fillMissingPitchFromNeighbors(QVector<AudioAnalyzer::Note> &notes);
    /** Removes notes that have no valid pitch or duration. */
    void removeNotesWithoutPitch(QVector<AudioAnalyzer::Note> &notes);

    /** Averages all channels into one mono stream. */
    [[nodiscard]] std::vector<float> downmixToMono(const std::vector<float> &interleaved,
                                                   std::size_t channelCount);
    /** Returns mono audio for the selected region; sets timeOffsetSec for note times. */
    [[nodiscard]] std::vector<float> extractRegionMono(const std::vector<float> &monoSamples,
                                                       uint_t sampleRateHz,
                                                       const AudioAnalyzer::AnalysisOptions &options,
                                                       double &timeOffsetSec);

    /** Builds note segments from onset times; pitch is filled in later. */
    [[nodiscard]] QVector<AudioAnalyzer::Note>
    buildNotesFromOnsets(const std::vector<double> &onsetTimesSec, double totalDurationSec,
                         double timeOffsetSec);

    /** High-pass filter to make pick attacks easier to detect. */
    [[nodiscard]] std::vector<float> highPassForOnsets(const std::vector<float> &monoSamples,
                                                       uint_t sampleRateHz);
    /** Removes onset times that are too close together. */
    void mergeOnsetTimes(std::vector<double> &onsetTimesSec, double minGapSec);

    /** Runs one aubio onset method and returns onset times in seconds. */
    [[nodiscard]] std::vector<double>
    detectOnsetTimesSecWithMethod(const std::vector<float> &monoSamples, uint_t sampleRateHz,
                                  const char *method, smpl_t threshold, QString &errorMessage);

    /** Combines HFC and complex onsets for guitar-friendly strike detection. */
    [[nodiscard]] std::vector<double> detectOnsetTimesSec(const std::vector<float> &monoSamples,
                                                          uint_t sampleRateHz,
                                                          QString &errorMessage);

    /** Returns how many seconds two note segments overlap. */
    [[nodiscard]] double overlapDurationSec(const AudioAnalyzer::Note &a,
                                            const AudioAnalyzer::Note &b);

} // namespace AudioAnalyzerInternal

#endif // AUDIOANALYZERCOMMON_H
