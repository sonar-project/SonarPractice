#ifndef IREMINDERCOMPLETIONREPOSITORY_H
#define IREMINDERCOMPLETIONREPOSITORY_H

#include <QDate>
#include <optional>

class IReminderCompletionRepository {
  public:
    virtual ~IReminderCompletionRepository() = default;

    IReminderCompletionRepository(const IReminderCompletionRepository &) = delete;
    IReminderCompletionRepository &operator=(const IReminderCompletionRepository &) = delete;
    IReminderCompletionRepository(IReminderCompletionRepository &&) = delete;
    IReminderCompletionRepository &operator=(IReminderCompletionRepository &&) = delete;

    virtual bool isAccepted(qlonglong reminderId, const QDate &date) const = 0;
    virtual bool setAccepted(qlonglong reminderId, const QDate &date, bool accepted) = 0;
    virtual bool clearOverride(qlonglong reminderId, const QDate &date) = 0;

  protected:
    IReminderCompletionRepository() = default;
};

#endif // IREMINDERCOMPLETIONREPOSITORY_H
