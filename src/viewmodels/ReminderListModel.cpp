/**
 * @file ReminderListModel.cpp
 * @brief Loads reminders and builds schedule labels for QML.
 */

#include "ReminderListModel.h"

#include "Reminder.h"
#include "ReminderCondition.h"
#include "interfaces/IReminderConditionRepository.h"
#include "interfaces/IReminderRepository.h"
#include "interfaces/ISongRepository.h"

#include <QDate>

ReminderListModel::ReminderListModel(IReminderRepository &reminderRepo,
                                     IReminderConditionRepository &conditionRepo,
                                     ISongRepository *songRepo, QObject *parent)
    : QAbstractListModel(parent), m_reminderRepo(reminderRepo), m_conditionRepo(conditionRepo),
      m_songRepo(songRepo) {}

int ReminderListModel::rowCount(const QModelIndex &parent) const {
    if (parent.isValid()) {
        return 0;
    }
    return static_cast<int>(m_rows.size());
}

QVariant ReminderListModel::data(const QModelIndex &index, int role) const {
    if (!index.isValid() || index.row() < 0 || index.row() >= m_rows.size()) {
        return {};
    }

    const ReminderRow &row = m_rows.at(index.row());

    switch (role) {
    case ReminderIdRole:
        return row.id;
    case SongIdRole:
        return row.songId;
    case TitleRole:
        return row.title;
    case ReminderDateRole:
        return row.reminderDate;
    case ScheduleLabelRole:
        return row.scheduleLabel;
    case IsDailyRole:
        return row.isDaily;
    case IsWeeklyRole:
        return row.isWeekly;
    case IsMonthlyRole:
        return row.isMonthly;
    case IsActiveRole:
        return row.isActive;
    case IntervalDaysRole:
        return row.intervalDays;
    case WeekdayRole:
        return row.weekday;
    case HasConditionRole:
        return row.hasCondition;
    case ConditionStartBarRole:
        return row.conditionStartBar;
    case ConditionEndBarRole:
        return row.conditionEndBar;
    case ConditionMinBpmRole:
        return row.conditionMinBpm;
    case ConditionMinMinutesRole:
        return row.conditionMinMinutes;
    default:
        return {};
    }
}

QHash<int, QByteArray> ReminderListModel::roleNames() const {
    return {
        {ReminderIdRole, "reminderId"},
        {SongIdRole, "songId"},
        {TitleRole, "title"},
        {ReminderDateRole, "reminderDate"},
        {ScheduleLabelRole, "scheduleLabel"},
        {IsDailyRole, "isDaily"},
        {IsWeeklyRole, "isWeekly"},
        {IsMonthlyRole, "isMonthly"},
        {IsActiveRole, "isActive"},
        {IntervalDaysRole, "intervalDays"},
        {WeekdayRole, "weekday"},
        {HasConditionRole, "hasCondition"},
        {ConditionStartBarRole, "conditionStartBar"},
        {ConditionEndBarRole, "conditionEndBar"},
        {ConditionMinBpmRole, "conditionMinBpm"},
        {ConditionMinMinutesRole, "conditionMinMinutes"},
    };
}

void ReminderListModel::setSongId(qlonglong songId) { m_songId = songId; }

void ReminderListModel::setPracticeAssetId(qlonglong practiceAssetId) {
    m_practiceAssetId = practiceAssetId;
}

void ReminderListModel::setFilterDate(const QDate &date) { m_filterDate = date; }

void ReminderListModel::setFilterByDate(bool enabled) { m_filterByDate = enabled; }

void ReminderListModel::reload() {
    beginResetModel();
    m_rows.clear();

    QList<Reminder> reminders;
    if (m_filterByDate && m_filterDate.isValid()) {
        reminders = m_reminderRepo.listForDate(m_filterDate);
    } else if (m_practiceAssetId > 0) {
        reminders = m_reminderRepo.listForPracticeAsset(m_practiceAssetId);
    } else if (m_songId > 0) {
        reminders = m_reminderRepo.listForSong(m_songId);
    }

    m_rows.reserve(reminders.size());

    for (const Reminder &reminder : reminders) {
        ReminderRow row;
        row.id = reminder.id;
        row.songId = reminder.songId;
        row.title = reminder.title;
        row.reminderDate = reminder.reminderDate;
        row.scheduleLabel = buildScheduleLabel(reminder);
        row.isDaily = reminder.isDaily;
        row.isWeekly = reminder.isWeekly;
        row.isMonthly = reminder.isMonthly;
        row.isActive = reminder.isActive;
        row.intervalDays = reminder.intervalDays;
        row.weekday = reminder.weekday;

        if (reminder.id > 0) {
            const QList<ReminderCondition> conditions =
                m_conditionRepo.listForReminder(reminder.id);
            if (!conditions.isEmpty()) {
                const ReminderCondition &condition = conditions.first();
                row.hasCondition = true;
                row.conditionStartBar = condition.startBar;
                row.conditionEndBar = condition.endBar;
                row.conditionMinBpm = condition.minBpm;
                row.conditionMinMinutes = condition.minMinutes;
            }
        }

        m_rows.append(row);
    }

    endResetModel();
}
