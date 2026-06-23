#ifndef REMINDERCOMPLETIONEVALUATOR_H
#define REMINDERCOMPLETIONEVALUATOR_H

#include "JournalEntry.h"
#include "ReminderCondition.h"

#include <QList>
#include <QString>

/**
 * @brief Evaluates whether journal entries satisfy reminder practice conditions.
 */
class ReminderCompletionEvaluator {
  public:
    enum class Status {
        NotPracticed,
        Completed,
        Partial,
        ManuallyCompleted,
    };

    struct Result {
        Status status{Status::NotPracticed};
        bool hasJournalEntry{false};
        bool hasCondition{false};
        QString detailMessage;
    };

    [[nodiscard]] static Result evaluate(const QList<JournalEntry> &entries,
                                         const ReminderCondition &condition,
                                         bool manuallyAccepted);

  private:
    [[nodiscard]] static bool conditionHasRequirements(const ReminderCondition &condition);
    [[nodiscard]] static bool entryMeetsBarAndBpm(const JournalEntry &entry,
                                                  const ReminderCondition &condition);
    [[nodiscard]] static int totalDurationMinutes(const QList<JournalEntry> &entries);
    [[nodiscard]] static QString buildPartialDetail(const QList<JournalEntry> &entries,
                                                    const ReminderCondition &condition);
};

#endif // REMINDERCOMPLETIONEVALUATOR_H
