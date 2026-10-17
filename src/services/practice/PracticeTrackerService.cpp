#include "PracticeTrackerService.h"

#include "JournalEntry.h"
#include "PracticeConstants.h"
#include "interfaces/IPracticeJournalRepository.h"

#include <QElapsedTimer>
#include <QtMath>

PracticeTrackerService::PracticeTrackerService(IPracticeJournalRepository &journalRepo)
    : m_journalRepo(journalRepo) {}

int PracticeTrackerService::computeStreak(int totalReps, int successfulReps) {
    if (totalReps <= 0 || successfulReps <= 0) {
        return 0;
    }

    const int cappedSuccesses = qMin(successfulReps, totalReps);
    return cappedSuccesses >= PracticeConstants::kMaxSuccessfulStreak
               ? PracticeConstants::kMaxSuccessfulStreak
               : cappedSuccesses;
}

int PracticeTrackerService::durationMinutesFromSeconds(int durationSeconds) {
    if (durationSeconds <= 0) {
        return 0;
    }
    return qMax(PracticeConstants::kMinimumElapsedSeconds,
                static_cast<int>(qCeil(durationSeconds /
                                       static_cast<double>(PracticeConstants::kSecondsPerMinute))));
}

bool PracticeTrackerService::isTimerRunning() const { return m_timerRunning; }

int PracticeTrackerService::elapsedSeconds() const {
    if (!m_timerRunning) {
        return 0;
    }
    return static_cast<int>(m_elapsedTimer.elapsed() / PracticeConstants::kMillisecondsPerSecond);
}

bool PracticeTrackerService::canSave(const PracticeTrackerParams &params) const {
    qDebug() << "PracticeTracker canSave params: \nDate: " << params.practiceDate
             << "\n assetId: " << params.assetId << "\n StartBar: " << params.startBar
             << "\n EndBar: " << params.endBar;
    return params.assetId > 0 && params.endBar >= params.startBar && params.startBar > 0;
}

bool PracticeTrackerService::startTimer() {
    if (m_timerRunning) {
        return false;
    }

    m_elapsedTimer.start();
    m_timerRunning = true;
    return true;
}

std::optional<qlonglong> PracticeTrackerService::stopAndSave(const PracticeTrackerParams &params,
                                                             int elapsedSecondsOverride) {
    if (!m_timerRunning || !canSave(params)) {
        return std::nullopt;
    }

    int elapsedSeconds = 0;
    if (elapsedSecondsOverride >= 0) {
        elapsedSeconds = elapsedSecondsOverride;
    } else {
        elapsedSeconds =
            static_cast<int>(m_elapsedTimer.elapsed() / PracticeConstants::kMillisecondsPerSecond);
        if (elapsedSeconds < PracticeConstants::kMinimumElapsedSeconds) {
            elapsedSeconds = PracticeConstants::kMinimumElapsedSeconds;
        }
    }

    m_timerRunning = false;

    JournalEntry entry;
    entry.userId = params.userId > 0 ? params.userId : 1;
    entry.assetId = params.assetId;
    entry.practiceDate =
        params.practiceDate.isValid() ? params.practiceDate : QDateTime::currentDateTime();
    entry.startBar = params.startBar;
    entry.endBar = params.endBar;
    entry.practicedBpm = params.targetBpm;
    entry.totalReps = params.totalReps;
    entry.successfulStreaks = computeStreak(params.totalReps, params.successfulReps);
    entry.durationSeconds = elapsedSeconds;

    return m_journalRepo.createEntry(entry);
}

void PracticeTrackerService::cancelTimer() { m_timerRunning = false; }
