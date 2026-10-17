#ifndef PRACTICESESSIONCONFIG_H
#define PRACTICESESSIONCONFIG_H

#include "PracticeConstants.h"
#include "PracticeSessionKind.h"

#include <QtGlobal>

/**
 * @brief Parameters for a future synced practice / probe transport session.
 *
 * Phase 0: data shape only. Engines and persistence wire up in later phases.
 * Media IDs are optional (0 = unset) so GP-only or audio-only sessions are valid.
 */
struct PracticeSessionConfig {
    PracticeSessionKind kind{PracticeSessionKind::Practice};
    qlonglong practiceAssetId{};
    qlonglong guitarProMediaId{};
    qlonglong audioMediaId{};
    /** Score bar 1 aligned to this offset in the backing file (Phase 3). */
    qint64 audioOffsetMs{};
    int targetBpm{PracticeConstants::kDefaultTargetBpm};
    int tempoPercent{PracticeConstants::kDefaultTempoPercent};
    int startBar{PracticeConstants::kDefaultStartBar};
    int endBar{PracticeConstants::kDefaultEndBar};
    int targetReps{PracticeConstants::kMaxSuccessfulStreak};

    [[nodiscard]] bool hasGuitarPro() const { return guitarProMediaId > 0; }
    [[nodiscard]] bool hasAudio() const { return audioMediaId > 0; }
    [[nodiscard]] bool hasAnyReference() const { return hasGuitarPro() || hasAudio(); }
};

#endif // PRACTICESESSIONCONFIG_H
