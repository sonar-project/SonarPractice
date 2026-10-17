#ifndef REMINDERCONDITION_H
#define REMINDERCONDITION_H

#include <QtGlobal>

struct ReminderCondition {
    qlonglong id{};
    qlonglong reminderId{};
    int startBar{};
    int endBar{};
    int minBpm{};
    int minMinutes{};
    bool countInEnabled{};
    /** Count in on the first Play only. Loops continue without another count-in. */
    bool countInOneTime{};
    bool metronomeEnabled{};
    int transposeSemitones{};
    /** Index in the Guitar Pro file. -1 means the player default. */
    int trackIndex{-1};
};

#endif // REMINDERCONDITION_H
