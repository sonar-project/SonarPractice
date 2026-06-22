#ifndef AUDIOANALYZERMelodic_H
#define AUDIOANALYZERMelodic_H

#include "AudioAnalyzer.h"

#include <aubio.h>

#include <QString>
#include <vector>

namespace AudioAnalyzerInternal {

    /**
     * @brief Melodic detection: track note on/off events with aubio_notes.
     *
     * Uses yinfft pitch refinement. Best for clean practice tracks and slow melodies.
     */
    [[nodiscard]] QVector<AudioAnalyzer::Note>
    detectNotesMelodic(const std::vector<float> &monoSamples, uint_t sampleRateHz,
                       double timeOffsetSec, QString &errorMessage);

} // namespace AudioAnalyzerInternal

#endif // AUDIOANALYZERMelodic_H
