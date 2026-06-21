#ifndef IREMINDERCONDITIONREPOSITORY_H
#define IREMINDERCONDITIONREPOSITORY_H

#include <optional>

#include <QList>

#include "ReminderCondition.h"

class IReminderConditionRepository {
  public:
    virtual ~IReminderConditionRepository() = default;

    IReminderConditionRepository(const IReminderConditionRepository &) = delete;
    IReminderConditionRepository &operator=(const IReminderConditionRepository &) = delete;
    IReminderConditionRepository(IReminderConditionRepository &&) = delete;
    IReminderConditionRepository &operator=(IReminderConditionRepository &&) = delete;

    virtual std::optional<qlonglong> createCondition(const ReminderCondition &condition) = 0;
    virtual std::optional<ReminderCondition> getCondition(qlonglong id) = 0;
    virtual bool updateCondition(const ReminderCondition &condition) = 0;
    virtual bool deleteCondition(qlonglong id) = 0;
    virtual bool deleteConditionsForReminder(qlonglong reminderId) = 0;
    virtual QList<ReminderCondition> listForReminder(qlonglong reminderId) = 0;

  protected:
    IReminderConditionRepository() = default;
};

#endif // IREMINDERCONDITIONREPOSITORY_H
