/**
 * @file PracticeTrackerController.cpp
 * @brief Timer, journal persistence, and training defaults for the Practice Hub.
 */

#include "PracticeTrackerController.h"

#include "ApplicationErrorLog.h"
#include "JournalEntry.h"
#include "JournalTableModel.h"
#include "PracticeConstants.h"
#include "PracticeNotice.h"
#include "PracticeTrackerService.h"
#include "Reminder.h"
#include "ReminderCondition.h"
#include "interfaces/IPracticeJournalRepository.h"
#include "interfaces/IPracticeNoticeRepository.h"
#include "interfaces/IReminderConditionRepository.h"
#include "interfaces/IReminderRepository.h"

#include <QDateTime>
#include <QTime>
#include <QTimer>

PracticeTrackerController::PracticeTrackerController(IPracticeJournalRepository &journalRepo,
                                                     IPracticeNoticeRepository &noticeRepo,
                                                     IReminderRepository &reminderRepo,
                                                     IReminderConditionRepository &conditionRepo,
                                                     ApplicationErrorLog &errorLog,
                                                     QObject *parent)
    : QObject(parent), m_service(journalRepo), m_journalModel(journalRepo, this),
      m_journalRepo(journalRepo), m_noticeRepo(noticeRepo), m_reminderRepo(reminderRepo),
      m_conditionRepo(conditionRepo), m_errorLog(errorLog),
      m_selectedDate(QDate::currentDate()) {
    auto *tickTimer = new QTimer(this);
    tickTimer->setInterval(PracticeConstants::kMillisecondsPerSecond);
    connect(tickTimer, &QTimer::timeout, this, [this]() {
        if (m_service.isTimerRunning()) {
            updateElapsedDisplay();
            emit timerTick();
        }
    });
    tickTimer->start();
}

qlonglong PracticeTrackerController::songId() const { return m_songId; }

void PracticeTrackerController::setSongId(qlonglong songId) {
    if (m_songId == songId) {
        reloadJournal();
        reloadJournalNote();
        return;
    }
    m_songId = songId;
    setStatusMessage(QString(), false);
    reloadJournalNote();
    emit songIdChanged();
}

qlonglong PracticeTrackerController::assetId() const { return m_assetId; }

void PracticeTrackerController::setAssetId(qlonglong assetId) {
    if (m_assetId == assetId) {
        reloadJournal();
        return;
    }
    m_assetId = assetId;
    m_journalModel.setAssetId(assetId);
    reloadJournal();
    emit assetIdChanged();
}

QDate PracticeTrackerController::selectedDate() const { return m_selectedDate; }

void PracticeTrackerController::setSelectedDate(const QDate &date) {
    if (m_selectedDate == date) {
        return;
    }
    m_selectedDate = date;
    m_journalModel.setSelectedDate(date);
    reloadJournalNote();
    emit selectedDateChanged();
}

int PracticeTrackerController::startBar() const { return m_startBar; }

void PracticeTrackerController::setStartBar(int value) {
    const int clamped = qMax(1, value);
    if (m_startBar == clamped && m_endBar >= clamped) {
        return;
    }
    m_startBar = clamped;
    if (m_endBar < m_startBar) {
        m_endBar = m_startBar;
    }
    emit paramsChanged();
}

int PracticeTrackerController::endBar() const { return m_endBar; }

void PracticeTrackerController::setEndBar(int value) {
    const int clamped = qMax(m_startBar, value);
    if (m_endBar == clamped) {
        return;
    }
    m_endBar = clamped;
    emit paramsChanged();
}

int PracticeTrackerController::targetBpm() const { return m_targetBpm; }

void PracticeTrackerController::setTargetBpm(int value) {
    if (m_targetBpm == value) {
        return;
    }
    m_targetBpm = value;
    emit paramsChanged();
}

int PracticeTrackerController::totalReps() const { return m_totalReps; }

void PracticeTrackerController::setTotalReps(int value) {
    if (m_totalReps == value) {
        return;
    }
    m_totalReps = value;
    emit paramsChanged();
}

int PracticeTrackerController::successfulReps() const { return m_successfulReps; }

