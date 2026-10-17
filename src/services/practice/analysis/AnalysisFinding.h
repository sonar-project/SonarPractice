#ifndef ANALYSISFINDING_H
#define ANALYSISFINDING_H

#include <QString>

#include <QtGlobal>

/**
 * @brief One structured observation from an analyser module (Phase 0 shape).
 *
 * Consumers (BPM progression, coach) will read these in later phases.
 */
struct AnalysisFinding {
    QString kind{};
    QString messageKey{};
    double severity{};
    int startBar{};
    int endBar{};
    qint64 positionMs{};
};

#endif // ANALYSISFINDING_H
