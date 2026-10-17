#ifndef JOURNALENTRY_H
#define JOURNALENTRY_H

#include <QDateTime>
#include <QtGlobal>

class QString;

struct JournalEntry {
    qlonglong id{};
    qlonglong userId{1}; // user 1 is always admin
    qlonglong assetId{};
    QDateTime practiceDate{};
    int startBar{};
    int endBar{};
    int practicedBpm{};
    int totalReps{};
    int successfulStreaks{};
    int durationSeconds{};
    qlonglong noticeId{};
};

#endif // JOURNALENTRY_H
