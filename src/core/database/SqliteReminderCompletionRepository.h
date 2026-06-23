#ifndef SQLITEREMINDERCOMPLETIONREPOSITORY_H
#define SQLITEREMINDERCOMPLETIONREPOSITORY_H

#include "interfaces/IDatabaseConnection.h"
#include "interfaces/IReminderCompletionRepository.h"

class SqliteReminderCompletionRepository final : public IReminderCompletionRepository {
  public:
    explicit SqliteReminderCompletionRepository(IDatabaseConnection &connection);

    [[nodiscard]] bool isAccepted(qlonglong reminderId, const QDate &date) const override;
    bool setAccepted(qlonglong reminderId, const QDate &date, bool accepted) override;
    bool clearOverride(qlonglong reminderId, const QDate &date) override;

  private:
    IDatabaseConnection &m_connection;
};

#endif // SQLITEREMINDERCOMPLETIONREPOSITORY_H
