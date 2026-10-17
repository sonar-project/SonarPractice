#ifndef PRACTICESYNCMARKER_H
#define PRACTICESYNCMARKER_H

#include <QtGlobal>

/**
 * @brief One hand-placed pin: a score position matches this audio-file time.
 *
 * Tick is the AlphaTab playback tick at the pause. Bar and beat are the
 * 1-based labels shown in the player. Audio time is source milliseconds.
 */
struct PracticeSyncMarker {
    qlonglong id{};
    qlonglong assetId{};
    int bar{1};
    int beat{1};
    int tick{0};
    qint64 audioMs{};
};

#endif // PRACTICESYNCMARKER_H
