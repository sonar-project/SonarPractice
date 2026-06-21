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
    if (m_startBar == value) {
        return;
    }
    m_startBar = value;
    emit paramsChanged();
}

int PracticeTrackerController::endBar() const { return m_endBar; }

void PracticeTrackerController::setEndBar(int value) {
    if (m_endBar == value) {
        return;
    }
    m_endBar = value;
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
    emit saveFailed(message);
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

// Adopts bar range and tempo from the last journal entry or an active reminder condition.
void PracticeTrackerController::loadTrainingDefaults(int fallbackBpm) {
    if (m_songId <= 0) {
        return;
    }

    const std::optional<JournalEntry> lastEntry = m_journalRepo.lastEntryForAsset(m_assetId);
    if (lastEntry.has_value()) {
        m_startBar = lastEntry->startBar;
        m_endBar = lastEntry->endBar;
        m_targetBpm = lastEntry->practicedBpm > 0 ? lastEntry->practicedBpm
                                                  : (fallbackBpm > 0 ? fallbackBpm : m_targetBpm);
        emit paramsChanged();
        return;
    }

    const std::optional<ReminderCondition> condition = conditionForSong(m_songId);
    if (condition.has_value()) {
        m_startBar = condition->startBar;
        m_endBar = condition->endBar;
        m_targetBpm = condition->minBpm > 0 ? condition->minBpm
                                            : (fallbackBpm > 0 ? fallbackBpm : m_targetBpm);
        emit paramsChanged();
        return;
    }

    m_startBar = PracticeConstants::kDefaultStartBar;
    m_endBar = PracticeConstants::kDefaultEndBar;
    m_targetBpm = fallbackBpm > 0 ? fallbackBpm : PracticeConstants::kDefaultTargetBpm;
    emit paramsChanged();
}

std::optional<ReminderCondition>
PracticeTrackerController::conditionForSong(qlonglong songId) const {
    const QList<Reminder> reminders = m_reminderRepo.listForSong(songId);

    for (auto it = reminders.crbegin(); it != reminders.crend(); ++it) {
        if (!it->isActive) {
            continue;
        }
        const QList<ReminderCondition> conditions = m_conditionRepo.listForReminder(it->id);
        if (!conditions.isEmpty()) {
            return conditions.first();
        }
    }

    for (auto it = reminders.crbegin(); it != reminders.crend(); ++it) {
        const QList<ReminderCondition> conditions = m_conditionRepo.listForReminder(it->id);
        if (!conditions.isEmpty()) {
            return conditions.first();
        }
    }

    return std::nullopt;
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