void PracticeTrackerController::setSuccessfulReps(int value) {
    if (m_successfulReps == value) {
        return;
    }
    m_successfulReps = value;
    emit paramsChanged();
}

bool PracticeTrackerController::timerRunning() const { return m_service.isTimerRunning(); }

const QString &PracticeTrackerController::elapsedDisplay() const { return m_elapsedDisplay; }

JournalTableModel *PracticeTrackerController::journalModel() { return &m_journalModel; }

const QString &PracticeTrackerController::journalMarkdown() const { return m_journalMarkdown; }

void PracticeTrackerController::setJournalMarkdown(const QString &markdown) {
    if (m_journalMarkdown == markdown) {
        return;
    }
    m_journalMarkdown = markdown;
    emit journalMarkdownChanged();
}

const QString &PracticeTrackerController::statusMessage() const { return m_statusMessage; }

bool PracticeTrackerController::statusIsError() const { return m_statusIsError; }

void PracticeTrackerController::setStatusMessage(const QString &message, const bool isError) {
    if (m_statusMessage == message && m_statusIsError == isError) {
        return;
    }
    m_statusMessage = message;
    m_statusIsError = isError;
    emit statusMessageChanged();
}

void PracticeTrackerController::reportError(const QString &context, const QString &message) {
    m_errorLog.logError(QStringLiteral("PracticeTracker.%1").arg(context), message, false);
    setStatusMessage(message, true);
}

void PracticeTrackerController::reloadJournalNote() {
    if (m_songId <= 0) {
        setJournalMarkdown(QString());
        return;
    }

    const std::optional<PracticeNotice> notice =
        m_noticeRepo.findForSongAndDate(m_songId, m_selectedDate);
    setJournalMarkdown(notice.has_value() ? notice->content : QString());
}

bool PracticeTrackerController::saveJournalNote() {
    if (m_songId <= 0) {
        return false;
    }

    if (m_noticeRepo.upsertForSongAndDate(m_songId, m_selectedDate, m_journalMarkdown)) {
        return true;
    }

    reportError(QStringLiteral("saveJournalNote"), tr("Could not save journal note."));
    return false;
}

bool PracticeTrackerController::startTimer() {
    if (!m_service.startTimer()) {
        return false;
    }
    updateElapsedDisplay();
    emit timerStateChanged();
    emit timerTick();
    return true;
}

// Stop the timer and save the workout session to the journal.
bool PracticeTrackerController::stopAndSave() {
    if (!m_service.canSave(buildParams())) {
        reportError(QStringLiteral("stopAndSave"),
                    tr("Invalid bar range or no exercise selected."));
        return false;
    }

    const std::optional<qlonglong> entryId = m_service.stopAndSave(buildParams());
    if (!entryId.has_value()) {
        reportError(QStringLiteral("stopAndSave"), tr("Could not save practice session."));
        return false;
    }

    m_elapsedDisplay = QStringLiteral("00:00");
    emit timerStateChanged();
    emit timerTick();
    reloadJournal();
    setStatusMessage(tr("Practice session saved."), false);
    emit journalSaved(*entryId);
    return true;
}

void PracticeTrackerController::stopAndSaveWithAssetId(qlonglong assetId) {
    if (assetId > 0) {
        setAssetId(assetId);
    }
    stopAndSave();
}

void PracticeTrackerController::cancelTimer() {
    m_service.cancelTimer();
    m_elapsedDisplay = QStringLiteral("00:00");
    emit timerStateChanged();
    emit timerTick();
}

void PracticeTrackerController::reloadJournal() { m_journalModel.reload(); }

