/**
 * @file ReminderController.cpp
 * @brief CRUD and schedule mapping for song/day reminders in QML.
 */

#include "ReminderController.h"

#include "JournalEntry.h"
#include "JournalDayEntry.h"
#include "Reminder.h"
#include "ReminderCompletionEvaluator.h"
#include "ReminderCondition.h"
#include "PracticeTrackerService.h"
#include "ReminderConstants.h"
#include <QSet>
#include "interfaces/IReminderRepository.h"
#include "interfaces/ISongRepository.h"

ReminderController::ReminderController(IReminderRepository &reminderRepo,
                                       IReminderConditionRepository &conditionRepo,
                                       IPracticeJournalRepository &journalRepo,
                                       IReminderCompletionRepository &completionRepo,
                                       ISongRepository &songRepo,
                                       PracticeAssetController &assetController, QObject *parent)
    : QObject(parent), m_reminderRepo(reminderRepo), m_conditionRepo(conditionRepo),
      m_journalRepo(journalRepo), m_completionRepo(completionRepo), m_assetController(assetController),
      m_songReminders(reminderRepo, conditionRepo, &songRepo, this), m_dayReminders(this),
      m_filterDate(QDate::currentDate()) {}

qlonglong ReminderController::songId() const { return m_songId; }

void ReminderController::setSongId(qlonglong songId) {
    if (m_songId == songId) {
        return;
    }
    m_songId = songId;
    m_songReminders.setSongId(songId);
    emit songIdChanged();
    reloadSongReminders();
}

qlonglong ReminderController::practiceAssetId() const { return m_practiceAssetId; }

void ReminderController::setPracticeAssetId(qlonglong practiceAssetId) {
    if (m_practiceAssetId == practiceAssetId) {
        return;
    }
    m_practiceAssetId = practiceAssetId;
    m_songReminders.setPracticeAssetId(practiceAssetId);
    emit practiceAssetIdChanged();
    reloadSongReminders();
}

QDate ReminderController::filterDate() const { return m_filterDate; }

void ReminderController::setFilterDate(const QDate &date) {
    if (m_filterDate == date) {
        return;
    }
    m_filterDate = date;
    m_dayReminders.setFilterDate(date);
    emit filterDateChanged();
    reloadDayReminders();
}

ReminderListModel *ReminderController::songReminders() { return &m_songReminders; }

DayReminderModel *ReminderController::dayReminders() { return &m_dayReminders; }

int ReminderController::dayReminderCount() const { return m_dayReminders.rowCount(); }

int ReminderController::dailyReminderCount() const { return m_dayReminders.dailyCount(); }

int ReminderController::periodicReminderCount() const { return m_dayReminders.periodicCount(); }

const QString &ReminderController::statusMessage() const { return m_statusMessage; }

bool ReminderController::showAllReminders() const { return m_showAllReminders; }

void ReminderController::setShowAllReminders(bool showAll) {
    if (m_showAllReminders == showAll) {
        return;
    }
    m_showAllReminders = showAll;
    emit showAllRemindersChanged();
    reloadDayReminders();
}

void ReminderController::setStatusMessage(const QString &message) {
    if (m_statusMessage == message) {
        return;
    }
    m_statusMessage = message;
    emit statusMessageChanged();
}

void ReminderController::reloadSongReminders() {
    m_songReminders.setFilterByDate(false);
    m_songReminders.reload();
    emit remindersChanged();
}

void ReminderController::reloadDayReminders() {
    m_dayReminders.setFilterDate(m_filterDate);

    QList<ReminderDayEntry> entries;
    if (m_showAllReminders) {
        if (!m_allActiveCacheValid) {
            m_allActiveCache = m_reminderRepo.listAllActiveWithSong();
            m_allActiveCacheValid = true;
        }
        entries = m_allActiveCache;
        m_dayReminders.applyEntries(entries);
    } else {
        if (m_dayCache.contains(m_filterDate)) {
            entries = m_dayCache.value(m_filterDate);
        } else {
            entries = loadDayEntries(m_filterDate);
            m_dayCache.insert(m_filterDate, entries);
        }
        m_dayReminders.applyEntries(entries, buildCompletionList(entries, m_filterDate));
    }

    emit remindersChanged();
    emit calendarDataChanged();
}

