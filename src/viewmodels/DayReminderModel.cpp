/**
 * @file DayReminderModel.cpp
 * @brief Dashboard list model for reminders on the selected day.
 */

#include "DayReminderModel.h"
#include "ReminderListModel.h"

#include <QHash>
#include <QVariantMap>

DayReminderModel::DayReminderModel(QObject *parent) : QAbstractListModel(parent) {}

int DayReminderModel::rowCount(const QModelIndex &parent) const {
    if (parent.isValid()) {
        return 0;
    }
    return static_cast<int>(m_rows.size());
}

QVariant DayReminderModel::data(const QModelIndex &index, int role) const {
    if (!index.isValid() || index.row() < 0 || index.row() >= m_rows.size()) {
        return {};
    }

    const DayReminderRow &row = m_rows.at(index.row());

    switch (static_cast<Roles>(role)) {
    case Roles::ReminderIdRole:
        return row.reminderId;
    case Roles::SongIdRole:
        return row.songId;
    case Roles::ReminderTitleRole:
        return row.reminderTitle;
    case Roles::SongTitleRole:
        return row.songTitle;
    case Roles::ScheduleLabelRole:
        return row.scheduleLabel;
    case Roles::IsDailyRole:
        return row.isDaily;
    case Roles::IsWeeklyRole:
        return row.isWeekly;
    case Roles::IsMonthlyRole:
        return row.isMonthly;
    case Roles::IntervalDaysRole:
        return row.intervalDays;
    case Roles::BaseBpmRole:
        return row.baseBpm;
    case Roles::PracticeAssetIdRole:
        return row.practiceAssetId;
    case Roles::CompletionStatusRole:
        return row.completionStatus;
    case Roles::CompletionDetailRole:
        return row.completionDetail;
    case Roles::IsCompletedRole:
        return row.isCompleted;
    case Roles::HasJournalEntryRole:
        return row.hasJournalEntry;
    default:
        return {};
    }
}

QHash<int, QByteArray> DayReminderModel::roleNames() const {
    return {
        {static_cast<int>(Roles::ReminderIdRole), "reminderId"},
        {static_cast<int>(Roles::SongIdRole), "songId"},
        {static_cast<int>(Roles::ReminderTitleRole), "reminderTitle"},
        {static_cast<int>(Roles::SongTitleRole), "songTitle"},
        {static_cast<int>(Roles::ScheduleLabelRole), "scheduleLabel"},
        {static_cast<int>(Roles::IsDailyRole), "isDaily"},
        {static_cast<int>(Roles::IsWeeklyRole), "isWeekly"},
        {static_cast<int>(Roles::IsMonthlyRole), "isMonthly"},
        {static_cast<int>(Roles::IntervalDaysRole), "intervalDays"},
        {static_cast<int>(Roles::BaseBpmRole), "baseBpm"},
        {static_cast<int>(Roles::PracticeAssetIdRole), "practiceAssetId"},
        {static_cast<int>(Roles::CompletionStatusRole), "completionStatus"},
        {static_cast<int>(Roles::CompletionDetailRole), "completionDetail"},
        {static_cast<int>(Roles::IsCompletedRole), "isCompleted"},
        {static_cast<int>(Roles::HasJournalEntryRole), "hasJournalEntry"},
    };
}

int DayReminderModel::dailyCount() const { return m_dailyCount; }

int DayReminderModel::periodicCount() const { return m_periodicCount; }

QVariantList DayReminderModel::openSongBlocks() const { return m_openBlocks; }

QVariantList DayReminderModel::completedSongBlocks() const { return m_completedBlocks; }

int DayReminderModel::completedExerciseCount() const { return m_completedExerciseCount; }

void DayReminderModel::setFilterDate(const QDate &date) { m_filterDate = date; }

void DayReminderModel::applyEntries(const QList<ReminderDayEntry> &entries,
                                    const QList<DayReminderCompletion> &completion) {
    beginResetModel();
    m_rows.clear();
    m_rows.reserve(entries.size());

    for (int i = 0; i < entries.size(); ++i) {
        DayReminderRow row = rowFromEntry(entries.at(i));
        if (i < completion.size()) {
            const DayReminderCompletion &info = completion.at(i);
            row.completionStatus = info.status;
            row.completionDetail = info.detail;
            row.isCompleted = info.isCompleted;
            row.hasJournalEntry = info.hasJournalEntry;
        }
        m_rows.append(row);
    }

    rebuildCounts();
    rebuildSongBlocks();
    endResetModel();
    emit countsChanged();
}

