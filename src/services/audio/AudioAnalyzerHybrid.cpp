#include "AudioAnalyzerHybrid.h"

#include "AudioAnalyzerCommon.h"
#include "AudioAnalyzerMelodic.h"
#include "AudioAnalyzerRhythm.h"

namespace AudioAnalyzerInternal {

    /**
     * @brief Uses rhythmic timing and copies melodic pitch when segments overlap enough.
     */
    QVector<AudioAnalyzer::Note>
    detectNotesHybrid(const std::vector<float> &monoSamples, uint_t sampleRateHz,
                      double timeOffsetSec, QString &errorMessage) {
        QString rhythmError;
        QVector<AudioAnalyzer::Note> rhythmicNotes =
            detectNotesRhythmic(monoSamples, sampleRateHz, timeOffsetSec, rhythmError);
        if (!rhythmError.isEmpty()) {
            errorMessage = rhythmError;
            return {};
        }

        QString melodicError;
        const QVector<AudioAnalyzer::Note> melodicNotes =
            detectNotesMelodic(monoSamples, sampleRateHz, timeOffsetSec, melodicError);
        if (!melodicError.isEmpty()) {
            errorMessage = melodicError;
            return {};
        }

        if (rhythmicNotes.isEmpty()) {
            return melodicNotes;
        }
        if (melodicNotes.isEmpty()) {
            return rhythmicNotes;
        }

        for (AudioAnalyzer::Note &rhythmNote : rhythmicNotes) {
            const double rhythmDuration = rhythmNote.endSec - rhythmNote.startSec;
            if (rhythmDuration <= 0.0) {
                continue;
            }

            double bestOverlap = 0.0;
            double melodicFrequencyHz = 0.0;
            for (const AudioAnalyzer::Note &melodicNote : melodicNotes) {
                const double overlap = overlapDurationSec(rhythmNote, melodicNote);
                if (overlap > bestOverlap) {
                    bestOverlap = overlap;
                    melodicFrequencyHz = melodicNote.frequencyHz;
                }
            }

            if (bestOverlap >= rhythmDuration * 0.35 && melodicFrequencyHz > 0.0) {
                rhythmNote.frequencyHz = melodicFrequencyHz;
            }
        }

        fillMissingPitchFromNeighbors(rhythmicNotes);
        removeNotesWithoutPitch(rhythmicNotes);
        return rhythmicNotes;
    }

} // namespace AudioAnalyzerInternal