void ReminderController::invalidateDayCache() {
    m_dayCache.clear();
    m_allActiveCache.clear();
    m_allActiveCacheValid = false;
}

QList<ReminderDayEntry> ReminderController::loadDayEntries(const QDate &date) {
    return m_reminderRepo.listForDateWithSong(date);
}

void ReminderController::applyScheduleType(const QString &scheduleType, const QDate &date,
                                           Reminder &reminder, int intervalDays, int weekday) {
    reminder.isDaily = false;
    reminder.isWeekly = false;
    reminder.isMonthly = false;
    reminder.intervalDays = 0;
    reminder.weekday = -1;
    reminder.reminderDate = date;

    if (scheduleType == QStringLiteral("daily")) {
        reminder.isDaily = true;
        return;
    }

    if (scheduleType == QStringLiteral("weekly")) {
        reminder.isWeekly = true;
        if (weekday >= ReminderConstants::kReminderSundayIndex && weekday <= 6) {
            reminder.weekday = weekday;
        } else if (date.isValid()) {
            const int qtWeekday = date.dayOfWeek();
            reminder.weekday = qtWeekday == ReminderConstants::kQtSundayDayOfWeek
                                   ? ReminderConstants::kReminderSundayIndex
                                   : qtWeekday;
        }
        return;
    }

    if (scheduleType == QStringLiteral("monthly")) {
        reminder.isMonthly = true;
        return;
    }

    if (scheduleType == QStringLiteral("interval")) {
        reminder.intervalDays = intervalDays > 0 ? intervalDays
                                                   : ReminderConstants::kDefaultIntervalDays;
        return;
    }

    reminder.reminderDate = date;
}

QString ReminderController::scheduleTypeFromReminder(const Reminder &reminder) {
    if (reminder.isDaily) {
        return QStringLiteral("daily");
    }
    if (reminder.isWeekly) {
        return QStringLiteral("weekly");
    }
    if (reminder.isMonthly) {
        return QStringLiteral("monthly");
    }
    if (reminder.intervalDays > 0) {
        return QStringLiteral("interval");
    }
    return QStringLiteral("once");
}

QVariantMap ReminderController::reminderEditPayload(qlonglong reminderId) const {
    QVariantMap payload;
    const std::optional<Reminder> reminder = m_reminderRepo.getReminder(reminderId);
    if (!reminder.has_value()) {
        return payload;
    }

    payload.insert(QStringLiteral("reminderId"), reminder->id);
    payload.insert(QStringLiteral("songId"), reminder->songId);
    payload.insert(QStringLiteral("practiceAssetId"), reminder->practiceAssetId);
    payload.insert(QStringLiteral("title"), reminder->title);
    payload.insert(QStringLiteral("isActive"), reminder->isActive);
    payload.insert(QStringLiteral("scheduleType"), scheduleTypeFromReminder(*reminder));
    payload.insert(QStringLiteral("reminderDate"), reminder->reminderDate);
    payload.insert(QStringLiteral("intervalDays"), reminder->intervalDays);
    payload.insert(QStringLiteral("weekday"), reminder->weekday);

    const QList<ReminderCondition> conditions = m_conditionRepo.listForReminder(reminderId);
    if (!conditions.isEmpty()) {
        const ReminderCondition &condition = conditions.first();
        payload.insert(QStringLiteral("conditionStartBar"), condition.startBar);
        payload.insert(QStringLiteral("conditionEndBar"), condition.endBar);
        payload.insert(QStringLiteral("conditionMinBpm"), condition.minBpm);
        payload.insert(QStringLiteral("conditionMinMinutes"), condition.minMinutes);
    }

    return payload;
}