void DayReminderModel::clearRows() {
    beginResetModel();
    m_rows.clear();
    m_openBlocks.clear();
    m_completedBlocks.clear();
    m_dailyCount = 0;
    m_periodicCount = 0;
    m_completedExerciseCount = 0;
    endResetModel();
    emit countsChanged();
}

DayReminderModel::DayReminderRow DayReminderModel::rowFromEntry(const ReminderDayEntry &entry) {
    DayReminderRow row;
    row.reminderId = entry.reminder.id;
    row.songId = entry.reminder.songId;
    row.groupSongId = entry.displaySongId > 0 ? entry.displaySongId : entry.reminder.songId;
    row.practiceAssetId = entry.reminder.practiceAssetId;
    row.reminderTitle = entry.reminder.title;
    row.scheduleLabel = ReminderListModel::buildScheduleLabel(entry.reminder);
    row.isDaily = entry.reminder.isDaily;
    row.isWeekly = entry.reminder.isWeekly;
    row.isMonthly = entry.reminder.isMonthly;
    row.intervalDays = entry.reminder.intervalDays;
    row.baseBpm = entry.baseBpm;

    if (!entry.songTitle.isEmpty()) {
        row.songTitle = entry.songTitle;
    } else {
        row.songTitle = tr("Exercise");
    }

    return row;
}

QString DayReminderModel::exerciseLabelFor(const DayReminderRow &row) {
    const QString song = row.songTitle.trimmed();
    const QString title = row.reminderTitle.trimmed();
    const QString prefix = song + QStringLiteral(" - ");
    if (!song.isEmpty() && title.startsWith(prefix, Qt::CaseInsensitive)) {
        return title.mid(prefix.size()).trimmed();
    }
    if (title.isEmpty() || title.compare(song, Qt::CaseInsensitive) == 0) {
        return row.scheduleLabel;
    }
    return title;
}

QVariantMap DayReminderModel::exerciseMap(const DayReminderRow &row) {
    QVariantMap map;
    map.insert(QStringLiteral("reminderId"), row.reminderId);
    map.insert(QStringLiteral("songId"), row.songId);
    map.insert(QStringLiteral("songTitle"), row.songTitle);
    map.insert(QStringLiteral("reminderTitle"), row.reminderTitle);
    map.insert(QStringLiteral("exerciseLabel"), exerciseLabelFor(row));
    map.insert(QStringLiteral("scheduleLabel"), row.scheduleLabel);
    map.insert(QStringLiteral("baseBpm"), row.baseBpm);
    map.insert(QStringLiteral("practiceAssetId"), row.practiceAssetId);
    map.insert(QStringLiteral("completionStatus"), row.completionStatus);
    map.insert(QStringLiteral("completionDetail"), row.completionDetail);
    map.insert(QStringLiteral("isCompleted"), row.isCompleted);
    map.insert(QStringLiteral("hasJournalEntry"), row.hasJournalEntry);
    return map;
}

void DayReminderModel::rebuildCounts() {
    m_dailyCount = 0;
    m_periodicCount = 0;

    for (const DayReminderRow &row : m_rows) {
        if (row.isDaily) {
            ++m_dailyCount;
        } else {
            ++m_periodicCount;
        }
    }
}

void DayReminderModel::rebuildSongBlocks() {
    m_openBlocks.clear();
    m_completedBlocks.clear();
    m_completedExerciseCount = 0;

    QHash<qlonglong, int> openIndex;
    QHash<qlonglong, int> completedIndex;

    const auto appendExercise = [](QVariantList &blocks, QHash<qlonglong, int> &index,
                                   const DayReminderRow &row) {
        const QVariantMap exercise = exerciseMap(row);
        const qlonglong groupId = row.groupSongId > 0 ? row.groupSongId : row.songId;
        const int existing = index.value(groupId, -1);
        if (existing < 0) {
            QVariantMap block;
            block.insert(QStringLiteral("songId"), groupId);
            block.insert(QStringLiteral("songTitle"), row.songTitle);
            block.insert(QStringLiteral("baseBpm"), row.baseBpm);
            block.insert(QStringLiteral("exercises"), QVariantList{exercise});
            index.insert(groupId, static_cast<int>(blocks.size()));
            blocks.append(block);
            return;
        }

        QVariantMap block = blocks.at(existing).toMap();
        QVariantList exercises = block.value(QStringLiteral("exercises")).toList();
        exercises.append(exercise);
        block.insert(QStringLiteral("exercises"), exercises);
        blocks[existing] = block;
    };

    for (const DayReminderRow &row : m_rows) {
        if (row.isCompleted) {
            appendExercise(m_completedBlocks, completedIndex, row);
            ++m_completedExerciseCount;
        } else {
            appendExercise(m_openBlocks, openIndex, row);
        }
    }
}
