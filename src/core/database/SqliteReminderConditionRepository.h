#ifndef SQLITEREMINDERCONDITIONREPOSITORY_H
#define SQLITEREMINDERCONDITIONREPOSITORY_H

#include "interfaces/IDatabaseConnection.h"
#include "interfaces/IReminderConditionRepository.h"

class SqliteReminderConditionRepository : public IReminderConditionRepository {
  public:
    explicit SqliteReminderConditionRepository(IDatabaseConnection &connection);

    [[nodiscard]] std::optional<qlonglong> createCondition(const ReminderCondition &condition) override;
    [[nodiscard]] std::optional<ReminderCondition> getCondition(qlonglong id) override;
    [[nodiscard]] bool updateCondition(const ReminderCondition &condition) override;
    [[nodiscard]] bool deleteCondition(qlonglong id) override;
    [[nodiscard]] bool deleteConditionsForReminder(qlonglong reminderId) override;
    [[nodiscard]] QList<ReminderCondition> listForReminder(qlonglong reminderId) override;

  private:
    static ReminderCondition conditionFromQuery(const class QSqlQuery &query);

    IDatabaseConnection &m_connection;
};

#endif // SQLITEREMINDERCONDITIONREPOSITORY_H