bool ReminderController::createReminder(const QString &title, const QDate &date,
                                        const QString &scheduleType, bool isActive, int startBar,
                                        int endBar, int minBpm, int minMinutes, int intervalDays,
                                        int weekday) {
    if (m_songId <= 0) {
        setStatusMessage(tr("No exercise selected."));
        return false;
    }

    if (m_practiceAssetId <= 0) {
        setStatusMessage(tr("No practice material selected."));
        return false;
    }

    Reminder reminder;
    reminder.songId = m_songId;
    reminder.practiceAssetId = m_practiceAssetId;
    reminder.title = title.trimmed();
    reminder.isActive = isActive;
    applyScheduleType(scheduleType, date, reminder, intervalDays, weekday);

    const std::optional<qlonglong> id = m_reminderRepo.createReminder(reminder);
    if (!id.has_value()) {
        setStatusMessage(tr("Could not save reminder."));
        return false;
    }

    saveCondition(*id, startBar, endBar, minBpm, minMinutes);

    setStatusMessage(tr("Reminder saved."));
    invalidateDayCache();
    reloadSongReminders();
    reloadDayReminders();
    return true;
}

bool ReminderController::updateReminder(qlonglong reminderId, const QString &title,
                                        const QDate &date, const QString &scheduleType,
                                        bool isActive, int intervalDays, int weekday) {
    const std::optional<Reminder> existing = m_reminderRepo.getReminder(reminderId);
    if (!existing.has_value()) {
        setStatusMessage(tr("Reminder not found."));
        return false;
    }

    Reminder reminder = *existing;
    reminder.title = title.trimmed();
    reminder.isActive = isActive;
    applyScheduleType(scheduleType, date, reminder, intervalDays, weekday);

    if (!m_reminderRepo.updateReminder(reminder)) {
        setStatusMessage(tr("Could not update reminder."));
        return false;
    }

    setStatusMessage(tr("Reminder updated."));
    invalidateDayCache();
    reloadSongReminders();
    reloadDayReminders();
    return true;
}

bool ReminderController::deleteReminder(qlonglong reminderId) {
    m_conditionRepo.deleteConditionsForReminder(reminderId);

    if (!m_reminderRepo.deleteReminder(reminderId)) {
        setStatusMessage(tr("Could not delete reminder."));
        return false;
    }

    setStatusMessage(tr("Reminder deleted."));
    invalidateDayCache();
    reloadSongReminders();
    reloadDayReminders();
    return true;
}

bool ReminderController::saveCondition(qlonglong reminderId, int startBar, int endBar, int minBpm,
                                       int minMinutes) {
    if (reminderId <= 0) {
        return false;
    }

    m_conditionRepo.deleteConditionsForReminder(reminderId);

    const bool hasValues = startBar > 0 || endBar > 0 || minBpm > 0 || minMinutes > 0;
    if (!hasValues) {
        reloadSongReminders();
        return true;
    }

    ReminderCondition condition;
    condition.reminderId = reminderId;
    condition.startBar = startBar;
    condition.endBar = endBar;
    condition.minBpm = minBpm;
    condition.minMinutes = minMinutes;

    if (!m_conditionRepo.createCondition(condition).has_value()) {
        setStatusMessage(tr("Could not save condition."));
        return false;
    }

    setStatusMessage(tr("Condition saved."));
    reloadSongReminders();
    return true;
}

QVariantMap ReminderController::practiceAssetPayload(qlonglong assetId) const {
    QVariantMap map;
    const auto asset = m_assetController.loadAsset(assetId);
    if (!asset.has_value()) {
        return map;
    }

    map.insert(QStringLiteral("assetId"), asset->id);
    map.insert(QStringLiteral("songId"), asset->songId);
    map.insert(QStringLiteral("guitarProId"), asset->guitarProId);
    map.insert(QStringLiteral("audioId"), asset->audioId);
    map.insert(QStringLiteral("videoId"), asset->videoId);
    map.insert(QStringLiteral("imageId"), asset->imageId);
    return map;
}

