#ifndef SQLITEREMINDERREPOSITORY_H
#define SQLITEREMINDERREPOSITORY_H

#include "interfaces/IDatabaseConnection.h"
#include "interfaces/IReminderRepository.h"

class SqliteReminderRepository : public IReminderRepository {
  public:
    explicit SqliteReminderRepository(IDatabaseConnection &connection);

    [[nodiscard]] std::optional<qlonglong> createReminder(const Reminder &reminder) override;
    [[nodiscard]] std::optional<Reminder> getReminder(qlonglong id) override;
    [[nodiscard]] bool updateReminder(const Reminder &reminder) override;
    [[nodiscard]] bool deleteReminder(qlonglong id) override;
    [[nodiscard]] QList<Reminder> listForSong(qlonglong songId) override;
    [[nodiscard]] QList<Reminder> listForPracticeAsset(qlonglong practiceAssetId) override;
    [[nodiscard]] QList<Reminder> listForDate(const QDate &date) override;
    [[nodiscard]] QList<ReminderDayEntry> listForDateWithSong(const QDate &date) override;
    [[nodiscard]] QList<ReminderDayEntry> listAllActiveWithSong() override;
    [[nodiscard]] QList<Reminder> listActiveInRange(const QDate &from, const QDate &to) override;

  private:
    static Reminder reminderFromQuery(const class QSqlQuery &query);
    static ReminderDayEntry entryFromQuery(const class QSqlQuery &query);
    QList<Reminder> listAllActive();
    QList<ReminderDayEntry> queryCandidatesForDate(const QDate &date);

    IDatabaseConnection &m_connection;
};

#endif // SQLITEREMINDERREPOSITORY_H
