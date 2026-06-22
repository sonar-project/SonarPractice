#ifndef AUDIOANALYZERRHYTHM_H
#define AUDIOANALYZERRHYTHM_H

#include "AudioAnalyzer.h"

#include <aubio.h>

#include <QString>
#include <vector>

namespace AudioAnalyzerInternal {

    /**
     * @brief Rhythmic detection: find pick strokes, then measure pitch per segment.
     *
     * Uses combined onset detection and mcomb pitch. Best for gallops, palm mutes,
     * and fast repeated notes.
     */
    [[nodiscard]] QVector<AudioAnalyzer::Note>
    detectNotesRhythmic(const std::vector<float> &monoSamples, uint_t sampleRateHz,
                        double timeOffsetSec, QString &errorMessage);

} // namespace AudioAnalyzerInternal

#endif // AUDIOANALYZERRHYTHM_H