QString ReminderController::statusKey(ReminderCompletionEvaluator::Status status) {
    switch (status) {
    case ReminderCompletionEvaluator::Status::Completed:
        return QStringLiteral("done");
    case ReminderCompletionEvaluator::Status::Partial:
        return QStringLiteral("partial");
    case ReminderCompletionEvaluator::Status::ManuallyCompleted:
        return QStringLiteral("manual");
    case ReminderCompletionEvaluator::Status::NotPracticed:
    default:
        return QStringLiteral("pending");
    }
}

QList<JournalEntry> ReminderController::journalEntriesForReminder(const ReminderDayEntry &entry,
                                                                  const QDate &date) const {
    if (entry.reminder.practiceAssetId > 0) {
        return m_journalRepo.listForAssetAndDate(entry.reminder.practiceAssetId, date);
    }
    return m_journalRepo.listForSongAndDate(entry.reminder.songId, date);
}

DayReminderModel::DayReminderCompletion
ReminderController::completionForReminder(const ReminderDayEntry &entry,
                                          const QDate &date) const {
    DayReminderModel::DayReminderCompletion info;
    const QList<JournalEntry> entries = journalEntriesForReminder(entry, date);
    info.hasJournalEntry = !entries.isEmpty();

    ReminderCondition condition;
    if (entry.reminder.id > 0) {
        const QList<ReminderCondition> conditions =
            m_conditionRepo.listForReminder(entry.reminder.id);
        if (!conditions.isEmpty()) {
            condition = conditions.first();
        }
    }

    const bool manuallyAccepted =
        m_completionRepo.isAccepted(entry.reminder.id, date) && info.hasJournalEntry;
    const ReminderCompletionEvaluator::Result result =
        ReminderCompletionEvaluator::evaluate(entries, condition, manuallyAccepted);

    info.status = statusKey(result.status);
    info.detail = result.detailMessage;
    info.isCompleted = result.status == ReminderCompletionEvaluator::Status::Completed ||
                       result.status == ReminderCompletionEvaluator::Status::ManuallyCompleted;
    return info;
}

QList<DayReminderModel::DayReminderCompletion>
ReminderController::buildCompletionList(const QList<ReminderDayEntry> &entries,
                                      const QDate &date) const {
    QList<DayReminderModel::DayReminderCompletion> completion;
    completion.reserve(entries.size());
    for (const ReminderDayEntry &entry : entries) {
        completion.append(completionForReminder(entry, date));
    }
    return completion;
}

ReminderController::DayCompletionSummary ReminderController::summarizeDay(const QDate &date) const {
    DayCompletionSummary summary;
    if (!date.isValid()) {
        summary.status = QStringLiteral("empty");
        return summary;
    }

    const QList<JournalDayEntry> journalEntries = m_journalRepo.listDayEntriesWithSong(date);
    summary.practiced = !journalEntries.isEmpty();

    const QList<ReminderDayEntry> reminders = m_reminderRepo.listForDateWithSong(date);
    summary.dueCount = reminders.size();

    for (const ReminderDayEntry &entry : reminders) {
        const DayReminderModel::DayReminderCompletion info = completionForReminder(entry, date);
        if (info.isCompleted) {
            ++summary.completedCount;
        } else if (info.status == QStringLiteral("partial")) {
            ++summary.partialCount;
        }
    }

    if (summary.dueCount > 0) {
        if (summary.completedCount == summary.dueCount) {
            summary.status = QStringLiteral("complete");
        } else if (summary.partialCount > 0) {
            summary.status = QStringLiteral("partial");
        } else if (summary.completedCount > 0) {
            summary.status = QStringLiteral("mixed");
        } else {
            summary.status = QStringLiteral("due");
        }
    } else if (summary.practiced) {
        summary.status = QStringLiteral("practiced");
    } else {
        summary.status = QStringLiteral("empty");
    }

    return summary;
}

