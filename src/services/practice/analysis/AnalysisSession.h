#ifndef ANALYSISSESSION_H
#define ANALYSISSESSION_H

#include "PracticeSessionConfig.h"

#include <QByteArray>
#include <QtGlobal>

/**
 * @brief Capture window and references for analyser modules.
 *
 * Phase 0: PCM and timing fields are placeholders; Rec fills them in Phase 5.
 */
struct AnalysisSession {
    PracticeSessionConfig config{};
    /** Interleaved or mono PCM captured between Play and Stop (later). */
    QByteArray recordedPcm{};
    int sampleRateHz{};
    int channelCount{};
    qint64 captureStartMs{};
    qint64 captureEndMs{};
};

#endif // ANALYSISSESSION_H
