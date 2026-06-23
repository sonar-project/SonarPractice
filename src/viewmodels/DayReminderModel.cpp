/**
 * @file DayReminderModel.cpp
 * @brief Dashboard list model for reminders on the selected day.
 */

#include "DayReminderModel.h"
#include "ReminderListModel.h"

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
    endResetModel();
    emit countsChanged();
}

void DayReminderModel::clearRows() {
    beginResetModel();
    m_rows.clear();
    m_dailyCount = 0;
    m_periodicCount = 0;
    endResetModel();
    emit countsChanged();
}

DayReminderModel::DayReminderRow DayReminderModel::rowFromEntry(const ReminderDayEntry &entry) {
    DayReminderRow row;
    row.reminderId = entry.reminder.id;
    row.songId = entry.reminder.songId;
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