QVariantList ReminderController::monthCalendarSummary(int year, int month) const {
    QVariantList days;
    if (year < 1 || month < 1 || month > 12) {
        return days;
    }

    const QDate monthStart(year, month, 1);
    const int daysInMonth = monthStart.daysInMonth();

    for (int day = 1; day <= daysInMonth; ++day) {
        const QDate date(year, month, day);
        const DayCompletionSummary summary = summarizeDay(date);

        QVariantMap map;
        map.insert(QStringLiteral("day"), day);
        map.insert(QStringLiteral("status"), summary.status);
        map.insert(QStringLiteral("practiced"), summary.practiced);
        map.insert(QStringLiteral("dueCount"), summary.dueCount);
        map.insert(QStringLiteral("completedCount"), summary.completedCount);
        map.insert(QStringLiteral("partialCount"), summary.partialCount);
        days.append(map);
    }

    return days;
}

QVariantList ReminderController::dayPracticeDetails(const QDate &date) const {
    QVariantList items;
    if (!date.isValid()) {
        return items;
    }

    QSet<qlonglong> songsWithReminders;
    const QList<ReminderDayEntry> reminders = m_reminderRepo.listForDateWithSong(date);
    for (const ReminderDayEntry &entry : reminders) {
        songsWithReminders.insert(entry.reminder.songId);

        const DayReminderModel::DayReminderCompletion info = completionForReminder(entry, date);
        QVariantMap map;
        map.insert(QStringLiteral("kind"), QStringLiteral("reminder"));
        map.insert(QStringLiteral("reminderId"), entry.reminder.id);
        map.insert(QStringLiteral("songId"), entry.reminder.songId);
        map.insert(QStringLiteral("songTitle"), entry.songTitle);
        map.insert(QStringLiteral("baseBpm"), entry.baseBpm);
        map.insert(QStringLiteral("practiceAssetId"), entry.reminder.practiceAssetId);
        map.insert(QStringLiteral("completionStatus"), info.status);
        map.insert(QStringLiteral("completionDetail"), info.detail);
        map.insert(QStringLiteral("isCompleted"), info.isCompleted);
        items.append(map);
    }

    const QList<JournalDayEntry> journalEntries = m_journalRepo.listDayEntriesWithSong(date);
    for (const JournalDayEntry &entry : journalEntries) {
        if (songsWithReminders.contains(entry.songId)) {
            continue;
        }

        QVariantMap map;
        map.insert(QStringLiteral("kind"), QStringLiteral("journal"));
        map.insert(QStringLiteral("entryId"), entry.entryId);
        map.insert(QStringLiteral("songId"), entry.songId);
        map.insert(QStringLiteral("songTitle"), entry.songTitle);
        map.insert(QStringLiteral("baseBpm"), entry.baseBpm);
        map.insert(QStringLiteral("practiceAssetId"), entry.assetId);
        map.insert(QStringLiteral("completionStatus"), QStringLiteral("practiced"));
        map.insert(QStringLiteral("completionDetail"),
                   tr("Practiced (%1 min)")
                       .arg(PracticeTrackerService::durationMinutesFromSeconds(
                           entry.durationSeconds)));
        map.insert(QStringLiteral("isCompleted"), true);
        items.append(map);
    }

    return items;
}

bool ReminderController::acceptPartialCompletion(qlonglong reminderId, const QDate &date) {
    if (reminderId <= 0 || !date.isValid()) {
        return false;
    }

    if (!m_completionRepo.setAccepted(reminderId, date, true)) {
        setStatusMessage(tr("Could not save completion."));
        return false;
    }

    invalidateDayCache();
    reloadDayReminders();
    emit calendarDataChanged();
    return true;
}
