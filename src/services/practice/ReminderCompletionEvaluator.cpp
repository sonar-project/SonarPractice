#include "ReminderCompletionEvaluator.h"

#include "PracticeConstants.h"
#include "PracticeTrackerService.h"

#include <QCoreApplication>

bool ReminderCompletionEvaluator::conditionHasRequirements(const ReminderCondition &condition) {
    return condition.startBar > 0 || condition.endBar > 0 || condition.minBpm > 0 ||
           condition.minMinutes > 0;
}

bool ReminderCompletionEvaluator::entryMeetsBarAndBpm(const JournalEntry &entry,
                                                      const ReminderCondition &condition) {
    if (condition.startBar > 0 && entry.startBar > condition.startBar) {
        return false;
    }
    if (condition.endBar > 0 && entry.endBar < condition.endBar) {
        return false;
    }
    if (condition.minBpm > 0 && entry.practicedBpm < condition.minBpm) {
        return false;
    }
    return true;
}

int ReminderCompletionEvaluator::totalDurationMinutes(const QList<JournalEntry> &entries) {
    int totalSeconds = 0;
    for (const JournalEntry &entry : entries) {
        totalSeconds += entry.durationSeconds;
    }
    return PracticeTrackerService::durationMinutesFromSeconds(totalSeconds);
}

QString ReminderCompletionEvaluator::buildPartialDetail(const QList<JournalEntry> &entries,
                                                        const ReminderCondition &condition) {
    QStringList parts;

    if (condition.minMinutes > 0) {
        const int practicedMinutes = totalDurationMinutes(entries);
        if (practicedMinutes < condition.minMinutes) {
            parts.append(QCoreApplication::translate(
                "ReminderCompletionEvaluator", "%1 min practiced (%2 required)")
                             .arg(practicedMinutes)
                             .arg(condition.minMinutes));
        }
    }

    if (condition.minBpm > 0) {
        int bestBpm = 0;
        for (const JournalEntry &entry : entries) {
            bestBpm = qMax(bestBpm, entry.practicedBpm);
        }
        if (bestBpm < condition.minBpm) {
            parts.append(QCoreApplication::translate(
                "ReminderCompletionEvaluator", "%1 BPM practiced (%2 required)")
                             .arg(bestBpm)
                             .arg(condition.minBpm));
        }
    }

    if (condition.startBar > 0 || condition.endBar > 0) {
        bool barsMet = false;
        for (const JournalEntry &entry : entries) {
            if (entryMeetsBarAndBpm(entry, condition)) {
                barsMet = true;
                break;
            }
        }
        if (!barsMet) {
            parts.append(QCoreApplication::translate("ReminderCompletionEvaluator",
                                                     "Bar range not met"));
        }
    }

    return parts.join(QStringLiteral(" · "));
}

ReminderCompletionEvaluator::Result
ReminderCompletionEvaluator::evaluate(const QList<JournalEntry> &entries,
                                      const ReminderCondition &condition, bool manuallyAccepted) {
    Result result;
    result.hasJournalEntry = !entries.isEmpty();
    result.hasCondition = conditionHasRequirements(condition);

    if (manuallyAccepted && result.hasJournalEntry) {
        result.status = Status::ManuallyCompleted;
        result.detailMessage =
            QCoreApplication::translate("ReminderCompletionEvaluator", "Marked as done by you");
        return result;
    }

    if (!result.hasJournalEntry) {
        result.status = Status::NotPracticed;
        return result;
    }

    if (!result.hasCondition) {
        result.status = Status::Completed;
        result.detailMessage =
            QCoreApplication::translate("ReminderCompletionEvaluator", "Practiced");
        return result;
    }

    bool barsAndBpmMet = false;
    for (const JournalEntry &entry : entries) {
        if (entryMeetsBarAndBpm(entry, condition)) {
            barsAndBpmMet = true;
            break;
        }
    }

    const int practicedMinutes = totalDurationMinutes(entries);
    const bool durationMet =
        condition.minMinutes <= 0 || practicedMinutes >= condition.minMinutes;

    if (barsAndBpmMet && durationMet) {
        result.status = Status::Completed;
        result.detailMessage =
            QCoreApplication::translate("ReminderCompletionEvaluator", "Condition met");
        return result;
    }

    result.status = Status::Partial;
    result.detailMessage = buildPartialDetail(entries, condition);
    return result;
}
