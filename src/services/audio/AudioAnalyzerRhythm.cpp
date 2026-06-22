#include "AudioAnalyzerRhythm.h"

#include "AudioAnalyzerCommon.h"

namespace AudioAnalyzerInternal {

    /**
     * @brief Finds pick strokes with onsets, then measures pitch with mcomb.
     */
    QVector<AudioAnalyzer::Note>
    detectNotesRhythmic(const std::vector<float> &monoSamples, uint_t sampleRateHz,
                        double timeOffsetSec, QString &errorMessage) {
        QVector<AudioAnalyzer::Note> detectedNotes;

        const std::vector<double> onsetTimesSec =
            detectOnsetTimesSec(monoSamples, sampleRateHz, errorMessage);
        if (!errorMessage.isEmpty()) {
            return detectedNotes;
        }

        const double totalDurationSec =
            static_cast<double>(monoSamples.size()) / static_cast<double>(sampleRateHz);
        detectedNotes = buildNotesFromOnsets(onsetTimesSec, totalDurationSec, timeOffsetSec);

        refineNoteFrequencies(monoSamples, sampleRateHz, timeOffsetSec, detectedNotes,
                              PitchAlgorithm::Mcomb);
        fillMissingPitchFromNeighbors(detectedNotes);
        removeNotesWithoutPitch(detectedNotes);
        return detectedNotes;
    }

} // namespace AudioAnalyzerInternal
