#ifndef REMINDERDAYENTRY_H
#define REMINDERDAYENTRY_H

#include "Reminder.h"

#include <QString>

/**
 * @brief One reminder row with song title and BPM from a database JOIN.
 */
struct ReminderDayEntry {
    Reminder reminder{};
    QString songTitle{};
    int baseBpm{};
    /** Song that owns the Guitar Pro file. Falls back to the reminder's hub song. */
    qlonglong displaySongId{};
};

#endif // REMINDERDAYENTRY_H
