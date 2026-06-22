#ifndef AUDIOANALYZERHYBRID_H
#define AUDIOANALYZERHYBRID_H

#include "AudioAnalyzer.h"

#include <aubio.h>

#include <QString>
#include <vector>

namespace AudioAnalyzerInternal {

    /**
     * @brief Hybrid detection: rhythmic timing with melodic pitch where they agree.
     *
     * Keeps onset-based note count and replaces pitch when a melodic note overlaps
     * at least 35% of the rhythmic segment.
     */
    [[nodiscard]] QVector<AudioAnalyzer::Note>
    detectNotesHybrid(const std::vector<float> &monoSamples, uint_t sampleRateHz,
                      double timeOffsetSec, QString &errorMessage);

} // namespace AudioAnalyzerInternal

#endif // AUDIOANALYZERHYBRID_H
