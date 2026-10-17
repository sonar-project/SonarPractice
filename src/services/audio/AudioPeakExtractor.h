#ifndef AUDIOPEAKEXTRACTOR_H
#define AUDIOPEAKEXTRACTOR_H

#include <QVector>
#include <vector>

class AudioPeakExtractor {
  public:
    [[nodiscard]] static QVector<float> computePeaks(const std::vector<float> &interleavedSamples,
                                                     int channelCount, int bucketCount);
};

#endif // AUDIOPEAKEXTRACTOR_H
