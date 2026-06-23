#ifndef JOURNALDAYENTRY_H
#define JOURNALDAYENTRY_H

#include <QString>
#include <QtGlobal>

/**
 * @brief One practice journal row joined with song metadata for dashboard views.
 */
struct JournalDayEntry {
    qlonglong entryId{};
    qlonglong assetId{};
    qlonglong songId{};
    QString songTitle{};
    int baseBpm{};
    int durationSeconds{};
    int startBar{};
    int endBar{};
    int practicedBpm{};
};

#endif // JOURNALDAYENTRY_H
