#ifndef RUBBERBANDPIPELINE_H
#define RUBBERBANDPIPELINE_H

#include <QCoreApplication>
#include <QString>
#include <functional>
#include <vector>

class RubberBandPipeline {
    Q_DECLARE_TR_FUNCTIONS(RubberBandPipeline)

  public:
    using ProgressCallback = std::function<void(int percent)>;

    [[nodiscard]] bool stretch(const std::vector<float> &sourceInterleaved, int channelCount,
                               int sampleRateHz, double tempoRatio,
                               std::vector<float> &outputInterleaved, QString &errorMessage,
                               const ProgressCallback &onProgress = {},
                               const std::function<bool()> &isCancelled = {});
};

#endif // RUBBERBANDPIPELINE_H