// Adopts bar range and tempo from a source reminder, last journal entry, or reminder condition.
void PracticeTrackerController::loadTrainingDefaults(int fallbackBpm, qlonglong reminderId) {
    if (m_songId <= 0) {
        return;
    }

    // Opening a specific reminder (Dashboard / calendar) must apply its condition first.
    if (const std::optional<ReminderCondition> fromReminder = conditionForReminder(reminderId);
        fromReminder.has_value()) {
        applyTrainingCondition(*fromReminder, fallbackBpm);
        return;
    }

    const std::optional<JournalEntry> lastEntry =
        m_assetId > 0 ? m_journalRepo.lastEntryForAsset(m_assetId) : std::nullopt;
    if (lastEntry.has_value()) {
        m_startBar = lastEntry->startBar;
        m_endBar = lastEntry->endBar;
        m_targetBpm = lastEntry->practicedBpm > 0 ? lastEntry->practicedBpm
                                                  : (fallbackBpm > 0 ? fallbackBpm : m_targetBpm);
        emit paramsChanged();
        return;
    }

    if (const std::optional<ReminderCondition> condition = conditionForContext();
        condition.has_value()) {
        applyTrainingCondition(*condition, fallbackBpm);
        return;
    }

    m_startBar = PracticeConstants::kDefaultStartBar;
    m_endBar = PracticeConstants::kDefaultEndBar;
    m_targetBpm = fallbackBpm > 0 ? fallbackBpm : PracticeConstants::kDefaultTargetBpm;
    emit paramsChanged();
}

void PracticeTrackerController::applyTrainingCondition(const ReminderCondition &condition,
                                                       int fallbackBpm) {
    m_startBar = condition.startBar > 0 ? condition.startBar : PracticeConstants::kDefaultStartBar;
    m_endBar = condition.endBar > 0 ? condition.endBar : PracticeConstants::kDefaultEndBar;
    if (m_endBar < m_startBar) {
        m_endBar = m_startBar;
    }
    m_targetBpm = condition.minBpm > 0 ? condition.minBpm
                                       : (fallbackBpm > 0 ? fallbackBpm : m_targetBpm);
    emit paramsChanged();
}

std::optional<ReminderCondition>
PracticeTrackerController::conditionForReminder(qlonglong reminderId) const {
    if (reminderId <= 0) {
        return std::nullopt;
    }
    const QList<ReminderCondition> conditions = m_conditionRepo.listForReminder(reminderId);
    if (conditions.isEmpty()) {
        return std::nullopt;
    }
    return conditions.first();
}

std::optional<ReminderCondition>
PracticeTrackerController::conditionFromReminders(const QList<Reminder> &reminders) const {
    for (auto it = reminders.crbegin(); it != reminders.crend(); ++it) {
        if (!it->isActive) {
            continue;
        }
        if (const std::optional<ReminderCondition> condition = conditionForReminder(it->id);
            condition.has_value()) {
            return condition;
        }
    }

    for (auto it = reminders.crbegin(); it != reminders.crend(); ++it) {
        if (const std::optional<ReminderCondition> condition = conditionForReminder(it->id);
            condition.has_value()) {
            return condition;
        }
    }

    return std::nullopt;
}

std::optional<ReminderCondition> PracticeTrackerController::conditionForContext() const {
    // PracticeHub reminders are material-linked; song-level listForSong excludes those.
    if (m_assetId > 0) {
        if (const std::optional<ReminderCondition> fromAsset =
                conditionFromReminders(m_reminderRepo.listForPracticeAsset(m_assetId));
            fromAsset.has_value()) {
            return fromAsset;
        }
    }
    return conditionFromReminders(m_reminderRepo.listForSong(m_songId));
}

// Formats the elapsed practice time as MM:SS.
void PracticeTrackerController::updateElapsedDisplay() {
    const int totalSeconds = m_service.elapsedSeconds();
    const int minutes = totalSeconds / PracticeConstants::kSecondsPerMinute;
    const int seconds = totalSeconds % PracticeConstants::kSecondsPerMinute;
    m_elapsedDisplay = QStringLiteral("%1:%2")
                           .arg(minutes, PracticeConstants::kTimeDisplayFieldWidth,
                                PracticeConstants::kDecimalRadix, QLatin1Char('0'))
                           .arg(seconds, PracticeConstants::kTimeDisplayFieldWidth,
                                PracticeConstants::kDecimalRadix, QLatin1Char('0'));
}

PracticeTrackerParams PracticeTrackerController::buildParams() const {
    PracticeTrackerParams params;
    params.assetId = m_assetId;
    params.startBar = m_startBar;
    params.endBar = m_endBar;
    params.targetBpm = m_targetBpm;
    params.totalReps = m_totalReps;
    params.successfulReps = m_successfulReps;
    params.practiceDate = QDateTime(m_selectedDate, QTime::currentTime());
    return params;
}
